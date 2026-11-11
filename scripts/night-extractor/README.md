# night-extractor

Unpacks a NIGHT (T.G. Entertainment, 1998) CD image into a directory the port can run from -- the
installer's payload plus one raw-frame file per CD-DA track, so no 633 MB disc image is read at run time.
The format notes, the measurements and the proof are in `docs/455-disc-extraction-installer-free-run.md`
at the repo root; this file is only the install/run half of that story.

## Install

From the repo root, into its venv (the root `pyproject.toml` declares this directory as a path
dependency, so `uv sync` does it for you):

    uv sync
    uv run night-extractor --help

Standalone, anywhere:

    uv tool install path/to/night-extractor
    pip install -e path/to/night-extractor

With no install at all, the repo's own entry point is a shim into this package:

    python3 scripts/extract_disc.py      # same code, no venv required

## Run

    cd /path/to/the/rip && night-extractor        # -> ./extracted, nothing else to type
    night-extractor --cue GAME.CUE --img GAME.BIN -o out
    night-extractor --only audio --force          # re-cut just the tracks
    night-extractor --verify                      # re-hash the output tree against MANIFEST.tsv

The disc is found by suffix and not by name: the first `.cue` in the current directory, then in `./NIGHT`;
the image from that CUE's own `FILE` line, then the first `.img`, then `.bin`.  Every choice is printed
with the rule that made it.

    ./build/night                                 # boots from ./extracted by default (docs/455)
    ./build/night --res-dir DIR                   # any directory holding night.res + TG_NIGHT.CUE; audio
                                                  # comes from DIR/cd/trackNN.bin, or from the image named
                                                  # by that CUE's FILE line when there is no DIR/cd/

## Layout

    night_extractor/disc.py      CUE geometry, the MODE1/2352 ISO9660 reader, the container format
    night_extractor/extract.py   streaming copies, the three parts, the manifests, --verify
    night_extractor/cli.py       discovery and the argument line (`python3 -m night_extractor` works too)
