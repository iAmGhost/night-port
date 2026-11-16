#!/usr/bin/env python3
"""Does night-extractor recognise every shape the browser port accepts?  (docs/464, docs/465)

Two halves.

The cheap half -- and the one worth running after any change to `source.py` -- opens each shape and asserts
what the tool *decided*: which member is the resources, which is the image, whose cue it is, whether the
image is a fan rip, whether the fan corrections applied, and how many audio tracks the plan cuts.  A shape
that is misread is a wrong-asset bug, not a crash, so the decision is what has to be asserted.

The expensive half (`--bytes`) actually writes a tree for each shape and hashes it against the verified
tree in `extracted/` -- because the fan repack's `MUSIC.RES` and the shipped `TG_NIGHT.IMG` are supposed to
produce the *same bytes* through different arithmetic (`MUSIC_SHIFT`, `MUSIC_EXTRA`), and only a hash says
so.  That one moves ~1.2 GB per shape; it is opt-in.

    uv run python scripts/analysis/check_input_shapes.py
    uv run python scripts/analysis/check_input_shapes.py --bytes
"""
from __future__ import annotations

import hashlib
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "night-extractor"))

from night_extractor import slots as nkslots                       # one slot table for every port  # noqa: E402
from night_extractor.source import (DISC_SECTORS, MUSIC_CRC, RES_CRC, open_source)  # noqa: E402

REF = REPO / "extracted"
OUT = REPO / "scratch" / "nkx-shapes"
CACHE = REPO / "scratch" / "nkx-shapes-cache"
SAMPLE = ["NIGHT.RES", "cd/track02.bin", "cd/track08.bin", "cd/track09.bin", "cd/track15.bin"]

failures = []


def sha(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def check(name, cond, detail=""):
    print(f"  {'ok  ' if cond else 'FAIL'} {name}{('  ' + detail) if detail else ''}")
    if not cond:
        failures.append(name)


def src_of(paths, **kw):
    return open_source(paths, cache_dir=CACHE, **kw)


def shape_dir_pair():
    """The repo's own rip: a CUE and the image its FILE line names, passed by name."""
    s = src_of([REPO / "TG_NIGHT.CUE", REPO / "TG_NIGHT.IMG"])
    yield ("a pair, by name", s, dict(kind="files", fan=False, own_toc=False, tracks=14,
                                      res_in_image=True, res=None, cue="TG_NIGHT.CUE", img="TG_NIGHT.IMG"))


def shape_dir_cwd():
    """The same pair in one directory: nothing named but the folder."""
    s = src_of([REPO])
    yield ("a rip directory, nothing named", s, dict(kind="files", fan=False, own_toc=False, tracks=14,
                                                      res_in_image=True, cue="TG_NIGHT.CUE",
                                                      img="TG_NIGHT.IMG"))


def shape_stored_zip():
    """TG_NIGHT.CUE + TG_NIGHT.IMG, stored in a zip -- the shape a download hands you."""
    z = Path("/tmp/nkzip/orig.zip")
    if not z.is_file():
        return
    s = src_of([z])
    yield ("a zip holding the pair (stored)", s, dict(kind="zip", fan=False, own_toc=False, tracks=14,
                                                       res_in_image=True, cue="TG_NIGHT.CUE",
                                                       img="TG_NIGHT.IMG"))


def shape_renamed_file_line():
    """A pair renamed, with the cue's FILE line rewritten to match -- the cue is the authority, not us."""
    d = OUT / "renamed"
    d.mkdir(parents=True, exist_ok=True)
    cue = (REPO / "TG_NIGHT.CUE").read_text().replace("TG_NIGHT.IMG", "Eojjeonji Disc.bin")
    (d / "Eojjeonji Disc.cue").write_text(cue)
    img = d / "Eojjeonji Disc.bin"
    if not img.is_symlink():
        img.unlink(missing_ok=True)
        img.symlink_to(REPO / "TG_NIGHT.IMG")
    s = src_of([d])
    yield ("a pair renamed, cue's FILE line rewritten", s, dict(kind="files", fan=False, own_toc=False,
                                                                tracks=14, res_in_image=True,
                                                                cue="Eojjeonji Disc.cue",
                                                                img="Eojjeonji Disc.bin"))


def shape_bare_image():
    """An image with no cue at all: this port's own table of contents, chosen by size."""
    s = src_of([REPO / "TG_NIGHT.IMG"])
    yield ("a bare image (own table of contents)", s, dict(kind="files", fan=False, own_toc=True,
                                                            tracks=14, res_in_image=True, img="TG_NIGHT.IMG"))


def shape_res_only():
    """NIGHT.RES alone: the game, silently.  No image, no plan, no error."""
    s = src_of([REF / "NIGHT.RES"])
    yield ("NIGHT.RES alone (no music)", s, dict(kind="files", tracks=0, img=None, cue=None,
                                                 res="NIGHT.RES"))


def shape_fan_loose():
    """A fan repack's files picked together: SOUND.RES is its cue, MUSIC.RES its image."""
    d = Path("/tmp/nkzip")
    if not (d / "MUSIC.RES").is_file():
        return
    s = src_of([d / "NIGHT.RES", d / "SOUND.RES", d / "MUSIC.RES"])
    yield ("a fan repack, loose files", s, dict(kind="files", fan=True, corrected=True, tracks=14,
                                                res="NIGHT.RES", img="MUSIC.RES"))


def shape_fan_zip():
    """The same fan repack inside its 533 MB zip, deflated -- the staging path."""
    z = Path("/tmp/nkzip/dosbox.zip")
    if not z.is_file():
        return
    s = src_of([z])
    yield ("a fan repack, deflated zip", s, dict(kind="zip", fan=True, corrected=True, tracks=14,
                                                 res="NIGHT.RES", img="MUSIC.RES"))


def expected(name, s, want):
    got = dict(kind=s.kind, fan=s.fan, own_toc=s.own_toc, corrected=s.corrected, tracks=len(s.plan),
               res=(s.res.base if s.res else None), img=(s.img.base if s.img else None),
               cue=(s.cue.base if s.cue else None), res_in_image=s.res_in_image is not None,
               has_res=s.has_res, crc=s.img.crc if s.img else None)
    bad = {k: (v, got.get(k)) for k, v in want.items() if got.get(k) != v}
    check(name, not bad, "" if not bad else " ".join(f"{k}: want {v!r} got {got.get(k)!r}"
                                                     for k, v in bad.items()))
    if s.img is not None and want.get("fan"):
        check(f"{name}: image CRC guards the corrections", s.img.crc == MUSIC_CRC,
              f"{s.img.crc:08x}" if s.img.crc is not None else "no CRC known")
    if s.res is not None:
        check(f"{name}: resources CRC is the verified container",
              s.res.crc is None or s.res.crc == RES_CRC, f"{s.res.crc:08x}" if s.res.crc is not None else "")
    if not want.get("own_toc") and not want.get("fan") and s.cue_out is not None:
        check(f"{name}: the cue worth writing is the visitor's", s.cue_out is s.cue_text)
    if want.get("own_toc") or want.get("fan"):
        check(f"{name}: what gets written is this port's own TOC", s.cue_out is None)
    if want.get("tracks"):
        gap = s.plan[1]["lba"] - s.plan[0]["lba"]
        check(f"{name}: the disc-present gate sees its number", gap == 3153, f"gap {gap}")
    if s.img is not None and not want.get("fan"):
        check(f"{name}: a disc image is sized like this disc",
              abs(s.img.size // 2352 - DISC_SECTORS) < 4096, f"{s.img.size // 2352} sectors")
    if s.plan:                                     # the card binds files to slots; one table decides which
        tracks = [t["track"] for t in s.plan]
        ids = {t: nkslots.id_for_track(tracks, t) for t in tracks}
        vals = sorted(ids.values())
        check(f"{name}: every track is bound to a distinct audio slot",
              all(v in nkslots.AUDIO_IDS for v in vals) and len(set(vals)) == len(vals),
              ",".join(str(v) for v in vals))
        check(f"{name}: the resource slot is not an audio slot", nkslots.RES_ID not in vals,
              f"slot {nkslots.RES_ID}")
    return got


def bytes_against_reference(name, s, out_dir):
    """Write the tree and hash it against the verified one -- the fan repack must land on the same bytes."""
    import subprocess
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = [str(p) for p in s.paths]
    r = subprocess.run([sys.executable, str(REPO / "scripts" / "extract_disc.py"), "--data", *paths,
                        "-o", str(out_dir), "--cache", str(CACHE)], capture_output=True, text=True,
                       cwd=REPO)
    if r.returncode != 0:
        check(f"{name}: the run succeeds", False, r.stderr.strip()[-300:])
        return
    sm = out_dir / "slot_map.tsv"          # the file a person copies from, so it has to name the container
    if sm.exists():
        rows = [l.split("\t") for l in sm.read_text().splitlines() if l and not l.startswith("#")][1:]
        want = sorted([nkslots.RES_ID] + [nkslots.id_for_track([t["track"] for t in s.plan], t["track"])
                                          for t in s.plan])
        got = sorted(int(r[0]) for r in rows)
        check(f"{name}: slot_map.tsv binds the container and every track", got == want,
              f"{got[:3]}... want {want[:3]}...")
    else:
        check(f"{name}: slot_map.tsv written", False, "missing")
    for f in SAMPLE:
        p, q = out_dir / f, REF / f
        if not p.exists():
            check(f"{name}: {f} written", False, "missing")
            continue
        same = p.stat().st_size == q.stat().st_size and sha(p) == sha(q)
        check(f"{name}: {f} byte-identical to the verified tree", same,
              f"{p.stat().st_size:,} B")


def main() -> int:
    do_bytes = "--bytes" in sys.argv
    only = [a.split("=", 1)[1] for a in sys.argv if a.startswith("--only=")]
    shutil.rmtree(OUT, ignore_errors=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for fn in (shape_dir_pair, shape_dir_cwd, shape_stored_zip, shape_renamed_file_line, shape_bare_image,
               shape_res_only, shape_fan_loose, shape_fan_zip):
        for name, s, want in fn():
            if only and not any(o.lower() in name.lower() for o in only):
                continue
            print(name)
            got = expected(name, s, want)
            if do_bytes:
                bytes_against_reference(name, s, OUT / name.replace(" ", "-").replace(",", ""))
    print(f"\n{len(failures)} failure(s)" if failures else "\nALL SHAPES RECOGNISED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
