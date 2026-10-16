# Heap allocator (two-level) — analysis & port

Status: **All heap functions ported and verified (batches 7–8); 118 `test_f1`
checks PASS.** This is the game's own DPMI-era memory allocator, reached from
the public wrappers `heap_malloc` (`0x041af8`, malloc) and `heap_free_host`
(`0x041c31`, free). Ported 1:1 from disassembly; verified by a reference model
that hand-traces the original's free-list invariants.

> **Decompiler caveats (verified against disassembly).**
> 1. The wrappers (`0x041af8`/`0x041c31`) decompile to nonsense (DPMI
>    register-passing artifacts: `extraout_ECX`, `in_DS`). They are ported from
>    the raw disassembly in this doc, NOT the decompiler.
> 2. Early decompiles of `heap_unlink` dropped the `next->prev = prev` write
>    and `heap_alloc`'s split path looked like it clobbered the region pointer.
>    Both were **re-verified against the disassembly and found to be clean**:
>    `heap_unlink` is a full double unlink, and the split path is a clean
>    in-place relink. See the per-function sections below.

## Two-level structure

There are **two linked lists using the same offset-4/8 (prev/next) fields on
different kinds of nodes**:

1. **All-blocks chain (regions):** a doubly-linked list of *regions* (sub-heaps).
   Head = `DAT_00053350` (`g_heap_head`), cursor = `DAT_00053354`
   (`g_heap_cursor`). Nodes linked via offset 8 (next) / offset 4 (prev),
   **NULL-terminated with `0`** (the port's arena reserves offset 0 so no
   region is ever at 0). `heap_unlink` and the public wrappers operate on
   this chain.
2. **Internal free list (per region):** each region holds a free list of small
   *free blocks*, walked via `next` (offset 8) until `next == &region[0x20]`
   (the **sentinel**, a 12-byte marker block embedded in the region header).
   `heap_alloc`/`heap_free` operate on this.

A **region** is a large node whose 44-byte (0x2c) header holds the free-list
metadata and whose data area holds the free blocks.

## Node layouts

Both a *region* and a *free block* begin with the same 12-byte header shape:

```
offset 0:  size  (bit 0 = allocated flag for free blocks)
offset 4:  prev  (uint32 offset)
offset 8:  next  (uint32 offset)
```

### Free block (the allocatable unit)
- **Alloc flag (low bit of offset 0):** `1` = allocated; `0` = free. `free`
  bails if `(*hdr & 1) == 0`; `alloc` sets `*block |= 1`.
- **Data pointer:** `alloc` returns `block + 4`; `free` recovers `ptr - 4`.
- **prev/next (offset 4/8) live in the first 8 data bytes** — valid only while
  free. Minimum block = 4 (size) + 8 (prev/next) = `0xc`, matching the
  `size < 0xc -> 0xc` clamp and the split threshold `remainder >= 0xc`.

### Region (heap struct)

| offset | meaning                                        |
|--------|------------------------------------------------|
| 0x00   | total size (header + data area)                |
| 0x04   | prev region (all-blocks chain)                 |
| 0x08   | next region (all-blocks chain, `0` = end)      |
| 0x0c   | root free block (free-list start)              |
| 0x10   | largest free-block size (cache)                |
| 0x14   | high-water / max allocatable size (limit)      |
| 0x18   | live (allocated) block count                   |
| 0x1c   | free block count                               |
| 0x20   | sentinel block (size@0x20, prev@0x24, next@0x28)|
| 0x2c   | data area start (first free block)             |
| 0x2c+total-0x2c | last free block; `[region+total] = 0xffffffff` end marker |

### Globals

| address      | port name          | role                                    |
|--------------|--------------------|-----------------------------------------|
| DAT_00053350 | `g_heap_head`      | all-blocks chain **head** (0 = empty)   |
| DAT_00053354 | `g_heap_cursor`    | all-blocks chain **cursor** (last tried)|
| DAT_00053358 | `g_heap_highwater` | high-water size, reset each malloc try  |
| DAT_00053384 | `g_heap_seg_limit` | cached DPMI segment limit               |
| DAT_000533b2 | `g_heap_cfg_b2`    | DPMI state byte (top-check gate)        |
| DAT_000533b4 | `g_heap_cfg_b4`    | DPMI state byte (top-check gate)        |
| DAT_00063fbd | `g_heap_flag`      | wrappers zero; never read               |

## Functions

### 0x042c8d — `heap_unlink(node)` — **PORTED (batch 7)**
Unlinks a node from the **all-blocks (region) chain**. Disassembly (0x042cb5..0x042cc6)
does a **full double unlink**, both directions NULL-guarded:
```c
void heap_unlink(uint32_t node) {
    if (g_heap_cursor == node) g_heap_cursor = B_NEXT(node);
    if (g_heap_head   == node) g_heap_head   = B_NEXT(node);
    uint32_t prev = B_PREV(node);
    uint32_t next = B_NEXT(node);
    if (prev) B_NEXT(prev) = next;     /* 0x042cbd/cbf */
    if (next) B_PREV(next) = prev;     /* 0x042cc2/cc6 */
}
```
**Correction (batch 7→8):** an earlier read (and the decompiler) omitted
`next->prev = prev`. The disassembly writes **both** `prev->next = next`
(offset 8 at `prev`) **and** `next->prev = prev` (offset 4 at `next`), each
NULL-guarded. Test: `test_f1.c` → PASS.

### 0x042ada — `heap_alloc(region, size)` — **PORTED (batch 8)**
Best-fit allocation within one region. Disassembly-verified (EAX = size,
EBX = region):
1. `aligned` — if `size == 0` or `size + 7` overflows 32 bits → return 0.
   Otherwise `aligned = (size+7)` with the **low byte masked by 0xfc**
   (`AND AL,0xfc` at 0x42ae9 — high bytes are preserved, low byte & 0xfc),
   then clamped to `max(aligned, 0xc)`.
2. If `aligned > region[0x14]` (high-water) → return 0.
3. Start the free-list walk at `region[0xc]` (root); if `aligned <=
   region[0x10]` (largest) instead start at `region[0x28]` (sentinel.next)
   and reset largest-tracking to 0.
4. Walk via `next` (offset 8), tracking the **largest** block seen (`EDI`);
   stop when `next == &region[0x20]` (sentinel). If a block's `size >=
   aligned`, it fits → go to (5). If the walk ends with no fit:
   `region[0x14] = largest; return 0`.
5. **Fit:** `remainder = block.size - aligned`; `region[0x10] = largest`
   (max of the *other* blocks — the fitted block's size is not tracked);
   `region[0x18]++` (alloc count); `next = block.next`.
   - **No split** (`remainder < 0xc`): fall to (6) with the whole block.
   - **Split** (`remainder >= 0xc`): `rem_block = block + aligned`;
     `region[0xc] = rem_block` (root = remainder); `rem_block.size = rem`;
     `block.size = aligned`; relink the remainder **in place** of the block
     (`rem.prev = prev`, `rem.next = next`, `prev.next = rem`, `next.prev =
     rem`), then (6).
6. **Common tail:** `region[0x1c]--` (free count); unlink `block` from the
   free list (`prev->next = next`, `next->prev = prev`, `region[0xc] = prev`);
   `block.size |= 1` (allocated); **return `block + 4`**.

**Correction (batch 8):** the split path is **clean**, not a bug. An early read
claimed `0x42b48` does `EBX = block.prev` (clobbering the region). The actual
sequence `0x42b48..0x42b59` reads `prev = block.prev` into a scratch register
and then relinks the remainder block (`rem`) in place of `block` — `0x42b59`
is `JMP 0x42b70` (the clean common tail). No region-pointer clobber, no
orphaned remainder. Ported 1:1 and verified.

### 0x042b82 — `heap_free(region, data)` — **PORTED (batch 8)**
Free one block (EAX = data ptr, EBX = region). Disassembly-verified:
1. `block = data - 4`; if `data == 0` (0x42b88 OR; 0x42b8a JZ) or
   `block.size & 1 == 0` (0x42b93 TEST AL,1; 0x42b95 JZ) → bail. (These two
   early exits route through the 0x042b7d epilogue, see below.)
2. Clear the alloc flag; `nxt = block + block.size`.
3. If `nxt` is free (bit 0 clear) or is the region's last block →
   **coalesce forward**: `block.size += nxt.size`; unlink `nxt`; `region[0x1c]--`.
   Otherwise **insert `block` at its sorted (address) position**, updating
   prev/next and the sentinel references.
4. **Rebalance heuristic:** while the free list is fragmented, walk up to
   `2*(live_count - free_count)` free blocks looking for further coalescing
   (absorb a predecessor whose end meets the block's start).
5. Update `region[0x18]` (live count), `region[0x10]` (largest), and
   `region[0x14]` (high-water).

The two early-exit branches at 0x42b8a/0x42b95 `JZ` to **0x042b7d**, a 5-byte
`JMP 0x042c88` thunk that lands on the routine's common epilogue
(`POP DS/ECX/EDI/ESI; RET` at 0x42c8c). The port folds that thunk into the two
`if (...) return;` statements.

### 0x042b7d — (free epilogue thunk) — **folds into `heap_free`**
`JMP 0x00042c88` — a short out-of-line branch reached by the two `JZ`s inside
`0x042b82` (the `data == 0` and not-allocated early exits). Not a logical
function; the port handles it as `heap_free`'s two `return`s. Counted in the
inventory and marked done (folded).

### 0x042e2d — `heap_insert_init(region)` — **PORTED (batch 8)**
Inserts `region` into the **all-blocks chain** (head `g_heap_head`, sorted by
address via offset 4/8) and initializes the **44-byte (0x2c) header**:
```
region[0x00] = total        (preset by caller)
region[0x04] = prev         region[0x08] = next     (sorted insert)
region[0x0c] = sentinel     (root = region+0x20)
region[0x10] = 0            (largest)
region[0x18] = 0            (alloc_count)
region[0x1c] = 0            (free_count)
region+0x20 .size = 0       (sentinel.size)
region+0x20 .prev = sentinel  .next = sentinel   (circular)
region+0x2c .size = total - 0x2c   (the one big free block0)
region+total .dword = 0xffffffff   (end marker just past the node)
returns region + 0x2c       (data start)
```
**Resolved (batch 8):** `0x042e2d` manages the **same all-blocks chain** as
`heap_alloc`/`heap_free` — it is the *region creator*, not a separate pool.
Called by the region-creation path (`0x042d2a`, `0x042fc2`, `0x043071`).

**DPMI zero-reliance:** the original does **NOT** initialize `region[0x14]`
(high-water). A freshly acquired DPMI region is zeroed-page memory, so
high-water starts at 0 and the acquire path sets it to `0xffffffff` explicitly
afterwards. The port's arena zeroes every allocated slot (see below) to
replicate that zeroed-page guarantee. `region[0x1c]` (free_count) **is**
explicitly written to 0 (0x42e80).

### 0x042ea1 — `heap_top_check()` — **PORTED (batch 8)**
Top-of-heap / DPMI segment-limit query:
```c
uint32_t heap_top_check(void) {
    uint32_t region = g_heap_head;
    if (region == 0) return 0;
    uint32_t last = R_SENT_PREV(region);     /* [head+0x24] */
    uint32_t size = B_SIZE(last);
    if (g_heap_cfg_b2 >= 2 && g_heap_cfg_b2 <= 8 && g_heap_cfg_b4 == 0)
        g_heap_seg_limit = (g_ds_segment + 1) * 0x10000u;  /* DPMI SegmentLimit */
    uint32_t end = (uint32_t)(last + size + 4);
    if (end != g_heap_seg_limit) return 0;
    return size;   /* this is the top-of-heap block */
}
```
Returns a block's size when it ends exactly at the cached DPMI segment limit
(`g_heap_seg_limit`), i.e. "is this the top-of-heap block?"; else 0. DPMI-
specific; the port maps the segment limit to `g_ds_segment` (flat model: 0).

### 0x04322c — `heap_grow(size)` — **PORTED (batch 6, stub → 0)**
`XOR EAX,EAX; RET`. The outer memory-acquire of `heap_malloc`. As a stub the
heap never grows via DPMI. See `primitives.md`.

## Region-creation pattern (the port's DPMI replacement)

The original acquires a region by asking DPMI for a fresh segment and calling
`heap_insert_init` on it. The port replaces the DPMI acquirer (`FUN_00043071`)
with **`heap_acquire_region(size)`**, which reproduces the observable
post-conditions using the static arena:
1. `data_area = max(0xc, (size+3) & ~3)`; `total = 0x2c + data_area`.
2. `region = heap_arena_alloc(total)` — place a 64-byte-aligned, **zeroed**
   slot in the arena (0 if no room).
3. preset `region[0x00] = total`; `heap_insert_init(region)`.
4. Mark the big block allocated: `hw(region+0x2c)[0] |= 1`;
   `R_ALLOC(region)++`; `R_HWM(region) = 0xffffffff`.
5. `heap_free_host(region + 0x2c + 4)` — free the big block so it is linked
   into the region's free list as the one initial free block.
Returns the usable data pointer.

## Public wrappers — **PORTED (batch 8)**

**`heap_malloc(size)` (`0x041af8`):**
1. Reject `size == 0` and `size > 0xffffffd4`.
2. `aligned = max(0xc, (size+3) & ~3)`; if `aligned <= g_heap_highwater`
   start the region walk at the head (resetting high-water to 0); else start
   at the cursor (falling back to head, resetting high-water).
3. For each region `r` in the chain (via offset 8): set `g_heap_cursor = r`;
   call `heap_alloc(r, size)`. On success, return. On failure,
   `g_heap_highwater = max(g_heap_highwater, r[0x14])` and advance.
4. When the chain is exhausted: try `heap_acquire_region(size)` (once, the
   DPMI-acquire replacement); then `heap_grow(size)` (stub → 0). If either
   returns non-zero, restart the walk; else return 0.

**`heap_free_host(data)` (`0x041c31`):**
1. Walk the chain from the head (via offset 8) to find the region `r` that
   contains `data` (`r <= data < next`); the last region is the fallback.
2. `heap_free(r, data)`.
3. If `r != g_heap_cursor`, `g_heap_highwater = max(g_heap_highwater,
   r[0x14])`.
4. `g_heap_flag = 0`.

## Port representation (32-bit → 64-bit)

The original stores 32-bit addresses at fixed offsets (size@0, prev@4, next@8,
data@4). The port keeps the **exact byte layout** but represents every heap
pointer as a **`uint32` offset from `g_heap_base`**, a 1 MiB static arena
(`s_heap_arena`). `hw(o)` = the 32-bit word at offset `o`; the field macros
(`B_SIZE/B_PREV/B_NEXT`, `R_ROOT/R_LARGEST/R_HWM/R_ALLOC/R_FREE/R_SENT*`)
address the layout directly. This is a faithful 32→64-bit pointer adaptation
(same offsets, same algorithm, same counters).

**NULL guard:** the region chain is NULL-terminated with `0`, but 0 is a valid
arena offset. The arena reserves the first 64 bytes (`s_arena_used` starts at
`0x40`) so no region/block is ever at offset 0, preserving the 0 = empty
convention.

**Zeroed slots (DPMI simulation):** DPMI returns freshly zeroed pages.
`heap_insert_init` relies on that for `region[0x14]` (high-water, never
written). `heap_arena_alloc` `memset`s each slot to 0 before returning it, so
the port replicates the zeroed-page guarantee.

**Alignment quirk:** `heap_alloc` aligns with a **low-byte-only mask**
(`AND AL,0xfc`): `aligned = (size+7)` with high bytes preserved and the low
byte & 0xfc, clamped to `>= 0xc`. This is NOT a 32-bit round-up:
`align(0x100) = 0x104`, `align(0x101) = 0x108`. The port replicates it exactly
(`(t & 0xFFFFFF00) | ((t & 0xFF) & 0xFC)`).

## Verification

`port/test/test_f1.c` (batch 8): `test_heap_arena`, `test_heap_insert_init`,
`test_heap_unlink`, `test_heap_alloc_free` (hand-traced reference model with
the original's low-byte alignment), `test_heap_top_check`, and `test_heap_stress`
(2000 random alloc/free ops with an invariant checker after every op). The
invariant checker walks the free list (`while (p != sentinel)`) and verifies
size/prev/next/count consistency. **118 checks PASS, 0 FAIL.**
