# 51 — the logo screen only covers the first 64 KB of the framebuffer (open lead)

With the pacing fixed, the boot sequence now reaches the logo (`logo.pcx` is the first resource the game
opens, held ~1.4 s before `opening.fli`), but the **logo screen renders wrong**: only the top 102 rows of
the 640x480 screen carry anything, and everything below is black.

Measured per-row mean brightness (port hold-shot vs the PCX asset `scratch/res/logo.pcx`, 640x480):

| row | logo.pcx | port (t=400 / 600 ms) |
|---|---|---|
| 0 | 103.2 | 36.8 / 57.0 |
| 100 | 102.8 | covered |
| 120 | 105.2 | **0.0** |
| 240 | 124.0 | **0.0** |
| 450 | 46.4 | **0.0** |

102 rows x 640 px = 65280 ≈ **0x10000**, which is exactly the wrap the loader uses:

```c
void fn_03628e(const char *name, uint32_t dl) {      /* 0x03628e, the "screen RLE loader" */
    ...
    while (pos < 0x4b000) {            /* always decodes 307200 bytes (640*480) */
        base[off++] = value;
        if (off < 0x10000) continue;
        off = 0; chunk++; fn_03f3be(chunk);      /* wrap the 0xA0000 window; chunk -> palette step */
    }
```

The port models the 0xA0000 window as 64 KB and lets `off` wrap (documented as a latent quirk), so the
decoded image overwrites the same 64 KB about 4.7 times and only the last pass survives. For a 320x200
still (64000 bytes, e.g. `title1.pcx` via the `0xfa00`-bounded loader `fn_036190`) that is harmless and
the picture is **pixel-correct** — measured: title1.pcx row means match the port frame within ~1 %
(45.5/45.0, 65.9/65.5, 86.1/85.7, 93.8/93.5 …). For a 307200-byte still the same model cannot be right.

So the question is how the original reaches past 0x10000: an S3 SVGA 640x480 (mode 12h) framebuffer is
307200 bytes behind a 64 KB window, which needs **bank/window programming** (S3 extended register / CRTC
bank via 0x3D4-0x3D5 or the 0x3D8/0x3D9 bank register), and the port's `src/sys/sys.c` models only the
DAC (0x3C8/0x3C9) — no bank register at all. Two candidate readings, to be separated next:

1. the still screens really are 640x480 and the game sets the bank somewhere the port has not modelled
   (then the wrap-branch `chunk` counter, or a port write near the loader, selects the bank — and
   `fn_03f3be(chunk)` is not the palette call the port assumes), or
2. the logo is drawn in a 320x200 screen and `logo.pcx`'s 640x480 payload is a *pre-scaled* asset that a
   different code path resamples — in which case the port's 640x480 presentation for the logo is itself
   the host-side mistake.

## Disassembly: each 64 KB wrap makes a VESA call — the window has to slide

`0x36399-0x363d4` (the fill arm; the literal arm `0x363d8-0x3640a` is identical):

```
000363a2  mov  edx, [ebp-0x10]      ; off
000363a5  inc  dword [ebp-0x10]
000363a8  add  edx, [ebp-0x18]      ; + window base (0xa0000)
000363ae  mov  byte [edx], al       ; the pixel
000363b6  cmp  dword [ebp-0x10], 0x10000
000363bd  jl   ...                  ; still inside the 64 KB window
000363bf  mov  dword [ebp-0x10], 0
000363c6  mov  eax, [ebp-0xc]       ; chunk
000363c9  inc  dword [ebp-0xc]
000363cf  call 0x3f3be              ; per wrap, with the chunk counter
```

and `fn_03f3be` is not a palette write at all:

```
0003f3d9  mov  edx, [0x13e5c] ; an index slot (the twin fn_03f410 uses [0x13e58])
0003f3df  shl  edx, 2
0003f3e2  mov  esi, 0x13e50 ; a cache array
0003f3f5  mov  edx, [ebp-4] ; the argument
0003f3f8  shl  edx, cl      ; cl = [0x13e4c], a mode-dependent shift
0003f3fa  cmp  edx, [esi]   ; only act on a change
0003f3fe  mov  [esi], edx
0003f400  mov  eax, 0x4f05
0003f405  int  0x10         ; VESA AX=4F05h, DX = the value
```

So the still-image loader crosses 64 KB boundaries by calling **INT 10h AX=4F05h** with `DX = chunk << cl`
— i.e. it reprograms the memory window/bank, and 307200 bytes of a 640x480 screen are reached in five
64 KB chunks. The port models `fn_03f3be` as `palette_set(index, color)` and lets the offset wrap in place,
which is why only the last pass over the first 64 KB survives on screen.

Plan: give `src/sys/sys.c` a window model for the A0000 area (a bank base selected by that VESA call),
keep the palette path (`board_out` of the `>>2` table, which is what actually drives the fades) separate,
then let `fn_03628e` write at `window_base + off`. The fades still have to be verified afterwards, because
`fn_03f3be` currently doubles as the palette-step recorder.

Harness support added while measuring this: named events now queue a **hold shot** (`--shot-delay <ms>`,
default 1000 ms) so a screenshot is taken after the retrace-paced fade has brought the picture up — the
instant-of-lookup shots are black by construction. `docs/manual-playtest/02-shots/03-logo_hold_port.bmp`
(next to `03-logo_reference.pcx.png`) is the artifact; `02-shots/02-playing-hud-fixed.bmp` shows a path
that is already correct.
