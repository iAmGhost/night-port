# 0x03c853 — the recursive stage / room node loader

Ported as `fn_03c853` in `port/src/f/f.c`; verified by `test_fn_03c853` (12 checks, two-room chain).
Size 639 B, L12. It was the last dependency blocking the L15 stage loaders `0x0374ad` / `0x0376c8`
(both call `0x037b03`, whose only unported callee was this one).

## ABI

`PUSH 0x30; CALL 0x0418ee` guard, then **EAX = stage node** (saved `[EBP-8]`), **EDX = io node**
(`[EBP-4]`). Reads come from `fn_041c8f` (single bytes) and `fn_04192e(dest, EDX = count,
EBX = element size, ECX = io node)`.

## What one node holds (fields written)

| offset | source |
|---|---|
| `+0x00, +0x04, +0x08, +0x0c, +0x12, +0x16` | one dword each off the stream |
| `+0x10, +0x11` | one byte each off the stream (`+0x10` selects the geometry branch) |
| `+0x1a` | `+0x08 - +0x00` (row count) |
| `+0x26` | constant 1 |
| `+0x40` | `DAT_000639e8++` (monotonic node index) |
| `+0x348` | allocation offset: `rows * 0xc8` when `+0x10 == 0`, else `(+0x0c - +0x04) * 0x140`, then that many bytes are streamed into it |
| `+0x48` | 3 × 0x100 = 768 bytes (three tables) |
| `+0x3c` | dword; when non-zero an extra `2 * N`-byte block is allocated at `+0x44` and streamed (`count 2 × element N`) |
| `+0x34c` | next room (host pointer) |
| `+0x354` | previous room (host pointer) — see the slot shift below |

## Chain and termination

```
++DAT_0006335c
if (DAT_0006335c == DAT_000639d8) node[0x34c] = 0;       /* DAT_639d8 = room count */
else { child = heap_malloc(0x354); child = child ? fn_03c6ec(child) : child;
       node[0x34c] = child; child[0x350] = node; recurse(child); }
```
So the recursion is *not* data-driven: it stops when the loaded-room counter reaches the stage's
room count, which the caller (`0x037b03`) installs in `DAT_000639d8`.

## Port-model decisions (important for the next ports)

* **Slot shift for the second link.** The original link pair is `+0x34c` (next) / `+0x350` (back),
  only 4 bytes apart. Widening them to host pointers would overlap, so the port keeps `next` at
  `+0x34c` (already read that way by `fn_03d88f`) and moves the back pointer to **`+0x354`**. The
  two existing readers in `fn_03ce0b` (orig `e[0x350]`) and their tests were moved to `+0x354` for
  coherence. Third occurrence of this widening clash (after the entity list `+0x13f`/`+0x147` and
  the `emit_ctx` tail field).
* **Node allocation size.** The original asks `0x041bdc` (the `heap_malloc` wrapper) for `0x354`
  bytes; the port asks for **0x35c** so the shifted back pointer stays inside the block.
* **Struct fields keep heap offsets** (`+0x348`, `+0x44`), while globals keep host pointers —
  consistent with `fn_01435f`/`fn_03e0a0`. The stream reads are aimed at `g_heap_base + off`.
* **Latent original bug, deliberately not replicated:** if the child allocation fails, the original
  stores through the null child (a DOS low-memory write at `DS:0x350`) and recurses with a null
  node. There is no DOS low-memory mirror in the port, so the store and the null recursion are
  skipped; the success path is identical.

## Test notes

A single synthetic stream feeds both rooms; `DAT_639d8 = 2` pins the gate so exactly one child is
allocated by the loader itself, and the chain (parent `+0x34c` → child, child `+0x354` → parent,
child `+0x34c` = 0) is asserted directly. `DAT_000639e8` is asserted to advance once per node.
