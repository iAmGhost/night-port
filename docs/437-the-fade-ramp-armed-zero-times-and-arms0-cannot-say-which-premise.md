# 437 — `fade arms=0`: the ramp never ran, and "0" cannot say which premise failed

`0.2.11` shipped the panel-side palette ramp (docs/436) with `FADE_RAMP=1`. The boot says it did nothing:

```
| fade arms=0 steps=0 n=0
```

No `fade ramp: n=...` lines either. So one of my three premises is false, and the number does not tell me
which:

1. `of_setting_fade_ramp()` returned 0 — the ini value never reached the app;
2. `sys_vga_palette_writes()` does not move on the device — the DAC stream does not go through the counter I
   trusted;
3. the check itself never runs — the code path I inserted is not the path the device takes.

A counter reading zero is ambiguous across all three. That is the whole lesson, so `0.2.12` replaces it with
one line that cannot be:

```
| fade runs=%u arms=%u steps=%u n=%u dwmax=%u dw=%u w=%u fr=%d
```

`runs` counts every time the fade check executed (0 = the branch is dead), `w` is the raw DAC write counter
(0 = the counter is not moving), `dwmax` is the largest interval seen (small-but-nonzero = the fades are
spread across presents, not between two), `fr` is the setting as the app read it. Plus the boot marker now
echoes `fade ramp: setting=..`, so hypothesis (1) is answered before the first second of the run.

## What was ruled out before adding anything

* **The card is not stale.** `md5` of the card's `night.elf` equals the build tree's, and `verify_card.py`
  byte-checks that pair (docs/428) — so the ramp code was on the panel's core, and the log itself stamps
  `core 0.2.11`.
* **The game really does write whole DAC tables.** `NIGHT_DACLOG` on a desktop stage-1 warp gives
  **321 pushes of `n=256`** (a full 768-byte table each) in 400 frames. So the premise that there are ~50-step
  full-table fades to see is true on the shared code path; this is not a "the game never fades" discovery.
* **The rest of the run is unchanged and healthy** (docs/436 baseline): 640x480 16-33 fps at 30.0 ms copy,
  gameplay 42 fps at 6.6 ms, `rows 0/0` with `ROW_SKIP=0`, stall witnesses identical to `0.2.10` — the only
  new thing this build did was add code that then did nothing.

## What this shape is worth

The alternative was to ship the ramp, see no fade, and start guessing among three causes with a 25-minute
feedback loop each guess. Instrumenting the premises instead makes the next boot a decision rather than a
coin flip, and it cost one build. It is the same move as `gap worst=` (docs/431) and the `split` line
(docs/425): when a measurement can be read two ways, the cheap fix is a second measurement that can't.
