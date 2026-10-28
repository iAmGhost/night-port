# 118. State injection in the drive language (why keypress-only replay is the wrong lever for rare states)

Date: 2026-10-29. Prompted by the user: *"why are you only simulating by keypresses? isn't enemy state
simulatable programmatically? like set health to 1 and simulate damage."* Correct, and now implemented on the
port side.

## 1. What was added

`port/src/app/main.c` (script parser + driver) and `port/src/app/rig_sink.c`:

| drive line | effect |
|---|---|
| `12345 poke 0x639bc 0` | write a DS global through the **rig's own cell table** (`rig_ds_write`) |
| `12345 ent 2 0x29 1` | write 4 bytes at `record+0x29` of entity slot 2 in the **same `+0x13f` list walk the state dumper uses** (`rig_ent_slot` / `rig_ent_poke`) |
| `12345 ent 2 0x17 0x15` | force a record into a state (here: reeling) |

The injection goes through exactly the objects the state oracle reads, so an injected value and a
game-produced value are indistinguishable to the diff -- that is what makes it usable as a rig primitive
rather than a debug hack. Both verbs emit rig markers (`poke ds ...`, `poke entNN +off = val`) on the dump
timeline, so windows stay attributable.

Verification (`scripts/rig/drives/ep1_poke.txt`, 40 rounds of `ent N 0x17 0x15` + one attack, plus
`ent N 0x29 1` to force lethal hp; run `/tmp/rig/pk2`):

```
injections executed: 120        (all logged "ok", none "no such slot")
reeling-state (0x15) entity samples: 10 in ep1_kill3 (no injection)  ->  659 with injection
fallen-family (0x5b..0x66) samples : 0                              ->    0
atype alphabet on hits that landed  : {1, 2}                        ->  {1, 2, 11}
```

## 2. What the experiment falsifies

docs/117 inferred the knockdown path as *"the victim's current state indexes the hit record, so a second hit
while the victim is reeling (0x15) resolves to a record declaring type 22..32 -> fallen 0x66"*. Holding the
victim in 0x15 for 659 entity-samples and landing hits **did not** produce any type >= 22 and no fallen
state. So the lookup at `f.c:24266` (guest 0x00014439-59)

```c
rec_e = *(hb + *(hb + *(e + 0x123) + 0x04) + 4 * *(e + 0x17));
```

is not being driven by the victim state I injected -- either `e` there is the *attacker* (a combo finisher
declares the knockdown type), or the chain container at `+0x123` has to be advanced (docs/110 measured that
the port consumes chain slots more often than shipped, which is the same suspect seen from the other side).
The experiment constrains the mechanism and docs/117's sentence should be read as a hypothesis, not a fact.

## 3. A probe that must not be re-added carelessly

A census verb was prototyped (`ent <slot> 0xffffffff 0`: walk states 0..0xff, resolve the record the way
`fn_01441e` does, and report the declared type ranges). It ran and printed ranges like `types 0..226` /
`0..252` -- implausible -- and then **crashed the port (SIGSEGV)**. The block has been removed from the tree
(`make -C port app` clean, `ep1_poke.txt` run rc=0 afterwards). Recording it because the census is still the
right question: the answer needs the record layout decoded properly first (the `+0x00` slot count and the
`+0x38` type byte are per-record-array, and the level-2 chain pointer at `*(hb + *(e+0x123) + 0x04)` is not
validated by the guest either -- the shipped code trusts the disc).

## 4. Remaining asymmetry to close

Injection currently exists **only on the port side**. For a differential statement about injected states the
shipped side needs the same pokes through the emulator's memory API (`replay_original.py` already resolves DS
physical addresses for reads; writes need the corresponding endpoint, and entity records additionally need the
heap-relative resolution recorded as an open item in docs/115). Until then, injected-state runs are
port-side *reachability* probes (does the port ever produce the state when its precondition is forced), and
the shipped comparison stays on identical keypress drives -- which is also why a fallen-state red/green is
still open despite docs/117 and this experiment.

## 5. Player-targeted injection (`play`), and why the first combo sweep proved nothing

`12345 play 0x17 0x29` writes into **the player entity** (`rig_player_poke` -> `g_player_ent`), because the
shipped hit code resolves the reaction type through the striker: in `fn_01441e` the record index is
`*(e + 0x17)` where `e` is the attacker and `cur` the victim (the victim is written at `f.c:24518`). That is
consistent with the user's reading -- *"maybe it's related with the player's combo state, not the period
between damage"* -- and inconsistent with my docs/117 hypothesis, which the negative test in §2 already
contradicts.

The first sweep (`scripts/rig/drives/ep1_combo2.txt`: set the player's state to 0x8c..0x9f and a handful of
combat states, then press attack once from each) executed all 27 injections but landed only 4 hits, with
atype still {1,2}. The design is wrong: overwriting `+0x17` immediately before pressing attack cancels the
animation that would have produced the combo position, so the state at hit time is whatever the fresh attack
sets, not the injected one. A valid probe has to advance the *chain container* (`+0x123` list, the
slot-consumption quantity docs/110 measured as divergent) or land the second hit of a genuine combo, and that
needs the chain layout decoded first -- which is the same prerequisite as the census that crashed in §3.

## 6. Second negative test: DS 0x62d58 is not the combo lever

`scripts/rig/drives/ep1_combo3.txt` pokes DS `0x62d58` (the action-chain state id seen in docs/107) through
16 values and lands one attack per value: all 16 injections executed, 7 hits landed, **atype stayed {1, 2}**.
So the chain position that selects a knockdown-type record is not that DS global. The record resolution reads
the chain through the *entity's own* container -- `*(e + 0x123)`, then `*(hb + that + 0x04)`, then
`+ 4 * *(e + 0x17)` (`f.c:24266`) -- so the quantity that must be advanced lives in heap records, and the
probe needs that container's layout (how the slot cursor and the state index move) decoded from the
disassembly before injection can force a combo finisher. Recorded as a negative test, not as evidence that
the user's combo reading is wrong: the reading predicts the *striker's* record, and that is exactly the
subsystem this probe failed to reach.

## 7. Where `+0x123` comes from (static, this turn) -- the lever is the striker's *state*, and it is per entity type

Program-wide, only two sites write `entity+0x123`: `fn_010efc` at 0x000110ae (a second write at `fn_0376c8`:
0x00037846). `fn_010efc` is the **entity constructor**: it fills the record from the type tables --
`+0x25` from `DAT_00053ea0[type*0x88]`, `+0x29` (hit points, u16) from `DAT_00053ea4[type*0x44]`, `+0x0d` =
type -- and then stores the action-container heap offset at `+0x123`. So the container is **fixed per entity
type**, chosen at spawn; there is no per-hit chain cursor in a DS global to poke (consistent with the two
negative tests above).

Consequence for the fallen item: the only input to the record selection

```
rec_e = *(hb + *(hb + *(e+0x123) + 0x04) + 4 * *(e+0x17))
atype = hb[rec_e + slot + 0x38]
```

is the striker's **state value** and the **slot** (`+0x11`, modulo `*(hb+rec_e)`). So the decisive measurement
is the per-(type, state, slot) table of declared reaction types -- which is exactly the census that crashed in
§3 because the record-array layout was assumed rather than decoded. Decoding that layout (the container
header: `+0x00` count, `+0x6d` frame array, `+0x38` type bytes, and how `DAT_00053ea0[type*0x88]`/
`DAT_00053ea4[type*0x44]` point into it) is the next static task, and it is also the strongest candidate for
the fidelity audit, because it is a guest data convention the port must mirror exactly.

## 8. Decoding plan for the action container (the only thing keeping the fallen item open)

Facts already established, so the next pass can start here rather than re-deriving:

1. `entity+0x123` is set at spawn by `fn_010efc` (0x000110ae) from the type tables (`DAT_00053ea0[type*0x88]`,
   `DAT_00053ea4[type*0x44]`) -- fixed per entity type; and rewritten only by `fn_0376c8` (0x00037846).
2. `fn_01441e` reads it as: `lvl2 = *(hb + *(e+0x123) + 0x04)`, `rec_e = *(hb + lvl2 + 4*state)`,
   `nslots = *(hb + rec_e)`, `fslot = e[+0x11] % nslots`, `frame = u16[hb + *(hb + rec_e + 0x6d) + 2*fslot]`,
   `atype = hb[rec_e + slot + 0x38]`.
3. Reaction targets: atype 1/2 -> 0x0b/0x0c (observed on ep1), atype 12 -> 0x15, atype 22..32 -> 0x66..0x5b.

Steps, cheapest first:
  a. `fn_0376c8` (the other writer) -- says what switches an entity's container mid-life; that is probably the
     combo/stance switch, and it is one decompile.
  b. `fn_010927`/the resource loader path that built the `0x88`-stride table -- gives the container header
     layout (what lives at +0x00, +0x04, +0x6d, +0x38 per record) in the shipped bytes.
  c. With (b), a **safe** census: walk it in Python over the extracted resources (`scratch/res`, the
     `decode_act.py`/`decode_key.py` parsers are already byte-exact against the C decoders) and print the
     (type, state, slot) -> atype map. No C witness, no crash risk -- the §3 crash came from resolving guest
     pointers with an assumed layout inside the running port.
  d. If the map shows a reachable 22+ for the player, the drive/injection to force it becomes mechanical
     (`play 0x17 <state>` plus the right slot), and the fallen item finally gets red-before / green-after
     against the shipped side.
- Step (a) fact: `fn_0376c8` has exactly **one caller**, `fn_03af18` (the game main) -- so the mid-life container switch happens once per main-loop pass, not from combat code. That makes the constructor-written container (`fn_010efc`) the one that governs combat, and pushes step (b) (the `0x88`-stride type table layout) to the front of the queue.
- Step (b) entry point: DS `0x53ea0` (the `0x88`-stride entity-type table read by the constructor for `+0x25`) is referenced from a family of small accessor functions -- `fn_01f064`, `fn_020170`, `fn_02119c`, `fn_0221b8`, `fn_02319c`, `fn_0265ac`, `fn_02adf5`, `fn_02c838`, `fn_02fe80` -- i.e. the type table is read through per-field accessors, which is the fastest way to recover its field layout (and hence the action-container header) statically.
- Tried `fn_01f064` as a field accessor: it is a large routine (own 0x80-byte frame, floats), not a per-field getter — so field offsets should instead be recovered from the **constructor reads** in `fn_010efc` (`+0x25` = `DAT_00053ea0[type*0x88]`, `+0x29` = `DAT_00053ea4[type*0x44]`, type at `+0x0d`) and the sibling `DAT_00053ea4` accessor set. Recorded so the next pass does not chase `fn_01f064`.
