# 219 — The shipped font descriptors, read out of the shipped heap: font A is 10 glyphs of col 4 x 7 rows, font B col 10 x 12 rows, and the port's glyph streams come out of the shipped resources

Two measurements that narrow item 2a by eliminating candidates.  Shipped side: `/tmp/rig/ents-orig/orig`
(drive `ep1_kill3`), the 352-byte block captured for the DS pointer cells `g_dat_639c0` / `g_dat_639c4` at
state 363.  Port side: `/tmp/rig/gd1` (`port/test/night --script ep1_kill3`, `NIGHT_GLYPHDBG=1`, unpinned clock so
the kill popup actually posts: 80 `expnum` + 80 `stagespr` + 60 `valuedisp` witness lines).

## 1. The shipped font A / font B descriptors

`DS 0x639c0`'s block (font A, the one `fn_03b718` uses when `entity[0] == 0`):

```
+0x00  0x0000000b                        11 (= glyph count)
+0x04  0x00292dac 0x00292ddc 0x00292e0c 0x00292e3c 0x00292e6c 0x00292e9c
+0x28  0x00292ecc 0x00292efc 0x00292f2c 0x00292f5c 0x00292f8c 0x00292fbc   (12 record pointers, stride 0x30)
+0x34  0x0029aea8  0x002a2d34            the +0x34 object (glyph-offset table) and the +0x38 RLE base
+0x3c  0x00000031                        49
+0x40  0x04 x 10                         per-glyph column count: 4 for all ten digits
+0x68  0x0d  0x31  0x07 x 10             and per-glyph row count: 7 for all ten digits
```

`DS 0x639c4` (font B, used for `entity[0] != 0`):

```
+0x00  0x0000000d                        13
+0x04..0x30  twelve record pointers (stride 0x38: 0x29af18, 0x29af50, 0x29af88, 0x29afc0, 0x2a2f40, ...)
+0x34  0x002b2eb8  0x002b2ef0            +0x34 object, +0x38 RLE base
+0x3c  0x00335544 ; 0x00000039
+0x40  0x0a x 10                         per-glyph column count: 10
+0x68  0x16 0x39 ; 0x0c x 10             per-glyph row count: 12
```

`fn_03b718`'s per-digit blit passes `push 7 ; mov ecx, 4` for font A and `rows 0xc, col 10` for font B
(guest 0x0003b7aa-0x0003b7ac and branch B at 0x0003b840-0x0003b914).  Those are exactly the numbers the shipped's
own tables hold: 4 columns / 7 rows for the ten font-A digits, 10 columns / 12 rows for the ten font-B digits.
The port hard-codes the same pair (`port/src/f/f.c:14532-14547`, docs/210 §1), so the metrics are not the
divergence, and neither is the digit index arithmetic (docs/215 §3) nor the per-digit advance `i*5`
(guest 0x0003b7c9 `lea edx, [edx+edx*4]`).

## 2. The port's glyph streams are the shipped resources, at real offsets

`NIGHT_GLYPHDBG=1` prints the byte window each blit consumes (`menucell tag=... dest=... srow=... col=... rows=...
stream=<hex>`).  Matching each witnessed window against the extracted CD resources under `scratch/res/`:

| witness | dest | col x rows | stream found in | offset |
|---|---|---|---|---|
| `valuedisp` (kill number, font A) | 11 | 4 x 7 | `scratch/res/system.spr` | 0x9a |
| `valuedisp` (kill number, font A) | 16 | 4 x 7 | `scratch/res/system.spr` | 0x38 |
| `expnum` (HUD field, `fn_0137c1`) | 65..89 | 5 x 7 | `scratch/res/exp.spr` | 0x102 |
| `stagespr` (room composite) | 1993..3514 | 20..173 x 22..173 | `scratch/res/st1s.spr` | 0x38, 0x112b, 0x19cf, 0x21c0, 0x261c, 0x3421 |

So the port is not reading a wrong font file, and its digit data is genuinely inside the shipped `system.spr`.
Combined with §1 (metrics agree) and docs/218 §3 (the shipped presents the number at the coordinates the shared
arithmetic predicts, rising and dying on schedule), item 2a's remaining candidates are narrow: which entry of the
`+0x34` glyph-offset table each side selects for a given digit (`[char_obj + digit*4 - 0xc0]`, guest 0x0003b7a3),
and `fn_03bb42`'s row/clip handling (docs/212 §4: the shipped's left/right clip arms at 0x0003bc43 / 0x0003bc4e are
mutually exclusive and it bails at `dest_off > 0x140` where the port tests `> 0x13f`).

## 3. What the next measurement is, stated so it is not repeated

Comparing glyph *shapes* between a shipped emulator frame reply (docs/218 §1: 2560 B/row, 8 bytes per reported
320-column) and the port's own saved 320x200 buffers is not admissible, and decoding a witnessed byte window as if
it were exactly `rows` rows of one glyph is not reliable either -- the witnessed window runs past the glyph (the
`stream` bytes of consecutive `valuedisp` samples at the same `dest` differ only by rotation, i.e. the window is a
fixed-size slice of the resource, not one glyph).  The admissible comparison is
`shipped +0x34 table entry for digit d` vs `port char_table - 0xc0 + d*4`, both resolved to offsets inside
`system.spr`, plus the row-RLE decode of exactly the bytes each side hands to `fn_03bb42`.  The shipped side needs
the 12 record pointers at `+0x04..0x30` (already in the dump) dereferenced -- which requires heap blocks beyond the
63 captured pointer cells, i.e. the `heapdump` drive step or a `--heap-bytes` increase in `replay_original.py`.
