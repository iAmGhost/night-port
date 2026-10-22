# Script commands (fn_03e20f) and the room's trigger model

`fn_03e20f` (0x03e20f, 2862 B, 677 instructions of jump-table switch code) is the interpreter for the
room's **command list**. `fn_0003ed6f` reads the current record from `DAT_00063e18`, calls
`fn_03e20f(record)`, and then inspects the record itself — so applying a command is expected to make
the record *change*, and the only thing the pump looks at is whether the opcode byte survived.

## Record layout

| offset | meaning |
|---|---|
| +0x00 | opcode byte (1, 2, 3, 4, 6; 0 and 5 do nothing) |
| +0x01 | entity type for the spawn |
| +0x05 | sub-command / trigger slot (opcode 2 selects 1..6; opcode 1 selects left/right side) |
| +0x09 | X |
| +0x0d | Y |
| +0x11 | spawn gate (opcode 1) / third box value (opcodes 3, 4, 6) |
| +0x15 | camera gate (opcode 1) / fourth box value |
| +0x19 | fifth box value (opcode 6 only) |

All the dwords sit at `+1 mod 4`, i.e. the record is a packed byte record read through the flat
address. Every comparison in the function is **signed**.

## Opcodes

* **1 — spawn a trigger entity.** `fn_00041bdc(0x147)` then `fn_010efc(block, anchor_x, 0, Y, type, 1)`;
  the new entity's `+0x1b` receives the record's sub-command, and when the camera gate (`+0x15`) is 1
  the anchor becomes the record's own X and `+0x17` is stamped with 3. Which band guards the spawn
  depends on the room byte `+0x11`:
  * gate (`+0x11`) == 1 → the box armed by opcode **4** (`DAT_5d08c/90/94/98` = X lo, Y lo, X hi, Y hi);
  * otherwise → the live entity count `DAT_53c68` must be **below** `g_tab_528b4[(stage-1)*0x28 + room[0x40]*4]`,
    and the anchor comes from the camera instead of the record: `DAT_639dc - 0x1e` for sub-command 0,
    `DAT_639dc + 0x15e` for 1.
  * With `room[0x11] == 0` the sub-command-0 side additionally requires `DAT_639dc - 0x32 >= X`, and the
    sub-command-1 side requires `X <= DAT_639dc + 0x172`. With `room[0x11] != 0` the polarity flips and the
    bands are `[DAT_639dc - 0x32, DAT_639dc + 0x140]` / `[.., DAT_639dc + 0x172]`.
  * Two of these paths **`return` from the whole function** (the record stays armed, later records in the
    list are not processed this frame). That is the original control flow, kept verbatim.
* **2 — latch the room's edge triggers.** Six sub-commands, each behind the *strict* camera window
  `DAT_639e0 < Y < DAT_639e0 + 0xc8`:

  | sub | extra gate | effect |
  |---|---|---|
  | 1 | `X < DAT_639dc` | `room[0x2a] = X` |
  | 2 | `DAT_639dc < X` | `room[0x2e] = X` |
  | 3 | `DAT_639dc < X < DAT_639dc + 0x140` | `room[0x32] = X` |
  | 4 | same | `room[0x36] = X` |
  | 5 | `DAT_639dc < X` | `room[0x3a] = 1` (**byte**) |
  | 6 | `DAT_639dc < X` | `room[0x3b] = 1` (**byte**) |

* **3 — append a wall/clamp entry**: `g_tab_5c8bc[g_dat_62a8c]` gets the record's four dwords
  (`a1_lo, v_lo, a1_hi, v_hi`; the struct's 5th slot is untouched) and `g_dat_62a8c` is incremented.
  `fn_01586b` / `fn_01545e` read that array as their clamp pre-pass, so this is how a room installs its
  own walls at runtime.
* **4 — arm the trigger box** `DAT_5d08c..98`, but only while **exactly one entity is alive**
  (`DAT_53c68 == 1`, i.e. the player standing alone in the room), inside
  `DAT_639dc .. +0x13f` × `DAT_639e0 .. +0xc7`.
* **6 — arm the second box** `DAT_5d0a0..b0` with five dwords under the same window, and it also
  increments `g_dat_62a8c` — the same counter opcode 3 uses. Almost certainly a bug in the original;
  kept, because a spurious wall entry only adds a clamp row that reads as zero-padded image data.

## New data: `g_tab_528b4`

DS `0x528b4`, 18 rows of 10 dwords (stride 0x28), indexed `[stage-1][room+0x40]`. Stages 1-9 hold mostly
9 with a few 5s, later rows are 1000 (no cap). Nothing in the EXE writes it — image data
(`scripts/extract_ds.py 0x528b4 0x2d0`), same class as `g_tab_52738`.

## Test

`test_fn_03e20f` (13 checks) drives one record at a time over a synthetic room (`room[0]/room[8]` = X
band, `room[0x40]` = cap column, `room[0x11]` = polarity) with the camera at 1000/500, and checks the
opcode byte surviving or being consumed as the visible side effect: opcode 4's one-entity gate and its
off-by-one band edge, opcode 6's counter bump, opcode 3's table row, opcode 2's strict Y gate and byte
latch, opcode 1's spawn gate, the abort path, the camera-anchored anchor values (`+0x1e`, `+0x15e`), the
`+0x17 = 3` stamp, and opcode 5 hitting the default.

## Who pumps it

`fn_0003ed6f` is called from `fn_03d244` (the level-up / room transition) and from the per-frame game
loop, so a command record can survive many frames: the interpreter's early `return`s leave the record
armed, and the pump simply revisits it. `fn_03d244` is also where the command list gets its chance
between rooms -- it runs the status screen, moves `DAT_00063354` to the next room node and then pumps
the list before repainting, which is how a new room's wall rows (opcode 3) and trigger boxes (opcodes
4/6) are installed before the first frame of that room is drawn.
