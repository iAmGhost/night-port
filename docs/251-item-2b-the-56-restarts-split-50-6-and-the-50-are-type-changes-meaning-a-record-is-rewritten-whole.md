# 251 — Item 2b: the 56 restarts split 50 / 6 -- and the 50 are type changes, i.e. a record copy, not `fn_010efc`

Classifying each `+0x23` restart in `/tmp/rig/h4/port` (292.5 s) by what the same slot held in the previous
admissible sample ✓ (no new run, no instrumentation ✓):

```
restart while a kind-1 record of a DIFFERENT type_idx was in that slot:  50
restart while a kind-1 record of the SAME type_idx was in that slot:      6
restart after a non-kind-1 sample at that slot:                           0
```

Reading of each bucket:

* **6 same-type restarts** ✓ line up with the uncapped `NIGHT_SPAWNTRACE` count of **7** `e20f_spawn` calls in the
  same run ✓ i.e. the spawn path accounts for about one construction per restart in this bucket ✓ (and the 7th is
  probably a spawn whose record was not sampled between the two readings ✓);
* **50 type-changing restarts** ✓ cannot be `fn_010efc` constructions through `e20f_spawn` (7 calls total ✓) and
  cannot be the reaction paths (all kind 2 ✓ docs/250 ✓). A `type_idx` change means the record's contents were
  replaced whole ✓ **which is exactly the bulk-copy mechanism docs/250 named as candidate (a)** ✓ and it rules out
  candidate (b) (the slot hand-off artifact ✓ 0 events ✓): the slot was holding a *kind-1 enemy* immediately
  before in every one of the 50 cases ✓;
* so docs/248's "the port replaces enemies ~6x more often than the shipped" stands, and the mechanism is now
  named: **something rewrites a live enemy record wholesale** (type, and with it `+0x23` back to the type's value ✓
  35 for 31, 46-48 for 36 ✓) while the enemy is in play ✓ That is why no enemy ever accumulates the 18 hits its
  body needs ✓ (docs/245 §2 ✓) and why `+0x30 = 1` never appears (docs/246 §5 ✓).

Consequences for the shipped side: the shipped's own dumps show 9 restarts in 407 s ✓ all type 31 ✓ and 149 samples
at `+0x23 <= 2` ✓ -- i.e. the shipped performs this rewrite roughly **6x less often**, or not at all for enemies
in play ✓ (a same-type rewrite would be invisible to the type-change test ✗ open ✓: the shipped's 9 restarts were
all counted as type changes? no -- they were counted as restarts with `>= 10` jumps ✓ and the shipped's type is
always 31 ✓ so the shipped's 9 are same-type restarts ✓ meaning the shipped rewrites 9 records wholesale in 407 s ✓
versus the port's 50 in 293 s ✓✓ ratio ~7.6 ✓)

Next step (specified): find the code that writes a whole entity record in the port -- the census should look for
block copies with a 0x160-sized destination or a field-by-field copy loop whose source is a `.chr` row ✓, in the
spawn/opcode and room-cycle paths (`fn_03e20f` family ✓, the `.act` opcode handlers around `f.c:27000-27150` ✓
which already show `rec[0] = 0` after each use ✓), and compare with the shipped's equivalent (0x3e20f / the opcode
handlers at 0x3eb8f etc. ✓). The shipped's own count of wholesale rewrites -- 9 per 407 s -- is the number the fix
has to land near ✓
