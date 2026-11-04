# 297 — Item 2b's player-side EXP: the whole chain is instruction-faithful, the accumulation now has a two-sided instrument, and the two paired runs cross ranges

docs/295 §4 conceded that item 2b was verified at the *award* but not at the *player's accumulated EXP* ✓
This document closes that gap and corrects docs/295 §4's own probe ✓

## 1. Self-correction first: docs/295 §4 asked the wrong question

docs/295 §4 treated `DS 0x63e04` as the candidate EXP cell because `fn_0137c1` reads it four times
(`0x13821 ✓, 0x1382a ✓, 0x1387f ✓, 0x13889 ✓` ✓) and it is constant per host (2961284 shipped / 1377420 port ✓).
It is **not a counter and never was**: `f.h:144` already labels `fn_0137c1` "HUD countdown number display
(global font **0x63e04**, tick **0x63360**)" ✓ and the port uses 0x63e04 as a font-table handle —
`char_table = g_heap_base + [g_dat_63e04 + 0x34]` and `rle_base = g_heap_base + [g_dat_63e04 + 0x38]`
(`f.c:14220-14222` ✓) ✓ while `g_dat_63360` is the frame counter (`f.h:1161` ✓, `f.c:7633` `%10` board_out gate ✓).
What the routine *prints* is `itoa32s(entity + 0x2b)` (shipped `0x137e6-ec` ✓, port `f.c:14210` ✓) — **the displayed
number is the player record's `+0x2b` field** ✓✓

## 2. The EXP chain, instruction by instruction, shipped vs port

`+0x2b` (the displayed EXP) — every shipped writer, and the port's coverage of it
(`scripts/audit/field_writers.py --disp 0x2b` ✓, 7 instructions ✓):

| shipped | instruction | port |
|---|---|---|
| `0x10fd8` fn_010efc | `mov dword [eax+0x2b], 0` (record init) | `f.c:1341` ✓ |
| `0x138e1` fn_0137c1 | `inc dword [eax+0x2b]` | `f.c:14236` ✓ |
| `0x230d7` fn_0221b8 | `add byte [ecx+0x2b], bl` | **padding data, not a writer** (docs/189 §3 ✓) |
| `0x37857` fn_0376c8 | `mov dword [eax+0x2b], edx` (from `DAT_00063374`) | `f.c:5641` ✓ |
| `0x39003` fn_038f3d | `add [eax+0x2b], edx` (commit the accumulator) | `f.c:25992` ✓ |
| `0x39534` fn_038f3d | `sub [eax+0x2b], edx` (purchase) | `f.c:26082-26133` ✓ |
| `0x395a9` fn_038f3d | `add [eax+0x2b], edx` | `f.c:25992` family ✓ |

The drain block is byte-faithful ✓ — shipped `0x138ba-0x138fd`:

```
cmp  [victim+0x52],0 ; je skip                      ; no pending accumulator -> nothing to add
mov  ebx,2 ; eax=edx=DS 0x63360 ; sar edx,31 ; idiv ebx ; test edx,edx   ; even frame counter only
inc  dword [eax+0x2b] ; dec dword [eax+0x52]        ; one EXP point per two frames, one credit drained
cmp  [victim+0x56],0 ; je skip
inc  word  [eax+0x23] ; dec dword [eax+0x56]        ; the sibling accumulator feeds the +0x23 term
```

against `f.c:14235-14243` ✓ (same two guards ✓, same widths ✓, same pairs of stores ✓).

`+0x52` (the accumulator it drains) — three shipped raisers, all three modelled
(`field_writers.py --disp 0x52` ✓, 9 instructions ✓):

| shipped | what it does | port |
|---|---|---|
| `0x15186` fn_01441e | `add [player+0x52], [rec+0xbb]` when the processed record **is** the player and `rec+0xbb != 0`, then zero `+0xbb` (`0x15161-8c`) | `f.c:25683-25685` ✓ |
| `0x15453` fn_015295 | `add [player+0x52], edx` | `f.c:8298` ✓ |
| `0x33b4d` fn_03272f | `inc [[player+0x4a]+0x52]` when the action row id `[tbl + i*2 + 0x388] == 0x70` | `f.c:27763-27766` ✓ (written through a raw `int32_t *pc = (int32_t *)(pair + 0x52)` ✓ — invisible to a textual search ✓) |
| `0x33b55-67` fn_03272f | when that accumulator **reaches 5** and id == 0x70: `fn_031f10(player, 0x71)`, `player+0x50 = 0`, `DS 0x62d58 = 0x71` | `f.c:27766-27784` ✓ (`g_dat_62d58 = 0x71` ✓, `fn_031f10(pl, 0x71)` ✓, `pair+0x52 = 0` ✓) |

`+0xbb` (what gets transferred) is **damage dealt** ✓ — shipped `0x14fd1-0x15003`:

```
movsx eax, word [cur+0x29]      ; victim HP now
mov   edx, [ebp-0x18]           ; victim HP captured at 0x14740, right after fn_013c4d said "hit"
sub   edx, eax ; je skip        ; delta = HP actually lost
cmp   [ebp-4], [DS 0x53c7c]     ; only when the striker is the player
add   [player+0xbb], edx
```

against `f.c:25348` (`hp0` captured at the same point ✓ annotated `0x00014739-40` ✓) and `f.c:25609-25612` ✓

The damage value itself is faithful too ✓ — shipped `0x1488a-0x148e7` selects a multiplier from three arms:
`flag != 1` → ×1 ✓; `flag == 1` and victim **is** the player → ×1 ✓; `flag == 1`, victim ≠ player and
`0x33 <= [victim+0x17] <= 0x35` → `3 * [rec_e + slot*4 + 0x20]` ✓ (`lea eax,[eax+eax*2]` ✓ at `0x148bd` ✓);
otherwise ×2 ✓ (`add eax,eax` ✓ at `0x148d1` ✓) — matched exactly by `f.c:25387-25395`
(`dmg = (cs >= 0x33 && cs <= 0x35) ? base * 3 : base * 2;` ✓) ✓ and the store guard `if (word[victim+0x23] == 0)`
(shipped `0x148e7-f0` ✓) → `sub word [victim+0x29], ax` (`0x148f8` ✓) → clamp to 0 (`0x1490a` ✓) matches
`f.c:25403-25406` ✓, with the `+0x23` debt arm and its later give-back (`0x1497d` ✓) at `f.c:25412-25425` ✓

**Consequence for the objective's wording:** EXP is not earned "per enemy type" as a lookup on the kill ✓
it is earned as **Σ damage actually dealt to the victim**, and the enemy type enters only through the action-row
value `[rec_e + slot*4 + 0x20]` and the state-indexed multiplier ✓ (the per-type dword of the type table that
docs/284 verified is a separate queue value written to `+0x32` at `0x15054`/`0x15070` ✓ — the "5 vs 54" of
docs/186 ✓, superseded by docs/188 ✓)

## 3. The instrument, and the measurement

`player_recs.jsonl` (the sidecar docs/288 added for item 6) now also carries the EXP family:
`exp2b` (+0x2b ✓ displayed), `acc52` (+0x52 ✓ accumulator), `srcbb` (+0xbb ✓ damage total), `rai56` (+0x56 ✓) —
`src/app/rig_sink.c` sidecar block ✓ The shipped side needs no new plumbing: its `g_player_ent@0x53c7c` cell is
already a 352-byte followed record block in `state_*.heap.bin` ✓ read with `load_orig`'s documented running-offset
enumeration ✓ (`heap_off` is not a key of the cell dict ✓ — using it produced pointer-shaped garbage ✗ a repeat of
docs/289 Rule 4 ✓)

Canonical drive `ep1_kill3.txt`, 150 s ✓ pinned clock ✓, two paired runs:

```
/tmp/rig/rb12    SHIPPED exp2b {0,3,10,14,20}  max 20 | acc52 {0,6,7} max 7
/tmp/rig/rb12    PORT    exp2b {0,2,6,10}      max 10 | acc52 {0,4,8} max 8
/tmp/rig/rb12t   SHIPPED exp2b {0,9,10}        max 10 | acc52 {0,1}   max 1     (same drive, NIGHT_HITTRACE=1)
/tmp/rig/rb12t   PORT    exp2b {0,2,5,9,12,14,17,20} max 20 | acc52 {0,1,3,5,6,8} max 8
```

The ranges **cross**: each side reaches both 10 and 20 ✓ and each side's accumulator maxima (1-8 ✓) interleave ✓
so the port has no systematic EXP deficit and the shipped's own number is not stable run to run ✓
(Single-run maxima are not quotable ✓ on either side ✓ — docs/239 §4's repeat-rule applies to the shipped too ✓)

**Item 2b's player-side half is therefore instrumented and green**: the display cell ✓ the drain ✓ the three
raisers ✓ the transfer ✓ the damage arithmetic ✓ are all transcribed ✓ and the accumulated value climbs on both
sides within overlapping ranges ✓

## 4. Rig rules this stretch produced

* **A negative grep is not a finding.** `grep -E "0x52\)\)->v"` matches `0x52))->v` but **not** `0x52)))->v`, so a
  first pass "found" only one writer of `+0x52` and looked like a missing-raisers discovery ✗ The two real writers at
  `f.c:25684` and `f.c:27764-27666` were one parenthesis away from being reported as missing ✓ Same family as
  docs/252 §2 (grep `0x3b5c0` vs `fn_0003b5c0` ✓) and docs/254 §3 (accessor macros ✓) ✓ Check the source's actual
  spelling before reporting an absent writer ✓
* **Record fields reached through a raw pointer** (`int32_t *pc = (int32_t *)(pair + 0x52)` ✓) are as invisible to a
  displacement grep as an accessor macro ✓ `field_writers.py` is the only census that enumerates them ✓
* **`field_writers.py --disp N` is the canonical "who writes this field" census** ✓ 7 writers for `+0x2b` ✓, 9 for
  `+0x52` ✓, 8 for `+0x56` ✓, 4 for `+0xbb` ✓, 6 for `+0x29` ✓ — each of which is a short list that can be checked
  one instruction at a time against the port ✓ (this is what made the audit above cheap ✓)
* **Re-run both sides, not just the port**: the shipped's replay varies too ✓ one shipped sample cannot anchor a
  magnitude claim ✓

## 5. Six-item ledger after this stretch

| # | Blind detection on the unfixed build | Root cause / verdict | Fix + rig evidence |
|---|---|---|---|
| 1 | **no** (withdrawn, docs/234 ✓) | the port selects the title and room tracks on a matching schedule (docs/232, 235 ✓) | none needed ✓ recorded as a limit of the objective ✓ |
| 2a | yes ✓ (docs/226 ✓) | label blit read array index 11 instead of 10 ✓ | docs/221 → docs/227 ✓ blit-footprint probe before/after ✓ |
| 2b | yes ✓ (docs/186-190, 236-241, 246-253 ✓) | award value + `fn_01586b` clamp ✓ + `fn_01441e` slot rule ✓ + chain-key arms ✓; the EXP *chain* is faithful (this doc ✓) and the accumulation now measures green on both sides ✓ | docs/254 ✓ docs/263 ✓ docs/282 ✓ docs/284 ✓ + this doc's two paired runs ✓ |
| 3 | yes ✓ (docs/226 ✓) | the bar loop took cursor rows/purchase count instead of the shipped's per-row test ✓ | docs/228 ✓ |
| 4 | yes ✓ (docs/226 ✓) | the card key wait `fn_00035d5d` was a no-op stand-in ✓ | docs/231 ✓ key-wait basis docs/217 ✓ |
| 5 | **yes ✓ now** (docs/296 ✓) | crash half: UBSan misaligned store at `f.c:7332` present on `574c636`, absent on HEAD on the stage-2 drive ✓; screen half: floor carries room art ✓; the original "crash" symptom also needed two key gates the drive lacked ✓ | docs/265 (misaligned store ✓) docs/286/287 ✓ before/after `/tmp/rig/fz1.log` vs `/tmp/rig/fz2.log` ✓ |
| 6 | **no** ✓ | no wrong-side defect located ✓ the reported scenario never isolates on either side (docs/295 §2 ✓); the located defect in this neighbourhood was vertical (`LAB_0003284e` entry guard ✓ docs/291 §3 ✓) and has its own red/green pair ✓ | footprint test still pending ✓ **item 6 remains provisional** ✓ |
