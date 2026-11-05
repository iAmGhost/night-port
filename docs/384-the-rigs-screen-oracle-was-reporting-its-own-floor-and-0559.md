# 384 — the screen oracle was reporting its own floor: 0.5590 carried no information

Three different Pocket runs scored exactly `0.5590` against the original capture (docs/379's unanchored run, docs/380's
anchored run, docs/382's warp run) while their `distinct_pictures` were 2 049, 1 436 and 4 543.  Identical to four
decimals across runs that cannot be identical is not a coincidence, so I tested the instrument.

## 1. The controls

Using `diff_screen.py`'s own `read_frames`/`align` on the original capture's 434-frame signature stream:

| port side | route score |
|---|---|
| the original itself | **1.0000** |
| the warp run (real grids) | 0.559047 |
| a different Pocket run (real grids) | **0.559047** (identical to 6 decimals) |
| the warp run with **every grid zeroed** | **0.559023** |
| the warp run with its grids shuffled | 0.619573 |

A port stream with **no picture in it at all** scores the same as a port stream with 6 001 real frames.  The number I
quoted in docs/379 §5, docs/380 §4 and docs/381 §4 was a content-independent floor, not a fidelity measurement.

## 2. Why

`align()` is a monotone DP whose window is measured **in port-side frames**: `j` may only sit within `±90` frames of
`i * nb / na`.  At the shim's 54.5 presentations/s that is about 1.6 s.  The original capture reaches stage play at
t ≈ 104 s; the warp run reaches it at t ≈ 3 s.  No legal pair exists, the DP takes the cheapest available mismatch,
and the mean collapses to `1 - mean|grid| / 255` — a brightness term, which is why zeroing every grid barely moves it.

The tool's own docstring describes the premise it needs (docs/56, docs/82): *the same route, on a pinned clock*.  An
original playthrough against a warp-entry run does not satisfy it, and the tool did not say so.

## 3. Fixed: `--win` and `--floor`

`diff_screen.py` now takes `--win` (lag window in port frames) and `--floor`, the latter scoring the same alignment
against zeroed grids so the fallback is printed instead of hidden.  With the window opened to 4 000 port frames
(≈73 s, i.e. free realignment) and the floor printed alongside:

| Pocket route | floor | route score | of the way from floor to identical |
|---|---|---|---|
| warp + `route_esc_menu.txt` | 0.5590 | **0.8056** | 55.9 % |
| `--warp-stage 1` | 0.5590 | 0.7786 | 49.8 % |
| `--script route_pause_menu.txt` | 0.5590 | **0.5590** | 0.0 % — pure floor, no signal |
| the original against itself | 0.5590 | 1.0000 | 100 % |

At `--win` 90, 400 and 1 500 every one of those runs scores exactly the floor: **the alignment basis produced nothing
at all at the default window**, which is the honest reading of every 0.5590 in this project's docs.

The ranking that does emerge is consistent with the picture-level results of docs/382-383: the route that plays a stage
*and* opens the menu carries the most signal (55.9 %), the route that plays a stage only slightly less (49.8 %), and the
attract/title route — which never enters a stage — carries none.

## 4. A second, smaller defect worth recording

The original-side grids in docs/379 come from `scripts/rig/pngs_to_frames.py`, and the port-side grids from
`rig_sink.c:553-560`.  Same 40×30 sampling, same `(r*77 + g*150 + b*29) >> 8` weighting — and still only **2 of 40**
compared grids were byte-identical; the differences are ±1 to ±3 per cell (C `77,62,62,60` vs python `78,64,64,60`).
That does not explain the floor, but it does mean a genuinely matched pair across those two capture paths will not score
1.0000, and any future cross-capture grid threshold has to allow for it.

## 5. Corrections folded into the earlier docs

docs/380 §4, docs/381 §4 and docs/382 §1 each lean on 0.5590 in some way; each now carries a correction note pointing
here.  The conclusions those sections drew about *content* stand on the label/best-match basis (docs/381, docs/382,
docs/383), which was measured against pictures rather than a lag-windowed alignment — stage play 55/55 at NCC 0.9998,
slides 130/133, the pause menu 41/41 at 1.0000 — and that basis is unaffected by this defect.

> **Superseded in part (docs/387).**  The `--win 4000` numbers in section 3 above were measured through a second defect:
> `align()` limited each monotone step to 3 port frames per original frame, while these streams need ~32, so honest
> paths were illegal.  With the step limit corrected (`max(3, nb // na + 1)`) the same runs score 0.7753 / 0.7869 /
> 0.9370 (warp), 0.8780 (mirror route) and 0.6428 / 0.6511 / 0.7828 (attract route) against the same 0.5590 floor -- so
> "0.0 % of the way from floor to identical" for the attract route was the bug talking, not the run.  The controls in
> section 1 (self 1.0000, zeroed grids 0.559023) stand, and they are what found this.

