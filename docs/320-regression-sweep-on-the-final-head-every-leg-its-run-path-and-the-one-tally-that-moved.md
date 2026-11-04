# 320. Regression sweep on the final HEAD: every leg, its run path, and the one tally that moved

Goal task-6.  HEAD at the time of the sweep: `8fd5c2a` (docs/312 §5 filled); the fixes inside it are
`444c2aa`, `b5355d3` (round 2), `9abb102` (fn_03ce0b axes/arms), `6c2d327`+`f6afb71` (item D threshold),
`03ff479` (docs/318 witnesses), `14d6b6a` (item G index fixes), `8fd5c2a`.  Canonical env throughout:
`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60` (`SDL_AUDIODRIVER=dummy`).

## 1. Legs and numbers

| leg | run path | number |
|---|---|---|
| unit suite | `make -C port test` | **ALL PASS** ✓ (the target rebuilds `port/test/night`, docs/181 §5) |
| nine-episode ASan/UBSan soak | `/tmp/rig/soak320` (`OUT=/tmp/rig/soak320 JOBS=4 CAP=420 FRAMES=4000 scripts/rig/asan_soak.sh`, rebuild on ✓ docs/265 §2) | 9/9 stages `rc=0 ubsan=0 asan=0 segv=0`, `frames presented: 4000` in every stage log |
| coverage-guided fuzz | `/tmp/rig/fuzz320` (`--episodes 1..9 --seeds 40 --workers 4 --cap 120`) | **crashes 0**; coverage `234/503` functions entered, 269 never entered — coverage quoted as a range across censuses (254/237/233/234), the crash census is the gate (docs/308) |
| blit row-crossing + arm census | `/tmp/rig/t11le` (`ep1_leftedge4.txt`, `NIGHT_BLITEDGE=1 NIGHT_BLITDBG=1`) | `blt_rows calls=5159 out_of_band=0`; `spans_inspected=2265525 inside_one_row=2265525 crossings=0` |
| blit arm census, legitimate chain basis | `/tmp/rig/f317b` (`ep1_walk_st2.txt`) | `calls=2297 out_of_band=0`, arms `B0=1956 B1=0 B2=125 B3=91`; `spans_inspected=5469145 crossings=0` |
| canonical route, shipped half reused | `/tmp/rig/rb320` (`REUSE_ORIG=/tmp/rig/rb15 … route.sh scripts/rig/drives/ep1_kill3.txt 150`) | state: `cells=346 phases=274 tally SAME 71, PTR 52, SIZE? 34, REPR 19, DIFF-SIZE 14, DIFF 26`, **26 same-width gameplay residuals**; activity: 271 cells, 275 phases, `{'SAME': 271}`; screen: orig 1422 frames / 204 distinct / 18.6 changes per s, port 17999 / 5453 / 78.6, aligned pairs 1422, **score 0.7088**, 8 divergent runs; audio sfx + cd legs: every phase `SAME` |
| item 4 key-wait gate | `/tmp/rig/o320c` (`--warp-episode 3`, `ep1_stage3_slides.txt`, `NIGHT_SLIDEDBG=1`) | 6 gates (= `g_tab_52f2c[3] = 6`), each ended on a press after 1881 / 6588 / 18761 / 21811 / 25148 / 29447 spins, cumulative `edges=0`, **0 gates satisfied with no press** |
| item 2b award oracle | `scripts/rig/exp_award.py --orig /tmp/rig/rb17/orig --port /tmp/rig/rb17/port`, and `--orig /tmp/rig/rb320/orig --port /tmp/rig/rb320/port` | shipped `type_idx=31` award values `{0, 5, 10}` = port `{0, 5, 10}`; shipped dying records 234 with `kind=1 type_idx=31 award(+0x32)=10` ×32, port 262 dying records |
| item 2a glyph label | `/tmp/rig/o320a` (`ep1_kill3.txt`, `NIGHT_GLYPHDBG=1`, 4000 frames) | `tag=expnum` lines: **0** — expected, because the canonical route produces zero kills on both sides (docs/197, g37), so the kill label cannot arm there.  The claim is carried by the fixture (label blit reads sprite-array index 10, docs/227) inside ALL PASS above |
| item 3 upgrade menu | `/tmp/rig/g319c`, `/tmp/rig/g319d`, `/tmp/rig/g319h`, `/tmp/rig/g319i` (`NIGHT_BUYDBG=1`) | every committed purchase steps `sel` by exactly 1 (`attempts=2..4 committed=2 rejected_max=0 refund=0`); the old `NIGHT_LUMENU` row-visit witness no longer exists in the build, so the lit-without-purchase claim is restated as this census (docs/312 §5) |
| item 5 fault | `/tmp/rig/soak320` (above) | 0 sanitizer/UBSan/SEGV lines across 36,000 frames; the historical stage-2 "bus error" remains withdrawn (docs/299) and the single `rc=139` at 16000 frames remains unreproducible (docs/316: same drive + env ran clean 3×, brackets 800…12800 all `rc=0`) |

## 2. The one tally that moved, and why the baseline row is not literally identical

The contract asked for tallies identical to the `/tmp/rig/rb15` baseline under `REUSE_ORIG`.  Measured:

| run | port half built at | state tally | residuals |
|---|---|---|---|
| `/tmp/rig/rb15` (the frozen shipped capture, and its own port half) | the build of that session | `SAME 70, REPR 19, DIFF 27` | 27 |
| `/tmp/rig/rb17` (docs/312's sweep, `REUSE_ORIG=/tmp/rig/rb15`) | `b5355d3` | `SAME 70, REPR 19, DIFF 27` | 27 |
| `/tmp/rig/rb11` (docs/294's recorded baseline) | `0996686` | `SAME 71, REPR 20, DIFF 26` | 26 |
| **`/tmp/rig/rb320` (this sweep)** | `8fd5c2a` | **`SAME 71, REPR 19, DIFF 26`** | **26** |

So the current HEAD sits at the *better* end of the recorded band (26 residuals, one cell closer to `SAME` than
rb15/rb17 were), not at rb15's literal numbers — rb15's own port half predates `9abb102` (fn_03ce0b's row/column
axis and the signed arm tests, docs/311) and `f6afb71` (item D's threshold polarity).  Nothing moved the other
way: activity profiles 271/275 `SAME`, the screen score 0.7088 with the same 8 divergent runs, and the same
pacing row (`orig 2.67 vs port 23.81 changes per s`, ratio 0.11, the docs/168 pacing residual family) all match.

`REUSE_ORIG` reuses the shipped half, so this sweep cannot re-verify the shipped side (docs/307).  The container
engine for a fresh shipped capture is unresponsive on this machine, so no new shipped half exists; that gap is
recorded rather than papered over.

## 3. Re-check after item H landed (`2336118`)

Item H changes the host presentation geometry, so the sweep rows above (taken at `8fd5c2a`) were re-run
on the new HEAD before anything was quoted from them.

| leg | run | number |
|---|---|---|
| unit suite | `make -C port test` | ALL PASS (also re-run after the fix, in the `/tmp/rig/h321e` build) |
| canonical route, shipped half reused | `/tmp/rig/rb321` and `/tmp/rig/rb322` (two runs, `REUSE_ORIG=/tmp/rig/rb15 … route.sh scripts/rig/drives/ep1_kill3.txt 150`) | state `cells=346 phases=274 tally {'SIZE?': 34, 'PTR': 52, SAME 70, 'REPR': 19, 'DIFF-SIZE': 14, 'DIFF': 27}`, **27 same-width residuals**; activity 271 cells / 275 phases `{'SAME': 271}`; screen 1422 aligned pairs, **score 0.7088**, 8 divergent runs -- identical to `rb320`'s screen and activity rows |
| residual-set diff | `/tmp/rig/rb320.res.txt` vs `/tmp/rig/rb321.res.txt` vs `/tmp/rig/rb322.res.txt` | exactly one cell new: **`0x63ed8 g_dat_63ed8`**, present in both post-fix runs and absent pre-fix |

`g_dat_63ed8` is the key-state byte `fn_038f3d` tests at 0x39732 (`cmp byte [0x13ed8], 0`), i.e. one of
the latch bytes of the docs/292 family.  `sys_vga_set_mode()` writes only `sys.c`'s geometry variables,
and `grep -n "sys_video_w\|sys_video_h" port/src/f/f.c` returns nothing -- the modelled game code never
reads them -- so no modelled store changed.  The plausible mechanism is the one docs/310 warns about:
drives are wall-clock anchored, a mode change costs a texture rebuild in `main.c:ensure_mode`, and the
phases the state oracle pairs therefore land slightly differently.  The row is recorded as a new residual
rather than explained away: the count moved 26 -> 27 on the canonical route, and the screen/activity rows
did not move at all.

## 4. Every sweep leg re-run once more on the final HEAD (`cea5721`)

Because item H changed `sys.c`/`f.c`, every regression leg was re-run after it rather than inherited
from §1.

| leg | run | number |
|---|---|---|
| nine-episode ASan/UBSan soak | `/tmp/rig/soak321` (`OUT=/tmp/rig/soak321 JOBS=4 CAP=420 FRAMES=4000`, rebuild on) | 9/9 stages `rc=0 ubsan=0 asan=0 segv=0`; zero `ERROR: AddressSanitizer` / `runtime error` lines in any stage log |
| coverage-guided fuzz | `/tmp/rig/fuzz321` (`--episodes 1..9 --seeds 40 --workers 4 --cap 120`) | **crashes 0**; coverage 236/503 (the coverage census range across runs is 234-257/503; the crash census is the gate) |
| item 6 left-edge row-crossing probe | `/tmp/rig/q322le` (`ep1_leftedge4.txt`, `NIGHT_BLITEDGE=1 NIGHT_BLITDBG=1`) | `spans_inspected=2335538 inside_one_row=2335538 crossings=0`; `blt_rows calls=5240 out_of_band=0`, arms `B0=5240 B1=0 B2=0 B3=0` |
| item 4 card key-wait gate | `/tmp/rig/q322sl` (`--warp-episode 3`, `ep1_stage3_slides.txt`, `NIGHT_SLIDEDBG=1`) | 6 gates (`g_tab_52f2c[3] = 6`), every one `returned 0` after a press; **0 gates satisfied with no press** |
| item 2b per-type award | `scripts/rig/exp_award.py --orig /tmp/rig/rb322/orig --port /tmp/rig/rb322/port` | shipped `type_idx=31` awards `{0, 5, 10}` = port `{0, 5, 10}`; a dying `kind=1 type_idx=31` record carries award 10 (shipped 32 records, port 44) |
| item 1 audio legs | `/tmp/rig/rb321`, `/tmp/rig/rb322` (`report_audio_sfx.txt`, `report_audio_cd.txt`) | every phase `SAME` (the docs/304 instrument floor still applies: the shipped half carries no CD-DA) |
| item 2a kill label | `make -C port test` | ALL PASS, which is where the index-10 label-blit fixture lives; the canonical route still yields no kill, so no live witness is available there (docs/197) |
| item 3 upgrade menu | `/tmp/rig/g319c`, `/tmp/rig/g319d`, `/tmp/rig/g319h`, `/tmp/rig/g319i`, item G's green `/tmp/rig/g319i` | every committed purchase steps `sel` by 1 and HP by one table row (100 -> 200 -> 300) |
| item 5 fault | `/tmp/rig/soak321` | 0 faults over 36,000 frames across nine episodes |

> **RETRACTION of the "item 4 card key-wait gate" leg (2026-11-04, docs/322 / docs/323).**  That leg
> (`/tmp/rig/q322sl`, `--warp-episode 3`, `ep1_stage3_slides.txt`, 6 gates, every one ended by a press) does not
> gate the reported dialogue defect.  `--warp-episode 3` sets the **stage/map** counter, so the run sat on map 3,
> and the gates it counted are the **title-path slide show's** card gates, while the report is about an overlay in
> the last room of a map (`fn_03a70e`, armed at 0x3aa4c-0x3aa8f, press gate 0x3a785-0x3a7a4, release wait
> 0x3a7cd-0x3a7df).  The leg survives only as a statement about the slide show.  The gates for that item are now
> `/tmp/rig/e325r` (red, 4 no-press gates) and `/tmp/rig/g325a` (green, 0), both on
> `--warp-stage 1 --warp-column 3`.

Against the cached reference itself, the final HEAD matches: the divergent-cell list in
`/tmp/rig/rb322/DIVERGENCE.md` has 81 entries and the set diff against `/tmp/rig/rb15/DIVERGENCE.md` is
empty (`/tmp/rig/rb15.res.txt` vs `/tmp/rig/rb322.res.txt`), with the same 27 same-width gameplay
residuals `rb15` reported.  The dip to 26 in `rb320` (§2) was therefore not a new state of the port --
it was one cell moving one way under the `f6afb71` build and back under `2336118`, and the row that was
new in §3 (`g_dat_63ed8`) is a cell `rb15` already listed.
