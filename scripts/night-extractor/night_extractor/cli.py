"""The `night-extractor` command line: find the disc, run the three parts, write the manifests.

`main()` is the console-script entry point (pyproject.toml) and `__main__.py` gives the same thing to
`python3 -m night_extractor`.  scripts/extract_disc.py calls straight into here.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import __doc__ as PKG_DOC, __version__
from .disc import SECTOR, DataTrack, parse_cue
from .extract import (KIND_ORDER, cross_check_installed, load_manifest, load_prev, part_audio, part_cue,
                      part_game, verify, write_manifests)


def first_by_suffix(dirs, suffixes):
    """(file, dir) for the first file carrying one of `suffixes` (case-insensitive), in name order.

    Suffix order dominates over directory order, so a .img anywhere beats a .bin somewhere else.

    "Any name" is the contract: a rip is not necessarily called TG_NIGHT.*, and neither is a disc
    anyone hands this script. What has to stay deterministic is WHICH one: sorted(), because the
    directory order os.listdir reports can differ between two runs of the same directory, and a tool
    that picks a different image on the second run is worse than one that asks. Raw disc images also
    go by .bin in the wild, and a directory full of per-track .bin files would then be a minefield --
    which is why the CUE's own FILE line is tried before any of this, and why the chosen file is
    printed with the rule that found it.
    """
    for suffix in suffixes:
        for d in dirs:
            try:
                cands = sorted(p for p in Path(d).iterdir() if p.is_file() and p.suffix.lower() == suffix)
            except OSError:
                continue
            if cands:
                return cands[0], Path(d)
    return None, None


def find_disc(cue_arg, img_arg):
    """(cue, cue_how, img, img_how) -- located by suffix, never by file name.

    The current directory first, because that is where a rip lives.  A `NIGHT` subdirectory of it next,
    because that is where this repo keeps its disc, and `night-extractor` with no arguments from the
    repo root is the invocation that ought to work: telling the user to cd somewhere is a worse answer
    than finding the one .cue one level down and printing that it did.  The directory that produced the
    cue is then part of the image search, so a rip with both files in `NIGHT/` needs no arguments
    either.
    """
    cwd = Path.cwd()
    if cue_arg:
        cue, cue_how = Path(cue_arg).expanduser(), "given"
        if not cue.exists():
            sys.exit(f"no cue at {cue}")
        base = [cue.parent, cwd, cwd / "NIGHT"]
    else:
        cue = cue_how = None
        base = [cwd, cwd / "NIGHT"]
        for d in base:
            found, where = first_by_suffix([d], [".cue"])
            if found is not None:
                cue, cue_how = found, f"first .cue in {where}"
                break
        if cue is None:
            sys.exit(f"no .cue in {cwd} or {cwd / 'NIGHT'} -- cd to the directory holding the disc "
                     f"image, or pass --cue FILE")
    if img_arg:
        img, img_how = Path(img_arg).expanduser(), "given"
        if not img.exists():
            sys.exit(f"no disc image at {img}")
        return cue.resolve(), cue_how, img.resolve(), img_how

    tracks, _pregap, data_file = parse_cue(cue)          # the CUE's own FILE line is the pairing that
    named = cue.parent / data_file if data_file else None  # cannot be wrong, so it is tried first
    if named is not None and named.exists():
        return cue.resolve(), cue_how, named.resolve(), "named by the cue's FILE line"
    img, where = first_by_suffix(base, [".img", ".bin"])
    if img is None:
        sys.exit(f"no disc image beside {cue} or in {' / '.join(str(b) for b in base)} -- pass --img FILE")
    return cue.resolve(), cue_how, img.resolve(), f"first .img/.bin in {where}"


# ---------------------------------------------------------------- main


def main() -> int:
    ap = argparse.ArgumentParser(description=PKG_DOC.splitlines()[0],
                                 epilog="the formats and the measurements behind it: the night_extractor "
                                        "package docstring, and docs/455 in the port's repo")
    ap.add_argument("--version", action="version", version=f"night-extractor {__version__}")
    ap.add_argument("--cue", help="the CUE sheet (default: the first .cue in the current directory, then "
                                  "in ./NIGHT -- any name)")
    ap.add_argument("--img", help="the raw 2352-byte-frame image (default: the CUE's FILE line, then the "
                                   "first .img or .bin beside it -- any name)")
    ap.add_argument("-o", "--out", default="extracted", help="output directory (default: ./extracted)")
    ap.add_argument("--res", help="installed dir to cross-check against (default: the CUE's directory)")
    ap.add_argument("--only", default="all", choices=["all", "game", "audio", "cue"])
    ap.add_argument("--force", action="store_true", help="rewrite files that already look right")
    ap.add_argument("--verify", action="store_true", help="re-hash the output tree against MANIFEST.tsv and exit")
    args = ap.parse_args()

    out = Path(args.out)
    if args.verify:                          # needs no disc: it is a statement about the output tree
        return verify(out)

    cue, cue_how, img_path, img_how = find_disc(args.cue, args.img)
    res_dir = Path(args.res) if args.res else cue.parent
    tracks, pregap, _data_file = parse_cue(cue)
    img_sectors = img_path.stat().st_size // SECTOR
    audio = [t for t in tracks if t[1].startswith("AUDIO")]
    data = [t for t in tracks if not t[1].startswith("AUDIO")]
    if not audio:
        sys.exit("the cue lists no AUDIO tracks")
    # The data track runs to the first audio track's INDEX 01 -- the limit scripts/cd2iso.py hard-codes.
    data_sectors = (audio[0][3] - data[0][3]) if data else img_sectors
    print(f"disc    cue {cue} ({cue_how}): {len(tracks)} tracks ({len(data)} data + {len(audio)} audio)")
    print(f"disc    img {img_path} ({img_how}) = {img_sectors} sectors "
          f"({img_path.stat().st_size} B), PREGAP {pregap}, output {out.resolve()}")

    out.mkdir(parents=True, exist_ok=True)
    prev = load_prev(out)
    rows = []
    with img_path.open("rb") as img:
        if args.only in ("all", "game"):
            if data:
                rows += part_game(img, out, DataTrack(img, data[0][3], data_sectors), args, prev)
            else:
                print("[game] no MODE1 data track in the cue -- nothing to unpack")
        if args.only in ("all", "audio"):
            rows += part_audio(img, out, audio, pregap, img_sectors, args, prev)
        if args.only in ("all", "cue"):
            rows += part_cue(cue, out, args)
    # A manifest describes the tree, not the last invocation: rows for the parts this run did not write
    # come from the previous manifest, and only the parts it did write are replaced.
    written = {r["kind"] for r in rows}
    merged = [r for r in load_manifest(out) if r["kind"] not in written] + rows
    merged.sort(key=lambda r: (KIND_ORDER.index(r["kind"]) if r.get("kind") in KIND_ORDER else 9,
                               r["file"]))
    write_manifests(out, merged)

    total = sum(r["bytes"] for r in rows)
    print(f"\nwrote {len(rows)} files, {total} bytes -> {out}")
    if args.only in ("all", "game"):
        cross_check_installed(out, rows, res_dir)
    print(f"\nthis is what the port now boots from by default:  ./build/night")
    print(f"  (--res-dir {out} to name it; audio resolves from {out}/cd/trackNN.bin, NIGHT_CD_DIR overrides)")
    return 0


if __name__ == "__main__":
    sys.exit(main())


if __name__ == "__main__":
    sys.exit(main())
