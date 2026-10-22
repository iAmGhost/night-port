# Real stage 1 loads end-to-end (master data → stage → hero spawn) — and the heap fix that unlocked it

Added 2026-07-13. With the real-file backend (`docs/14-real-resource-backend.md`) in place, the port now
runs `fn_03690b` (master data load) → `fn_037b03(0)` (stage load) → `fn_0376c8` (hero spawn) **against the
shipped `NIGHT/NIGHT.RES`**, inside the test suite (`test_real_master_load`, 29 checks; skips cleanly when
the container is absent, `NIGHT_RES_DIR` overrides the default `../NIGHT`).

## The blocker: fresh heap regions were 4 bytes too small

Getting that far required fixing a real port bug — the "heap_acquire_region under-provisions" issue that
had been open since the heap was first ported.

`heap_alloc` (`0x042ada`) demands a free block whose `B_SIZE` is at least its own rounded request:

```
0x42ae4  ADD  EDX, 7                  ; edx = size + 7
0x42ae9  AND  DL, 0xfc                ; round the LOW byte DOWN to a multiple of 4
         ...  EAX = (edx & 0xffffff00) | (low & 0xfc)      ; = floor4(size + 7)
0x42af9  CMP  EAX, [region+0x14]      ; vs high_water
0x42b0f  CMP  EAX, [block.size]       ; block must be >= eax
```

`heap_acquire_region` used to size the new region's data area as `round4(size)`, i.e. **smaller than the
very request that triggered it** (`floor4(size+7) > round4(size)` whenever `(size+7) & 3 >= ...`, e.g.
`size = 15770` → area 15772, request 15776). So the sequence *acquire-region → retry* always failed, and
any allocation that could not be served from existing slack returned 0 — silently, because the loaders
don't check. It stayed invisible under the synthetic fixtures (small allocations, big pre-seeded regions)
and under the 1 MiB arena cap.

Fix (`port/src/f/f.c`): size the data area with the same rounding `heap_alloc` uses:

```c
uint32_t t = size + 7;
uint32_t need = (t & 0xFFFFFF00u) | ((t & 0xFFu) & 0xFCu);   /* 0x42ae9 */
if (need < 0xc) need = 0xc;                                  /* 0x42aeb-af7 */
data_area = (need + 3) & ~3u;
```

After the fix the port allocates everything real data asks for (probe: 16 … 8,000,000 bytes all succeed),
and the DPMI substitution stays faithful — the original grows a whole DPMI block (`FUN_00043071`), so a
fresh region is always bigger than a single request; the port's region now satisfies that invariant too.

Two heap unit tests asserted the old formula and were updated to the corrected sizes (`0x2c + 0x1004` for
a `0x1000` request, coalesced block `0x1004`, no-split size `0x1004`). A third (`fn_0100d9: region full
before free`) had been passing *because* of the bug — `heap_malloc` returned 0 since fresh regions could
never be created — and now expresses its intent explicitly with `heap_arena_cap_set(0x300)`.

Corollary: `heap_arena_cap_set` now accepts caps down to the arena granularity (`0x40`) instead of
clamping to 4096.

## Second finding: the record cache is cross-test state

`DAT_63e14/63e18/63e1c/63e20` (the stage record cache: count / list head / tail / cursor) survive between
tests. Once allocations started succeeding, `fn_037b03` really appended record nodes (`fn_03e0a0`) to
whatever chain an earlier test left behind, and `fn_03ee0a`'s unlink walk followed stale links into
unrelated arena bytes → wild write / SIGBUS. The stage tests now `cache_reset()` (start from an empty
cache, which is what a fresh stage looks like) and `cache_restore()` afterwards.

Worth recording as a latent original bug: `fn_03e14c` unlinks a node with an **unguarded** write through
the neighbour links, so removing the last node of a list writes through offset `0` — in the original that
is a write to `DS:0x51/0x55` (below the data area), in the port into the reserved NULL-guard bytes.

## What the shipped stage 1 actually contains (all asserted now)

| item | real value |
|---|---|
| `stage1.wal` room count (`DAT_639d8`) | **4** |
| room chain | `DAT_6334c != 0`, `DAT_63354 == DAT_6334c` |
| `st1s.spr` stage sprite struct | loaded, header `7` / `0x63` (99) |
| `stage1.stg` record count (`DAT_63e14`) | **88** |
| `night1.chr` entry 0 first word | **100** (2-byte header skipped, 116-byte records) |
| per-type slots filled by stage 1 | `01 SA · 09 -A · 0b SA · 1f SA · 20 SA · 24 SA · 25 SA · 29 SA · 2a SA · 2e SA` |
| hero spawn (`fn_0376c8`) | `DAT_53c7c` != 0, `DAT_53c84` mirrors it |

Note the asymmetry at **type 0x09: an action table with no sprite struct**. That comes out of the real
kind-1 record walk (the loader loads whatever the record names), so some types legitimately have one
without the other — the earlier "sprite and action come in pairs" assumption only held for the fixture.

## Master-load facts (real container)

`fn_03690b` returns 1 and fills: 10 key tables (real counts, e.g. `gun.key` = **10** after leading blank
lines, `catch.key` = **2**); action containers `gun.act` = **192**, `gun1.act` = **196** records; the sprite
structs `pause 10 / system 11 / system2 13 / over 7 / face 92 / gage 3 / gun 266 (0x10a) / mark 76 (0x4c) /
dust 16`; all 100 `.voc` sound slots; `exptab.dat` row 0 = `300 1000 4000 5000 5000` (six rows) — i.e. the
numeric text loader (`0x032464` + `0x043a7f` + `0x041ffc`) parses the shipped text format correctly.

## Format confirmation from byte accounting

`pause.spr` (16,294 bytes) = 4 (count) + 10 × (0x30 header + 4 length) + 15,770 payload, matching the
pass-1 totals computed by `fn_03d9cf` byte for byte — the "0x30 header + length + payload" sprite model is
confirmed against real data, not just inferred from the disassembly.

## Suite

`make -C port test`: **2139 PASS, 0 FAIL** (was 2110), ASan clean. `TOTALS` unchanged (466/528 — this is
integration, not new functions).

## Next

* Real `.act` / `.spr` consumers in the entity path: run the entity constructor + the frame dispatchers on
  the real stage (types 0x01/0x0b/0x1f/0x20/0x24/0x25/0x29/0x2a/0x2e are the ones stage 1 actually spawns).
* `heap_grow` (`0x04322c`) is still a stub — with the sizing fix it is no longer needed for stage 1, but it
  should still be ported before the arena cap is treated as a hard limit.
* `fn_037347` teardown over real data (frees the real chains) — the natural follow-up check.
