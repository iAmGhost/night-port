# 151 — The differential rig as the standing verification basis (methodology)

This goal replaced human playtesting with a machine rig. This document is the operating manual: what the rig
is, how to run it, what each oracle can and cannot prove, and the rules that the goal's findings forced into
the rig itself.

## 1. Three layers, all driven from the shipped binary

* **Static equivalence audit** (`scripts/audit/static_audit.py`, `triage.py`, `reports/static_findings.json`).
  Checks are keyed to `NIGHT.EXE` itself: DS symbol intervals vs overlaps (c2), image-initialised bytes
  compared against the port's hand-written initializers at file offset `DS - 0x7600` (c3, c3b), access width
  from the Ghidra access table split into `direct` vs `based` (c9), and call-graph reachability of shipped
  functions (docs/120's gap frontier). Findings are either fixed or *filed* with a reason — "filed" is a
  legitimate end state, "quiet" is not.
* **Runtime differential rig** (`scripts/rig/`). One deterministic drive file drives both sides: the container
  running `NIGHT.EXE` under pinned cycles (`bringup.sh`, `replay_original.py` via the dosbox-automation REST
  API) and the port running the same drive (`route.sh` builds first, then runs with `--rig-dump`). Alignment
  is **event-anchored** (`wait ds <cell> <value>` on both sides, `rig_rebase` shifting the port's clock), never
  raw frame numbers.
* **Coverage-guided fuzzing** (`scripts/rig/fuzz.py`, `episode_curve.sh`). The coverage signal is a per-function
  entry census whose names *are* guest addresses (`scripts/audit/gen_fn_table.py`, 503 shipped functions), so a
  coverage number is a statement about shipped code, not about the port's own structure.

## 2. The oracles, and what each is allowed to say

| oracle | command | it can prove | it cannot prove |
|---|---|---|---|
| state / DS value oracle | `scripts/rig/diff_state.py` | a DS cell holds different values in the same phase window | anything about a cell whose shipped span/width is unknown (verdict `SIZE?`/`REPR`) |
| record oracle | same, record regions | byte-level divergence in a record block, with candidate field names | a field name is only as good as the offset calibration; entity-shaped blocks only (`looks_like_entity`) |
| entity census | `scripts/rig/ent_list.py`, `frame_counter.py`, `player_record.py` | state census, position continuity, counter magnitude, count-vs-chain agreement | behaviours no stored drive reaches (docs/118's fallen family) |
| screen oracle | `scripts/rig/diff_screen.py`, `diff_activity.py` | picture-change pacing, per-window divergence, DS attribution of a divergent window | visual "looks wrong" judgements |
| audio oracle | `scripts/rig/diff_audio.py`, `port_mix.py`, `music_melody.py`, `cd_schedule.py` | play/stop schedule equivalence, teardown invariant, audibility by melodic fingerprint | anything if the capture rate is wrong (docs/130 voided a whole comparison) |
| sanitizers | `make asanapp asanapp-o2`, `scripts/rig/asan_soak.sh`→`asan_soak.sh` | memory errors and UB at the *app's* optimisation level | anything about flat-heap wild pointers the arena cannot see (docs/128) |
| fuzz census | `scripts/rig/episode_curve.sh` | which shipped functions the corpus reaches, per episode, per round | that an unreached function is correct |

## 3. Rules this goal had to learn the hard way

1. **A rig defect gets fixed, never smoothed.** Every false finding was traced to the rig: the array-dimension
   parser that invented a 232-vs-1000 divergence (docs/121), the "teleportation" that was three accounting
   defects (docs/115), the `+0x2b` "EXP total" that was a display ramp (docs/125 §5), the EXP divergence that
   was filler bytes in one run (docs/125 §6), the shipped audio capture recorded at 22050 into a 44100 header
   (docs/130), the audio oracle that scored the board stream while the music lived in `cdda.raw` (docs/129),
   the dumper that read 352 bytes out of anything a pointer cell named (docs/146 §3), and `--frames` being a
   floor rather than a budget (docs/146 §9).
2. **A live witness outranks a dump agreement.** docs/142 §4 read a record block off a DS pointer, agreed with
   itself on both sides, and was still wrong: the in-process `ROOM` witness contradicted it, and the block was a
   clamp-bounds struct (docs/143). If a dump-vs-dump agreement disagrees with what the running code sees, the
   reading layer is the defect.
3. **Read the dump's own geometry.** Offsets come from `meta.json`/`timeline.json`, never from summing record
   sizes or assuming a block width (docs/124, docs/114). The shipped block size is derived:
   `len(state_NNNN.heap.bin) / len(ptr_cells)`.
4. **Field names come from writers, not histograms.** `+0x2b` looked like an EXP total; the writer `fn_0137c1`
   showed it was a timed ramp driven by `+0x52` (docs/125 §5). Same for `+0x23`/`+0x56`: a 100-step countdown,
   not damage debt (docs/137).
5. **State the run, the cadence and the live sample count with every claim.** 10 Hz against 4 Hz produced at
   least two false divergences; max/median are cadence-independent, ratios are not.
6. **Instrumentation must be at the shipped optimisation level.** The `-O1` sanitizer build survived drives the
   `-O2` app faulted on; `asanapp-o2` is what found the unguarded list walks (docs/146 §5).
7. **Injection is only a test if the shipped game could hold the state.** A count the `+0x13f` chain cannot
   back, or a raw integer written into a pointer cell, kills the port on arithmetic the shipped game cannot
   execute either. The rig now refuses and counts those instead of generating crashes (docs/146 §2, §7).
8. **Retract in public.** docs/145 §5/§7, docs/125 §6, docs/131's supersession of docs/96, docs/146 §8's
   `rec[+0x17]` bound that four fixtures rejected. A wrong fix that makes a witness quiet is worse than a crash.

## 4. Standing sweep (what to run, in order)

```sh
make test && make selftest            # fixtures: the shipped behaviour pinned in code
make app asanapp asanapp-o2 censusapp
sh scripts/rig/asan_soak.sh                            # nine episodes, sanitizers, rc=0 expected
sh scripts/rig/asan_o2_sweep.sh                        # stored fuzz corpus, -O2 sanitizers
sh scripts/rig/episode_curve.sh                        # per-episode coverage curve + union
python3 scripts/audit/static_audit.py && python3 scripts/audit/triage.py
sh scripts/rig/route.sh ep1_kill3 150                  # one anchored route: state/screen/audio oracles
python3 scripts/rig/diff_state.py --run /tmp/rig/<route>
python3 scripts/rig/ent_list.py --orig .../orig --port .../port
uv run --with capstone scripts/audit/disasm.py --entry 0x1271e --end 0x12810   # any question about shipped code
```

## 5. What the rig has and has not bought

Closed by rig signals alone, each with red -> green numbers: the sound-slot alias (docs/113), the reaction table
(docs/111), the EXP alias (docs/109), the CD teardown (docs/106), the image-initialised tables the port had
zeroed (docs/122, 12 globals + the TZ cluster), the sound-slot table alias, the flat-model alignment family
(5825 sites, docs/126), the unguarded list walks and the injection semantics (docs/146), and the action trip
count `record[+0x5c]` (docs/147) which removed the spurious player death, the CONTINUE countdown, the extra CD
selection and the code-11 hit on the player.

Still open, each with the measurement that is missing: camera clamp path (27 vs 158), `fn_03e20f`'s extra
`rec[0]=0` arms against the shipped 12, `DAT_00063e14` holding 67, hit-code 11 on enemies (`.act` chain byte
`record + slot + 0x38`), music audibility of track 11, and fallen-state coverage. None of them are "no signal"
— they are signals that need a drive or an instrumentation the rig does not have yet, which is the honest way to
leave a ledger.
