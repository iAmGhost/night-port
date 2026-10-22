# The game-over screen (fn_038445) and the open font-field convention

`fn_038445` (0x038445, 1498 B) is the screen shown when the player dies: three pre-rendered
`gameove<N>.pcx` screens composited with six parallax layers plus a blinking CONTINUE countdown.

## Shape

* One-time setup: `number3.spr` (60-byte parallel-array block via `fn_03d8d4`), then
  `sprintf("gameove%d.pcx", n)` (`fn_0428f8`) + `fn_036190` into three `0xfa00` buffers. The video
  window is seeded from buffer 0, the six layers and the digit row are composited once straight into
  the `0xa0000` window, the DAC fades in (`fn_040660`) and the frame gate is entered.
* The frame gate (`0x0003863c`) is re-entered every pass -- the attract branch never returns; only the
  four key flags (`DAT_00063ea4` / `63ea5` / `63ec0` / `63ec1`) leave it.
* Attract branch: the blink counter (`byte [EBP-4]`) is advanced only while `DAT_00063ed0` / `63ed8`
  (select / exit keys) are set, wraps at exactly `0x14`, and is pinned to `0x13` when they are clear;
  the half-screen toggle (`[EBP-0x2c]`) flips once per fresh blink and arms `DAT_00053dd8`.
  The screen shown is `buf[DAT_00053138[(scroll % 78) / 13]]`; every third frame rotates two palette
  rows through `fn_03d67c` + `board_out`.
* Exit branch: arms `DAT_00053ddc`, loads `continue.pcx` (toggle 0) or `gameove1.pcx` (toggle 1),
  creeps the layers +-9 px per frame (`i % 2`) until layer 4's offset slides past -60, fades out,
  frees the digit block and the three screens, decrements `DAT_00053100` and returns 2 (CONTINUE)
  or 0 (plain game over).

## DS data added for it

| DS | meaning |
|---|---|
| 0x53100 | CONTINUE countdown, starts at 3, one per game-over pass |
| 0x53108 | six layer destination offsets: 70, 165, 211, 228, 229, 217 |
| 0x53120 | six layer column widths: 29, 29, 68, 90, 108, 174 |
| 0x53138 | scroll phase -> which of the three screens: 0,1,2,2,1,0 |

Resource names are image strings reached as `const + 0x50000` with the DS file delta: 0x15aa
`number3.spr`, 0x15b6 `gameove%d.pcx`, 0x15c4 `continue.pcx`, 0x15d1 `gameove1.pcx`.

## Resolved: the font pointer fields (fixed with this port)

`fn_0383a4` (the integer-digit renderer) takes the spr struct that `fn_03d8d4` filled, and at
`0x0003840b-0x00038419` it does

```
edx = d * 4                      ; d = the ASCII digit
eax = [font + 0x34]              ; s[13]
edx += eax                       ; char_table + d*4
eax = [font + 0x38]              ; s[14]
eax += [edx - 0xc0]              ; glyph_off = offsets[d - 0x30]
```

so the glyph table is indexed from `'0'` at index 0 (`- 0xc0`, not `- 0x40`), and both cells are the
`s[13]` / `s[14]` cells that `fn_03de85` fills during `fn_03d9cf` -- heap offsets in this port, not host
addresses. An earlier pass of the port had widened those two cells to 8-byte host pointers at `+0x34` /
`+0x3c`, which no production loader could satisfy; that is now corrected, and `test_fn_038445` proves
the whole chain (a real `number3.spr` two-pass load feeding a real digit blit) works.

## Follow-up: the same convention in fn_038b30, fn_038c23 and fn_0137c1

Porting `fn_038f3d` (the level-up screen) forced the issue, because it renders numbers with
`fn_038b30` / `fn_038c23` using `number1.spr` / `number2.spr` structs produced by `fn_03d8d4`.
Reading the machine code:

| function | glyph index | cells |
|---|---|---|
| `fn_0383a4` | `[char_table + d*4 - 0xc0]` (0x0003840b-0x00038419) | `font+0x34`, `font+0x38` dwords |
| `fn_038b30` | `[char_table + d*4 - 0xc0]` (0x00038be6-0x00038bf4) | same |
| `fn_038c23` | `[char_table + d*4 - 0xc0]` (0x00038c9f-0x00038cad) | same |
| `fn_0137c1` | `[char_table + d*4 - 0xbc]` (0x00013884-0x00013891) | same |

An earlier pass had widened those two dwords into 64-bit host pointers at `+0x34` / `+0x3c` and used
`-0x40` / `-0x3c` indices, which no `fn_03d9cf`-produced struct can satisfy (it stores heap offsets in
`s[13]` / `s[14]`). All four now resolve the two cells through `g_heap_base`.

Quirk worth knowing: with the `-0xc0` form, `char_table[0]` is BOTH the blank pad glyph of `fn_038b30`
and the digit `'0'`; with `-0xbc`, `char_table[1]` is BOTH the blank pad of `fn_0137c1` and the digit
`'1'`. That is what the original does, and the tests now say so.
