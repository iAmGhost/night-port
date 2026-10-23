# 61 — The level-up screen: reaching it, and what the run shows

## Where the game reaches it

`fn_038f3d` (orig 0x00038f3d) is the routine that loads `levelup.spr`, `number1.spr`, `number2.spr` and
`levelup.pcx` (variant 0) or `levelup2.pcx` (variant 1). Its call sites are all inside other screens' flow:

| site | context |
|---|---|
| `f.c:24998` (0x0003d2ac) | the setup/pause menu's entry 0 (`m == 0` -> status screen) |
| `f.c:25054` (0x0003d2b1) | the same menu's variant-1 path |
| `f.c:25921` (0x0003b54c) | **stage completion**: right after `g_dat_63370 += 1` in `fn_03af18`, guarded by `pl[0x30] != 1` |

so in normal play the level-up picture appears *between stages*. Scripted play cannot clear a stage inside a
run's lifetime, which is why the same host-warp convention as `--warp-stage` (docs/52) is used:
`NIGHT_SHOWSCREEN=levelup|status|help` with `NIGHT_SHOWSCREEN_AT=<frame>` calls the game's own routine from
the host frame pump. Only the *moment* of entry is host-chosen; the loads, compositing, palette work and key
handling under test are the shipped ones.

Measured: `showscreen warp: levelup at frame 2777`, with a stage live (3 entities). The warp must only fire
once a stage is loaded -- called with no stage live it faults in exactly the way the null player pointer of
docs/55 did (first attempt: warp at frame 2723 while still on the title card -> SIGSEGV).

## What the run shows

`scripts/playtest/route_levelup_warp.txt` = the anchored episode-1 route plus dense shots across the warp
window (the automatic per-lookup screenshots do not cover every loader, see below):

```
0048_lu_00  bright=124.04
0050_lu_01  bright=149.23  changed 48.38% rows 38..189 cols 0..319
0054_lu_03  bright=144.55  changed 43.22%
0055_lu_04  IDENTICAL to the previous shot
0061_lu_07  bright= 95.28  changed 89.95% rows 0..199 cols 0..319     (the whole screen is replaced)
0063_lu_08  bright= 96.73  changed  9.69% rows 5..192 cols 88..292
0064_lu_09  bright= 97.54  changed 11.91% rows 31..192 cols 86..287
0066_lu_10  bright= 97.67  changed 10.13% rows 31..192 cols 118..282  (settling, localized)
```

That matches the routine's own documented opening: "the first frame is a fade-in: when `DAT_000633a4 == 1`
the picture goes straight to the A0000 window and fades, otherwise `help1.pcx` is shown first, the flag is
latched, and only then the level-up picture is copied and faded in" -- a full-screen change (help card, then
the level-up picture) followed by progressively smaller localized changes as the fade settles. The resource
log confirms the help card: `shot 0110 help1.pcx frame=2778` (one frame after the warp) and
`help1.pcx_hold frame=2833`.

## Trace coverage, and the gap that remains

Two loaders were added to the resource-lookup announce path (`sys_res_notify`) so screens leave a trace and
automatic screenshots: `fn_036190` (a still image straight into a buffer) and `fn_03d8d4` (the `.spr`/`.act`/
font block loader). `help1.pcx` now appears ✓. **`levelup2.pcx`, `levelup.spr`, `number1.spr` and
`number2.spr` still do not**, so this screen loads its own art through some other path than those two
routines; identifying it is the next step for this item (it is a harness-visibility gap, not a claim that the
assets are not loaded -- the screen visibly changes over the frames above).

As for every other screen, the matching frame from the original is still blocked on the DOSBox capture path
(docs/46).
