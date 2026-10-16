# Heap allocator (two-level) — analysis

Status: **`heap_unlink` ported (batch 7); `heap_alloc` / `heap_free` / the
public wrappers analyzed but not yet ported.** This is the game's own
DPMI-era memory allocator, reached from the public wrappers `FUN_00041af8`
(malloc) and `FUN_00041c31` (free). It is a core subsystem and gets a
focused, disassembly-verified port as its own sub-batch.

> **Decompiler caveats (verified against disassembly).**
> 1. The wrappers (`FUN_00041af8`/`FUN_00041c31`) decompile to nonsense
>    (DPMI register-passing artifacts: `extraout_ECX`, `in_DS`). Do NOT port
>    them from the decompiler — use the raw disassembly in this doc.
> 2. `heap_unlink` decompile added a **phantom `next->prev = prev` write** that
>    is NOT in the disassembly (it only writes `prev->next = next`).
> 3. `heap_alloc`'s **split path has a latent heap-corruption bug**: it
>    clobbers the region pointer (EBX) and the tail then updates the wrong
>    object. See "heap_alloc split-path bug" below.

## Two-level structure

There are **two linked lists using the same offset-4/8 (prev/next) fields on
different kinds of nodes**:

1. **All-blocks chain (regions):** a doubly-linked list of *regions* (sub-heaps).
   Head = `DAT_00053350`, cursor = `DAT_00053354` (globals). Nodes linked via
   offset 8 (next) / offset 4 (prev). `heap_unlink` (0x042c8d) and the public
   wrappers operate on this chain.
2. **Internal free list (per region):** each region holds a free list of small
   *free blocks*. Root = `region[0x0c]`, nodes linked via offset 8/4, walked
   until `next == &region[0x20]` (the **sentinel**, a one-past-end marker
   embedded in the region header). `heap_alloc`/`heap_free` operate on this.

A **region** therefore looks like a large node whose header (offsets 0..0x28)
holds the free-list metadata, and whose data area holds the free blocks.

## Node layouts

Both a *region* and a *free block* begin with the same 12-byte header shape:

```
offset 0:  size  (bit 0 = allocated flag for free blocks)
offset 4:  prev  (uint ptr)
offset 8:  next  (uint ptr)
```

### Free block (the allocatable unit)
- **Alloc flag (low bit of offset 0):** `1` = allocated; `0` = free (on the
  free list). `free` bails if `(*hdr & 1) == 0`; `alloc` sets `*block |= 1`.
- **Data pointer:** `alloc` returns `block + 4`; `free` recovers `ptr - 4`.
- **prev/next (offset 4/8) live in the first 8 data bytes** — valid only while
  free. Minimum block = 4 (size) + 8 (prev/next) = `0xc`, matching the
  `size < 0xc → 0xc` clamp and the split threshold `remainder >= 0xc`.

### Region (heap struct, = EBX inside alloc/free)

| offset | meaning                                      |
|--------|----------------------------------------------|
| 0x04   | prev region (all-blocks chain)                |
| 0x08   | next region (all-blocks chain)                |
| 0x0c   | root free block (internal free list)          |
| 0x10   | largest free-block size (cache)               |
| 0x14   | high-water / max allocatable size (limit)     |
| 0x18   | live (allocated) block count                  |
| 0x1c   | free block count                              |
| 0x20   | free-list sentinel (walk stops when next==this)|
| 0x24   | sentinel value (compared in free's rebalance) |
| 0x28   | last free block (best-fit start hint)         |

### Globals

| address      | role                                            |
|--------------|-------------------------------------------------|
| DAT_00053350 | all-blocks chain **head** (port: `g_heap_head`) |
| DAT_00053354 | all-blocks chain **cursor** (port: `g_heap_cursor`) |
| DAT_00053358 | high-water size, reset each malloc attempt      |

## Functions

### 0x042c8d — `heap_unlink(node)` — **PORTED (batch 7)**
Unlinks a node from the **all-blocks (region) chain**:
```c
void heap_unlink(struct heap_node *n) {
    if (g_heap_cursor == n) g_heap_cursor = n->next;
    if (g_heap_head   == n) g_heap_head   = n->next;
    if (n->prev) n->prev->next = n->next;   // does NOT touch n->next->prev
}
```
Verified: the disassembly writes only `prev->next = next` (offset 8); the
decompiler's `next->prev = prev` is a phantom. Test: `test_f1.c` → PASS.

### 0x042ada — `heap_alloc(region, size)` (internal malloc)
Disassembly-verified (EAX = size, EBX = region):
1. `aligned = max(0xc, (size+7) & 0xfc-lowbyte)`; if `size == 0` or `size+7`
   overflows → return 0.
2. If `aligned > region[0x14]` (high-water) → return 0.
3. Start the free-list walk at `region[0xc]` (root); if `aligned <=
   region[0x10]` (largest) instead start at `region[0x28]` (last free) and
   reset the largest-tracking to 0.
4. Walk via `next` (offset 8), tracking the **largest** block seen (`EDI`);
   stop when `next == &region[0x20]` (sentinel). If a block's `size >=
   aligned`, it fits → go to (5). If the walk ends with no fit:
   `region[0x14] = largest; return 0`.
5. **Fit:** `remainder = block.size - aligned`; `region[0x10] = largest`
   (max of the *other* blocks — the fitted block's size is not tracked);
   `region[0x18]++` (alloc count); `next = block.next`.
   - **No split** (`remainder < 0xc`): fall to (6) with the whole block.
   - **Split** (`remainder >= 0xc`): `remainder_block = block + aligned`;
     `region[0xc] = remainder_block` (root = remainder); `remainder_block.size
     = remainder`; `block.size = aligned`; then (BUG, see below) link the
     remainder into the free list in place of `block`.
6. **Common tail** (EBX = region on the no-split path): `region[0x1c]--`
   (free count); unlink `block` from the free list
   (`prev->next = next`, `next->prev = prev`, `region[0xc] = prev`);
   `block.size |= 1` (allocated); **return `block + 4`**.

#### heap_alloc split-path bug (original latent bug)
In the split path, `0x42b48` does `EBX = block.prev` — **clobbering the region
pointer**. The common tail then runs with `EBX = prev` (a free block), so:
- `DEC [EBX+0x1c]` → decrements `prev[0x1c]` (a free block's data), not
  `region[0x1c]` (free count).
- `prev->next = next` (0x42b65) **overwrites** the `prev->next = remainder`
  set at 0x42b51 — the remainder block is orphaned from the free list (only
  reachable via the stale `region[0xc]`).
- `MOV [EBX+0xc], ESI` → writes `prev[0xc] = prev` (corrupts `prev`'s data),
  not `region[0xc]`.
This is only "benign" if the split block is the free-list root whose `prev` is
the sentinel (`region+0x20`), in which case the writes land on region fields.
The walk does not guarantee that, so this is a **real heap-corruption bug** in
the original. A faithful port must replicate it 1:1 (and we can offer a
"fixed" variant separately). **This is the main risk for the `heap_alloc`
port and needs a hand-traced reference test.**

### 0x042b82 — `heap_free(region, ptr)` (internal free)
Disassembly-verified (EAX = data ptr, EBX = region):
1. `block = ptr - 4`; if `block.size & 1` is 0 (already free) → bail.
2. Clear the alloc flag; `nxt = block + block.size`.
3. If `nxt` is free (or is the region's last block / at `region[0x24]`),
   **coalesce forward**: `block.size += nxt.size`; unlink `nxt`.
4. Otherwise **insert `block` into the free list** at its sorted (address)
   position, updating prev/next and the counts.
5. **Rebalance heuristic:** while the free list is fragmented
   (`live_count/(free_count+1) < free_count` and `live_count - free_count <=
   free_count`), walk up to `2*(that)` free blocks looking for further
   coalescing.
6. Update `region[0x18]` (live count, DEC), `region[0x10]` (largest), and
   `region[0x14]` (high-water). Contains a 2-byte trampoline `0x42b7d`
   (`JMP 0x42c88`) that folds into this routine (see trampoline note).

### 0x042b7d — trampoline
`JMP 0x00042c88` — a 2-byte tail-jump reached from two points inside
`0x042b82` (free). `0x00042c88` is the end-of-free label. Folds into free;
**not** a separate logical function.

### 0x042e2d — `heap_insert_init(block)` (sorted insert + 44-byte init)
Inserts a node into the all-blocks chain (head `DAT_00053350`, sorted by
address, offset 4/8) and initializes a **44-byte (0x2c) header**:
```
node[0] = total size      node[4] = prev     node[8] = next
node[12] = node+32        node[16] = 0       node[24] = 0
node[28] = 0              node[32] = 0       node[36] = node+32
node[40] = node+32        node[44] = total - 0x2c   (data size)
node[total] = 0xffffffff  (end sentinel just past the node)
```
Returns `block + 0x2c` (the data start). **Open question:** this 44-byte-header
layout differs from the 12-byte free-block header, so `0x042e2d` may manage a
*separate pool/arena* (not the malloc/free regions) even though it shares the
`DAT_00053350` head. Needs caller xrefs + the region-creation code to confirm.

### 0x042ea1 — `heap_top_check()` (top-of-heap / segment-limit query)
```c
if (DAT_00053350 != 0) {
    p = *(uint**)(DAT_00053350 + 0x24);       // head's field at off 36
    if (1 < DAT_000533b2 && DAT_000533b2 < 9 && DAT_000533b4 == 0) {
        u = SegmentLimit(in_DS);               // DPMI SegmentLimit
        DAT_00053384 = (ushort)u + 1;
    }
    if ((int)p + *p + 4 == DAT_00053384) return *p;  // block ends at limit
}
return 0;
```
Returns a block's size when it ends exactly at the cached DPMI segment limit
(`DAT_00053384`), i.e. "is this the top-of-heap block?"; else 0. Uses
`DAT_00053350` (head), `DAT_000533b2`/`b4` (DPMI state), `DAT_00053384`
(segment limit). `LSL EAX,EAX` on the `0x533b2==2..8 && 0x533b4==0` branch
computes the DS segment limit (DPMI real-segment size).

### 0x04322c — `heap_grow(size)` — **PORTED (batch 6, stub → 0)**
`XOR EAX,EAX; RET`. The outer memory-acquire of the malloc wrapper
`FUN_00041af8` (`if (heap_grow(size)==0) break;`). As a stub the heap never
grows. See `primitives.md`.

## Public wrappers (deferred — port from disassembly, NOT the decompiler)

**`FUN_00041af8` (malloc, EAX = size):**
1. Reject `size == 0` and `size >= 0xffffffd4`.
2. `rounded = max(0xc, (size+3) & 0xfffc-lowbyte)`; compare to
   `DAT_00053358` (high-water): if `rounded <= high-water`, start the region
   walk at the head; else start at the cursor (`DAT_00053354`, falling back to
   head if 0).
3. For each region `r` in the all-blocks chain (via offset 8): set
   `DAT_00053354 = r`; call `heap_alloc(r, size)`. On success, return.
   On failure, update `DAT_00053358 = max(DAT_00053358, r[0x14])` and advance.
4. When the chain is exhausted: call `FUN_00043071(size)` (inner acquire,
   once); then `heap_grow(size)` (stub → 0). If either returns non-zero,
   restart the walk; else return 0.

**`FUN_00041c31` (free, EAX = data ptr):**
1. Walk the all-blocks chain (from head, via offset 8) to find the region
   `r` that CONTAINS `ptr` (`head <= ptr <= r[?]`) — i.e. locate the owning
   region.
2. Call `heap_free(r, ptr)`.
3. If `r == DAT_00053354` (cursor), update `DAT_00053358 = max(DAT_00053358,
   r[0x14])`.

## Port layout decision (32-bit → 64-bit)
The original stores 32-bit addresses at fixed offsets (prev@4, next@8, data@4).
On a 64-bit host, pointers are 8 bytes. The port uses **`struct heap_node`
with 64-bit `prev`/`next` fields** (see `f.h`) — algorithm-identical, but the
byte layout and the `0xc` minimum / data-offset constants are adapted to the
struct. This is a sys-layer adaptation (address width), logged here so the
`heap_alloc`/`heap_free` port keeps the same representation.

## Porting plan (next sub-batch)
1. Hand-trace `heap_alloc` (both paths) and `heap_free` (coalesce + rebalance)
   on concrete inputs; build a **reference model of the exact original
   semantics** (including the split-path bug), NOT libc malloc.
2. Port `heap_alloc`/`heap_free` 1:1 (trampoline folds into free); port
   `heap_top_check` (map the DPMI SegmentLimit to a `sys_` hook returning the
   DS limit).
3. Decide `0x042e2d` (separate pool vs. same chain) from caller xrefs.
4. Port the wrappers `FUN_00041af8`/`FUN_00041c31` last (region search / grow).
5. Differential test: small region + free blocks, alloc/free sequences, check
   block placement, coalescing, free-list order, counters, and the
   split-path behavior against the reference model.
