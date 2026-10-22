# The per-entity step dispatch (0x01194c)

`fn_0001194c` is what an entity does with its current state on a given frame. It is one jump table on
`entity[+0x17]`; each entry chains into range tests on `entity[+0x0d]` (the type/serial) and finally on
`entity[+0x11]` (the action code currently running). There is no state machine here beyond those three
fields -- the table says which of three leaves the combination takes.

## Leaves

| leaf | effect |
|---|---|
| `RYMV` | `fn_01545e(e, prng_next() % 0x15 - 10)` -- move the entity itself by a random -10..10 (band-clamped) -- then `fn_011827(e, -10, -4, 1, 4)`: build the trail companion at `+0x01 - 10`, `+0x09 + 1`, state 4 |
| `MV` | that spawn alone (state 4 asks for `+9 / +1`; states 0x8c / 0x8d ask for `+0x20 / +1`) |
| `SND` | `fn_0113e7(e)` -- the three-way companion spray at -0x2c / -0x0c / +0x16 |

`fn_011827` takes `(src, dx, EBX, dy, stack-arg)`, which is why the trail spawn is 1 pixel **down**: the
original hands `EBX = -4` (which lands in the spawn's `+0x05`) and `ECX = +1` (the `+0x09` delta).

## Table entries

| state | shape |
|---|---|
| 4 | action 0x12 / 0x0b -> `MV(+9,+1)` |
| 0x15 | types 0x32, 7: 0x2b -> `RYMV`, 0x3e -> `SND`; types 1, 2: 0x24 -> `MV`, 0x37 -> `SND`; anything else: 0x3a -> `RYMV`, 0x4d -> `SND`. **The player skips the nudge** (0x3a -> `MV`) and has its own pair 0x3a / 0x50 |
| 0x16 | player: 0x25 -> `MV`, 0x37 -> `SND`; anyone else: 0x22 -> `RYMV`, 0x33 -> `SND` |
| 0x17 | full band tree: types 1, 2 -> 0x35 `MV` / 0x4e `SND`; band 0x24..0x28 -> 0x4f / 0x68; band 0x29..0x2d -> 0x4c / 0x65; band 0x2e..0x32 -> 0x41 / 0x5a; everywhere else -> 0x4b `RYMV` / 100 `SND` |
| 0x18 | the same band tree with 10 as the move and pairs 0x3f/0x59, 0x32/0x43, 0x3e/0x50 |
| 0x1a | bands 0x24..0x2d and 0x2e..0x32 folded: 0x20 `RYMV` / 0x33 `SND`, inner bands 0x1d / 0x31, 0x20 / 0x34 |
| 0x1d | 0x30 `RYMV` / 0x42 `SND` |
| 0x1e | 0x31 `RYMV` / 0x3b `SND` |
| 0x20 | bands as 0x18, but the innermost tests additionally require the entity **not** to be the player; pairs 0x2d/0x40, 0x24/0x36, 0x33/0x45 |
| 0x8c, 0x8d | player only: action 0x3f (0x8c) or 0x38 (0x8d) -> `MV(+0x20,+1)` |

Empty entries: 0..3, 5..0x14, 0x19, 0x1b, 0x1c, 0x1f, 0x21..0x8b, 0x8e.. -- the entity simply does nothing
extra that frame.

## Two transcription hazards

1. Ghidra's `if (state < 0x16) { if (3 < state) { if (state < 5) {...} else if (state == 0x15) {...} } }`
   nests the 0x15 case *inside* the `< 0x16` guard. Re-materialising those cases as a sibling `else if`
   chain at the outer level makes state 0x15 unreachable, since 0x15 < 0x16.
2. The band tests are deliberately overlapping (`ty < 0x24 || 0x28 < ty` after `ty < 0x1f || 0x23 < ty`
   means types 0x24..0x28 take the *else* branch of the first test). Copying the tree verbatim keeps that
   visible; flattening it into a switch would silently change which action wins for a boundary type.
