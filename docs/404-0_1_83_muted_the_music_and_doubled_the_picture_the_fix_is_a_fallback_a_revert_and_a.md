# 404 — 0.1.83 muted the music and doubled the picture; the fix is a fallback, a revert, and a printf that had been lying since 0.1.79

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* the fallback and revert it records are the shipping shape.


Core **0.1.84**, card ELF md5 `867489942ae47ef5b5d1c5ab5eb40757` == `core.json` 0.1.84, 29 ini keys,
`pocket/src/night make test` green on both halves, repo `make test` `== ALL PASS ==` (see below), `src/f` untouched.
Evidence kept: `analysis/legacy/cardlog_0_1_83_async_failure.sav`, `analysis/refs/pocket_0_1_83_title_corrupted.png`.

## 1. The user's three symptoms, in the order the log explains them

### "Music doesn't play at all" — my fallback was missing, and the fuses were wrong
The 0.1.83 log (34 KB, fresh boot, 1 160 frames ≈ 21 s):

```
cdda async issues=1 waits=1 falls=1 staged_sectors=27 ...
cdda async issues=2 waits=2 falls=2 staged_sectors=27
budget ... disc ops=2 sectors=54 mean_op=0us max_op=0us ... cdda fills=2 mean=127us max=133us frames=0
audio cdda=0/0/2 ... ring=0 head=-1
```

Three things follow, and the third is the bug:

- `mean_op=0us max_op=0us` over two 27-sector reads. I bill a read at the falling edge of
  `of_file_async_busy()`; a zero mean means that edge **never happened** — the completion never arrived, so neither
  stage ever held landed bytes (`staged_sectors=27` pinned forever: 27 sectors promised, none consumable).
- `cdda fills=2 ... frames=0`. Two fill calls, zero audio frames written to the ring, for 21 seconds.
  `ring=0 head=-1`: no voice started. That is why there was no music.
- **`fill_sectors_raw` returned without reading the disc.** When the async path had nothing landed I wrote
  `return frames` — a dead end instead of the read the pre-async code had always done. So a device where the
  async API does not behave became a device that cannot play music at all. That is strictly worse than the stall
  docs/403 set out to fix, and it was my design, not the OS's.

Both fuses I put in for exactly this failure also failed, for a reason worth keeping: fuse 1 required
`falls >= 8 && !issues`, and `issues` was 2, so it could never fire; fuse 2 required 32 consecutive
waits, and `waits` only reached 2 because the track never got as far as pumping. **A guard written against the
failure I imagined does not cover the failure that occurs.**

Fix, in order of importance:
1. `fill_sectors_raw` now **always falls through to the synchronous read** when the stage has nothing. The async
   path became an attempt with a floor, never a route.
2. The wait-fuse dropped to 8 consecutive fills, and it disables prefetch rather than returning.
3. `DISC_ASYNC=0` ships. `ab.sh disc-async` puts it back for a re-try, with the honest instruction to judge it on
   `cdda async waits` climbing while `dry` stays 0 — not on music playing, because music now plays either way.

Desktop proof that the audio path is the known-good shape again: `--warp-stage 1 --frames 1200` →
`cdda.raw` md5 `ad81a3e0f5567c210c358123accfe2e9`, byte-identical to the pre-async baseline, with
`audio_low=0 dry=0 room_hold=0`, `disc ops=163 sectors=1932`.

### "Title screen corrupted" — the native 640×480 surface path is not trustworthy
The screenshot (`analysis/refs/pocket_0_1_83_title_corrupted.png`, 640×480 = a capture of a 640×480 video mode)
shows the title artwork twice, side by side, each copy clipped, with black bars top and bottom.

I tried to name the transform instead of guessing. Decoding the shipped `scratch/res/title1.pcx` (480×640) and
searching crops and magnifications against the panel, score = zero-mean/unit-variance luminance correlation
(so the mid-fade palette ramp and the panel's colour processing cannot flatter a match), and separately a fixed
hypothesis set (left/right half only, half tiled twice, 2× magnify with column wrap, stride-read-as-double):

```
best crop+magnify NCC      0.562      (a true picture match on this rig scores > 0.9)
best fixed hypothesis      0.460      (identity 0.553 -- i.e. the card does not explain the panel at all)
horizontal self-correlation: no periodicity (lag 64 = 0.407, decaying monotonically -- smooth artwork, not a repeat)
```

So the doubled picture is **not** a crop, a magnify, or a wrap of the card, and it is not a periodic stride
repeat. Whatever it is, it is not a transform I can invert from a colour screenshot, and the path that introduced
it — `FB_MODE=1`, a native 640×480 surface plus a row memcpy — shipped 0.1.81 as a **speed** decision
(19.5 MB/s against 0.93 MB/s, docs/401) and was never checked against a screenshot. The older captures on the
card (`20261105_1607xx`, 320×240) are the other shape, and the user never reported doubled art on it.

Decision: **correctness outranks the copy cost.** 0.1.84 ships `FB_MODE=0` (OS 320×240 surface, software
downscale, every row written) — the geometry every build 0.1.31–0.1.80 used. That re-admits the ~83 ms present
docs/401 measured, so the intro will be slower until the native path is understood. Three `ab.sh` levers keep
the question open without another rebuild: `picture-hardware` (the shipping shape), `picture-native` (reproduce
the doubling), `picture-scaler` (`FIT=2`: let the **core** scaler stretch, which is the untried answer to the
copy cost — the aperture sustains 19.5 MB/s, so the expense is my downscale loop, not the write).

## 2. The `blit dirty` line has been printing shifted numbers since 0.1.79 — including the ones I quoted
```c
NLOG("blit dirty rows_written=%u ... too_big=%u too_small=%u surface=%ux%u stride=%u stage=%dx%d | palette pushes=%u skips=%u\n",
     /* 12 specifiers */ g_rows_written, g_rows_same, g_stage_frames, g_stage_fallbacks, g_stage_small,
     g_applied_w, g_applied_h, g_applied_stride, g_pal_pushes, g_pal_push_skips);   /* 10 arguments */
```
Two phantom fields (`stage=%dx%d`) with no arguments. Everything after `too_small` shifted by two: the geometry I
read as `surface=640x480 stride=640` was correct, but `stage=103x739` was **the palette counters**, and the
`palette pushes=124 skips=2` printed after it was varargs garbage. The true palette numbers for that boot are
pushes=103 skips=739.

Consequences I want on the record rather than quietly amended:
- The palette-gate verdict in docs/399 §5 read `pushes=124 skips=2` from a field that held nothing. The *mechanism*
  still holds (an FNV hash over the 768-byte palette gating the push), but that count was noise.
- docs/400's retirement of partial commit came from `of_video.h:10` (the display is triple-buffered and
  `of_video_flip()` does not wait), not from this line — so that conclusion stands on its own.
- `staged_frames=1 rows_written=0 rows_same=0` is one of the three correctly-mapped fields, and it means staging
  was reached once even with `BLIT_DIRTY=0`. That path writes only rows it believes changed, which docs/400 shows
  is wrong on a triple-buffered display, so the fact that it was reachable at all is a defect. `stage_begin()` is
  now the only entry and the gate around it is commented with the reason; the boot line states the picture shape
  — verified in the ELF the card runs, not just in the source:
  `blit shape: staging=%d (blit_dirty) fb_mode=%d present_sync=%d | …` followed by `fit=%d` — so no future log
  leaves it ambiguous. The corrected `blit dirty` line carries 10 specifiers and 10 arguments, counted from the
  string in the ELF rather than from the source.

I also invented a `BLIT_STAGE` ini key while reasoning about this and put it in `mk_dist`; it exists nowhere in
the C. Removed — `BLIT_DIRTY` is the real gate. Writing a key that no parser reads is the same class of error as
the phantom format fields: the log looks informative and says nothing.

## 3. Shipping shape (0.1.84)
`DISC_ASYNC=0` `DISC_RAMP=0` `DISC_PREFILL_FIRST=150` `DISC_MIN_WANT=12` `DISC_BATCH=12` `BLIT_DIRTY=0`
`BLIT_SKIP=0` `FB_MODE=0` `PRESENT_SYNC=0` `SCREEN=0` `FIT=1` `RESTORE=1` `MASTER_VOL=255` `LOG=1` (for the next
card log) `DIAG=0`. `ab.sh player` puts `LOG=0` back for a player's card.

## 4. What the next boot decides, and only these measures count
1. **Music**: `cdda fills … frames=` must be non-zero and growing, `dry=0`, `audio_low=0`, and
   `audio cdda=…` non-zero with `ring` not pinned at 0. That is the symptom the user reported, and it is now a
   code path that cannot starve itself.
2. **Picture**: a screenshot of the **title** screen. If it is single and correct under `FB_MODE=0`, the native
   640×480 surface path is the defect and it stays off until I can reproduce the doubling on a bench I can
   actually read — which means the arena witness and the surface bytes, not a colour photograph.
3. `blit shape:` and the corrected `blit dirty …` line at boot: the first tells me the shape the device is using,
   the second's palette counters are finally the counters.

## 5. Asked the right question after the fact: "we were fixing audio, so why is the title broken?"
That question was correct and it broke my assumption. `git diff 0.1.82..0.1.83 -- pocket/src/night/main_of.c` is
21 added / 15 removed lines, and the only picture-code change is `copy_row` **moving** out of a `#ifndef OF_PC`
guard — the same function, compiled for the device before and after, byte-identical codegen. The audio build
touched no picture code. So "the native 640x480 geometry is wrong" was a guess I dressed up as a measurement,
and bundling the audio fix with a picture revert in one build was a method error: it made the next boot unable to
say which change mattered.

Asking the question properly — *what in this file can draw pixels that are no transform of the card?* — found a
real defect that had nothing to do with either change, in `present()`:

```c
uint8_t *surface = of_video_surface();          /* the 320x240 buffer */
uint8_t *fb = surface;
...
if (native) fb = of_video_surface();            /* re-pointed at the new 640x480 buffer; `surface` still names the OLD one */
...
if (fb != surface) stage_commit(surface);       /* fires on its own, and commits with the PREVIOUS mode's stride/rows */
```

Three things compose into the corruption:
1. `fb` is re-pointed on the boot-time 320x240 -> 640x480 mode switch, but `surface` is not;
2. the display is **triple-buffered** (`of_video.h:10`), so the "old" buffer is still one of three the OS rotates
   through and still being presented;
3. `stage_commit` walks `g_stage_stride`/`g_stage_rows`, which still describe the *previous* mode.

So on that one frame the core writes a stale picture, at the wrong stride, into a live display buffer. That
produces exactly what the screenshot shows — artwork that is not a crop, a magnify, or a periodic repeat of the
card — and it fires on the boot-time mode switch, which is **when the title card is drawn**. `staged_frames=1`
with `rows_written=0` in the 0.1.83 log is the fingerprint of exactly one such spurious commit, and it was the
one correctly-mapped field on the broken printf line that pointed at it.

Fix (0.1.85): the commit asks an engagement flag the code owns (`g_stage_engaged`, set only when `stage_begin()`
actually redirected the blit into `g_stage`), and the mode-switch block moves **both** pointers together and drops
staging for that frame rather than committing it with the wrong geometry.

This also re-opens the speed question. The bug lived only in the `native` branch, i.e. under `FB_MODE=1`, which
is the 19.5 MB/s path (against 0.93 MB/s, docs/401) — so the fast picture may be usable after all. I am not
flipping it in the same build as the fix, for the reason the user's question just taught me: **one variable per
boot.** `ab.sh picture-native` flips it in one ini line on this same ELF, no rebuild.

Card is now 0.1.85 (md5 `b8270e61b31087f0c4c80db665564a03` == build == `core.json`), shipping
`FB_MODE=0` + `DISC_ASYNC=0`, so the next boot tests: does audio come back (fallback fix), and is the title
single and correct (stale-buffer fix, on the shape that was never in question).
