# 248 — Self-correction: "the shipped's enemy slots never change identity" measured the wrong thing

docs/247 §1 reported `slot identity changes: none` for the shipped and 101 for the port. The identity key used was
`(kind, type_idx)` ✓ which cannot see **a same-type enemy replacing a same-type enemy** ✓ -- and on the shipped's
route every enemy is `type_idx = 31` ✓ so the shipped's 0 is not "no replacement", it is "no *type* change" ✓
The port's 101 is likewise a type-change count (31 <-> 36 ✓) and not a replacement count ✓

Re-running the census with run lengths (how long one slot identity persists) makes the contamination visible:

```
shipped (4 Hz, 406.5 s):  type 31: runs = 49   median 3 samples   max 315 samples
port    (10 Hz, 292.5 s): type 31: runs = 142  median 7 samples   max 521 samples
                          type 36: runs = 108  median 3 samples   max 108 samples
```

A median of 3 samples on the shipped side is not a 3-sample enemy lifetime ✓ it is the admissibility gate
(`kind == 1`, `state & 0x7fffffff < 0x100`, `type_idx < 0x1000`) rejecting a sample of the *same* record -- the
same high-bit state values that docs/246 §1 needed the 0x7fffffff mask for ✓ produce gaps ✓ so "runs" count gate
gaps as well as replacements ✓ **Neither number may be read as a lifetime** ✓

What survives from docs/247 unchanged, because it does not depend on run lengths:

* the port drives every enemy's `+0x23` to a floor of 21-29 and writes `+0x30 = 1` on a kind-1 record **zero**
  times in two runs (docs/246 §5 ✓);
* the shipped reaches `+0x23 = 0 / 2 / 7` in three runs and shows 31 samples with `+0x30 == 1` in the longest one ✓;
* spawn-time placement matches (`ry = 193 / 150` for type 31 ✓ docs/247 §2 ✓);
* no room re-entry inside the measured window (docs/247 §4 ✓).

A clean lifetime measurement needs an identity that the gate cannot break ✓ the next attempt should use the
port's own record identity (its arena offset, stable in `meta['cells']` ✓) plus a spawn counter witness at the
builder (`fn_010efc` ✓), and on the shipped side the *sequence of `+0x23` values* per slot rather than a type key
(a strictly decreasing chain that restarts at 35 is one enemy dying/replaced ✓ which is gate-independent ✓) ✓

## The gate-light replacement census (the number to quote instead)

Per record slot, follow `word[record + 0x23]` (the pending-HP term) across samples and count a *restart* whenever
it jumps back up by >= 10 (a chain that was being eaten alive is suddenly full again = a new body in that slot ✓),
plus how many samples show the term at <= 2 (an enemy at the edge of the settle threshold ✓):

```
shipped (/tmp/rig/ents-orig/orig, 406.5 s, 4 Hz):
   restarts:  type 31 -> 9          samples with +0x23 <= 2: 149

port (/tmp/rig/h4/port, 292.5 s, 10 Hz):
   restarts:  type 36 -> 50, type 31 -> 6   (56 total)     samples with +0x23 <= 2: none
```

So, per unit time: the port replaces enemies **~6x more often** than the shipped (56 in 293 s vs 9 in 407 s ✓
0.19/s vs 0.022/s ✓) and **never** brings an enemy within 19 points of its settle threshold ✓, while the shipped
holds enemies at `+0x23 <= 2` for 149 samples ✓ enough to cross 0 and write `+0x30 = 1` (31 samples ✓ docs/246 §5 ✓) ✓
This census does not depend on run lengths, only on the value sequence per slot ✓ so it survives the gate-contamination
problem above ✓
