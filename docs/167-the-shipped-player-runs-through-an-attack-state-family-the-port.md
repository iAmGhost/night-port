# 167. The shipped player runs through an attack-state family the port never enters

The audio cluster (docs/166 §14) ended at a wall: the port's sound-dispatch arms all live under entity states
`0x15, 0x16, 0x17, 0x18, 0x1e, 0x8d`, and on `ep1_kill3` the port's dispatcher sees states 0, 1, 2 only.  This
document measures the other side of that wall instead of guessing at it.

## 1. Ground truth from the shipped run, read through the shipped player pointer

The shipped code identifies the player by comparing a record pointer with `DS 0x53c7c` (guest `0x12647`:
`cmp eax, dword ptr [0x3c7c]`), and the rig's dump already carries that cell as `g_player_ent` (cell 12 of the
64 pointer blocks in `g15/orig/timeline.json`).  Reading `state` at record `+0x17` for that block across all
1846 samples (300 s, 10 Hz):

```
state 0x28  x722     39% of the run          state 0x33 x85    state 0x34 x57    state 0x35 x58
state 0x01  x462                             state 0x36  x3
state 0x02  x268                             state 0x0c x72    state 0x0b x60
state 0x00  x55
```

Type byte of that record is `0` in 1842/1842 samples: the pointer really is the player.

## 2. Transitions: the 0x33-0x35 family is the bulk of the player's behaviour

432 state changes in 1842 samples.  Top pairs:

```
0x01 -> 0x02   x73      0x33 -> 0x01   x70      0x01 -> 0x33   x60
0x02 -> 0x01   x56      0x01 -> 0x34   x31      0x34 -> 0x01   x31
0x01 -> 0x35   x27      0x02 -> 0x33   x11      0x0b -> 0x0c   x15
0x0c -> 0x01   x12      0x02 -> 0x0b   x7
```

Roughly **219 of the 432 changes are into or out of 0x33/0x34/0x35**, always returning to 0x01 -- an
attack-state family that a kill drive should be exercising, and the same alphabet the playtest list complains
about (missing punch SFX, infinite punching, player stuck when hit mid-kick).  The reaction pair `0x0b -> 0x0c`
appears too (15 times), which is the state family docs/108 and docs/111 already fixed.

Action codes (`+0x11`) while the player sits in 0x33/0x34/0x35 are small and repeated (`4, 5, 10`, `10, 0, 9`,
`17, 7, 0`) -- the shape of an animation/attack sequence rather than a random value.

## 3. The port's player never enters that family

In-process census of the same drive (`NIGHT_AUDIODUMP` entry census at `fn_01194c`, the dispatcher whose 29
`SND` arms are `fn_0113e7(e)` calls), 4000 frames:

```
2500 dispatch entries:  state 0x01 x925, 0x02 x575, 0x00 x2      entity+0x30 == 0 in all 1502 sampled prints
fn_0113e7 entries:      0
```

So the port's player is confined to `{0, 1, 2}` while the shipped player spends 39% of its samples in `0x28` and
makes 219 transitions through `0x33-0x35`.  The missing sound triggers, the missing hurt-stagger residue and the
SFX census (`61 of 62 requests from `fn_03f6bc+0x208`, the timed-event path`) are downstream of the same fact.

## 4. What the state field is written by, and what the port has

Enumerating every byte-pattern form that can store to `[entity+0x17]` (docs/166 §15): only
`mov dword ptr [eax+0x17], edx` occurs (32 pattern hits, 14 decoded instructions on instruction boundaries),
located in `fn_03272f` (4 sites, the `.act` chain matcher), the per-type controller family
`fn_02319c, fn_024bf9, fn_0265ac, fn_027bec, fn_029709, fn_02adf5, fn_02c838, fn_02e2cc, fn_02fe1e, fn_031f5f`,
and `fn_01441e` (the hit code).  The port has state-write sites in **all 14** of them, and `fn_03272f`'s own
dispatcher has explicit branches for `0x15, 0x16, 0x1e, 0x28, 0x47, 0x98, 0x99-0x9b` -- the alphabet is modelled.
Its single state *store* is `pair[+0x17] = 1; pair[+0x11] = 0`, a reset to state 1.

That is the sharpest open question in the gameplay cluster: the shipped chain matcher's reset and the
controller-family stores produce `0x28`/`0x33-0x35` in the shipped player, and in the port the same code paths
leave the player at 1 or 2.

## 5. Next measurement, named

A port-side witness that logs `player[+0x17]` transitions (from -> to -> action code +0x11) at the state-store
sites, run on the same drive, compared against §2's transition table.  The comparison is a set difference, not a
frame alignment, so it is immune to the timing problems docs/158/docs/165 had to solve.  Whatever `from -> to`
pair the port cannot produce is the specific defect, and `fn_03272f`'s reset site is the first suspect because
it is the only state store in the chain matcher.
