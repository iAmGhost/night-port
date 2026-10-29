# 150 — Blind-gate ledger: each playtest item, the rig signal that found it, and the fix it produced

The gate contract (docs/105): the rig must detect and localise every open item in
`docs/manual-playtest/02-2026-10-28.md` with no human playtest input, on the unfixed build. This ledger closes
the bookkeeping: one row per item, naming the oracle that produced the signal, the window, the suspect
function/DS region, the fix, and the red -> green measurement. Where the rig initially missed an item, the miss
is recorded as a rig defect and the rig was fixed before the item was re-tested.

| # | playtest item | rig signal (which oracle) | window | suspect region found | fix | red -> green |
|---|---|---|---|---|---|---|
| 1 | music never stops at the title | CD-schedule oracle `scripts/rig/cd_schedule.py` (play/stop witness marks) | 3.7-8.1 s probe; 45.5-47.4 s route | `fn_000405a7` / `sys_cd_audio` teardown path | docs/106: `cdda_play` re-armed a single SDL stream without stop/flush on replacement | red "4 plays, 4 stops, 1 change with NO stop" -> green "4 plays, 5 stops, 0 changes with no stop" |
| 2 | no slideshow track | same schedule oracle | P6@70000 (70.75 s stop+play) | `cd_track_for_room` / `g_tab_52da4`, and the slide-show trip count | docs/133: `g_dat_52f08` (pictures per stage) was zeroed where the image holds `2,5,6,6,6,4,3,9,...` | trip count 0 -> image values; `c3b` byte mismatches 3 -> 0 |
| 3 | no stage-start change | same schedule oracle | P1@49300, P6@70000 ordering | same | docs/106 teardown + docs/135 confirmed `g_tab_52da4` byte-identical to the image | selection ordering equivalent (same LBAs 0x918e/0x2d320/0x367aa/0x9ddf, same lengths 42/369/238/261 s); residual is *audible* ordering, docs/131 |
| 4 | EXP not awarded on kill | state oracle record census + `NIGHT_ACTTRACE` EXP witnesses | kill windows of `ep1_kill3` | `fn_01441e` award site (guest 0x15453) and the `DAT_00053ea8` array | docs/109: `DAT_00053ea8` is `DAT_00053e9c + 0xc` -- the port kept an unfilled array and added a spurious `+0x0c` | player EXP 0 -> 20 for 2 kills (shipped 10/kill); witnesses `adds=2 added=20` |
| 5 | missing punch SFX | audio oracle (activity profile per drive phase) | 12 windows, every combat window | sound-slot pool DS `0x53d0c` and the slot table one dword below it | docs/113: the port invented `g_tab_53d08[101]`, never written, so `fn_04185e` always received null | `SILENT` in all combat windows -> `played=74 outside-pool=0`; PCM 478 KB -> 1.14 MB (2.4x), concentrated in the fight window; `NIGHT_PLAYVOC=7` peak 13568 |
| 6 | hurt/stagger missing, infinite punching | state oracle entity census (`scripts/rig/ent_list.py`, +0x17 state census) | fight windows; shipped 488 player records | `fn_01441e` reaction-table lookup (guest 0x14c6c-0x14cb4) | docs/111: the port gated the lookup on `player && state == 0x97`; `DAT_00051b60` transcribed for 0x20 of 0x400 bytes | player samples in 0x0b/0x0c: 0 -> 49 (shipped 47) |
| 7 | stuck when hit mid-kick | same state census | P14@86000 (`0x3 -> 0x79` vs `0x4b`) | same reaction path | docs/111 (entry) plus docs/137 stride-4 decoding `tbl[4*code]` | shipped 0x0b x16 / 0x0c x18 reproduced in the port; port's spurious `0x15` x168 traced to the population divergence (docs/137 §4) |
| 8 | fallen state barely happens | same census over `0x5b..0x66` | 0 samples on **both** sides for the combat drives | the `0x5b..0x66` band of `DAT_00051b60` | docs/118 decoded the real chain: the pick-up block at guest 0x00014de1+ fires on the **attacker's** state 0x65/0x99; episode 1 contains exactly one fallen-class record (type 0 / state 0x65 / slot 0 / atype 0x15) | open as a coverage question, not a missing transition: injection cannot force it because the controller rewrites `+0x17` every frame |
| 9 | enemy sometimes teleports | entity census / position continuity | all entity windows | none found | **rig defect**: docs/115 fixed three oracle bugs (record offsets must use the dump's own `offset`, cadence normalisation for 4 Hz vs 10 Hz, identity-linked transitions within 48 px) | after the fix: jumps 0.41 vs 0.43 per sample, unlinked appearances 5.9% vs 5.3%, hurt band 0.100 vs 0.091 -- equivalent |

## 1. The two misses were rig defects, and both became rig fixes

* **EXP (item 4)**: the first "no divergence" verdict came from `scripts/audit/port_src.py` failing to evaluate
  the array dimension `0x4ec/4`, falling back to one element and mis-scoping `g_tab_52a1c` (docs/121). After the
  parser fix the same route moved 23 record-level verdicts, and the record census then showed the shipped dying
  entity carrying `+0x32 = 10` where the port carried 0 (docs/108) -- the signal that led to docs/109.
* **Teleport (item 9)**: the rig "detected" teleportation before it could measure it. The three oracle defects
  fixed in docs/115 removed the effect entirely; the honest verdict is that the port is equivalent to the
  shipped game on jump rate and unlinked appearances, i.e. the playtest item was not reproduced by a corrected
  rig rather than confirmed by it.

## 2. What is still open, and where it sits

Item 3's *audible* ordering, item 8, and the population/position divergence that docs/144/145 localised
(player travels 603 px where the shipped player never passes 217, camera 298 vs 27, `px - cam > 0xbe` in 24%
against 0.6%) are one thread: the extra population is what puts hit code 11 on the player, and the player death
is what triggers the CONTINUE countdown and the extra CD selection (docs/135). docs/145 §5 and §7 removed two
candidate explanations (the `room[+0x16]` clamp, a missing state family), so the remaining question is the
per-frame displacement itself -- docs/144 §2 measured 60-69 px/s shipped against 70-79 px/s port with a
shipped-absent 80-89 band.
