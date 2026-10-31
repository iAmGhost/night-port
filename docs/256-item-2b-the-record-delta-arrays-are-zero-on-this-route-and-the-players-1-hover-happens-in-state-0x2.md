# 256 — Item 2b: the record delta arrays are all zero on this route, so the player's ±1 hover comes from the walk body, and it happens in state 0x2

`NIGHT_YCLAMP=1` run of the `ep1_kill3_dx1.txt` drive, 300 s (`/tmp/rig/v3y`, rc=0, 19546 witness lines ✓
`stdbuf` interleaving with the pump lines is present ✓ so line-anchored parsing is noisy ✓ only the clean groups are
quoted ✓)

## 1. The action-record delta arrays contribute nothing

Every `[ydelta]` line (fn_01586b's delta provenance witness ✓ `arr[frame]` = the record's `+0x75` Y-delta array read
at the frame index ✓ `passed_d` = what the caller actually passed ✓) shows:

```
kind=0  fr=1..12  arr[frame]=0   passed_d=-1     (and mirrored +1 rows)
kind=1  fr=0..6   arr[frame]=0   passed_d=+-1
```

**`arr[frame] == 0` in every single case** ✓ So on this route the shipped's `[record + 0x75][frame]` delta arrays
are zero for both the player and the enemies, and *all* vertical motion comes from the caller's own step value ✓
docs/243's phrasing ("delta from [record+0x75][frame]") describes a path that is loaded but inert here ✓ recorded as
a correction ✓

## 2. The hover is state-0x2 behaviour; the descent is state-0x1 behaviour

Player (`kind == 0`) `passed_d` grouped by the record's state field `+0x17`:

```
d = +1 :  state 0x2 -> 292      state 0x1 -> 6
d = -1 :  state 0x2 -> 289      state 0x1 -> 25
bounds used: mn=139 mx=195 in all 611 fn_01586b calls on the player ✓ (docs/242's numbers confirmed)
```

`[ydelta]` per (kind, state) totals: `1/0x2: 2538 calls, up 1282 / down 1256, net +26` ✓ `0/0x2: 581 calls, up 292 /
down 289, net +3` ✓ `2/0x1: 131 calls, net -1` ✓ — **state 0x2 alternates ±1 and nets ~0** ✓ while state 0x1 nets
downward ✓✓

Pair that with the state census (docs/254 §4 ✓):

```
shipped player +0x17: 1 x351, 2 x231, 0 x120, 51 x79, 52 x58, 11 x58, 53 x57      (state 1 dominant ✓)
port    player +0x17: 1 x281, 0 x241, 21 x174, 2 x126, 51 x42, 5 x18, 12 x9       (0 and 0x15 much larger ✓)
```

The shipped spends its time in the state that *descends* ✓ (1 ✓ 351 samples ✓ then parks at the clamp's `mn` ✓) ✓
the port spends a quarter of its samples in state 0 and 0x15 (`fn_035c5c`'s state ✓) and only a fifth in state 2 —
where its Y step alternates ✓ That is why the port's player sits at 172 (docs/255 §1 ✓) instead of 139 ✓

## 3. What to fix, stated as a measurable question

The descent mechanism and the clamp are both faithful ✓ (docs/244, docs/254 §1 ✓) and the delta arrays are inert ✓
(§1 ✓) — so item 2b's remaining leg is **why the port's player leaves state 1 (the descending state) earlier than
the shipped's** ✓ `lab_0003284e` is the walk body for states 1/2/0x28 ✓ (`f.c:27255-27279` ✓) and `fn_03272f` owns
the transitions out of it ✓

Specified measurement: a witness at every store to the player's `+0x17` (a `STATE` line: old, new, the modelling
line's shipped-address comment ✓) on one port route ✓ paired with the shipped-side state census from docs/254 §4
(as close as the shipped dump can give ✓ change-triggered ✓) ✓ The candidate transitions are the arms of `fn_03272f`
guarded by `PL32(0x11)` (the per-action step counter ✓ e.g. `PL32(0x11) < 0x32`, `< 0x56`, `< 0x1e`, `< 0x44`,
`9 < PL32(0x11) < 0x3b` ✓) and by `fn_03493e()`'s key value ✓ — an off-by-one in one of those thresholds moves the
port out of state 1 early ✓ which is exactly the shape of the defect the numbers now describe ✓

Until that transition is localized, item 2b's evidence chain stands as: **root cause (local, proven) = the
`fn_01586b` clamp guard `cur < mn || cur > mx` instead of the shipped's `cur > mx`** ✓ (docs/254 ✓ — measurable
effect: player at 139 up 268 → 406 samples ✓ enemy hits 8 → 11 ✓ debt floor 29 → 25 ✓ state-oracle DIFF 35 → 34 ✓
suite ALL PASS ✓); **root cause (open) = the player leaving the descending state too early**, which keeps 31 of the
32 px of vertical separation between the player and the enemies' 141 line ✓ and therefore prevents the 18 landed
hits a type-31 enemy needs ✓
