# 210 — Item 2a's text candidate, named from shipped bytes: `fn_03b718`, the camera-relative floating number

`gate3-exp-text` has been hunting a "panel" for the kill-time EXP text and found none (docs/204: the `x=0x68,0xac`
and `x=0x118,0xae` panels are the CONTINUE countdown's game-over screen).  The shipped binary has a third number
renderer, and it is the one positioned **relative to an entity**, which is what "text appears when an enemy dies"
describes.

## 1. The routine and its geometry (`fn_03b718`, 0x0003b718..0x0003b9b6)

```
itoa32s(entity[+0x01], buf, 10); len = str_len(buf)                  0x3b735-0x3b750
font A when entity[0] == 0 : advance 5,  rows 7,  col 4              0x3b7aa / 0x3b7ac / 0x3b7cc  (DAT_000639c0)
font B otherwise          : advance 0xb, rows 0xc, col 10            0x3b885 / 0x3b899 / 0x3b8a2  (DAT_000639c4)
glyph = *(char_table - 0xc0 + digit*4)                               0x3b7a3
per digit: fn_03bb42(base, i*adv + entity[+0x09] - DAT_000639dc,     0x3b7db
                       entity[+0x0d] - entity[+0x05] - DAT_000639e0 - 0xf, col, rows, rle_base+glyph, 0)
terminator: dest x = entity[+0x09] - DAT_000639dc - (0xf | 0x18),    start_row - (0xe | 0xd)
aging:  if (DAT_00063360 % 3 == 0) entity[+0x05]++;                  0x3b91d-0x3b927
death:  if (entity[+0x05] > 0x28) { fn_03b67e(entity); heap_free }    0x3b936-0x3b946
```

So the number is drawn at the **entity's own coordinates minus the camera** (`DAT_000639dc` horizontal,
`DAT_000639e0` vertical), with `entity[+0x01]` as the value, `entity[+0x09]/+0x0d` as the anchor and
`entity[+0x05]` as its age.  The same `entity[+0x09] - DAT_639dc` arithmetic is the camera-relative basis
`gate3-side-glitch` (item 6) needs for sprite x, so items 2a and 6 share one offset family.

## 2. Its invocation chain is exactly one, and it is a list walk

`call_sites.py --target 0x3b718` -> **one caller: `fn_03b953` at 0x0003b99a**, whose body is
`for (i = 0; i < DAT_000639c8; i++) fn_03b718(DAT_000639d4, base);` -- a count cell (`DS 0x639c8`) and a record
list (`DS 0x639d4`).  The port models the whole chain (`port/src/f/f.c:14516` the renderer, `f.c:14587` the loop).
Note `DS 0x63360`, the frame counter used by the aging test, is the same cell docs/205's screen attribution showed
changing at every shipped mode switch (`0x63360 0000069f -> 000006b1` etc.).

## 3. What the rig measures about it

The port has a `GLYPH_TRACE("valuedisp", ...)` in this renderer (`f.c:14541`), so the census is direct.  On the
canonical route with `NIGHT_GLYPHDBG=1`:

```
tag=stagespr 80   tag=expnum 80   tag=numfield5 0   tag=valuedisp 0
```

The port never renders a floating number on any dumped route -- the same null result as the EXP award
(docs/199: no award made, `adds=0`) and the kill counter (DS 0x639bc stays 0 on both sides, docs/197).  The gate
therefore cannot be satisfied by rendering this field on a route where nothing dies.

## 4. The concrete next question for items 2a and 5

docs/196 asked "who posts a record at kill time?" and found nothing in `g_dat_62a90 / 0x5d0b4`.  This routine names
a *different* queue: the floating-number list at **`DS 0x639c8` (count) and `DS 0x639d4` (record list)**.  The next
measurement is a static census of writers to those two cells across the 523 modelled bodies (the `field_writers.py`
machinery, absolute-DS form), followed by the dynamic check of whether the port ever runs the create path -- if the
create path is modelled and never reached, item 2a's missing text follows the same unfinished kill chain as item 2b
rather than being a glyph/geometry defect.
