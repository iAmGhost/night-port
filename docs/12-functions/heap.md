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
