# 387 — one anchored route walks the whole episode, and the oracle's alignment had a slope it could not afford

Two things came out of trying docs/386's next step, and the second one changes a number I reported in docs/384.

## 1. `route_mirror_episode1.txt` now covers the episode on the Pocket path

The route follows the original drive file's phase order through `wait res` anchors, and the rig's `res.jsonl` sidecar
proves the game went there:

```
lookups: (1, logo.pcx) (138, opening.fli) (2345, title1.pcx) (2441, 1s1.pcx)
         (2602, 1s2.pcx) (2820, 1s3.pcx) (3039, stage1.pcx)          0 anchor timeouts
```

Label/best-match score for that one run against the original capture:

| original window | matched at NCC ≥ 0.90 |
|---|---|
| slides | **130 / 133** |
| gameplay | **55 / 55** |
| `esc_win_` (pause menu) | **41 / 41** |

Getting there needed a lesson the drive file had already written down.  My first version anchored on `1s3.pcx` and
pressed nothing while waiting; the show stalled at `1s1.pcx` (lookups ended at frame 2441).  Each intermission card
holds until a key arrives, so the show is *walked* — one press per card — exactly as the drive file's three presses with
soft waits do.  Its warning about an extra Enter applies to pressing on the **title card**, which skips the whole show;
pressing on each card is what advances it.

## 2. The oracle's DP could not follow the slope between the two streams

`align()` walked its monotone path with `prev = min(dp[i-1][max(0, j-3):j+1])`: **at most 3 port frames of advance per
original frame**.  These streams need ~32 (14 001 port frames against 434 original ones), so every honest path was
illegal and the DP settled for pairs it could afford.  The step limit is now `max(3, nb // na + 1)`.

Free-search control first — best grid match for original frames against *all* 14 001 port frames:

```
slides    0.983 0.977 0.977 0.977 0.977 0.977 0.977
gameplay  0.991 0.979 0.982 0.976 0.967 0.972 0.971
esc menu  0.972 0.959 0.959 0.959 0.959
```

so the grids did match; the alignment was what could not reach them.  After the fix, with `--floor` printed (0.5590 on
this capture):

| Pocket route | win=90 | win=400 | win=4000 |
|---|---|---|---|
| `--warp-stage 1` | 0.7753 (49.0 %) | 0.7869 (51.7 %) | **0.9370 (85.7 %)** |
| `route_mirror_episode1.txt` | 0.7777 (49.6 %) | 0.7837 (50.9 %) | 0.8780 (72.3 %) |
| `route_pause_menu.txt` (attract/title, never a stage) | 0.6428 (19.0 %) | 0.6511 (20.9 %) | 0.7828 (50.7 %) |
| control: the original against itself | 1.0000 | 1.0000 | 1.0000 |

**This supersedes the win=4000 numbers in docs/384 §3** (0.7786 / 0.8056 / 0.5590): those were measured through the
buggy step limit, and one of them was reported as "pure floor" when the route actually has 50.7 % signal.

## 3. What the two bases now agree on

Sequence alignment and label/best-match pairing rank the same three routes the same way, and both put the stage-playing
routes far above the attract-only route.  With `--win` widened and the step limit fixed, the alignment basis says the
Pocket host follows the original capture's picture sequence at **0.9370** on the warp route — 85.7 % of the way from the
metric's own floor to identical — while the picture basis says the same route matches 55/55 gameplay frames at NCC
0.9998 (docs/382) and 41/41 menu frames at 1.0000 (docs/383).

Remaining honest gap on this basis, unchanged by any of this: the original capture's 15 DOS text-mode frames and the
menu sub-frame timing (docs/382 §3), and the fact that a route score is only meaningful *with* its `--win` and floor
reported, which is now what the tool prints by itself when asked.
