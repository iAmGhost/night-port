# 227 — Item 2a fixed: the EXP label blit read dword index 11 of the font arrays where the shipped reads index 10

Fix task: `fix-exp-text`.  Red evidence was committed before the change (docs/221 `77034ad`, gate report
docs/226 `4ea273d`).

## 1. Root cause, from the shipped bytes

`fn_03b718` (shipped 0x0003b718, port `f.c:14516`) draws a floating number and then, in its tail, the "EXP"
label.  The tail call's three parameters come from the font control block's transposed parallel arrays, and the
shipped reads the **last** entry of each -- dword index 10, displacement **+0x28**:

```
font A branch (0x3b7e2-0x3b836)                     font B branch (0x3b8bb-0x3b90f)
0x3b7e4  push 0                    ; mode 0         0x3b8bb  push 0
0x3b7ea  mov  edx, [0x639c0+0x34]  ; char-table obj 0x3b8c3  mov  edx, [0x639c4+0x34]
0x3b7f2  mov  eax, [0x639c0+0x38]  ; RLE blob base   0x3b8cb  mov  eax, [0x639c4+0x38]
0x3b7f5  add  eax, [edx + 0x28]    ; glyph = idx 10  0x3b8ce  add  eax, [edx + 0x28]
0x3b7f8  push eax                                    0x3b8d1  push eax
0x3b7fe  mov  eax, [0x639c0+8]     ; array #2 (rows) 0x3b8d7  mov  eax, [0x639c4+8]
0x3b801  push [eax + 0x28]         ; rows = idx 10   0x3b8da  push [eax + 0x28]
0x3b809  mov  eax, [0x639c0+4]     ; array #1 (cols) 0x3b8e2  mov  eax, [0x639c4+4]
0x3b80c  mov  ecx, [eax + 0x28]    ; col  = idx 10   0x3b8e5  mov  ecx, [eax + 0x28]
0x3b833  mov  eax, [ebp-4] ; base                    0x3b90c  mov  eax, [ebp-4]
0x3b836  call 0x3bb42                                0x3b90f  call 0x3bb42
```

`fn_03de85` sizes every parallel array at `count*4`.  `system.spr` (font A, `DS 0x639c0`) has **11** records, so
its arrays are `11*4 = 0x2c` bytes: index 11 (+0x2c) is **one dword past the allocation**.  The port read +0x2c
(`f.c:14596-14598` before the fix), so it picked up the adjacent heap dword -- `0x31` = 49, the constant that
sits at +0x3c of the loaded font block (docs/219 §1) -- as both `col` and `rows`, and read `glyph = 0`, pointing
the label's RLE stream at the first byte of the payload blob (the digit '0') and parsing it as 49 rows of a
49-column cell.

Record 10 is the label: `col = 13`, `rows = 6`, glyph offset `0x1a7` = 423 = the sum of the ten digits' payload
lengths (`0x2e+0x22+0x2c+0x2b+0x2c+0x2a+0x2b+0x25+0x2e+0x2c`, docs/221 §1) ✓ so index 10 is exactly the record
the shipped's `+0x28` names.  The sibling routine `fn_03ace9` correctly uses `+0x2c`/`+0x30` for
`system2.spr`'s 13-entry arrays (docs/221 §5): the defect is local to `fn_03b718`.

## 2. The change

`src/f/f.c` tail call, three displacements `+0x2c` -> `+0x28`, with the shipped sites cited:

```c
(int32_t)((struct ua32 *)((hb + f[1] + 0x28)))->v,   /* 0x3b801 (A) / 0x3b8da (B): col  */
((struct ua32 *)((hb + f[2] + 0x28)))->v,            /* 0x3b801 / 0x3b8da:              rows  */
rle_base + ((struct ua32 *)((hb + f[13] + 0x28)))->v /* 0x3b7f5 / 0x3b8ce:              glyph */
```

## 3. The test that had been asserting the defect

`test/test_f1.c` had a `fn_03b718` test whose fixture parked the label values at `+0x2c` while its own
comment said "(orig +0x28)", and it reserved `0x34` bytes for arrays the shipped sizes at `0x2c`:

```c
uint32_t o4 = heap_malloc(0x34), o8 = heap_malloc(0x34);
*(uint32_t *)(hbx + o4 + 0x2c) = 3;   /* terminator col  (orig +0x28) */
```

So the suite was green *because* the test agreed with the port's off-by-one.  The fixture now reserves `0x2c`
(the shipped's `count*4`) and stores at `+0x28`, so a regression to index 11 reads past the allocation and trips
ASan.  The two checks renamed to `... (rows/col from +0x28 = record 10, docs/227)`.

## 4. Red before / green after, same drive, same rig

Drive `scripts/rig/drives/ep1_kill3.txt`, `NIGHT_GLYPHDBG=1`, `NIGHT_SEED=13579`, unpinned clock (the popup only
posts unpinned: docs/221), `--rig-hz 10 --frames 12000`.  The oracle is the blit footprint probe (`f.c:3168`
`blit_probe_note`, armed only under `NIGHT_GLYPHDBG`), which records the canvas offsets and length of every byte
run the tail blit actually writes -- a screen-level measurement that does not depend on comparing two dumps'
presentation bases (docs/202, docs/205).

| run | binary | tail parameters | footprint written into the canvas |
|---|---|---|---|
| `/tmp/rig/e2a_asan` | `night-asan` built from the **pre-fix** source (`/tmp/prefix` worktree at `4ea273d`) | `port[+0x2c]=idx11 col=49 rows=49 glyph=0` | **147 px in a 4x49 box** -- x 268..271, y 95..143 / 96..144 / 97..145 (6 samples) |
| `/tmp/rig/e2a_before` | `night` built from the same pre-fix source | same `col=49 rows=49 glyph=0` | 0 px (the mis-parsed stream bailed: the label was absent), 6 samples |
| `/tmp/rig/e2a_after` | `night` with the fix | `used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7` | **63 px in a 13x6 box** -- x 254..266, y 95..102 / 96..101 / 97..102 (6 samples) |
| `/tmp/rig/e2a_asan_after` | `night-asan` with the fix | `used[+0x28]=idx10 col=13 rows=6 glyph=0x1a7` | **42 px in a 9x6 band** -- x 0..8, y 99..104 / 100..105 (6 samples; the popup sat near the left edge in that run, the extent is what discriminates) |

The pile's geometry matches the report: 4 px wide, 49 px tall, i.e. 49 rows of digit data re-consumed as label
rows -- "1 / 2 / 3 / ... instead of EXP".  The fixed footprint is a horizontal band at most 13x6 = 78 px, of
which 63 px are ink ✓ the label's own extent.

**The number blits are unchanged**: `tag=valuedisp` digit blits count 60 in both runs, `col=4 rows=7` at the
digit advance of 5 (font A), i.e. no collateral change to the digits, which docs/215/docs/218 already showed to
match the shipped's coordinates.

## 5. A pixel-region census cannot show this (recorded so it is not mistaken for evidence)

Counting non-modal pixels in the label box (x 254..266, y 95..102) over the saved witness frames:

```
before: 1551 frames, 1378 with >= 20 px, max 62 px
after:  1674 frames, 1603 with >= 20 px, max 63 px
```

The room's own art fills that box on this route, so a region census cannot separate the builds.  The footprint
probe (§4) is the admissible basis, and this is the same rule docs/202/docs/205 established: region pixel
comparisons need a same-state basis, and the two dumps do not share one.

## 6. Sanitizer and suite state

* `make test`: **== ALL PASS ==**, 3 067 checks, rc=0 (the two previously-failing checks appeared only
  after the source fix and were the tests quoted in §3).
* Fixed `night-asan` on the same drive (`/tmp/rig/e2a_asan_after`, 12 000 frames, 10 Hz): footprint 42 px in a
  9x6 band, 6 samples; digit blits 60 (`col=4 rows=7`); **zero** `ERROR: AddressSanitizer`; the single sanitizer
  output is the standing UBSan misaligned store at `f.c:7360` (`toks[cnt1-1] = NULL` in `fn_04589e`), which
  appears on every route including ones that never reach a popup (docs/224 §6).
* The fix also removes an out-of-bounds read: index 11 of a `count*4 = 0x2c` allocation.  With the fixture now
  sized `0x2c`, reverting the displacement trips ASan instead of silently reading a neighbour dword.

## 7. docs/65 rows added

| claim | verdict | evidence |
|---|---|---|
| The EXP label's col/rows/glyph come from array index 11 (`+0x2c`) | **no: index 10 (`+0x28`), docs/227 §1** | 0x3b7f5 / 0x3b801 / 0x3b80c (font A), 0x3b8ce / 0x3b8da / 0x3b8e5 (font B); `fn_03de85` sizes the arrays at 11*4 = 0x2c |
| `fn_03ace9` proves the port's `+0x2c` reads are right | **no: it addresses a 13-entry font block, docs/221 §5** | `system2.spr` count 13 -> index 11/12 are in range there |
| The `fn_03b718` suite test proves the label is faithful | **no: the fixture parked its values at `+0x2c` and reserved 0x34, docs/227 §3** | `test_f1.c` `heap_malloc(0x34)` + `[o4+0x2c] = 3  /* (orig +0x28) */` |
| A pixel-region census can demonstrate the label fix | **no on this route, docs/227 §5** | box census 1378/1551 vs 1603/1674 frames, max 62 vs 63 px -- the room art dominates |
