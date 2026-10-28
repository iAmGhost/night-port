# 109: EXP-on-kill fixed -- DAT_00053ea8 was a second array for one table

## Red before (docs/107 §q1, docs/108 §2)

* Player record `+0x2b` (EXP total, `f.c:13394-13396` drains the `+0x52` accumulator into it one point per
  even tick) went **0 -> 10** on the shipped side with `+0x52 == 10` observed in one sample, and stayed **0
  in all 3383 port samples** (`+0x52` never nonzero).
* Field-by-field at a kill, the dying enemy is identical on both sides -- state `0x15`, `+0x30 == 1`, both HP
  words 0 -- except `+0x32`: **10 shipped, 0 port**. The award is exactly
  `fn_015295: *(g_player_ent + 0x52) += a0->0x32` (guest `0x15453`), so the award was 0.

## Cause, from the shipped bytes

Guest `0x00015067` (inside `fn_01441e`, the death case `0x00015003-0x00015070`) is
`MOV EDX, dword ptr [EAX + 0x53ea8]` with `EAX = e[+0x0d] * 0x88` -- **no extra +0xc**. And `DAT_00053ea8`
is not a table of its own: it is `DAT_00053e9c + 0xc`, the same 0x88-stride entity-type table addressed at
a different field (the docs/91 aliasing family; the same table is also seen as `0x53eb4 = +0x18` and
`0x53eb8 = +0x1c`, the latter already aliased in the port).

The port kept a **separate, never-written array** `g_dat_53ea8[0x88*4]` *and* added a spurious `+ 0x0c` at
the `fn_01441e` site. Runtime proof from the shipped DS snapshot (route `ents-orig`, t=266819, the dying
enemy with `idx = 31`): row base `0x53e9c + 31*0x88 = 0x54f14`, dwords `[0x23, 0x2, 0, 0xa, 0x2, 0x12c,
0x96, 0x2932]` -- `[0x54f20] = 0x53ea8 + idx*0x88 = 0xa = 10` ✓ equals the observed `+0x32`; the port's
`+0x0c` variant would read `0x96` (150) and its separate array read `0`.

## Fix

`port/src/f/f.c`: delete the array, add `uint8_t *const g_dat_53ea8 = g_dat_53e9c + 0x0c;` next to the
table (same pattern as the existing `g_dat_53eb8` alias), and drop the extra `+ 0x0c` at the `fn_01441e`
site. `port/src/f/f.h` declares it as a pointer. Rig cell table regenerated.

## Green after (`/tmp/rig/ents-port2`, same drive ep1_kill3, same rig, no manual steps)

```
dying (state 0x15, +0x30==1) samples: 40        their +0x32 values: [10]        (before: [0])
player +0x2b: 0 -> 20,  +0x52 nonzero in 3 samples   (2 kills x 10 = the shipped per-kill value)
kill counter DAT_000639bc: 0 -> 2 @168891
```

Shipped per-kill EXP is 10 (0 -> 10 after one kill, route-k3 t=272899), so 2 kills -> 20 matches.
