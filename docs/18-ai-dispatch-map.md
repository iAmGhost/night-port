# The entity AI dispatch map (0x031a8e)

Source: `analysis/pseudo/082_FUN_00031a8e_031a8e.c` (Ghidra) read against the pending set on
2026-07-13. Nothing here is ported yet: `0x031a8e` cannot link before its handler set exists, so this
note exists so the handler ports have a fixed map to work toward.

## Shape of the function

ABI: `EAX = entity block` (Ghidra renders the `PUSH imm; CALL 0x0418ee` stack probe as
`iVar1 = FUN_000418ee()`, but the probe preserves EAX, so `iVar1` is the entity). Ghidra's function length is
991 B; the STATUS row lists 1153 B for the entry (the extra bytes are the trailing epilogue/next-entry gap) —
re-derive the exact span with `scratch/fdis.py 31a8e` before porting.

Two shared state-machine arms run for every entity, then the per-type handler is dispatched:

1. **State 2 (patrol/hesitation)** — `entity[0x17] == 2`: increments `entity[0x103]`; on the 8th call
   it resets that counter and compares the stored position words `entity[0x107]` / `entity[0x10b]`
   against the live position from `fn_0001519f` (x) and `fn_000151d5` (y). If *both* are unchanged the
   entity is considered stuck/idle: `entity[0x17] = 1`, `entity[0x1f] = 0`, `entity[0xaf] = 1`,
   `entity[0xc7] = 1`. Otherwise the live position is stored into `0x107` / `0x10b` for the next round.
2. **State 3 (approach)** — `entity[0x17] == 3`: `entity[0x6b]` is an approach timer. It starts only
   while the entity is horizontally within +/-200 px of the player (`g_dat_53c7c`). Once running, the
   timer times out to state 6 (`entity[0x17] = 6`, `entity[0x11] = 0`) at **> 999** frames, or much
   earlier (**> 99**) when the entity's per-type table entry says the type is fast:
   `*(int32 *)(DAT_53eac + entity[0xd] * 0x88) >= 0x1f`. Proximity also forces state 6: within
   0x50 px horizontally *and* 0x28 px vertically, or within 10 px horizontally.

The player block itself never dispatches: the whole switch is guarded by `entity != g_dat_53c7c`, and
just before it, a player-only fix-up clears `player[0x4a]` whenever the player state word
`player[0x17]` is in `0xb4..0xb9`.

## The per-type behaviour table and the case table

The switch selector is **not** the entity type byte; it is the first word of the type's config record:

```
behaviour = *(int32_t *)(g_dat_53eac + entity[0xd] * 0x88)
```

so `DAT_53eac` is a `0x88`-stride table indexed by `entity[0xd]` (the .stg type byte, e.g. 0x01, 0x0b,
0x1f, 0x20, 0x24, 0x25, 0x29, 0x2a, 0x2e in stage 1) — the same records `fn_010efc` (the entity
constructor) reads. `behaviour` then selects the handler:

| behaviour | handler | size | extra arg |
|---|---|---|---|
| 0x00 | 0x017f13 | 3956 | |
| 0x01 | 0x018e88 | 4031 | |
| 0x02 | 0x019e48 | 4223 | |
| 0x03 | 0x01aec8 | 4063 | |
| 0x04 | 0x01bea8 | 4283 | |
| 0x05 | 0x01cf64 | 4134 | 2 |
| 0x06 | 0x01df8b | 4312 | |
| 0x07 | 0x01f064 | 4363 | |
| 0x08 | 0x020170 | 4139 | |
| 0x09 | 0x01cf64 | 4134 | 0 |
| 0x0a | 0x01cf64 | 4134 | 1 |
| 0x0b | 0x02119c | 4123 | |
| 0x15 | 0x0221b8 | 4067 | |
| 0x1f | 0x02319c | 6748 | 2 |
| 0x20 | 0x024bf9 | 6578 | |
| 0x21 | 0x0265ac | 5695 | 0 |
| 0x22 | 0x027bec | 6940 | 0 |
| 0x23 | 0x029709 | 5867 | 0 |
| 0x24 | 0x02adf5 | 6722 | 0 |
| 0x25 | 0x02c838 | 6803 | 0 |
| 0x26 | 0x02e2cc | 6993 | 0 |
| 0x27 | 0x02fe1e | 7279 | 0 |

Unlisted behaviour values (0x0c-0x14, 0x16-0x1e, >0x27) fall through with no handler.

Notes for the ports:

* `0x01cf64` is shared by behaviours 0x05 / 0x09 / 0x0a and receives the behaviour value as a second
  argument (2 / 0 / 1 respectively — note the remap: behaviour 0x05 passes **2**).
* `0x02319c` (behaviour 0x1f) passes 2; `0x0265ac`..`0x02fe1e` (0x21-0x27) all pass 0.
* That is 22 behaviour slots over **18 distinct handler functions** — all pending, all reported clean
  by the capstone dependency scan (no pending callees), i.e. each depends only on already-ported code.
* Porting order suggestion: start with the smallest (`0x017f13`, 3956 B) and keep one handler per
  session; the disassembly alone is ~900-1700 instructions each.
* Once at least one handler per needed behaviour exists, `0x031a8e` itself can be ported (it also
  unblocks `0x0322f5`, 133 B).
