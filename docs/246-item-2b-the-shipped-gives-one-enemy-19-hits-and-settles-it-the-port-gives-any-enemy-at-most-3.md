# 246 — Item 2b, red basis at last: the shipped gives one enemy 19 hits and settles it; the port gives any enemy at most 3

`scripts/rig/hit_rate.py` after fixing the bug that made its shipped side read nothing (see §1). Same route,
`ep1_kill3.txt`, plain replay dumps, no `--rig-dump` inflation (docs/234 §3).

## 1. Two tool corrections first

1. `hit_rate.py`'s record walker branched on `side == "orig"` while the caller passes `"shipped"` ✓ -> the shipped
   side silently took the port branch, found no `host_size` cells, and reported `enemy-record-samples = 0` ✓ Fixed ✓
   (this was the whole of the docs/245 §5 discrepancy with `exp_award.py` ✓ the walkers agree now ✓)
2. The gate needed the shipped's own mask on the state field (`and 0x7fffffff` ✓ 0x00014749 ✓): enemy records carry
   the high bit set on `[+0x17]` from time to time, exactly why the shipped masks it before using it as a table
   index ✓ docs/245 §5's "the gate is too strict" note is resolved by the mask, not by loosening the bounds ✓

What the shipped dump actually contains (974 samples, 63 pointer cells, `rec_bytes = 352` each): cell names include
`ent00..ent11` ✓ `g_player_ent` ✓ `g_latched_ent` ✓ and 48 other DS cells whose blocks are *not* entity records --
that is where the kind bytes 56 / 34 / 7 / 192 of docs/199 and docs/242 §1 come from ✓ Among the 7 883 blocks with
`kind == 1`, only **1 065** carry a sane `type_idx` (31 ✓ 1 065 times ✓) and the rest are pointer-valued
(2 576 054 147 ✓ 33 554 432 ✓ 2 155 905 152 ✓) ✓ Those 1 065 are the quotable enemy samples ✓

## 2. The measurement

Per record **slot** (position in the dump's cell list, never value-matched ✓ docs/186 §1): `word[record + 0x23]`
(the pending-HP term, decremented only at 0x1495e ✓ docs/245 §1) -- hit counts and the lowest value reached:

```
shipped: slots seen = 7
         hits per slot:      slot 52 -> 19   slot 53 -> 15   slot 10 -> 6      (others 0)
         debt range per slot: 52: 0..35   53: 2..35   10: 0..35
         global minimum debt reached: 0
         +0x30 == 1 observed in 29 samples;  +0x30 -> 2 transitions: 0

port:    slots seen = 9
         hits per slot:      slot 4 -> 21  slot 3 -> 17  slot 5 -> 17  slot 2 -> 11
                             slot 1 -> 6   slot 0 -> 5   (two more with 1)
         debt range per slot: 0..5 all 29..50   (29 = the floor reached)
         global minimum debt reached: 29
         +0x30 == 1 observed in 0 samples;  +0x30 -> 2 transitions: 0
```

Read carefully, because two things in that block are not what they look like:

* the port's per-slot hit counts (21, 17, 17, 11 ✓) are **larger** than the shipped's (19, 15, 6 ✓) -- but its debt
  never falls below **29** ✓ and 35 - 29 = 6 = **three landed hits at dmg 2** ✓: the port hits an enemy three times
  and then the slot's debt is back at 35/46 ✓ (the 46-50 values are a *different* enemy type -- 36 -- reusing the
  same dump slot ✓ docs/245 §5's slot-reuse caveat, visible here rather than hidden ✓). The shipped's 19-hit slot
  went 35 -> 22 -> 17 -> 15 -> 2 -> 0 ✓ one enemy, all the way down ✓
* so the defect is not "the port never hits" and not "the damage is too small per hit" (both sides deal 2 ✓) -- it is
  that **the port's enemies do not stay in play long enough to accumulate 18 hits** ✓ The shipped's enemy slot 52
  absorbed 19 hits; the port's best enemy absorbed 3 before its slot carried a new enemy ✓

## 3. Why the settle flag never appears, in one line

`+0x30 = 1` needs either HP <= 0 with debt 0 (0x148f2-0x14913) or the debt drain at 0x14968-0x1499f with
`+0x29 + debt <= 0` ✓ The port satisfies neither because its victims' `+0x23` bottoms out at 29 (HP `+0x29` is 0 ✓
and the pool `+0x56` is 0 ✓ docs/245 §2 ✓) ✓ The shipped's victims reach `+0x23 = 0` ✓ -> 0x14968 falls through ->
`+0x30 = 2` -> `+0x29 (0) + 0 <= 0` -> **`+0x30 = 1`** ✓ (29 samples ✓) ✓

## 4. This is the red basis, stated as a gate

For item 2b the before/after pair is now quotable from dumps alone, repeated per side (docs/239 §4):

| measure | shipped (`/tmp/rig/ents-orig/orig`) | port (`/tmp/rig/h3/port`) |
|---|---|---|
| debt decrements observed | 40 (all `type_idx = 31`, all <= 16 px in magnitude ✓) | 79 (73 type 31, 6 type 36; 21 > 16 = slot-reuse suspects ✓) |
| lowest debt any enemy reached | **0** | **29** |
| hits absorbed by one enemy | **19** | **3** (net of slot reuse) |
| records with `+0x30 == 1` | **29** | **0** |
| award field `+0x32` on those records | **10** (docs/199 §1, `type_idx = 31`) | never observed |

Next turn, before any code change: (a) repeat both sides twice to make the pair repeatable ✓ (b) find *why a port
enemy leaves play after ~3 hits* -- the candidates are the enemy lifetime/room-transfer path (`fn_03a829`'s room
cycle ✓ docs/223 §3's return values ✓, `fn_03cc6b`'s room chain ✓, and the spawn/reuse path `fn_010efc` clearing
`+0x30` on reuse ✓ docs/239 §1 ✓) rather than the award code ✓
