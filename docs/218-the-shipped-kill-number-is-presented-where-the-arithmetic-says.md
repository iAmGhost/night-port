# 218 — Two self-corrections and the kill number seen on the shipped side: it is presented, at the coordinates the shipped arithmetic predicts

Everything here is measured on the frozen unfixed build (`build/night` of docs/182) and on the shipped
state dump `/tmp/rig/ents-orig/orig` (drive `ep1_kill3`, 974 states, 4 Hz).

## 1. Correction A: the shipped frame payload is 2560 bytes per row, not 320x4

`state_NNNN.frame.bin` is 512 015 bytes: a 15-byte header `40 01 00 00 | c8 00 00 00 | 00 0a 00 00 | 20 00 00`
= width 320, height 200, and a third dword **0xa00 = 2560**, then 512 000 payload bytes = 200 x 2560.  So a row is
2560 bytes for 320 reported columns: 8 bytes per reported column, i.e. the reply is the 640-wide 32-bit plane
sampled/declared as 320 columns (each reported column covers 2 game columns; `capture_original.py`'s `frame_geom`
already refuses a format claim that contradicts the stride for exactly this reason, and `replay_original.py`
stores the raw reply untouched).

Every shipped-side region measurement made with a `row = y*320*4` index therefore sampled the wrong bytes.  That
voids docs/215 §2 (the "0 changed pixels in the box x 4..30, y 70..120 across states 362..371" table) and the
"only 6 pixels changed at y=118" note: they were computed against a wrong stride.

## 2. Correction B: docs/216 is retracted -- the copy direction was read backwards

cdecl pushes arguments right-to-left, so the first `push` is the *last* argument.  At guest 0x00038154-0x00038168:

```
0x38154  push 0xa0000            ; arg3
0x38159  mov eax, 0x3e80; push   ; arg2
0x3815f  mov eax, [ebp-4]; push  ; arg1  (the composite buffer, DAT_000633b0 -- confirmed by the caller:
                                 ; 0x0003ab27 mov eax,[0x133b0]; 0x0003ab2c call 0x38098)
0x38163  call 0x404c6             ; cpy_dw(arg1=src, arg2=dword count, arg3=dst)
```

and the callee body says which slot is which:

```
0x404cc  mov edi, [ebp+0x10]     ; edi = arg3 = destination
0x404cf  mov esi, [ebp+8]        ; esi = arg1 = source
0x404d2  mov ecx, [ebp+0xc]      ; ecx = arg2 = 0x3e80 dwords = 64 000 B
0x404d5  rep movsd es:[edi], [esi]
```

So the shipped copies **composite buffer -> 0xa0000**: a full 320x200 presentation of the room composite, exactly
what `src/f/f.c:15388` does (`cpy_dw(video_map(base), 0x3e80, g_video_ram)`).  There is no direction
divergence, and docs/216's claim (and its consequences for item 5's black floor and docs/215 §4's presentation
question) is withdrawn.  What survives from docs/216 is the census, which now *agrees* with the shipped: the port
presents the composite into the video plane 5 839 times in a 140 s run, and this site -- not `fn_03ace9` -- is the
room-frame presentation.

## 3. The shipped kill number is in the presented frame, where the shipped arithmetic says it should be

Popup record (DS 0x639cc's block, byte fields, states 363-369): `+0x01 = 10`, `+0x05` age = 3, 8, 15, 21, 28, 34,
40, `+0x09 = 11`, `+0x0d = 113`; `DS 0x639c8` (list count) = 1 for states 363-369 and 0 at 370; camera cells
`DS 0x639dc = 0`, `DS 0x639e0 = 0`; `DS 0x639bc` (kill counter) becomes 1 at state 371.

`fn_03b718`'s own arithmetic (guest 0x0003b795-0x0003b7db, docs/215 §3) predicts, per digit i:
`dest = i*5 + 11 - 0` = 11 and 16, `start_row = 113 - age - 0 - 15` = 98, 93, 83, 77, 70, 64, 58 for the seven
sampled ages, and death when `age > 0x28`.

Diffing consecutive presented frames with the correct 2560 stride, columns 0..39 of the 320-column reply (= game
x 0..79) and rows 45..115, the columns whose ink appears in *every* step of the popup's life and disappear with it
are, in reported columns: 6, 8, 9 at ages 15-40 with the dense 7-row band moving 65-78 -> 60-73; in game
coordinates that is x 12, 16, 18 with the band at rows 66-74 at state 368 (predicted 64-70, i.e. matching within
the sampling of a 4 Hz capture of a moving object).  The ink vanishes at 369->370, the step in which the record
dies at age 40 > 0x28.  So: the number is drawn, it rises, it dies on schedule, and its placement matches
`dest = i*5 + rec[+0x09] - DAT_639dc` -- the same formula the port implements.

## 4. Where item 2a stands, and the oracle that can actually close it

What the rig now has for item 2a: the field (game x 11..20, rows ~58..78 on this route), the draw site
(`fn_03b718` via `fn_03b953`, posted by `fn_015295`), the value (10 both sides), the aging and death behaviour
(both sides), and placement agreement (this document).  What it does **not** have is a *same-basis* picture
comparison: the shipped capture is the emulator's scaled 32-bit reply (each reported column spans two game
columns, and threshold-level ink differs from the game's own 8-bpp bytes), while the port's frames are the port's
own presented buffer.  Comparing glyph *shape* across those two bases is not admissible -- that is the basis
mistake that produced docs/201's "garbled glyph" maps and docs/215 §2's voided table.

The admissible comparison is the glyph raster itself, before any screen: read the shipped's font A control block
(`DAT_000639c0`, 0x3c-byte struct, +0x34 = digit-offset object, +0x38 = RLE base) and its digit row-RLE out of the
CD image, decode it with `fn_03bb42`'s semantics (N rows, then per run: advance, run-length, data), and compare
the rasters against what the port decodes from `spr_block("system.spr")` (`src/f/f.c:2819`, `f.c:14532-14547`)
under `NIGHT_GLYPHDBG=1`.  That comparison is stage-independent and needs no capture basis, and it is the next
measurement for gate3-exp-text; until it is done the item-2a verdict is DETECTED-AND-LOCALIZED for the field and
RIG-GAP for the garble itself.
