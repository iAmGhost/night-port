# 214 — Two measurements: the shipped EXP number is clock-driven (no key needed), and the port's sprite geometry reproduces the shipped formula in 11 976 of 11 976 cases

## 1. Shipped: floating-number records appear and expire with zero key events in the drive

`/tmp/rig/ents-orig/orig` (drive `ep1_kill3`; its last key event is at nominal 300 820 ms, so the samples below,
nominal ≈ 268 s to 288 s, are inside a completely key-free stretch of the drive):

```
state  t_rel   DS 0x639c8 (popup list count)   DS 0x63ed0   DS 0x63370   DS 0x63360 (timer)
   365   268329        1            1              1          1            7058
   369   269551        1            0              1          1            7133
   371   270149        0            0              1          1            7169     <- count drops, no key
   387   275238        0            0              1          1            7459
   389   275846        1            0              1          1            7496     <- a NEW record, no key
   395   277671        1            0              1          1            7607
   397   278278        0            0              1          1            7644
   431   288542        0            0              1          1            8273
```

`DS 0x639c8` goes 1 -> 0 -> 1 -> 0 across key-free samples while the frame counter advances, which is exactly
`fn_03b718`'s clock-driven aging (`if (DAT_00063360 % 3 == 0) node[+0x05]++;` at 0x0003b91d-27, removal at
`+0x05 > 0x28` via `fn_03b67e` at 0x0003b936-46).  So on the shipped side the kill number is a *timed* display; a key
is not part of its lifecycle.  That separates item 2a (the number) from item 4 (the card that needs a key), and it
means any "the EXP text advanced without a keypress" reading of item 4 cannot be about the floating number.

## 2. Port: the sprite geometry the mirror arms consume, witnessed 11 976 times

New witness `NIGHT_ENTGEOM=1` (+ `NIGHT_ENTGEOMBUDGET`, `port/src/f/f.c` inside `esprr_dump`) prints the four sprite
arrays and the arm actually taken:

```
entsprgeom spr=7 W=27 H=79 X=10 Y=76 worldx=15 camx=0 face=0 dest=5 arm=forward tick=... player=1
```

Run `/tmp/rig/le6` (drive `ep1_leftedge`, 11 976 witnessed composites, 3 100 of them the player record):

```
dest != shipped formula (face 0: worldx-X-camX ; face 1: worldx-(W-X)-camX):  0 of 11 976
player face distribution:            face 0 x2980   face 1 x120
player composites with worldx<=20:   10   (face 0 x6, face 1 x4)   min worldx 15
player composites with worldx<=40:   2134 (face 0 x2110, face 1 x24)
player (spr,W,X) families:           (0..5,53,24) (6,42,17) (7,27,10) (8,28,10) (9,30,14)
cases with X > W:                    0 of 11 976
```

The same cell read on the shipped side (the 352-byte player block behind `DS 0x53c7c`, 974 states):

```
shipped player face: 1 x648, 0 x326 overall
shipped player with worldx <= 20: 94 samples -> face 0 x68, face 1 x26
shipped player with worldx <= 40: 107 samples -> face 0 x75, face 1 x32
shipped player worldx range: -30 .. 153
```

So at the left edge the shipped game is also *predominantly* face 0 (forward arm), and no witnessed composite has
`X > W` (the shape that would put a sprite on the opposite side when the mirror arm is taken).  Item 6 is therefore
not explained by the facing bit, the arm choice, the x formulas, the sprite arrays as far as they were sampled, or
the blitter's clip (docs/212) -- and the rig has no witnessed wrong-side blit yet.

## 3. The open basis question that blocks both screen halves

The shipped frame halves at the popup states do not show ink where the record says the number is: the record in
state 0363 places the digits at x = 11 and 16, y ≈ 95..102, and the region x 6..32 / y 92..112 is pixel-identical in
states 361, 363, 366, 369 (docs/211 §4) even though whole-frame hashes *do* change state-to-state in that range
(half0 hashes 616d0dbc, 5a8fd791, f21b4bce, de03a04d for 363/364/366/369; half1 differs too).  A difference-set test
(pixels that change only in the popup window 362-369 and never in the control window 352-359) gives 1 404 pixels
spread over bbox x 0..92, y 40..199 -- diffuse, not a glyph cluster.  Half 1 of those frames is uniformly lit in the
same window.  Combined with docs/61:56 ("the matching frame from the original is still blocked on the DOSBox capture
path"), the honest statement is: **which shipped buffer is the presented 320x200 canvas is not established**, and
until it is, region-level screen claims (items 2a and 6, and the black-floor claim in item 5) are inadmissible.  The
rig's screen gates therefore currently rest on the port's own back-buffer witnesses (`tag=entspr`, `tag=valuedisp`,
`GLYPH_TRACE` row-RLE), which are stage-independent and do measure the draw sites.
