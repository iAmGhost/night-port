#!/usr/bin/env python3
"""Repack the shipped NIGHT disc into the files the Pocket core's data slots read.

WHAT IT WRITES (scratch/pocket/, i.e. regenerable; the repo only carries this script)

  NIGHT.RES              the resource container, byte-for-byte (slot 4)
  cd/trackNN.bin         one CD-DA track per file: raw 2352-byte frames, no header (slots 20..33)
  manifest.tsv           what was written, from what byte range, with the slot id

WHY PER-TRACK FILES: the SDL host reads the whole disc through one TG_NIGHT.IMG (src/cd/cd_iso.c,
fseek+fread).  On the Pocket a data slot is a file, and the game asks for a *track* (fn_010b1d resolves
a TOC index to an LBA and a sector count), so the disc is cut along its own track boundaries: slot
N+18 == audio track N, exactly the mapping scripts/pocket/mk_dist.py writes into data.json and
pocket/src/night/of_disc.c uses at run time.

WHY THE BYTE RANGES ARE lba-150: TG_NIGHT.IMG carries no PREGAP, while the LBAs in the CUE (and the
ones the game reads) are MSCDEX disc coordinates that do -- the same -150 cd_iso.c applies
(cd_iso_read_audio(37262) reads IMG sector 37112, asserted by the fixture suite).  Between two AUDIO
tracks there is no pregap, so a track's length is simply the next track's LBA minus its own, which
reproduces the sector counts the game itself asks for (track 2 -> 3153 = 0xc51, the disc gate's value).

CD-DA sectors are already 44.1 kHz little-endian signed 16-bit stereo PCM: 2352 bytes = 588 samples,
588*4 bytes.  Nothing is decoded, re-sampled or de-interleaved here; the mixer resamples at play time.

Run:  python3 scripts/pocket/repack.py            (or: make -C pocket/src/night data)
"""
from __future__ import annotations

import hashlib
import json
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
NIGHT_DIR = REPO / "NIGHT"
CUE = NIGHT_DIR / "TG_NIGHT.CUE"
IMG = NIGHT_DIR / "TG_NIGHT.IMG"
RES = NIGHT_DIR / "NIGHT.RES"
OUT = REPO / "scratch" / "pocket"

SECTOR = 2352
PREGAP = 150          # the CUE's single PREGAP 00:00:02, in sectors
SLOT_BASE = 20        # scripts/pocket/mk_dist.py: audio track 02 -> slot 20
SLOT_TRACK0 = 2


def msf_to_frames(t: str) -> int:
    m, s, f = (int(x) for x in t.split(":"))
    return m * 4500 + s * 75 + f


def parse_cue(path: Path):
    """Return [(track, mode, disc_lba)] with PREGAP folded in, the same way src/cd/cd_iso.c does."""
    tracks, cur, mode, first, pregap = [], 0, "", None, 0
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if line.startswith("TRACK "):
            _, tn, m = line.split()
            if cur and first is not None:
                tracks.append((cur, mode, msf_to_frames(first) + pregap))
            cur, mode, first = int(tn), m, None
        elif line.startswith("PREGAP "):
            pregap += msf_to_frames(line.split()[1])
        elif line.startswith("INDEX "):
            parts = line.split()
            if int(parts[1]) == 1 and first is None:
                first = parts[2]
    if cur and first is not None:
        tracks.append((cur, mode, msf_to_frames(first) + pregap))
    if not tracks:
        sys.exit(f"{path}: no TRACK lines")
    return tracks, pregap


def sha1_head(path: Path, n: int = 1 << 20) -> str:
    h = hashlib.sha1()
    with path.open("rb") as f:
        read = 0
        while read < n:
            b = f.read(min(1 << 20, n - read))
            if not b:
                break
            h.update(b)
            read += len(b)
    return h.hexdigest()[:12]


def main() -> int:
    for p in (CUE, IMG):
        if not p.is_file():
            sys.exit(f"missing {p} -- the game data is user-supplied and lives outside the repo")

    tracks, pregap = parse_cue(CUE)
    img_sectors = IMG.stat().st_size // SECTOR
    total = img_sectors + pregap
    audio = [(t, lba) for t, _m, lba in tracks if t != tracks[0][0]]
    if not audio:
        sys.exit("no AUDIO tracks in the CUE")

    cd_dir = OUT / "cd"
    cd_dir.mkdir(parents=True, exist_ok=True)
    rows = []

    # ---- the resource container: copied untouched, it is the container the loader parses ----
    res_dst = OUT / RES.name
    if RES.is_file():
        if res_dst.is_file() and res_dst.stat().st_size == RES.stat().st_size:
            print(f"RES  : {res_dst} already {RES.stat().st_size} bytes, skipped")
        else:
            print(f"RES  : {RES} -> {res_dst} ({RES.stat().st_size} bytes)")
            shutil.copy2(RES, res_dst)
        rows.append({"slot": 4, "file": res_dst.relative_to(OUT).as_posix(),
                     "bytes": res_dst.stat().st_size, "sha1_1MiB": sha1_head(res_dst)})
    else:
        print(f"RES  : {RES} missing -- the core cannot boot without it")

    # ---- one file per audio track ----
    lbas = [lba for _t, lba in audio]
    for i, (track, lba) in enumerate(audio):
        # The IMG begins at disc LBA 150 (the pregap is not in the file).
        start_img = lba - pregap
        end_lba = lbas[i + 1] if i + 1 < len(lbas) else total
        end_img = end_lba - pregap
        n_sectors = end_img - start_img
        if start_img < 0 or n_sectors <= 0 or end_img > img_sectors:
            sys.exit(f"track {track:02d}: bad range start={start_img} end={end_img} "
                     f"(img sectors {img_sectors})")
        dst = cd_dir / f"track{track:02d}.bin"
        tmp = dst.with_suffix(".bin.tmp")
        with IMG.open("rb") as src, tmp.open("wb") as out:
            src.seek(start_img * SECTOR)
            remaining = n_sectors * SECTOR
            while remaining:
                chunk = src.read(min(remaining, 4 << 20))
                if not chunk:
                    break
                out.write(chunk)
                remaining -= len(chunk)
            if remaining:
                sys.exit(f"track {track:02d}: short read, {remaining} bytes missing")
        tmp.replace(dst)
        slot = SLOT_BASE + (track - SLOT_TRACK0)
        print(f"track{track:02d}: slot {slot} {n_sectors} sectors "
              f"({n_sectors / 75:.1f} s) img[{start_img}..{end_img - 1}] -> {dst.name}")
        rows.append({"slot": slot, "file": dst.relative_to(OUT).as_posix(), "track": track,
                     "sectors": n_sectors, "img_first_sector": start_img, "disc_lba": lba,
                     "bytes": dst.stat().st_size, "sha1_1MiB": sha1_head(dst)})

    manifest = OUT / "manifest.tsv"
    with manifest.open("w") as f:
        f.write("slot\tfile\tbytes\ttrack\tsectors\timg_first_sector\tdisc_lba\tsha1_1MiB\n")
        for r in rows:
            f.write("\t".join(str(r.get(k, "")) for k in
                              ("slot", "file", "bytes", "track", "sectors",
                               "img_first_sector", "disc_lba", "sha1_1MiB")) + "\n")
    print(f"manifest: {manifest}")

    # The run-time table has to agree with these numbers; regenerate it in the same breath.
    import subprocess
    subprocess.run([sys.executable, str(REPO / "scripts" / "pocket" / "gen_disc.py")], check=True)
    subprocess.run([sys.executable, str(REPO / "scripts" / "pocket" / "mk_dist.py")], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
