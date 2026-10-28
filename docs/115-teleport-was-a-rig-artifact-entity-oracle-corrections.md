# 115. "Enemy teleporting" is a rig artifact; three entity-oracle defects corrected

Date: 2026-10-29. Route: `ep1_kill3` 320 s, entity walks on **both** sides (`RIG_ENTS=12`), out=`/tmp/rig/k4`,
port built from the current tree (all docs/109/111/112/113 fixes in).

## 1. Three defects in my own entity oracle

1. **Record offsets.** `ent_list.py` located each `ent##` record by summing `rec_bytes` over the preceding
   cells. The dump gives every cell an explicit `offset`, and cells outside the record area also carry
   `rec_bytes` (with `rec_offset = -1`), so the running sum drifted: e.g. `ent00` really lives at blob offset
   25860 while the sum said 98208 (out of bounds). Fixed to use `offset`. **Every port-side entity reading in
   docs/108/110/111 that came from `ent_list.py` was therefore reading the wrong bytes**; the DS-cell evidence
   in those documents is unaffected.
2. **Sample-rate normalisation.** The port is dumped at `--rig-hz 10`, the shipped side at `--hz 4`. Raw counts
   were compared as if the cadences matched. k4: orig 1083 live entity samples vs port 1982 -- a 1.8x count
   inflation for the port in every per-sample statistic.
3. **Slot identity.** Transitions were computed per list slot. The entity list re-links between samples, so two
   entities exchange slots and the slot view shows mirrored "jumps" at identical timestamps:
   `slot0 238->98` while `slot1 98->238`, `slot2 336->567` while `slot4 567->336`. `speed_model.py --link`
   now tracks identity instead (nearest same-type predecessor within 48 px).

## 2. What the corrected oracle says on the current build

| measure | shipped | port | verdict |
|---|---|---|---|
| hurt-band (state in 0x0b..0x1e) live samples | 108 / 1083 = **0.100** | 181 / 1982 = **0.091** | equivalent |
| reaction state 0x0c samples | 42 (4.3 %) | 69 (3.2 %) | present on both (docs/111 fix holds) |
| state alphabet (top 10) | 1,2,21,41,0,12,3,51,52,4 | 1,2,21,41,12,0,51,3,11,53 | same shape |
| position jumps > 12 px, normalised | 443/1083 = 0.41 | 858/1982 = 0.43 | equivalent |
| identity-linked samples with no predecessor <= 48 px | 64 (5.9 %) | 105 (5.3 %) | equivalent |
| speed p50 / p90 / p99.5 (px/s, identity-linked) | 0 / 56 / 336 | 0 / 70 / ~same | equivalent |
| fallen family 0x5b..0x66 | 0 | 0 | **not covered by this drive** |
| `fn_015295` EXP gate as encoded in `ent_list.py` | pass=0 (type fails) | pass=0 (type fails) | **my gate encoding is wrong on both sides**; EXP itself was proven from DS cells (docs/109: 20 EXP / 2 kills vs shipped 10/kill) |

## 3. Consequence for the manual-playtest items

- **"Enemy teleporting"**: no divergence is detectable by the state oracle. Both sides show the same
  normalised jump rate, the same speed distribution, and the same unlinked-appearance rate; the large
  "jumps" are list re-linking. Either the symptom has another cause (screen-side: the *rendered* sprite
  jumping because of draw-order/palette, which the screen oracle would see) or it was fixed by an earlier
  change. It must not be claimed fixed on this evidence -- the honest status is *not reproducible at the
  entity-record level*, with the screen-side check as the remaining route.
- **"Missing hurt-stagger" / "player stuck when hit mid-kick"**: green at the record level on the current
  build (0.100 vs 0.091 hurt fraction, 0x0c present on both sides) after the docs/111 reaction-table fix.
- **"Missing fallen state"**: still zero samples on both sides for this drive -- needs a drive that throws or
  picks up an entity (or kills the player) before it can be called either way.
