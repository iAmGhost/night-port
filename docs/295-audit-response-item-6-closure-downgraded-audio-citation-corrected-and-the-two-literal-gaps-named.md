# 295 — Answering the completion audit: item 6's closure is downgraded to provisional, the audio citation is corrected, and the two literal blind-detection gaps get named, dated probes

The independent audit of the completion request verified the sweep numbers, the item-2a/2b/3/4 fixes and the
`LAB_0003284e` guard fix ✓ but rejected completion on four evidence-quality grounds ✓ This document answers each ✓

## 1. Objection 3 (stale audio citation) — FIXED

docs/294 §4 quoted `/tmp/rig/rb10`'s audio rows ✓ The final sweep's own run is `/tmp/rig/rb11` ✓ whose reports read:

```
report_audio_sfx.txt: port PCM 758125/758125 samples placed into  99/1499 record windows @10989 Hz
                      (69.0 s of sound in 227.9 s of route)
                      original 133.0 s @44100 -> 1331 buckets   onsets 9   active_frac 0.959  rms 1147.9
                      port     69.0 s @10989  -> 2028 buckets   onsets 59  active_frac 0.048  rms  314.9
report_audio_cd.txt:  original 1331 buckets  onsets  9  active_frac 0.959  rms 1147.9  zcr 0.064
                      port     5591 buckets  onsets  4  active_frac 0.991  rms 3442.6  zcr 0.056
                      P0@0  orig onsets=7 active=0.89 | port onsets=4 active=0.93   [SAME]
```

The attribution is unchanged (CD-DA stream matches in activity fraction and onset order ✓ the PCM/SFX row keeps its
long-standing per-window accounting attribution ✓ docs/129-133, 167 family ✓) — but the numbers in docs/294 §4 are
superseded by the rows above ✓ and docs/294 carries an erratum pointer ✓

## 2. Objection 1 (item 6 is not cleanly closed) — CONCEDED, status downgraded

The audit is right on both counts ✓ and docs/293/294 overstated the closure ✓

* The blind detection recorded for item 6 (docs/225's eight `+0x1b` stores ✓ docs/290's flip census ✓) was **retracted**
  by docs/291 §1 and docs/292 §1 ✓ — so item 6 was never blind-detected *as a wrong-side limb* ✓
* The only defect actually located and fixed in item 6's neighbourhood is **vertical** (`LAB_0003284e`'s entry
  guard ✓ docs/291 §3 ✓) ✓ not side-of-limb ✓
* The both-sides window that docs/293 §1 used is a **general room-play** window ✓, not the reported
  left-edge-attack scenario ✓

**New measurement on the scenario-matched drive** (`/tmp/rig/le5` = `ep1_leftedge2.txt`, 220 s ✓ with the fixed
`capture_original.py` ✓ and `NIGHT_RIG_SCREEN_EVERY=2` ✓ giving 2206 port `320x200` witnesses ✓ and 714 shipped
frames ✓):

```
left-bound AND attack state (X <= -22, state 0xb/0xc):  shipped 0 samples ✗   port 0 samples ✗
left-bound (X <= -22):                                  shipped 27 samples, states {0:25, 1:1, 2:1} ✓
                                                        port   275 samples, states {0:257, 1:17, 2:1} ✓
                                                        both min X = -30 ✓✓  both max X differ (303 / 165) ✗
attack states anywhere:                                  shipped 55 ✓   port 164 ✓
```

So the reported scenario — *attacking while pinned at the far left edge* — **does not isolate on either side** ✓:
neither build ever holds an attack state at the bound on this drive ✓ (left presses and attack presses do not
coincide at the bound in either build ✓) ✓ That is a symmetric finding ✓ but it means the rig has never observed
the reported situation on either side ✓ and no screen claim about it is admissible ✓

Room-anchored column census (`DAT_00053e9c >= 1` as each side's own room-entry anchor ✓ shipped 76 frames ✓ port
472 bins ✓ each against its own modal picture ✓ ground rows 150-200 ✓):

```
L/R around each frame's own activity centroid:  shipped 1.01 ✓   port 1.04 ✓  (no side flip ✓)
40-px column shares %:  shipped {0:11.4, 40:16.8, 80:15.7, 120:6.1, 160:11.4, 200:16.8, 240:15.7, 280:6.1}
                        port    {0:26.8, 40:22.1, 80: 8.8, 120:9.6, 160:11.7, 200: 6.4, 240: 6.4, 280:8.2}
                        left half (cols<120): shipped 28.2 % vs port 48.9 % ✗
```

Interpretation stated carefully ✓: the centroid-relative asymmetry agrees (1.01 vs 1.04 ✓ — nothing is drawn
systematically on the opposite side ✓) while the *absolute* column distribution differs (28.2 % vs 48.9 % ✓) ✓ and
that second measure is dominated by how much walking/scrolling each side packed into its 45 s window (76 vs 472
frames ✓ different phase content ✓) ✓ **It is therefore not item-6-decisive in either direction** ✓

**Item 6 status: provisional — no located wrong-side defect, closure not claimed.** The decisive instrument, still
to be built, is a **limb-record footprint test**: dump the port's actual destination rect for each limb/parts
composite (`blit_probe_note` ✓ `src/app/rig_sink.c`-independent, `src/f/f.c:3212` ✓ accumulating
`(x0..x1, y0..y1)` from the composite routine `fn_03bb42` ✓ `f.c:7733` ✓) and compare it against the rect the
shipped's own arithmetic computes from the shipped's own record values (`0x13dc1-0x13ddb` / `0x13ee6-0x13f00` ✓
with the record bytes read from the shipped's heap dump ✓), fed the *same* record values ✓ A match closes item 6 as
"not producible by the port"; a mismatch localizes it to a named blit site ✓ Neither result depends on cadence
✓, capture geometry ✓ or colour convention ✓

## 3. Objection 2 (items 1 and 5 were not literally blind-detected) — CONCEDED, with the probe now available

* Item 1: the blind verdict was withdrawn (docs/234 ✓ `--rig-dump` timing artifact) and the item closed as "no
  surviving defect" (docs/235 ✓) ✓ The rig therefore **never blind-detected** item 1's symptom ✓
* Item 5: the crash half was recorded as NOT REPRODUCIBLE (docs/226 gap 3 ✓), and docs/287 later showed the
  symptom was a drive gap (two different key gates ✓) ✓ Again: not a blind detection of a defect ✓
* What the audit called the open question — "a crash may need a configuration the rig has not used" — now has a
  concrete, dated probe: run the **frozen unfixed build** (`574c636`, the build the blind gate was measured
  against ✓) on `scripts/rig/drives/ep1_roomclear3.txt` ✓ — a drive that **did not exist during the blind gate** ✓
  (it was authored in docs/286) ✓ — under ASan/UBSan with the floor census armed ✓ If the frozen build faults or
  renders a black floor on that route ✓ item 5 becomes a genuine blind detection on the unfixed build with a
  before/after pair against HEAD ✓; if it does not ✓ then the report's crash is unreproducible even on the build
  that was reported ✓ and that is recorded as a limit of the objective rather than as a pass ✓

## 4. Objection 4 (item 2b's player-side EXP is only half-instrumented) — CONCEDED, probe started and failed informatively

docs/284 §4 is right: the *award* is verified against shipped memory ✓, the player's accumulated EXP is not ✓
First probe: `fn_0137c1` (the HUD number drawer ✓ `P. 00000` ✓) reads `DS 0x63e04` four times in its format block
(`0x13821 ✓, 0x1382a ✓, 0x1387f ✓, 0x13889 ✓`), so `0x63e04` was the candidate cell ✓ Census on two routes:

```
/tmp/rig/rb11  DAT_63e04: shipped constant 2961284 (0 transitions ✗)   port constant 1377420 (0 transitions ✗)
/tmp/rig/le5   DAT_63e04: shipped constant 2961284 ✗                   port constant 1377420 ✗
```

A value that never changes and differs per host is **not a counter** ✓ (host-dependent, same family as the
`host_address_space` non-gameplay rows ✓) ✓ so `0x63e04` is the wrong cell ✓ The instrument must instead follow the
award's *destination*: the shipped's award block `0x15060-0x15070` (`imul eax,[eax+0xd],0x88; mov edx,[eax+0x3ea8]` ✓)
writes its result into a specific accumulator — that store's target cell is the field to instrument ✓ and it is the
next static step ✓ (`scripts/audit/field_writers.py` on the destination displacement of the award's `add`/`mov` ✓
then dump that cell on both sides on a drive that produces a settled kill ✓ docs/282's pulse-shaped drive does ✓) ✓

## 5. What is not claimed any more

* Item 6 is **not** claimed closed ✓ (docs/293 §2, docs/294 §row 6 are superseded by §2 above ✓)
* "The rig independently detected all six" is **not** claimed ✓ — it detected and localized four ✓ (2a, 2b, 3, 4) ✓,
  withdrew one detection (1 ✓) ✓ and found item 5's symptom to be drive-dependent rather than a defect ✓
* Completion will not be requested again until §2's footprint test or §3's frozen-build probe has produced a
  result ✓ and §4's accumulator cell has been identified ✓
