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

## 6. §3 was a frame-budget artifact -- corrected by measuring with a longer budget

§3 concluded from a 4000-frame run that the port's player never enters `0x33-0x35`.  At 12 000 frames the same
drive gives a transition profile that matches the shipped table almost line for line:

```
port run 2 (12000 frames)              shipped (docs/167 §2)
0x1 -> 0x33   x73                      0x01 -> 0x02   x73
0x33 -> 0x1   x73                      0x33 -> 0x01   x70
0x1 -> 0x2    x69                      0x01 -> 0x33   x60
0x2 -> 0x1    x68                      0x02 -> 0x01   x56
0x1 -> 0x34   x29   0x34->0x1 x29      0x01 -> 0x34   x31   0x34->0x1 x31
0x1 -> 0x35   x25   0x35->0x1 x25      0x01 -> 0x35   x27   0x35->0x1 x24
0xb -> 0xc     x4   0xc ->0x1  x3      0x0b -> 0x0c   x15   0x0c->0x01 x12
```

So the attack-state family **is** present in the port.  What §3 saw was a 4000-frame run that only reaches the
first few seconds of play: 4 transitions logged, all `1 -> 0xb -> 0xc -> 1`.  Rule recorded: a port-side
"never happens" claim needs the frame budget stated **and** the phase the budget actually reaches; 4000 frames of
this drive is not 300 s of play (docs/108 already measured that `--frames 300` presented 11 412 frames).

Two divergences survive the longer budget:

* **`0x28` never appears in the port** -- shipped holds the player there for 722/1842 samples (39%).  The only
  writer of `0x28` found is the idle-timer forced transition, and the port's check site never sees `500`: the
  in-process witness at `f.c:26996` reported the largest value reaching the comparison as **83** and **210** on
  two 12 000-frame runs, `val=500` observed **zero** times.  The comparison itself is faithful -- shipped at
  guest `0x32760` is `cmp dword ptr [0x1339c], 0x1f4` / `jne`, an equality, exactly as rendered.
* **Reaction states are under-populated in the port**: `0x0b/0x0c` appear 17/17 and 4/3 times per run against
  shipped's 60/72.

The shipped timer's shape, read out of the shipped DS dump: `DS 0x63360` (frame counter) advances **+9 per
100 ms sample** (90/s) and `DS 0x6339c` advances **+1 per frame-counter tick** whenever `player[+0x17] == 1`
(ratio measured 1.000), climbing 406, 415, 424, 433, 442, 452, 461, 470, 479, 489, 498 -- it crosses 500
*between* samples.  The shipped block at guest `0x3ab68-0x3ab81` is reproduced faithfully by the port
(`inc [0x63360]`; `if (player[+0x17] == 1) ++timer; else timer = 0`), so the divergence is behavioural: the
port's player does not sit in state 1 long enough -- 210 ticks max against a threshold of 500.

## 7. Port-side non-determinism, reproduced and quantified (docs/108's open residual)

Two runs of the *same binary, same drive, same --frames* differ:

| configuration | `fn_03272f` entries | idle-timer max at the check | player transitions |
|---|---|---|---|
| no pins, 4000 frames | 844 vs 816 | 35 vs 13 | 1 vs 1 |
| `NIGHT_SEED=13579`, 4000 frames | 905 vs 244 | 101 vs 71 | 4 vs 1 |
| `NIGHT_SEED=13579` + `NIGHT_RIG_STEP_CALLS=490` + `NIGHT_RIG_FPS=60`, 12 000 frames | 1200 (cap) vs 1200 (cap) | 83 vs 210 | 328 vs 404 |

Cause of the first row is `main.c:1379` -- `if (now < g_tick_next && !g_fixed_step_calls) return;` -- the tick
pump converts wall time into PIT calls and re-anchors when the host falls behind.  Pinning the step *and* the
seed narrows it but does not remove it, confirming docs/108's "STILL OPEN" line with a crisp symptom: the same
script produces 328 or 404 player state changes.  Consequence for the rig: any port-side count is quoted with a
run count, never from a single run.

## 8. Two rig defects filed while doing it

* **The witness-trap from docs/166 §9 happened a third time.**  `cntchk` printed nothing for a whole measurement
  cycle and I nearly reported "`fn_03272f` never reaches the 500 check" -- `strings port/test/night | grep -c
  cntchk` = 0 proved the block was empty because three trailing comment lines opened a comment on the `if`
  line and closed it on the `fprintf` line.  After rewriting the comments above the statement, `strings` = 1 and
  the check was logged 844 times in that same run.
* **Cell-owner contamination for `DS 0x63360` / `DS 0x6339c` in the port dump.**  Reading those two cells out of
  `cells.bin` gives a frame counter whose median per-sample delta is **0** and whose max is **4 294 967 295**
  (a wrap), and `0x6339c` values like 1 056 964 608 -- while the in-process witness sees a sane counter topping
  at 210.  So the rate comparison between the two sides through these cells is unusable until the cell owner is
  resolved (docs/112 / docs/121's contested-cell family); the in-process numbers stand, the DS-cell ones do not.
