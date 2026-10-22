# Open port issue: the chunked resource reader loses the handle/buffer position invariant

**Severity: blocks the real CD/.IMG backend and any random-access resource loading.**
Found 2026-07-13 while testing `fn_03f6bc` (0x03f6bc, the timed event player), which reads the same
resource stream twice. Repro: `scratch/io_pos_repro.c` (build at the bottom).

## Symptom

After `fn_0366a4` (index parse) has read 24 bytes from `g_639b8`, a `fn_0367c9("ev1.dat")` seek to
file offset 24 is accepted, but the following `fn_04192e(buf, 0x80, 1, node)` returns bytes from file
offset **48** (zeros in the repro image) instead of offset 24 (the record header). Re-seeking never
recovers: the second seek + read shows the identical off-by-24.

Observed node state (`res_reader`; the values are heap-relative):

```
after open      pos=0   remaining=0   end=0    id=0   flags=41/00  sysTell=0
after index     pos=148 remaining=132 end=124  id=0   flags=49/00  sysTell=156
after seek1     pos=148 remaining=132 end=124  id=0   flags=49/00  sysTell=156   (amount 0, no lseek)
after read1     pos=276 remaining=4   end=124                                     (+128 read)
```

`end` (+0x08) is the **buffer base**, `pos` (+0x00) the read cursor, `remaining` (+0x04) the bytes
still buffered -- so `pos - end = 24` bytes have been consumed from the block.

## The invariant the original keeps

`fn_04247a` mode 0 (absolute seek) — verified against `0x424f0-0x42543`:

```
cur     = fn_044848(id)                       ; tell(handle)
amount  = value - (cur - node->remaining)     ; logical = handle - buffered
if (res_consume(node, amount) == 0) return 0  ; in-buffer skip succeeded -> NO lseek
lseek(id, value, mode)                        ; only when the in-buffer skip failed
```

and `res_consume` (`0x42438`) succeeds only for `(end - pos) <= amount <= remaining`, i.e.
`-consumed <= amount <= buffered`; on success `pos += amount; remaining -= amount` and it **returns
0**; on failure it drains the buffer (`pos = end; remaining = 0`) and returns 1.

So the original depends on:

> **handle_position == logical_position + remaining**

(`pos - end` = bytes consumed inside the block.) In the repro that invariant is violated:
`cur - remaining = 156 - 132 = 24`, but the real logical position is **48** (the block starts at file
offset 24 and the cursor is 24 bytes into it). Hence `amount` is computed 24 too small, the in-buffer
skip "succeeds", no `lseek` is issued, and the read lands 24 bytes past the record.

## Where to look

* `fn_041d47` (the refill): sets `+0x00 = +0x08` (cursor := buffer base) and `+0x04 = bytes read`. If
  the handle was **not** at the logical position when the refill ran, the buffered block no longer
  starts at the logical position, and the invariant is broken for every later `fn_04247a`.
* `fn_04192e` / `fn_041c8f` (the consumers): they advance the cursor and decrement `+0x04`. Check
  whether any path reads through the handle directly (bypassing the buffer) — that would break the
  invariant exactly as observed (the 24 bytes read *before* the block was filled).
* `fn_04247a` itself is faithful: `je -> return 0 without lseek` is the original's behaviour, and it
  is why the game only ever seeks within the buffered block or right after an open.

## Consequences for the port

* Tests that pass today read resources **in index order from a freshly opened stream**, which is the
  pattern the original itself uses (`fn_037b03` walks resources sequentially and only restores a
  position with `fn_04247a(g_639b8, pos, 0)` inside the still-buffered range).
* `fn_03f6bc`'s test works around the gap: the `f6bc_run` helper re-opens `night.res` per case.
* Before wiring the real CD/.IMG backend — and before the planned `0x03a70e` slide-show test, which
  re-uses one stream — this invariant must be re-derived from disasm and fixed, otherwise
  multi-resource loads read at the wrong offsets.

## Repro build

```sh
cd port
cc -std=c11 -O0 -g -Isrc -o ../scratch/io_pos_repro ../scratch/io_pos_repro.c \
   src/f/f.c src/f/fx_04794a.c src/sys/sys.c -lm && ../scratch/io_pos_repro
```
