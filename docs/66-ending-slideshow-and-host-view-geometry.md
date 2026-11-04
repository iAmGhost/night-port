# The ending slide show, reached and verified -- and two host presentation defects it exposed

Date: 2026-10-24. Code: `src/f/f.c` (`fn_03af18_ending_slideshow`, `fn_03628e`), `src/app/main.c`
(`NIGHT_SHOWSCREEN=ending`, pump presentation). Evidence: `NIGHT_RESLOG` + `NIGHT_VIDRAW` capture, compared with
`scripts/playtest_compare_slides.py::pcx_indices` (docs/63 basis).

## 1. Reaching the ending

`fn_03af18`'s `ending:` block (LAB_0003b2b1, reached only from `if (g_dat_63370 == 9)` at 0x03b1d4, i.e. after
clearing episode 9) was extracted verbatim into `fn_03af18_ending_slideshow()`. The game's control flow is
unchanged -- `fn_03af18` calls it at the same point and then runs the same tail (fn_000405a7, `g_dat_63370 = 1`,
entity remove + heap free, fn_037347, menu loop). The host warp `NIGHT_SHOWSCREEN=ending` calls it while stage 9
is live, exactly like the level-up/game-over warps (docs/61).

Result: all ten cards load and present -- `resseq end0001.pcx` ... `resseq end0010.pcx` -- with CD track 10
requested through `fn_10b1d` and the palette staged through `fn_041e20` + `board_out`, as in the original.

## 2. Byte-for-byte against the shipped cards

For each card, the arena bytes behind the presented frames vs an independent decode of the shipped PCX:

| card | frames sampled | frames byte-identical (all 307,200 bytes) |
|---|---|---|
| end0001.pcx | 19 | 10 |
| end0002.pcx | 11 | 11 |
| end0003.pcx | 11 | 11 |
| end0004.pcx | 11 | 11 |
| end0005.pcx | 13 | 13 |
| end0006.pcx | 12 | 12 |
| end0007.pcx | 12 | 12 |
| end0008.pcx | 12 | 12 |
| end0009.pcx | 13 | 13 |
| end0010.pcx | 11 | 11 |

116 frames are byte-identical over the whole 640x480 index field; zero frames differ. Screenshots:
`docs/manual-playtest/05-shots/ending_02.bmp`, `ending_06.bmp`.

## 3. Defect A: 640x480 screens after the first room were presented at 320x200

`fn_03628e` loads a card by writing the whole 0x4b000 bytes (640x480x8) through the slid VESA window. The port
tracks framebuffer geometry with `g_view_320x200`, but that flag was cleared **only** in `fn_03f462` (the slide
path) and set by the room flip (`fn_038098` path, f.c ~14233). So after the first room set it to 1, every
`fn_03628e` screen stayed a 320x200 view: the host read the first 64,000 arena bytes as a 320x200 grid, i.e. the
left quarter of a 640x480 picture stretched across the window. Affected: the ending slide show and
`cdinsert.pcx` -- both only reachable after a room.

Fix: clear the flag at the end of `fn_03628e` (it is host instrumentation: the original always displays VRAM).
Measured before/after on the same run: ending-window shots report `320x200` before, `640x480` after
(`raw` dumps grow from 64,000 to 307,200 bytes, which is what made the full-frame comparison above possible).
A stage-1 run still transitions correctly: logo 640x480 -> intro .fli 320x200 -> title/slides 640x480 ->
gameplay 320x200.

## 4. Defect B (found while fixing A, and my own regression while attempting it)

Screens whose game loop blocks inside a single host step (slide shows, the CG picture show, the ending) never
reached the frame-boundary `present()`: the window sat on the previous scene for the whole screen. Presenting
from the cooperative pump fixes that, but the first attempt reused `g_present_next`, starving the main loop's
present branch -- which is where `g_frames` advances. The frame counter froze at 1 while wall-clock timing stayed
identical (title1.pcx at 42,009 ms vs 42,014 ms), so every frame-gated script step, warp and `--frames` budget
stopped firing. The pump now uses its own 60 Hz throttle. Lesson kept: `g_frames` is a *presentation* counter,
so anything that touches presentation throttling is a timing change.

## 5. Verification

`make test` -- 2873 checks, ALL PASS. ASan build of the suite: ALL PASS, no sanitizer reports. Nine
episode sweeps and the level-up/pause/game-over warps unchanged.
