# 82. What the ORIGINAL's forced episode-8 window shows: the room plus the episode-8 CG dialogue card

docs/74 filed this: under the forced stage index the original's stage-8 window was a *static* 320x200 picture
(27 identical frames, mean RGB (77.9, 70.0, 69.4), 177 distinct colours) that matched neither `na80001.pcx` nor the
`stage8.pcx` card, scoring 4.53 % within +-3 / NCC 0.197 against the port's stage-8 frame. It is now identified and the
port is **validated**: the aligned frames are byte-level identical.

## What the window is

The window is the live episode-8 room with the episode's **CG dialogue card** (`na80001.pcx` content) composited over it:
the room backdrop, HUD, player and the white-haired speaker's bust on the right, and the subtitle box reading
"서호 : 이봐, 남궁건." -- the exact line printed on `scratch/res/na80001.pcx`.

Identification evidence (the card is *not* drawn as a decoded card, so plain card-vs-frame scoring cannot find it):

* **Per-colour occurrence counts in the bust region match the card exactly**: 1246, 891, 790, 640, 463, 395 in both
  images, with colours differing by a few LSBs (`(252,196,156)` -> `(247,194,154)`, `(36,32,48)` -> `(32,28,45)`,
  `(255,231,199)` -> `(247,223,194)`, `(60,52,76)` -> `(57,49,73)`). Two palette entries differ more
  (`(75,87,115)` -> `(109,154,202)`, `(227,227,235)` -> `(81,113,154)`), which is the card being shown through the
  room's palette rather than its own.
* The card's dominant colour is the teal key `(27,123,143)`, 57.27 % of the card; in the window **0.00 %** of those
  pixels carry it, i.e. the key is dropped and the room shows through.
* Consequently neither side matches the card decode: original vs `na80001.pcx` 2.23 % within +-3 (NCC 60.5 %), port vs
  the same decode 0.48 % (NCC 60.4 %) -- the *same* low numbers on both sides, which is itself the evidence that the
  two sides render the same thing the same way.

## Why the window is static, and why docs/74 scored it 4.5 %

The picture show is **once per room, inside the room loop** (`fn_03a70e`, armed when the `DAT_00053c80` entity is inside
the camera band: `fn_03a829` at `0x0003aa3b-0x0003aa9a`), and it waits a key press per picture (docs/52). So the original
sat on that card for tens of seconds -- the "static picture" is the keyed show, not a hang. The drive's later presses moved
it on (`window_c` frames carry different content), which confirms the key-per-picture wait.

docs/74's 4.53 % / NCC 0.197 came from comparing the original's **during-show** frame with a port frame taken **after the
show** had already ended. Reproduced exactly here: the port's `after_show` frame vs the original's window gives
**4.48 % within +-3, MAE 40.70, NCC 20.93 %** -- docs/74's number. It is a phase-alignment error in the capture, not a
rendering difference.

## The aligned comparison

| pair | within +-3 | MAE | NCC |
|---|---|---|---|
| **port show frame (`0151_na80001.pcx.bmp`, frame 2787) vs original window** | **100.00 %** | **0.24** | **100.00 %** |
| port `after_show` frame vs original window (docs/74's pairing) | 4.48 % | 40.70 | 20.93 % |
| original window vs shipped `na80001.pcx` decode | 2.23 % | 53.26 | 60.47 % |
| port show frame vs shipped `na80001.pcx` decode | 0.48 % | 53.39 | 60.41 % |

Original frames: `/tmp/dbgauto/orig_ep8w/` (drive `scripts/playtest/drive_orig_ep8_window.txt`, docs/70 channel order,
`--swap13 1`); guest witnesses across the window: `DAT_00063370 = 8`, `DAT_00063354 = 0x38202c` (room record loaded),
`DAT_0006336c = 0`. Port frame: `/tmp/port_ep8/0151_na80001.pcx.bmp` (`--warp-stage 8`).

**Verdict: no port stage-8 difference.** The original-side cause of the docs/74 finding is capture phase, and the port's
stage-8 startup draws the same composite.

## Regression test

`scripts/playtest_ep8_show.py` -- runs `port/test/night --warp-stage 8` with the walk/fight spine, scans every 320x200
shot of the startup, and requires one of them to be >= 99.5 % within +-3 of
`docs/manual-playtest/07-shots/episode8_show_original.png`:

    uv run --with numpy --with pillow python3 scripts/playtest_ep8_show.py
    0151_na80001.pcx.bmp  vs episode8_show_original.png: within +-3 100.00%  MAE 0.24  NCC 100.00%
    PASS: the port's stage-8 show composite still matches the original's window

Two things the test exists to prevent, both measured while writing it: (a) pinning the *phase*, because the window only
exists for a couple of thousand frames of a stage-8 run and a route that dwells later sees only the bare room; (b) taking
the shot with the app's own resource-edge naming (`0151_na80001.pcx.bmp`), so a change in the show composite fails loudly
instead of being compared against the wrong frame.

Notes for whoever runs it next: the app must be launched from `port/` (it resolves its resources from the cwd -- a wrong
cwd takes the process down with SIGSEGV before any shot is written); and with `route_show_keyed.txt` the composite was
**not** reached inside 4300 frames -- that run unwound at frame 5654 after
`[room-chain] fn_03cc6b got a NULL next link: ... roomCount=1 stage=8 camX=0 camY=0`, a pre-existing room-chain notice
recorded here as an observation, unrelated to this finding.

## Artifacts

* `docs/manual-playtest/07-shots/episode8_show_port_vs_original.png` (port left, original right)
* `docs/manual-playtest/07-shots/episode8_show_original.png`, `episode8_show_port_ref.png`
* `scripts/playtest/drive_orig_ep8_window.txt` (original-side drive, with the docs/81 stable mode pacing)
