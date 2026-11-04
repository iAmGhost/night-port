# 181 — The event table was the missing entry mechanism (and `make test` never built the app)

Three state rows had resisted every causal attempt: `DS 0x53451`, `DS 0x64090`, `DS 0x64094` held values in the
shipped run and stayed zero in the port.  docs/179 named their writers, docs/180 ruled out fall-through.  The answer
was in front of the audit the whole time: **`DS 0x537dc` is a function-pointer table, and the port registers those
routines in it with a `NULL` handler.**

## 1. The shipped startup calls a dispatcher, not the routines

`branch_xrefs.py` (new, `scripts/audit/branch_xrefs.py`) walks *every* terminator edge -- CALL, JMP and conditional
jumps -- out of all 523 modelled bodies.  For `0x4589e` and `0x4933a`: **zero incoming edges**, in any form.  It also
finds the startup spine:

```
fn_0420bc  (the DOS4GW startup body, 0x420bc..0x422d5)
   0x422b7  MOV [DAT_00053388], EAX      <- "source command line" heap offset
   0x422bc  MOV [DAT_0005338C], ...
   0x422c7  CALL 0x44618                 <- dispatch_a(0xff)   "run the registered table entries"
   0x422ce  CALL 0x445C9                 <- fn_0445c9, the game main chain
```

`dispatch_a` (0x44618..0x44663) scans `DS 0x537dc..0x5380c` in 6-byte entries, picks the last entry whose key passes,
calls its function pointer, marks the entry flag 2, and re-scans.  A routine registered there has no CALL site
anywhere in `NIGHT.EXE` and still runs before the title screen -- which is exactly the pattern docs/179/§180 kept
failing to explain.

The tool had one bug of its own first: `body_start`/`body_end` are both flat addresses, and the first version sliced
`image[off : end - off]`, i.e. ran every body far past its end.  It reported "distinct branch targets: 0" across 523
bodies, which is impossible for this binary; corrected to `end - CODE_BASE`, it finds 7 565 targets and reproduces
known edges (`fn_010010 -> 0x10181`, `fn_0420bc -> 0x445c9`).

## 2. The shipped table, read out of a runtime dump

```
DS 0x537dc, 8 entries of 6 bytes:
  flag key  fn        flag key  fn        flag key  fn        flag key  fn
   2   32  0x1fb8e6    2   32  0x1fd702    2    1  0x1fbf9c    2    2  0x1fd9bd
   2   32  0x1ff61d    2   32  0x1ff89e    2   32  0x20333a    2   32  0x204a49
```

The keys match the port's table comments byte for byte (`0x20, 0x20, 0x01, 0x02, 0x20, 0x20, 0x20, 0x20`), and every
flag is already 2 in the first shipped memory sample -- dispatch had consumed all eight entries before the game was
playable.  The port's own comment in `f.c` lists the guest address of each handler, and two of them are the two
"uncalled" routines:

```c
{ 0, 0x20, NULL },  /* 0x537fa: 0x4589e */     { 0, 0x20, NULL },  /* 0x53800: 0x4933a */
```

The comment even said why: "fn pointers are wired to the ported handlers once they land (NULL = not yet ported)".
Both handlers *had* landed (`fn_04589e` at `f.c:7288`, `fn_04933a` at `f.c:5693`); nobody went back to the table.

## 3. Why one NULL silently disables the whole table

`dispatch_a`'s tail is `OR EAX,EAX; JZ exit` -- a NULL handler **breaks out of the dispatch loop**, it does not skip
the entry.  Since the scan keeps the *last* passing index, a NULL at a high index is chosen first and aborts
everything below it.  With the table as written (all NULL), `dispatch_a(0xff)` did nothing no matter how many
handlers were wired.

The fix keeps unmodelled entries honest rather than invented: an unwired entry starts at **flag 2**, the same state
the shipped table shows after dispatch consumed it, so dispatch skips it exactly as it would have after executing it.
Only the two entries with real handlers start at flag 0:

```c
{ 0, 0x20, fn_04589e },  /* 0x537fa: 0x4589e -- publishes DAT_00053378/0x5337c then DAT_00064090/0x64094 */
{ 0, 0x20, fn_04933a },  /* 0x53800: 0x4933a -- the only modelled writer of DAT_00053451 */
```

and `main.c` now reproduces the shipped order (`dispatch_a(0xff)` at `0x422c7` **before** `fn_0445c9_startup()`, which
models `0x445d8-0x445fe`, because shipped calls the dispatcher first).  A witness (`NIGHT_EVTRACE=1`) prints the
selection so the mechanism is observable rather than assumed:

```
evtrace scan best=6 fn=0x10287d470   evtrace call idx=6
evtrace scan best=5 fn=0x10287d1d4   evtrace call idx=5
evtrace scan best=-1                 evtrace exit: no entry
```

## 4. Red to green, canonical route (`/tmp/rig/g33`, ep1_kill3, 150 s, `RIG_HZ=10`)

| cell | shipped, first sample | port before | port after |
|---|---|---|---|
| `DS 0x53378` | `0x1` | `0x0` | **`0x1`** |
| `DS 0x64090` | `0x1` | `0x0` | **`0x1`** |
| `DS 0x5337c` | `0x19d0d1` | `0x0` | `0x4075` (own-arena heap offset) |
| `DS 0x64094` | `0x19d0d1` | `0x0` | `0x4075` -- and it equals the port's own `0x5337c`, the same relation shipped shows |
| `DS 0x53451` | `0x2000004` | `0x0` | `0x4` (shipped carries one extra `0x2000000` bit) |

`residual_shape.py` on the new route: **25 residual rows, down from 28** (14 same-value-set/different order-dwell,
5 partial overlap, 3 disjoint, 1 port-all-zero, 1 port-holds-value, 1 monotone-stamp).  The "port holds a constant
zero" cluster collapsed from four rows to one: `DS 0x62d5c`, whose cause is separate and still open -- the seven
missing `fn_03272f` stores of docs/179 §1.  `0x64094` moved to the base-relative class, where it is comparable,
instead of the always-zero class, where it was not.

## 5. The rig defect this uncovered: `make test` never built the app

While checking why the fix appeared to do nothing, `build/night` turned out to be **four hours older than
`src/f/f.c`** while `make test` printed `== ALL PASS ==`:

```
20: test: $(T)/test_f1          # builds and runs the fixtures only
37: app: $(T)/night             # the rig binary lives on a different target
```

`route.sh` has a stale-binary guard (added after docs/133 §7-9), but any direct `./build/night` run bypasses it,
and a green suite is not evidence that the measured binary contains the edit.  `test` now depends on `$(T)/night` as
well, so a stale app binary cannot produce a passing suite.  Confirmed with `strings -a build/night | grep -c
evtrace` going 0 -> 4 before the measurement in §4 was believed.

## 6. What is still open in this family

Six table entries have no port counterpart (`0x43702`, `0x41f9c`, `0x439bd`, `0x493b2`) or a counterpart that is not
yet wired (`0x418e6` canary_ss_save, `0x41fa1`), and one entry -- `0x53806 -> 0x4aa49` -- points into the region
`0x4aa48..0x4e757`, which is **not code at all**: it is 15.6 KB of resource-name strings (`cdinsert.pcx`,
`boss1.spr`, `4@gun.spr`, ...), found while looking for unmodelled code.  Wiring `0x4561d` (`calibrate_clock`) is
deliberately excluded from this change: it would change the game clock rate, and docs/168's calibration
(`NIGHT_RIG_STEP_CALLS=665`) should be re-derived against it as its own experiment, not folded into this fix.

Also open from this run: the original-side **screen capture session failed** (`IndexError: index out of range` after
"boot + capture"), so `report_audio.txt` is absent and the pacing line in this route's report (orig 0.99 vs port
26.67 changes/s) comes from a partial capture.  That is a rig failure to fix before screen/audio claims are quoted
from `g33`.
