# 289 — Methodology refresh #7: what the rig proved about items 5 and 6, the sweep numbers on the current source, and the rules this stretch's mistakes produced

## 1. Six-item status (docs/manual-playtest/03-2026-10-29.md), rig-evidenced

| # | Item | Status | Basis |
|---|------|--------|-------|
| 1 | intro music never switches to title music | **closed, no surviving defect** | docs/232 (the CD-age counter is aged by the shipped timer IRQ the port fires from its frame pump), docs/234/235 (the red measurement was a `--rig-dump` artifact; the selection schedule matches) |
| 2a | EXP text garbled on kill | **fixed** | docs/221/docs/227: the label blit read array index 11 where the shipped reads 10; blit-footprint probe is the admissible basis (docs/227 §5 shows a pixel census cannot demonstrate it) |
| 2b | EXP must be per enemy type | **closed** | docs/284: the per-type award is the first dword of each 0x88-stride row of `DAT_00053ea8` (shipped `0x15060-0x15070` `imul eax,[eax+0xd],0x88; mov edx,[eax+0x3ea8]`), the port reproduces both arms; docs/282 (chain selection fixed by pulse-shaped drive repeats), docs/283 (award gates faithful, three resolved callers) |
| 3 | upgrade menu lights every item | **fixed** | docs/228: the bar loop now takes the cursor rows / purchase count (`sel[]` rows), two invented fixtures corrected (docs/263 §4 family) |
| 4 | level-up intro auto-advances | **fixed** | docs/231: `fn_00035d5d` was a no-op stand-in; the press-and-release gate is modelled |
| 5 | stage 2 crashes / floor black | **closed, no surviving defect** | docs/287: on a drive that satisfies both key gates the port loads `stage2.pcx/stage2.wal/stage2.stg/night2.chr` + `2s1/2s2.pcx`, runs 58.7 s of 320x200 room compositing, floor rows 100-199 pure-black **0.00 %** of 7.36 M sampled px; `night-asan` on the same route `rc=0`, 0 ASan/UBSan/SEGV with 13249 stage-2 room frames |
| 6 | limb drawn on the wrong side at the left edge | **open — not reproduced, not resolved** | docs/288: on `ep1_leftedge2` both sides park the player at `X=-30`, `Y=172`, `+0x1b=0`, state 0 ✓✓; every limb-draw input is proven byte-faithful (docs/212, 220, 229, 230, 255); the ground-line cluster census cannot resolve a 20-40 px offset at cadence ratio 0.57 → no red and no green claim |

## 2. Sweep numbers on the current source (`f6943e2`)

* `make test` → **ALL PASS** (the `test` target rebuilds `build/night` ✓ docs/181 §5 ✓)
* nine-episode ASan/UBSan soak `/tmp/rig/soak288` → stage 1..9 `rc=0 ubsan=0 asan=0 segv=0`, 4000 frames each; `grep -l "ERROR: AddressSanitizer|runtime error|Segmentation"` over its logs returns **no file** ✓
* coverage-guided fuzz (`scripts/rig/fuzz.py --episodes 1..9 --seeds 12 --workers 3 --cap 120`) → **coverage 260/503 functions, never-entered 243, crashes 0** ✓ (above the 248/503 baseline ✓)
* canonical state route (`ep1_kill3.txt`, 150 s, `RIG_HZ=10`, pinned clock) `/tmp/rig/rb10` → cells 346, phases 274, tally `SAME 70, DIFF 27, SIZE? 34, PTR 52, REPR 19, DIFF-SIZE 14`, activity profiles 271 **all SAME**, divergent runs 8 ✓ vs rb9 (`SAME 69, DIFF 28, SIZE? 32, PTR 52, REPR 19, DIFF-SIZE 16`) → one row moved DIFF→SAME, no new unexplained class ✓
* screen basis: `channel-order check declared=0.972 ok` ✓ on rb10; pacing row `orig 2.73 vs port 19.31 changes/s ratio 0.14` remains the standing pacing residual (docs/168 family ✓ attribution unchanged ✓)

## 3. New instruments this stretch

1. **`player_recs.jsonl`** (`src/app/rig_sink.c`, docs/288 §2): the player record's `x(+0x01)`, `y(+0x09)`,
   `st(+0x17)`, `f1b(+0x1b)` and `hp(+0x29)` per tick ✓ — `records.jsonl` has no position fields and the
   `ent##` cells read as zeros on the left-edge route ✓ Its shipped-side counterpart already existed
   (`diff_state.load_orig` record blocks ✓), which is what made the §3 both-sides table possible ✓
2. **`room_start` / room-loop / tail witnesses** under `NIGHT_ROOMSTART=1` (`f.c`): they are what localized item 5's
   symptom to `fn_038f3d`'s exit gate in one run instead of a week of DS archaeology ✓
3. **Two new drives**: `ep1_roomclear3.txt` (a key stream that satisfies `fn_038f3d`'s Shift/Esc exit *and*
   `fn_00035dfc`'s four game keys across the whole window ✓) and `ep1_leftedge2.txt` (kill3 bring-up + walk to the
   left bound + attack in place ✓) ✓

## 4. Rules this stretch produced (each from a mistake I made)

1. **A drive is part of the measurement basis.** Both sides must be in the same *phase* before any screen claim:
   `le1` compared shipped rooms against port slide cards ✓✗ because a release-only key event cannot satisfy a
   press gate ✓
2. **Know each wait's accepted keys before claiming a hang.** `fn_038f3d` exits on Esc/LShift/RShift
   (`0x3920a-31` ✓); `fn_00035dfc` advances only on `0x63ea4/0x63ea5/0x63ec0/0x63ec1` (docs/97 ✓) ✓ Shift presses
   fixed one gate and stalled the other ✓
3. **A counter-advance flag is not a phase predicate.** `g_dat_63370` increments at `0x3b520`, *before*
   `fn_038f3d` at `0x3b54c` ✓, so "stage 2 samples" and "`DAT_00063354 != 0`" both failed to prove a live room
   (docs/286 §4 ✓ superseding docs/269's basis ✓) ✓
4. **Never hand-roll a dump parser.** My `Struct("<IIHHI")` guess for `frames.bin` produced mode `(50679,50584)`
   garbage ✓ (docs/285 §4 ✓); and `frames/NNNNNN_WxH.bin` names are change-counter ordinals, not timestamps ✓ —
   stage windows must be selected by each mode segment's share of the changed frames ✓ (docs/287 §4 ✓)
5. **Absolute colour across the two dumps is still inadmissible** (port `(255,195,195)`, shipped `(121,121,0)` ✓
   same signature in stage 1 ✓): compare ink presence and cluster geometry, not RGB values ✓ (docs/287 §2 ✓)
6. **An instrument that reads zeros everywhere is a broken instrument, not a finding** (`min world X = 0` in
   1677/1677 `ent##` samples ✓ docs/288 §2 ✓) — write a dedicated sidecar rather than interpret a
   pointer-following cell ✓
7. **Fix the fixture-oracle gap in the direction of the disassembly**, and say so: this goal has now corrected
   three tests that asserted invented rules (docs/263 §4, docs/265 §1 ✓) ✓

## 5. The one open item's next concrete step

Item 6 (docs/288 §5): a **same-record ⇒ same-rect** test — dump the port's actual limb blit destination rects at the
left bound (`blit_probe_note`, `f.c:3168`, armed under `NIGHT_GLYPHDBG`) and compare them against the destination
computed from the shipped's faithful arithmetic (`0x13dc1-0x13ddb` / `0x13ee6-0x13f00`, docs/255 §2) fed with the
*record values now known to be identical on both sides* ✓ That test is independent of cadence, of the capture
basis, and of the colour basis ✓ If the port's rects match the arithmetic, item 6 closes as "no port-side defect";
if they do not, the divergence is localized to a specific blit site ✓
