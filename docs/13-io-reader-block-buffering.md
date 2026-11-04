# The chunked resource reader is block-buffered — seek semantics and the test gotcha

Added 2026-07-13 while testing `fn_03f6bc` (the timed event player). An earlier draft of this note
claimed a port-side positioning **bug**; a minimal repro (`analysis/probes/io_pos_repro.c`) disproved it. What
follows is the verified behaviour — worth knowing before wiring the real CD/.IMG backend and before
writing any test that drives the reader twice.

## Verified model (repro output)

```
after open      pos=0   remaining=0   end=0    sysTell=0
after index     pos=148 remaining=132 end=124  sysTell=156      ; whole file (156 B) was block-read
after seek1     pos=148 remaining=132 end=124  sysTell=156      ; amount 0 -> in-buffer skip, NO lseek
after read1     pos=276 remaining=4   end=124                   ; +128 B consumed
after seek2     pos=148 remaining=132 end=124  sysTell=156      ; BACKWARD seek inside the block works
```

Field meanings (one node, two views):

* `+0x00` read cursor (**heap offset**), `+0x04` bytes still buffered, `+0x08` the **block base**
  (not an end), `+0x0c/0x0d` flags, `+0x10` handle, `+0x14` block size (4096 in the repro).
* `pos - end` = bytes consumed inside the block, so
  `fn_04247a` mode 0's `amount = value - (tell - remaining)` is exactly right, and the invariant
  **handle == logical + buffered** holds (156 = 24 + 132).

## Seek semantics (`fn_04247a` 0x424f0-0x42543 + `res_consume` 0x42438)

* `res_consume(amount)` succeeds only for `(end - pos) <= amount <= remaining`, i.e.
  `-consumed <= amount <= buffered`; on success `pos += amount; remaining -= amount` and it returns
  **0**; on failure it drains the block (`pos = end; remaining = 0`) and returns 1.
* `fn_04247a` mode 0: `if (res_consume(...) == 0) return 0;` — a successful in-buffer skip means **no
  lseek at all**. The `lseek(handle, value, mode)` only happens when the in-buffer skip failed, after
  which the next read refills the block from the handle.
* Consequence: **backward seeks are free inside the buffered block** (they just un-consume), and seeks
  beyond the block go through the drain + `lseek` + refill path. Both paths were exercised by the repro
  and reposition correctly.

## The gotcha that looks like a bug

The reader serves bytes from a **cached block copy**. Writing to the backing image (or, in the real
game, to the medium) after the block was filled is invisible until the block is exhausted and refilled
— verified: rewriting the record tag inside the image mid-stream did not change what the next read
returned.

That is why a test that rewrites a record between two reads of the *same* open stream sees stale
bytes. `test_f1.c`'s `f6bc_run` helper therefore re-opens the stream per case
(`sys_file_image_set` + `fn_04270d` + `fn_0366a4`) — the same pattern the original itself uses
(`fn_036853` re-opens resources; `fn_037b03` walks resources in index order and only restores a
position inside the buffered range with `fn_04247a(g_639b8, pos, 0)`).

## Notes for the CD/.IMG backend

* A block read must fetch `min(node->+0x14, bytes available)` and set `+0x04` to what was actually
  read; if the medium returns fewer bytes than requested, the reader treats the block as smaller (the
  repro's 156-byte image against a 4096-byte block).
* `+0x00` is stored as a **heap offset**, so the block buffer must live in the port's heap arena.
* Keep `handle == logical + buffered` after every refill, otherwise `fn_04247a` mode 0 mis-computes
  `amount` and silently skips the `lseek`.

## Repro build

```sh
cd port
cc -std=c11 -O0 -g -Isrc -o /tmp/io_pos_repro analysis/probes/io_pos_repro.c \
   src/f/f.c src/f/fx_04794a.c src/sys/sys.c -lm && /tmp/io_pos_repro
```
