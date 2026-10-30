# 193 — `fn_03bb42`: the callee reads argument slots the call sites do not fill

Item 2a's second candidate (docs/192 §2) is now measured on both sides, and it is not clean.

## 1. What the callee reads

`fn_03bb42` (guest `0x3bb42`, body to ~`0x3beba`; the port models it at `f.c:7665`) binds its register arguments as

```
0x3bb57 mov [ebp-0x10], eax      ; base (video base)
0x3bb5a mov [ebp-0x0c], edx      ; x   -- clamped: x + [ebp-4]  vs 0x140
0x3bb5d mov [ebp-0x8],  ebx      ; y   -- clamped: y + [ebp+0x10] vs 0xc8
0x3bb60 mov [ebp-4],    ecx      ; width
```

and then uses, for the RLE stream and the mode, the **fourth and fifth stack slots**:

```
0x3bbbf  mov  edx, [ebp + 0x14]     ; glyph/RLE pointer
0x3bbc7  add  dword [ebp + 0x14], 2 ; advances it per run
0x3bbef  add  dword [ebp + 0x14], eax
0x3bbf7  dec  dword [ebp + 0x14]
0x3bbff  add  dword [ebp + 0x10], eax
0x3bc25  mov  al, byte [ebp + 0x18] ; the mode byte
```

`sub esp, 0x2c` puts the frame's locals at `[ebp-4]..[ebp-0x2c]` (four of them are the saved registers above), so
`+0x10`, `+0x14`, `+0x18` are unambiguously argument slots.

## 2. What the call sites pass

`branch_xrefs.py --target 0x3bb42` lists **32 CALL sites**.  Two of them, read in full:

```
fn_0137c1 @0x1384d / 0x138b3   three stack pushes: 0, glyph, 7      (ECX=5 width, EBX=0x14 y, x = 0x41 + slot*6)
fn_0383a4 @0x38436             two stack pushes:  glyph, 7          (ECX=7 width, EBX=[ebp-4] y,
                                                                   x = [ebp-0x24]*8 + [ebp-8])
```

so the callee's `+0x14`/`+0x18` slots are **beyond what these callers push** (`+0x8`/`+0xc`/`+0x10` are the deepest a
caller fills).  Two shipped number routines therefore hand the glyph stream to the blit in a slot the blit does not
read, and read its mode from memory neither caller wrote.  The two routines also differ from each other in ways the
port reproduces faithfully: glyph index `char_table[d*4 - 0xbc]` in `fn_0137c1` versus `char_table[d*4 - 0xc0]` in
`fn_0383a4` (0x38419), and a per-slot x advance of 6 versus 8 (`imul eax, ..., 6` vs `shl edx, 3`).

The port's binding is `fn_03bb42(base, x, y, col, row_count, rle, mode)`, i.e. it maps the three deepest pushed
words to `row_count=7`, `rle=glyph`, `mode=0`.  Whether that reproduces the shipped behaviour depends on the calling
convention this build actually uses — which the two sides cannot be argued into agreement about from the disassembly
alone, because the shipped call sites do not fill the slots the shipped body reads.

## 3. The discriminator, and why it is the right next step for item 2a

Dump the port's blit arguments and stream head at the EXP-number draws (`GLYPH_TRACE("expnum", ...)` already exists
at `f.c:13986` and `f.c:13994`; the same witness can print `(x, y, w, row_count, first 8 bytes of rle)`), and compare
those RLE bytes against the shipped glyph stream for `0..9` read out of `exp.spr` through `char_table[1]` and
`char_table[d-0x2f]`:

* if the bytes match, the port's slot binding reproduces shipped and item 2a's garble must be in the **font data**
  (`exp.spr` parse: `char_table = [g_dat_63e04+0x34]`, `rle_base = [g_dat_63e04+0x38]`);
* if they do not match, the binding is the defect, and the vertical-stacking symptom reported for the port is its
  natural shape (a wrong stream head or a wrong row advance reads the same bytes on successive rows).

Either outcome is a localization with a named byte or offset, which is what `gate3-exp-text` requires; today the
measurement is not yet taken, so item 2a is still open.
