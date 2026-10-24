# Feature sweep: which asset families the port actually touches

Companion to the feature-sweep task in the playtest goal. The question is not "does the sprite table look
right" but "for each shipped asset family, is there an observed draw somewhere in a headless run" - and where
there is none, why.

## Method

The host now prints every resource lookup in order: `NIGHT_RESLOG=1` makes `reslog_add()` (which already backs
the script's `wait res` anchor, docs/56) emit `resseq <name> n=<index>` lines. Nine-episode sweep:

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_BOARD=1 NIGHT_RESLOG=1 \
  ./test/night --script scripts/playtest/route_show_keyed.txt --warp-stage N --no-audio --frames 9000
```

Across the nine runs: **293 distinct resource lookups**, of which 122 are `.spr`/`.act` blocks.

## What each episode loads (measured)

| stage | .spr families observed |
|---|---|
| 1 | st1s, boss1, mboss1-1, ena-1/2, enb-1/2, enc-1/2, end-1 |
| 2 | st2s, boss2, mboss1-2, mboss2, ene-1/2, enf-1/2, eng-1/2/3 |
| 3 | st3s, boss3-1, enh-1/2, eni-1/2, enj-1/3 |
| 4 | st4s, boss3-2, ene-3/4, enf-3 |
| 5 | st5s, boss3-3, enk-1/2 |
| 6 | st6s, boss4, mboss1-1, ena-1/2/3, enb-1/2, enc-1/2/3, end-1/2/3 |
| 7 | st7s, boss5, mboss1-1, ena-1/2/3, enb-1/2/3/4, enc-1/2/3, end-1/2/3/4 |
| 8 | st8s, boss6 (no enemy families - the picture-show episode, docs/52) |
| 9 | st9s, boss7, enh-1/2/3, eni-1/2/3/4, enj-1/2/3/4 |

Every episode also loads the shared screen/HUD set: `dust.spr`, `mark.spr`, `exp.spr`, `face.spr`, `gage.spr`,
`gun.spr`, `over.spr`, `pause.spr`, `system.spr`, `system2.spr`.

Against the family list in the task contract: **43 of 53 families are observed loading in a headless run**,
including all of `ena..enk` (the sweep reaches `enk`, which the contract list stops at), `st1s..st9s`,
`mboss1-1/1-2/2`, and `boss1..boss7` (one per episode).

## Filed gaps, with root cause

1. **`mboss3..mboss6` never load in any episode.** They are entity **types 14, 15, 16, 17** in the image's
   per-type name arrays (read straight out of `NIGHT.EXE`: DS `0x51d88` for `.spr`, `0x52198` for `.act`;
   types 13..17 are `mboss2..mboss6`). `fn_037b03` loads a type's assets only when the stage's records mention
   that type, and across all nine shipped stage sets none mentions 14..17. So these four families are
   **unreferenced shipped content**, not a port-side omission. (The name-array transcription that once drew the
   wrong sprite is now pinned separately: a test hashes all 101 `.spr` and all 101 `.act` names against a copy
   read out of the EXE - `all 101 .spr type names match NIGHT.EXE DS 0x51d88`.)
2. **`levelup.spr` / `levelup2.pcx` / `number1.spr` / `number2.spr` / `help1.pcx`** do not appear in the
   nine-episode sweep because they belong to nested screens, and those screens only run when the game reaches
   them. They *are* observed (and screenshotted) on the warp routes: docs/61 shows `fn_038f3d` allocating the
   four blocks and `fn_036190: levelup2.pcx`, with the level-up picture rendered in the `warp_NN` frame series;
   the help screen appears one frame after the same warp family.
3. **A unit-driven draw fixture for the unreferenced families is not in the suite, on purpose.** Loading works
   (`mboss3.spr loads through the game's own block loader`, `has a drawable cell`), but drawing needs the
   per-cell argument convention, and with the pause/over convention `fn_03bb42` draws **nothing** for
   `mboss3.spr` cell 1 (measured, 0 pixels). Guessing an argument order is exactly the mistake that produced
   the game-over crash (docs/61), so it was withdrawn rather than committed. Two side findings from that
   attempt, worth their own investigation:
   * loading four `mboss*` blocks back to back in the 48 MB fixture arena ends in a **bus error inside
     `heap_free` (f.c:2962, via `fn_03dcbd` -> `heap_free_host`)** on the fourth family - ASan: `WRITE` at a
     high address. Either the fixture frees in a way the allocator does not support, or repeated
     `.spr` load/unload corrupts a region record. Not reproduced anywhere in the game's own paths yet.
   * the `.spr` sub-array layout used by `fn_03bb42` is *not* uniform across families: `pause.spr`,
     `over.spr` and the stage sprites all resolve their cells, `mboss3.spr` cell 1 does not draw with the same
     arguments.

## Screens (screenshots exist; comparison is the blocked half)

* pause menu - docs/60 (entry indices, MUSIC/EFFECTS toggles, `pause.spr` cell arithmetic, redraw rows/cols);
* level-up / upgrade screen - docs/61 (`warp_NN` series, the card read visually);
* game-over / CONTINUE screen - docs/61 (fixed this cycle, `0205_warp_25.bmp` shows GAME OVER / CONTINUE ? /
  YES / NO / CREDIT 3);
* help and status screens - reachable through the same `NIGHT_SHOWSCREEN` warp family.

Pixel comparison against the original for all of these is blocked on the capture routes measured in docs/46;
the substitute basis (independently decoded shipped assets) is docs/63.
