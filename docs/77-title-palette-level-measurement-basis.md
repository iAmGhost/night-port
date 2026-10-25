# 77. The settled-title palette level: the port is at the shipped level; the ~0.79 figure was fitted to frames that are not the card

Closes the docs/72 log item *"the port's settled title sits at fitted gain ~0.997 against the card while the original
settles at ~0.79; the palette level of the title screen is still unexplained."*

Verdict: **not a port defect.** The port reproduces the shipped card palette verbatim; the ~0.79 number was a
least-squares gain fitted to original-side captured frames that contain **none** of the card's colours, so it never
measured a palette level.

## What the port actually shows

`scripts/playtest_title_palette.py` picks the port frame whose screen-arena indices are byte-identical to the card
decode (`0556_card_448.bmp`, exact-index 1.0000) and compares its presented RGB per index against the shipped
`title3.pcx` trailer palette:

| index | pixels | card RGB | port RGB |
|---|---|---|---|
| 3 | 224 | (16, 24, 57) | (16, 24, 56) |
| 5 | 978 | (33, 8, 8) | (32, 8, 8) |
| 6 | 2410 | (33, 8, 16) | (32, 8, 16) |
| 9 | 10660 | (41, 8, 8) | (40, 8, 8) |
| 11 | 6236 | (49, 8, 16) | (48, 8, 16) |
| 14 | 9379 | (57, 16, 16) | (56, 16, 16) |

Every channel agrees within 1 LSB, the -1 on the red channel being exactly the game's own 6-bit DAC quantization
(`v >> 2 << 2`, i.e. `v & 0xFC`, from `fn_036190` at `0x361f1-0x36203`). Mean RGB: card (78, 47, 40), port (78, 47, 40)
to within that quantization. So the port's title sits at gain 1.0 of the shipped palette -- which is what the
~0.997 figure meant. Nothing is faded or scaled on the port side, and this is consistent with the independent palette
finding in docs/70 (the room DAC push is 768/768 bytes identical to `scratch/res/stage1.wal`).

## Why the original-side ~0.79 cannot be a palette level

`scripts/playtest_orig_title_frames.py` checks the premise before trusting any gain. Reference: `title3.pcx` has
**171 distinct colours**, mean (78, 47, 40), largest flat colour 4.2 % of the frame.

For the original's captured frames that the drive harness labelled as the title sequence
(`/tmp/dbgauto/orig_die2/0018..0025_title_0N.png`, `orig_ep1/0042_title_card.png`, `orig_slides2/0040_title_melt_09.png`
and every other `*title*` PNG in the surviving capture directories), in **both** channel orders:

```
0018..0021_title_01..04   colours=1     exact-card-colour share=1.000   (solid black, MAE 55.4)
0022_title_05             colours=42    exact-card-colour share=0.000   MAE 43.4 / 29.2 (swap), gain 0.797 0.395 0.516 -> worse
0023_title_06             colours=98    exact-card-colour share=0.000   MAE 58.2 / 45.9, gain 1.053 0.746 0.972 -> worse
0024_title_07             colours=124   exact-card-colour share=0.000   MAE 48.5 / 34.0, gain 0.985 0.566 0.737 -> worse
0025_title_08             colours=149   exact-card-colour share=0.000   MAE 52.6 / 37.5
0042_title_card           colours=159   exact-card-colour share=0.000   MAE 53.2 / 41.0
```

* **Zero of those frames' pixels carry a colour that the card's own palette produces** (share 0.000), in either
  channel order. A palette held at 79 % would show colours that are the card's scaled by 0.79 -- and the fitted gains
  are strongly per-channel (0.31/0.40/1.28 one way, 0.80/0.40/0.52 mirrored), which no DAC fade can do: a fade scales
  all three channels together.
* Fitting a gain makes the residual *worse* (e.g. 29.2 -> 34.3), the signature of fitting two different pictures rather
  than one picture at a different level.
* A translation search of +-24 px does not close the gap either (best MAE 43.1 at dx=-4, dy=-24, worse than no shift).
* The frames ramp in distinct-colour count 1 -> 42 -> 98 -> 124 -> 149 -> 159, i.e. the capture shows a *transition*
  (black to something with a collapsing colour count: 42-159 colours against the card's 171), not a settled card.
* Scanning every surviving capture (all `/tmp/dbgauto/*` directories, both `.raw` payloads and `.png` frames, both
  channel orders, against `title1` / `title3` / `title4`): the best frame anywhere is MAE 28.6 with 11.5-31.0 % of
  pixels exactly equal. No surviving original capture contains a verified settled title frame.

So the docs/72 sentence *"title held at fitted gain ~0.79"* was a gain statistic of a non-matching captured picture, and
the port/original "discrepancy" was an artifact of the measurement basis, exactly the failure mode docs/68 already
warned about (a gain-fitted NCC can look convincing while the picture is wrong: it also gave 0.99 for a
channel-mirrored pair in docs/70).

## Rule recorded for future comparisons

Report a fitted gain **next to a premise check**: distinct-colour count, the share of pixels whose colour exists in the
reference's colour set (or index agreement against a settled card decode), and the channel order. If that share is not
close to 1, the gain is meaningless and must not be quoted as a palette level. `scripts/playtest_orig_title_frames.py`
prints all of it in one pass.

## Still open on the original side (filed, not a port defect)

The original's actual palette level at the settled title remains unmeasured, because no surviving capture frame is a
faithful picture of that screen. Settling it needs a capture whose frame is first *proven* to be the card (index
agreement against the shipped decode, as docs/63 does for the port) before a level is read from it; the drive harness
labels (`title_melt`, `title_card`) are not such a proof, as the table above shows. This does not affect the port's
correctness, which is pinned by the shipped data instead.

## Reproduce

    python3 scripts/playtest_title_palette.py /tmp/dbgauto/orig_intro_raw <port_shot_dir>   # port per-index palette
    python3 scripts/playtest_orig_title_frames.py                                           # premise check on captures
