"""Building the run tree: streaming copies that hash as they go, the three parts, the manifests, verify.

Every file is written to `.part` and renamed, and every one carries its sha256 into MANIFEST.tsv, so
"the tree I have is the tree that was verified" is a question `--verify` answers instead of a hope.
"""
from __future__ import annotations

import hashlib
from pathlib import Path

from .disc import SECTOR, DataTrack, IsoStream, decode_container_dir

CHUNK = 1 << 20

def sha256_file(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for block in iter(lambda: f.read(CHUNK), b""):
            h.update(block)
    return h.hexdigest()


def load_manifest(out: Path):
    """The previous run's MANIFEST.tsv as rows -- so a partial run can keep the parts it did not touch.

    `--only audio` used to rewrite MANIFEST.tsv from its own rows alone, which dropped the game rows:
    the next `--verify` then verified 14 files of 21, and load_prev() lost the hashes that make a re-run
    cost one stat.  A manifest describes the tree, not the last invocation.
    """
    man = out / "MANIFEST.tsv"
    if not man.exists():
        return []
    rows = [l.split("\t") for l in man.read_text().splitlines() if l.strip() and not l.startswith("#")]
    if len(rows) < 2:
        return []
    hdr = rows[0]
    out_rows = []
    for r in rows[1:]:
        d = dict(zip(hdr, r))
        if "kind" in d and "file" in d:
            out_rows.append(d)
    return out_rows


def load_prev(out: Path):
    """{file: (bytes, sha256)} from the previous run's manifest, so a kept file costs one stat."""
    return {r["file"]: (int(r["bytes"]), r["sha256"]) for r in load_manifest(out)
            if r.get("bytes", "").isdigit()}


def write_stream(dst: Path, src, off: int, size: int, force: bool, prev=None):
    """Copy `size` bytes from `src` at `off` into dst -> (bytes, sha256, wrote).

    Idempotent by size: an existing file of the right length, whose hash the previous manifest knows,
    is neither read nor rewritten unless --force, so a re-run costs one stat per file.  --verify is
    what re-hashes.  Written to .part then renamed, so an interrupted run cannot leave a half-written
    NIGHT.RES that the next run would trust.
    """
    if dst.exists() and dst.stat().st_size == size and not force:
        if prev and dst.name in prev and prev[dst.name][0] == size:
            return size, prev[dst.name][1], False
        return size, sha256_file(dst), False
    dst.parent.mkdir(parents=True, exist_ok=True)
    tmp = dst.with_suffix(dst.suffix + ".part")
    h = hashlib.sha256()
    src.seek(off)
    left = size
    with tmp.open("wb") as g:
        while left:
            block = src.read(min(CHUNK, left))
            if not block:
                raise EOFError(f"short read at image byte {off}: {left} bytes missing")
            g.write(block)
            h.update(block)
            left -= len(block)
    tmp.replace(dst)
    return size, h.hexdigest(), True



def part_game(img, out: Path, track: DataTrack, args, prev):
    """The installer's payload: NIGHT.DAT's container entries, verbatim, under the names the disc uses.

    The container's own index spells its six entries lower case -- `night.res`, `readme.txt` -- and that
    is what SETUP.EXE would have written on a DOS disk. What the CD calls them is upper case: the ISO9660
    record (which forces A-Z), the installer's name table at the EOF of NIGHT.TGS (`DOS4GW.EXE NIGHT.BAT
    NIGHT.EXE NIGHT.ICO NIGHT.RES README.TXT`), and the string NIGHT.EXE prints when the container is
    missing (`"NIGHT.RES File Not Found!!!"`, src/f/f.c:2915). Bytes come out of a CD image, so the file
    keeps the disc's spelling; the archive's spelling is recorded in the manifest's note column.
    """
    hit = track.find("NIGHT/NIGHT.DAT") or track.find("NIGHT.DAT")
    if hit:
        lba, fsize = hit
        print(f"[game] NIGHT.DAT at volume LBA {lba}, {fsize} bytes")
    else:
        lba = track.find_container(6)
        if lba is None:
            sys.exit("[game] NIGHT.DAT neither in the volume tree nor by container signature")
        fsize = None
        print(f"[game] volume tree unreadable; NIGHT.DAT found by container signature at LBA {lba}")

    count = int.from_bytes(track.user(lba)[:4], "little")
    ents = decode_container_dir(track.user(lba, (4 + count * 20 + 2047) // 2048)[:4 + count * 20])
    if not ents:
        sys.exit(f"[game] NIGHT.DAT directory did not decode (count={count})")
    total = 4 + count * 20 + sum(s for _, _, s in ents)
    if fsize and total != fsize:
        print(f"[game] NOTE header + directory + blobs = {total} != file size {fsize}")
    # NIGHT.DAT is an ISO9660 *file*: its blobs are addressed inside the file's user data, and every
    # 2048-byte chunk of that sits behind a 16-byte frame header in the image.
    dat = IsoStream(track, lba, fsize if fsize else sum(s for _, _, s in ents) + 4 + count * 20)
    print(f"[game] container holds {count} entries (volume LBA {lba}, {count * 20 + 4} B of directory):")
    rows = []
    for name, off, size in ents:
        ship = name.upper()          # the disc's spelling, not the container index's (see the docstring)
        got, sha, wrote = write_stream(out / ship, dat, off, size, args.force, prev)
        rows.append(dict(kind="game", file=ship, bytes=got, sha256=sha,
                         source=f"NIGHT.DAT@{lba} +{off}..+{off + got - 1}",
                         note=f"NIGHT.DAT entry {name!r} at +{off}, {size} B -- installed by SETUP.EXE"))
        spelled = f"  (the container index spells it {name})" if ship != name else ""
        print(f"       {'wrote' if wrote else 'kept '} {ship:12s} {got:>10} B  sha256 {sha[:12]}{spelled}")
    return rows


def part_audio(img, out: Path, audio, pregap, img_sectors, args, prev):
    """Cut each CD-DA track to cd/trackNN.bin -- the Pocket's slot files, byte for byte."""
    cd = out / "cd"
    rows = []
    print(f"[audio] {len(audio)} CD-DA tracks -> {cd}/trackNN.bin (raw {SECTOR}-byte frames, no header)")
    for i, (num, mode, lba, frame) in enumerate(audio):
        first_img = frame                    # the CUE's INDEX 01 is already the position in the image
        n = (audio[i + 1][3] - frame) if i + 1 < len(audio) else (img_sectors - first_img)
        name = f"cd/track{num:02d}.bin"
        got, sha, wrote = write_stream(cd / f"track{num:02d}.bin", img, first_img * SECTOR,
                                       n * SECTOR, args.force, prev)
        rows.append(dict(kind="audio", file=name, bytes=got, sha256=sha, track=num, sectors=n,
                         disc_lba=lba, img_first_sector=first_img,
                         source=f"img sector {first_img}..{first_img + n - 1}",
                         note="raw CD-DA frames; 2352 B = 588 audio frames"))
        print(f"       {'wrote' if wrote else 'kept '} track{num:02d}.bin {got:>10} B  {n:>6} sect  "
              f"disc lba {lba:>7}  sha256 {sha[:12]}")
    if len(audio) >= 2:                             # the disc gate's own number, printed not assumed
        gap = audio[1][2] - audio[0][2]
        print(f"[audio] track {audio[1][0]:02d} - track {audio[0][0]:02d} = {gap} sectors "
              f"(0x{gap:x}) -- the difference fn_010c3e compares")
    return rows


def part_cue(cue: Path, out: Path, args):
    """Copy the CUE: the port still reads the TOC from it, and it is 669 bytes."""
    if (out / cue.name).resolve() == cue.resolve():
        return []                                   # already the file itself
    data = cue.read_bytes()
    dst = out / cue.name
    if dst.exists() and dst.stat().st_size == len(data) and not args.force:
        print(f"[cue ] kept  {dst.name}")
    else:
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(data)
        print(f"[cue ] wrote {dst.name}")
    return [dict(kind="cue", file=cue.name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                 source=str(cue), note="the TOC src/cd/cd_iso.c answers MSCDEX from")]



# The audio rows carry four columns more than the other kinds.  They are part of MAN_COLS rather than
# dropped, because MANIFEST.tsv is also what a partial run reads back to keep the parts it did not
# write -- with them missing, cd/manifest.tsv would lose the track/sector/LBA columns after a
# `--only game`.
MAN_COLS = ("kind", "file", "bytes", "sha256", "source", "note", "track", "sectors", "disc_lba",
            "img_first_sector")
KIND_ORDER = ("game", "audio", "cue")          # the order a manifest lists its parts in
CD_COLS = ("slot", "file", "bytes", "track", "sectors", "disc_lba", "img_first_sector", "sha256")


def write_manifests(out: Path, rows):
    (out / "MANIFEST.tsv").write_text(
        "# generated by scripts/extract_disc.py -- do not edit; --verify re-hashes against this\n"
        + "\t".join(MAN_COLS) + "\n"
        + "".join("\t".join(str(r.get(c, "")) for c in MAN_COLS) + "\n" for r in rows))
    cd = out / "cd"
    if cd.is_dir():
        (cd / "manifest.tsv").write_text(
            "# generated by scripts/extract_disc.py.  A TRACKnn audit records offsets relative to the\n"
            "# track file; img_first_sector + offset/2352 maps one onto TG_NIGHT.IMG, which is what\n"
            "# scripts/pocket/audit_disc.py does.  slot is blank: this tree is not an APF dist (that one\n"
            "# is scripts/pocket/repack.py's manifest.tsv).\n"
            + "\t".join(CD_COLS) + "\n"
            + "".join("\t".join(str(r.get(c, "")) for c in CD_COLS) + "\n"
                      for r in rows if r["kind"] == "audio"))



def verify(out: Path) -> int:
    man = out / "MANIFEST.tsv"
    if not man.exists():
        print(f"FAIL no manifest at {man}")
        return 1
    rows = [l.split("\t") for l in man.read_text().splitlines() if l.strip() and not l.startswith("#")]
    hdr, rows = rows[0], rows[1:]
    bad = miss = 0
    for r in rows:
        d = dict(zip(hdr, r))
        p = out / d["file"]
        if not p.exists():
            print(f"MISS {d['file']}"); miss += 1; continue
        got = sha256_file(p)
        size_ok = p.stat().st_size == int(d["bytes"])
        ok = got == d["sha256"] and size_ok
        print(f"{'OK  ' if ok else 'FAIL'} {d['file']:24s} {p.stat().st_size:>10} B"
              f"{'' if ok else '   got ' + got[:12] + (' size' if not size_ok else '')}")
        bad += 0 if ok else 1
    print(("OK   %d files verified against %s" % (len(rows), man.name)) if not bad
          else f"FAIL {bad} of {len(rows)} differ ({miss} missing)")
    return 0 if not bad else 1


def cross_check_installed(out: Path, rows, res_dir: Path):
    """Say plainly whether the extracted install set equals the installed copy the repo already ships.

    'Replaces the installer' is a claim about bytes, so it is measured here rather than asserted.  The
    the container's index and the disc disagree about case, and the tree now keeps the disc's; matching
    stays case-insensitive, which is exactly the DOS behaviour the installer ran under.
    """
    if not (res_dir / "NIGHT.RES").exists():
        print(f"\nno installed copy in {res_dir}/ to cross-check against (--res to point at one)")
        return
    installed = {p.name.upper(): p for p in res_dir.iterdir() if p.is_file()}
    print("\nthe install set, against the installed copy in %s/:" % res_dir)
    for r in (x for x in rows if x["kind"] == "game"):
        b = installed.get(r["file"].upper())
        if not b:
            print(f"  {r['file']:12s} no installed copy to compare")
            continue
        same = r["bytes"] == b.stat().st_size and r["sha256"] == sha256_file(b)
        print(f"  {r['file']:12s} {'byte-identical' if same else 'DIFFERS (extract %d B vs installed %d B)' % (r['bytes'], b.stat().st_size)}")


