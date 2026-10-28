# 120 — Coverage-guided fuzzing: a per-function entry census keyed to shipped addresses, the first nine-stage sweep, and crash attribution by counterfactual

Date: 2026-10-29. Rig pieces added here: `make -C port censusapp`, `scripts/audit/gen_fn_table.py`,
`port/src/app/rig_census.c`, `scripts/rig/fuzz.py`, `make -C port asanapp`.

## 1. The coverage signal is keyed to the shipped binary, not to the port's structure

The port's function names **are** guest addresses, so a per-function entry census over the port is directly a
census over shipped functions:

- `scripts/audit/gen_fn_table.py` reuses the static audit's own parser (`port_src.functions()`) and emits
  `port/src/gen/fn_table.c`: **503 entries** (384 guest-named `fn_*` plus 119 host helpers such as
  `spr_block`, `flat_ofs`), with `g_fn_addr[]` / `g_fn_name[]` kept index-aligned (addresses are link-time, so
  the sort is a permutation built once at startup — never sort the two arrays separately).
- `port/src/app/rig_census.c` implements `__cyg_profile_func_enter` for the `-finstrument-functions` build:
  binary search over the sorted permutation, one byte per function, and two outputs — `census.txt`
  (`seen|never <name>`) and `census.bin` (a 503-byte vector the fuzz driver unions).
- A `SIGTERM` handler writes the census and exits 124. This mattered immediately: the driver kills runs at the
  wall-clock cap, and before this the killed runs contributed **zero** coverage.
- Built by `make -C port censusapp` (`-finstrument-functions`); the hook is a no-op in the normal build, so
  `rig_census.c` + `fn_table.c` now sit in `SRC_RIG` and every target links.

## 2. The driver

`scripts/rig/fuzz.py`: greedy coverage-guided corpus. Each seed mutates a base drive (the ep1 bring-up,
`scripts/rig/drives/ep1_kill3.txt` truncated at `--base-until`) with randomized key down/up/tap events inside a
per-seed window, optionally plus state writes from the drive language (`poke`, `ent`, `play`). A seed is
accepted into the corpus if it strictly increases the union of entered functions; a deterministic quarter of the
rest are kept anyway for diversity, because a plateau usually means the *grammar* is exhausted rather than the
seeds. Every run has a hard `--cap` in seconds **and** a `--frames` budget, `--workers` run in parallel, and
`SDL_AUDIODRIVER=dummy` is forced (docs/116: this host's CoreAudio fallback ladder costs minutes per refused
device and poisons fuzz timing).

Episode reach comes from the existing host debug warp (`--warp-stage N`, docs/52): the stage number is what the
stage loader builds every per-stage name from, so one flag enters any of the nine episodes.

## 3. First nine-stage sweep (`/tmp/rig/fz9`: 36 seeds, stages 1-9, cap 110 s, 4500 frames, 4 workers)

| round | union of entered functions | corpus size | SIGSEGVs |
|---|---|---|---|
| 0 | 245 / 503 | 12 | 5 |
| 1 | **262 / 503** | 20 | 8 (cumulative) |

Per stage (union over that stage's seeds): stage 1 → 222, 2 → 228, 3 → 231, 4 → 230, 5 → 238, 6 → 221,
7 → 236, 8 → 228, **9 → 199** (stage 9 seeds mostly timed out before its rooms loaded).

Never entered after the sweep: **241 of 503**, of which **180 are shipped-named functions**
(`/tmp/rig/fz9/gap_functions.txt`).

## 4. Cross-check against the static call graph (this is the part that makes the gap list actionable)

Intersecting the census with `port_src.functions()`'s call graph:

- guest-named functions that are **both** never entered by any fuzz run **and** never called by any port
  function: **0** — i.e. the two audits agree that there is no shipped function the port forgot to wire up
  *and* the fuzzer failed to reach (the 59 entries in that class are all host helpers).
- guest-named functions **called somewhere in the port but never entered** by 36 fuzz runs: **180**.
  Sample: `fn_0100d9 fn_010376 fn_0105f7 fn_01072f fn_010cbd fn_010e15 fn_013908 fn_01520b fn_015b5b
  fn_015d16 fn_015ed1 fn_01603e fn_0163b4 fn_01656f`. These are genuine fuzz gaps — reachable in principle,
  not reached by the current input grammar (menus, pause/continue, CD-driven sequences, room transitions).

## 5. Crash attribution by counterfactual

Eight seeds exited with SIGSEGV (`rc=-11`). The driver now re-runs a crashing drive **with every state write
removed** and labels the crash from the result:

```
CRASH r1s2_st3 rc=-11 [injection-induced (rc=-11 -> without injections rc=0)]
```

All eight cleared without their injections (`rc=0`), and the canonical case was minimized and localized:

- repro: ep1 bring-up (6 events) + the seed's 253 key events + `130680 poke 0x53c68 1` + `63420 poke 0x53c68 3`
  at `--warp-stage 3`, reproducible 2/2, `rc=139`;
- `lldb -b -o run -o "thread backtrace"`: `EXC_BAD_ACCESS (address=0x30)` in `fn_012efa (e=NULL)` at
  `f.c:14237`, called from `fn_03203d` (the entity draw sweep, guest 0x0003203d);
- `fn_03203d` walks `DAT_00053c70` through the `+0x13f` links for `DAT_00053c68` iterations, re-reading the
  count each iteration and **not** re-validating the walk pointer — exactly as the shipped code does (the
  port's annotation records that: "the count is re-read on every test (0x32072) … the walk pointer is NOT
  re-validated"). Raising the count above the length of the live chain is a state the game itself cannot
  produce, so this is a rig-induced invalid state, not a port defect.

Caveat stated plainly: the same injection on the shipped side has not been run (shipped-side injection parity
is not implemented), so "the original would fault identically" is an inference from the shipped instruction
sequence, not a measurement.

## 6. What the sweep does *not* yet show

No input-only crash and no oracle divergence: in 36 runs across all nine stages every non-injected seed exited
`rc=0`. So fuzzing has not yet produced a new bug — it has produced a coverage map and a gap list. The
plateau (round 1 added 17 of 18 seeds' worth of nothing) says the next gain is grammatical, not numerical.

## 7. Next steps, in order of expected yield

1. **Differential fuzzing**: pair each accepted seed with the shipped replay (`scripts/rig/route.sh`) and run
   the state oracle over the fuzz window, so coverage growth is matched by divergence detection. Coverage
   without an oracle only finds crashes; the goal is misbehaviour.
2. **Grammar for the 180 gap functions**: menu/pause/continue/CD sequences and room transitions, keyed off the
   call graph (which port function calls the gap function, and what drive event reaches that caller).
3. **Shipped-side injection parity** through the emulator memory API, so injected-state experiments (including
   this crash attribution) become true differential statements.
4. Stage 9 seeds need a longer `--cap`/`--frames` (it entered the fewest functions).
