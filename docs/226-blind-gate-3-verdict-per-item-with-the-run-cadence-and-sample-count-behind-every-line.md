# 226 — Blind gate 3: verdict per item on the unfixed build, with the run, cadence and sample count behind every line

Gate contract: the rig must, with **no human playtest input**, detect and localize the six items of
`docs/manual-playtest/03-2026-10-29.md` on the unfixed build, and say which items needed a new oracle.
Frozen build under test: `574c636` (docs/182; the binaries `port/test/night`, `port/test/night-asan` are
git-tracked, so the baseline commit's own binaries were re-run in `/tmp/base574`).  Canonical port env:
`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`; canonical route runner `scripts/rig/route.sh`.
Docs/94, 96, 98, 99 §2 were treated as hypotheses; each is marked confirmed or superseded below.

## Verdict table

| # | item | verdict | new oracle needed | doc |
|---|---|---|---|---|
| 1 | intro music never switches to title | **DETECTED-AND-LOCALIZED** | `res` marker stream + track-request witness | 183, 184, 185, 187, 188 |
| 2a | kill EXP text garbled | **DETECTED-AND-LOCALIZED** | glyph witness + blit-bbox probe | 210, 211, 214, 215, 218, 219, 221 |
| 2b | EXP gain per enemy type | **DETECTED-AND-LOCALIZED** | record-sampling oracle (`exp_award.py`) | 186-190, 199, 203 |
| 3 | upgrade menu lights every row | **DETECTED-AND-LOCALIZED** | `NIGHT_LUMENU` witness + time-paired screen oracle | 206, 207 |
| 4 | level-up intro auto-advances | **DETECTED-AND-LOCALIZED** | picture-identity census over a key-free window | 209, 217, 224 |
| 5 | stage-2 crash + black floor | screen half **DETECTED-AND-LOCALIZED**, crash half **NOT REPRODUCIBLE** | mode-segment census (`floor_band.py`) | 208, 222, 223, 224 |
| 6 | limb drawn on the wrong side at the left edge | **DETECTED-AND-LOCALIZED** | `NIGHT_ENTGEOM` anchor witness + `field_writers.py` census | 212, 213, 214, 220, 225 |

Six of seven sub-items were detected *and* localized to a named site from the rig alone.  Item 5's crash claim
is the one claim the rig cannot reproduce; it is recorded as a negative with its five route/build combinations.

## Item 1 — music never switches

* Route `OUT=/tmp/rig/g36`, `route.sh scripts/rig/drives/ep1_title_music.txt 180`.  Shipped 4 Hz, 582 states,
  t = 25..279 795 ms; port 10 Hz, 1 800 samples, t = 16..239 850 ms (docs/183 §2-3).
* Measurement: the port holds track 1's 42 s request for 166.7 s and never requests the title track LBA
  `0x9ddf` (40 415) in 240 s; the shipped re-requests at each track end (docs/183).
* Mechanism (docs/184): the shipped bounds play by `DS 0x63e38 / 490 > DS 0x53c48`, and the selection routine
  `fn_010d47` has exactly one caller, `fn_03a829`.  Title track choice comes from the room table
  `DS 0x52da4[room[+0x40] + (stage-1)*10]` (docs/185) — the port's 100-entry table is byte-identical to
  NIGHT.EXE, so the data is not the defect.
* Localization: the port never runs `fn_03a829 → fn_010d47` while the title/menu presentation blocks inside one
  game step (a 65 s hole in the presentation/frame loop); the 53/54 values first read as track ids are state
  ids (docs/188 supersedes docs/187's queue-value reading, which superseded docs/186's constant reading).
* Hypothesis status: docs/94 §"music selection table" **superseded** (the table is right); the defect is the
  call path, not the table.

## Item 2a — the EXP label blit

* Route `/tmp/rig/gt3`, drive `ep1_kill3.txt`, unpinned clock (the popup only posts unpinned), 300 s, 2 145
  samples at 10 Hz, popup active in 36 samples, t = 242 837..288 793 ms (docs/221 §4).
* Measurement: `fn_03b718`'s tail (the "EXP" label) reads glyph offset, `col` and `rows` from the resource's
  transposed parallel arrays at `+0x2c` (dword index 11) where the shipped reads `+0x28` (index 10).
  `fn_03de85` sizes each array at `count*4 = 0x2c` for `system.spr`'s 11 records, so index 11 is **one dword
  past the allocation**: the port reads `col=49, rows=49, glyph=0` (the neighbour dword 0x31) instead of record
  10's `col=13, rows=6, glyph=0x1a7`.  The probe measures 147 px written into a 4×49 box (x 194..197,
  y 95..145) — a vertical pile of digit fragments where a 13×6 horizontal label belongs ✓ the garble.
* Sibling audit: `fn_03ace9` correctly uses `+0x2c`/`+0x30` for `system2.spr`'s 13-entry arrays, so the bug is
  local to `fn_03b718` (fix target `f.c:14596-14598`).
* Supporting closures: the digit arithmetic is faithful (docs/215 §3), the font metrics are the table values
  (font A 4×7, font B 10×12, docs/219), the shipped *does* present the number at `dest = i*5 + rec[+0x09]`,
  `start_row = 113-age-15` (docs/218), and the port's glyph streams resolve to real `system.spr`/`exp.spr`
  offsets (docs/219) — so "wrong font", "wrong arithmetic" and "invisible popup" are all disproved.
* Hypothesis status: docs/96 (kill-popup rendering) **confirmed in shape, superseded in cause**; docs/216's
  copy-direction claim was retracted by docs/218 (stride 2560 B/row, pitch 0xa00, not 1280).

## Item 2b — per-type EXP award

* Run `/tmp/rig/ents-orig` + `/tmp/rig/ents-port`, drive `ep1_kill3.txt`, 10 Hz, window t = [150 000, 300 000] ms:
  shipped 974 states / 13 267 admissible record samples, port 1 292 samples / 10 068 record samples (docs/199).
* Measurement: the same record class (`kind=1, type_idx=31, state=0x15, hp=0`) carries award `+0x32 = 10` in the
  shipped (31 dying samples) and `0` in the port (38).  Source: `DS 0x53e9c` row 31 = `(35, 2, 0, 10)` read in
  972/974 states.  The port never executes the table write (0×) and never sets `+0x30 = 1` on a kind-1 record
  (`settlecheck` at `f.c:25262` / shipped 0x15003: 60 samples, every one `+0x30 = 0`) (docs/199).
* Census behind it: `fn_015295` 1 108/1 396 gate rejections at `+0x30 != 1` (docs/190); no per-type EXP table
  exists other than `DS 0x53e9c` (docs/189, `field_writers.py`); the "54" in `+0x32` is a state id, not an
  amount (docs/188 superseding docs/187 and docs/186).
* Upstream blocker named: the player's damage **debt** bookkeeping — shipped debt > 0 in 201/447 samples vs
  port 60/447, settle flag 216 vs 357, and the shipped's damage stage gates on `debt == 0` (0x148ea-0x148f0),
  which is why the port's victims reach settle with `+0x30 = 0` and why the canonical route produces zero kills
  on both sides (docs/203, docs/197).

## Item 3 — upgrade menu lights every row

* Static half (docs/206): shipped reads the selection state per row `sel[i]`, the port reads `sel[page]`; over
  the run's 196 menu frames (1 176 witnessed rows) 338 rows exceed `selMax`, and the port blits the "selected"
  variant on all six rows where the shipped lights one.
* Screen half (docs/207): 28 of 42 blit cases land on rows whose shipped `sel[i] = 0`; bar rows y = 111/131/151/170
  with sprite offsets 31/77; ink counts 928 / 530 / 852 / 963 non-modal pixels in `/tmp/rig/l5/frames/005669_320x200.bin`.
* New oracle: `scripts/rig/screen_time.py` (time-paired screen comparison, mean score 0.7506) replaced the
  ±90-frame dynamic-programming matcher in `diff_screen.py`, which saturated (docs/205).

## Item 4 — the level-up intro advances by itself

* Route `k1` = `route.sh scripts/rig/drives/ep1_keywait.txt 140`, port dumped at 10 Hz, shipped state replay at
  4 Hz, shipped screen capture at 25 Hz (docs/217 §1).
* Key-free window census: shipped holds one picture for 18 states = **4.5 s** (35.2-50.0 s window, 43 samples,
  26 distinct pictures, 1.73 new/s) and 30 states = **7.5 s** (50.0-80.0 s, 84 samples, 26 distinct, 0.87 new/s);
  the port changes picture every ~0.17 s (5.64-5.94 new pictures/s).
* What it is *not*: the cursor does not self-advance — page 0 for all 1 726 menu frames of `/tmp/rig/l6`, 0 page
  changes with no keys (docs/209).  `help8.pcx` is held 97 frames while a key is down and replaced at key
  **release** (t = 100 617 ms, 600 ms after load) (docs/209).
* Localization: the card's wait condition — `fn_035f1b("help8.pcx", 1)` at `f.c:25750` returns where the shipped
  keeps waiting (`fn_038f3d`), with the `up_ctr`/`down_ctr` release drain at `f.c:25783-25797` /
  0x396a0-0x39767 (docs/209, docs/217).  The same site is what makes the port stall after stage completion
  (docs/224 §4), so items 4 and 5 share one blocking site.

## Item 5 — stage 2: crash and floor

* Natural transition, `/tmp/rig/rc2`, drive `ep1_roomclear2.txt` (poke `DS 0x6336c = 3`, the one value that makes
  `fn_03a829` return 1 at 0x0003abca-0x0003abdd, plus held Enter so the end slide show advances), 13 200 frames,
  2 179 samples at 10 Hz: `endcard: slide 1 wait`, `res 1e1.pcx / help1.pcx / help8.pcx / levelup2.pcx`
  (`fn_038f3d(1)` runs), and `DS 0x63370: 1 -> 2` at t = 94 316 ms with no stage-cell poke (docs/224 §3).
* Screen oracle: `frames.bin` mode segments show **zero 320x200 entries after t = 93.3 s** — 15 305 entries in
  640x480 for the remaining 127 s / 7 600 frames (1 771 changed), the last three witness frames (215.7, 216.4,
  217.1 s) byte-identical per-row ink profiles, modal `(255,247,247)`; and no `stage2.wal / stage2.stg /
  night2.chr / 2s1.pcx / 2s2.pcx / stage2.pcx` lookup, while `--warp-stage 2` requests all six and renders a
  floor (bottom-half ink 22 270 / 22 515 px, 100/100 rows with ink, 0 black rows) (docs/224 §3-4,
  `scripts/rig/floor_band.py`).
* Baseline reproduction: `/tmp/base574` (`574c636`) with the same drive locks 640x480 from t = 90.3-266.0 s
  (19 137 entries, 1 803 changed) after stage 2 at frame 4768 ✓ the defect is in the unfixed build, not in the drive.
* Crash half: warp sweep of stages 2-9 on `night-asan` (`/tmp/rig/sweep`, `/tmp/rig/sweep2`, 3 000 and 6 600
  frames) all rc=0 with zero AddressSanitizer/SIGSEGV; docs/208's stage-cell poke (13 435 frames at stage=2)
  rc=0; docs/222's quota-poke route rc=0; baseline warp and baseline room clear rc=0.  docs/52's stage-5
  SIGSEGV no longer reproduces.  The historical `bus error ./test/night` line is the harness's own frame-budget
  `longjmp` ("frame budget: … unwinding the game main") (docs/223 §2).
* The only sanitizer output in any of these runs is UBSan `f.c:7360` (`toks[cnt1-1] = NULL` in `fn_04589e`, the
  widened token array at `tail = buf + nbytes`, unaligned when `nbytes` is), and it fires on routes that never
  touch a stage transition — the counterfactual that separates it from item 5 (docs/224 §6).

## Item 6 — limb on the wrong side at the left edge

* Runtime anchor census `/tmp/rig/le6.log` (`NIGHT_ENTGEOM`, drive `ep1_leftedge.txt`), 11 976 composites:
  `arm=forward` 3 712 (all `face=0`, `dest == worldx - camx - X` in 3 712/3 712), `arm=mirror` 8 264 (all
  `face=1`, `dest == worldx - camx - (W - X)` in 8 264/8 264); left-edge player subset
  (`worldx <= 20, camx = 0`): `spr=7 W=27 X=10` x6 face=0 and `spr=9 W=30 X=14` x4 face=1 (docs/220, docs/225).
* Static: both arms of `fn_012efa` (0x000133a4-0x000133bd, 0x0001344b-0x00013479) and the row anchor
  (`EBX = [ent+5] + [ent+9] - Y[i] - DAT_000639e0`, 0x0001337d-0x0001339e) are byte-faithful; `fn_03bebb`'s
  forward and reverse setups and all three clip arms match `f.c:7581-7665` (docs/225 §1-2).
* The divergence is the **selector**: `field_writers.py --disp 0x1b` finds 28 shipped functions touching
  `[ent+0x1b]`; the port models 27 of them but eight stores are absent — `fn_03e20f`'s six command-path mirror
  writes (0x3e361, 0x3e483, 0x3e58f, 0x3e6bc, 0x3e804, 0x3e932; only the spawn-path store 0x0003e2f6 is
  modelled, `f.c:26926`) and `fn_03272f`'s pair (0x328ff `=1`, 0x32923 `=0`).  Consequence: the port takes arm B
  (`fn_03bb42` forward, `f.c:14924`) where the shipped takes arm C (mirror dest + reverse runs, `f.c:14926`), a
  `W - 2X` displacement (14 px for W=42/X=14, 15 px for W=41/X=13, 7 px for the player's W=27/X=10 at the left
  edge) with the run reversal dropped (docs/225 §3).
* Closed instead: the bail-threshold (`0x13f` vs `0x140`) and mutually-exclusive-clip candidates are provably
  equivalent (docs/225 §2), so docs/212 §4's clamp hypothesis is superseded, and docs/220 §1-2 closes the anchor
  hypothesis.

## New oracles built for this gate

| oracle | closes |
|---|---|
| `scripts/rig/screen_time.py` | time-paired screen comparison (`diff_screen.py`'s DP saturated) — items 3, 5 |
| `scripts/rig/floor_band.py` | per-row ink/black profile of presented frames — item 5's floor claim |
| `scripts/rig/exp_award.py` | record-sampling award oracle (`+0x32` on dying records) — item 2b |
| `scripts/rig/debt_trace.py` | player debt/settle series — item 2b's upstream cause |
| `scripts/audit/field_writers.py` | writes to a displacement in any addressing form — items 2b, 6 |
| `scripts/audit/imm_scan.py`, `branch_xrefs.py`, `uncalled_port_fns.py` | reachability + immediate census — items 1, 5 |
| `NIGHT_ENTGEOM` witness (`entsprgeom` lines) | per-blit anchor basis on both arms — item 6 |
| `NIGHT_GLYPHDBG` / `NIGHT_LUMENU` / `NIGHT_TYPETABLE` / `NIGHT_SET30` / `NIGHT_SLIDEDBG` witnesses | glyph and row selection, award gates, slide shows — items 2a, 2b, 3, 4, 5 |
| drives `ep1_title_music`, `ep1_keywait`, `ep1_lu_hold`, `ep1_lu_confirm`, `ep1_leftedge`, `ep1_roomwalk[2]`, `ep1_roomclear[2]`, `ep1_stage2` | a route per item |
| `/tmp/base574` worktree of the frozen build | baseline-vs-HEAD attribution for item 5 |

## Gaps the fix phase inherits

1. No shipped-side *pixel* basis: the shipped dump and the port dump do not share a presentation basis
   (docs/202's dither finding was itself retracted by docs/204 as a misparse; docs/205 shows 8 shipped mode
   segments vs 4 port segments), so cross-side screen claims rest on the port's own witnesses plus shipped state
   replays.  Item 6's contract wording ("both sides") is met on code, not on pixels.
2. No kill-bearing route: `ep1_kill3` yields zero kills on both sides (kill counter `DS 0x639bc` = 0 in 894
   shipped states and 2 257 port samples, docs/197/g37) because of the debt bookkeeping gap (docs/203).  Items
   2a/2b red-before/green-after evidence must therefore come from a fixed-kill route or from the unpinned popup
   route (`/tmp/rig/gt3`).
3. Item 5's crash claim stays unmeasured: five route/build combinations say the current and frozen builds do
   not fault.  If a crash exists it needs a configuration the rig has not used.
4. Standing host-side UB at `f.c:7360` (unaligned widened pointer store) is the only sanitizer output seen; it
   is not one of the six items but is the first fix candidate for the regression-cleanliness requirement.
