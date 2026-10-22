# The title / attract menu loop (0x039c03)

`fn_00039c03` is the screen the player lands on after the picture show: a five-entry menu drawn from the
`pause.spr` sprite, with two option rows (music, sound effects) and a start entry.

## Layout

| frame slot | meaning |
|---|---|
| `[ebp-4]`  | entry `m` (0..4); parked at `0x64` when the "any key" set ends the loop |
| `[ebp-0x18]`, `[ebp-0x14]`, `[ebp-0x10]` | auto-repeat counters for the down, up, and left/right keys |
| `[ebp-0x24]` | five toggle bytes `f[0..4]`; only the first four are cleared on entry, `f[4]` is the byte at `[ebp-0x20]` and starts at 1 |
| `[ebp-0x38]` | five dwords copied out of the image (`g_tab_531e0`): the `start_row` (Y) of each entry |
| `[ebp-0xc]`, `[ebp-8]` | saved `DAT_00063e24` / `DAT_00063e28` tick counters |
| `[ebp-0x1c]` | return value |

`f[3]` mirrors `DAT_00053338` (music) and `f[2]` mirrors `DAT_00051a10` (sound effects): the entry seeds
the flag from the global, and the left/right key writes it back.

## Key repeat

A press sets its counter to 1. While the key stays held the counter increments and wraps at `0x14`, so a
held key presses again 19 frames later. Releasing parks the counter at `0x13`; a fresh hold is rejected
once and presses on the following frame (the counter goes `0x13 -> 0x14 -> 0` in one frame). So walking
0 -> 4 -> 3 takes one hold, a release, and a two-frame hold.

## The sprite

`DAT_00063e0c` is the `pause.spr` control block written by `fn_0003d8d4`, whose cells are **heap offsets**:

| cell | contents |
|---|---|
| `+0x04` | per-entry destination column |
| `+0x08` | per-entry row count |
| `+0x34` | per-entry byte offset into the RLE stream |
| `+0x38` | the RLE stream itself |

Index 0 is the fixed header (drawn at `start_row` 99, destination 0xbe), index `m+1` is the current entry
(destination 200, `start_row` from `g_tab_531e0[m]`), and entries 2/3/4 additionally draw their toggle
row at index `f[m]+6` (entries 2/3) or `f[4]+8` (entry 4) at destination 0x100, `start_row` 0xad.

`fn_0003bb42` does not advance the RLE cursor for a positive `start_row` -- it only clamps `row_count` to
`0xc8 - start_row` -- so every entry's stream starts exactly at its own offset.

## Leaving

| entry | effect |
|---|---|
| 0 | fade out, `fn_00038f3d(0)` (status screen, four rows), present `DAT_000633b4`, fade in, re-enter this loop |
| 1 | same with `fn_00038cde` (help screen) |
| 2, 3 | clear `DAT_00062c58`, restore the tick counters, `DAT_00052ff8 = 1`, return 1 |
| 4 | as above, but returns 0 (after a DAC fade-out) when `f[4]` is clear |
| any key set | spins until `DAT_00063e89/63eb2/63ebe` are clear, then behaves like entry >= 2 with `m = 0x64` |
