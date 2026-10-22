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

## Open item: teardown over real data (`fn_037347`) faults in `heap_free`

`fn_037347` after a real stage load crashes (`heap_free` → wild write, SIGBUS / ASan BUS). It is *not*
caused by bogus offsets — a dry run of the teardown loop (`scratch/real_act_free.c`, committed as the
repro) prints, for the real stage 1 data:

* all 9 sprite structs with 14 valid sub-object offsets each, and
* all **1031** action records with valid `+0x6d` / `+0x71` / `+0x75` sub-object offsets, none zero
  (`heap_malloc` inside `fn_010181` never failed once the region sizing was fixed).

So the fault is **heap bookkeeping** — most plausibly a double free: `fn_037347` frees each slot's sprite
struct (`fn_03d999` + the struct itself) and its action container (`fn_0100d9` + the container), and one of
those was already freed (candidates: the `fn_0376c8` "resume" reload path re-entering `fn_037b03`, or
`fn_03de85`'s free-old-sub-objects step interacting with the `.spr`/`.act` pairing). The test therefore
does not call `fn_037347` on real data yet; the check is documented in place.

Next step for it: run `heap_verify` after each free phase (sprite sub-objects, sprite struct, action
records, action array, action container, room chain) and find the first invariant break.

## Suite

`make -C port test`: **2140 PASS, 0 FAIL** (was 2110), ASan clean. `TOTALS` unchanged (466/528 — this is
integration, not new functions). The real-data tests raise the heap cap to 48 MiB (physical arena is now
64 MiB); the default usable cap stays 1 MiB so every older test is unaffected.

## Next

* Real `.act` / `.spr` consumers in the entity path: run the entity constructor + the frame dispatchers on
  the real stage (types 0x01/0x0b/0x1f/0x20/0x24/0x25/0x29/0x2a/0x2e are the ones stage 1 actually spawns).
* `heap_grow` (`0x04322c`) is still a stub — with the sizing fix it is no longer needed for stage 1, but it
  should still be ported before the arena cap is treated as a hard limit.
* `fn_037347` teardown over real data — see "Open item" above (repro: `scratch/real_act_free.c`).
* Then the playable chain continues: the entity frame dispatchers and the AI handlers on the real stage 1
  types (01/0b/1f/20/24/25/29/2a/2e).
