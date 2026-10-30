# 211 — Item 2a: the shipped kill popup is the `+0x32` value posted by the kill branch, and the port posts 10 at the same place

## 1. The shipped post site (named, with bytes)

`fn_015295`'s settle branch posts the floating-number node at 0x000153fe-0x00015432:

```
0x153fe  mov eax, 0x19 ; call 0x41bdc          ; heap_malloc(0x19)
0x15417  xor eax, eax  ; push eax              ; flag = 0  (the other branch pushes 1)
0x1541d  mov ecx, [eax + 0x32]                 ; THE VALUE  -> node+0x01
0x15423  mov ebx, [eax + 9] ; sub ebx, 0x1e    ; y - 30     -> node+0x0d
0x1542c  mov edx, [eax + 1]                    ; x          -> node+0x09
0x15432  call 0x3b5c0                           ; list insert, DS 0x639c8 count++, head 0x639cc
```

so the number the shipped game prints when an enemy dies **is the same `+0x32` field** that item 2b's award
writes (docs/199: shipped row 31 of `DS 0x53e9c` = `(35, 2, 0, 10)` -> `+0x32 = 10`).

## 2. The shipped record, read out of a shipped heap block

`/tmp/rig/ents-orig/orig` (drive `ep1_kill3`, 974 states, 63 ptr-cell blocks of 352 B each).  `DS 0x639c8` (the
floating-number list count) is 1 in **14 of 974** states (samples 363-369 and 389-395); the kill counter
`DS 0x639bc` first becomes 1 at sample 371 and reaches 2.  The node at `DS 0x639cc` (guest address 0x383250) in
state 0363 decodes exactly as the port's `fn_0003b5c0` layout predicts:

```
byte 0 (flag/font) = 0      +0x01 (value) = 10      +0x05 (age) = 3
+0x09 (x) = 11              +0x0d (y) = 113         (+0x11 / +0x19 links)
DS 0x639c0 object +0x00 = 11 (font A control block), DS 0x639c4 object +0x00 = 13 (font B control block)
```

With `flag = 0` the renderer picks font A (`DAT_000639c0`: advance 5, rows 7, col 4), and `fn_03b718` places the
two digits `1`, `0` of `itoa32s(10)` at `x = 11` and `x = 16`, `y = 113 - 3 - DAT_639e0 - 0xf`.

## 3. The port posts the same value at the same place (new witness `NIGHT_POPUPDBG=1`, f.c:8196)

Unpinned run of the same drive (`NIGHT_SEED=13579`, no `NIGHT_RIG_STEP_CALLS`), `/tmp/rig/v3`:

```
popup post: a0+0=1 a0+0x17=21 a0+0x29=0 +0x32=10 ent+1=318 ent+9=141 latched=0
```

and the same configuration with `NIGHT_GLYPHDBG=1` (`/tmp/rig/v2`, 12 000 frames, kill counter max 2, `DS 0x639c8`
nonzero in 36 samples) renders it:

```
menucell tag=valuedisp k=0 dest=11 srow=97 col=4 rows=7 ...
menucell tag=valuedisp k=0 dest=16 srow=97 col=4 rows=7 ...
```

Two glyphs, 5 px apart, col 4 / rows 7 = font A, starting at x = 11 -- identical geometry to the shipped record.
So items 2a's "garbled text" is **not** a wrong value, a wrong font choice, or a wrong anchor on this route.
Corrections to record: (a) docs/210's `fn_03b953` walk cursor is `DS 0x639d4 = DS 0x639cc` (the head), which is why
`DS 0x639d4` reads 0 in the shipped samples while the list is non-empty; (b) the pinned clock
(`NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`) produces **zero** posts and zero popups on this drive
(`/tmp/rig/v1`: `g_dat_639bc` max 0, `g_dat_639c8` max 0, 0 valuedisp lines), whereas the unpinned loop produces
posts -- so any popup/award census must state which clock it used (docs/190 §1, docs/191 §1 family).

## 4. What the shipped frame dump does NOT show (basis limit, measured)

The number region of the shipped frames is **pixel-identical across states 361, 363, 366 and 369** -- including
states before the popup exists (361) -- e.g. x 6..32, y 92..111 in `state_0361/0363/0366/0369.frame.bin` half 0:
a static band `##################` at y=96-97 and 106-111 with scenery marks at y=102-105 (`####` at x=20-23,
`##` at x=28-29), no ink appearing or disappearing at x 11..21 / y 95..102 where the popup should be, and half 1 is
uniformly lit in the same window.  So these particular shipped `frame.bin` captures are not a valid screen basis
for the popup: either the states were captured while the game presented a static card, or the presented frame is
not the buffer the glyph compositor wrote.  The screen half of `gate3-exp-text` therefore needs a shipped capture
whose frames change with the gameplay clock (`fn_03f15f`'s arena, docs/205) before any region-level claim about the
popup's appearance can be quoted; the honest status of item 2a today is: **draw site, post site, value, font and
anchor all localized and matching; the garble itself is not yet observed on either side.**
