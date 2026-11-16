# 466 — A fan repack's music is this disc's music but not this disc's bytes, and now every route says which it got

The Pocket core reads no cue sheet and no image: the disc arrives as `NIGHT.RES` plus one file per audio
track (`trackNN.bin`), bound to data slots, and the game asks for audio by the **disc's** LBAs, which
`disc_table.h` compiled in from the disc's own table of contents (docs/330, docs/455).  So a question that
`docs/464` and `docs/465` had left implicit became sharp the moment someone asked to fill a card from a fan
repack instead of from a disc image:

> If the music comes from `MUSIC.RES` and gets cut into `track02.bin … track15.bin`, does the Pocket play
> this disc's music?

The answer measured here is: **the same recordings, sliced at offsets that hold on some tracks and drift
inside others.**  Five of fourteen tracks are byte-for-byte the shipped disc.  That is a better answer than
either claim people were working from — "a repack is the same disc, just repacked" and "a repack is a
different rip, throw it away" — and it changed four pieces of code.

## What the image actually is

The repack used for the measurements is the Internet Archive's *Eojjeonji Definitive for DOSBOX*
(`dosbox.zip`, 533 MB deflated), whose audio image is `MUSIC.RES`:

| | |
|---|---|
| bytes | 546,522,480 = 232,365 sectors of 2352 |
| CRC-32 | `36c386de` — the value the slice corrections in `plan_tracks` are guarded on |
| tracks | 14 CD-DA tracks, described by `SOUND.RES`, which wears a cue sheet's `.RES` name |
| first 174 sectors | the **disc's ISO9660 volume header** — primary, supplementary, and the path/volume descriptors, all readable |
| the volume's files | absent: `NIGHT.DAT` is not where the volume's own directory table says it is |

That last pair is the trap.  `byod.js` had been calling an image a fan rip when it opened with **no**
ISO9660 volume (`docs/464`), and this image *does* open with one — the rip kept the header and cut the
contents.  So the browser classified it as a disc, sliced it with the disc's numbers, and was 560 bytes late
on every track and 25.26 s late on tracks 08 and 09.  The rule is now "what the image *holds*":

```js
const fan = !!img && !(disc.volume && disc.res);   /* wasm/runtime/byod.js */
```

Python's mirror in `night_extractor/source.py` takes the same test (`src.fan = not src.volume or
src.res_in_image is None`), and both print why they decided what they decided.

## The measurement, and the two tools that made it

`scripts/night-extractor` was extended with `--wav`, which writes each cut track a second time under `wav/`
with a RIFF/WAVE header on the same bytes: 44.1 kHz stereo s16, which is what CD-DA already is, so nothing
is decoded.  Raw frames answer "is it the same bytes"; a WAV answers "would anyone notice", and only the
second one is a question a listener can settle alone.

`scripts/analysis/fan_audio_vs_disc.py` compares two trees of `cd/trackNN.bin` one second at a time and
reports the identical fraction, the first second that differs, and each side's RMS and peak (a level change
means different material; equal levels with different bytes means alignment):

A card copy list that leaves the container out is a card that does not boot, which is the bug this route
had for a while: `night-extractor` wrote its `slot_map.tsv` from the rows that carried a slot, and only the
audio rows did — the container row was written by the game part and carried none.  `NIGHT.RES` now carries
`slot 4` in its manifest row wherever it came from (handed over, or read out of the installer payload), so
the extractor's `slot_map.tsv` and the Pocket prep's agree line for line, and
`check_input_shapes.py` asserts it twice: that every track gets a distinct id from `AUDIO_IDS`, and that the
written `slot_map.tsv` binds the container plus every track.

```sh
python3 scripts/pocket/repack.py --data /tmp/nkzip/NIGHT.RES /tmp/nkzip/SOUND.RES /tmp/nkzip/MUSIC.RES \
       -o scratch/pocket_fan          # or night-extractor --wav for the listening copies
python3 scripts/analysis/fan_audio_vs_disc.py scratch/nkx/fan/cd extracted/cd
```

`extracted/cd` is the verified tree from the shipped disc (`make extract-verify`), so the comparison has a
side that is known good rather than merely older.

```
track      sec   identical  first diff          RMS A/B       peak A/B  verdict
track02.bin     42      100.0%           -    2133/2133      22078/22078    byte-identical
track03.bin    261      100.0%           -    3936/3936      24246/24246    byte-identical
track04.bin    228      100.0%           -    4478/4478      32768/32768    byte-identical
track05.bin    258      100.0%           -    3863/3863      23728/23728    byte-identical
track06.bin    243       99.6%         229    3379/3379      23414/23414    drifts after 229 s
track07.bin    286       43.7%         100    2842/3128      21204/24376    drifts after 100 s (91% level)
track08.bin    331      100.0%           -    4317/4317      27296/27296    byte-identical
track09.bin    294       53.1%         156    2893/3041      21402/21402    drifts after 156 s (95% level)
track10.bin    369       91.3%          20    2860/2860      20742/20742    drifts after 20 s
track11.bin    137       73.0%           0    1474/1482       9064/9064     drifts after 0 s
track12.bin    238       28.2%           0    3319/3319      21317/21317    drifts after 0 s
track13.bin    238       74.8%           0    3462/3462      17988/17988    drifts after 0 s
track14.bin    130       97.7%          83    2517/2517      15234/15234    drifts after 83 s
track15.bin     11       18.2%           2    3280/3196      12779/12779    drifts after 2 s (103% level)
```

The single-shift model the slice table already carried — `MUSIC_SHIFT = -560`, with
`MUSIC_EXTRA = {8: 4455768, 9: 4455768}` — is confirmed exactly where it holds (tracks 02–05 and 08 are the
disc from the first byte, and the extra 4,455,768 bytes is what makes the first 156 s of track 09 the disc
instead of nothing) and is confirmed to be *all* it can do: on six tracks the agreement ends inside the
track, on two of them after the first second.

The table is the *after* picture: it compares the cuts the ports now make, which bound a shifted track at
the next track's start (see below).  Track 09 is the only row that moved — 53.1 % of 294 s instead of 48.9 %
of 319 s — because the 25 s it used to finish with belonged to track 10.

## Where the disc's bytes actually sit, and why no shift can find all of them

A second probe took eight 512 KB windows spread across each drifting track, searched for each window in
`MUSIC.RES` with `mmap.find` inside ±32 MB of the naive position, and printed where it landed relative to
the shift the model predicts:

```
track       at    0s     at    1s     at    2s     at    3s     at    4s     at    5s     at    6s     at    7s
track06           +0           +0           +0           +0  -21,329,636           +0  -21,329,648    not found
track07           +0           +0  -12,540,748  not found    -25,081,508  -33,166,500  -33,166,508   +4,455,768
track09           +0           +0   -9,337,540 -28,012,692    not found  -23,130,888  -32,468,448  -32,468,432
track10           +0           +0           +0           +0  -32,392,748  -32,392,764    not found  -32,392,740
track11     +4,139,184           +0    not found    not found    not found           +0    not found           +0
track12     not found    not found    not found    not found    not found    not found    not found    not found
track13     not found    not found    not found -10,417,008  -20,834,016  -20,834,016  -31,251,024  -31,251,024
track14           +0           +0           +0           +0           +0    not found           +0           +0
```

Three things fall out of that table, and the third is the one that closes the enquiry:

1. The offset is not one number per track.  It is constant inside a stretch, then jumps — which is what
   material being present in one image and absent in the other looks like.
2. The jumps are large: 8 MB, 12 MB, 21 MB, 32 MB, and on track 13 they accumulate ~10 MB per eighth of the
   track.  That is not a seek error; something is in one stream and not the other.
3. The jumps are **not multiples of 2352**.  `-21,329,636 / 2352 = 9069.57`, `-12,540,748 / 2352 = 5332.8`.
   A CD rip can only lose or gain whole frames — a multiple of 2352 bytes — so "the rip dropped a frame"
   is ruled out by arithmetic.  Whatever was done to those segments was done at sample granularity, i.e.
   after the audio left the disc.

So there is no per-track shift, and no anchor table of the size anyone would want, that makes a fan
repack's `MUSIC.RES` equal the shipped disc end to end.  The honest options are "use the disc image" or
"use the repack and know that some tracks drift".  There is no third option that is true, and one of the
tasks set out to *find* a full-track alignment (a realignment scan over the whole image) was abandoned at
its timeout with this arithmetic in hand as the reason it was not worth continuing.

## The one thing a fan image forces: a track that has to stop early

`MUSIC_EXTRA` moves the starts of tracks 08 and 09 4,455,768 bytes (1,894 sectors) later, which is measured
and right.  Keeping each track's *disc* length after such a move is not right, because the fan image is not
denser than the disc everywhere: it puts 25 s the disc does not have before tracks 08 and 09, and drops the
same amount between 09 and 10.  So track 09's own length ends 1,895 sectors past the sector the image
itself calls track 10's first — and as served, track 09 finished by playing the first 25 s of track 10.
`byod-test.js` had been printing that as a fact of life (`OVERLAP track 9 and 10`), and the browser's
`stage()` had the complementary bug: a slice running past the end of the image was skipped, losing a track's
music entirely.

Both now bound a slice by its neighbour's start, and the last one by the image's end — `plan_tracks` and
`planTracks`, in one post-pass each, reporting it so nobody reads it as a bug in the cutter:

```
[audio] 1 track(s) cut short where the next track's audio begins in this image (track09 at 22078 sect)
        -- the layout of a fan rip's image, where the disc's own length would run into its neighbour
```

A truncated track is wrong.  A track that turns into another track at its loudest moment is wrong in a way
that gets blamed on the port.  `node wasm/scripts/byod-test.js /tmp/nkzip/dosbox.zip` now says
`no two tracks overlap`, and `check_slots.py --dir scratch/pocket_fan` passes on the clipped tree.

## What each route does with that, and the bug this measurement caught

**The browser** (`wasm/runtime/byod.js`, `shell.js`, service worker v24) now recognises the repack as a fan
image by what it holds, measures a picked image's CRC when nothing declares one (so a hand-unzipped
`MUSIC.RES` gets the corrections instead of being sliced 560 bytes late on every track), and says in the
log which of the two it is: `a fan rip's image: sliced at the offsets measured for it, which make it
byte-identical to the shipped disc on some tracks and drift inside the others`.

Measuring a CRC for a picked file exposed a second bug, which is worth keeping in the record because the
same code was the one that caught it: the first version measured for *any* image with no declared CRC,
including a **deflated zip member** — so it hashed the member's compressed bytes, got `a04cda77`, overwrote
the zip's own declared `36c386de`, and staging then refused the file it had just unpacked correctly
(`CRC-32 36c386de, the zip says a04cda77`).  A zip member's CRC is over its *uncompressed* bytes and is
already the declared one; only a picked file needs measuring.

**The extractor** prints, for a fan image, that its audio matches the shipped disc byte for byte on some
tracks and drifts inside others, and that `--wav` writes the cuts so the person can listen.

**The Pocket route** is where this mattered most, because that route has no cue sheet at run time to be
wrong in a self-describing way.  `scripts/pocket/repack.py` now asks the extractor's input layer
(`night_extractor.source`) instead of re-finding `TG_NIGHT.CUE` and `TG_NIGHT.IMG` itself, and it hands the
table generator a `disc.json` — the cue the card was cut with, and the geometry that goes with it.  It then runs `mk_dist.py` with the track numbers from that plan (`--tracks`), so a fan card
build leaves a core tree whose `data.json` and `slot_map.tsv` bind the same files — no cue sheet the core build
could read is needed anywhere in the run.  Making
that seam explicit caught a real mistake in the first draft: for a fan image `src.cue_text` is still the
**fan's** cue, whose track 02 sits at sector 174 of the rip, while the game asks for disc LBA 37262.
Handing that text to `gen_disc.py` baked the rip's coordinates into `disc_table.h` and would have played
every track from the wrong place.  The cue the compiled table is built from must be the same one
`extract.part_cue` writes — this port's own table of contents — and `DISC_IMG_SECTORS` must be the disc's
269,303 rather than the rip's 232,365, because `of_disc.c` answers the disc gate's 7-byte header and the
TOC records from those numbers.

```sh
python3 scripts/pocket/repack.py --data /tmp/nkzip/NIGHT.RES /tmp/nkzip/SOUND.RES /tmp/nkzip/MUSIC.RES \
       -o scratch/pocket_fan
python3 scripts/pocket/check_slots.py --dir scratch/pocket_fan
```

```
  [SKIP] byte-for-byte comparison against the image: the data is a fan rip's image (MUSIC.RES), whose audio drifts inside the disc's track boundaries
  [PASS] track3 - track2 = 3153 (0xc51), the value fn_010c3e compares
  [PASS] track02: 3153 sectors, LBA 37262..40414 at the disc's geometry at 4 sampled boundaries
  ...  [PASS] for all 14 tracks, plus the lead-in refusal and the slot bindings
```

Against the shipped disc the same commands keep the stronger claim: 15 files byte-identical to `extracted/`
(`NIGHT.RES` at 50,964,894 B and all 14 tracks), and `pocket/src/night/disc_table.h` regenerated
byte-identically to the committed one.

## So, the answer to the question asked

- **Geometry: always right.**  Track count, per-track sector counts, the disc LBAs, the lead-out and the
  slot bindings come out of the disc's own table of contents whichever input shape is used, so the game
  asks in the right places on a fan card and on a disc card alike (`check_slots.py` both ways).
- **Audio from a disc image: right byte for byte.**  Five shapes are recognised (`pair by name`, a rip
  directory, a stored zip, a renamed pair with its `FILE` line, a bare image, `NIGHT.RES` alone, a loose or
  zipped fan repack — `scripts/analysis/check_input_shapes.py`, `ALL SHAPES RECOGNISED`), and the disc
  shapes reproduce `extracted/` without a difference.
- **Audio from a fan repack: this disc's music, not this disc's bytes.**  02/03/04/05/08 are the disc;
  06/10/14 are the disc until 229 s / 20 s / 83 s; 07/09/11/12/13/15 stop matching earlier, two of them
  immediately.  Nothing in the port can fix that by cutting harder, and it now says so instead of
  pretending the slice table was a repair.

## Still open

- No attempt has been made to build a per-segment map for the drifting tracks.  The arithmetic in section
  three says a frame-aligned map cannot exist; anything that did would have to describe edits made after
  the audio left the disc, and would need a second copy of the audio to listen against.
- The repack's `SOUND.RES` cue is used for slicing and never given to the game; whether a future repack
  numbers its tracks differently is answered by the CRC guard, which refuses the corrections rather than
  applying them to an image it was not measured on.
