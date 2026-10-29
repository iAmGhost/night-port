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
