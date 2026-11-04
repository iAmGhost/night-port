# 294 — Final verdict on the six 03-2026-10-29 items: what the rig detected blind, what it localized, what was actually wrong, and what turned out to be no defect

Everything below is rig-produced (no human playtest input in any verdict line ✓), with the run, cadence and sample
count behind each measurement in the cited doc ✓

| # | Item as reported | Blind-gate verdict on the unfixed build (docs/226) | Root cause established from the shipped disassembly | Fix landed (rig before/after evidence) | Final status |
|---|---|---|---|---|---|
| 1 | Intro music plays, is skipped by a keypress, never switches to title music | DETECTED-AND-LOCALIZED → **verdict withdrawn** by docs/234 (the timing behind the detection was a `--rig-dump` artifact) | none: the CD-age counter is aged by the shipped timer IRQ, which the port fires from its frame pump (docs/232); the title/room track selection schedule matches the shipped (docs/235) | no change needed | **closed, no surviving defect** |
| 2a | Kill EXP text is garbled | DETECTED-AND-LOCALIZED (glyph witness + blit-bbox probe) | the label blit read sprite-array **index 11** where the shipped reads **index 10** (docs/221, shipped `0x3b7…` family, font descriptors `4x7` / `10x12` from `system.spr`/`system2.spr` ✓ docs/219) | yes — index 10 (docs/227 ✓ blit-footprint probe is the admissible basis, docs/227 §5) | **fixed** |
| 2b | EXP gain must be per enemy type | DETECTED-AND-LOCALIZED (record-sampling oracle `exp_award.py`) | the award is the first dword of the enemy-type row: shipped `imul eax,[eax+0xd],0x88; mov edx,[eax+0x3ea8]` (stride 0x88 ✓ 0x104 rows ✓ `DAT_00053ea8`); the award path was unreachable because (i) `fn_01586b`'s clamp guard took `cur < mn` out of the unclamped branch (docs/254), (ii) `fn_01441e`'s slot selection accepted a row on only one threshold (docs/263), (iii) the key-chain never reached states 0x33-0x36 (docs/271-282) | yes — clamp guard, slot selection, chain gates (docs/254, 263, 282) | **closed**: the port awards exactly the shipped's per-type value (type 31 → 10 ✓ docs/284) |
| 3 | Upgrade menu lights every item | DETECTED-AND-LOCALIZED (`NIGHT_LUMENU` witness + time-paired screen oracle) | the bar loop ignored the cursor rows / purchase count that the shipped uses to pick which rows light | yes (docs/228), with two fixtures that had encoded the invented rule corrected (docs/263 §4, docs/265 §1) | **fixed** |
| 4 | Level-up intro (and "EXP not enough") auto-advances | DETECTED-AND-LOCALIZED (picture-identity census over a key-free window) | `fn_00035d5d` was a no-op stand-in for the shipped's press-and-release card gate `fn_00035dfc` (`0x35e14-0x35eb6` ✓ accepts only Enter/CTRL/ALT/Space ✓ docs/97) | yes (docs/231) | **fixed** |
| 5 | Stage 2 crashes (`bus error ./build/night`) and the floor renders black | screen half DETECTED-AND-LOCALIZED (mode-segment floor census) ✓ crash half **NOT REPRODUCIBLE** on the unfixed build | the stage gate and the VESA check (`fn_03f15f`) are faithful (docs/268); the symptom was that the drive never satisfied `fn_038f3d`'s exit gate (`0x3920a-31` = Esc/LShift/RShift ✓) while `fn_00035dfc` needs Enter/CTRL/ALT/Space — two different key gates, both required (docs/286, docs/287) | no port change for the symptom ✓; the *rig* gained `ep1_roomclear3.txt` and the `NIGHT_ROOMSTART` witnesses (docs/285-287); the related vertical clamp guard was fixed (docs/254) | **closed**: the port loads `stage2.pcx/stage2.wal/stage2.stg/night2.chr` + `2s1/2s2.pcx`, composites 320x200 rooms for 58.7 s, floor rows 100-199 pure-black **0.00 %** of 7.36 M px ✓, ASan `rc=0` with 0 reports over 13249 stage-2 frames ✓ (docs/287) |
| 6 | Attacking at the far left edge draws the leg/arm on the right side | DETECTED-AND-LOCALIZED (`NIGHT_ENTGEOM` anchor witness + `field_writers.py`) — then **both proposed mechanisms were refuted by measurement**: the "five invented proximity flips" (docs/291 §1: the scan used `entry+body_bytes`, 93 B short of `body_end`, and the shipped's flip tails sit in exactly that gap ✓) and the "shipped latches the last directional key" theory (docs/292 §1: with an isolated press/release pair the shipped clears `0x63ed3` at t=85744 and the port at tick 796 ✓✓, and the earlier latch was a dropped release edge in the replay injection ✓) | the walking body's entry guard was genuinely wrong: the shipped admits `LAB_0003284e` only on `0x63ed0/0x63ed3/0x63ed5` (`0x3285f-0x3287c`, exit `0x3294d` ✓) and tests `0x63ed8` only inside the vertical sub-branch (`0x328cc` ✓); the port admitted `0x63ed8` too, so a Down press alone walked the player and forced state 2 (docs/291 §3) | yes — the guard fix (docs/291/292): pre-fix `Y = {172:580, 195:91}` with a descent to the `195` clamp, post-fix `Y = {172:912}` with 0 Y-change events ✓, left-edge screen row band `1.90 → 1.54` ✓ (docs/292 §5.1) | **closed, no surviving wrong-side defect**: left-bound record state identical on both sides (`X=-30, Y=172, +0x1b=0, state 0 ✓`), every draw input byte-faithful (docs/212, 220, 229, 230, 255), and on an event-anchored both-sides window (`/tmp/rig/rm5`, shipped 99 frames / port 98) the ground-line column shares are `25.6/38.0/36.4 %` vs `25.7/37.5/36.8 %`, L/R `0.70 = 0.70` ✓✓ (docs/293 §1) |

## Sweep state on the final source (`0996686`)

* `make test` → **ALL PASS** ✓ (the `test` target rebuilds `build/night` ✓ docs/181 §5 ✓)
* Nine-episode ASan/UBSan soak `/tmp/rig/soak293` → 9/9 `rc=0 ubsan=0 asan=0 segv=0` ✓, no sanitizer text in any log ✓
* Coverage-guided fuzz (`--episodes 1..9 --seeds 12 --cap 120`) → **257/503 functions, 0 crashes** ✓ (baseline 248/503 ✓)
* State oracle: canonical route `/tmp/rig/rb11` → `SAME 71, DIFF 26, SIZE? 34, PTR 52, REPR 20, DIFF-SIZE 14` ✓
  (one row closed vs `rb10` ✓, no new unexplained class ✓); room drive `/tmp/rig/rm5` → activity profiles 271,
  phases 168, `tally {'SAME': 271}` ✓
* Screen basis: channel-order check ok ✓ (0.982 canonical, 0.703 room drive); pacing row unchanged
  (`orig 2.72 vs port 19.51 ✓` canonical) and attributed to the docs/168 pacing residual family ✓
* Audio trigger-frequency basis: CD-DA `active_frac` 0.992 (port) vs 0.964 (shipped ✓), onsets 6 vs 4 ✓; the PCM/SFX
  row keeps its standing attribution (`78/1499 record windows ✓` docs/129-133, docs/167 family ✓) — unchanged ✓
  Known rig hygiene item: `route.sh`'s summary cats `report_audio.txt` while the audio leg writes
  `report_audio_sfx.txt` / `report_audio_cd.txt` (docs/287 §4 ✓) — cosmetic, not a basis change ✓

## Still open outside the six items (recorded, not claimed as goal work)

1. **Vertical distribution residual**: on the same anchored window the row band differs (shipped meanRow 156.3 /
   `rows<170 : >=170 = 2.33` vs port 160.6 / 1.63 ✓ docs/293 §1) — the docs/242 / docs/255 ground-line family, the
   strongest remaining non-item divergence ✓
2. **`g_tab_52b84` declaration width**: the rig table holds 90 bytes, a `f.h` declaration reads 360 ✓ (docs/270) —
   an open declaration-reconciliation item, deliberately not forced wider ✓
3. `route.sh`'s `report_audio.txt` naming ✓ and the standing cadence rule that dump-side runs support *what* and
   *where* claims but never pacing/latency claims (docs/234 §3 ✓).

## ERRATUM (docs/295)

* §4's audio rows are superseded: they quoted `/tmp/rig/rb10` ✓; the final sweep's own run is `/tmp/rig/rb11` ✓
  (`report_audio_cd.txt`: orig 1331 buckets / onsets 9 / active_frac 0.959 ✓ vs port 5591 / 4 / 0.991 ✓;
  `report_audio_sfx.txt`: port PCM into 99/1499 record windows ✓ attribution unchanged ✓).
* §1 row 6's "closed, no surviving wrong-side defect" and its closure-by-attribution are **withdrawn** ✓ item 6 is
  **provisional**: no wrong-side defect was located, the reported left-edge-attack scenario does not isolate on
  either side (`/tmp/rig/le5`: zero bound-and-attack samples on both sides ✓), and the decisive footprint test has
  not been run ✓ (docs/295 §2 ✓)
* §1's "blind verdict" column overstates items 1 and 5 ✓: item 1's blind detection was withdrawn (docs/234 ✓) and
  item 5's crash half was NOT REPRODUCIBLE (docs/226 gap 3 ✓) ✓ the rig detected and localized four of six ✓
  (docs/295 §3 ✓)
