#!/usr/bin/env python3
"""Unpack the shipped disc into extracted/ -- the DOS installer's job, plus the CD-DA the port plays.

The implementation lives in the `night_extractor` package under scripts/night-extractor/, which is also
what installs the `night-extractor` command line (docs/455, scripts/night-extractor/README.md).  This
file stays as the repo's entry point -- `make extract`, the docs and the C comments all cite it -- and it
deliberately needs no install step: it puts scripts/night-extractor on sys.path and calls the same
`main()`.  The local package wins over an installed copy, so `make extract` always runs the code in the
working tree.

    python3 scripts/extract_disc.py                     # the first .cue in the current directory
    python3 scripts/extract_disc.py --cue D.CUE --img D.BIN -o out
    python3 scripts/extract_disc.py --verify

Run `night-extractor --help` (once installed) for the full option list; the behaviour is identical.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "night-extractor"))

from night_extractor.cli import main  # noqa: E402  (the path above is what makes this importable)

if __name__ == "__main__":
    sys.exit(main())
