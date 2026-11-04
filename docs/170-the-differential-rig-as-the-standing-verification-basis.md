# 170 — The differential rig as the standing verification basis (methodology)

What the rig is, how to run it, what each part can and cannot prove, and the evidence rules it operates under.
Written at closeout so a later session can reproduce every claim in this goal without re-deriving the tooling.

## 1. Components

**Static equivalence audit** (`scripts/audit/`, run via `scripts/audit/static_audit.py` + `triage.py`)

| check | what it compares | filed as |
|---|---|---|
| `c1` DS aliasing | DS symbol intervals vs port globals | `FILED-SPAN` (141) |
| `c2` port span overlap | two port objects claiming one DS address | 7 rows, incl. the docs/112 pool family |
| `c3` image-init vs port zero | `NIGHT.EXE` bytes at file offset `DS-0x7600` vs a port global that stays zero | 10 rows |
| `c3b` full-span initializers | byte-compares every DS-marked global *with* a brace initializer over the port's own declared span | 0 real mismatches; 2 code-region informational; 10 BSS |
| `c9` access width vs port type | shipped load/store width from `ds_access_widths()` (**direct** only; based forms like `MOV EAX,[EAX+0x53d08]` touch EAX+0x53d08, not DS 0x53d08) | fixed `g_dat_63338`, `g_dat_63380/63384`, `g_dat_63a4`, `g_dat_53788/537a7` |
| `c4` unreachable guest functions | shipped functions with no port call path + `gap_frontier.json` caller-reachability split | 64 `FILED-CALLGRAPH-GAP` |
| `call_sites.py` | disassembles every caller and prints the instructions feeding each CALL to a target | used to find fn_010b1d's 9 callers, fn_040b83's 1 caller |
| `disasm.py` | pattern search over `NIGHT.EXE` with `--all` (guest address + enclosing function + decoded instruction) | absence claims must use this, not `--entry/--end` ranges |

**Runtime rig**

- `scripts/rig/bringup.sh` — container bringup with pinned `cpu_cycles`, `[mixer] rate = 44100`, `imgmount -t cdrom`.
- `scripts/rig/replay_original.py` — frame-indexed input replay + DS/frame/heap sampling (`state_NNNN.{ds,frame,heap}.bin`, `timeline.json`), optional `--io 0x02,0x03,0x83,0x0a` for DMA programming events.
- `scripts/rig/capture_original.py` — the screen/audio capture session (ZMBV frames, `audio.wav`).
- `scripts/rig/route.sh [drive] [seconds]` — one command, both sides, one `DIVERGENCE.md`: builds the port first (`build_stamp.txt`), runs the original twice in fresh containers, runs the port with `NIGHT_SEED` pinned and `NIGHT_RIG_STEP_CALLS=665` (docs/168 §2b), then every oracle.
- Port-side sink: `src/app/rig_sink.c` — `cells.bin` (16-byte file header, 40-byte packed record header, cell payload, entity slots), `records.jsonl` (provenance sidecar), `markers.jsonl`, `pcm.wav`, `cdda.raw`/`cdda.jsonl`, `frames.bin`.

**Oracles** (`scripts/rig/`)

`diff_state.py` (DS-cell + record verdicts with `NON-GAMEPLAY:*` families), `diff_screen.py` (lag-window DP alignment),
`report_activity` profile (269/269 SAME today), `diff_audio.py` + `port_mix.py` (board PCM and CD-DA reconstruction),
`sfx_mask.py` (content-based music masking, never DS 0x53c44), `io_census.py` (DMA programming events),
`cd_schedule.py` (play/stop/teardown invariant), `ent_list.py` / `rec_stream.py` / `pop_align.py` (entity population
and record streams), `cam_policy.py` / `cam_jump.py` / `room_record.py` (camera and scroll gates),
`debt_trace.py` / `speed_model.py` (damage bookkeeping, motion bands), `drift_probe.py` (phase-vs-value test),
`cells_reader.py` (the only sanctioned `cells.bin` reader).

**Witness family** (in-process, `NIGHT_*=1`): `NIGHT_STTRACE`, `NIGHT_HITTRACE`, `NIGHT_ACTTRACE`,
`NIGHT_ACTCENSUS`, `NIGHT_DEATHTRACE`, `NIGHT_SPAWNTRACE`, `NIGHT_SCROLLTRACE`, `NIGHT_WALKTRACE`,
`NIGHT_AUDIODBG`, `NIGHT_AUDIODUMP`, `NIGHT_PLAYVOC`, `NIGHT_RIGDBG`, plus the arena sanitizer
(`arena_live_add/del/has`, `arena_flag_check`).

**Fuzzing**: `scripts/rig/fuzz.py` (coverage-guided corpus, `--warp-stage N`, `--families N`, `--macros`, injection
verbs `poke/ent/play/actdump`), `make censusapp` + `scripts/audit/gen_fn_table.py` (503 shipped-named
functions as the coverage signal).

**Sanitizers**: `scripts/rig/asan_soak.sh` (nine episodes, ASan+UBSan), `Makefile` `asanapp-o2`.

**Fixtures**: `make test` (per-function fixtures keyed to guest addresses), `make selftest`.

## 2. The standard sweep

```sh
make app && make test && make selftest
OUT=/tmp/rig/gNN bash scripts/rig/route.sh ep1_kill3 300
python3 scripts/rig/diff_state.py /tmp/rig/gNN          # value oracle
python3 scripts/rig/diff_screen.py /tmp/rig/gNN         # framebuffer + pacing
python3 scripts/rig/diff_audio.py /tmp/rig/gNN          # board/SFX window oracle
python3 scripts/rig/port_mix.py /tmp/rig/gNN            # CD-DA reconstruction
bash scripts/rig/asan_soak.sh
python3 scripts/rig/fuzz.py --stages 1 2 3 4 5 6 7 8 9 --seeds 18 --workers 4
python3 scripts/audit/static_audit.py && python3 scripts/audit/triage.py
```

## 3. Alignment and evidence rules

1. **Anchor on a transition, never a level.** `DAT_00063e14 88 -> 67` (room-script drain) is the standing anchor;
   "first live entity" is not comparable because the shipped title screen already owns entities at 400 ms while the
   port's title does not (docs/158 §5).
2. **A claim names its run, cadence, frame budget and live-sample count.** "State X never happens" was once an
   artifact of a 4000-frame budget (docs/167 §6); "EXP diverges" was an artifact of filler bytes in one run's
   record block (docs/125 §6); record-level numbers quoted from the wrong blob or the wrong stride are retracted in
   docs/167 §19.
3. **Distinguish presence proof from equivalence proof.** The activity-profile oracle (269/269 SAME) proves the
   same cells change at the same times; the value oracle (DIFF 38 today) proves the values agree. Only the second
   supports "fixed".
4. **Red -> green pairs are the unit of a fix.** Every fix in this goal is recorded with the measurement before and
   after on the same drive (`docs/111` 0 -> 49 reaction samples; `docs/113` 0 -> 74 SFX plays; `docs/122` DIFF
   274/274 -> 269/269 SAME; `docs/133 §10` DIFF 111 -> 74 verdicts; `docs/168` quantum 7 -> 9/10).
5. **Rig defects are fixed, never worked around**, and a rig defect that produced a published number forces a
   retraction: parser dimension eval, teleport accounting, envelope thrown away, 22050/44100 capture, wrong audio
   stream, 352-byte dumper reads, `--frames` as a floor, `--search` printing one hit, `cells.bin` framing.
6. **Never sample a port that changes what it measures**: 0x22e is off-limits because reading it pops the DSP
   acknowledge byte the game waits on.
7. **A sidecar the oracle cannot read is worse than none**: `records.jsonl` needed an explicit overflow guard
   (docs/167 §18) and `cells_reader.py` raises on a ragged payload (docs/168 §4).
8. **`make test` before any commit touching `src/`** — witness-only edits included (docs/169).

## 4. What the rig demonstrably cannot see yet

- **States the drives never reach.** No drive enters `0x66`/`0x70` with a `'*'`/`'6'` slot byte, and
  `fn_031f10`'s deferred queue is called zero times on every drive tried (`+0x3e`/`+0x42` zero in 1200/1200
  samples). The pair/deferred machinery — where "player stuck when hit mid-kick" lives — is unexercised, and the
  fixture suite is what caught a real defect there (docs/169).
- **Fallen states** (`0x5b..0x66`): zero on both sides for every drive tried (docs/117, docs/156).
- **Non-comparable DS families**: `NON-GAMEPLAY:device_plumbing` 27, `dos_extender` 10, `host_address_space` 2,
  `arena_state` 3, plus 14 ENTROPY cells — reported with a written reason, never filtered out.
- **Absolute heap addresses** (`0x53350/53354/53358`): the port's flat heap is its own arena, so only dynamics are
  comparable.
- **Entity lifetimes**: identity links only form between adjacent samples, so lifetime numbers are untrustworthy
  until re-derived from consecutive-sample runs (docs/115 caveat, third occurrence).

## 5. Residual ledger at closeout

| item | state |
|---|---|
| 7 playtest items | all closed with ROOT CAUSE / DOCS / EVIDENCE in `docs/manual-playtest/02-2026-10-28.md`; item 6 (fallen) and item 4 (teleport) closed as **no divergence** after rig defects were fixed |
| value state oracle | **not green**: 38 same-width gameplay divergences (camera/scroll cluster, entity population, hit/reaction bookkeeping, CD selection, CONTINUE countdown), 6 DIFF-WIDTH, 13 DIFF-SIZE, 35 REC-DIFF / 15 REC-SAME, 11 REC-NULL |
| screen oracle | pacing 5.02 vs 27.39 picture changes/s — open |
| audio oracle | shipped programs 324 DMA transfers in 150 s on a 257-byte ladder (19.5 kB/s) vs the port's ~73 whole-block feeds (1.7 kB/s); trigger wiring present, trigger *frequency* is the gap — mechanism identified, **no port change yet** |
| game clock | port loop ~70 ticks/s vs shipped ~95 — **open port defect** (docs/168 §2a); oracle calibrated to 665 PIT Hz |
| chain reachability | 262-267 of 503 functions entered under three fuzz grammars; 236-241 never entered; state-directed family injection reached 248 max |
| pair/deferred chain fixtures | green again after docs/169's restore |

## 6. Standing sweep re-run on the current build (after docs/173, docs/175, docs/176)

| Check | Command | Result |
| --- | --- | --- |
| Port unit + fixture suite | `make test` | **ALL PASS** (including the rewritten `DAT_532f4` fixtures, docs/176 §4) |
| Nine-episode sanitizer soak | `scripts/rig/asan_soak.sh` | 9/9 `rc=0`, `ubsan=0`, `asan=0`, `segv=0`; 987-4000 frames presented per stage (`/tmp/rig/soak`) |
| Canonical state route | `OUT=/tmp/rig/g27 RIG_HZ=10 scripts/rig/route.sh ep1_kill3 150` | 346 cells; SAME 69, DIFF 35, DIFF-SIZE 14, PTR 52, REPR 19, SIZE? 34; **35 residual same-width gameplay divergences**; activity-profile oracle 271/271 SAME |
| Coverage-guided fuzz | `scripts/rig/fuzz.py --episodes 1..9 --seeds 9 --workers 4 --cap 110 --frames 5200 --inject` | `247/503` functions entered, **0 crashes** (`/tmp/rig/fz14`); the same plateau as the earlier 262-267/503 sweeps at higher seed counts |

Two quotability rules that this re-run depends on:

1. **Route duration.** State-oracle counts are comparable only on the canonical 150 s route.  On 45 s routes the
   shipped timeline (which starts near +47 s and runs stretched relative to wall clock) overlaps the port timeline
   for under 8 s, 244 of 274 windows come back SHIPPED-QUIET, and the residual count drops to 30-31 without the
   port changing at all (docs/176 §7).
2. **Cadence.** `RIG_HZ=10` on the shipped side; the route script's default of 4 Hz produced 156 shipped samples
   where 424-853 are needed for the same window grid.
