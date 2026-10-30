# 221 — Item 2a found: the EXP-label blit reads `+0x2c` (array index 11) where the shipped reads `+0x28` (index 10); the port paints a 4x49 garbage column where the shipped paints a 13x6 label

Gate: `gate3-exp-text`, measured on the **unfixed** build (frozen baseline docs/182, git `574c636`; only a
`NIGHT_GLYPHDBG` diagnostic was added to reach the measurement, no behavior change).

## 1. What fn_03b718 actually does: digits, then a label

`fn_03b718` (guest 0x0003b718-0x0003b94e, port `port/src/f/f.c:14516`) has four `call 0x3bb42` sites: a
per-digit loop and a **tail call**, in each of the two font branches.  The tail call (font A at
0x0003b7e2-0x0003b836, font B at 0x0003b8bb-0x0003b90f) is not a digit:

```
0x3b7ea  edx = [DS 0x639c0 + 0x34]              ; the char-offset array
0x3b7cb  eax = [DS 0x639c0 + 0x38]              ; the payload blob
0x3b7f5  add  eax, [edx + 0x28]                 ; glyph offset = array[10]
0x3b7f8  push eax                               ;   ...stream
0x3b801  push [ [DS 0x639c0 + 8] + 0x28 ]       ; rows  = row-array[10]
0x3b80c  ecx  = [ [DS 0x639c0 + 4] + 0x28 ]     ; col   = col-array[10]   (ECX = arg 2)
0x3b821  ebx = ([ent+0xd] - [ent+5] - DS 0x639e0) - 0xe     ; row  (font B: - 0xd at 0x3b8fa)
0x3b830  edx = ([ent+9] - DS 0x639dc) - 0xf                 ; dest (font B: - 0x18 at 0x3b909)
0x3b836  call 0x3bb42
```

All three data reads are at **`+0x28`** = dword index 10 of a parallel array.  The arrays come from the
resource loader `fn_03d9cf` (port `f.c:7942`): each `.spr` record's 12 header dwords are transposed into 12
arrays of `count` dwords (`fn_03de85` allocates `s[1..13]` at `count*4` bytes each), and `s[13]` is the array
of running payload offsets.  So `+0x28` is record **index 10**, the entry after the ten digits.

In the shipped resources, record 10 is the label, and it is not a digit:

| resource | records | records 0..9 | record 10 | record 11 |
|---|---|---|---|---|
| `system.spr` (font A, kill popup) | 11 | digits, col 4 x rows 7 | **col 13 x rows 6, len 0x5f @0x3e7** | (does not exist) |
| `system2.spr` (font B, per-type numbers) | 13 | digits, col 10 x rows 12 | **col 22 x rows 10, len 0xe4 @0x711** | col 118 x rows 27, len 0xa8e |

Decoding record 10 of `system.spr` with `fn_03bb42`'s row-RLE semantics (run = gap byte, length byte, data;
0xf4 = the transparent index) gives a 13 x 6 three-letter picture:

```
###.#.#.##...
#....#..#.#..
###..#..#.#..
#...#.#.##...
#####.#.#..#.
.............
```

That is the "EXP" text of item 2a: the shipped draws the label at `dest - 15`, `row - 14` and the digits at
`i*5 + dest`, 4 columns apart.

## 2. The port's reads are one index late

`port/src/f/f.c:14560-14565` (the tail call) reads `f[1] + 0x2c`, `f[2] + 0x2c`, `f[13] + 0x2c` -- index
**11**, not 10.  For `system.spr` the arrays are `11 * 4 = 0x2c` bytes long, so index 11 is one dword past
the end of the allocation.  The witness (added under `NIGHT_GLYPHDBG=1`, `valuedisp_tail`, run
`/tmp/rig/gt3`, drive `scripts/rig/drives/ep1_kill3.txt`, unpinned clock so the popup posts, 300 s,
2145 samples at 10 Hz, popup active in 36 samples t = 242837..288793 ms):

```
valuedisp_tail font=A count=11 arraysize=0x2c port[+0x2c]=idx11 col=49 rows=49 glyph=0
             | shipped[+0x28]=idx10 col=13 rows=6 glyph=0x1a7        (x8, identical in every sample)
```

`col = rows = 49 = 0x31` is the adjacent heap dword (0x31 is the constant that sits at +0x3c of the loaded
font block, docs/219 §1), and `glyph = 0` points the stream at the **first byte of the payload blob**, i.e. at
the digit '0' glyph, parsed as if it were 49 rows of a 49-column cell.  `0x1a7 = 423` is the sum of the ten
digits' payload lengths (`0x2e+0x22+0x2c+0x2b+0x2c+0x2a+0x2b+0x25+0x2e+0x2c`, the file parse of §1) -- so
index 10 is exactly record 10's data, confirming which entry the shipped's `+0x28` names.

## 3. The screen-side measurement of that blit

A probe window into `fn_03bb42` (armed only while `NIGHT_GLYPHDBG=1`, `blit_probe_note` at `f.c:3168`, which
records the canvas offset and length of every byte run the blit writes) measures the tail call's real
footprint on the canvas in the same run:

```
valuedisp_tail_blit wrote=147 px bbox x 194..197 y 95..143   (4 x 49)     x1 sample
valuedisp_tail_blit wrote=147 px bbox x 194..197 y 96..144   (4 x 49)     x3
valuedisp_tail_blit wrote=147 px bbox x 194..197 y 97..145   (4 x 49)     x2
```

and the two digit blits of the same record in the same frames:

```
tag=valuedisp dest=209 srow=86..96 col=4 rows=7    (11 successive rows, one per 3 clock ticks)
tag=valuedisp dest=214 srow=86..96 col=4 rows=7    (dest = 209 + 5*1  -> the font-A advance of 5)
```

So the digits are right -- 4x7 at x 209 and x 214, rising one row every three ticks, which is what the
shipped presents at the same coordinates (docs/218 §3) -- and the label is wrong in shape and extent:
**a 4 px wide, 49 px tall column of stream garbage 15 px left of the digits, where the shipped's own
parameters (col 13, rows 6, glyph at blob+0x1a7) can cover at most 13 x 6 = 78 px in a horizontal band.**
49 rows of digit data re-consumed as label rows is a vertical pile of digit fragments -- the shape the
playtest report described for item 2a ("1 / 2 / 3 / ... instead of EXP"), and the same 19-row-tall,
9-px-wide cluster that docs/215 §1 measured in the port's frame diffs at the number's coordinates.

Frame-grid diffs against the shipped dump are still inadmissible for this item (docs/202 basis, docs/205
pacing 5.20 vs 25.08 changes/s), so the admissible pairing here is parameter-vs-write-footprint on the port
against the shipped's parameter set read out of the shipped's own instruction bytes -- not a
port-frame-vs-shipped-frame pixel diff.

## 4. Fix target (not applied; this document is gate evidence)

Three reads at `+0x2c` -> `+0x28` in the tail call of `fn_03b718` (`f.c:14560`, `f.c:14561`, `f.c:14563`).
Before fixing, the same convention must be audited everywhere the port indexes those parallel arrays, because
the comment above `fn_03b718` ("the +0x28 dwords inside the pointed-to objects are the port's +0x2c")
asserts the equivalence generally, and `fn_03de85` sizes the arrays at `count*4` -- for any resource with
`count` records, `+0x2c` is index `count`, i.e. out of bounds whenever `count == 11`.
