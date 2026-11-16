"""The `night-extractor` command line: find the data, run the three parts, write the manifests.

`main()` is the console-script entry point (pyproject.toml) and `__main__.py` gives the same thing to
`python3 -m night_extractor`.  scripts/extract_disc.py calls straight into here.

What the tool is: the original game ships as a CD image whose content is a DOS installer, so playing it the
original way means mounting a disc and running `SETUP.EXE`.  This does the installer's useful part -- out
of the ISO9660 payload, byte for byte -- and cuts the CD-DA the port plays into one file per track, so
nothing downstream ever needs a disc image again.

What it takes in is whatever the game data is *called* today (see `source.py`, and docs/464/docs/465): a
directory holding a CUE and its image, a zip of any of those, the two files picked by name, an image with no
cue at all, or a `NIGHT.RES` handed over directly.  Nothing is decided by our own file names, and every
decision is printed with the rule that made it.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import __doc__ as PKG_DOC, __version__
from .disc import SECTOR
from .extract import (KIND_ORDER, cross_check_installed, load_manifest, load_prev, part_audio, part_cue,
                      part_game, verify, write_manifests)
from .slots import CARD_DIR
from .source import CACHE_NAME, default_paths, open_source


# ---------------------------------------------------------------- main


def main() -> int:
    ap = argparse.ArgumentParser(description=PKG_DOC.splitlines()[0],
                                 epilog="the formats and the measurements behind it: the night_extractor "
                                        "package docstring, and docs/455, docs/464 and docs/465 in the "
                                        "port's repo")
    ap.add_argument("--version", action="version", version=f"night-extractor {__version__}")
    ap.add_argument("--data", nargs="+", metavar="PATH",
                    help="the game data: a directory, a zip, a CUE and its image, a bare image, or "
                         "NIGHT.RES alone -- any names, several paths at once (default: the current "
                         "directory, then ./NIGHT)")
    ap.add_argument("--cue", help="the CUE sheet to use, when the data holds more than one")
    ap.add_argument("--img", help="the raw 2352-byte-frame image to use, when the data holds more than one")
    ap.add_argument("-o", "--out", default="extracted", help="output directory (default: ./extracted)")
    ap.add_argument("--res", help="installed directory to cross-check the install set against "
                                  "(default: the directory the data came out of)")
    ap.add_argument("--cache", help=f"where a compressed image is unpacked once for random access "
                                    f"(default: ./{CACHE_NAME} in the current directory)")
    ap.add_argument("--wav", action="store_true", help="also write each CD-DA track under wav/ as a 44.1 kHz "
                    "stereo 16-bit WAV -- the same bytes as cd/trackNN.bin, with a header, so you can listen "
                    "to what was cut")
    ap.add_argument("--only", default="all", choices=["all", "game", "audio", "cue"])
    ap.add_argument("--force", action="store_true", help="rewrite files that already look right")
    ap.add_argument("--verify", action="store_true", help="re-hash the output tree against MANIFEST.tsv and exit")
    args = ap.parse_args()

    out = Path(args.out)
    if args.verify:                          # needs no disc: it is a statement about the output tree
        return verify(out)

    paths = [Path(p) for p in args.data] if args.data else default_paths()
    src = open_source(paths, cue=args.cue, img=args.img, cache_dir=args.cache).describe()
    res_dir = Path(args.res) if args.res else next((p.parent for p in paths if p.is_file()),
                                                   paths[0] if paths[0].is_dir() else Path("."))

    out.mkdir(parents=True, exist_ok=True)
    prev = load_prev(out)
    rows = []
    if args.only in ("all", "game"):
        rows += part_game(out, src, args, prev)
    if args.only in ("all", "audio"):
        rows += part_audio(out, src, args, prev)
    if args.only in ("all", "cue"):
        rows += part_cue(out, src, args)
    # A manifest describes the tree, not the last invocation: rows for the parts this run did not write
    # come from the previous manifest, and only the parts it did write are replaced.
    written = {r["kind"] for r in rows}
    merged = [r for r in load_manifest(out) if r["kind"] not in written] + rows
    merged.sort(key=lambda r: (KIND_ORDER.index(r["kind"]) if r.get("kind") in KIND_ORDER else 9,
                               r["file"]))
    write_manifests(out, merged)

    total = sum(r["bytes"] for r in rows)
    print(f"\nwrote {len(rows)} files, {total:,} bytes -> {out}")
    if args.only in ("all", "game"):
        cross_check_installed(out, rows, res_dir)
    print(f"\nthis is what the port now boots from by default:  ./build/night")
    print(f"  (--res-dir {out} to name it; audio resolves from {out}/cd/trackNN.bin, NIGHT_CD_DIR overrides)")
    if any(r.get("slot") for r in merged):
        print(f"\nand the same files are the Analogue Pocket core's data slots -- copy them flat into "
              f"<card>/{CARD_DIR}/")
        print(f"  {out}/NIGHT.RES -> slot 4, and {out}/cd/trackNN.bin -> slot as listed in "
              f"{out}/slot_map.tsv")
    return 0


if __name__ == "__main__":
    sys.exit(main())
