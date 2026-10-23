# The stage/title palette question: what has been measured, and the next step

The SDL host reaches the real title/attract screen and a real stage, and the **pixel composition is
recognisable** (title lettering, `START GAME` / `EXIT TO DOS`, the 1997 credit lines, and in the stage the
character sprites plus the HUD frame), but the colours are wrong: the picture is washed out. This document
records what has been measured so far, so the next session does not repeat the work.

## Ground truth available offline

`scratch/dosbox/captures/` holds real captures (`obs_4_stage.png` is the title/attract screen: dark red
sunset sky, black city silhouette, white menu text). `port/src/res/res_pcx.c` records the verified rule for
slides: the display palette is the **raw last 768 bytes of the PCX, RGB order, no shift** — cross-checked
against `scratch/dosbox/captures/neu_menu2.png`.

The extracted resource tree (`scratch/res`, 174 PCXs) is available, e.g. `title1.pcx`, `title3.pcx`,
`title4.pcx`, `logo.pcx`, `1s1.pcx`, `stage4.pcx`.

## What the port does with the DAC (verified correct)

* `board_out` (0x040042) = `outb(0x3C8, start)` then `3*b` bytes to `0x3C9`.
* The sys model keeps the DAC table with the hardware's auto-indexing (`sys_outb`, ports `0x3C8`/`0x3C9`),
  plus a single-register path `sys_vga_palette_set()` for the VBE write.
* Unit-checked both ways: `board_out(0,0x100,buf)` reproduces `buf` exactly in `sys_vga_palette()`
  (`test_fn_03f3be`, 3 new checks), and `fn_03f3be` / `fn_03f410` (VBE INT 10h set-palette-register) now
  program that same table from the packed RGB dword (byte0 = R, then G, then B) instead of being a pure
  cache write. Suite: **2775 -> 2778 PASS**, ASan clean.
* `fn_040660` / `fn_0406f5` are the ascending/descending DAC fade pairs (`local[j] = src[j] * i / frames`),
  so a fade can only make the table **darker** than the source table — it cannot explain an over-bright
  picture.

## Who actually pushes palettes (measured in the running host)

`board_out` now records its distinct call sites; the host logs them per dumped frame:

| caller | pushes by frame 240 | what it pushes |
|---|---|---|
| `fn_038a20` (company-logo screen) | 1 + 50 + 51 | `DAT_633b8` (cleared to black) then the `DAT_636b8` fade pair |
| `fn_03f885`, `fn_036417`, `fn_03a224` | 1, 1, 51 | screen/loader palettes, `DAT_633b8` row rotation (`fn_03d67c` then a 765-byte push) |
| `fn_03af18` (game main) | 5 + 75 + 78 + 50 + 51 | the slide fades |

So the palette lives in two buffers: **`DAT_636b8`** = the palette that belongs to the screen the loader
just decoded, and **`DAT_633b8`** = the live/active table the in-game code rotates rows of.

## Negative results (so they are not re-tried)

* The DAC table in the running host is **not** any PCX palette in the container: compared against all 174
  PCX palettes, the best mean absolute difference is 61-84 per byte (i.e. unrelated), and a shift-search
  over register offsets found no alignment under 30.
* The exact triple the mirror holds (`134,81,56` and friends) appears **nowhere in any resource file**.
* The palette mirror is stable across frames 30-1200 while the DAC write counter keeps growing, i.e. later
  pushes do not change registers 0-9 — consistent with pushes that start at a non-zero register
  (`board_out(hdr[2], (0x100 - hdr[2]) & 0xffff, tab)`).
* Screen content is *not* shifted or mis-pitched: the dominant indices in the 640x480 arena map to muted
  mid-tones (blue-greys/creams) rather than the sunset reds of the title, i.e. content and palette belong
  to different screens.

## The next diagnostic (concrete)

Instrument the screen loaders (`fn_03628e` / `fn_036190` / `fn_035e..`, the ones with
`if ((dl & 0xff) == 1) board_out(0, 0xff, g_dat_636b8);`) at the moment they fill `DAT_636b8`, and compare
those 768 bytes against the trailing 768 bytes of the PCX being loaded:

1. if they **match**, the loader is fine and the fault is downstream — some later push (a fade from an
   empty/stale `DAT_633b8`) replaces the table on screen;
2. if they **differ**, the palette extraction is the port bug (wrong offset, or a `>>2` applied where the
   original keeps raw bytes — note `f.h` still labels `g_dat_633b8` as a "font/glyph table", which the
   palette-row rotation code contradicts).

Either answer is one focused session of work, and the raw dumps needed for it are already in place: the
host writes `pal<frame>.bin` (the DAC table) and `vid<frame>.bin` (307200 bytes of screen) next to each BMP
dump.
