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

## 9. What actually crosses the 500 threshold: a 5.5 s idle streak, and the port's longest is 210 ticks

Streak census on the shipped player record (`state_*.heap.bin`, blocks followed through `g_player_ent`):

| state | streaks | longest streak | mean |
|---|---|---|---|
| `0x01` | 195 | **55 samples = 5.5 s** | 2.4 |
| `0x02` | 74 | 13 samples = 1.3 s | 3.6 |
| `0x28` | **1** | **722 samples = 72.2 s** | 722 |
| `0x33` | 71 | 2 samples = 0.2 s | 1.2 |
| `0x0b` | 15 | 8 samples | 4.0 |
| `0x0c` | 15 | 5 samples | 4.8 |

The two ends of that table are the whole story.  A 55-sample idle streak at the measured 9 frame-counter ticks per
sample is **~495 ticks**, and the idle counter (same code site, same condition on both sides) tops out at **498**
-- so exactly one streak crossed the `== 500` threshold, and the player entered `0x28` **once** and stayed there
for **72.2 s**.  `0x33` appears as 71 streaks averaging 1.2 samples: attacks are short, the stuck state is long.

On the same scale the port's longest accumulation at that site is **210 ticks** (83 in the other run), i.e. the
port's player never idles for the ~5.5 s the shipped player does, so the forced transition never fires and the
72-second `0x28` dwell is never reproduced.  Both numbers are counted at the same guest site under the same
condition (`player[+0x17] == 1` -> `++DS 0x6339c`, else `= 0`), so they are comparable without any timing
alignment.

Also measured here: the shipped player's `+0x30` flag takes only **0 and 2** across 1842 samples -- never `1`.
So `fn_03272f`'s opening gate (`if player[+0x30] == 1` jump to the epilogue, guest `0x3274f-0x3275a`) is not
taken on the shipped side either, and the port's observed `flag=0` matches it.

### Rig trap, fifth occurrence of the "read the right blob" class

While computing these streaks I read `+0x17` out of `state_*.ds.bin` and got `0x2e6e7567` (778990951) with the
block starting `75 6e 2e 61 63 74 00 ...` = `un.act\0gun.act\0...`.  That is not the player record: in the DS
blob, `DS 0x53c7c` is just the four pointer bytes plus whatever else sits in DS.  The record bytes live in
`state_*.heap.bin`, which is the blob where `read_ptrs` **followed** the pointer (`read_at(value, 0x160)`).  The
§1 and §2 numbers came from `heap.bin` and stand; any cross-side record claim must name which blob it read.

## 10. The port's own `g_player_ent` record block is not readable -- so §9's comparison uses tick units

Trying the same streak census on the port side through its dump gives:

```
g_player_ent  (rec_bytes 352, guest_size 4, ptr 2)   1726 samples
top state(+0x17): 0x0 x803, 0x3f x33, 0x3e800 x20, 0x900 x20, 0x80808080 x18, 0x2400 x18
+0x30 values: 100 distinct values 0..255
state 0x01 streaks: 14, longest 2 samples      (shipped: 195, longest 55 samples)
```

`0x80808080` is the poison fill and `0x3e800 / 0x900 / 0x2400` are byte-shifted readings of other fields, so this
is the dump, not the game.  `port/src/app/rig_sink.c:291-318` dumps a pointer cell heap-relative and fills its
record block with `memcpy(p + g_recoff[i], obj, rec_span(obj))`, i.e. whatever `obj` currently is, while the
entity records the oracle can trust are the `ent##` blocks produced by walking the list head `g_dat_53c70`
(`rig_sink.c:319-327`) -- a cell named `g_player_ent` is not the same thing as a walked entity record.  This is
the same open item docs/108 filed ("four pointer-shaped globals differ in every sample -- the port stores *host*
pointers in guest-shaped tables").

Consequence for §9, stated precisely: the port-vs-shipped idle comparison is quoted in **ticks counted at the
same guest site** (`shipped 498` from `DS 0x6339c` in the shipped DS blob; `port 210 / 83` from the in-process
witness at the same statement), not from record dumps on either side.  Record-dump-based streak numbers are
trustworthy on the shipped side (`state_*.heap.bin`, blocks read through `timeline.json`'s `ptr_cells`) and are
**not** trustworthy on the port side for `g_player_ent`.

## 11. The whole chain, with the shipped side's own clock: 5.5 s of idle at t=112 s, then 0x28 forever

Locating the 55-sample streak in time and looking at what happens at its end:

```
sample 1119  t=111.9s  state 0x1   ac=21  hp=925
sample 1120  t=112.0s  state 0x1   ac=30  hp=925
sample 1121  t=112.1s  state 0x1   ac=39  hp=925
sample 1122  t=112.2s  state 0x1   ac=49  hp=925
sample 1123  t=112.3s  state 0x1   ac=58  hp=925     <- 55 consecutive state-1 samples (106.9s..112.3s)
sample 1124  t=112.4s  state 0x28  ac=8   hp=925     <- forced transition, action code reset
sample 1125  t=112.5s  state 0x28  ac=17  hp=925
   ... 0x28 continues to sample 1845 (t=184.5s, the end of the capture): 722 samples = 72.2 s
```

Two things the timestamps make unambiguous.  First, the action code `+0x11` advances **+9 per sample** while the
player idles in state 1 -- the same rate as `DS 0x63360` -- so the idle timer, the action code and the frame
counter are all per-frame quantities, and 55 samples x 9 ticks = 495 ~ the 498 the counter reached: the
`== 500` test fires at the *end* of a 5.5 s idle stretch.  Second, HP is constant (925) across the whole
transition, so nothing combat-related caused it: it is the idle timer alone.  After it fires, `0x28` never
releases for the remaining 72 s of the capture.

The port is faithful in mechanism (same site, same condition, same 1:1 coupling -- its frame counter advanced
exactly 2999 across 3000 visits of the increment site) and diverges only in **dwell length**: its longest
state-1 run at that site is 210 frames (83 in the other run), 2.4x short of the 498 the shipped player
accumulated.  The upstream candidate is the population divergence measured in docs/161 and docs/162 -- the port
carries ~30% more type-1 enemies and 294 samples with >5 entities against shipped's 3 -- so the port's player
is kept busy and never idles long enough for the timer to fire.

That closes the loop opened by docs/166 §14: the sound-dispatch arms live under states `0x15, 0x16, 0x17, 0x18,
0x1e, 0x8d`, and `fn_03272f`'s own dispatcher has an explicit branch for `0x28` (`if (st < 0x29)
lab_0003284e(pl)`).  A state the shipped player occupies for 39% of the run is a state the port never enters,
which is a single upstream cause for the SFX-arm census, the playtest item "player stuck when hit mid-kick", and
part of the reaction-state under-population (`0x0b/0x0c`: port 17/17 and 4/3 vs shipped 60/72).

Not fixed, and deliberately not "fixed" here: the dwell difference is a *consequence* to be traced through the
population numbers, not a value to be tuned at the timer.

## 12. The port's record dumps are systematically unreliable -- the port-side oracle is the witness family

§10 flagged `g_player_ent`; the same check on the walked `ent##` records fails too.  Selecting the block that is
plausibly the player (type byte 0, state < 0x400, 0 < hp <= 1000) from the 12 `ent##` blocks over 2171 samples
(100 ms cadence, `meta.json.period_ms`):

```
plausible player blocks: 286 of 2171 samples
state census in those blocks: 0x0 x251, 0x79 x5, 0x2 x5, 0x7 x5, 0x7f x4, 0x40 x3, 0x1 x3, 0x87 x3
+0x30 values: 35 distinct, including 56, 88, 131, 199, 224, 246
state 0x01 streaks: 3, longest 1 sample          state 0x02: 5 streaks, longest 1 sample
```

Against the same run's in-process witness, which reports sane states (`1`, `0xb`, `0x33`), 498-tick-scale idle
dwell, and `+0x30` values 0/2 only.  The dump and the game disagree, and the disagreement is systematic, not
occasional: `0x79 / 0x7f / 0x87` states and `+0x30 == 246` are not values the port's own code produces.

Standing rule, replacing several earlier habits: **on the port side, record-level claims come from the witness
family** (`NIGHT_STTRACE`, `NIGHT_HITTRACE`, `NIGHT_ACTTRACE`, `NIGHT_DEATHTRACE`, the arena sanitizer), which
read the live record through the same pointers the game uses.  `cells.bin`/`ent##` blocks are usable for
*presence* counts (docs/161/docs/162's entity-population numbers) but not for field-level claims about the
player's state machine -- including some of docs/162's record histograms, which need this caveat attached.

Named fix rather than a workaround: make the dumper prove what it dumped.  `rig_sink.c:319-327` walks
`g_dat_53c70` and copies `rec_span(e)` bytes per link; it should stamp each block with the arena offset it came
from plus the docs/128 magic word (freed blocks marked `0xdeadbeef`), and the oracle should reject any block
whose state/action-code/hp triple is outside the alphabet the witnesses observe -- then the mismatch becomes a
measured count of stale blocks instead of a silent corruption of the comparison.

## 13. With provenance the port side reads clean -- and three divergences become specific

`rig_sink.c` now writes `records.jsonl` next to `cells.bin`: every sample lists the arena offset `g_player_ent`
points at, and for each walked entity slot the offset it was read from, the byte span actually available, and the
`(type, state, action, hp, +0x30)` tuple.  Re-running `ep1_kill3`, 12 000 frames, fixed step + pinned seed,
10 Hz, and identifying the player block by **offset equality** with `g_player_ent`:

```
samples 2000, refused blocks 0
player block identified in 1370 samples;  +0x30 = 0 in all of them;  hp = 1000 in all of them (never changed)
state census: 0x1 x574, 0x2 x306, 0x0 x232, 0x33 x120, 0x34 x69, 0x0c x38, 0x0b x31
streaks:  0x1  173 streaks, longest 44 samples = 4.4 s   (shipped: 195, longest 55 = 5.5 s)
          0x2   62 streaks, longest 19 = 1.9 s           (shipped: 74, longest 13)
          0x33  85 streaks, mean 1.4, longest 2          (shipped: 71, mean 1.2, longest 2)
          0x34  24 streaks, mean 2.9, longest 3          (shipped: 57 samples)
          0x35  NEVER HELD                                (shipped: 58 samples, 1->35 x27 / 35->1 x24)
          0x28  NEVER HELD                                (shipped: 1 streak of 722 samples = 72.2 s)
          0x0b   6 streaks / 0x0c 6 streaks               (shipped: 15 / 15)
entity slots walked per sample: mean 1.87, max 5         (refused 0 -- no dangling link on this run)
```

So the §12 trust problem is gone: `refused 0`, `+0x30` takes the single value 0, `hp` is 1000 throughout, and the
states are all members of the shipped alphabet.  Against shipped the remaining gaps are now specific rather than
atmospheric:

1. **`0x35` is never entered** even though `0x33` and `0x34` are, with the same streak shape as shipped.  A
   missing member of the attack family, not a missing family -- that is a concrete, testable hole.
2. **The longest idle is 4.4 s against 5.5 s**, which is exactly the margin that decides whether the 500-tick
   forced transition fires: the port's counter tops at 210-400 ticks while shipped crossed 500 once, and `0x28`
   (72 s of dwell on the shipped side) is never entered.
3. **`hp` never changes on the port's player in 2000 samples** (1000 throughout) while the shipped player was at
   925 by t=112 s.  Combined with docs/162 §12 (51 of 52 hits change no hp because the debt word absorbs them),
   the port now measures as *no* hp change at all on this route.

`0x0b/0x0c` at 6 streaks each against shipped's 15 remain the reaction-state residue of the same picture.
