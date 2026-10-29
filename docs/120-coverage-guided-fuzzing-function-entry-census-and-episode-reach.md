
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
