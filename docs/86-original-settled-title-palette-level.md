# 86. The original's settled title palette is the shipped palette at gain 1.00 — docs/77's gap was a half-finished wipe plus an un-mirrored 640x480 capture

Closes the last open item of docs/77 ("the original's actual palette level at the settled title remains
unmeasured, because no surviving capture frame is a faithful picture of that screen").

**Answer: the original settles at the shipped level — per-index median ratio (1.000, 0.988, 0.988), delta
vs the card palette mean (-0.42, -1.29, -1.27), p5/p95 = -2..+2 LSB — and the port matches that settled
screen at 99.99 % within ±3, MAE 0.13.** The docs/72 "~0.79 gain" was two measurement-basis failures
compounded: the capture window ended *inside* the slow title wipe, and the 640x480 payload of this capture
configuration is R/B-mirrored, which `scripts/playtest_orig_drive.py --swap13` does not correct because it
mirrors only `h == 200` payloads.

## Capture (headless, scripted; docs/77's "capture-method fix")

`scripts/playtest/drive_orig_title_hold.txt` — same spine as docs/81/85 but the title window is simply held
and sampled long enough (`record title_long 26000`, `--hz 4`, 101 samples of the title screen):

```sh
docker exec dbga bash -c 'for f in /proc/[0-9]*/comm; do case "$(cat $f 2>/dev/null)" in *dosbox*) kill -9 ${f#/proc/};; esac; done'
docker exec -d dbga bash -c "cd /work/dosbox-automation-0.85.1-7567ec4-linux-x86_64 && \
  xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_title1.log 2>&1"
export DOSBOX_API_TOKEN=...
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_title1 --drive scripts/playtest/drive_orig_title_hold.txt --hz 4
```

Why docs/77 found nothing: its own reference (`/tmp/dbgauto/orig_slides2/events.tsv`) shows the harness's
card similarity ramping `0.0000 → 0.3695 (title_melt_09) → 0.8390 (title_melt_35)` and that was the *last*
sample of the 6 s window. The wipe is ~8.5 s long, so every older capture is a mid-wipe frame.

## The wipe and the hold (per-frame series, `scripts/playtest_title_palette_level.py`)

Scored against the independent decode of `scratch/res/title3.pcx` (640x480, 255 palette entries), in the
**corrected** channel order (`within3(swapped)`); `changed-vs-prev>3` is the byte movement between samples:

```
title_long_01..08   colours=  1  within3= 0.00%                                   (black: mode-480 fill)
title_long_09       colours= 38  within3= 6.33%   changed=100.00%   t=36184 ms
title_long_16       colours=159  within3=19.39%   changed= 70.64%
title_long_24       colours=228  within3=34.87%   changed= 55.58%
title_long_33       colours=255  within3=64.51%   changed= 33.28%
title_long_38       colours=255  within3=94.21%   changed=  6.83%
title_long_42       colours=255  within3=99.99%   changed=  5.79%   t=44724 ms  <- settled
   ... +59 consecutive samples, all identical metrics; sample_50 vs sample_101 changed>3 = 0.0000%
title_long_101      colours=255  within3=99.99%                                  t=59952 ms
```

So the palette/picture is **settled and byte-stable for 15.2 s** (60 samples), with the distinct-colour count
pinned at 255 for the whole plateau — the docs/77 premise "distinct-colour count stable across the hold".

## Premise checks on the settled frame (`0144_title_long_101.png`)

| check | value |
|---|---|
| distinct colours | 255 (card palette has 255 entries) |
| **picture identity** (per-index functional map over all 307 200 pixels) | indices seen 255/255, **colours per index 1.00**, **dominant class 100.00 %** |
| exact-card-colour share (±1 LSB) | as-captured **3.12 %**, R/B-swapped **51.07 %** |
| within ±3 of the card RGB | as-captured **0.00 %**, R/B-swapped **99.99 %** |
| vs the port's settled title frame | as-captured 0.00 % / MAE 64.13, R/B-swapped **99.99 % / MAE 0.13** |

The identity row is the anchor that docs/77 asked for: every screen index of the shipped `title3.pcx` decode
maps to exactly **one** RGB value in the captured frame (1.00 colours/index, 100.00 % of pixels in that
dominant class), i.e. the captured picture *is* the shipped card index-for-index. That also makes the channel
order unambiguous without an R==B share: the full 255-entry colour map matches under an R/B exchange and
matches **zero** pixels under identity. (The ±1 LSB exact-colour share stays at 51 % only because the DAC
output quantizes — see the table — so use ±3 for the share on captures.)

## Per-index palette table (card / original as captured / original swapped)

| idx | pixels | card RGB | original (as captured) | original swapped | swapped/card R,G,B |
|---|---|---|---|---|---|
| 9 | 10660 | (41, 8, 8) | (8, 8, 40) | (40, 8, 8) | 0.976, 1.000, 1.000 |
| 11 | 6236 | (49, 8, 16) | (16, 8, 49) | (49, 8, 16) | 1.000, 1.000, 1.000 |
| 14 | 9379 | (57, 16, 16) | (16, 16, 57) | (57, 16, 16) | 1.000, 1.000, 1.000 |
| 21 | 7043 | (74, 8, 8) | (8, 8, 73) | (73, 8, 8) | 0.986, 1.000, 1.000 |
| 23 | 5899 | (82, 24, 24) | (24, 24, 81) | (81, 24, 24) | 0.988, 1.000, 1.000 |
| 69 | 7062 | (140, 41, 41) | (41, 41, 140) | (140, 41, 41) | 1.000, 1.000, 1.000 |

Over all 255 used indices: median ratio (1.000, 0.9878, 0.9878); mean delta (-0.42, -1.29, -1.27),
p5/p95 -2/+2; least-squares gain over the settled palette **0.9796** — now with the premise satisfied
(dominant class 100.00 %, share 51.07 %). The deltas are the 6-bit DAC truncation, the same `v & 0xFC`
quantization docs/77 measured on the port side (`fn_036190` at `0x361f1-0x36203`), so both sides sit on the
shipped palette and differ only by truncation noise.

## Consequences

1. **No port defect.** The port's title (`/tmp/port_title/0741_card_541.bmp`) is 100.00 % within ±3 of the
   shipped card, and 99.99 % / MAE 0.13 against the original's settled screen.
2. **docs/72's "settles at ~0.79" is withdrawn** — it was a gain fitted to mid-wipe frames in the wrong
   channel order (docs/77 already proved the frames carried no card colours; the wipe explains why).
3. **Capture-configuration rule, sharpened:** the mirror belongs to the *payload format*, not the mode.
   `playtest_orig_drive.py --swap13` currently conditions on `h == 200`, so 640x480 payloads come out
   mirrored. Any colour-exactness score against a 640x480 original frame must apply the exchange (this
   script reports both orders) or the harness must mirror by payload format. Existing 640x480 original-side
   numbers in docs/69/71 are timing / per-channel-mean / edge statistics, so their conclusions stand, but
   they were never colour-exactness scores.

## Reproduce

```sh
uv run --with numpy --with pillow python3 scripts/playtest_title_palette_level.py \
  --orig /tmp/dbgauto/orig_title1 --pick title_long --port /tmp/port_title
# port shots: cd port && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./test/night --no-audio \
#   --frames 1200 --shots /tmp/port_title --shot-limit 900
```
