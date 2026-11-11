"""Unpack the shipped disc into extracted/ -- the DOS installer's job, plus the CD-DA the port plays.

WHY THIS EXISTS.  Playing the game the original way needs three things: an ISO9660 volume, a DOS
InstallShield v3 SETUP.EXE to unpack TG_NIGHT.DAT into the game directory, and a mounted disc
(or TG_NIGHT.IMG) for the music.  All three are scaffolding.  This script does the useful part of the
first two and cuts the third down to size, so the port runs from one flat directory with no 633 MB
image behind it:

    extracted/night.res       the resource container the game opens as "night.res" (fn_03690b)
    extracted/night.exe       the rest of the install set, byte-for-byte what SETUP.EXE installs
    extracted/dos4gw.exe      (the port runs none of these; they are the install set, complete)
    extracted/night.ico / night.bat / readme.txt
    extracted/TG_NIGHT.CUE    copied: the TOC the MSCDEX shim answers from (src/cd/cd_iso.c)
    extracted/cd/trackNN.bin  one CD-DA track per file: raw 2352-byte frames, no header -- the SAME
                              byte ranges scripts/pocket/repack.py cuts for the Pocket's APF slots
    extracted/cd/manifest.tsv track, sectors, disc LBA, IMG sector range, sha256 (audit_disc.py maps a
                              TRACKnn audit onto the IMG with it; its slot column is blank here)
    extracted/MANIFEST.tsv    every file above: bytes, sha256, and the disc range it came from

THE FORMATS, measured on TG_NIGHT.IMG rather than taken from documentation:

  * TRACK 01 of TG_NIGHT.CUE is MODE1/2352: 2352-byte frames whose ISO9660 user data starts at offset
    16.  Reading user data of frames 16..22 gives a standard volume descriptor ("CD001" at +1 of the
    user data, root directory record at +156) whose tree holds NIGHT/DISK1.ID, NIGHT/NIGHT.DAT (volume
    LBA 11780, 51 563 830 B) and NIGHT/NIGHT.TGS (LBA 500, 23 100 441 B).
  * TG_NIGHT.DAT is not an installer-only format at all: it is one NIGHT.RES container
    (docs/02-container.md) holding count = 6 -- the reverse-XOR-chain directory (b[i] ^= b[i-1] walking
    backwards, then b[0] ^= 0x20), then the blobs contiguous and uncompressed.  Its six entries are
    exactly the install set, and night.res as decoded here comes out byte-identical to the installed
    copy the repo already ships in extracted/ -- the run re-measures that, it is not assumed.
  * TG_NIGHT.TGS is the installer's own compressed twin of the same six files (entropy 7.97, a name
    table at EOF listing DOS4GW.EXE, NIGHT.BAT, NIGHT.EXE, NIGHT.ICO, NIGHT.RES, README.TXT).  Nothing
    here needs it: NIGHT.DAT yields the install set verbatim.
  * The audio tracks need no decoding.  CD-DA frames are 44.1 kHz little-endian signed 16-bit stereo
    PCM: 2352 bytes = 588 audio frames.  TG_NIGHT.IMG carries no PREGAP while the CUE's LBAs (and the
    game's) are MSCDEX disc coordinates that do, so a track's first IMG byte is (disc_lba - 150) *
    2352 -- the same -150 cd_iso.c applies and the fixture suite asserts (docs/96).

WHAT IT DOES NOT TOUCH: the disc image stays where it is, so every comparison basis that reads it
(scripts/pocket/audit_disc.py, the docs/419 asset audit, the fixture suite's real-disc test) keeps
working, and the two disc backends can be A/B'd against each other byte for byte.

The disc is found where you are standing -- by name, not by convention: the first .cue in the current
directory (any name), the image from that CUE's own FILE line, and failing that the first .img (then
.bin) beside the CUE or in the current directory. Everything found is printed with the rule that found
it, so a pick that is wrong is visible in the first line rather than in the output.

    cd /path/to/the/rip && night-extractor                          # -> ./extracted, nothing to type
    night-extractor --cue GAME.CUE --img GAME.BIN -o out            # or say what you mean
    night-extractor --only audio --force                            # re-cut just the tracks
    night-extractor --verify                                        # re-hash the output tree

    python3 scripts/extract_disc.py                                 # the same tool with no install step

The two calls are the same code: `scripts/extract_disc.py` is a shim that imports this package out of
`scripts/night-extractor/`, so neither the repo's own `make extract` nor any citation in a document or a
C comment depends on the package being installed.  The console script comes from this directory's
pyproject.toml (`uv pip install -e scripts/night-extractor`, or `uv tool install`), and
`python3 -m night_extractor` works as well.

The disc is found where you are standing -- the current directory first, then a `NIGHT` subdirectory of
it, which is where this repo keeps its disc, so `night-extractor` with no arguments from the repo root
does the expected thing, and prints which rule found what.
"""

__version__ = "0.1.0"
