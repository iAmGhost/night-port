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
