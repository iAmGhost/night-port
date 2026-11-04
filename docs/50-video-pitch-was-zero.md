# 50 — the frame pitch was never set: every pitch-relative write collapsed onto row 0

The reported "health bar in the wrong place" was not an offset slip in the HUD code: the port's copy of
**DAT_00040010 (the frame pitch) was never given a value at runtime.**

The original keeps one global for the pitch. The pixel helpers read it directly:

```
0x040024  base[c * DAT_00040010 + b] = d
0x04052c  rect_fill: p = a + c * DAT_00040010 + b, rows advance by DAT_00040010 - width
0x040506  vline_set: the caller passes the pitch explicitly (0x140)
```

and it is written exactly once, by `fn_040631(0x140)` in the boot/mode sequence (0x365cd).

The port had **two names** for that global — `g_dat_40010` (written by the ported `fn_040631`) and
`g_video_stride` (read by the ported helpers, `int32_t g_video_stride = 0;`). Nothing ever copied one to
the other, so in a real run the pitch stayed 0:

* `rect_fill(canvas, 0x1f, 0x17, 0x64, 2, c14)` (the player's gauge fill, 100x2 at column 0x1f, row 0x17)
  wrote at `0 * 0 + 0x1f` → **a 100-px bar pinned to row 0**, which is the magenta strip the report shows
  at the very top of the window;
* the single-pixel helper `0x040024` likewise wrote every dot to row 0.

`fix: fn_040631` now moves both names together, with the reasoning in a comment. Measured with
`NIGHT_HUDDBG=1` (a gauge-geometry trace: slot, gauge index, amount, split values, plus pitch-relative
`rect_fill`/`vline_set` calls that land near the top of the screen — before the fix no `rect_fill` was
reported near row 0 even though the bar was visibly there, which is what pointed at the pitch rather than
the call site).

Before / after (`02-shots/01-playing-episode1-sprites-fixed.bmp` → `02-shots/02-playing-hud-fixed.bmp`):

| row | before | after |
|---|---|---|
| 0 | 100 px of gauge colour at x=31..130 (nothing should be drawn there) | background |
| 23..24 | empty | gauge fill, x=31..130 |

Compared against the original capture `analysis/refs/stage1-screenshot.png` (portrait, `EXP 00000`, the filled
bar sitting directly under the EXP row) the HUD layout now agrees.

## Regression test

Added to the `fn_040631` test: `fn_040631(0x1234)` must move `g_video_stride` too, and a `rect_fill` into
a 320-wide canvas with the real `0x140` pitch must paint exactly the 100x2 block at `(0x1f, 0x17)` and
nothing on the neighbouring rows/columns. 2805 checks, ALL PASS.

## Note for the other open issues

Because the two broken helpers are used by fades, markers and single-pixel effects, this fix is also a
prerequisite for judging the remaining visual reports (the intro .fli artifact and the stage-transition
corruption): those were previously evaluated against a frame where pitch-relative writes were landing on
row 0.

## The remaining offset slip: `rect_fill`'s argument pair was transposed in `fn_01348f`

Fixing the pitch made the bar appear, but at the wrong place. Reading the push order at the call sites
settles it — pushes are right-to-left, so the parameters read in reverse:

```
00013745  push c14            ; colour
00013749  push 2              ; height
0001374b  push 0x64           ; width
0001374d  push 0x1f           ; -> 3rd parameter
0001374f  imul eax, [slot], 0xaa ; + 0x17
00013759  push eax            ; -> 2nd parameter
0001375d  push base           ; 1st
0001375e  call 0x4052c        ; rect_fill(base, COLUMN, ROW, width, height, colour)
```

so the gauge fill is `column = slot*0xaa + 0x17`, `row = 0x1f` — which sits inside the gauge panel blitted
at row `0x1d` (29) by `fn_03bb42`. The port had those two arguments swapped (`0x1f` as column,
`slot*0xaa+0x17` as row), putting the bar 8 px right and 8 px high. Both sites (the 100-px `q1` fill at
0x1375e and the remainder `r1` fill at 0x13789) are corrected; the third site in the same function
(0x13696: `0x20`, `0xbb`) was already in the right order, verified the same way.

Measured after the fix (stage-1 gameplay frame, 320x200): the fill is exactly **columns 23..122, rows
31..32** — `slot*0xaa + 0x17 = 23`, `0x1f = 31`, 100 x 2 — with no red pixels on rows 27-30 or 33-37. A
regression check now pins the (base, column, row) order itself.
