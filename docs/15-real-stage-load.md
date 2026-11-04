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

Fix (`src/f/f.c`): size the data area with the same rounding `heap_alloc` uses:

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

## Second finding: the whole stage state is cross-test state — and the teardown frees what it finds

Not just the record cache: `DAT_63dfc` (stage sprite struct), `DAT_6334c` (room chain), `DAT_63354`
(current room), `DAT_63358`, `DAT_6335c` (room counter), `DAT_62a90` (record count) and the per-type tables
`DAT_639ec[0x104]` / `DAT_53818[0x104]`. `fn_0374ad` / `fn_037347` **free whatever they find there**, so a
stale value from an earlier test is freed as if it were live stage data → heap-metadata corruption and a
wild write (observed as SIGBUS inside `heap_free`). The real-data test now saves and zeroes all of it, i.e.
starts from the clean slate a fresh boot has, and restores it afterwards.

A concrete consequence of not doing that: the per-type slot dump first appeared to show stage 1 loading type
`09` with an action table but no sprite — that was a **leftover from an earlier test**. With clean tables the
real stage 1 fills exactly nine slots, all paired:

```
01 SA   0b SA   1f SA   20 SA   24 SA   25 SA   29 SA   2a SA   2e SA
```

(the counts are real: slots `1f/24/29/2e` carry 266-entry sprites and 192-record action tables — those types
name `gun.spr`/`gun.act`, so the loader loads a fresh copy per record rather than sharing the type-0 one).

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
| per-type slots filled by stage 1 | `01 0b 1f 20 24 25 29 2a 2e`, each with both `.spr` and `.act` |
| action record counts | 56 / 52 / 192 / 53 / 192 / 51 / 192 / 51 / 192 (1031 records total) |
| hero spawn (`fn_0376c8`) | `DAT_53c7c` != 0, `DAT_53c84` mirrors it |
| config seed (`fn_0374ad`) | `DAT_53c7c[0x17] == 1`, `DAT_53c7c[0x11] == 0`, `DAT_63354 == DAT_6334c` |

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

## The teardown fault was the allocator substitution, not the game data (RESOLVED)

`fn_037347` after a real stage load faulted inside `heap_free`. It was never a game-data problem:

* a dry run of the teardown loop validated **every** block it is about to free — 4293 blocks, including
  all 1031 action records and their `+0x6d`/`+0x71`/`+0x75` sub-objects and the 14 sub-objects of every
  sprite struct: **0 anomalies** (each offset lands inside its own region, its header says allocated, its
  size is plausible and does not overrun the region);
* but the region chain under the port's default geometry held **7,450 regions** for one stage, almost all
  of them `total = 0x38` — one per allocation — and walking that chain eventually ran into a non-region
  offset. The corruption is in the arena substitution's own bookkeeping under a geometry the original
  cannot produce, and it surfaced as a wild write in a later `heap_free`.

Cause: `heap_acquire_region` sized a new region to exactly the request that triggered it. The original's
acquirer (`FUN_00043071`) asks DPMI for a whole memory block, far larger than the request, so a fresh
region always has room left over. With per-request regions, `heap_malloc` can never reuse one and every
allocation grows the chain.

Fix: a granularity knob, `heap_region_floor_set(bytes)` (default 0 = the old per-request sizing, which the
allocator unit tests pin). The real-container tests set a 64 KiB floor, mirroring a DPMI block. Effect on
the same real stage-1 load: **regions 7,450 -> 34**, and the teardown now runs cleanly — pinned by two new
allocator checks and five teardown checks:

```
regions in chain: 34          (floor 0x10000; 7450 with per-request sizing)
checked=4293 anomalies=0
fn_037347 survived
```

The stage lifecycle is therefore fully covered over shipped data: `fn_03690b` -> `fn_0374ad` ->
`fn_0376c8` -> `fn_037347`, including the original's quirk that `DAT_63354` is left dangling at the freed
room chain.

Debug probes kept for reference: `analysis/probes/real_chain_probe.c` (region chain walk + anomalies),
`analysis/probes/real_block_probe.c` (block-header validation of everything a teardown frees),
`analysis/probes/real_teardown_probe.c` (free-order replica with a double-free bitset), `analysis/probes/floor_probe.c`
(region granularity), `analysis/probes/real_act_free.c` (first repro).

## Suite

`make test`: **2147 PASS, 0 FAIL** (was 2110 before this work), ASan clean. `TOTALS` unchanged
(466/528 — this is integration, not new functions). The real-data tests raise the heap cap to 48 MiB
(physical arena 64 MiB) and the region floor to 64 KiB; the defaults stay 1 MiB / per-request sizing so
every older test is unaffected.

## Next

* Real `.act` / `.spr` consumers in the entity path: run the entity constructor + the frame dispatchers on
  the real stage (types 0x01/0x0b/0x1f/0x20/0x24/0x25/0x29/0x2a/0x2e are the ones stage 1 actually spawns).
* `heap_grow` (`0x04322c`) is still a stub — with the sizing fix it is no longer needed for stage 1, but it
  should still be ported before the arena cap is treated as a hard limit.
* The playable chain continues: the entity frame dispatchers and the AI handlers on the real stage 1
  types (01/0b/1f/20/24/25/29/2a/2e).
