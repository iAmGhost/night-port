# 253 — Item 2b with a stable identity: the port's enemies absorb 1-2 hits, the shipped's absorb ~16, and the reason sits in the player's vertical hold

Two independent, repeatable measurements on the canonical drive (`drives/ep1_kill3.txt`,
`PORT_ENV="SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60"`), both free of the
pointer-cell defect docs/252 exposed ✓

## 1. Identity-linked entity census (`scripts/rig/ent_life.py`, docs/115 nearest-predecessor linking, lifetimes in ms)

`ent_life.py --orig /tmp/rig/ents-orig/orig --port /tmp/rig/rb1/port` (anchor `DAT_00063e14 88→67 ✓ shipped
t=72696 ms ✓ port shift −78237 ms ✓):

| | shipped (407 s) | port (151 s) |
|---|---|---|
| kind-1 (enemy) identities | **5** | **6** |
| kind-1 lifetime ms | 0 / 2613 / 110904 (median) | 2200 / 44100 / 65700 |
| kind-1 `+0x23` debt values seen | **35, 29, 24, 22, 17, 2** ✓ 28 transitions | **33, 35, 48, 50 only** ✓ 61 transitions, **no downward chain** |
| kind-1 how they leave | hp0 ×3, flag30 ×2 ✓ | hp0 ×6 (vacuous: every kind-1 record has `+0x29 == 0` on both sides ✓) |
| settle flag over all live samples | (2, 739), (0, 233) ✓ | (0, 876), (1, 23) ✓ -- the 23 are kind-2 records ✓ docs/241 ✓ |

Reading: the shipped's enemy debt term walks **35 → 2** (33 points removed ✓ at the 2-per-hit damage the docs/245
model gives ✓ ≈ 16 hits per enemy ✓) ✓ the port's enemy debt term **never leaves its spawn value** ✓

## 2. Damage-victim census (`NIGHT_HITTRACE=1` + `NIGHT_RECBUILD=1`, `/tmp/rig/rb2`, 150 s)

57 damage applications ✓ 69 record constructions ✓:

```
48  victim kind=0 (player)   attacker kind=1 type=31
 1  victim kind=0 (player)   attacker kind=1 type=36
 8  victim kind=1 type=31    attacker kind=0 (player)
constructions: 6 kind-1 (type 31 x3, type 36 x2, +1), 20 kind-2 type 201, 40 kind-2 type 250, 1 kind-0
```

So **49 of 57 landed hits hit the player** and only **8 hit an enemy** ✓ spread over 6 constructed enemies ✓ = **1-2
hits per enemy** vs the shipped's ~16 ✓ Each enemy needs debt 35 / 2 per hit = 18 hits ✓ (docs/245 §2 ✓) ✓ so
nothing reaches 0 ✓ no settle ✓ no queue post ✓ no award ✓ ✓ item 2b's EXP stays 0 ✓✓

## 3. Where the misses come from (cadence caveat first)

Shipped player `+0x09` run lengths: `172 ×46, 166, 162×3, 167×18, 157×12, 154, 147×12, 144, 139 ×878` ✓ holds the
room's lower bound in **878 of 972 samples (90 %)** ✓
Port: `172 ×335, 166, 169, 179, 176, 173, 179, 164, 172×47, 175, 176, 181×46, 176, 167, 162, 153×33, 148, 139 ×268,
172 ×34, 167, 157 ...` ✓ holds 172 in 416/875 (48 %), reaches 139 for 268 samples ✓ then leaves it ✓ churns ±20 px ✓

*Hold fractions* are safe to compare across the two cadences (shipped dump is change-triggered ~4 Hz ✓ port dumps at
10 Hz ✓ docs/115 ✓). **Step magnitudes are not** ✓: the shipped's per-sample deltas (−6, −4, +5, −10, −3, −7) are
aggregates of several logic steps ✓ so any "the shipped descends by 1, the port by ±1" claim is void ✓ (that voids
the magnitude half of docs/242 §3-4 and docs/244's per-step comparison ✓ docs/65 row ✓) ✓ Note also that
`player_y.py`'s kind-1 filter returns **0 samples on both sides** ✓ (docs/186 §1's gate ✓) so its "player samples
within the kind-1 Y envelope: 0/972 and 0/875" line is vacuous ✓ must not be quoted ✓

docs/245 measured the consequence directly: every one of 352 GATE4 rejections used the shipped's own allowance
`dl = 7` ✓ and the attacker stood **15-24 px off vertically** ✓ -- exactly the band the port's player occupies while
churning between 172 and 181 ✓ while the shipped's player sits at 139 ✓ (the enemies' spawn lines are 193 and 150 ✓
docs/247 §2 ✓) ✓

## 4. Root-cause target, and the transcription gap that matches it

The award machinery is now exonerated end-to-end: `fn_015295`'s gates, its two `fn_0003b5c0` queue posts (shipped
0x153a7 flag 1 after `alloc 0x19`, 0x15432 flag 0 ✓ carrying `cur[+0x32]`, `cur[+1]`, `cur[+9]-0x1e ✓`) and the
0x15453 award `player[+0x52] += cur[+0x32]` are all present and faithful in the port ✓ (docs/252 §2 ✓), the per-type
table `g_dat_53e9c` matches row-for-row ✓ (docs/237 ✓), the damage model is branch-faithful ✓ (docs/245 ✓), and both
movers and the walk step table are faithful ✓ (docs/243, docs/244 ✓).

What is *not* modelled anywhere in the port, and is exactly a "hold instead of churn" mechanism, is
**`fn_03272f`'s two settle conversions** ✓ (docs/239 §4 item 3 ✓ never acted on since ✓):

* `0x32c48-0x32c6e`: `if (word[player + 0x23] == 0 && player[+0x30] == 1) player[+0x30] = 2;`
* `0x32d38-0x32d4c`: `if (player[+0x30] == 1) player[+0x30] = 2;`

docs/203's earlier census put the port in state 2 **more** often than the shipped (357 vs 357 vs 216 ✓ direction to
be re-measured dump-side, since docs/239 §4 showed in-process censuses are not reproducible ✓).

Next measurement, specified: a **dump-side `+0x17` state census for the player record**, both sides, repeated twice
per side ✓ (dump-side is reproducible ✓ docs/239 §4 ✓) → that is item 2b's red basis for this leg ✓ then transcribe
the two `fn_03272f` stores from the shipped body ✓ implement ✓ and gate on (i) the state census moving toward the
shipped's, and (ii) the `+0x23` debt chain appearing on a kind-1 identity (`ent_life.py ✓`) with
`player[+0x52]` (EXP ✓ DS-resident and dump-visible ✓) rising ✓
