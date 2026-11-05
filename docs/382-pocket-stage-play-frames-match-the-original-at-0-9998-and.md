# 382 — Pocket stage-play frames match the original at NCC 0.9998; the 208 unmatched originals were route coverage

docs/381 §6 concluded the score gap was a route/phase difference but had not proved it.  Splitting the score by the
original's own event windows proves it, and the number for live stage play is better than expected.

## 1. Two Pocket routes, opposite coverage

| Pocket route | what it actually shows |
|---|---|
| `--script route_pause_menu.txt` (docs/381) | boot, attract, `.fli` cutscenes, title, slides — and it **ends on a static 640×480 still** |
| `--warp-stage 1` (docs/363's state-anchored route) | straight into stage 1 and plays it |

The tell in the first run is that its `--warp-stage` counterpart scores 55/55 on that window while it scores 0/55:
the scripted route **never enters a stage**.  Its `sfx=107` transfers are attract-loop sounds, and its 320×200 samples
are movies and menus rather than stage backgrounds.

An earlier version of this section argued the same conclusion from the run's view samples being "all `static`"
(`chg` below a threshold, max 73) -- that instrument is weaker than I assumed, and `scripts/pocket/accuracy_sweep.sh`
now reports it without leaning on it: the warp run, whose stage frames match the original's gameplay frames at 0.9998,
also has no view sample above 1000 changed pixels (its max is 46).  `chg` counts changed pixels between *the host's own
view samples*, and a stage where the player stands still barely changes at all -- which is also true of the original's
captured gameplay frames, and is why they match so well.

## 2. Per-window score, same original capture (434 frames), Pocket side = `--warp-stage 1`, 400 witnesses

| original window | frames | matched at NCC ≥ 0.90 |
|---|---|---|
| **gameplay** | 55 | **55 / 55** |
| slides (intermission cards) | 133 | 130 / 133 |
| title_melt | 42 | 41 / 42 |
| esc_win (pause/menu windows) | 82 | 41 / 82 |
| boot (DOS text mode, 720×400) | 24 | 8 / 24 |
| fli (intro movie) | 54 | 0 / 54 |

Best pairs inside the gameplay window, with the comparator's strict pixel agreement in the last column:

```
0260_gameplay_06.png  0.9998  <-> 0259_gameplay_05.png   strict 1.000/1.000
0271_gameplay_17.png  0.9997  <-> 0271_gameplay_17.png   strict 0.992/0.981
0261_gameplay_07.png  0.9884  <-> 0261_gameplay_07.png   strict 0.988/0.988
```

The `.fli` column is 0/54 here for an obvious reason that is worth stating rather than hiding: a warp run skips the
intro, and that window is 46/54 on the scripted route (docs/381).  The two routes are complementary, and the honest
aggregate is **per-phase**, not one route score:

| phase | Pocket route that covers it | match |
|---|---|---|
| intro `.fli` | scripted attract route | 46 / 54 |
| title + title melt | either | 41 / 42, 1 / 1 |
| intermission slides | either | 130 / 133 |
| **live stage play** | warp-stage route | **55 / 55** |
| pause/menu windows | scripted route visits them; warp route does not | 41 / 82 best |
| DOS text-mode boot | neither reproduces mode 3 | 8 / 24 |

## 3. What is genuinely still divergent

- **The DOS text-mode boot (mode 3, 720×400).**  15 of the original's frames are its real DOS/VGA text screen; the port
  has no text-mode presentation, so 8/24 is the incidental overlap and the rest is a difference the port has never
  claimed to close.  Worth one sentence in the port record rather than a fix.
- **Half of the pause/menu window frames** (41/82).  The windows are reached and their content is close, but the two
  sides are not showing the same sub-frame of the menu animation.  This is where the sequence-alignment basis
  (docs/380's 0.559) and the best-match basis finally agree that something is left.
- The witness cap remains a sampling limit (`NIGHT_VIDRAW_MAX`, docs/381 §6), though coverage was proved not to be the
  score's main limit.

## 4. Method worth keeping

Scoring **one route end-to-end** against an original capture answered the wrong question and produced a 0.559 that
looked like a fidelity verdict.  Scoring **per original event window**, with a Pocket route chosen to be *in* that
phase, answers it: the port's pictures are near-identical where it plays the same content (stage play 55/55, slides
130/133), and the zeros were routes that were never in that phase.  Any future accuracy claim for this port should
name the window and the route together.

Harness-only again: card 0.1.67 unchanged, `src/f` untouched, fixture suite `== ALL PASS ==`.

> **Correction (docs/384).**  The 0.5590 quoted above is `diff_screen.py`'s *floor*, not a fidelity score: its lag
> window is ±90 port frames (~1.6 s), the two runs reach the same phases tens of seconds apart, and the DP's mean then
> collapses to a brightness term -- a port stream with every grid zeroed scores 0.559023 against this same 0.559047.
> With `--win 4000` the same tool reports 0.8056 (warp + ESC), 0.7786 (warp), 0.5590 (attract/title route, i.e. pure
> floor) against a floor of 0.5590.  Statements here that compare *content* rest on the label/best-match basis
> (docs/381-383), which is unaffected.
