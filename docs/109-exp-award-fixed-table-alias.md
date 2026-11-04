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

`src/f/f.c`: delete the array, add `uint8_t *const g_dat_53ea8 = g_dat_53e9c + 0x0c;` next to the
table (same pattern as the existing `g_dat_53eb8` alias), and drop the extra `+ 0x0c` at the `fn_01441e`
site. `src/f/f.h` declares it as a pointer. Rig cell table regenerated.

## Green after (`/tmp/rig/ents-port2`, same drive ep1_kill3, same rig, no manual steps)

```
dying (state 0x15, +0x30==1) samples: 40        their +0x32 values: [10]        (before: [0])
player +0x2b: 0 -> 20,  +0x52 nonzero in 3 samples   (2 kills x 10 = the shipped per-kill value)
kill counter DAT_000639bc: 0 -> 2 @168891
```

Shipped per-kill EXP is 10 (0 -> 10 after one kill, route-k3 t=272899), so 2 kills -> 20 matches.

## Re-measure after the fix (`ent_list.py --orig ents-orig/orig --port ents-port2`, t=[150,300] s)

```
port states: 1x174 2x124 51x47 53x31 52x30 21x12 3x4        (0x0b/0x0c still ZERO; shipped 0x0b x25, 0x0c x54)
EXP-award gates: port fail={type:410, +0x25=ff:12}   (before: {type:435, +0x30:171, +0x11:1, +0x05:5, +0x25=ff:15})
position jumps >12 px: orig=370  port=83
```

So the award path is fixed (independently confirmed by the EXP total reaching 20), and the port's dying
entities now clear the `+0x30`/`+0x11` gates that used to fail. Two sub-items stay red:

1. **Reaction states 0x0b/0x0c: still zero on the port.** The reaction table `g_tab_51b60` has exactly one
   consumer in the guest (`0x00014cab`, and it only fires for `victim == DAT_00053c7c && victim[+0x17] ==
   0x97`), and neither side's samples ever show state `0x97` -- so the shipped `0x0b`/`0x0c` samples are NOT
   produced by the table. Next step: split the hurt-band census by *which slot/entity* holds them (compare
   the slot against the player pointer, and key the entity by `+0x0d` type index and `+0x123` action-table
   value), then diff the action-chain transition for that same entity type; i.e. the states most likely come
   from the entity's own action-record chain (`+0x11` frame index -> next-state arrays), not from combat.
2. **"Enemy teleporting" is not supported by raw position jumps**: the shipped side has *more* >12 px
   sample-to-sample jumps (370) than the port (83), and the quoted jumps are ordinary walking at 4 Hz
   (17-23 px per ~310 ms). A valid signature has to separate movement-per-tick from discontinuities: use
   the slot's own per-tick speed (dx / dticks) or a jump that exceeds the entity's animation speed by a
   large factor, and check `+0x33 -> 0x33` style state changes at the jump.

## The player half of the reaction gap, isolated by entity type (same two dumps)

Splitting the hurt-band (+0x17 in 0x0b..0x1e) samples by the entity's own type index `+0x0d` and its action
container `+0x123`:

```
ORIG  per-slot: ent00 x47, ent01 x36, ent02 x18
ORIG  type idx(+0x0d): 31 x54 (act 0x51c27c)   0 x47 (act 0x24cb90)      <- idx 0 = the player
PORT  per-slot: ent02 x18, ent01 x14, ent00 x6(player slot), ent03 x1, ent05 x1
PORT  type idx(+0x0d): 31 x40 (act 0x34b744)   -- idx 0 (the player): ZERO samples
```

So the shipped player really does spend 47 samples in the reaction band while the port's player **never**
enters it at all, which is the machine-detectable form of two playtest items at once ("missing hurt-stagger
state / infinite punching" and "player stuck when hit mid-kick") -- and it pairs with the earlier census
that the port's player sits in `0x33` (the punch state, 177 samples in route-k3) with no exit.
Non-player entities (idx 31) do reach the band on the port (40 samples), so the gap is specific to the
player's state machine (`fn_03272f` / `fn_012616` for the player node), not to the entity system in general.

Next step (fresh session): the shipped player's action container is `+0x123 = 0x24cb90`; dump the player's
action-record chain on both sides (the `--chain-cells 0x53c7c` read now has the corrected blob ordering) and
diff the record the player is running at the moment the shipped side moves to `0x0b`/`0x0c`, plus the
`+0x6c` gate byte of that record. The port's own `CHAIN`/`RECDUMP` witness (`NIGHT_CHAINTRACE=1`) already
prints the same chain for entities; widen it to the player node to compare directly.
