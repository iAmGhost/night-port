# 0x038b30 (L11, 242 B) — 5-glyph right-justified number field (font-table + RLE blit)

Same family as 0x038c23 (ported). Clean: callees itoa32s (0x041f42), str_len (0x041f5d),
fn_03bb42 (0x03bb42), canary (0x0418ee) — ALL ported.

## ABI (raw disasm; blit convention EAX=base/EDX=dest_off/EBX=start_row/ECX=col, + 3 stack args)
This fn's own regs: EAX=base, EDX=font, EBX=col0, ECX=start_row, [stack]=number (RET 0x4).
`fn_038b30(uint32_t base, uint8_t *font, int32_t col0, int32_t start_row, int32_t number)`

## Logic (ground truth from raw disasm 0x038b30..0x038c20)
```
itoa32s(number, buf, 10);
len = str_len(buf);
pad = 5 - len;                       // left-pad count (blank glyphs)
char_table = *(font+0x34);           // 8B host ptr (port model; orig 4B dword)
rle_base   = *(font+0x3c);           // 8B host ptr (orig 0x38, widened to 0x3c)
// LOOP 1 -- pad: draw (5-len) BLANK glyphs (glyph for char at char_table[0])
for (i = 0; i < pad; i++)
    fn_03bb42(base, i*7 + col0, start_row, 6, 7, rle_base + *(uint32_t*)char_table, 0);
// LOOP 2 -- digits: right-justified, start at column (5-len)
for (i = 0; i < len; i++) {
    d = (uint8_t)buf[i];
    fn_03bb42(base, (i + pad)*7 + col0, start_row, 6, 7,
              rle_base + *(uint32_t*)(char_table - 0x40 + d*4), 0);
}
```
Total field = 5 glyphs * 7 cols = 35 cols, starting at col0, on start_row (7 rows each).

### fn_03bb42 arg mapping (both loops, identical shape)
EAX=base, EDX=dest_off, EBX=start_row, ECX=col(=6), stack: row_count(7), rle, mode(0).
- Loop1 dest_off = i*7 + col0 (i=0..pad-1); rle = rle_base + char_table[0].
- Loop2 dest_off = (i+pad)*7 + col0 (i=0..len-1); rle = rle_base + *(char_table-0x40+d*4).

## Port-modeling notes (mirror 0x038c23 exactly)
- font[0x34] and font[0x3c] (orig 0x38) are 8-byte host pointers (pointer-width adaptation;
  orig had adjacent 4-byte dwords). Do NOT overlap them (the 0x038c23 overlap bug).
- glyph_off for digit d = *(uint32_t*)(char_table - 0x40 + d*4); pad glyph = *(uint32_t*)char_table.

## Test plan (mirror test_fn_038c23)
- `static uint32_t char_map[256]`; char_map[16]=pad(glyph) offset; char_map['0'..'9']=d offsets.
  `static uint8_t rle_buf[10*28]`: glyph d = 7 rows of [1,0,1,(0xa0+d)]; pad glyph (say idx 16->
  use a distinct byte 0xb0) = 7 rows of [1,0,1,0xb0]. `static uint8_t font[0x44]`.
  `*(u64*)(font+0x34) = char_map + 16`  (16 elem = 0x40 BYTES so (char_table-0x40)=char_map base);
  `*(u64*)(font+0x3c) = rle_buf`.
- base=0xa0000, col0=100, row=5.
- T1 number=42 -> len=2, pad=3. Blank glyphs at cols 100,107,114 (X=0xb0); '4'@121, '2'@128.
    (cols = col0 + k*7 for k=0..4: pad at k=0,1,2; digits at k=3 ('4'), k=4 ('2').)
    Verify scr[(5+r)*0x140 + 100+35] ... check cols 100/107/114 = 0xb0 and 121/128 = 0xa4/0xa2,
    rows 5..11.
- T2 number=5 -> len=1, pad=4. Blanks at cols 100,107,114,121; '5'@128 (X=0xa5).
- T3 (edge) number=0 -> len=1 ("0"), pad=4. Blank x4 + '0'@128 (X=0xa0).
- Also verify the char_table[0] (pad) read: if char_map[16]=0 (unset), pad glyph rle = rle_base+0.

## Reference: 0x038c23 (already ported) differs only in: no pad loop, 2 fixed glyphs with a
3-elem rotate of buf, dest_off=(1-i)*7+col0. 0x038b30 generalizes to a variable-width
right-justified field with blank padding.
