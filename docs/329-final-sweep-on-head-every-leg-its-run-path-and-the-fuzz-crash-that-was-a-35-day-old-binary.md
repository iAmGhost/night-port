# 329 — Final sweep on HEAD 4b6fcf5: every leg, its run path, and the one crash that turned out to be a 35-day-old binary

Goal task-24.  HEAD at sweep start: `4b6fcf5` (item H's arm replaced, docs/328).  Every leg below names the
command and the directory that holds its artifacts.

## 1. Unit suite

`make -C port test` -> `== ALL PASS ==` (rc=0), and the target rebuilds `port/test/night` (mtime Nov 4 16:29,
after the last source change) — the docs/181 §5 rule still holds.  Last witnesses printed: the `cd_iso` disc
block (15 tracks, the 0xc51 gate, `cd_iso_read_audio(37262)` -> IMG sector 37112) and the `--skip-intro` block
(neither `fn_038a20` nor `fn_038ae0` runs, both VESA mode sets stay, the CD still asks for track 1).

## 2. Canonical route, tallies against the frozen shipped half

```
OUT=/tmp/rig/rb330 REUSE_ORIG=/tmp/rig/rb15 scripts/rig/route.sh ep1_kill3 150      -> rc=0
port env: SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
samples: orig n=412 t=[47236..284318] | port n=1500 t=[16..235683]
```

`REUSE_ORIGIN.txt` stamps that the shipped halves were reused (no container booted), so this leg re-verifies the
**port** half only ✓ (REUSE_ORIG cannot re-verify the shipped side, docs/318 rule).

Class census, HEAD (`rb330`) against the frozen reference (`rb15`), same 346 cells / 274 phases:

| class | rb15 | rb330 |
|---|---|---|
| SAME | 70 | 71 |
| DIFF | 27 | 26 |
| REC-SAME / REC-DIFF / REC-NULL | 13 / 34 / 14 | 16 / 35 / 10 |
| **DIFF-SIZE** | **14** | **14** |
| **DIFF-WIDTH** | **6** | **6** |
| **PTR** | **52** | **52** |
| SIZE? / REPR / ENTROPY | 34 / 19 / 14 | 34 / 19 / 14 |
| NON-GAMEPLAY: device_plumbing | 27 | 27 |
| NON-GAMEPLAY: dos_extender / flat_pointer_slot / arena_state / clock_stamp / host_address_space | 10 / 4 / 3 / 3 / 2 | 10 / 4 / 3 / 3 / 2 |

The residual set is identified and unchanged in every structural class; the only movement is inside the
state-identity buckets (`SAME`, `DIFF`, `REC-*`), i.e. phase movement inside the rb15 baseline — the same reading
as docs/320 §3, and the same 27 `device_plumbing` residuals.  Activity profiles: 271 cells, all `SAME` ✓
divergent screen runs (score < 0.88): 8 ✓ channel-order check `declared=0.973 R/B-swapped=0.973 (chroma 24.6) ok`
✓ pacing `orig=2.67 port=23.34 ratio=0.11` — the standing pacing residual (docs/168, docs/269), not a new class.
Shipped-quiet windows: 71 of 274, 32 of them with the other side moving a non-entropy cell (docs/178 family).

## 3. ASan / UBSan nine-map soak

```
scripts/rig/asan_soak.sh      -> rc=0, log /tmp/rig/soak330.log, artifacts /tmp/rig/soak
stage 1 rc=0 ubsan=0 asan=0 segv=0 frames presented 1052     stage 6  ... 1114
stage 2 rc=0 ... 1050   stage 3 ... 1056   stage 4 ... 1045   stage 5 ... 1115
stage 7 rc=0 ... 1114   stage 8 ... 4000   stage 9 ... 1113
```

9/9 rc=0, zero sanitizer reports ✓ The script builds `make -C port asanapp` first (docs/265's stale-soak fix);
the binary was rebuilt again during §5's investigation (`night-asan`, Nov 4 17:52).

## 4. Blit census

```
NIGHT_BLITDBG=1 NIGHT_BLITEDGE=1 ... --script ep1_kill3 --frames 6000   -> /tmp/rig/bl330
blt_rows calls=2515 out_of_band=0  fn_03ce0b arms B0=2515 B1=0 B2=0 B3=0
blitedge census: spans_inspected=933777 inside_one_row=933777 crossings=0
```

0 out-of-band row blits and 0 row crossings, with the number of inspected spans printed (docs/312: a crossing
count without a denominator is not a result).  Basis limit stated: on this route `fn_03ce0b` took the B0 arm only,
so the crossing census covers the un-offset spans; the offset arms (docs/311's fix, B2/B3) ran on the ep1 chain
basis recorded in docs/312 and are not re-gated here.

## 5. Fuzz census, and the crash that was a stale binary

| census | args | coverage | crashes |
|---|---|---|---|
| `/tmp/rig/fuzz330.log` | `--episodes 1..9 --seeds 4` | 222/503 | 0 |
| `/tmp/rig/fuzz330b.log` | `--episodes 1..9 --seeds 9` | 231/503 | **1** — `CRASH r0s3_st4 rc -11` |
| `/tmp/rig/fuzz330c.log` | `--episodes 4 --seeds 4` | 217/503 | **1** — `CRASH r0s2_st4 rc -11 [input-only]` |
| `/tmp/rig/fuzz330d.log` | `--episodes 1..9 --seeds 9`, census binary rebuilt | **252/503** | **0** |

The two SIGSEGVs were real signals, so they were chased rather than explained away:

1. `port/test/night-census` had a directory date of **Oct 30 00:54** — 35 days older than HEAD, older than the
   `fn_03ce0b` arm fixes (docs/311), item 6's mirrored-blit fix (docs/303), the NULL chain-link guards and the
   item E/F fixes.  `scripts/rig/fuzz.py` never built it.
2. The saved crashing drive `/tmp/rig/fuzz/r0s2_st4/drive.txt` (123 lines, `[input-only]`: keys only, no state
   pokes) was replayed against the ASan app build: `rc=0`, no `ERROR: AddressSanitizer` line, no SEGV report.
3. `make -C port censusapp -B` (Nov 4 17:54) and the identical seed set re-run: **0 crashes**, coverage 252/503.

`scripts/rig/fuzz.py` now runs `make -C port censusapp` before a census (`FUZZ_NO_BUILD=1` opts out) — the fuzz
route had the same defect `asan_soak.sh` already guards against (docs/265 §2).  Coverage is stated as a range,
**222-252 of 503** shipped-named functions across four censuses on HEAD; the gate is the crash census, which is 0
on a fresh binary.

## 6. The six 2026-10-29 oracles, re-run

| item | leg, this sweep | reading |
|---|---|---|
| 1 — title/room track selection and the CD-DA delay | `/tmp/rig/rb330` audio oracles | board/SFX: port `412123/412123` samples placed into 89 of 1499 record windows at the programmed 10989 Hz (37.5 s of sound in 235.6 s of route); CD: `orig 137.5 s -> 1376 buckets, onsets 11, active 0.961` vs `port 155.8 s -> 1559 buckets, onsets 3, active 0.997` |
| 2a — the EXP label glyph | docs/327 (measured in this goal, `/tmp/rig/k327` + `/tmp/rig/rb15/orig`) | kills 2 and 2; `valuedisp_tail ... used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7`, `valuedisp_tail_blit wrote=63 px bbox 13x6`; the pre-fix neighbour read is printed beside it as `idx11 col=49 rows=49 glyph=0` |
| 2b — the per-type award | `exp_award.py --orig /tmp/rig/rb330/orig --port /tmp/rig/rb330/port` | `kind=1 type_idx=31 award(+0x32)=10 state=0x15 hp=0`; per-type award values `{0: [0,52,53,54], 31: [0,10], 201: [0], 250: [0]}` — the settled award is the shipped's own type-row value (docs/284) |
| 3 — straight-down walking | `/tmp/rig/w330` (`ep1_downonly.txt`) | `walk census: walking-body (LAB_0003284e) calls 6022, player DOWN fn_01586b calls 4653`; player y span 172..195 inside stage 1's band 139..195 (docs/314), `DAT_639e0` 0 throughout — stage 1 legitimately cannot scroll vertically |
| 4 — the slide-show key gate | `/tmp/rig/s330b` (`NIGHT_SLIDEDBG=1`, `--warp-stage 3`, `ep1_stage3_slides.txt`) | 6 cards, each gate ended by a press: `card 6 gate returned 0 ... spins=947130 edges=0`, `keywait: press arm taken at spin 191768 (enter=1)`; pre-latch every gate `enter=0 space=0 punch=0 kick=0 esc=0` -> no gate satisfied by a carried latch |
| 5 — the stage-2 crash and tail | §3 soak (stage 2 `rc=0`, 0 asan/ubsan/segv) + docs/328's tail | the only fault witness available is a sanitizer report and there is none on any of the nine maps |
| 6 — the mirrored-limb blit | §4 blit census (`spans_inspected=933777, crossings=0`) | no run ever leaves its row, which is the defect docs/303 fixed |

Round-4 legs re-checked on HEAD in passing: item E at `/tmp/rig/e330g` (`show: entry stage=1 count=5`, five
pictures, `press taken by enter(63ea4) (gate spins 400565)`, **0** gates satisfied with no press — red was 4,
docs/323) and item F in the same run (`reveal: passes=79 gate_spins=959 row_copies=15800 colAcc=320
elapsed_ms=2635.4 presents=80 n=7` — paced, presented, 80 shear passes reaching the screen; the red was 80 passes
in 0.1 ms, docs/325).

## 7. Two rig corrections this sweep produced

- `scripts/rig/fuzz.py` builds `censusapp` before censusing (§5).  A crash census against a stale census binary
  produced two SIGSEGVs that the current HEAD does not produce.
- A zero from a witness is a filter bug until the witness name is checked: the item-4 slide gates print under
  `NIGHT_SLIDEDBG`, and the first run asked for `NIGHT_SHOWDBG` and reported nothing.  The instrument was not
  silent, my grep was (docs/289 §5 class).
