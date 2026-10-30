# 239 — Item 2b on a 300 s window: the settle flag does flow, `fn_015295`'s calls now die at the "already awarded" byte, and two `fn_03272f` settle stores turn out to be missing from the port

Fourth look at item 2b (docs/236 -> docs/237 -> docs/238 -> this one).  Two things change the picture, and one tool
defect changes what earlier documents were allowed to say.

## 1. Tool defect first, because it revises earlier numbers

`scripts/audit/field_writers.py` claimed to report *writes* to a displacement, but its guard only rejected
non-store mnemonics at operand 0 ✓ so `mov eax, dword ptr [ecx + 0x30]` -- a **read** of the field -- was counted
as a write ✓ Patched: the memory operand carrying the displacement must be operand 0 (Intel-syntax destination) ✓

Effect on the numbers quoted in docs/187 / docs/189 / docs/236: `--disp 0x30` went from **57 "writers"** to
**24 instructions**, of which 21 touch a record field (3 are `fn_044c23`'s `[esp+0x30]` stack slots ✓ not records)
✓ Every "N writers" figure this tool produced before today is inflated and has to be re-read from the corrected
run before being quoted ✓

## 2. Record `+0x30` (settle) store coverage, corrected

| shipped store | value | port model | status |
|---|---|---|---|
| `fn_010efc` 0x10fe2 | 0 | `f.c:1328` (cited 0x10fdf) | ✓ |
| `fn_0113e7` 0x11651 / 0x11662 / 0x11673 | 1 | `f.c:15383-15387` | ✓ |
| `fn_011827` 0x1191a | 1 | `f.c:14460` | ✓ |
| `fn_013c4d` 0x141ca | 1 | `f.c:24950` (cited 0x141c7-ca) | ✓ |
| `fn_01441e` 0x14913 | 1 | `f.c:25234` (uncited) | ✓ present |
| `fn_01441e` 0x1498d | 2 | `f.c:25254` | ✓ |
| `fn_01441e` 0x1499f | 1 | `f.c:25258` | ✓ |
| `fn_01441e` 0x14c1a | 1 | `f.c:25322` | ✓ |
| `fn_01441e` 0x14cd5 | 1 | `f.c:25352` | ✓ |
| `fn_01441e` 0x14d06 | 1 | `f.c:25357` | ✓ |
| `fn_01441e` 0x1504d | 2 | `f.c:25457` | ✓ |
| **`fn_03272f` 0x32c6e** | **2** | **no store, no citation** | **✗ missing** |
| **`fn_03272f` 0x32d4c** | **2** | **no store, no citation** | **✗ missing** |
| `fn_035c5c` 0x35cc3 | 1 | `f.c:1252` | ✓ |
| `fn_0374ad` 0x374ca / `fn_038f3d` 0x395e0 | 0 | `f.c:5479` / `f.c:25931` | ✓ |
| `fn_03d8d4` 0x3d962, `fn_03dcbd` 0x3de39, `fn_03de85` 0x3df62 | 0 / 0 / eax | not found | open (resource-table routines) |

The two missing ones are both **player-record** conversions, read out of the shipped bytes:

```
0x32c48  eax = [DS 0x53c7c]              ; the player record
0x32c4d  movsx eax, word [eax + 0x23]    ; the debt word (docs/203)
0x32c51  test eax, eax ; jne 0x32c67
0x32c55  al = [player + 0x30] ; cmp 1 ; je 0x32c69
0x32c67  jmp 0x32c72                                   ; otherwise: leave it alone
0x32c69  ... 0x32c6e  [player + 0x30] = 2              ; debt drained && settle == 1  ->  settle = 2
0x32c72  call 0x348e2

0x32d38  al = [record + 0x30] ; cmp 1 ; je 0x32d47
0x32d47  eax = [DS 0x53c7c] ; 0x32d4c  [player + 0x30] = 2        ; settle 1 -> 2
0x32d50  call 0x348e2
```

`fn_03272f` is modelled (`f.c:23482` calls it from the room loop ✓) but these two conversions are not ✓ Note the
direction of the likely consequence, stated as a hypothesis not a conclusion: docs/203 measured the *player*
settle byte as `== 2` in 216 shipped samples vs **357 port** samples, so the port already lands in state 2 more
often than the shipped does -- adding the two conversions has to be measured, not assumed ✓

## 3. The 300 s window changes which gate is the blocker

Same drive (`ep1_kill3.txt`), same pinned environment, but 18 000 frames / 300 s instead of 9 000 / 150 s
(`/tmp/rig/x3`, `--rig-hz 10`, `NIGHT_ACTTRACE=1`):

```
150 s (docs/237 §3):  fn_01441e walk kind = 20 / 6 / 0 ...      +0x30 seen: 26 samples, all 0
                      fn_015295: +0x30!=1 1224 ... reached 0
300 s (this run):     fn_01441e walk kind = 106 / 30 / 0 ...    +0x30 seen: 0 x88, 1 x1, 2 x47
                      fn_015295: +0x30!=1 0   +0x25==ff 231   +5!=0 51   +0x11<=0x15 5   +0x00!=1 1   reached 0
                      fn_035c5c calls=0   fn_01441e sets(+0x30=2)=0   exp award adds=0
```

* the earlier "nothing ever has `+0x30 == 1`" was a **window artifact** ✓ at 300 s the walk does see settle flags
  (1 once, 2 forty-seven times) and `fn_015295` no longer fails at that gate at all ✓ (231 + 51 + 5 + 1 = 288
  calls, each failing exactly one gate ✓);
* the gate that now dominates is **`+0x25 == 0xff` (231 of 288)** ✓ which in the shipped is not an input condition
  but an *output*: `fn_015295` writes `[a0 + 0x25] = 0xff` at 0x1530e **after** passing its gates ✓✓ so a record
  showing 0xff before the gate is one that was already processed once -- or one whose block is stale reused memory
  (docs/191 §1 heap-reuse family ✓);
* `+0x25` store coverage is otherwise complete, so the 0xff values are not being written by an extra port path:
  shipped writers are 0x10fa5 (`fn_010efc`, from the `.chr` row's `+0x04` column -> `DS 0x53ea0` ✓ the port does the
  same at `f.c:1322` ✓ and the loaded rows carry 0 / 1 / 2 ✓ never 0xff ✓), 0x1530e (`fn_015295` ✓ `f.c:8254` ✓),
  0x37881 (`fn_0376c8` player init ✓ `f.c:5630` ✓), 0x39b09 (`fn_038f3d` ✓ `f.c:26029` ✓) ✓
* `fn_035c5c` still never runs ✓ it needs a visited record that has `+0x30 == 1` **and** equals `DS 0x53c80`
  (`g_latched_ent`, set by `fn_010efc` when `type_idx == DS 0x63370` ✓ 0x10f1d-26 ✓ modelled at `f.c:1297-1299` ✓)
  -- and the walk only saw `+0x30 == 1` once in 300 s ✓

## 4. Rig caveat this window exposed, and what it changes about the plan

The same drive, the same pinned environment, run twice more back to back (`/tmp/rig/x3` then `/tmp/rig/x4`, both
18 000 frames / 300 s, `--rig-hz 10`):

```
x3:  walk 106 kind0 / 30 kind1     +0x30 seen: 0 x88, 1 x1, 2 x47   fn_015295 calls 288
x4:  walk  87 kind0 / 28 kind1     +0x30 seen: 0 x59, 1 x0, 2 x56   fn_015295 calls   0
both: fn_035c5c calls = 0   fn_01441e sets(+0x30=2) = 0   exp award adds = committed = 0
both: 18 000 frames presented, ~7.4 k "game main unwound" lines at the frame-budget tail (docs/223 §2: that is the
      harness longjmp, not a fault)
```

So docs/191 §1's variance warning applies to these in-process censuses too, **on the dumped route with the clock
pinned** ✓ the gate distribution (which gate the calls die at) is not reproducible run to run, and a single run's
numbers must not be quoted as a red/green basis ✓ What *is* stable across both runs is the headline that matters
for item 2b: **`fn_035c5c` is never called and no EXP award is ever given** ✓✓

Consequences for the plan:

1. The green measurement for item 2b cannot be an in-process counter from one run ✓ it has to be the dump-side
   census (`scripts/rig/exp_award.py`, per-type `+0x32` values over the record blocks ✓ docs/199 §1's shipped
   `type_idx=31: [0, 10]` vs port `[0]`) **repeated** on both sides, with the run count stated ✓
2. Before any fix is attempted on the settle path, the route needs a *reproducible* kill-bearing basis ✓ the
   canonical drive has no kill-counter movement on either side (docs/197) and its settle census varies (this
   section) ✓
3. `fn_035c5c` requires a visited record that has `+0x30 == 1` **and** equals `DS 0x53c80` (`g_latched_ent`, set by
   `fn_010efc` when `type_idx == DS 0x63370` ✓ 0x10f1d-26 ✓ modelled at `f.c:1297-1299` ✓) ✓ with the walk seeing
   `+0x30 == 1` either once or never in 300 s ✓ the settle pass is effectively unreachable on this route ✓
4. The two missing `fn_03272f` conversions (§2) stay open: they act on the *player's* settle byte, and docs/203's
   census already shows the port in state 2 more often than the shipped (357 vs 216 samples) ✓ so adding them has to
   be measured in the direction of the shipped's numbers, not assumed ✓
