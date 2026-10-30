# 240 — Item 2b, one measurement further: `fn_01441e`'s latch test is reached once per 300 s run, and the latch cell is NULL when it is

Small follow-up to docs/239, from one new census counter (`g_1441e_latch_hits[3]`, `f.c` at the 0x000150c8 test,
printed by the `NIGHT_ACTTRACE` block as "fn_01441e 0x150c8 latch census").

Canonical drive `ep1_kill3.txt`, pinned clock, 18 000 frames / 300 s, `--rig-hz 10` (`/tmp/rig/x5`):

```
exp award gates : +0x30!=1 952  +0x25==ff 231  +5!=0 51  +0x11<=0x15 5  +0x00!=1 1  reached 0
walk census     : kind 0..7 = 70 19 0 0 0 0 0 0 ;  +0x30 seen: 0 x69, 1 x1, 2 x19
latch census    : latch NULL 1   live-but-not-this 0   identity match 0
fn_035c5c calls : 0        exp award adds/commits : 0
```

## 1. What this says

* the 0x150c8 test (`cur == DS 0x53c80`, the gate that would call `fn_035c5c`) is **entered once in 300 s** ✓ because
  it sits inside the `cur[+0x30] == 1` block, and the walk sees `+0x30 == 1` exactly once ✓;
* at that single visit **`g_latched_ent` was NULL** ✓ so the identity test could not have matched anyway ✓;
* therefore `fn_035c5c` (the per-type settle pass) is effectively unreachable on this route in the port ✓ and the
  in-process award counters stay 0 ✓

## 2. Which makes the *other* award producer the one that matters

The shipped has two per-type award writers (docs/237 §1, docs/199 §1): `fn_035c5c` 0x35cf0/0x35cf6 **and**
`fn_01441e` 0x15067/0x15070 ✓ The second one needs only

```
cur[+0x30] == 1  (0x15003)   AND   cur != DS 0x53c7c (the player)  (0x15013)
AND  ( cur[+0x5a] != 0  (0x1502b)   OR   word[cur + 0x29] == 0  (0x15036) )
```

-- **no latch involvement** ✓ That is the branch whose shipped output was actually measured (31 shipped dying
samples with `+0x32 = 10`, `type_idx = 31` ✓ docs/199 §1) ✓ So item 2b does not hinge on the latch cell at all: it
hinges on **records reaching the settle block with `+0x30 == 1`** ✓ which happens 0-1 times per 300 s in the port and
often enough in the shipped to leave 31 measurable samples ✓

## 3. Next measurement (not yet run)

Count the stamps themselves, so the rate is attributed to a writer instead of inferred:

| site | shipped | what to count |
|---|---|---|
| `fn_013c4d` 0x141ca (`+0x30 = 1` on the victim at hit resolution ✓ modelled `f.c:24950`) | 1 | how many GATEOK-resolved contacts actually stamp |
| `fn_0113e7` 0x11651/62/73 ✓ `f.c:15383-15387` | 3 | how often the three-way reaction spawn runs |
| `fn_011827` 0x1191a ✓ `f.c:14460` | 1 | how often the step dispatch spawns a stamped reaction record |
| `fn_01441e` 0x14913 / 0x1499f / 0x14c1a / 0x14cd5 / 0x14d06 ✓ `f.c:25234-25357` | 5 | how often the settle walk itself stamps 1 |

Compare those counters against docs/195's `GATEOK` census (33 on the 150 s route, 81 on the longer hit-trace run) ✓
If the stamps are ~0 while GATEOK is tens, the stamping path -- not the gates -- is the item-2b defect ✓
