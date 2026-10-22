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

## Open item: who owns the font pointer fields

`fn_0383a4` (the integer-digit renderer) reads its glyph table and RLE base from `font+0x34` and
`font+0x3c`, treating both as host pointers, and indexes the glyph table at
`(font+0x34) - 0x40 + ASCII*4` -- sixteen dwords *below* the array. The blocks actually handed to it
come from `fn_03d8d4` / `fn_03d9cf`, whose `s[13]` / `s[14]` fields (offsets `0x34` / `0x38`) hold
4-byte heap offsets allocated by `fn_03de85`. So the widened-pointer convention in `fn_0383a4` and the
offset convention in the loader do not agree, and the -0x40 index needs the block slack that the
original's heap provided. `fn_038445` is ported faithfully (it passes the block through unchanged);
the convention has to be settled once, in `fn_0383a4` plus its four test fixtures, before the digit
paths (`fn_038445`, the HUD score, the title menu) can be tested against real resource streams.
