# 252 — Two retractions: `ent##` dump cells are pointer-following cells (not record identity), and `inc word [eax+0x23]` at 0x138f6 is a real per-frame +1

## 1. The construction census (new witness `NIGHT_RECBUILD=1`, `fn_010efc` entry)

`fn_010efc` 0x10f53-59 is the only shipped store that can put a new `type_idx` into a record (docs/249 ✓), so
counting constructions decides what the dump-side "+0x23 restarts" mean ✓ Canonical run
`OUT=/tmp/rig/rb1 PORT_ENV="SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
NIGHT_RECBUILD=1" route.sh drives/ep1_kill3.txt 150` → 66 constructions:

| kind | type | count | caller |
|---|---|---|---|
| 2 | 201 | 20 | one caller (`fn_013c4d`'s effect actor ✓ docs/241/242 ✓) |
| 2 | 250 (0xfa) | 40 | four callers = `fn_0113e7` / `fn_011827` reaction spawns ✓ docs/250 ✓ |
| **1** | **31** | **3** | one caller (`e20f_spawn` ✓) |
| **1** | **36** | **2** | same caller |
| 0 | 0 | 1 | player spawn ✓ |

**The port builds 5 enemy records in 150 s** ✓ (the uncapped `NIGHT_SPAWNTRACE` said 7 in 300 s ✓ consistent ✓)

## 2. Retraction: docs/248 §"replacement census" and docs/251 §1's 50/6 split are void

97 `+0x23` restarts appear in `/tmp/rig/rb1/port` when keyed by dump cell name -- **20x the construction count** ✓
so they cannot be constructions ✓ The reason is the identity itself: the `ent00..ent11` cells are *DS 0x53c70
pointer-array slots* (docs/186 §1: twelve `ent##` slots share DS 0x53c70 ✓) -- each dump sample follows the current
pointer ✓ so an `ent##` cell is **"whatever record the array points at now"**, not a record ✓ A pointer-array
re-point (which needs no construction at all ✓) moves the cell to a different live record and its `+0x23` jumps ✓

Consequences that have to be re-verified, not assumed:

* docs/246 §2's "the shipped gives one enemy 19 hits, the port at most 3" -- that per-slot hit census used the same
  pointer-following cells on **both** sides ✓ so it may be a per-*pointer-slot* census, not a per-*enemy* census ✓
* docs/248's "~6x replacement rate" ✓ and docs/251's "50 wholesale rewrites" ✓ are withdrawn ✓ (docs/251 §1's
  classification cannot distinguish a re-point from a rewrite because both look identical through a pointer cell ✓)

What survives: `+0x30 == 1` on a kind-1 record never appears in the port dumps (0 samples, several runs ✓) and does
appear in the shipped's longest capture (31 samples ✓); the award path itself is modelled faithfully (fn_015295's
gates, its two `fn_0003b5c0` queue posts at 0x153a7/0x15432 ✓ and its `player[+0x52] += cur[+0x32]` at 0x15453 ✓ all
present in the port at `f.c:8248-8283` ✓ including the queue post ✓ so docs/249/250's "fn_03b5c0 is missing from the
port" -- which I formed mid-investigation and never committed -- is wrong: the port names it `fn_0003b5c0`
(`f.c:1131-1160` ✓) and calls it at `f.c:8277` ✓ my grep pattern `0x3b5c0` simply did not match `0x0003b5c0` ✓).

## 3. Retraction: `inc word [eax + 0x23]` (0x138f6) is not a draw cursor

docs/249 annotated 0x138f6 as "the HUD number field's draw cursor ✓ not an entity record" ✓ It is in fact modelled
at `f.c:14132-14135` inside `fn_0137c1` as:

```c
if (entity[+0x56] != 0) {              /* 0x0138ea-fd, per frame */
    (int16)entity[+0x23] += 1;        /* 0x138f6 */
    entity[+0x56] -= 1;
}
```

i.e. a **per-frame +1 to the pending-HP term whenever the reserve pool is non-zero** ✓ a genuine raiser of `+0x23` ✓
docs/249's claim "nothing in the shipped can raise +0x23 on an existing record" is therefore **wrong** ✓ the
shipped raises it once per frame for any entity `fn_0137c1` is called on that has `+0x56 != 0` ✓

However the call census bounds the damage: `imm_scan --call 0x137c1` finds exactly three shipped callers
(0x38073, 0x38134, 0x38306 ✓ all inside fn_03801b / fn_038098 / fn_0381d7 ✓) and the port models all three
(`f.c:15303`, `f.c:15521`, `f.c:15598` ✓) **each passing `g_dat_53c84` = the player** ✓ So this regen applies to the
player only, on both sides ✓ it is not an enemy self-heal ✓ and it is not item 2b's mechanism ✓ but it *is* why a
player's `+0x23` can rise ✓ (docs/242's "shipped player freezes from P167 onward" ✓ worth re-reading with this in
mind ✓)

## 4. Corrected next measurement (identity by construction, not by pointer slot)

The five constructed enemy records have their arena offsets printed by the witness (`off=` ✓) ✓ Re-run the `+0x23`
census **restricted to records identified at construction** -- host-side map `arena offset -> construction id` fed
by the `RECBUILD` lines ✓ and take hits per construction id ✓ For the shipped side an equivalent identity is
needed before any per-enemy comparison is quotable ✓ (the shipped dump's cells are the same pointer-following kind ✓
docs/246 §2's shipped numbers carry the same defect ✓): the shipped-side alternative is to follow a *value chain*
inside one snapshot stream -- a monotone `+0x23` sequence with no re-point -- or to use the shipped's own entity
list walk (DS 0x639cc/0x639d0 list and the DS record array at 0x53c70 ✓) as the identity source ✓

Until then, item 2b's quotable red basis is: **the port constructs 5 enemy records in 150 s and never settles any
kind-1 record (`+0x30 == 1`: 0 samples), while the shipped settles kind-1 records (31 samples in its 407 s
capture)** ✓ the "how many hits per enemy" layer is withdrawn pending a stable identity on both sides ✓
