"""Where the disc and the game's own files are: found by suffix, not by name (docs/455).

Twenty-odd scripts hard-coded `NIGHT/TG_NIGHT.IMG` or `NIGHT/NIGHT.EXE`, and `NIGHT/` was one rip's
directory name rather than a rule -- when the tree was reorganized, every one of them broke at import.
Same idea as scripts/extract_disc.py's discovery: the repo root first, then a `NIGHT/` under it (a rip
still works), then the current directory; matched by suffix, case-insensitively, sorted so the answer
is the same twice.  Every caller still takes an explicit path when one is given, and the environment
(`NIGHT_EXE`) still wins for the rigs that name their own binary.

Run it directly to resolve a path from a shell or a config:

    python3 scripts/disc_loc.py cue | img | res | exe
"""
from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]

# Where to look, in order.  A `NIGHT/` is still searched -- it is what a rip directory is called in most
# of the documents -- it is simply no longer the only answer.
BASES = [REPO, REPO / "NIGHT", Path.cwd(), Path.cwd() / "NIGHT"]
RES_BASES = [REPO / "extracted", REPO / "NIGHT", REPO, Path.cwd()]


def _first(dirs, suffixes):
    for d in dirs:
        if not d.is_dir():
            continue
        for suf in suffixes:
            for c in sorted(p for p in d.iterdir() if p.is_file() and p.suffix.lower() == suf):
                return c
    return None


def find_cue(extra: Path | None = None) -> Path | None:
    """The disc's CUE sheet: an explicit argument wins, then repo, repo/NIGHT, ./NIGHT, the cwd."""
    if extra is not None:
        return Path(extra)
    return _first(BASES, [".cue"])


def find_img(cue: "Path | str | None" = None, extra: Path | None = None) -> Path | None:
    """The raw 2352-byte-frame image: an explicit argument, then the CUE's own FILE line, then .img/.bin
    beside the CUE and in the same bases find_cue() searched."""
    if extra is not None:
        return Path(extra)
    if cue is not None:
        cue = Path(cue)
        text = cue.read_text(errors="replace")
        for line in text.splitlines():
            parts = line.split()
            if parts and parts[0].upper() == "FILE" and len(parts) >= 2:
                named = cue.parent / parts[1].strip('"')
                if named.exists():
                    return named
    return _first(([cue.parent] if cue is not None else []) + BASES, [".img", ".bin"])


def find_res(extra: Path | None = None) -> Path | None:
    """NIGHT.RES, whatever case the tree happens to keep it in -- the container's own index spells the
    six install entries lower case, `make extract` writes the spelling the disc uses (docs/455)."""
    if extra is not None:
        return Path(extra)
    for d in RES_BASES:
        if not d.is_dir():
            continue
        for c in sorted(p for p in d.iterdir() if p.is_file() and p.suffix.lower() == ".res"):
            return c
    return None


def find_exe(extra: Path | None = None) -> Path | None:
    """NIGHT.EXE -- the shipped DOS binary, which every disassembly probe in scripts/analysis reads.

    The installer's payload and the ISO copy are the same file, and `make extract` writes the former to
    extracted/NIGHT.EXE, so the probes keep working from the extracted tree.  NIGHT_EXE still names a
    particular copy for a run that must say which one it measured.
    """
    if extra is not None:
        return Path(extra)
    env = __import__("os").environ.get("NIGHT_EXE")
    if env:
        return Path(env)
    for d in RES_BASES:
        if not d.is_dir():
            continue
        for c in sorted(p for p in d.iterdir() if p.is_file() and p.name.lower() == "night.exe"):
            return c
    return None


def main() -> int:
    what = sys.argv[1] if len(sys.argv) > 1 else "cue"
    got = {"cue": find_cue, "img": lambda: find_img(find_cue()), "res": find_res, "exe": find_exe}[what]()
    if got is None:
        sys.exit(f"no {what} found under {', '.join(str(b) for b in BASES)}")
    print(got)
    return 0


if __name__ == "__main__":
    sys.exit(main())
