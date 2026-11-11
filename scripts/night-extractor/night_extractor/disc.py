"""The disc itself: CUE geometry, the MODE1/2352 ISO9660 reader, the NIGHT.RES container.

Nothing here writes a file or prints anything.  The measurements that fixed these offsets are in the
package docstring and in docs/455; the comments here carry the parts that keep a future reader from
"simplifying" a trap back in.
"""
from __future__ import annotations

import re
from pathlib import Path

SECTOR = 2352          # every track of TG_NIGHT is stored as raw 2352-byte frames
UD_OFF = 16            # ISO9660 user data inside a MODE1/2352 frame
UD_LEN = 2048
PREGAP = 150           # the CUE's single PREGAP 00:02:00, in sectors

def msf_to_frames(t: str) -> int:
    m, s, f = (int(x) for x in t.split(":"))
    return m * 4500 + s * 75 + f


def parse_cue(path: Path):
    """(tracks, pregap_frames, data_file).  tracks = [(num, mode, disc_lba, img_frame)].

    The same arithmetic src/cd/cd_iso.c runs, with the two coordinate systems kept apart on purpose:
    an INDEX 01 time is a position *in the file*, and a track's disc LBA is that plus all PREGAP seen
    so far.  TG_NIGHT lists one PREGAP (00:02:00) inside TRACK 02, so track 01 sits below it (disc LBA
    0 = image sector 0, nothing subtracted) while every audio track sits 150 sectors above its own file
    position -- which is exactly the -150 cd_iso_read_audio() applies, and why folding the pregap into
    one number and then subtracting it puts the data track at image sector -150.
    """
    tracks, cur, mode, first, pregap, data_file = [], 0, "", None, 0, None   # (num, mode, lba, frame)
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line.startswith("FILE "):
            m = re.match(r'FILE\s+"(.*?)"\s+(\S+)', line)
            if m and data_file is None:            # TG_NIGHT uses one FILE for all 15 tracks
                data_file = m.group(1)
        elif line.startswith("TRACK "):
            p = line.split()
            if len(p) >= 3:
                if cur and first is not None:
                    tracks.append((cur, mode, msf_to_frames(first) + pregap, msf_to_frames(first)))
                cur, mode, first = int(p[1]), p[2].upper(), None
        elif line.startswith("PREGAP "):
            pregap += msf_to_frames(line.split()[1])
        elif line.startswith("INDEX "):
            p = line.split()
            if len(p) >= 3 and int(p[1]) == 1 and first is None:
                first = p[2]
    if cur and first is not None:
        tracks.append((cur, mode, msf_to_frames(first) + pregap, msf_to_frames(first)))
    if not tracks:
        sys.exit(f"{path}: no TRACK lines")
    return tracks, pregap, data_file


class IsoStream:
    """A read handle over one ISO9660 file's user data, with ordinary seek/read semantics.

    MODE1/2352 stores 2048 bytes of user data per 2352-byte frame, so a file byte N lives at
    img[(first + lba + N//2048)*2352 + 16 + N%%2048].  Mapping N linearly onto the raw image instead --
    the first version of this script did -- read every blob after the first sector 16 bytes early per
    sector travelled, which produced the right sizes with the wrong bytes: the extracted container came
    out 50 964 894 B long and did not match the installed copy.  Crossing the boundary in one place is
    what keeps that class of error out of every caller.
    """

    def __init__(self, track, lba: int, size: int):
        self.t, self.lba, self.size, self.pos = track, lba, size, 0

    def seek(self, off: int):
        if not 0 <= off <= self.size:
            raise ValueError(f"offset {off} outside the file ({self.size} B)")
        self.pos = off

    def tell(self):
        return self.pos

    def read(self, n: int) -> bytes:
        out = bytearray()
        while len(out) < n and self.pos < self.size:
            want = min(n - len(out), self.size - self.pos)
            sec, rem = divmod(self.pos, UD_LEN)
            got = self.t.user(self.lba + sec, 1)[rem:rem + want]
            if not got:
                break
            out += got
            self.pos += len(got)
        return bytes(out)


class DataTrack:
    """The MODE1/2352 data track inside a raw image: byte geometry + ISO9660 name lookup."""

    def __init__(self, fh, first_frame: int, nframes: int):
        self.fh, self.first, self.n = fh, first_frame, nframes

    def img_off(self, lba: int) -> int:
        """Byte offset in the image of logical sector `lba`'s user data."""
        return (self.first + lba) * SECTOR + UD_OFF

    def user(self, lba: int, n: int = 1) -> bytes:
        if not 0 <= lba < self.n:
            raise IndexError(f"logical sector {lba} outside the data track ({self.n} sectors)")
        if n > 64:                # a directory that spans 64 sectors is a misread record, not a directory
            raise ValueError(f"refusing to read {n} sectors at LBA {lba}")
        self.fh.seek((self.first + lba) * SECTOR)
        return self.fh.read(n * SECTOR)[UD_OFF:UD_OFF + n * UD_LEN]

    def root_record(self) -> bytes:
        """The 34-byte root directory record, from the volume descriptor sector.

        Field offsets come from the record's own shape -- length, ext-attribute 0, the both-byte LE/BE
        pair duplicated for LBA and for size, and a name length that makes the record exactly as long as
        it claims -- not from a remembered table.  TG_NIGHT's descriptor carries "CD001" at +1 of its
        user data instead of the book's +2, so offsets carried over from a table would have read the
        wrong dwords and still returned bytes; the shape test is what makes a wrong guess refuse rather
        than mislead.
        """
        cands = []
        for s in range(16, 33):
            d = self.user(s)
            if d.find(b"CD001") < 0:
                continue
            for o in range(0, 300):
                L = d[o]
                if not 33 <= L <= 54:
                    continue
                if int.from_bytes(d[o + 2:o + 6], "little") != int.from_bytes(d[o + 6:o + 10], "big"):
                    continue
                lba = int.from_bytes(d[o + 2:o + 6], "little")
                if int.from_bytes(d[o + 10:o + 14], "little") != int.from_bytes(d[o + 14:o + 18], "big"):
                    continue
                size = int.from_bytes(d[o + 10:o + 14], "little")
                il = d[o + 32]
                if 0 < lba < self.n and 0 < size <= 2048 * 64 and il == L - 33:
                    cands.append(d[o:o + L])
        if not cands:
            raise ValueError("no root directory record in sectors 16..32")
        return cands[0]

    @staticmethod
    def records(data: bytes):
        """A directory extent's records.  Stops at the first record that cannot be one, so a directory
        read that runs into file data returns what it recognised instead of inventing entries."""
        p, out = 0, []
        while p + 34 <= len(data):
            L = data[p]
            if L == 0:
                p += 1
                continue
            if p + L > len(data) or L < 33:
                break
            e = data[p:p + L]
            il = e[32]
            if 33 + il > L:
                break
            out.append(dict(lba=int.from_bytes(e[2:6], "little"),
                            size=int.from_bytes(e[10:14], "little"),
                            flags=e[25],
                            name=e[33:33 + il].decode("latin1").split(";")[0]))
            p += (L + 1) & ~1
        return out

    def find(self, name: str):
        """(lba, size) of a volume-relative file, breadth-first, case-insensitive.

        The self/parent entries are skipped by their identifier byte (ECMA-119 reserves 0x00 and 0x01
        for them) and every directory LBA is visited once.  Both guards are load-bearing: the first
        attempt neither skipped them nor remembered visited LBAs, so a name that is *not* on the disc
        re-entered the root through "." and ".." and re-read it 2^depth times -- a query for
        "NIGHT/NIGHT.DAT" hung for four minutes before it was interrupted.
        """
        want = name.upper().replace("/", "")
        root = self.root_record()
        rlba = int.from_bytes(root[2:6], "little")
        rsize = int.from_bytes(root[10:14], "little")
        queue, seen = [(rlba, rsize)], set()
        while queue:
            lba, size = queue.pop(0)
            if lba in seen or lba >= self.n or size > 2048 * 64:
                continue                            # visited, outside the track, or not a real extent
            seen.add(lba)
            for r in self.records(self.user(lba, (size + 2047) // 2048)):
                if r["flags"] & 2:                  # directory
                    if r["name"] not in ("\x00", "\x01"):
                        queue.append((r["lba"], r["size"]))
                elif r["name"].upper().replace("/", "") == want:
                    return r["lba"], r["size"]
        return None

    def find_container(self, count: int):
        """Fallback: scan the track for a NIGHT.RES container header with `count` entries whose decoded
        directory is self-consistent (its first blob starts exactly where the directory ends).  Used
        only when the volume tree cannot be walked; the consistency test is what makes this a
        signature rather than a guess that happens to find 06 00 00 00."""
        for s in range(0, self.n):
            d = self.user(s)
            if int.from_bytes(d[:4], "little") != count:
                continue
            ents = decode_container_dir(d[:4 + count * 20])
            if ents and ents[0][1] == 4 + count * 20:
                return s
        return None



def decode_container_dir(raw: bytes):
    """The 20-byte-entry directory block of a NIGHT.RES container -> [(name, offset, size)].

    Obfuscation, verified against the loader fn_0366a4 (docs/02-container.md): a reverse XOR chain over
    the whole block -- b[i] ^= b[i-1] walking backwards, then b[0] ^= 0x20.
    """
    count = int.from_bytes(raw[:4], "little")
    b = bytearray(raw[4:4 + count * 20])
    if len(b) != count * 20:
        return []
    for i in range(len(b) - 1, 0, -1):
        b[i] ^= b[i - 1]
    b[0] ^= 0x20
    out = []
    for i in range(count):
        e = b[i * 20:(i + 1) * 20]
        name = e[:12].split(b"\0", 1)[0].decode("latin1")
        size, off = int.from_bytes(e[12:16], "little"), int.from_bytes(e[16:20], "little")
        out.append((name, off, size))
    return out


