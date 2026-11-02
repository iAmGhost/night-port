# 305 — status refresh on all six 03-2026-10-29 items: what the rig detected blind, what was wrong, what was fixed, and which sweeps are green on the committed build

Date: 2026-11-02.  Supersedes the status tables in docs/294 and docs/295 for items 1 and 6, and records the
two corrections listed in §6.  HEAD after this commit carries the fixes for items 1 and 6; earlier fixes for
items 2-4 are unchanged.

## 1. Per-item verdict with the evidence path

| # | item | blind detection (unfixed build) | root cause | fix | before/after rig evidence |
|---|---|---|---|---|---|
| 1 | intro music never switches | docs/183 (g36: shipped selects 0x2d320→0x367aa→0x9ddf and holds; port never selected 0x9ddf) → **superseded** by docs/234: that red reading was a `--rig-dump` artifact; on a clean basis the port *does* select the title and room tracks | two separate things: (a) none in the selection layer ✓; (b) the **CD-DA pump had no elapsed-time term** — it asked for a fixed 30 sectors on every pump call and the pump runs ~100×/frame (docs/302 §3) | `g_cdda_budget` accumulates elapsed sectors, reset at PLAY, one 30-sector cushion (`port/src/app/main.c`, docs/302 §5) | unfixed `/tmp/rig/le8` 53 311 sectors = 710.8 s audio in 195.4 s wall = **3.64×**; fixed `/tmp/rig/le10` **1.07×**; canonical `/tmp/rig/rb15` 11 680 sectors in 150 s = **1.04×** ✓. Request schedule unchanged and matching the shipped (`index1 37262 → index9 185120 → index11 223146`, docs/299) ✓ |
| 2 | EXP text garbled; EXP per enemy type | docs/186/187/199: award field +0x32 = 10 shipped vs 0 port, 13 267/10 068 record samples | (a) the label blit read array index 11 instead of 10 (docs/221); (b) the per-type award is the first dword of the shipped's entity-type table row (docs/284) | docs/227 (index 10) and docs/284 (award == type-table row dword) | docs/227's blit-footprint probe (label-box pixel census is inadmissible, docs/227 §5); docs/284's per-type award census matches the shipped row for row ✓ |
| 3 | upgrade menu lights every item | docs/206/207 (drive `ep1_lumenu` + cell pokes `play 0x27 1` / `play 0x28 2`): shipped 0x3985b-0x39875 reads the two locals from `sel[i]`, port read `sel[page]` | cursor-indexed reads replaced by a page-indexed read | docs/228 (row loop takes cursor rows/purchase count) | `route.sh` before/after on `ep1_lumenu`: every item lit → only the purchased row lit ✓, with the other cursor-indexed reads in the same routine adjusted together (docs/228 §4) |
| 4 | level-up intro and "EXP not enough" advance with no keypress | docs/217 (route `k1`, `ep1_keywait`, 140 s, canonical env): shipped holds one picture 18 states (4.5 s) and 30 states (7.5 s) with no key; the port churned | the card key wait `fn_00035d5d` was a no-op stand-in | docs/231 (real key gate, `fn_00035dfc` accepts Enter/CTRL/ALT/Space, docs/97) | docs/217 window census before/after ✓ |
| 5 | stage 2 crashes (`bus error ./test/night`), floor renders black | docs/222 → docs/223 → docs/224 → docs/285 → docs/286: the natural room-clear chain reaches stage 2 and loads stage 2 ✓ | **no stage-2 defect exists.** The old symptom was a drive gap (docs/286) and a *startup* misaligned `char *` store in `fn_04589e` (docs/265), not stage 2 (docs/299 §item 5) | docs/265 fixes the misaligned store; the black-floor claim is superseded: stage-2 floor bands carry room art (docs/287) | docs/286/287 (stage-2 composite + ink presence ✓); ASan/UBSan: no stage-2-specific fault on three routes (docs/223) and 9/9 clean soak now ✓ |
| 6 | attacking at the far left edge draws the limb on the right side | docs/301: artifact confirmed in the rig's own witness frames (`/tmp/rig/le8` t=104100 ✓ absent mid-room ✓ absent in the shipped frame ✓); docs/298's "not producible" closure voided | `fn_03bb42`'s mirrored width-overflow clip anchored its run on `db - col + 1` where the shipped (`0x3be36-0x3be5c`) uses **`db - col_acc`** with only the count shortened; `cpy_back` (0x040475) writes *downward* from its first argument, so with `dest_off < 0` the tail landed on the previous row's cols 319, 318 … (docs/303 §2-3) | `cpy_back(video_map(db - col_acc), data, cnt)` (`port/src/f/f.c`) + guard aligned to shipped `cmp 0x140/jg` (docs/303 §8, provably inert) | witness `NIGHT_BLITEDGE=1`, same drive `ep1_leftedge4 220`: `/tmp/rig/le12` **~43 000 crossings (486 logged, every one site=5)** → `/tmp/rig/le14` **0**, with the phase matched (556 vs 559 samples at X∈{15,16,17} & mirror=1; activity 271 SAME in both) ✓. Fixture red/green by reverting only that line: `== FAIL (2) ==` → `== ALL PASS ==` ✓ |

## 2. Sweeps on the committed build

* `make -C port test`: `== ALL PASS ==` ✓ including the two new docs/303 fixtures.
* Nine-episode ASan/UBSan soak, built from the committed source: `/tmp/rig/soak303b` — stages 1-9 all
  `rc=0 ubsan=0 asan=0 segv=0`, `frames presented: 4000` each ✓ (`/tmp/rig/soak303`, same drive, binary built
  one commit earlier, was also 9/9 ✓).
* Coverage-guided fuzz census: `/tmp/rig/fuzz303` — 9 episodes, coverage 246/503 shipped-named functions,
  **0 crashes**, every seed `rc=0` ✓; a deeper 24-seed run is in `/tmp/rig/fuzz305`.
* Canonical route `/tmp/rig/rb15` (`ep1_kill3.txt`, 150 s, `PORT_ENV` with pinned clock):
  `tally SAME 70 / DIFF 27 / REC-SAME 13 / REC-DIFF 34` — identical to `/tmp/rig/rb13` (`SAME 70 / DIFF 27`)
  and within the pre-fix run-to-run band (`rb11`: `SAME 71 / DIFF 26`) ✓; 271 activity-profile cells `SAME` ✓;
  channel-order 0.973 declared = R/B-swapped ✓; pacing 2.67 orig vs 20.18 port = 0.13, the long-standing
  value on this route (docs/244) — **unchanged by both fixes** ✓.
* Audio trigger-frequency basis: `rb15` onset census reports `[SAME]` in all 274 phase windows ✓.

## 3. Item 1's remaining limit, stated plainly

docs/304: the shipped side's audio witness is the container's mixer capture
(`scripts/rig/replay_original.py:204-263`), and **it contains none of the disc's CD-DA samples** — NCC of the
disc's own bytes at the requested LBAs against that capture is 0.116 / 0.073 / 0.214 over a 130 s search, where
a genuine match scores ~1 by the tool's own basis ✓ Meanwhile `TG_NIGHT.IMG` *does* hold Red Book audio at
those LBAs (0 sync-headed sectors of 3153/4000/3000, rms 2 975-3 490, |max| 24-29 k ✓) while the data region is
100 % sync-headed ✓, and the port's stream is byte-identical to it ✓ So:

* the port's music content is verified against the **disc** (the ground truth the shipped would read) ✓;
* a port-vs-shipped *music content* comparison is impossible on this harness, because the shipped side renders
  no CD-DA at all (the CUE gives tracks 02-15 index offsets and no data source ✓);
* the previously quoted `corr=0.22-0.32 [NOT FOUND in the original mix]` lines are this instrument's **floor**
  against this capture, not evidence about the port ✓

## 4. Disposition of the four hypothesis docs

* docs/94 (two-way combat box frame index and gauge pointer): **confirmed faithful, not a defect** — the box
  arithmetic and the facing/mirror arm selection are byte-faithful (docs/255 §2), and item 6's defect was in the
  blit clip, not in the anchor arithmetic (docs/303).
* docs/96 (CDDA audio LBA offset): **confirmed** — the 150-sector offset is right (`cd_iso_read_audio(37262)` →
  IMG sector 37112 ✓ fixture), and item 1's defect was the stream *rate* (docs/302), not the offset.
* docs/98 (SFX board bringup and slot aliases): **confirmed** — the 68-byte sound-slot table aliasing was real and
  was fixed earlier (docs/176).
* docs/99 §2 (kill-quota image data and state items): **superseded** — the 0x32/53-54 values are state ids, not
  EXP (docs/188), and the per-type award is the entity-type table row's first dword (docs/284).

## 5. What is still open (honest list)

1. Item 1 content gating needs a shipped side that actually plays CD-DA (docs/304 §5: a cue whose audio tracks
   carry `FILE ... <mm:ss:ff>` offsets, then re-capture and re-run `audio_ncc.py`).
2. Item 2's *audible* garble (the vertical-stacking symptom the user reported) is closed at the blit level
   (docs/221/227) but has no pixel-level two-sided gate: label-box pixel census is dominated by room art
   (docs/227 §5).
3. `--warp-stage N` in `port/src/app/main.c` takes a *stage* index, so "episode 1 stage 2" is not expressible
   from the flag; the natural chain (`scripts/rig/drives/ep1_roomclear3.txt`) is required (docs/285/286).
4. The pacing residual (0.13 on `ep1_kill3`, 0.55-0.59 on the left-edge drive) is unchanged and still the
   largest un-attributed difference between the two sides.

## 6. Corrections recorded this turn

* **Retracted**: "the item-6 fix cut changed frames 5311 → 898".  The 898 came from `/tmp/rig/le13`, which never
  entered a room (player X = -30 in 975/975 samples, mirror 0) — a void run (docs/303 §6).  Matched runs differ
  5311 vs 5260 ✓
* **Retracted**: the `report_audio_cd.txt` "NOT FOUND in the original mix / corr 0.22-0.32" lines as evidence of a
  content defect (docs/304 §2) ✓
* `route.sh` no longer cats a `report_audio.txt` that no leg writes; it now includes the two audio reports that
  exist (docs/287 §4 closed ✓).
