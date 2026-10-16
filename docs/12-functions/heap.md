# Heap allocator (0x042ada / 0x042b82 / 0x042b7d) — analysis

Status: **analyzed, not yet ported.** This is the game's own memory allocator
(DPMI-era malloc/free), reached from the public wrappers `FUN_00041af8`
(malloc) and `FUN_00041c31` (free). It is a core subsystem, so it gets a
focused, disassembly-verified port as its own sub-batch.

> **Caveat — decompiler unreliability on the wrappers.** Ghidra's decompilation
> of `FUN_00041af8`/`FUN_00041c31` is muddled by DPMI register-passing
> artifacts (`extraout_ECX`, `in_DS`) and conflates the heap-struct pointer with
> the block pointer. The internal allocator (`0x042ada`/`0x042b82`)
> decompiles reasonably, but the split/coalesce logic must be confirmed
> against the raw disassembly before porting. Do not port the wrappers from
> the decompiler text.

## Block layout

Each memory block begins with a 4-byte header word:

```
offset  0:  size | ALLOC_FLAG        (low bit = allocated flag)
offset  4:  prev free block (uint*)  [valid while the block is FREE]
offset  8:  next free block (uint*)  [valid while the block is FREE]
offset 12+: user data ...
```

- **Alloc flag (low bit):** `1` = allocated (live, can be freed); `0` = free
  (sits on the free list). `0x042b82` (free) bails out with `if ((*hdr & 1) == 0)
  return;`; `0x042ada` (malloc) sets `*block |= 1` after carving a block out.
- **Data pointer:** `malloc` returns `block + 4` (first byte of user data);
  `free` recovers the header as `ptr - 4`.
- **Free-list pointers live in the first 8 data bytes**, so they are valid only
  while the block is free. Minimum block = 4 (header) + 8 (prev/next) = `0xc`
  bytes — matching the `size < 0xc → size = 0xc` clamp in the wrapper.

## Free list

A circular, doubly-linked list of **free** blocks (low bit 0), ordered by
address. Sentinel node at `heap+0x20` terminates the walk. Each free block's
data bytes hold `prev` (offset 4) and `next` (offset 8).

## Heap-struct fields (the `param_3` pointer in malloc/free)

| offset | type   | meaning                              |
|--------|--------|--------------------------------------|
| 0x0c   | uint*  | root / first free block               |
| 0x10   | uint   | largest free-block size (cache)       |
| 0x14   | uint   | high-water / largest-seen size        |
| 0x18   | int    | live (allocated) block count          |
| 0x1c   | int    | free block count                      |
| 0x20   | node   | circular free-list sentinel           |
| 0x24   | uint*  | sentinel value                        |
| 0x28   | uint*  | last free block (best-fit start hint) |

Fields 0x00–0x0b are other allocator state (to be pinned down from the
disassembly / the wrapper's setup).

## Globals (allocator runtime state)

| address      | role                                             |
|--------------|--------------------------------------------------|
| DAT_00053350 | head of the block chain (first block / anchor)    |
| DAT_00053354 | "last block tried" cursor (malloc retry path)     |
| DAT_00053358 | high-water size, reset each malloc attempt        |

The public wrappers walk the block chain from DAT_00053350 via field [8]
(next), so **all blocks (free and used) are also linked through offset 8** —
the free list and the all-blocks chain share the same `next` slot. This
dual use must be confirmed from the disassembly; it is the trickiest part to
port faithfully.

## Functions

### 0x042ada — `heap_alloc(size, ds, heap)` (internal malloc)
First-fit search of the free list for a block with `size >= aligned`
(aligned = 4-byte-rounded size, min 0xc). On a hit it either:
- **uses the whole block** when `size - aligned < 0xc` (not enough left to
  split), unlinking it from the free list; or
- **splits**: the low part becomes the allocated block, the high remainder
  (`aligned` bytes in, rest carved) is inserted into the free list in place.
Sets the alloc flag (`*block |= 1`) and returns `block + 4`. Returns `NULL`
on failure.

### 0x042b82 — `heap_free(ptr, ds, heap)` (internal free)
Recovers the header (`ptr - 4`), checks the alloc flag, clears it, then
**coalesces** with the adjacent block(s) that are free and inserts the merged
block into the free list at the correct sorted position. Updates the free
count, block count, largest-size, and high-water fields. Contains an internal
rebalance (walk the free list while `live_count/(free_count+1) < free_count`
and `live_count - free_count <= free_count`, walking up to `2*that` free
blocks) — a classic fragmentation heuristic.

### 0x042b7d — trampoline
`JMP 0x00042c88`. A 2-byte tail-jump called from two points inside
`0x042b82` (free) at 0x042b8a / 0x042b95. `0x00042c88` is a label near the end
of the free routine (also reached by a conditional jump at 0x042c83). Ghidra
splits `0x042b7d` as a "function" and leaves `0x042c88` as a bare label
(decompile fails on it). In the port this trampoline collapses into the free
routine — it is **not** a separate logical function.

### 0x042c8d — `heap_unlink(block)` (free-list / all-blocks unlink)
Unlinks a block from the doubly-linked list (via offsets 4=prev, 8=next):
```
if (block == DAT_00053354) DAT_00053354 = *(block+8);  // advance cursor if needed
if (block == DAT_00053350) DAT_00053350 = *(block+8);  // advance head if needed
prev = *(block+4); next = *(block+8);
if (prev) *(prev+8) = next;   // prev->next = next
if (next) *(next+4) = prev;   // next->prev = prev
```
Uses the same `DAT_00053350` (head) / `DAT_00053354` (cursor) globals. The
offset-8 field is used here as BOTH the free-list next and the all-blocks next
(`prev->next = next`), confirming the dual-use of offset 8 noted above.

### 0x042e2d — `heap_insert_init(block)` (sorted insert + block init)
Inserts a block into the all-blocks chain (sorted by address, via offsets
4=prev, 8=next, head = `DAT_00053350`), then initializes the block's internal
metadata. **Block layout here differs from malloc/free**: a **44-byte (0x2c)
header** with self-pointers:
```
off 0 : size (word)          off 4 : prev   off 8 : next
off 12: -> off 32            off 32: 0      off 36: -> off 32
off 40: -> off 32            off 44: size - 0x2c
and *(block + size) = 0xffffffff   (end sentinel)
```
Returns the init pointer (`block + 0x2c`). This looks like a **separate
pool/arena** (44-byte-header blocks) from the 4-byte-header malloc/free heap —
open question for the sub-batch: does `0x042e2d` manage the same `DAT_00053350`
chain as malloc/free, or a distinct one?

### 0x042ea1 — `heap_top_check()` (top-of-heap / segment-limit query)
```
if (DAT_00053350 != 0) {
    int *p = *(int**)(DAT_00053350 + 0x24);   // head's field at off 36
    if (1 < DAT_000533b2 && DAT_000533b2 < 9 && DAT_000533b4 == 0) {
        u = SegmentLimit(in_DS);               // DPMI SegmentLimit
        DAT_00053384 = (ushort)u + 1;
    }
    if ((int)p + *p + 4 == DAT_00053384) return *p;   // returns a size
}
return 0;
```
Computes the address of the last byte of a block (`p + *p + 4`) and compares it
to `DAT_00053384` (a DPMI segment limit, cached). Returns the block size when
the block ends exactly at the segment limit (i.e. "is this the top-of-heap
block?"), else 0. Uses `DAT_00053350` (head), `DAT_000533b2`/`b4` (DPMI state),
`DAT_00053384` (segment limit).

### 0x04322c — `heap_grow(size)` — **ported (stub, returns 0)**
`XOR EAX,EAX; RET`. The outer memory-acquire of the malloc wrapper
`FUN_00041af8` (called when the free list is exhausted; `if (heap_grow(size)==0)
break;`). As a stub the heap never grows. Ported in batch 6 as a faithful
`return 0`; see `primitives.md`.

## Porting plan (next sub-batch)

1. Disassemble `0x042ada`, `0x042b82` fully; confirm block/field offsets and
   the shared offset-8 (free-list next == all-blocks next) question.
2. Define `typedef struct heap { ... } heap_t;` and `typedef struct heap_block`
   with the layout above; mirror globals as `g_heap_*`.
3. Port `heap_alloc` and `heap_free` 1:1 (the trampoline folds into free).
4. Differential test: build a small heap, run alloc/free sequences, and check
   block placement, coalescing, free-list order, and the counters against a
   reference model of the exact semantics (NOT libc malloc).
5. Port the public wrappers `FUN_00041af8`/`FUN_00041c31` only after the
   internals are verified, using disassembly (not the decompiler) for the
   retry / grow (`FUN_00043071`/`FUN_0004322c`) path.
