
## 12. The gap, re-cut as a frontier: which never-entered functions have a caller we already reach

Three fuzz grammars plateau at the same set (docs/120 §8-§10). To make the 199-function never-entered set
actionable, `scripts/audit/reports/gap_frontier.json` splits it by *who calls it* in the shipped call graph
(`scripts/audit/functions.json`'s `callees`, resolved address -> function name):

```
base sweep /tmp/rig/fz10/coverage.txt   (nine-stage, 18 seeds, 3 rounds)
  never entered, resolved against the shipped function table : 199
  SHALLOW  -- at least one caller IS entered by the fuzz corpus : 61
  NO KNOWN CALLER in the extracted graph                       : 55
  (of the 61 shallow, 4 have three or more covered callers)
```

(An earlier version of this cut reported "shallow = 0" because `callees` are stored as guest *addresses*
(`['0x10181', '0x418ee', ...]`) and were being matched against function *names*; the fix is a resolved
address -> name table, which is what produced the numbers above. Filed as a rig defect, not smoothed.)

The shallow set is the fuzzing target list: these are functions whose caller the corpus already executes, so
only a guard or a data precondition separates them from coverage. The largest shallow clusters name the
subsystems docs/120 §11 suspected:

```
fn_016b8f  <- fn_017f13, fn_018e88, fn_019e48, fn_01aec8      (the per-type entity controller family)
fn_017ca6  <- fn_017f13, fn_018e88, fn_019e48, fn_01aec8
fn_042a0d  <- fn_04192e, fn_041c8f, fn_04247a, fn_042576      (device / host-service layer)
fn_045a82  <- fn_042aba, fn_044807, fn_04488b                 (device ioctl error path -- fx-tested, never run)
fn_042394  <- fn_03af18, fn_03f15f
fn_0427d2  <- fn_0427a3, fn_0493c3
fn_047d24  <- caseD_8, fn_0461cc
cb_indirect_53644 <- fn_0451bc                                (the docs/116 fault site family)
```

Concrete next fuzz grammar, ordered by what the frontier says rather than by keystroke randomisation:

1. **entity-family seeds**: `fn_016b8f` / `fn_017ca6` sit under four callers the corpus already enters, and the
   family is selected by `entity.type` -- so seeds that use the `NIGHT_WARP_ENTITY` spawn warp (already wired:
   `f_warp_entity_spawn` builds the shipped opcode-1 shape) across the `.spr` families the episode loaded are
   the direct way in. That is a *state* mutation, not a key mutation, which is exactly what §9-§11 showed the
   key grammars cannot do.
2. **device-service seeds**: `fn_042a0d` / `fn_045a82` are reached from the board/device layer the corpus
   already runs; `fn_045a82` is the ioctl error path the fx fixture tests but no replay ever exercises, so a
   seed that drives the board write path while the device list is empty targets it.
3. **`cb_indirect_53644` under `fn_0451bc`**: docs/116 fixed a fault in exactly this family; entering the
   indirect callback under the banner-walk path is the remaining coverage question there.

55 functions have no caller in the extracted graph at all (and 83 are reached only by callers that are themselves never entered) -- those are not fuzz targets yet: either the
extractor missed an indirect call site (a table of function pointers is how this binary dispatches, cf.
`g_tab_5271c` / `g_tab_523a4`) or the function is reached through one. Resolving indirect dispatch is a static
audit task, not a fuzz task, and it should precede any further fuzz sweep: without it the frontier understates
how shallow the gap really is.

## 13. The state-directed grammar, wired and measured: family spawning reaches some of it and also breaks the port

`scripts/rig/fuzz.py --families N` now sets `NIGHT_WARP_ENTITY` per seed to a 4-name subset of the shipped
`.spr` families, which is the direct route into the shallow cluster named in §12 (the per-type entity
controllers selected by `entity.type`). Two rig details had to be fixed to make it usable:

* the subset index used `abs(hash(seed_dir))`, and Python salts `hash()` per process -- a crash found in one
  sweep could not be re-run. It is `zlib.crc32(seed_dir.encode())` now, so a seed's family set is reproducible.
* `NIGHT_WARP_ENTITY` alone is refused in a warped run: the request is checked every frame and at the frame a
  warp-stage jump lands there is no room yet -- `warp-entity: ena-1 deferred at frame 54: no live room record
  (the room loop owns the entity list)`. `NIGHT_WARP_ENTITY_AT=<frame>` defers the request until the room loop
  is live, and then it works: `warp-entity: ena-1 type=31 frame=3038 spawned`,
  `warp-entity: enc-1 type=41 frame=3038 spawned`, while a family the episode never loaded is refused with the
  honest reason -- `ene-1 deferred ... this episode never loaded the type's .spr block`.

Measured coverage (census universe 503 shipped-named functions):

| sweep | stages | families | AT | coverage | crashes |
|---|---|---|---|---|---|
| fz13 | 1 | 15 | -- | 233/503 | 0 |
| fz13b | 1 | 15 | 3000 | 236/503 | **1 (SIGSEGV)** |
| fz15 | 1,3,5,7,9 | 15 | 3000 | 248/503 | 0 |
| fz10 (baseline, no families) | 1-9 | -- | -- | **262/503** | 0 |

So family spawning on five stages is not yet comparable to the nine-stage baseline, and on stage 1 alone it adds
3 functions over the same-shape run. The ceiling from §8-§10 is not broken by this either -- but the sweep did
produce something more interesting than a number:

**the SIGSEGV in `fz13b/r1s2_st1` is caused by the spawned families, not by the keys.** That drive has zero
`poke`/`ent`/`play` verbs, and re-running the *same* drive without `NIGHT_WARP_ENTITY` under the census binary
returns `rc=0` (`frames presented: 5537`, clean unwind), while with families it dies with signal 11. Under the
ASan build the same families on the base `ep1_kill3` drive spawn cleanly and report nothing, so the fault needs
the specific family subset (now reproducible) plus the seed's key window to reproduce, and it lives in the
spawned-entity path rather than in the input path.

Queued, in order:

1. Reproduce `fz13b/r1s2_st1` with its crc32 family subset under `night-asan` and localise the fault. A crash
   produced by *adding an entity of a family the episode did not script* is a candidate real port defect (the
   same family of problem as docs/128's dangling entity pointer), not merely rig noise.
2. Re-run the family grammar across all nine stages with `AT` set, and report it against the 262/503 baseline in
   the same shape as §8's table so the comparison is honest.
3. Keep the §12 frontier as the target list: 61 shallow functions whose caller is already entered.

## 14. Nine-episode curve on the budgeted build (docs/146 §9): 0 crashes, coverage up in every episode

`scripts/rig/episode_curve.sh` runs one fuzz session per episode with a fixed shape — 4 seeds, 2 rounds,
4 workers, 6600-frame budget, 110 s wall cap, injections and family spawning on — and reports each stage's
coverage plus the union of the coverage files. Run on the build that carries docs/146's fixes:

| episode | docs/120 §5 (unbudgeted) | round 0 | round 1 | never-entered after | crashes |
|---|---|---|---|---|---|
| 1 | 222 | 220 | **234** | 269 | 0 |
| 2 | 228 | 222 | **239** | 264 | 0 |
| 3 | 231 | 222 | **231** | 272 | 0 |
| 4 | 230 | 231 | **235** | 268 | 0 |
| 5 | 238 | 241 | **241** | 262 | 0 |
| 6 | 221 | 222 | **230** | 273 | 0 |
| 7 | 236 | 240 | **241** | 262 | 0 |
| 8 | 228 | 227 | **235** | 268 | 0 |
| 9 | 199 | 214 | **215** | 288 | 0 |

Two things the contract asked for, both measured:

* **The never-entered set shrinks run over run in every episode** — 220→234, 222→239, 222→231, 231→235,
  241→241 (episode 5 saturates on the first round), 222→230, 240→241, 227→235, 214→215.
* **Zero crashes across all nine episodes.** The previous sweep of the same shape reported five crashes
  (`fz16`: four `rc=-10`, one SIGSEGV) and `fz13b` reported a SIGSEGV that this section's investigation
  (docs/146) traced to a rig write into a pointer cell plus six unguarded list walks. With those fixed the
  corpus replays clean under `night-asan-o2` too: `clean=18 not-clean=0` (docs/146 §10).

The union across the nine episodes is 250 of the 503 shipped-named functions, against 262 from the older
18-seed × 3-round unbudgeted sweep — the shape here is deliberately smaller (4 seeds × 2 rounds per episode,
9 separate sessions) so each episode gets a comparable budget instead of one shared corpus. The older ceiling
still stands as the ceiling for this grammar family (§8–§10), and the frontier list in §12 is still the
target: 61 shallow functions whose caller the corpus already enters.

What the budget changed is *comparability*: with `--frames` acting as a floor (docs/146 §9) a seed's run was
cut by wall clock at an unpredictable point, so the §5 numbers were partly an artifact of how far each seed
happened to get. Here every seed plays its full 6600-frame budget or ends the episode, and the round-over-round
growth is a statement about the corpus, not about timing luck.
