# 215 — Item 2a screen half (superseded in part): the port's cluster is real; the shipped "no ink" table was computed with the wrong frame stride and is void (docs/218 §1-3)

> **§2 of this document is void.**  The shipped `frame.bin` payload is 2560 bytes per row (15-byte header
> `320, 200, 0xa00`), and the region diffs quoted below indexed rows as `y*320*4`.  With the correct stride the
> shipped's kill number *is* in the presented frame, at the coordinates its own arithmetic predicts, rising and
> dying on schedule -- see docs/218 §3.  §1 (the port side) and §3 (the digit-index arithmetic is faithful) stand.


## 1. Port: the kill number is visible and rises, at the witnessed coordinates

`/tmp/rig/v2` (drive `ep1_kill3`, unpinned clock) has `DS 0x639c8` (the floating-number list count) non-zero in 36
samples, first at t = 246 294 ms.  Consecutive saved 320x200 frames inside that window, differenced in the box
x 4..30, y 70..120:

```
frames 8995(t=246497) -> 9003(t=246647): 57 changed px
   y=87 ........#....##............          x = 12      x = 17,18
   y=88 ........##..####...........          x = 12,13   x = 17..20
   ...  (rows 88..96 hold the same two columns)
   y=97 ........##...###...........
frames 9003(t=246651) -> 9013(t=246858): 48 changed px
   y=84 ........#....##............   <- the same cluster, 3 rows higher
   ...
   y=93 ........##...###...........
```

Two glyph columns at x 12..13 and x 17..20 -- exactly the witnessed `tag=valuedisp dest=11` and `dest=16` with
`col=4`, and the cluster rises 3 rows per ~200 ms, which is `fn_03b718`'s aging (`DAT_00063360 % 3` -> `+0x05++`,
`start_row = dy - 0xf` shrinking).  So the field item 2a is about is at **x 11..19, y 82..100 of the 320x200 canvas**,
drawn by `fn_03b718` (`port/src/f/f.c:14516`), posted by `fn_015295`'s kill branch (0x000153fe-0x00015432), walked by
`fn_03b953` (0x0003b953/0x0003b99a).  The left glyph is a 2-px column and the right glyph is a 4-px column that is
solid in most rows with a `#.#`/`###` hole in three rows -- a "0" with its middle mostly filled in.

## 2. Shipped: the same box is unchanged for the whole life of the record

The same box on the shipped side, `/tmp/rig/ents-orig/orig`, states 362..371 (the record exists from 363 to 369 per
`DS 0x639c8`, and docs/214 shows it lives and dies with no key event):

```
362->363: 0 changed px      366->367: 0        369->370: 6 (y=118 only, x=12 and 17,18)
363->364: 0                 367->368: 0        370->371: 0
364->365: 0                 368->369: 6 (same 6 px)
365->366: 0
```

The whole frames *do* change state-to-state (docs/214 §3 lists the per-state half hashes), so this is not a stale
capture: the shipped simply has nothing moving at the coordinates its own record says the number is at
(`+0x09 = 11` x, `+0x0d = 113` y, age 3 -> start row 95).  The only change in the box during the popup's life is a
2-row, 6-pixel mark at y = 118 -- 23 rows *below* the computed position, in the same two columns.

## 3. The glyph-index arithmetic is faithful (one more hypothesis closed)

Shipped bytes for the digit blit, 0x0003b795-0x0003b7db, against `port/src/f/f.c:14532-14547`:

```
0x3b795  mov eax, [eax + 0x34]            ; the control block's glyph-offset object
0x3b798  add eax, edx                     ; + digit*4   (0xc0 = '0'*4 subtracted below)
0x3b79a  mov edx, [0x139c0]               ; DAT_000639c0 (font A control block)
0x3b7a0  mov edx, [edx + 0x38]            ; RLE base field
0x3b7a3  add edx, [eax - 0xc0]            ; + *(char_obj + digit*4 - 0xc0)
0x3b7aa  push 7 ; mov ecx, 4              ; rows 7, col 4
0x3b7b4  eax = [ent+0xd] ; sub eax, [ent+5] ; sub eax, [0x139e0] ; ebx = eax - 0xf     ; start_row
0x3b7c9  edx = i*5 ; add edx, [ent+9] ; sub edx, [0x139dc]                             ; dest
0x3b7db  call 0x3bb42
```

The port reproduces every term, including the `-0xc0` ASCII fold.  The font sources also check out as shipped
names: the port builds font A/B as `spr_block("system.spr")` / `spr_block("system2.spr")` (`f.c:2819-2820`), and the
shipped bringup trace requests `system.spr`, `system2.spr`, `number1.spr`, `number2.spr`, `number3.spr`; `number1.spr`,
`number2.spr`, `number3.spr` are present in `./NIGHT`.

## 4. What is localized, and what is still open

Localized for item 2a: the field (x 11..19, y 82..100 of the 320x200 canvas), the draw site (`fn_03b718`), the post
site (`fn_015295`), the value (10 on both sides, docs/211), the font (A: 5-px advance, 7 rows, col 4 on both sides),
and a concrete rendered disagreement: the port paints a glyph pair whose right glyph is a nearly solid 4x7 block
while the shipped paints nothing at those coordinates in its presented frame.  Still open: whether the shipped's
`fn_03b953` passes a blit base that is not the presented canvas (the back-buffer chain of docs/110/`fn_03ace9`,
whose `mem_copy(A0000 <- 633b4, 0xfa00)` at 0x0003add5 is the presentation step) -- that is the remaining
candidate, and it is a *base address*, not an offset inside the glyph table.  Verifying it needs the shipped's
per-composite buffer identity, which the current capture cannot name; the honest rig status for item 2a is
DETECTED-AND-LOCALIZED on the port half, RIG-GAP on which shipped buffer the number lands in.
