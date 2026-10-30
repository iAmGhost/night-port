# 192 — Item 2a: the digit index arithmetic in `fn_0137c1` is faithful, so the garble is in the font data or the blit

The shipped display loop was read end to end (`0x13807-0x138b3`, 54 instructions).  Both loops call the same blit
with the same geometry, and the glyph selection is:

```
0x13827 mov eax, [DS 0x63e04 + 0x34]        ; char_table (heap offset)
0x13830 mov edx, [DS 0x63e04 + 0x38]        ; rle_base
0x13833 add edx, dword [eax + 4]            ; blank glyph = char_table[1]
0x13843 imul eax, [ebp-0x20], 6             ; slot * 6
0x13847 lea edx, [eax + 0x41]               ; x = 0x41 + slot*6
0x1383e mov ebx, 0x14                       ; y = 20
0x13839 mov ecx, 5   / push 7               ; w = 5, h = 7

0x13871 mov  al, byte [i + ebp - 0x14]      ; buf[i]  (the itoa of entity[+0x2b])
0x1387a shl  eax, 2                         ; buf[i] * 4
0x13887 add  edx, eax                       ; char_table + buf[i]*4
0x13891 add  eax, dword [edx - 0xbc]        ; rle_base + char_table[buf[i] - 0x2f]
0x138a7 add  eax, [ebp-0x1c]                ; slot += pad
0x138aa imul eax, eax, 6
```

The port's model (`f.c:13977-14000`) computes `char_table[d - 0x2f]` for digits and `char_table[1]` for the pad, at
`x = 0x41 + slot*6`, `y = 0x14`, `5 x 7` — which is the same arithmetic (`-0xbc / 4 = -0x2f`).  So the "the port picks
the wrong glyph for a digit" hypothesis is closed, including the documented aliasing quirk (digit `0` and the blank pad
read the same entry, guest `0x00013833` vs `char_table[1]`).

That leaves exactly two places for the garble the playtest reports, and they are checkable without play:

1. **The font data itself.**  `char_table`/`rle_base` come from `exp.spr` (`f.c:2824` loads it into `g_dat_63e04`;
   the offsets `+0x34`/`+0x38` are heap offsets left by `fn_03d9cf`).  If the port's `exp.spr` parse yields a different
   `char_table` array or a different `rle_base`, every glyph is wrong while the arithmetic stays right.  `NIGHT.EXE`
   contains no `EXP` bytes anywhere (docs/191 §2), so the label glyphs can only come from this sprite.
2. **The blit `fn_03bb42`.**  Its prologue binds `EAX=base, EDX=x, EBX=y, ECX=w`, clamps `x + [ebp-4]` against
   `0x140` and `y + [ebp+0x10]` against `0xc8`, then reads the glyph pointer from `[ebp+0x14]` (`0x3bbf` block).  The
   port calls it as `fn_03bb42(base, x, 0x14, 5, 7, glyph, 0)`.  The shipped function's *stack* argument slots are
   worth one careful pass: the caller pushes `0`, glyph, `7` (three stack words), yet the body reads `[ebp+0x10]` as a
   height and `[ebp+0x14]` as the glyph pointer — four words' worth.  If the port maps glyph/height to different slots
   than shipped does, the RLE stream is read from the wrong address, which is precisely a garble that keeps the
   arithmetic intact.  This is the next thing to verify, from the disassembly plus the existing `NIGHT_GLYPHDBG`
   witness (`f.c:13986`, `f.c:13994`), against a known-good glyph from the shipped `exp.spr`.

The vertical-stack symptom reported for the port ("`1 2 3 … 0` on separate lines instead of `EXP 30`") is the shape of
(2): a fixed `x` advance with the wrong row stride reads the same RLE stream at successive rows.  It is recorded here
as a *report to be confirmed by the rig*, not as evidence: the crop in docs/191 §2 shows the field's own five glyphs
rendering side by side (`P. 00000`), so the two symptoms are different draws and item 2a needs the event-anchored
screen diff (docs/182 row 2a) before either is accepted.
