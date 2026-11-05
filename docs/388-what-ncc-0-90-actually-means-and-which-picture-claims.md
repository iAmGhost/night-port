# 388 — what "NCC ≥ 0.90" actually measures, and which picture claims survive exact-pixel testing

docs/381-383 and docs/367 §6 report picture agreement as counts at NCC ≥ 0.90.  This says what that metric is, which of
my claims it can carry, and where it was inflated by a bug of mine.

## 1. The metric: correlation of a 160×120 grey thumbnail

`scripts/playtest_compare_orig_port.py` compares `thumb(img, 160, 120)` -- a **luminance-only, resolution-normalized**
subsample -- with NCC (docs/91's basis).  So a count at "NCC ≥ 0.90" is a **same-scene detector**: it is insensitive to
resolution, cropping, colour accuracy and text sharpness, and it happily pairs a scene with a neighbouring moment of
itself.

Testing the same pairs on raw pixels, pairing chosen on the comparator's own basis:

| original frames | thumb NCC | raw MAE | exact pixels | within 3 LSB |
|---|---|---|---|---|
| gameplay 0259, 0260 | 0.9998, 1.0000 | **0.21, 0.17** | **65.8 %** | **100 %** |
| gameplay 0255-0258 | 0.997-0.9995 | 13.1 - 74.2 | 0 % | 0 % |
| slides 0121-0124 | 0.976-0.983 | 39.9 - 62.1 | 0 % | 0 % |

Two different facts inside one window.  The two best gameplay pairs are **near-identical on raw pixels** (mean absolute
error 0.17-0.21 of 255, every pixel within 3 LSB) -- that is the picture claim worth keeping.  The rest of the window
pairs the *same scene at a different moment*, which the thumbnail cannot tell apart from a match.

The slides window is worse than either: pairing a slide frame against its best witness crashed on shape -- the capture
holds 640-wide, **200-tall** frames there while the port's witnesses at that phase are 480 tall.  A thumbnail resamples
both to 160×120 and reports 0.98, which is not a fidelity statement about a 640×480 slide.  The hard evidence for the
slides remains what docs/363 and docs/373 measured in index space: arena bytes identical to the shipped PCX cards.

## 2. A bug of mine that made a claim look proven

`scripts/pocket/menu_region_compare.py` ended its NCC with `return ... if d > 0 else 1.0`.  A **flat** region has zero
variance, the denominator is 0, and that `else 1.0` handed out a perfect score.  Its own menu numbers -- "41/41 at mean
and minimum NCC 1.0000" (docs/383 §2, repeated in docs/367 §6) -- were partly that fallback, and pairing those same
frames by NCC against a solid-colour witness gave **0.0 % exact pixels**.

The tool now returns NaN for a flat side (never counted, always reported as a blind spot) and prints raw agreement next
to NCC.  Re-measured on the same run (`route_mirror_episode1.txt`, 371 witnesses at 320×200, the same 125×85 menu box):

| window | menu-box NCC | menu-box raw |
|---|---|---|
| `esc_win_` | mean 0.9965, min 0.8567, **40/41 ≥ 0.90** | **MAE 0.45, 99.6 % of pixels within 3 LSB**, 7.3 % exactly equal |
| `esc_win2_` | mean 0.7015, **1/41** | MAE 10.06, 82.9 % within 3 LSB, 62.1 % exactly equal |

So the honest version of the menu claim: **the port draws the game's pause menu essentially identically** -- mean
absolute error 0.45 of 255 over the menu box, 99.6 % of its pixels within 3 LSB -- but not byte-exact, and not in every
window of every run.  "Pixel-identical, 82/82 frames" was too strong, and the tool's fallback is what made it look
supportable.

## 3. What to quote from here on

- **Strong:** the stage picture is near-identical on raw pixels for the frames that share a moment (MAE 0.17-0.21, 100 %
  within 3 LSB); the menu box is drawn with MAE 0.45 and 99.6 % within 3 LSB; the intermission cards and CG shows are
  **byte-identical** to the shipped PCX in index space (docs/363, docs/373).
- **Weak, and to be labelled as such:** any window count at NCC ≥ 0.90 on thumbnails -- it means "the same scene appears
  on both sides", and 354/434 in docs/367 §6 should be read exactly that way.
- **Blind:** a flat region on either side.  Reported as a blind spot now, never as a match.
