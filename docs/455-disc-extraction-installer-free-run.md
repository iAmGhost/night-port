# 455 — The disc has three layers, and only one of them is needed to play the game

**Goal:** replace the DOS installer with a script, and stop needing `TG_NIGHT.IMG` for the music.
Both are done: `scripts/extract_disc.py` writes `extracted/` (570 MB, git-ignored, regenerable), and
`./build/night --res-dir extracted` boots, passes the disc gate and streams the opening theme with no
image on disk. The install set comes out **byte-identical to the installed copies**, which is the claim
that makes "replaces the installer" worth anything.

## What the disc actually is

`TG_NIGHT.CUE` describes 15 tracks in one `TG_NIGHT.IMG` of 269 303 raw 2352-byte frames
(633 400 656 B). Layer by layer, measured rather than assumed:

| layer | what it holds | how it was read |
|---|---|---|
| TRACK 01 `MODE1/2352`, disc LBA 0 | ISO9660, user data at frame offset 16 | volume descriptor at logical sector 16, root directory record at +156 |
| `NIGHT/NIGHT.DAT`, volume LBA 11780, 51 563 830 B | **a NIGHT.RES container with `count = 6`** — the install set, uncompressed | the reverse-XOR-chain directory `b[i] ^= b[i-1]` walking backwards, `b[0] ^= 0x20` |
| `NIGHT/NIGHT.TGS`, volume LBA 500, 23 100 441 B | the installer's *compressed* twin of the same six files (entropy 7.97, a name table at EOF listing `DOS4GW.EXE NIGHT.BAT NIGHT.EXE NIGHT.ICO NIGHT.RES README.TXT`) | not decoded; not needed |
| TRACKS 02–15 `AUDIO` | CD-DA: 2352 B = 588 stereo 16-bit little-endian frames, no decoding at all | cut along the CUE's own track boundaries |

`NIGHT.DAT` being a NIGHT.RES container (docs/02-container.md) is the interesting part: the installer
used the same container the game itself reads at run time, so unpacking it is one already-documented
decoder, not a new reverse-engineering job. `SETUP.LST` says `1; Night.tgs` and `SETUP.INS` installs
`Night.tgs` (`*.*`) into `Program Files\Night\`, so `SETUP.EXE`'s own source was `NIGHT.TGS` — but what
it *produced* is what `NIGHT.DAT` contains verbatim:

```
DOS4GW.EXE    265 420 B   dd9f4f342533    NIGHT.RES   50 964 894 B   eef68c5a4624
NIGHT.EXE     311 831 B   6cf96173537f    NIGHT.BAT          40 B   b8eb292cfd91
NIGHT.ICO       2 238 B   33d2f2cc4e87    README.TXT    19 283 B   362f3e032505
```

All six match the installed copies byte for byte — `extract_disc.py` prints that comparison itself
instead of asking to be believed. (`README.TXT` is 19 283 B in both, so the shipped one is the disc's
own, not a later edit.)

**The names are the disc's, which means upper case.** NIGHT.DAT's own index spells its six entries lower
case — `night.res`, `readme.txt` — and that is the spelling SETUP.EXE would have put on a DOS disk. The
disc spells them upper: ISO9660 forces A-Z, the installer's name table at the EOF of NIGHT.TGS lists
`DOS4GW.EXE NIGHT.BAT NIGHT.EXE NIGHT.ICO NIGHT.RES README.TXT`, and the string NIGHT.EXE prints when
the container is missing is `"NIGHT.RES File Not Found!!!"` (`src/f/f.c:2915`). Bytes come out of a CD
image, so they keep the image's spelling; the container index's spelling is recorded per file in the
note column of `MANIFEST.tsv`.

What *opens* the file is the game's own lower-case string — `fn_03690b` does `open("night.res")`
(`src/f/f.c:2907`, straight out of the executable). So the seam answers a DOS name the way DOS did:
try the name as given, and on a miss look it up in the resource directory (`sys_file_real_lookup()`,
`src/sys/sys.c:569`, guarded so the Pocket's slot-based host compiles it out). On the case-insensitive
APFS this runs on the two spellings are the same file; on a case-sensitive host — Linux — the fallback
is the difference between booting and the game's own not-found banner, and that nothing complained
earlier is a measurement of how much APFS forgives. `test_sys_file_case_insensitive` is the fixture.

## Two geometry traps, both caught by bytes rather than by reading

**A file is not its image.** The first version addressed `NIGHT.DAT`'s blobs as `img[lba*2352+16+off]`,
i.e. treated the ISO9660 file as contiguous bytes in the raw track. That yields the *right sizes with the
wrong bytes*: every blob past its first sector was 16 bytes short per sector travelled, and
the container came out 50 964 894 B and did not match. `IsoStream` in the script now maps file byte *N* to
`img[(first + lba + N/2048)*2352 + 16 + N%2048]` in one place, and the byte-identical line above is what
made that visible.

**A name that isn't there can be slower than one that is.** The directory walk neither skipped the
self/parent entries (ECMA-119 reserves identifiers `0x00`/`0x01`) nor remembered visited LBAs, so the
probing query `NIGHT/NIGHT.DAT` re-entered the root through `.` and `..` and re-read it 2^depth times —
four minutes before it was interrupted. A breadth-first walk with a visited set costs four sector reads.

The PVD's own offsets earned their comment: `TG_NIGHT` puts `CD001` at +1 of the user data rather than
the book's +2, so a remembered table would have read the wrong dwords and *still returned bytes*. The
root record is located by its shape instead — length, ext-attribute 0, both-byte fields whose LE and BE
halves agree, and a name length that makes the record exactly as long as it claims.

## `extracted/`, and why the audio is `cd/trackNN.bin`

```
extracted/NIGHT.RES . NIGHT.EXE . DOS4GW.EXE . NIGHT.ICO . NIGHT.BAT . README.TXT   51 563 706 B
extracted/TG_NIGHT.CUE                                                                669 B
extracted/cd/track02.bin .. track15.bin                                            546 113 232 B
extracted/cd/manifest.tsv, extracted/MANIFEST.tsv   (bytes, sha256, source range per file)
```

The tracks are **raw 2352-byte frames, one file per track, no header** — deliberately the same thing
`scripts/pocket/repack.py` already cuts for the Pocket's APF data slots, and checked as much:
`track02.bin`, `track07.bin` and `track15.bin` are byte-identical to `scratch/pocket/cd/`'s. One format
for both hosts, as asked. The byte ranges are the CUE's own track boundaries, which reproduce the
numbers the game uses: track 02 → 3153 sectors, and track 03 − track 02 = **0xc51** — the value the
disc-present gate `fn_010c3e` compares. The last track ends at 268 621 + 832 = 269 453, which is the
image's 269 303 sectors + the 150-sector PREGAP: the same lead-out either way.

Re-runs are idempotent (0.3 s: one stat per file, hashes carried from the previous manifest),
`--only game|audio|cue` and `--force` exist, and `--verify` re-hashes all 21 files against
`MANIFEST.tsv`. Files are written to `.part` and renamed, so an interrupted run cannot leave a
half-cut `NIGHT.RES` that the next run would trust by size.

## The port: one TOC, two byte sources

`src/cd/cd_iso.c` now answers audio reads from either source behind the same TOC:

* **IMG** — the image the CUE names: a disc LBA is `(lba − 150) * 2352` in the file, because the image
  carries no PREGAP. Still the default, so every existing basis (`audit_disc.py`, the docs/419 asset
  audit, the fixture suite) measures the same thing as before.
* **TRACKS** — `cd/trackNN.bin`: a file starts at its own track's INDEX 01, so the offset inside it is
  `(disc_lba − track_lba) * 2352`, **no −150** — that −150 exists only to convert a disc LBA into one big
  image's file sectors. `pocket/src/night/of_disc.c` makes the same point about its slots, which is why
  the two hosts' audio now differ in nothing but the handle they open.

Selection is one knob with an explicit ordering: `NIGHT_CD_DIR` given must work (a silent fallback would
let a rig measure the source it did not ask for); otherwise the image answers if it opens; only a
missing image sends the code looking for `cd/` (then `cdda/`) beside the CUE — which is exactly the
`--res-dir extracted` case. `cd_iso_source()` returns which one won, and the boot line prints it:

```
disc: 15 tracks, 269453 sectors, lead-out MSF-derived, audio from extracted/cd
```

Two refusals are part of the design, not accidents: a disc LBA inside the MODE1 track has no owning
file, so it is refused instead of answered with silence, and `cd_iso_read_sector()` is refused when the
image is absent — nothing in the port reads ISO user data (the resources come out of `NIGHT.RES`), so
there is no second source to fake it from.

The track files' own lengths are what bound the last track, since the CUE's last INDEX has nothing after
it; a track file longer than the span to the next INDEX is refused as a half-cut extraction rather than
silently clamped.

## What proves it

`test_cd_iso_tracks` (`test/test_f1.c`), 7 checks, run against the real disc and `extracted/cd`:

```
[PASS] the image answers every probe LBA, including the last frame of the last track
[PASS] the per-track files give the same total (269453) as the image, so the lead-out MSF is the same number
[PASS] the TOC header and the 15 entry records are identical from both sources -- fn_010c3e sees one disc
[PASS] the 5 probe LBAs come back byte-identical from cd/trackNN.bin as from the image
[PASS] a disc LBA inside the MODE1 track is refused (no file owns it) rather than answered with silence
[PASS] the first frame past the last track file is refused
[PASS] an ISO logical sector read is refused with no image present -- nothing in the port asks for one
```

The probe LBAs are the first and last frame of track 02 (the opening movie's track), the first frame of
track 03 (the boundary the 0xc51 sits on), and the first and last frame of the last track. The suite
still prints `== ALL PASS ==` with zero `[FAIL]` lines overall, and both new tests skip cleanly where
the disc or the extraction is absent.

Then the end-to-end run, which is the measurement that matters because it goes through the game's own
call chain: 500 frames from `extracted/` with `NIGHT_CD_AUDIT` on, audited by the repo's own disc
auditor against the untouched `TG_NIGHT.IMG`:

```
cdda: track index 1 lba=37262 sectors=3153 (42.0 s)
requests verified: 547  kinds: TRACK02=547
  track 02 coverage: 547 / 3153 sectors (17%)
OK every recorded read is byte-identical to TG_NIGHT.IMG
```

`audit_disc.py` needed one line for this: its manifest reader took the first line as the header, and
`extract_disc.py`'s manifest starts with a `#` explanation — which turned every request into "track 2 not
in manifest" and failed all 547. Comment lines are now skipped.

## Files and how to run it

The tool looks for the disc **by suffix and not by name**: the first `.cue` in the current directory
(sorted, so "first" means the same thing twice), then the first one in `./NIGHT`; the image from that
CUE's own `FILE` line, and only if that does not exist the first `.img` — then `.bin` — beside the CUE
or in the same places. `--res` for the install-set comparison defaults to the CUE's directory. Every
choice is printed with the rule that made it, so a wrong pick shows up in the first line instead of in
the output:

```
disc    cue /private/tmp/rip/GAME.CUE (first .cue in /private/tmp/rip): 15 tracks (1 data + 14 audio)
disc    img .../TG_NIGHT.IMG (first .img/.bin in /private/tmp/rip) = 269303 sectors, PREGAP 150
```

Suffix order beats directory order (a `.img` anywhere wins over a `.bin` somewhere else) and candidates
are sorted rather than taken in `listdir` order: picking a different image on the second run is worse
than asking. With nothing to find it says so instead of guessing. From the repo root `make extract`
passes no `--cue` at all — the rule finds the root's `TG_NIGHT.CUE`, and its `FILE` line names the
image sitting next to it.

```
cd /path/to/the/rip && night-extractor              # -> ./extracted, nothing to type
python3 scripts/extract_disc.py                     # the same code with no install step
night-extractor --cue GAME.CUE --img GAME.BIN -o out
make extract / make extract-verify                  # the repo's own disc, then re-hash it
./build/night                                       # boots from ./extracted, no IMG opened
NIGHT_CD_DIR=extracted/cd ./build/night --res-dir DIR   # per-track source while DIR keeps the IMG basis
```

The tool is a package (`scripts/night-extractor/`, `night-extractor` console script,
`python3 -m night_extractor`); `scripts/extract_disc.py` is a shim that puts that directory on
`sys.path` and calls the same `main()`, so nothing in the Makefile, the documents or the C comments
depends on the install. Its output layout is the same either way.

* Added: `scripts/extract_disc.py` (the shim), `scripts/night-extractor/` (`disc.py` the CUE/ISO9660/container
  layer, `extract.py` the writers and manifests, `cli.py` discovery and arguments), `test_cd_iso_tracks()`,
  `Makefile` (`extract`, `extract-verify`), `.gitignore`.
* Changed: `src/cd/cd_iso.{c,h}` (a second byte source, `cd_iso_source()`), `src/app/main.c` (the default
  `--res-dir`, and the boot log that names the source), `scripts/pocket/audit_disc.py` (manifest comment
  lines), `scripts/pocket/{gen_disc,mk_dist,repack}.py` (the disc is located, not hard-coded to `NIGHT/`).
* `TG_NIGHT.IMG` is left alone on purpose: it stays the comparison basis, and the two sources can be
  A/B'd against each other.
* `extracted/` is 570 MB, git-ignored, and regenerable in about 3 minutes; a re-run of an existing tree
  costs one `stat` per file (0.3 s measured) because the hashes come from its own manifest.

## Running with no disc data at all — the gate is bypassable

`fn_010c3e` polls the MSCDEX TOC until `DAT_00053c60 == 0xf` and the two records it compares differ by
`0xc51`. That is a signature check of the original pressing, and answering it meant parsing a CUE — which
made disc data a **start-up** requirement: a tree with nothing but `NIGHT.RES` painted `cdinsert.pcx`
forever and never began. The gate is now satisfied without any disc, and the music is simply not there.

`src/gen/disc_toc.h` carries the geometry — 15 tracks, their disc LBAs, the total 269 453 sectors — and
`cd_iso.c` loads it when there is no CUE to parse (or when `NIGHT_NO_DISC=1` asks for it). It is
generated by `scripts/gen/gen_disc_toc.py` from the real CUE and checked in, so **building never touches
the disc**; only re-running the generator does, and only for one number: a CUE sheet says nothing about
where the *last* track ends, so the total comes from the image's sector count, and the generator refuses
to guess it.

Nothing is invented with the TOC. With no byte source `cd_iso_source()` answers `"none"`,
`cd_iso_has_audio()` is 0, the app does not install the CDDA backend at all, and every
`cd_iso_read_audio()` / `cd_iso_read_sector()` is refused — the game's own play bookkeeping (`sys_cd`)
still counts the requests it makes. Boot says which case it is in:

```
disc: 15 tracks, 269453 sectors, lead-out MSF-derived, audio from none
disc: no disc in the house -- fn_010c3e is satisfied by the compiled-in TOC (src/gen/disc_toc.h);
      the game runs, there is no music. `make extract` for the tracks, NIGHT_REQUIRE_DISC=1 to stop here
```

### The two switches answer different questions

They compose, and getting their order wrong is what made this section wrong twice.

| `NIGHT_NO_DISC` | `NIGHT_REQUIRE_DISC` | what `cd_iso_open` does | what you see |
| --- | --- | --- | --- |
| | | parses `extracted/TG_NIGHT.CUE` | the game, with music |
| | set | parses it; a missing CUE is an error | the game, with music — requiring a disc is satisfied by the disc being there |
| set | | opens on the compiled-in TOC, refuses the bytes even though the cue is readable | the game, silent — the shipped-to-an-SSD path, measurable without moving 570 MB aside |
| set | set | fails: the drive is empty and nothing is substituted | **`cdinsert.pcx`, held** — the original's own answer to an empty drive |

`NIGHT_NO_DISC` answers *is there a disc to look at*; `NIGHT_REQUIRE_DISC` answers *may anything be
substituted for its absence*. Set both and you have the no-disc screen with the whole tree still on disk,
which is how the gate's failure branch gets looked at:

```
$ NIGHT_REQUIRE_DISC=1 NIGHT_NO_DISC=1 ./build/night --frames 300 --shots /tmp/both/shots
disc: the drive was treated as EMPTY (NIGHT_NO_DISC) and nothing is substituted for it
      (NIGHT_REQUIRE_DISC) -- fn_010c3e shows cdinsert.pcx and waits, as the original does
$ ls /tmp/both/shots | wc -l && md5 -q /tmp/both/shots/*.bmp | sort -u | wc -l
31
1                          # 31 captures, one distinct frame: it never leaves cdinsert.pcx
$ NIGHT_REQUIRE_DISC=1 NIGHT_NO_DISC=1 timeout 8 ./build/night; echo $?
124                        # still sitting on the screen when the timeout cut it off
```

An audit against the shipped image sets `NIGHT_REQUIRE_DISC=1` and *nothing else*: it then cannot measure
silence and call it a disc. `test_cd_iso_no_disc` asserts all four rows, including the one that used to
go the wrong way — a cue that was supposed to be hidden being read anyway, and a game booting past the
very screen the run was trying to see.

**A partial tree is tolerated too.** A missing `trackNN.bin` is recorded per track and refused per read,
so 13 files play 13 tracks; the boot log says `1 of 14 audio track files are not in extracted/cd`. The
one visible consequence is measured, not guessed: the last track's length is known only from its own
file, so with `track15.bin` gone the total the lead-out MSF is derived from falls from 269 453 to
268 621 — the end of the last track the CUE *can* bound.

The Pocket already worked this way: `pocket/src/night/disc_table.h` has always been the compiled-in
geometry, and `of_disc.c` returns -1 for a read whose slot file cannot be opened, which is "no music"
to its CDDA path. Its build reads no disc either; only `mk_dist.py`/`repack.py` need one, because
packing the music is the step that must read it. `scripts/pocket/disc_loc.py` now finds those files by
suffix instead of hard-coding `NIGHT/`, which is what four of them did and why they broke when the disc
moved.

`test_cd_iso_no_disc()` is the fixture: the geometry the header must carry, the refusals, `fn_010c3e`
entering and returning with no `cdinsert.pcx` and no CD stop, and both environment knobs. A 4 000-frame
boot from a directory holding only `NIGHT.RES` reaches the same card sequence (`card_168`) as the same
run on the complete tree, with `plays` counted and `cdda=0B` streamed.

## Not claimed

`NIGHT.TGS` is still undecoded — high entropy, a directory at EOF, described above only as far as the
evidence goes. The ISO reader is deliberately minimal (depth-limited, one FILE line, no Rock Ridge or
Joliet), and `cd_iso_read_sector()` cannot serve ISO sectors from the per-track layout or from no layout
at all. None of it is on the path the game walks: the resources come from `NIGHT.RES` and the music,
when there is any, from the track files.
