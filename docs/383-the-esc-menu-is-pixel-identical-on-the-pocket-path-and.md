# 383 — the ESC menu is pixel-identical on the Pocket path (region NCC 1.0000), and the menu gap was an absent key press

docs/382 left one window unexplained: the original's pause/menu frames matched only 41 of 82.  It is now explained,
and the answer is not a rendering defect.

## 1. The port routes never opened the menu

The original's ESC window is the game's own menu — LEVEL UP MODE / COMMAND LIST / BGM MODE / SOUND MODE / EXIT TO DOS —
drawn over the live stage.  Its fill is a purple no part of the stage uses, so it can be detected in pixels rather
than assumed.  Scoring 400 witnesses from `--warp-stage 1`:

```
port witnesses with a purple menu fill (>1500 px): 0
original esc frames with the fill: 41 of 48
```

Zero.  The Pocket side simply never opened the menu, so the window was being scored against witnesses that could not
contain it.  That is why the same window scored 1/41 on one route, 41/82 on another, and had a *negative* NCC in its
worst pairs: the pairs were a menu-covered frame against a menu-free stage frame.

Two earlier attempts at this measurement were wrong before they were right, and both are worth naming: matching the
window against `events.tsv`'s **label** column (values like `1s3`) instead of its event column, and then taking the
filename from the **bios** column, which handed PIL a file named `19`.  The columns are
`seq t_ms event label sim mode bios file`.

## 2. Press ESC on the Pocket path and the menu is exact

`scripts/playtest/route_esc_menu.txt` (DOS make codes on the shim's direct key path, docs/377 §5) with `--warp-stage 1`:

```
script: scripts/playtest/route_esc_menu.txt -> 4 key spans on the direct key path (0 held to the end), 0 `wait res` anchors
witnesses containing the purple menu fill: 219 of 400
```

**The menu opens on the Pocket path.**  Then measuring the menu box itself — found from pixels, 125×85 px at
x190..314, y100..184 (`scripts/pocket/menu_region_compare.py`) — separately from the whole frame:

| window | whole-frame NCC ≥ 0.90 | **menu region NCC** |
|---|---|---|
| `esc_win_` (41 frames) | 1 / 41 | **41 / 41, mean 1.0000, min 1.0000** |
| `esc_win2_` (41 frames) | 40 / 41 | **41 / 41, mean 1.0000, min 1.0000** |

The menu region is **identical in every frame of both windows** — every one of the 10 625 pixels.

### Which window a given run witnesses is not stable, and that is a property of the rig

`scripts/pocket/accuracy_sweep.sh` (section 4) re-ran all three routes independently:

| route | `esc_win_` menu box | `esc_win2_` menu box | gameplay, whole frame |
|---|---|---|---|
| `--warp-stage 1` alone | 0 / 41, no menu present | 0 / 41 | 55 / 55 |
| `--warp-stage 1` + `route_esc_menu.txt` | 40 / 41 | 1 / 41 | 55 / 55 |
| `--script route_pause_menu.txt` | **41 / 41 at 1.0000** | **41 / 41 at 1.0000** | 0 / 55 |

Same seed, same route file, different witnesses per run: the shim's pad timeline runs on wall clock (docs/333 §5), so a
route reaches a given phase at a slightly different game frame each time and witnesses only part of a window.  **The
claim that survives that is the exactness, not the coverage**: wherever a witness contains the menu, its box matches the
original's menu box with mean and minimum NCC 1.0000 — and a route that never opens the menu has nothing to match (0/41).
The table in section 2 is one run; this one is the shape to expect.
Whole-frame agreement then depends only on the stage behind it: the second window agrees 40/41 because the port's
stage and the original's happen to be at comparable animation moments, and the first window agrees 1/41 because they
are not (the warp route enters stage 1 directly; the original reached it by playing).

## 3. What this changes about the accuracy record

The last unexplained window in docs/382 is now split into a verified part and a named residual:

- **verified:** the port draws the game's pause menu exactly — mean and minimum NCC 1.0000 over its box, in every
  window a run reaches it (41/41 per window, docs/383 §2).
- **residual:** the stage *behind* a menu is at a different progression point between an original playthrough and a
  warp-entry run, which is a comparison condition rather than a defect.  Closing it means driving the original and the
  port along the same stage progression, which is the same open problem docs/380 §4 named from the alignment side.

It also adds a route the port's own testing should keep: the menu is reachable on the Pocket path with keyboard keys,
and `route_esc_menu.txt` is the short way to prove it.

## 4. One command for the whole table

`scripts/pocket/accuracy_sweep.sh` runs the three Pocket-shim routes (warp-stage, warp + ESC, scripted attract),
witnesses each with `NIGHT_VIDRAW_MAX`, decodes the witnesses to BMPs, and prints the per-window whole-frame score
alongside the menu-box score.  It needs an original-side capture directory (`ORIG=/tmp/orig71` by default, built as
docs/379 describes) and reports each run's view-sample statistics as measurements only — docs/382 §1 explains why the
`chg` figure is weaker evidence than it first looks.
