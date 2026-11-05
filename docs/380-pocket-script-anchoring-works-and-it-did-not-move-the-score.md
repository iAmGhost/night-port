# 380 — `--script` anchoring works on the Pocket path, and it did *not* move the score

docs/379 §5 named the next step: give `app_pc` the route file's anchor vocabulary so both sides are keyed to the game
rather than to a wall clock.  It is implemented, it works, and the comparison number did not budge.  Both facts are
worth recording.

## 1. What was built

`--script FILE` now understands `20000 wait res logo.pcx` alongside `key` lines.  The park is one number:

```
vpoll = polls - held          /* while the earliest open anchor is unsatisfied, held grows every poll */
```

so no later key step becomes due, and when the lookup happens `vpoll` resumes with the whole tail of the route
shifted by the slippage -- the same rule the SDL host applies by rebasing its slots (docs/55).

## 2. Two defects found while building it, both silent

- **`char dir[8]` with `"%7s"`**: the resource name `logo.pcx` is exactly 8 characters, so it was read as `logo.pc`.
  A branch keyed on `nf == 3` also read a fourth field `sscanf` had never filled.  Fixed to a 64-char field and a
  four-field line shape.
- **prefix matching instead of substring matching**: the first `res_seen` compared the wanted string only at the start
  of a recorded name, so a host reporting `NIGHT.RES:logo.pcx` could never anchor on `logo.pcx`.

## 3. The anchor source: the game's own trace, not the host hook

`sys_set_res_hook()` is the documented announce path (`src/sys/sys.c:826`, called from `fn_0367c9` and
`res_announce` in `f.c`), and the SDL host installs its own recorder with it (`src/app/main.c:1935`).  Installed on
the Pocket host it heard **nothing**:

```
script: `wait res logo.pcx` timed out after 8001 held polls (continuing)
script: 0 lookups heard, most recent:
script: `wait res opening.fli` timed out after 8002 held polls (continuing)
```

Every anchor timed out -- each park holding the timeline for the full 64 s ceiling -- and the run never advanced.
`src/app/main.c:1085` anchors on something else, and that is what works here too: `g_res_trace` (the last name handed
to `fn_0367c9`) and `g_res_trace_seq` (the counter of those lookups), both exported by `f.h`.  Sampling them on the
poll grid and recording each change gives the anchors something real to read:

```c
if (g_res_trace_seq != last_trace_seq) { last_trace_seq = g_res_trace_seq; script_res_hook(g_res_trace); }
```

The hook stays installed (it is the documented path and costs nothing), but the trace is what the anchors actually
use.  With that: **13 parks, 0 timeouts**, and the run gets to stage play -- `xfer=107 plays=107`, 11 947 frames at
+491 ticks/s, arena 3.90 MB.

## 4. The score did not move

Same original capture (434 labelled frames, docs/379), same oracle, Pocket side before and after anchoring:

| | wall-clock keys | anchored keys |
|---|---|---|
| route score | 0.5590 | **0.5590** |
| divergent runs (<0.88) | 4 | 4 (same windows, same worst scores) |
| distinct pictures on the port side | 2 049 | 1 436 |
| pacing ratio (orig/port picture changes per s) | 0.86 | 1.23 |

The timing clearly changed -- fewer distinct pictures, pacing moved from 14 % fast to 23 % slow -- and the score is
unchanged to four decimals, because the monotone alignment lands on the same picture pairs either way.  What that says
about the residual: **it is not a timing artifact**.  `diff_screen.py` asks whether the two sides pass through the
same *sequence* of pictures, and these two runs do not, in ways anchoring does not address: the original capture is a
live playthrough of `drive_orig_episode1.txt` (434 frames at 8 Hz, with the emulator's own waits), the Pocket run is
`route_pause_menu.txt` on this host's frame clock, and the alignment window is blind to labels.

## 5. The right basis for the next attempt

docs/91's number (348 of 517 original frames with a port counterpart at NCC ≥ 0.90) came from
`scripts/playtest_compare_orig_port.py`, which pairs **by label and by best match** (`--events`, `--window`) instead
of aligning blindly by order.  To give the Pocket path the same treatment it needs port-side PNG shots named by the
event that triggered them -- which `app_pc` does not write today (it writes `frames.bin`, `cells.bin`, `records.jsonl`
and the arena witnesses that docs/373 and the nine-episode sweep already score against shipped cards).  Adding
label-named PNG output to the shim's rig dump is the concrete next step, and it is a small one: the witness frames and
their labels are already in `cells.bin`/`records.jsonl`.

Card unchanged (0.1.67 -- this is harness work on the desktop shim and the host's script reader), fixture suite
`== ALL PASS ==`, `src/f` untouched.
