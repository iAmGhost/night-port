# docs/85 — the original's level-up / status presentation, frame by frame, vs the port

Goal task `orig-levelup-timeline`. docs/61 ported `fn_038f3d` (the level-up / status screen) and left one
open question: in the port's capture the middle ~800 frames showed the **stage framebuffer** instead of
holding the card (frame 2804 help card → 2832-3660 stage-like → 3710 level-up picture). This pass
captures the original's own presentation through dosbox-automation and scores both sides.

## How the original was driven (headless, scripted)

Spine = the proven docs/81 pacing (`scripts/playtest/drive_orig_levelup.txt`, sampled at `--hz 8`), then
one shipped state write in a live episode-2 room:

```
writemem 0x63354 read back 0x38202c      # a room record IS live
readptr 0x53c7c  = 0x631798 [absolute]   # the player entity (docs/89)
writemem 0x6336c 2                       # DAT_0006336c == 2 = "exit reached" -> room loop 0x0003ac2a
```

`DAT_0006336c == 2` is consumed by the room loop (`0x0003ac2a-0x0003ac7b`) and calls **fn_03d244**, which
the port already documents as *the level-up transition*: DAC fade-out (`fn_0406f5` at room+0x48, 50 rows),
move to the next room node, then `fn_038f3d(1)` — the presentation under test. Measured pacing facts:

* the fade-out is real and visible per frame: ch-ratio (frame mean / card mean) falls 0.725 → 0.655 →
  0.481 → … → 0.146 over 14 samples (1.75 s), with the *same* ratio on all three channels;
* `fn_038f3d` waits **one key per picture**, so a drive that presses Enter in bursts sails past the
  level-up picture (attempt 4 pressed 24 keys and ended on a help card: `help8.pcx` 100.00 % / MAE 1.57);
* during the show `DAT_0006336c` is not polled (further writes read back 2 unchanged) and
  `DAT_00063370` stays at its pre-show value (2) — the increment happens at `0x0003b52b` *after* the show.

## The original's timeline (`/tmp/dbgauto/orig_lu5`, one key per window, 19 samples ≈ 2.4 s each)

| window | content (best shipped `.pcx`) | vs levelup2.pcx within ±3 / MAE | first-vs-last stability |
|---|---|---|---|
| p00 | transition (no card; card-match 0.00 %) | 0.00 % / — | 98.11 % changed, maxΔ 255 |
| p01 | transition into help9 (`help9` 8.35 %) | 0.22 % / — | 98.43 % changed |
| p02 | **levelup2** 98.29 % | 98.29 % / 3.04 | 99.00 % changed (ramp-up) |
| p03 | **help8** 100.00 % | 83.21 % | **0.00 % changed, maxΔ 0** (held, dead still) |
| p04 | levelup2 98.03 % | 98.03 % / 3.52 | 17.17 % changed, maxΔ 243 (the value rows animate) |
| p05 | help8 100.00 % | 83.21 % | 0.00 % |
| p06 | levelup2 98.27 % | 98.27 % / 3.16 | 17.13 % |
| p07 | help8 100.00 % | 83.21 % | 0.00 % |
| p08 | levelup2 98.62 % | 98.62 % / 2.93 | 17.16 % |
| p09 | help8 100.00 % | 83.21 % | 0.00 % |
| p10 | levelup2 97.98 % | 97.98 % / 3.58 | 17.16 % |
| p11 | help8 100.00 % | 83.21 % | 0.00 % |

So the shipped presentation is a **two-card alternation** — the status picture (`levelup2.pcx`) and the
instruction card (`help8.pcx`), one key each — not a card over a live room. The two cards agree on
83.21 % of pixels (help8 *is* the status backdrop with different text), which is why every help-card
window still scores 83.21 % against `levelup2.pcx`.

**Premise checks (docs/77 rule) for every settled frame:** channel order = as-captured, anchored by
`help8.pcx` matching at 100.00 % within ±3 / MAE 1.57 while the R/B-exchanged order gives 0.22 %;
per-channel level ratios on the settled frames are (0.996, 0.991, 0.990) … (1.003, 1.002, 1.031) — i.e. the
palette is at its settled level, no gain fitting involved; distinct colours 81-86 in the frames vs 84 in the card.

## The port side (`NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=6000`, `/tmp/port_lu2`)

`warp_01 … warp_07` = **98.18-98.64 % within ±3, MAE 2.50-3.06** vs `levelup2.pcx`
(ch-ratio (0.997,0.994,0.993)); `warp_00` (gain 0.66) and `warp_08` (gain 0.54) are the DAC fade in/out.
New witness in the shipped card-show helper `fn_035f1b` (`NIGHT_GLYPHDBG=1`) prints the port's card order:

```
cardshow seq=1 name=help1.pcx dl=0
```

which is exactly the branch documented at `0x000391b1` ("otherwise `help1.pcx` is shown first", the
`DAT_00063ec3` help-page branch at `0x00039257` is what selects `help10/help8/help9.pcx`).

## Port vs original, on the same picture

Scoring the port's settled level-up frame (`0182_warp_05.bmp`) against the original's settled frames:

| original frame | within ±3 vs port warp_05 | MAE |
|---|---|---|
| 0277_p02_10 | **98.62 %** | 1.57 |
| 0315_p04_10 | 98.38 % | 1.97 |
| 0353_p06_10 | **98.64 %** | 1.49 |
| 0391_p08_10 | **98.66 %** | 1.58 |
| 0429_p10_10 | 98.33 % | 1.87 |

(the mid-window samples of the same windows score 82.91 % — they are inside the original's row-animation
ramp, not a different screen.)

## Answer to docs/61's open question

**Faithful, and the "middle ~800 frames of stage framebuffer" is a capture artifact, not a port phase.**
Evidence: (a) the original holds a card in every sampled window of the show — five windows are byte
identical across 19 samples (0.00 % changed, maxΔ 0) and the other four differ only in 17.2 % of pixels
(the value rows animating), with the palette settled (ch-ratio ≈ 1.00) throughout, so there is no
room-framebuffer phase to reproduce; (b) the port does not produce one either — `fn_038f3d`'s card-show
witness logs only `help1.pcx` in this run and its shot series holds the status picture at 98.18-98.64 %
across seven consecutive samples; (c) port and original score 98.33-98.66 % / MAE 1.49-1.97 against *each
other* on that picture. The real port/original difference this pass found is the **first card** of the show
(port: `help1.pcx`; original: `help9`-ish then `help8`), which the shipped code selects through
`DAT_00063ec3` / the help-page branch — filed as the next repro question, with the `cardshow` witness line
as the one-command check.

## Reproduce

```sh
# original side (container: docker start dbga, then launch the emulator -- note the directory is
# dosbox-automation-0.85.1-7567ec4-linux-x86_64 with a HYPHEN before x86_64, docs/74 prints it wrong):
docker exec dbga bash -c 'for f in /proc/[0-9]*/comm; do case "$(cat $f 2>/dev/null)" in *dosbox*) kill -9 ${f#/proc/}; esac; done'
docker exec -d dbga bash -c "cd /work/dosbox-automation-0.85.1-7567ec4-linux-x86_64 && \
  xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_lu5.log 2>&1"
export DOSBOX_API_TOKEN=...    # the container's bearer token
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_lu5 --drive scripts/playtest/drive_orig_levelup.txt --hz 8

# port side
NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=6000 NIGHT_GLYPHDBG=1 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/night --warp-stage 1 \
  --script ../scripts/playtest/route_walk_fight.txt --shots /tmp/port_lu2 --no-audio \
  --frames 12000 --shot-limit 900

# scoring (per-frame series + the docs/77 premise checks printed with every number)
uv run --with numpy --with pillow python3 scripts/playtest_levelup_timeline.py \
  --orig /tmp/dbgauto/orig_lu5 --port /tmp/port_lu2 --grep warp_ --every 1
```
