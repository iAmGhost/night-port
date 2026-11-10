# 436 — the original *does* fade into stage 1; the port cut straight to full, so the panel now replays the DAC steps

docs/435 closed the stage-entry *stall* as not-a-gap. The same container captures answered the other half of
that moment, and this one is a real gap.

## 1. The premise, measured rather than remembered

Mean luminance of the original's captured frames across stage entry (rig's anchored episode-1 drive, 4 Hz):

```
   89.2   26.5   stage_play      320x200     <- first gameplay capture: dark
   89.3   43.8   gameplay_01
   89.6   78.2   gameplay_02
   89.8   95.4   gameplay_03                 <- full brightness
   90.1   95.3   gameplay_04                 (steady from here)
```

A ramp, ~0.6 s of it, caught mid-flight by a 4 Hz sampler — so the true step count is larger than the four
values show. docs/417 measured the same shape on the logo card (0.83 s in, ~1.0 s at full, 0.83 s out).
The port shows `black -> FULL` in one frame (docs/426 §1: "at present granularity, stage 1 has no fade").

## 2. Why the port cannot see it

`fn_040660` / `fn_0406f5` (f.c:348, 376) program the VGA DAC 50 times in a tight loop — scale the table by
`i/frames`, `board_out` it — with **no wait and no yield** between steps (only `fn_00035ec2`, modelled as a
no-op sequencer ready-wait). So on the Pocket all 50 tables land *between two presents* and `palette_push()`,
which samples the palette once per present, uploads only the last one. The fade is complete in the game's
model and invisible on the panel — exactly what docs/426 found, and the reason `DELAY_US` was not the answer
(a nonzero value makes fades visible by inserting host yields, but it pushes the card holds PAST the original,
so `DELAY_US=0` ships, docs/416/417).

Yielding *inside* the fade loop is the obvious fix and is the wrong one: the fade runs inside a single pump
visit, and presenting from there re-enters the display path — the hazard docs/426 established.

## 3. What shipped instead: the ramp lives on the panel side

The game declared how many steps it wants; the port was just dropping them. `sys_vga_palette_writes()` counts
DAC writes (3 per entry set → **768 per full-table step**), so the number of steps that landed between two
presents is measurable without touching shared code:

```c
uint32_t steps = (writes_now - writes_at_last_present) / 768u;
if (fade_ramp && steps >= 4) { from = what the panel shows NOW; to = the game's table;
                               n = min(steps, 60); k = 0; }         /* arm */
```

and while a ramp runs, each present uploads `pal_ramp_step(from, to, k++, n)` instead of the target. `>= 4`
full-table steps (3 072 DAC writes) is the fade/gameplay-tweak boundary: gameplay palette changes are a
handful of entries. Mode changes (`applied_refresh`) drop the ramp, because after re-programming the display
path `from` no longer describes the panel. `k >= n` must land on the target **exactly** — that is the contract,
because a ramp that stops short is just wrong colours, which is the shape of both docs/422 (palette hash
covering 37 of 256 entries) and docs/373 (index 255).

Device-only (`#ifndef OF_PC`) on purpose: docs/407 — a knob that exists to fix a device number must be inert
on the host that is the measurement basis. Which means the ramp path, like row-skip before it, **has no
desktop validation**, so the math was pulled into `of_pal_ramp.h` and tested on its own
(`make ramp-test`, `scripts/pocket/test_pal_ramp.c`).

## 4. The test paid for itself in the first run

The first cut of `pal_ramp_step` subtracted two `uint32_t` channel values and then multiplied:

```c
(int32_t)((((b >> 16) & 0xffu) - ((a >> 16) & 0xffu)) * (int32_t)k / (int32_t)n)
```

The subtraction is **unsigned**, and `unsigned * (int32_t)` converts the multiplier to unsigned: a fade toward
black multiplied `0xFFFFFFEE` by 25 and divided that. `test_pal_ramp` printed

```
FAIL: fade-out half-way should be half the colour (got bffefb7c)
```

where `0x00001a` was wanted. Fixed by converting to `int32_t` before the arithmetic (`pal_ramp_ch`). Had this
gone to hardware it would not have failed to compile, would not have failed `make test`, and would have
produced 50 frames of scrambled colour at every stage entry — i.e. the flicker report I have been chasing
since 0.2.3 would have been *replaced by a worse one*, with a diff that could not be reproduced to find it in.

## 5. What the boot has to tell us

`FADE_RAMP=1`, `0.2.11` on the card. Look for:

- `fade ramp: n=.. (dac writes=.. since last present) from black=yes/no` — the arms. `from black=yes` is the
  stage-entry case docs/436 is about.
- `| fade arms=.. steps=.. n=..` on the perf line — did the ramp run and for how long.
- The player's judgement: does stage 1 come up like the original (0.6–0.8 s in) rather than a hard cut.

Two honest uncertainties. The ramp spans `n` *presents*, so at gameplay's 41 fps a 50-step ramp lasts ~1.2 s —
longer than the original's ~0.6–0.83 s; if that reads as sluggish, the fix is a step cap, not a rethink. And
the discriminator arms on any large multi-step palette move, which includes hard cuts between slides — the
original's own slide transition measured `6.7 -> 63.4 -> 193.5` in half a second, which may be a ramp or may be
a cut the port will now slow down. Judge from the boot, and `FADE_RAMP=0` is one line away.
