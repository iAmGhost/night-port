# docs/146 — the crash hunt came back with seven rig defects, one of them inside the previous fix

docs/120 §13 queued one job: reproduce the fuzz sweep's SIGSEGV with a reproducible family subset and
localise it. Doing that (the subset is `zlib.crc32`-derived now, so it *is* reproducible) ended in a fault
stack in `fn_016fec`, and following that stack back produced seven separate findings — all of them in the rig
or in the port's null-modelling, none of them a mystery crash any more. Every one is verified by re-running
the same drive: `fz16/r2s4_st5` went from `rc=139` (plain) / `ASan SEGV` (sanitized) to **`rc=0` with 0
sanitizer reports**.

## 1. The census build had not built since the Makefile was touched

`make -C port censusapp` failed to link: `duplicate symbol '___cyg_profile_func_enter'`, then
`_g_fn_name` / `_g_fn_addr` / `_g_fn_count`. The recipe listed `src/app/rig_census.c` and
`src/gen/fn_table.c` explicitly *and* through `$(SRC_RIG)`, so each was compiled twice. Every fuzz number
quoted after that point came from the binary built before the breakage. Fixed by listing them once
(`port/Makefile`), and the same rule applied to the new `asanapp-o2` target below.

## 2. `poke` wrote raw integers into pointer cells — the whole docs/120 §6 "injection-induced SIGSEGV" family

`rig_ds_write()` did `memcpy(c->host, &v, host_size)` for every cell. For a **pointer** cell that means the
value 7 landed in the low half of a 64-bit host address, i.e. the cell became a wild pointer that the game
and the dumper both trusted. DS-side, a pointer cell holds a *uint32 arena offset* (f.h:418), so an injected
value has to be read the way the game reads it. `rig_ds_write` now resolves the value through `flat_ofs()`
and refuses it when it is not an object in the arena:

```
rig: poke 0x53c68=7 rejected: the +0x13f chain only reaches 0 entities
rig: poke 0x53c40 rejected: pointer cell, 268435456 is not an arena offset
```

The refusals are counted and reported at close (`pokes rejected (pointer cell)=…`), so a drive that tried to
inject an impossible pointer is visible instead of dying.

## 3. The dumper read 352 bytes from whatever a pointer cell happened to hold

```
==52497==ERROR: AddressSanitizer: global-buffer-overflow on address 0x0001031203e1
READ of size 352 at 0x0001031203e1 thread T0
    #1 0x000102f33960 in rig_poll rig_sink.c:236
0x0001031203e1 is located 63 bytes before global variable 'g_dat_53619' defined in 'src/f/f.c' (size 4)
```

`rig_poll()` copied a fixed `RIG_REC` (352 B) out of the object each pointer cell named, and out of every
entity slot, with no bound at all. Same for the entity-list walk. Now `rec_span()` answers "how many record
bytes are really inside the arena here", the copy is clamped to that, and a refused read leaves the block
zero — which the state oracle already reads as *no data* rather than as fabricated bytes:

```
rig: record reads refused (outside arena)=504  pokes rejected (pointer cell)=0
```

504 refused reads in one drive is a **blind spot the oracle now states out loud**, and it is the number to
watch when judging record-level verdicts.

## 4. The zero page was 64 bytes wide — the previous fix's own modelling was out of bounds

`sys_linear0()` (docs/128) returns a mapped zero buffer, and the port applies entity offsets to it: the list
links live at `+0x13f` / `+0x147`, and a record dump reaches `+0x160`. The buffer was 64 bytes, so every
zero-page substitution read past its own end. It is `g_linear0[0x1000]` now.

## 5. Seven count-bounded entity walks had no null modelling; the -O1 sanitizer build hid them

The port's own comment for `fn_0322f5` says the count `g_dat_53c68` is re-read every iteration, and docs/128
established that a freed entity keeps its stale `+0x13f` link — so the chain can end before the count does.
`fn_03203d` (draw sweep) and the sort at f.c:1427 model that with the zero page; **seven other walks did
not**: `fn_016fec`, `fn_032158`, `fn_0322f5`, `fn_03211f`, `fn_03209c`, and `fn_01441e` pass 1 and pass 3.

The reason this took so long to see: `asanapp` builds at `-O1`, and the app builds at `-O2`. The same drive
that faults in the `-O2` app *survives* under the `-O1` sanitizer build. The optimisation level, not the
sanitizer, decides whether the fault appears — so `port/Makefile` grew `asanapp-o2`, sanitizers at the app's
flags, and it is the build that reports these with a stack:

```
src/f/f.c:24170:54: runtime error: load of null pointer of type 'uint8_t'
    #0 in fn_0322f5 f.c:24170     #1 in fn_03a829 f.c:22935     #2 in fn_03af18 f.c:27147
```

All seven now go through one helper, `f_ent_walk()`, which substitutes the mapped zero page exactly as the
guest does and counts it (`g_walk_null_hits`), so "the list ran out before the count" is a measured fact per
site instead of a crash.

## 6. `fn_0112fd` applied an offset to a null neighbour

```
src/f/f.c:1376:38: runtime error: applying non-zero offset 327 to null pointer
```

`fn_0112fd` picks head / tail / middle by comparing the node against head and tail, then writes through the
neighbour link. When head/tail/count disagree with the links, the "middle" branch has a null neighbour.
Reading `[0 + 0x13f]` is what the guest does; applying an offset to a C null pointer is undefined. It now
routes through `f_list_link()` (zero page, counted as `g_list_null_links`) — and the witness **does fire** on
this drive:

```
ent-list: fn_0112fd middle neighbour link is null while the node is neither head or tail
```

so a genuine bookkeeping inconsistency is still open. It is the same territory as docs/128's dangling entity
pointer, and it is now named at the site rather than hidden behind a fault.

## 7. Injection policy: a count the chain cannot back is not a legal test state

`poke 0x53c68 = 7` on a list whose `+0x13f` chain reaches 0 entities is not a probe of the port — the
shipped game dies on the same arithmetic, because a zeroed "entity" fed into the record machinery sends
`RECST`/`YARR` (f.c:25751) to offsets no mapping contains. So the injection path now asks the question the
state oracle already asks (docs/137's count-vs-chain), and refuses:

```c
if (ds == 0x53c68) { uint32_t live = rig_chain_len(); if (v > live) { ...rejected... } }
```

This changes what the fuzz corpus is allowed to contain: drives keep only injections the shipped
bookkeeping could itself produce, which is the difference between "the port crashed" and "the rig wrote an
impossible state" — a distinction docs/120 §6 previously had to make by re-running drives by hand.

## After

| check | before | after |
|---|---|---|
| `fz16/r2s4_st5` under `night` (plain) | `rc=139` SEGV | — |
| `fz16/r2s4_st5` under `night-asan` (-O1) | ASan SEGV in `fn_016fec` | `rc=0` |
| `fz16/r2s4_st5` under `night-asan-o2` (-O2) | UBSan null deref `fn_0322f5` | **`rc=0`, 0 reports** |
| `make -C port test` | ALL PASS | ALL PASS |
| `make -C port censusapp` | link error | builds |

Open, honestly:
* `record reads refused (outside arena)=504` on this drive — which cells, and why their pointers are outside
  the arena, is the next record-oracle question.
* `g_list_null_links` firing means the list's head/tail/count disagreed with its links during a normal
  (uninjected-pointer) drive. That is a candidate real port defect, not rig noise, and stays open.
* The `[room-chain] fn_03cc6b got a NULL next link` witness still fires on this drive; the room chain is a
  separate walk with the same shape and has not been given the same treatment.

## 8. A valid array pointer is not a valid extent — and the extent is not the frame count

`fn_012616` indexes the action record's Y/X delta arrays with the entity's frame counter `e[+0x11]`; nothing
in the shipped code bounds it. Following the bus error (`BUS at f.c:25775`, reached from `fn_03209c`, on
fuzz drive `r0s1_st2`, which has **no injection verbs at all**) produced a witness with real numbers:

```
flat-heap: YARR index 49 outside the record's 3 frames; entity type=0 state=0x1
flat-heap: XARR move index 49 outside the record's 3 frames; entity type=0 state=0x1
```

i.e. the player's frame counter ran to 49, 50, ... against a record whose frame byte says 3.

**The first fix was wrong and is retracted here.** It bounded the index by `rec[+0x17]` — the byte the
shipped code itself compares the counter against when it advances the state — and the port's own fixtures
refused it:

```
[FAIL] record+0x71[frame] moves Y through fn_01545e and +0x75[frame] moves X through fn_01586b
[FAIL] the player's Y delta is also accumulated into room+0x1e (the camera drag)
[FAIL] fn_015295 folds e+0x3e into e+5 (clamped to 0) and the non-zero X delta clears e+0x16
[FAIL] the same branch accumulates the X delta into room+0x22 even before the rectangle test
```

Those four tests pin the shipped behaviour, and they index these arrays past `rec[+0x17]` legitimately. So
the frame count is **not** the array length, and a bound built on it changes behaviour: the witness went quiet
by suppressing real reads, which is the worst possible outcome for a differential rig. `f_arr_at()` now
performs any read the arena can map, exactly as written, and only answers from the zero page when the port
genuinely cannot map the address — counted as `g_flat_oob` and printed. Behaviour preserved, wild case named.

What remains open is the interesting half: *why* the player's counter reaches 49 against a 3-frame record.
`scripts/rig/frame_counter.py` was written to ask the shipped side the same question from its own record
dumps, and it currently reports `player records=0` on `/tmp/rig/g11` — the block-reading layer still uses the
offset heuristic docs/124 said to retire, so it cannot answer yet. Filed as a rig gap, not worked around.

## 9. `--frames N` was only a floor, so every fuzz cap was cutting runs at random

The pumped-frame check lived *after* the game main unwound (main.c:1560). Asking for 300 frames therefore
played the whole episode: **11 412 frames presented, 221 s wall** for `--frames 300`. Since presentation is
paced to wall time (60 fps), the wall-clock caps in the fuzz corpus were the only real bound, and they cut
each seed at an unpredictable point — which is what docs/120 §6–§8 recorded as `rc=124` seeds, and why
coverage per seed was never comparable between sweeps.

`g_frame_budget` makes the budget something the game main actually unwinds at:

```
frame budget: 300 presented, unwinding the game main
frames presented: 300        rc=0    elapsed 6 s      (was 11412 frames, 221 s)
```

Consequences: fuzz runs are comparable seed to seed, a minutes-cap means what it says, and the nine-episode
sanitizer soak becomes tractable.

## 10. Whole stored corpus under sanitizers at the app's own optimisation level

`scripts/rig/asan_o2_sweep.sh` replays every stored fuzz drive (each with the same `crc32`-derived family
subset fuzz.py used) through `night-asan-o2`, capped at 200 s each, four at a time:

```
== clean=18 not-clean=0 total=18 ==
```

0 AddressSanitizer errors, 0 UBSan diagnostics, 0 SEGV, 0 BUS across the corpus — against 17/18 before this
set of fixes. Two drives now report rejected injections instead of dying:

```
r2s4_st5     sanitizer=0 ended=1 pokes_rejected=2
r0s0_st1     sanitizer=0 ended=1 pokes_rejected=1
```

which is the intended shape: an injection the shipped bookkeeping cannot hold is *refused and counted*, not
survived as a crash or smuggled into the oracle as fabricated bytes.

## 11. Status of the seven findings

| # | finding | state |
|---|---|---|
| 1 | census build did not link (duplicate objects in the recipe) | fixed |
| 2 | `poke` wrote raw integers into pointer cells | fixed, refused + counted |
| 3 | dumper read 352 B from anything a pointer cell named | fixed, clamped + reported (1596 refusals on one long drive) |
| 4 | zero page was 64 B wide | fixed (0x1000) |
| 5 | seven unguarded count-bounded walks, invisible at -O1 | fixed via `f_ent_walk`, new `asanapp-o2` build exposes them |
| 6 | `fn_0112fd` offset applied to a null neighbour | fixed + witnessed; the inconsistency it witnesses **stays open** |
| 7 | injection could set a count the chain cannot back | refused against the live `+0x13f` chain |
| 8 | array extent guess (`rec[+0x17]`) | **retracted** after the fixtures rejected it |
| 9 | `--frames` was a floor, not a budget | fixed; 221 s → 6 s |
