#!/usr/bin/env python3
"""Write a per-track BIN/CUE of the disc into the extracted tree, for the emulator.

WHY THIS EXISTS.  The disc ships as one 633 MB raw image plus a CUE that points every track at it
(`FILE "TG_NIGHT.IMG" BINARY`).  `scripts/extract_disc.py` cuts the CD-DA tracks out of it
(`extracted/cd/trackNN.bin`) because those are the Pocket's slot files, and it takes the data track's
*filesystem* out of it because that is what the port needs -- but the data track's raw sectors stay
inside the image, where nothing else can use them.

dosbox-automation -- and, through it, the original game -- disagrees with that shape.  Mounted from
the single raw image, the original answers with its insert-CD screen (`cdinsert.pcx`, photographed
2026-11-12: the girl-and-disc card, Korean text, "insert the game CD and press any key").  Mounted
from a CUE that names one BIN per track, with the data track's raw 2 352-byte sectors present, it is
supposed to boot -- and this script is how that mount gets built.

    scripts/disc_bin_cue.py                       # -> extracted/cd/track01.bin + extracted/cd/disc.cue
    scripts/disc_bin_cue.py --out scratch/disc-bin

Nothing here is invented: track sizes come from the source CUE's INDEX positions, PREGAP lines carry
over from the source, and every sector is copied byte for byte out of TG_NIGHT.IMG.  Each BIN begins
at its track's INDEX 01, so every INDEX line in the written CUE is 00:00:00 -- which is also why the
source's one PREGAP stays where it was: those 150 frames are not in any file.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "night-extractor"))
from night_extractor.disc import msf_to_frames                      # noqa: E402

SECTOR = 2352


def read_cue(path: Path):
    """[(num, mode, img_frame, pregap_str)] in disc order.  Positions are frames in the referenced file."""
    out, num, mode, first, pregap = [], 0, "", None, ""
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line.startswith("TRACK "):
            p = line.split()
            if len(p) >= 3:
                if num and first is not None:
                    out.append((num, mode, msf_to_frames(first), pregap))
                num, mode, first, pregap = int(p[1]), p[2].upper(), None, ""
        elif line.startswith("PREGAP "):
            pregap = line.split()[1]                     # TG_NIGHT carries exactly one, inside TRACK 02
        elif line.startswith("INDEX "):
            p = line.split()
            if len(p) >= 3 and int(p[1]) == 1 and first is None:
                first = p[2]
    if num and first is not None:
        out.append((num, mode, msf_to_frames(first), pregap))
    if not out:
        sys.exit(f"{path}: no TRACK lines")
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cue", default="TG_NIGHT.CUE")
    ap.add_argument("--img", default="TG_NIGHT.IMG")
    ap.add_argument("--out", default="extracted/cd")
    a = ap.parse_args()

    cue, img, out = Path(a.cue), Path(a.img), Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    tracks = read_cue(cue)
    img_sectors = img.stat().st_size // SECTOR
    print(f"[cue ] {cue}: {len(tracks)} track(s); last track ends at image sector {img_sectors}")

    bounds = {}
    for i, (num, mode, frame, _pg) in enumerate(tracks):
        end = tracks[i + 1][2] if i + 1 < len(tracks) else img_sectors
        bounds[num] = (frame, end)

    # The data track: the piece the extractor does not write, because the port never asks for it.
    dnum = next((n for n, m, _f, _p in tracks if m.upper() != "AUDIO"), None)
    if dnum is None:
        raise SystemExit(f"{cue}: no data track")
    dstart, dend = bounds[dnum]
    dn = dend - dstart
    dpath = out / f"track{dnum:02d}.bin"
    t0 = time.time()
    if dpath.exists() and dpath.stat().st_size == dn * SECTOR:
        print(f"[data] kept  {dpath.name}  {dn} sectors  {dpath.stat().st_size} B")
    else:
        with open(img, "rb") as f, open(dpath, "wb") as g:
            f.seek(dstart * SECTOR)
            left = dn * SECTOR
            while left:
                buf = f.read(min(left, 1 << 20))
                if not buf:
                    raise SystemExit("the image ended before the data track did")
                g.write(buf)
                left -= len(buf)
        sha = hashlib.sha256(dpath.read_bytes()).hexdigest()[:16]
        print(f"[data] wrote {dpath.name}  {dpath.stat().st_size:>11} B  {dn:>6} sect  "
              f"img[{dstart}:{dend}]  sha256 {sha}…  ({time.time() - t0:.1f} s)")

    # The audio tracks are already out there; say so with their sizes against the CUE's boundaries, so a
    # stale tree fails here rather than confusing somebody at the emulator twelve minutes later.
    for num, mode, _f, _p in tracks:
        if mode.upper() == "AUDIO":
            p = out / f"track{num:02d}.bin"
            want = (bounds[num][1] - bounds[num][0]) * SECTOR
            if not p.exists():
                raise SystemExit(f"{p} missing -- run scripts/extract_disc.py first")
            got = p.stat().st_size
            print(f"[aud ] {p.name}  {got:>11} B  {'ok' if got == want else f'MISMATCH want {want}'}")

    lines = []
    for num, mode, _f, pg in tracks:
        lines.append(f'FILE "track{num:02d}.bin" BINARY')
        lines.append(f"  TRACK {num:02d} {mode.upper()}")
        if pg:
            lines.append(f"    PREGAP {pg}")
        lines.append("    INDEX 01 00:00:00")
    dst = out / "disc.cue"
    dst.write_text("\n".join(lines) + "\n")
    print(f"[cue ] wrote {dst}  ({len(tracks)} FILE lines)")
    print(f"       imgmount D /path/to/{dst.name} -type cue")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
