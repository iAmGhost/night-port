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
    night-extractor --data GAME.CUE GAME.BIN -o out
    night-extractor --data ~/Downloads/repack.zip -o out
    night-extractor --data NIGHT.RES MUSIC.RES SOUND.RES -o out   # a fan repack, unzipped
    night-extractor --data DISC.IMG -o out        # a bare image: this port's own table of contents
    night-extractor --only audio --force          # re-cut just the tracks
    night-extractor --wav                         # and write each track as 44.1 kHz WAV under wav/
    night-extractor --verify                      # re-hash the output tree against MANIFEST.tsv

`--data` takes any shape a person owns, and says which one it recognised: a CUE and the image its `FILE`
line names (any names), a rip directory, a `.zip` read without unpacking it, a fan repack's loose files, a
bare image, or `NIGHT.RES` alone.  `scripts/analysis/check_input_shapes.py` is the test for that, and a
disc shape reproduces `extracted/` byte for byte.

A fan repack's music gets a warning, because it is earned: its audio matches the shipped disc byte for byte
on five of fourteen tracks and drifts inside the others (docs/466 has the per-track table).  `--wav` is how
to hear what that means.  The same cuts, with the data-slot ids they are bound to, are what fills an Analogue
Pocket card: see `pocket/INSTALL.md` section 0 and `scripts/pocket/repack.py`.

The disc is found by suffix and not by name: the first `.cue` in the current directory, then in `./NIGHT`;
the image from that CUE's own `FILE` line, then the first `.img`, then `.bin`.  Every choice is printed
with the rule that made it.

    ./build/night                                 # boots from ./extracted by default (docs/455)
    ./build/night --res-dir DIR                   # any directory holding night.res + TG_NIGHT.CUE; audio
                                                  # comes from DIR/cd/trackNN.bin, or from the image named
                                                  # by that CUE's FILE line when there is no DIR/cd/

## Layout

    night_extractor/disc.py      CUE geometry, the MODE1/2352 ISO9660 reader, the container format
    night_extractor/source.py    the input layer: containers, the fan-vs-disc probe, where each track sits
    night_extractor/extract.py   streaming copies, the three parts, the manifests, --verify
    night_extractor/slots.py     the APF data-slot ids the Pocket binds these files to
    night_extractor/wav.py       the RIFF header that turns a raw cut into something you can listen to
    night_extractor/cli.py       discovery and the argument line (`python3 -m night_extractor` works too)
