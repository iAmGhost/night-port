> **RETRACTED by docs/291.** §1's per-state facing shares are a cadence artifact (port dumps at 10 Hz,
> > the shipped replay at ~4 Hz, and the player's `+0x1b` oscillates with a ~0.8 s period — 23 flip events over
> > 1086 samples in the port's own dump ✓). §2's "five handlers the shipped never touches" is wrong: the scan
> > used `entry + body_bytes`, which stops short of `body_end`, and the shipped's flip tails sit in exactly that
> > gap (`fn_017f13` 0x18e51-0x18e78, `fn_018e88` 0x19e11-0x19e38 ✓). Kept only as the record of how the
> > measurement was made and why it failed; use docs/288 §3 (bound state identical on both sides ✓) and docs/291 ✓.

# 290 — Item 6 gets its first rig-visible red signal: the port flips the mirror/facing flag (`record+0x1b`) during the attack states far more often than the shipped does

## 1. The measurement (`/tmp/rig/le4`, drive `ep1_leftedge2.txt`, both sides, 220 s)

Player record per state, cross-tabulated with the mirror byte `+0x1b` (the value that selects which arm of the
limb/box draw runs — shipped `0x13db7 cmp [eax+0x1b],0` ✓ port `f.c:25052` ✓) and with the room-orientation cell:

```
both sides: DAT_00063398 (room orientation) = 0 in 712/712 shipped and 1086/1086 port samples ✓
            both park at X = -30, Y = 172 at the left bound (docs/288 §3 ✓)

state   SHIPPED f1b            PORT f1b
  0     0:32      1:0     (0%) 0:233     1:0     (0%)   match ✓
  1     0:232     1:147   (39%) 0:63     1:318   (83%)  DIVERGENT ✗✗
  2     0:6       1:65    (91%) 0:13     1:263   (95%)  close ✓
 11     0:83      1:6     ( 7%) 0:25     1:12    (32%)  DIVERGENT ✗✗
 12     0:100     1:7     ( 7%) 0:76     1:21    (22%)  DIVERGENT ✗✗
```

States 11/12 are the attack states ✓ — so **while attacking, the shipped holds the non-mirrored arm in 93 % of its
samples, the port in 78 %/68 %** ✓ i.e. the port spends a quarter to a third of its attack samples with the mirror
flag flipped ✓ and this drive holds `left` with the player against the left bound ✓✓ That is the shape item 6
describes ("attacking at the far left edge draws the leg/arm on the right side") ✓ It is a dump-side, per-state
conditioned count ✓ — no screen basis, no pacing, no colour basis involved ✓ (docs/288 §4's inconclusive cluster
census is superseded as the *only* evidence ✓, not as a caveat ✓)

## 2. Where the flag is written, both sides

`scripts/audit/field_writers.py --disp 0x1b`: **58 shipped writes, 0 indexed** ✓, in
`fn_010efc (0x10f81)`, `fn_0113e7 (0x11511/1151b/11525, 0x11633/1163d/11647)`, `fn_011827 (0x118ac, 0x11910)`,
`fn_013c4d (0x1420b, 0x14219, 0x1427e, 0x142e4, 0x1431a, 0x1433f)`, `fn_01441e (0x14f35)`, `fn_018e88
(0x19e21/0x19e38)`, `fn_01aec8 (0x1be81/0x1be98)` … ✓
The port writes `record+0x1b` at ~40 sites ✓, of which twenty are the *same proximity rule repeated across the
per-type handler family* (`f.c:16661/17013/17358/17704/18056/18423/18775/19130/19469/19797/20138/20555/20989/
21519/22025/22511/23015/23515/24209/24728`):

```c
if ((int32_t)fn_01519f(e) > p_x) ((struct sa32 *)((e + 0x1b)))->v = 1;
if ((int32_t)fn_01519f(e) < p_x) ((struct sa32 *)((e + 0x1b)))->v = 0;
```

— a "face toward/away from the player" flip driven by relative X ✓ If the shipped gates that flip (by state, by
kind, or only in one member of the family), the port would flip it every tick for every record in the room ✓✓
which is exactly the per-state over-flipping §1 measures ✓

## 3. Next step (the localization is one comparison away)

Match the shipped's 58 writers against the port's ~40 sites one instruction at a time (the census output above is
truncated at 24 rows — read the remaining 34 ✓), and for each shipped write record the *guarding* branch
(`scripts/audit/branch_xrefs.py ✓ back-edges ✓`) — specifically whether the proximity flip in the per-type family
is guarded by a state test (`[rec+0x17]`) or a kind test (`[rec]==1`) in the shipped and unguarded in the port ✓
The green measurement is then the same §1 table re-run: the attack-state mirror fraction must fall from 32 %/22 %
to the shipped's 7 %/6 % on the same drive ✓

Item 6 stays open until that fix-and-re-measure pair exists ✓ No fix is claimed ✓
