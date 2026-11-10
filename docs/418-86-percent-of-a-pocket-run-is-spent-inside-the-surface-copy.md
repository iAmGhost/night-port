# 418 — 86% of a 66.9 s Pocket run is spent inside the surface copy, and almost all of it copies unchanged bytes

docs/416 measured the mechanism and docs/417 closed its arithmetic on a second phase. This is the census: what
the present path costs across a whole boot-to-gameplay run, phase by phase, in one reusable tool —
`scripts/analysis/present_census.py` (output kept at `scratch/pocket/census_0193.txt`).

## The run (0.1.93 card log, 1088 lines, newest boot only)

```
t0..t1       view       hold_s sig_chg  pres/s  pres_us  copy_us  frames  n= shape
 0.0-  38.1  0 640x480   38.08       1   56.4    17296    17932    2149  n=38 MOSTLY STATIC 100% skippable
                                        surface: native 640x480   bytes/frame=307200  17.1 MB/s (pres 17296..65306 us)
38.1-  41.0  1 320x200    2.88       3   43.4     2894    16438     125  n= 3 MOSTLY STATIC  95% skippable
                                        surface: game 320x240     bytes/frame=76800   <-- too few samples (3) for a rate
41.0-  57.7  0 640x480   16.70       6   45.4    17301    19314     758  n=19 MOSTLY STATIC  98% skippable
                                        surface: native 640x480   bytes/frame=307200  15.9 MB/s (pres 17301..61814 us)
57.7-  66.9  1 320x200    9.25       9   45.3     2867     6211     419  n=10 MOSTLY STATIC  94% skippable
                                        surface: game 320x240     bytes/frame=76800   12.4 MB/s (pres 2867..43475 us)

wall 66.9 s; time inside the surface copy ~57.8 s (86% of it)
```

Three things the aggregate numbers hide, and the census exists to force out:

**1. The aperture is fine; the *number of bytes* is the problem.** A write pass over the display surface runs
15.9-17.1 MB/s at 640x480 (307 200 B, 17-19 ms) and 12.4 MB/s at 320x240 (76 800 B, 6.2 ms). Per byte, the small
surface is not slower — it just moves four times fewer bytes. That is the same fact docs/401 measured from the
other side (native row-memcpy 19.5 MB/s vs the software-downscale path at 0.93 MB/s): the downscale was never a
smaller picture, it was a per-pixel gather, and the real lever is writing fewer bytes or not writing at all.

**2. "Choppy intro" has a home, and it is the 640x480 phases.** Those frames run ~45-56/s, i.e. one present
every ~18-22 ms of which the copy is 17-19 ms — the frame budget is the copy, so any hiccup in the copy is
visible as stutter rather than absorbed. The 320x200 gameplay phases pay 6.2 ms and are not where the complaint
lives. This matches what was reported (the intro/slides stutter, gameplay does not).

**3. 86% of the wall inside one function, on a run that is mostly still.** Every phase here is "mostly static"
by signature. The logo card is the extreme case (docs/417: `sig` one value, one change, 38 s), but even the
title/attract phases change signature 3 and 6 times in 16.7 s — a change every couple of seconds, paid for by
~750 copies.

## What the skippable column does NOT mean

`% skippable` is computed from the readout's `sig`, and `fb_signature` strides by 37 (main_of.c:431). A small
move is invisible to it ~36 times in 37, so the column is an **upper bound**, and the honest reading is by phase:

- the two 640x480 phases (the 100% and 98%) are corroborated independently: they are card/hold phases whose
  fades are palette work, the signature never moves, and `arena` is flat;
- the 320x200 gameplay rows (95%, 94%) are **not** to be believed — gameplay moves sprites, and the full-byte
  hash `PRESENT_SKIP` actually uses will see changes this sampler cannot. Expect a much lower skip rate there,
  and that is fine: the point of skipping is the phases where nothing moves.

## How skippable is the animated stuff, measured (the caveat above, retired)

The `% skippable` column above came from the card readout's stride-37 sampler and was explicitly an upper bound
with nothing under it. Measuring the lower bound needs per-frame content on the desktop, and the rig already
writes it: every frame in `frames.bin` carries a 40x30 luminance thumbnail of the *presented* picture
(`rig_screen_frame`, RIG_GW/GH=40x30), so consecutive frames can be compared for real.

```
warp to stage 1, 2000 frames:                     identical-to-previous
  640x480   553 frames  10.14 s                         42.9%
  320x200  1448 frames  26.55 s                         24.0%
from boot, 1800 frames:
  640x480   137 frames   2.50 s                         41.9%
  320x200  1664 frames  30.51 s                         69.0%
```

Read the two measures against each other, because they differ for a specific and useful reason:

- the thumbnail is computed **after the palette**, so a card's fade-in -- the phase docs/417 proved has a
  `sig` that never changes once -- reads as "changed" on every fade step. That is why the boot's 640x480 phase
  measures 41.9% here while the card log says its surface changed once in 38 s. **At surface level that phase
  is ~100% skippable; the thumbnail says 41.9%, and the gap is entirely fades, which the skip handles for free**
  (the palette still uploads on a skipped frame).
- gameplay is the honest row: **24.0% of 320x200 gameplay frames are identical to their predecessor** even at
  this coarse sampling. That is the real ceiling for `PRESENT_SKIP` in motion, and it is nowhere near the 94-95%
  the stride-37 sampler implied -- exactly the direction the caveat predicted.

So the expected shape after the skip: card/hold/slide phases collapse (docs/417 predicts 38.08 s -> ~2.0 s on
the logo), and gameplay gets a modest ~1 in 4 presents removed. That is also why the choppiness fix cannot stop
at the skip: an animated 640x480 phase still pays the copy on 57% of its frames.

## What a skipped frame still costs (this is what sets the floor)

The log's `stages` line splits `present()` into its stages, and the copy is not just most of it -- it is all of it:

```
mean over 70 samples:  mode=15 us  pal=170 us  copy=16569 us  flush=290 us  flip=209 us
maxes:                 mode=16      pal=576      copy=64253      flush=693     flip=255
```

So `PRESENT_SKIP` takes 16.6 ms out of ~17.1 ms per present, and a skipped frame still costs ~0.7 ms. That
residual is *why* the prediction lands on the shim's 2.00 s rather than on f.c's bare 1.05 s pacing floor: with
the copy gone each spin pass costs about 0.7-0.9 ms, the loop's own deadline is 0.5 ms per clock read, so
~2048 reads x ~0.8 ms ~ 1.6 s, plus the fades and the mode set -- i.e. the 2.00 s the shim measures.

Two follow-ups, both measured, neither shipped with the skip (one variable per boot):

- `of_video_flush()` on a frame that wrote nothing is dead work (214-290 us). It is almost certainly safe to
  skip, and worth ~0.2 ms/present.
- `of_video_flip()` on an unchanged frame is also dead work *if* the palette bulk write takes effect without a
  flip. That "if" is the whole reason this is not folded into the skip: docs/399 established that re-programming
  the display path can drop the palette, so palette latching here is subtle, and a fade that only advances on
  flip would look like a frozen card -- the exact bug class docs/400 is about. Measure it separately.

## For the next boot

The same tool on a `PRESENT_SKIP=1` log should show the skip target phases' `copy_us` collapse and the
`skips=` counter climb (it prints per phase when non-zero). The phases' `hold_s` is the number that matters:
docs/417 predicts 38.08 s -> ~2.0 s on the logo card. If the copy stays at 17 ms while `skips` climbs, the skip
is not reaching the phase that costs, and the phase table is where that becomes obvious.
