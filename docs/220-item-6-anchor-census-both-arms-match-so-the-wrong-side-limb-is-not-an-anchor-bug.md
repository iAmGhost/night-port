# 220 — Item 6 anchor census: the port's x anchor matches the shipped on both mirror arms in every sampled composite, so the wrong-side limb is not an anchor bug

## 1. The shipped's two arms, byte for byte

Forward arm (`fn_0133xx`, guest 0x000133a4-0x000133c8):

```
0x133a4  eax = [ebp-0x10]           ; i (the sprite-part index)
0x133a7  shl eax, 2                 ; i*4
0x133ad  edx = [ent + 0x137]        ; the X array base
0x133b3  add  eax, edx              ; &X[i]
0x133b8  eax = [ent + 1]            ; entity world x
0x133bb  sub  eax, [edx]            ; -= X[i]
0x133bd  sub  eax, [0x139dc]        ; -= DAT_639dc  (camera x)
0x133c3  edx = eax
0x133c8  call 0x3bebb               ; the blit helper (NOT 0x3bb42 -- both arms use 0x3bebb)
```

Mirror arm (guest 0x0001344b-0x00013482):

```
0x1344e  eax = i*4 ; 0x13454 edx = [ent + 0x12b] (the W array base) ; 0x1345a add eax, edx   ; &W[i]
0x1345f  edx = i*4 ; 0x13465 esi = [ent + 0x137] (the X array base) ; 0x1346b add edx, esi   ; &X[i]
0x1346d  eax = W[i]
0x1346f  sub  eax, [edx]            ; eax = W[i] - X[i]
0x13474  edx = [ent + 1]            ; entity world x
0x13477  sub  edx, eax              ; -= (W[i] - X[i])
0x13479  sub  edx, [0x139dc]        ; -= camera x
0x13482  call 0x3bebb
```

So the shipped's anchor is `worldx - X[i] - camX` forward and `worldx - (W[i] - X[i]) - camX` mirrored: the part
anchor is flipped about the sprite's width, and *the same helper* `fn_03bebb` is called either way (so any column
reversal lives inside `fn_03bebb`, not in the caller).

## 2. The census against the port's own composites

`/tmp/rig/le6.log` (`NIGHT_ENTGEOM=1`, drive `ep1_leftedge`, docs/212 §1: ep1_kill3 truncated to 120 s plus eight
left holds and fourteen left-tap/attack pairs), 11 976 composite draws, each carrying `spr, W, H, X, Y, worldx,
camx, face, dest, arm`:

```
dest == worldx - camx - X        : all  3721/11976   forward 3712/3712   mirror    9/8264
dest == worldx - camx - (W - X)  : all  8273/11976   forward    9/3712   mirror 8264/8264
dest == worldx - camx + X        :    0              forward    0         mirror    0
dest == worldx - camx            :    0              forward    0         mirror    0
face/arm census: mirror 8264, forward 3712
min worldx 15, min dest 0, max dest 278 ; dest < 0 in 0 composites ; dest > 0x13f in 0 composites
left-edge composites (camx == 0 and worldx <= 24): 232, of which dest < 0 in 0
```

(Each formula matches in the 9 stray cases simply because `W - 2X == 0` there, so the two formulas coincide.)

The port therefore reproduces the shipped's anchor exactly on both arms in every sampled composite: 3712/3712
forward against 0x000133a4-0x000133c8, and 8264/8264 mirrored against 0x0001344b-0x00013482.  The `W - 2X` term that
the shipped's mirror arm introduces relative to the forward arm is present in the port.  Item 6 is **not** an anchor
or clamp bug on this drive, and the drive never produces a composite whose destination is off-canvas (min dest 0,
max dest 278 in a 320-wide canvas), so the clip divergence recorded in docs/212 §4 (the shipped's mutually
exclusive clip arms at 0x0003bc43/0x0003bc4e and its bail at `dest_off > 0x140` against the port's `> 0x13f`) is
real but never exercised by any route built so far.

## 3. What item 6 still needs, narrowed to two candidates

(a) **The column order inside the blit helper.**  Both shipped arms call `fn_03bebb` (0x0003bebb), not the plain
RLE blitter `fn_03bb42`.  Whether `fn_03bebb` walks the row runs backwards (a true column mirror) or forward is the
open question; the port models the mirror by flipping the anchor and (per docs/212 §3's composite census:
`gun.spr` 1401 mode=0 / 99 mode=1, `ena-1.spr` 1086 mode=1 / 414 mode=0) by a per-composite mode flag.  Reading
0x0003bebb's loop (its run cursor and its column accumulator) decides whether "draw the limb on the wrong side"
means the port mirrors the anchor but not the pixels, or the shipped mirrors neither.

(b) **The limb's own record.**  The attack part is the entity that resolves to `type_act = -1 / type_spr = -1`
(struct 00224b5c, docs/213 §2) -- the only composite in the left-edge run with no `.act`/`.spr` chain.  Its `W`, `X`
arrays are what the mirror arm consumes, so if the port leaves them at the un-mirrored layout for a spawned part
while the shipped writes them flipped at spawn time, the limb lands on the wrong side even with a faithful anchor.
That is measurable: dump `+0x12b/+0x12f/+0x133/+0x137` (W/H/R/X arrays) per chain target for that struct on both
sides at the moment of the attack, on a drive that puts the player at `camx == 0` with `worldx` at its minimum --
which `ep1_leftedge` already provides (min worldx 15, camx 0, 232 left-edge composites).

## 4. What `fn_03bebb` actually is (its head, guest 0x0003bedc-0x0003bf5e)

It is the *clipping* blitter, and its bail tests are the pair the port has no equivalent of:

```
0x3bedc  [ebp-0x14] = 0
0x3bee3  if (dest_x > 0x140) goto end                     ; 320 columns
0x3beec  if (dest_x + col_arg < 0) goto end               ; the right-hand edge test
0x3befb  if (dest_y > 0xc8) goto end                      ; 200 rows
0x3bf04  if (dest_y + row_arg < 0) goto end
0x3bf13  if (dest_y >= 0) -> the drawing path
         else: consume the row's runs without drawing   ; 0x3bf26-0x3bf5e: -dest_y iterations,
                                                       ; stream cursor += 2 per run, data skipped
```

So a part placed above the canvas top is *consumed* (its RLE is walked and discarded) rather than clamped into
view, and a part whose anchor plus column/row extent falls off the left or top is dropped outright.  `fn_03bb42`
(docs/212 §4's mutually exclusive clip arms) is a different helper with different bounds; item 6's case goes
through *this* one.  Whether its column walk reverses the runs is the remaining half of candidate (a) and needs
its drawing path (0x0003bf84 onward), which is the next read.
