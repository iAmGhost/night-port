"""What the data someone brought actually is: the container, the names, and the disc behind them.

The Python mirror of `wasm/runtime/byod.js`.  The browser port and this tool now answer the same
question by the same rules -- *what did this visitor hand me* -- and the rules are the interesting part,
because none of them is a name:

  * **the container is either a zip or a set of files**.  A pair picked together behaves like a zip whose
    members carry their own byte source: a zip member's bytes begin past its local header, a plain file's
    at byte 0.  Nothing downstream is allowed to care which.
  * **a CUE's own `FILE` line is the only authority on the image's name**.  `Eojjeonji Disc.bin` is as
    good a name for the disc as `TG_NIGHT.IMG`; `TG_NIGHT.*` only breaks a tie, and `SOUND.RES`/`MUSIC.RES`
    are aliases in the rule rather than the rule -- a fan repack names its image after its installer.
  * **fan rip or disc is settled by the image's bytes, not its names**.  A fan rip begins at the disc's
    track 02, so its first sector is a frame of music and carries no ISO9660 volume; and the same look
    says whether `NIGHT.RES` is waiting inside.  A fan repack copies the disc's cue lines for line, so
    neither a file name nor a cue's text can tell the two apart.
  * **an image with no cue gets this port's own table of contents**, and only if its size says it is this
    disc (+/- 2%).  Guessing an image's geometry is worse than asking for its cue.

The corrections a fan `MUSIC.RES` needs (`MUSIC_SHIFT`, `MUSIC_EXTRA`) are applied only to the bytes they
were measured against -- the CRC-32 guard, docs/464.  Any other copy is sliced by its own cue as written.

A `NIGHT.RES` alone is a complete answer: the game plays, silently.  `plan` comes back empty and there is
nothing for the audio part to cut.

    python3 scripts/extract_disc.py --data TG_NIGHT.CUE TG_NIGHT.IMG   # a pair, any names
    python3 scripts/extract_disc.py --data rip.zip                     # any of the shapes inside it
    python3 scripts/extract_disc.py --data NIGHT.RES                   # the game, no music
"""
from __future__ import annotations

import io
import re
import struct
import zlib
from pathlib import Path

from .disc import SECTOR, UD_LEN, DataTrack, IsoStream, decode_container_dir, msf_to_frames

RES_CRC = 0xF578A854          # NIGHT.RES as this port was verified against (docs/455)
MUSIC_CRC = 0x36C386DE        # the fan MUSIC.RES the offsets below were measured on (docs/464)
MUSIC_SHIFT = -560            # its audio sits 560 bytes before where its own cue says
MUSIC_EXTRA = {8: 4455768, 9: 4455768}    # and tracks 08/09 sit 25.26 s further out again
DISC_SECTORS = 269303         # this disc's own length, from the port's table of contents
CACHE_NAME = ".night-cache"   # where a deflated image is staged once (the port's OPFS staging, in a dir)

# The disc this port's whole analysis rests on, as sector numbers measured from the start of the image
# (docs/96's music points are LBAs, so the game asks for these).  Copied from byod.js's DISC_TOC, which is
# copied from cd_iso.c's compiled-in table: a sector number and a track order are the disc's table of
# contents, not its content, so a file that brings none can still be played.
DISC_TOC = {
    1:  ("MODE1/2352", 0, 0),
    2:  ("AUDIO", 37112, 150),        # 08:14:62 -- the 2-second gap is not in the image
    3:  ("AUDIO", 40265, 0),
    4:  ("AUDIO", 59904, 0),
    5:  ("AUDIO", 77031, 0),
    6:  ("AUDIO", 96388, 0),
    7:  ("AUDIO", 114678, 0),
    8:  ("AUDIO", 136158, 0),
    9:  ("AUDIO", 160997, 0),
    10: ("AUDIO", 184970, 0),
    11: ("AUDIO", 212667, 0),
    12: ("AUDIO", 222996, 0),
    13: ("AUDIO", 240852, 0),
    14: ("AUDIO", 258720, 0),
    15: ("AUDIO", 268471, 0),
}


def msf(sector: int) -> str:
    return "%02d:%02d:%02d" % (sector // 4500, (sector // 75) % 60, sector % 75)


def disc_cue() -> str:
    """This port's own table of contents, wearing a CUE sheet -- what a bare image's geometry becomes."""
    t = 'FILE "TG_NIGHT.IMG" BINARY\r\n'
    for n in sorted(DISC_TOC):
        mode, index, pregap = DISC_TOC[n]
        t += "  TRACK %02d %s\r\n" % (n, mode)
        if pregap:
            t += "    PREGAP %s\r\n" % msf(pregap)
        t += "    INDEX 01 %s\r\n" % msf(index)
    return t


# ---------------------------------------------------------------- the members of a container


class Slice(io.RawIOBase):
    """A bounded window into an open file: seek/read in [0, size), whose byte 0 sits at absolute `off`."""

    def __init__(self, fh, off: int, size: int, close=None):
        self.fh, self.off, self.size, self.pos, self.closer = fh, off, size, 0, close

    def readable(self):
        return True

    def seekable(self):
        return True

    def seek(self, pos, whence=0):
        if whence == 1:
            pos = self.pos + pos
        elif whence == 2:
            pos = self.size + pos
        self.pos = max(0, min(self.size, int(pos)))
        return self.pos

    def read(self, n=None):
        n = self.size - self.pos if n is None else min(n, self.size - self.pos)
        if n <= 0:
            return b""
        self.fh.seek(self.off + self.pos)
        got = self.fh.read(n)
        self.pos += len(got)
        return got

    def close(self):
        if self.closer:
            self.closer()
        super().close()



class Member:
    """One file inside a container: a name, a length, a CRC if the container knows one, and readers.

    `stream()` is a forward reader whose byte 0 is this member's first byte, whatever held it -- past a
    zip's local header, or a file's own start.  `reader()` adds seeks, which a deflated member cannot do
    in place: it is staged once instead, and `reader()`/`stage()` both give the staged path.
    """

    def __init__(self, name, size, crc=None, method=None, seekable=True):
        self.name, self.size, self.crc, self.method = name, size, crc, method
        self.base = name.replace("\\", "/").rstrip("/").split("/")[-1]
        self.seekable = seekable
        self.staged = None
        self.crc_measured = False       # set by measure_crc(): a CRC the tool worked out, not was told

    def stream(self):
        raise NotImplementedError

    def whole(self) -> bytes:
        with self.stream() as f:
            return f.read()

    def measure_crc(self) -> int:
        """The member's own CRC-32, computed -- for the rules a container would have declared."""
        if self.crc is not None:
            return self.crc
        crc = 0
        with self.stream() as f:
            for block in iter(lambda: f.read(1 << 20), b""):
                crc = zlib.crc32(block, crc)
        self.crc, self.crc_measured = crc & 0xFFFFFFFF, True
        return self.crc

    def stage(self, cache_dir: Path) -> Path:
        """A path a random reader may seek in -- staging once if the member cannot be seeked in place.

        This is the browser port's `memberToOpfs`: deflate cannot be seeked, so a compressed image is
        unpacked once into a cache named by its CRC, so two different files never share a staged copy and a
        stale one never answers for a new one.
        """
        raise NotImplementedError

    def reader(self, cache_dir: Path):
        """(handle, closer) whose byte 0 is this member's first byte, seekable.  Closer may be None."""
        p = self.stage(cache_dir)
        fh = p.open("rb")
        return fh, fh.close

    def label(self) -> str:
        m = {None: "file", 0: "stored", 8: "deflate"}.get(self.method, str(self.method))
        if self.crc is None:
            return m
        return f"{m}, CRC-32 {self.crc:08x}" + (" (measured here)" if self.crc_measured else "")


class FileMember(Member):
    """A file on disk -- including the files a file chooser handed over, which are the same thing here."""

    def __init__(self, path: Path):
        p = Path(path).expanduser()
        super().__init__(p.name, p.stat().st_size)
        # The name stays the one the person typed or the picker showed.  Resolving first would turn a
        # `Eojjeonji Disc.bin` that happens to be a symlink (or a hard link, or a mount point) into whatever
        # it points at, and the tool would then report a file the visitor never named.
        self.path = p

    def stream(self):
        return self.path.open("rb")

    def stage(self, cache_dir: Path) -> Path:
        return self.path


class DeflateStream:
    """A deflate member's bytes, decoded forward from the archive: raw off the file, inflated as it is read.

    Deliberately not `zipfile`.  `zipfile` validates the CRC-32 the container declares and raises mid-read
    when the bytes disagree with it, which would stop a game over a number the game never asks about -- and
    declared CRCs are exactly what a repacking tool gets wrong often enough to matter.  Reading the member
    from the offset the central directory leads to, with zlib, gives the same bytes with no veto.
    """

    def __init__(self, path: Path, data_off: int, csize: int):
        self._fh = path.open("rb")
        self._fh.seek(data_off)
        self._d = zlib.decompressobj(-15)             # -15: the raw deflate stream a zip member holds
        self._left, self._buf = csize, b""

    def readable(self):
        return True

    def seekable(self):
        return False

    def read(self, n=None):
        want = -1 if n is None else n
        while not (self._buf and (want < 0 or len(self._buf) >= want)):
            if self._left <= 0 or self._d.eof:
                break
            chunk = self._fh.read(min(1 << 20, self._left))
            if not chunk:
                break
            self._left -= len(chunk)
            self._buf += self._d.decompress(chunk)
        if want < 0 or want >= len(self._buf):
            out, self._buf = self._buf, b""
        else:
            out, self._buf = self._buf[:want], self._buf[want:]
        return out

    def close(self):
        self._fh.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


class ZipMember(Member):
    """One member of a zip, indexed by hand.

    The index is parsed here rather than through `zipfile` because the number needed is the member's *data
    offset*: with it a stored member is read in place with ordinary seeks, so a 633 MB image is never
    copied and a 546 MB one is copied exactly once.  `zipfile` keeps that number to itself.
    """

    def __init__(self, path: Path, e: dict):
        super().__init__(e["name"], e["size"], crc=e["crc"], method=e["method"], seekable=e["method"] == 0)
        self.path, self.data_off, self.csize = path, e["data_off"], e["csize"]

    def stream(self):
        if self.seekable:
            fh = self.path.open("rb")
            return Slice(fh, self.data_off, self.size, close=fh.close)
        return DeflateStream(self.path, self.data_off, self.csize)

    def stage(self, cache_dir: Path) -> Path:
        if self.seekable:
            # Still no ordinary path: hand back a seekable view by way of a staged file-like, which is what
            # `reader()` uses.  The path here only exists for callers that insist on one.
            return self.path
        if self.staged and self.staged.is_file() and self.staged.stat().st_size == self.size:
            return self.staged
        dst = Path(cache_dir) / ("night-%s-%s.img" % (self.base.lower(), format(self.crc or 0, "x")))
        dst.parent.mkdir(parents=True, exist_ok=True)
        tmp = dst.with_suffix(dst.suffix + ".part")
        with self.stream() as src, tmp.open("wb") as g:
            for block in iter(lambda: src.read(1 << 20), b""):
                g.write(block)
        # The container's CRC is not a condition of writing this file.  A repack whose archive disagrees with
        # its own bytes is still somebody's game data, and the length is what actually gets enforced: a
        # reader that comes up short raises `short read`, which is a failure with a place in it.
        tmp.replace(dst)
        self.staged = dst
        return dst

    def reader(self, cache_dir: Path):
        if self.seekable:
            fh = self.path.open("rb")
            return Slice(fh, self.data_off, self.size), fh.close
        p = self.stage(cache_dir)
        fh = p.open("rb")
        return fh, fh.close


def zip_entries(path: Path):
    """The central directory: name, size, CRC-32, method, and the offset where a member's data begins."""
    path = Path(path)
    fh = path.open("rb")
    size = path.stat().st_size
    back = min(66000, size)
    fh.seek(size - back)
    buf = fh.read(back)
    at = buf.rfind(b"PK\x05\x06")
    if at < 0:
        fh.close()
        raise SystemExit(f"{path.name}: no end-of-central-directory -- not a zip, or a zip64 this tool "
                         f"cannot index (unzip it and pass the files)")
    count = struct.unpack_from("<H", buf, at + 10)[0]
    cd_size = struct.unpack_from("<I", buf, at + 12)[0]
    cd_off = struct.unpack_from("<I", buf, at + 16)[0]
    if count == 0xFFFF or cd_off == 0xFFFFFFFF:        # zip64: the real numbers are in its own records
        loc = buf.rfind(b"PK\x06\x07", 0, at)
        if loc < 0:
            fh.close()
            raise SystemExit(f"{path.name}: a zip64 without its directory record -- unzip it and pass "
                             f"the files")
        z64 = struct.unpack_from("<Q", buf, loc + 8)[0]
        fh.seek(z64)
        rec = fh.read(64)
        count = struct.unpack_from("<Q", rec, 32)[0]
        cd_size = struct.unpack_from("<Q", rec, 40)[0]
        cd_off = struct.unpack_from("<Q", rec, 48)[0]
    fh.seek(cd_off)
    cd = fh.read(cd_size)
    out, p = [], 0
    while p + 46 <= len(cd) and cd[p:p + 4] == b"PK\x01\x02":
        # method at +10, then the two modification fields the parser must step over, then CRC-32, then the
        # compressed and uncompressed lengths: reading the CRC from +12 gets back the file's date, which is
        # a plausible number, silently disables the fan corrections, and makes staging fail on a zip that is
        # not damaged.
        method = struct.unpack_from("<H", cd, p + 10)[0]
        crc, csize, usize = struct.unpack_from("<III", cd, p + 16)
        nlen, elen, clen = struct.unpack_from("<HHH", cd, p + 28)
        name = cd[p + 46:p + 46 + nlen].decode("utf-8", "replace")
        hoff = struct.unpack_from("<I", cd, p + 42)[0]
        p += 46 + nlen + elen + clen
        if name.endswith("/"):
            continue                                     # a directory entry: there is nothing to read
        fh.seek(hoff)
        loc = fh.read(30)
        if loc[:4] != b"PK\x03\x04":
            fh.close()
            raise SystemExit(f"{path.name}: {name}'s local header is not one")
        ln, le = struct.unpack_from("<HH", loc, 26)
        out.append(dict(name=name, size=usize or csize, crc=crc, method=method, csize=csize,
                        data_off=hoff + 30 + ln + le))
    fh.close()
    return out


# ---------------------------------------------------------------- what the names mean

def is_cue(m: Member) -> bool:
    b = m.base.upper()
    return b.endswith(".CUE") or b == "SOUND.RES"


def is_img(m: Member) -> bool:
    b = m.base.upper()
    if re.fullmatch(r"TRACK\d\d\.BIN", b):
        return False        # already-cut audio, not an image: an extracted/ tree is a container too
    return b.endswith(".IMG") or b.endswith(".BIN") or b.endswith(".ISO") or b == "MUSIC.RES"


def is_res(m: Member) -> bool:
    return m.base.upper() == "NIGHT.RES"


def members_of(paths, notes):
    """The container's members, from whatever was named: one zip, one or more directories, or the files."""
    paths = [Path(p).expanduser().resolve() for p in paths]
    zips = [p for p in paths if p.suffix.lower() == ".zip" and p.is_file()]
    if zips:
        if len(zips) > 1:
            notes.append(f"{len(zips)} zips and a zip cannot be paired with another -- using {zips[0].name}")
        others = [p.name for p in paths if p != zips[0]]
        if others:
            notes.append(f"{len(others)} other path(s) ignored: {zips[0].name} is the container")
        return "zip", [zips[0]], [ZipMember(zips[0], e) for e in zip_entries(zips[0])]
    out = []
    for p in paths:
        if p.is_dir():
            out += [FileMember(q) for q in sorted(x for x in p.iterdir() if x.is_file())]
        elif p.is_file():
            out.append(FileMember(p))
        else:
            notes.append(f"{p}: not there")
    return "files", paths, out


def cue_file_line(text: str):
    """The image a cue says it describes -- `TG_NIGHT.IMG`, `MUSIC.RES`, whatever it is called.

    The name is taken from inside the quotes, whole: splitting the line on whitespace turns
    `FILE "Eojjeonji Disc.bin" BINARY` into `Eojjeonji`, which is a word out of a file name and no
    file at all -- the same reason byod.js's cueParse matches the quoted group.
    """
    for line in text.splitlines():
        line = line.strip()
        m = re.match(r'FILE\s+"([^"]+)"', line, re.I)
        if not m:
            m = re.match(r"FILE\s+(\S+)", line, re.I)
            if not m:
                continue
        return m.group(1).replace("\\", "/").split("/")[-1].upper()
    return None


def choose(members, notes):
    """(res, cue, img, how) by the rules in the module docstring -- never by our own names alone.

    `TG_NIGHT.*` only breaks a tie; a cue that names an image wins over any pick; two of something and no
    rule to split them is said out loud rather than resolved by `sorted()`.
    """
    res = next((m for m in members if is_res(m)), None)
    cues = [m for m in members if is_cue(m)]
    imgs = [m for m in members if is_img(m)]
    how = {}
    cue = next((m for m in cues if m.base.upper() == "TG_NIGHT.CUE"), None)
    if cue:
        how["cue"] = "named TG_NIGHT.CUE"
    elif len(cues) == 1:
        cue, how["cue"] = cues[0], f"the only cue here ({cues[0].base})"
    elif len(cues) > 1:
        notes.append(f"{len(cues)} cue sheets and none named TG_NIGHT.CUE -- cannot choose; pass one pair")
    img = next((m for m in imgs if m.base.upper() == "TG_NIGHT.IMG"), None)
    if img:
        how["img"] = "named TG_NIGHT.IMG"
    elif len(imgs) == 1:
        img, how["img"] = imgs[0], f"the only image here ({imgs[0].base})"
    elif len(imgs) > 1:
        img = next((m for m in imgs if m.base.upper() == "MUSIC.RES"), None)
        if img:
            how["img"] = "named MUSIC.RES (a fan repack's image)"
        else:
            notes.append(f"{len(imgs)} images and none named TG_NIGHT.IMG -- cannot choose; pass one pair")
    return res, cue, img, how


# ---------------------------------------------------------------- geometry, and what an image turns out to be

def cue_tracks(text: str):
    """[{track, mode, index, pregap}] in track order, INDEX 01 only -- those are the only ones read."""
    out, cur = [], None
    for raw in text.splitlines():
        line = raw.strip()
        m = re.match(r"TRACK\s+(\d+)\s+(\w+)", line, re.I)
        if m:
            if cur:
                out.append(cur)
            cur = dict(track=int(m.group(1)), mode=m.group(2).upper(), index=None, pregap=0)
            continue
        m = re.match(r"PREGAP\s+([\d:]+)", line, re.I)
        if m and cur:
            cur["pregap"] = msf_to_frames(m.group(1))
            continue
        m = re.match(r"INDEX\s+01\s+([\d:]+)", line, re.I)
        if m and cur and cur["index"] is None:
            cur["index"] = msf_to_frames(m.group(1))
    if cur:
        out.append(cur)
    return sorted((t for t in out if t["index"] is not None), key=lambda t: t["track"])


def track_lengths(tracks, end_sector):
    """Each track's sector count: the next INDEX 01 minus its own, the end of the image for the last one."""
    for i, t in enumerate(tracks):
        nxt = next(iter(tracks[i + 1:]), None)
        t["sectors"] = (nxt["index"] - t["index"]) if nxt else max(0, end_sector - t["index"])
    return tracks


def data_track(tracks, img_sectors):
    """The image's data track as its cue describes it: the one non-audio extent wider than 32 sectors."""
    return next((t for t in track_lengths(tracks, img_sectors)
                 if t["mode"] != "AUDIO" and t["sectors"] > 32), None)


def plan_tracks(img: Member, cue_text: str, fan: bool):
    """Where each audio track's bytes are in the image: {tracks: [{track, sectors, off, len, lba}], ...}.

    Offsets are into the image and not into the zip, which is what keeps this function testable with no
    container in sight.  `fan` comes from the volume probe and not from a name, and the corrections apply
    only to the CRC they were measured against -- taking a fan cue at its word would start track 08 with 25
    seconds of foreign music and cut its ending off.

    A track's disc LBA is its position in the file plus every PREGAP before it, the split cd_iso.c and
    repack.py keep apart; folding the pregap into one number and subtracting it puts the data track at image
    sector -150.
    """
    tracks = track_lengths(cue_tracks(cue_text), img.size // SECTOR)
    fan = bool(fan)
    corrected = fan and img.crc == MUSIC_CRC
    shift = MUSIC_SHIFT if corrected else 0
    extra = MUSIC_EXTRA if fan else {}
    pregap, out = 0, []
    for t in tracks:
        pregap += t["pregap"]
        if t["mode"] != "AUDIO" or t["sectors"] <= 0:
            continue
        off = t["index"] * SECTOR + shift + extra.get(t["track"], 0)
        if off < 0:
            continue
        out.append(dict(track=t["track"], sectors=t["sectors"], off=off, len=t["sectors"] * SECTOR,
                        lba=t["index"] + pregap))
    # A shifted start leaves the tail of a track standing on the next track's first bytes: the fan image
    # puts 25.26 s this disc does not have before tracks 08 and 09, and drops 25.26 s between 09 and 10, so
    # the disc's own length for track 09 ends 1895 sectors past where the image says track 10 begins
    # (measured: docs/466).  Play that and track 09 finishes with the first 25 s of track 10 -- a different
    # tune, at the loudest moment of the track.  So the last slice of a shifted track stops where the next
    # one starts, and the image's end bounds the last track of all.  Truncation is the failure that is
    # merely wrong, which is the better of the two available.
    limit = img.size
    for tr in reversed(out):
        if tr["off"] + tr["len"] > limit:
            tr["len"] = max(0, limit - tr["off"]) // SECTOR * SECTOR
            tr["sectors"] = tr["len"] // SECTOR
            tr["clipped"] = True
        limit = tr["off"]
    clipped = sum(1 for tr in out if tr.get("clipped"))
    return dict(fan=fan, corrected=corrected, tracks=out, cue_as_written=not fan or not corrected,
                clipped=clipped)


def probe_image(img: Member, cue_text: str, cache_dir: Path):
    """(volume, res, data_track) -- is there an ISO9660 volume where the cue says the data track is, and if
    so where NIGHT.RES sits inside the installer's payload.  A dozen frames, asked before any work starts,
    and it answers both questions at once: disc or fan rip, and is the game in here."""
    d = data_track(cue_tracks(cue_text), img.size // SECTOR)
    if d is None:
        return None, None, None
    fh, closer = img.reader(cache_dir)
    try:
        t = DataTrack(fh, d["index"], d["sectors"])
        try:
            t.root_record()
        except ValueError:
            return None, None, d                      # no volume: a fan rip begins at the disc's track 02
        hit = t.find("NIGHT/NIGHT.DAT") or t.find("NIGHT.DAT")
        if not hit:
            return True, None, d
        lba, fsize = hit
        dat = IsoStream(t, lba, fsize)
        count = int.from_bytes(dat.read(4), "little")
        if count == 0 or count > 64:
            return True, None, d
        # Back to byte 0 first: the directory block starts with the same count dword that was just read, so
        # a stream left at offset 4 hands decode_container_dir someone else's dword -- which is the class of
        # bug that gives back a plausible count and no entries at all.
        dat.seek(0)
        ents = decode_container_dir(dat.read(4 + count * 20))
        if not ents or ents[0][1] != 4 + count * 20:      # the index's own consistency test
            return True, None, d
        e = next((x for x in ents if x[0].upper() == "NIGHT.RES"), None)
        if not e:
            return True, None, d
        return True, dict(lba=lba, first=d["index"], track_sectors=d["sectors"], name="NIGHT/NIGHT.DAT",
                          count=count, size=fsize,
                          res_name=e[0], off=e[1], res_size=e[2],
                          index=[dict(name=x[0], off=x[1], size=x[2]) for x in ents]), d
    finally:
        if closer:
            closer()


def res_stream(img: Member, found, cache_dir: Path):
    """(stream, closer) over NIGHT.RES as it sits inside the payload -- byte 0 is the container's first byte.

    The stream is based at the payload file's own extent, because its container's offsets are relative to
    that: basing them on the volume's first sector is the one mistake that gives back the right size and the
    wrong bytes (docs/455).
    """
    fh, closer = img.reader(cache_dir)
    # Bounded by the payload file, not by the container entry being read: an entry's offset runs *inside* the
    # payload, and bounding the stream by the entry's own length stops the read exactly `off` bytes short --
    # the right size for the first blob and a short read for every one after it.
    return IsoStream(DataTrack(fh, found["first"], found["track_sectors"]), found["lba"], found["size"]), \
        closer


# ---------------------------------------------------------------- the whole thing


class Source:
    """What was handed over, resolved to the three things the port needs: resources, an image, a TOC.

    Built once by `open_source()`; everything that writes a file reads this instead of re-discovering.
    """

    def __init__(self, kind, paths, members, notes):
        self.kind, self.paths, self.members, self.notes = kind, paths, members, notes
        self.res = self.cue = self.img = None
        self.cue_text = None
        self.own_toc = self.fan = self.corrected = False
        self.volume = None
        self.res_in_image = None
        self.plan = []
        self.data = None
        self.how = {}
        self.cache_dir = Path(".")

    @property
    def has_res(self):
        return self.res is not None or self.res_in_image is not None

    @property
    def res_size(self):
        return self.res.size if self.res is not None else (self.res_in_image or {}).get("res_size", 0)

    @property
    def cue_out(self):
        """The CUE worth writing into the output tree: the disc's own numbers, not a fan rip's.

        The game asks in the disc's LBAs, so a fan cue -- whose numbers describe the fan image -- and a bare
        image both get this port's table of contents, which is also what the native port and the Pocket's
        generated disc_table.h want to be handed (docs/465).
        """
        return None if (self.fan or self.own_toc or self.cue_text is None) else self.cue_text

    def describe(self):
        for n in self.notes:
            print(f"note    {n}")
        print(f"data    {self.kind} of {len(self.members)} member(s): "
              + ", ".join(p.name for p in self.paths))
        for m in self.members:
            mark = "res" if is_res(m) else ("cue" if is_cue(m) else ("img" if is_img(m) else "   "))
            print(f"        [{mark}] {m.base:28s} {m.size:>12,} B  {m.label()}")
        if self.res is not None:
            line = f"res     {self.res.base} out of the container, {self.res.size:,} B"
            if self.res.crc is not None:
                line += (f" -- CRC-32 {self.res.crc:08x}, the copy this port was verified against"
                         if self.res.crc == RES_CRC else
                         f" -- CRC-32 {self.res.crc:08x}, not {RES_CRC:08x}: every resource offset may be wrong")
            print(line)
        if self.res_in_image:
            f = self.res_in_image
            print(f"res     {f['res_name']} inside {self.img.base}'s installer payload ({f['name']} at volume "
                  f"LBA {f['lba']}, {f['count']} entries, {f['res_size']:,} B)")
        if self.img is not None:
            how = ("this port's own table of contents (the file brought none it could use)" if self.own_toc
                   else ("a fan rip's image, sliced by the offsets measured for it"
                         + ("" if self.corrected else " -- uncorrected: its own cue is taken as written")
                         if self.fan else "the disc's own geometry"))
            print(f"music   {self.img.base} = {self.img.size // SECTOR:,} sectors -- {len(self.plan)} audio "
                  f"track(s), {how}")
        else:
            print("music   none -- the game will run silent")
        if self.cue_out is not None:
            print(f"cue     {self.cue.base}: {len(cue_tracks(self.cue_text))} tracks "
                  f"({self.how.get('cue', 'here')})")
        return self


def default_paths():
    """Where to look when nothing was named: here, and a `NIGHT/` under here.

    Both entry points need the same answer -- `night-extractor` with no arguments, and
    `scripts/pocket/repack.py` filling a card from the disc this repo has -- so the answer is the input
    layer's, not one script's.  `NIGHT/` is one rip's directory name rather than a rule, but it is the name
    most of the documents use.
    """
    cwd = Path.cwd()
    return [cwd] + ([cwd / "NIGHT"] if (cwd / "NIGHT").is_dir() else [])


def open_source(paths, cue=None, img=None, res=None, cache_dir=None) -> Source:
    """Resolve whatever was named -- a directory, a zip, a CUE and its image, a bare image, a NIGHT.RES --
    into a Source.

    The `cue`/`img`/`res` overrides exist for the one case a rule cannot decide: a rip directory with two
    images in it.  Anything they name is added to the container first, so it gets printed too.
    """
    notes = []
    want = [Path(p).expanduser().resolve() for p in paths]
    for p in (cue, img, res):
        if p is not None:
            p = Path(p).expanduser().resolve()
            if not p.exists():
                raise SystemExit(f"{p}: not there")
            if p not in want:
                want.append(p)
    kind, paths, members = members_of(want, notes)
    src = Source(kind, paths, members, notes)
    src.cache_dir = Path(cache_dir) if cache_dir else Path.cwd() / CACHE_NAME

    got = {}
    for m, key in ((res, "res"), (cue, "cue"), (img, "img")):
        if m is None:
            continue
        hit = next((x for x in members if Path(x.name).resolve() == Path(m).expanduser().resolve()), None)
        if hit is None:
            raise SystemExit(f"--{key} {m}: not in the container this tool read")
        got[key] = (hit, f"given with --{key}")

    auto_res, auto_cue, auto_img, how = choose(members, notes)
    src.res = got["res"][0] if "res" in got else auto_res
    src.cue = got["cue"][0] if "cue" in got else auto_cue
    src.img = got["img"][0] if "img" in got else auto_img
    src.how = {k: v[1] for k, v in got.items()}

    text = src.cue.whole().decode("latin1") if src.cue is not None else None
    if text is not None:
        named = cue_file_line(text)
        hit = next((m for m in members if m.base.upper() == named), None)
        if hit is not None and is_img(hit):
            if hit is not src.img:
                src.how["img"] = f"named by {src.cue.base}'s FILE line"
            src.img = hit
        elif named:
            notes.append(f"{src.cue.base} names {named}, which is neither in the {kind} nor passed to it")
        src.cue_text = text

    # A bare image: the track geometry is this port's own, offered only for an image the size of this disc.
    if src.img is not None and text is None:
        if abs(src.img.size // SECTOR - DISC_SECTORS) <= DISC_SECTORS * 0.02:
            src.own_toc, src.cue_text = True, disc_cue()
            notes.append(f"{src.img.base} came with no cue -- using this port's own table of contents "
                         f"({src.img.size // SECTOR:,} sectors against this disc's {DISC_SECTORS:,})")
        else:
            notes.append(f"{src.img.base} needs its .CUE passed too: the image does not say where its "
                         f"tracks are ({src.img.size // SECTOR:,} sectors, this disc is {DISC_SECTORS:,})")
            src.img = None

    if not src.has_res and src.img is None:
        raise SystemExit("nothing this port reads: no NIGHT.RES and no image with a table of contents "
                         "(--data takes a directory, a zip, a CUE and its image, a bare image, or "
                         "NIGHT.RES alone)")
    if src.img is not None:
        src.volume, src.res_in_image, src.data = probe_image(src.img, src.cue_text, src.cache_dir)
        # Fan or disc is answered by what the image *holds*, not by whether it opens like a disc.  Measured
        # here, not assumed: the fan repack's MUSIC.RES begins with the disc's 174 header sectors, so its
        # volume descriptors are present and read correctly -- and its files are not, because the rip cut the
        # disc's contents off after the header.  "No ISO9660 volume" would call this a disc and slice its
        # audio with the disc's numbers, which is 560 bytes late on every track and 25.26 s late on two of
        # them.  A damaged real image lands here too, and lands safely: the corrections are CRC-guarded, so
        # an image that is not the copy they were measured on is sliced by its own cue as written.
        src.fan = not src.volume or src.res_in_image is None
        if src.fan:
            # Said out loud because the byte comparison says so (docs/466): this repack's audio matches the
            # shipped disc byte for byte where its alignment holds -- five of fourteen tracks, end to end --
            # and stops matching partway through on the rest, by amounts that are not multiples of the
            # 2352-byte frame, which rules out "the rip simply dropped a frame". So a fan image gives the
            # game music of this disc, but not this disc's bytes; --wav lets anyone hear what that means.
            notes.append(f"{src.img.base} is a fan rip's image: its audio matches the shipped disc byte for "
                         f"byte on some tracks and drifts inside others. The slice positions used are the "
                         f"best measured; --wav writes the cuts as 44.1 kHz WAVs so you can listen")
        if src.fan and src.img.crc is None:
            # The corrections that make a fan image play this disc's music are guarded by a CRC, and a
            # repack someone unzipped by hand carries none.  Measuring it costs one pass over the image --
            # 546 MB in a second and a half -- and the alternative is slicing 560 bytes late on every track
            # and 25.26 s late on two of them.
            src.img.measure_crc()
        if src.volume and src.res_in_image is None and src.res is None:
            notes.append(f"{src.img.base} carries an ISO9660 volume whose payload cannot be read -- no "
                         f"NIGHT.DAT where its own directory says it is")
        plan = plan_tracks(src.img, src.cue_text, src.fan)
        src.plan, src.corrected = plan["tracks"], plan["corrected"]
        if src.fan and src.img.crc is not None and src.img.crc != MUSIC_CRC:
            notes.append(f"{src.img.base} is a fan rip's image, but its CRC-32 is "
                         f"{src.img.crc:08x} and not the {MUSIC_CRC:08x} the offsets were measured on, so "
                         f"its own cue is taken as written")
    if not src.has_res:
        raise SystemExit(f"no NIGHT.RES -- neither in the container nor inside "
                         f"{src.img.base if src.img is not None else 'the data passed'}: the game has "
                         f"nothing to load")
    return src
