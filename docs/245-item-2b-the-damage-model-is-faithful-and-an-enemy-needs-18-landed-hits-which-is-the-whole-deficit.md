# 245 — Item 2b: the entire damage model is faithful, and an enemy needs 18 landed hits -- which is the whole deficit

This closes the "is a branch missing?" question for item 2b definitively.

## 1. The shipped's damage block, transcribed end to end (0x148ea-0x149a3)

```
0x148ea debt = (int16)[B + 0x23]
0x148f0 if debt != 0 -> 0x1491c
0x148f2   dmg = [ebp-0x20]; 0x148f8 sub word [B + 0x29], dmg          ; HP -= dmg
0x14903   if ((int16)[B+0x29] > 0) -> 0x14917 (alive)
0x1490a   [B + 0x29] = 0
0x14913   [B + 0x30] = 1                                             ; settle stamp, HP-zero route
          -> 0x149ac
0x1491c   if (B == DS 0x53c7c /*player*/) -> 0x1495e
0x14927   if ([B + 0x56] == 0) -> 0x1495e
0x14932   sub dword [B + 0x56], dmg                                  ; the reserve pool
0x1493e   if ([B+0x56] >= 0) -> 0x1495c -> 0x14968
0x14944   else { add word [B+0x23], (int16)[B+0x56]; [B+0x56] = 0 }  ; overflow folds into debt
0x1495e   sub word [B + 0x23], dmg                                   ; THE DEBT DECREMENT (player, or pool == 0)
0x14968   if ((int16)[B+0x23] > 0) -> 0x149ac                        ; still owing HP -> nothing more
0x14973   else { add word [B+0x29], debt; 0x14984 [B+0x23] = 0; 0x1498d [B+0x30] = 2;
0x14994        if ((int16)[B+0x29] > 0) -> 0x149ac
0x1499f        else { [B+0x30] = 1; [B+0x29] = 0 } }                  ; settle stamp, debt-settled route
```

The port (`f.c:25254-25286`) has every one of these, in the same order, with the same branch conditions (its
`else if (cur == g_player_ent || ((struct sa32 *)((cur + 0x56)))->v == 0)` **is** 0x1491c-0x14930 ✓ its `else` **is**
0x14932-0x14955 ✓ and its drain block **is** 0x1495e-0x149a3 ✓ -- docs/242 §2's suspicion resolved ✓ **no missing
branch, no missing store** ✓

## 2. What an enemy actually needs (measured, port side)

`HIT` witness lines whose victim is an enemy (`/tmp/rig/h3`, 150 s, canonical drive):

```
vic(type=31 st=01) slot=0 atype=2 dmg=2 hp 0->0 debt 29 ovs 0
vic(type=31 st=01) ... dmg=2 ... debt 31 ovs 0
vic(type=31 st=01) ... dmg=2 ... debt 33 ovs 0
vic(type=31 st=02) ... dmg=2 ... debt 31 / 33 ovs 0
vic(type=36 st=01) ... dmg=2 ... debt 46 / 48 ovs 0
```

* `ovs` (`[B + 0x56]`, the reserve pool) is **0** for every enemy victim ✓ so the shipped's route is the
  **debt-decrement** one (0x1495e ✓) -- not the pool ✓
* debt starts at 35 (type 31) and 48-ish (type 36 ✓ type 36's `+0x23` is its own type value from the `.chr` row ✓)
  and falls by exactly `dmg = 2` per landed hit ✓
* therefore a type-31 enemy needs **18 landed hits** to bring debt to <= 0 ✓ at which point `+0x30 = 2`, then
  `+0x29 (= 0) + debt <= 0` -> **`+0x30 = 1`** ✓ which is the flag that docs/240 §2 says both award producers need ✓

The port lands **8 enemy hits per 150 s** (docs/242 §2: 5 x type 31, 3 x type 36 ✓) ✓ -> ~750 s per kill ✓
The shipped produced 31 dying record-samples carrying award 10 in the 150-300 s window (docs/199 §1) ✓✓ So the gap
is entirely **hit count** ✓

## 3. Where the hits are lost (the funnel, one more time, with the X numbers)

150 s, `/tmp/rig/h2` + `/tmp/rig/h3`:

| stage | count |
|---|---|
| `fn_013c4d` calls | 444 (404 A=player/B=enemy ✓ 40 A=enemy/B=player ✓) |
| rejected by the `dl = 7` Y window (GATE4, byte-faithful ✓ docs/238 §1) | 270 |
| rejected by the X corner test (GATE5 ✓ 0x14006-0x14026) | 114 |
| rejected by the Y corner test (GATE6 ✓ 0x14028-0x1405e) | 9 |
| passed all gates and returned 1 (`GATEOK`) | 31 (11 player->enemy ✓ 20 enemy->player ✓) |
| damage applied to an enemy | 8 |

The most common player/enemy geometry pair at call time is **A(y)=139, B(y)=141** (71 of 404 ✓) -- inside the
`dl = 7` window ✓ so the Y placement is frequently *right* ✓ and the near-misses are on **X**: examples of GATE5
rejections are `x A[363,387] B[326,360]` (3 px short ✓) and `x A[132,196] B[210,242]` (14 px short ✓) -- i.e. the
player attacks a few pixels past or short of the enemy's box ✓

Docs/244 established that the walk step magnitudes (1-2 px ✓ `DAT_00052ffc` = `1,2,2,1,1,2,1,2,1,1` then zeros ✓)
and the gate/index/sign logic (`lab_0003284e` ✓ 0x32856-0x328f5) are faithful ✓, and the drive vocabulary on this
route is `335 lctrl / 88 up / 40 right / 38 left / 34 enter / 12 down` ✓ (attack presses already every ~2 s ✓) --
so the presses are not the limit ✓ the *alignment* is ✓

## 4. The next experiment (specified)

Item 2b needs a drive that lands >= 18 hits on one enemy. Nothing in the port's code needs changing for that ✓
so the experiment is a **drive parameter sweep**, and it is machine-only:

* vary only the approach: cut the `left`/`right` hold windows so the player stops 3-14 px closer to the enemy
  (the measured GATE5 shortfall ✓), a handful of variants;
* run each variant on the port with `NIGHT_HITTRACE=1 NIGHT_SET30=1` and read `DMG`/`LETHAL`/`set30` counts;
* as soon as one variant shows `LETHAL` (i.e. `[+0x30] = 1` on a kind-1 record ✓), that same drive becomes the
  **red basis**: the port's dying record must then show `+0x32 = 0` (or the flat 5) where the shipped's shows
  `+0x32 = 10` for `type_idx = 31` (docs/199 §1) ✓ measured with `scripts/rig/exp_award.py`, repeated >= 2x per side
  (docs/239 §4);
* only then is a code fix on the award path admissible ✓ and its green measurement is the same census ✓

If no variant lands 18 hits, the honest conclusion is the one docs/244 §5 already points at: the alignment
shortfall is a consequence of the logic-cadence residual (docs/168, docs/203, docs/205), and item 2b's gate moves
behind the pacing item.

## 5. New tool, and a rig discrepancy it exposed (open)

`scripts/rig/hit_rate.py` counts landed hits from the dumps alone: a decrease of `word[record + 0x23]` between
consecutive samples of the same record slot is one damage application (0x1495e is the only writer of that field on
this path ✓ docs/245 1 ✓), and the `+0x30 -> 2 / == 1` transitions are read straight from the record.

Port side works (`/tmp/rig/h3`, current build):

```
== port: samples=1500 span=229.6s enemy-record-samples=1968 debt-decrease events=79 -> 0.34 hits/s
   per type_idx: [(31, 73), (36, 6)]
   settle transitions: +0x30->2 seen 0 times, +0x30 == 1 seen 0 times
```

Two caveats recorded rather than smoothed over:

1. **79 debt-decrease events vs 8 `DMG` witness lines** ✓ The dump-side count is inflated: a slot that gets
   re-assigned to a different enemy (lower `+0x23` ✓) also looks like a decrease ✓ The script splits decrease
   magnitudes (`<= 16` vs `> 16`) to make that visible, and the settled headline stays the witness-side one
   (docs/242 §2) until the slot-reuse cases are excluded ✓
2. **The shipped side reports `enemy-record-samples = 0`** ✓ even though docs/199 §1 measured 1460 shipped kind-1
   record samples from the same dump ✓ so `hit_rate.py`'s shipped-side walker does not match `exp_award.py`'s ✓
   The shipped dump's `ptr_cells` list (63 cells, each `rec_bytes = 352`) starts with `g_dat_51a0c`,
   `g_dat_53730`, `g_dat_537ca`, ... whose blocks are pointer-valued (kind bytes 64, 69, 56, 34 ✓ i.e. the
   "kind 56/34/7/8" populations of docs/242 §1 ✓), and the tool's stricter gate (`kind == 1`,
   `type_idx < 0x1000`, `state < 0x100`) accepts none of them ✓ **Open rig defect: reconcile the two walkers
   before any shipped-side hit-rate number is quoted** ✓ Until then the shipped's hit count is only known through
   its outcome (31 dying samples carrying award 10 ✓ docs/199 §1), not through a hit census ✓
