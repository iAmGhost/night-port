# The palette chain is verified correct end to end; the stage's missing look is the .wal background blit

This session closed the palette question completely and relocated the stage-visual problem with hard evidence.
Nothing here changes game semantics: everything added is host-side diagnosis.

## Measured, in order

1. **The PCX palette loaders are correct.** `fn_035f1b` / `fn_036041` / `fn_036190` / `fn_036417` read 768 B at
   file end (`fn_04192e(g_dat_636b8, 3, 0x100, …)`) and scale every byte `>> 2`. Probed at the load instant
   (`dbg_pal_load`), the table for `logo.pcx` is `0,0,0 44,34,36 …` = exactly `logo.pcx`'s trailing palette
   `>> 2`, and 758-765 of 768 bytes are non-zero for every screen actually loaded (`title1/3/4.pcx`,
   `1s1/1s2/1s3.pcx`, `stage1.pcx`).
2. **The DAC values are 6-bit.** Every fade target dumped from `fn_040660` has max byte 63, so the host's
   `value * 255 / 63` upscaling is right. (The earlier "the game pushes full 8-bit values" note was an
   artifact of reading the *already scaled* `g_palette8` mirror.)
3. **The fades reach their targets.** The push trace (`bo#… start n buf from=first`) shows the full
   ramp in 1-unit steps, e.g. the logo fade climbing to entry 1 = `(44,34,36)` = `logo.pcx>>2`, then fading
   out — the fade machinery (`fn_040660` / `fn_0406f5` / `board_out`) is faithful.
4. **The room palette is real stage data, byte-exact.** The room fade target is `room + 0x48`; the dump
   `/tmp/palfade005.bin` compares to `stage1.wal[320030 : 320030+768]` with **mean absolute difference 0.0**,
   i.e. identical. Per `scripts/decode_stg.py` the `.wal` segment layout is
   `bg map (W*200 bytes) then u8[768] palette`, and 320030 is exactly that palette slot for segment 0
   (`seg0 orient=0 rect=(0,0)-(1600,200)`, 320000-byte index map at 30).
5. **The room composite and the page flip both work.** The composite target is the back screen
   `DAT_633b0 = SCREEN_BASE = 0x200000` (a flat arena address, *not* the VGA linear window), and
   `fn_038098` really does take the draw branch (`base=0x200000 DAT_53104=70 n=7 DAT_63e24=0 -> draw+flip`)
   and execute its flip `cpy_dw(video_map(base), 0x3e80, g_video_ram)` (16000 dwords = 64000 bytes).
   Dumping the back screen (320x200, pitch 0x140) shows a real frame: HUD strip top-left, two walking
   characters, 248 distinct indices — and the 320x200 view of `g_video_ram` matches it, so the flip lands.

## What is actually wrong: no `.wal` background in the frame

The stage ground truth, decoded offline straight from `stage1.wal` segment 0 (index map + its own palette,
`/tmp/wal_seg0.png`) is a dark urban platform scene: a train carriage, a "BONNIE R" billboard, a DANGER
barrier, tiled pavement.

Comparing that source map against the port's back buffer (`vidscr01200.bin`) across every camera offset
0…1280 (20-px coarse, then ±20 fine) gives a matching-index fraction of **0.004 at every offset** — the
statistical chance level. The `.wal` background index map is therefore **never** copied into the back
screen: what is drawn there is stale slide/sprite data plus the HUD, and the (correct) stage palette turns
that stale data into the pink/cream speckle the dumps showed.

So the open stage task is *not* the palette: it is the background-plane path — whoever is supposed to blit
the camera-selected 320x200 slice of the segment index map (segment width 1600 in seg 0, strip height 200)
into `DAT_633b0`. `fn_038098`'s draw branch calls `fn_03ce0b(room, dst)`, `fn_03203d`, `fn_03df96`,
`fn_03b953`; none of them is producing the wal slice, and the `fn_03ca5e` chain comment ("it is what fills
the entity-type-adjacent stage geometry the sprite path (fn_03ce0b) later reads") points at the geometry
fill rather than at a background copy.

## Next step (concrete)

Find the original's background blit by walking xrefs to the `.wal` segment data pointer:
1. locate where the master data loader (`0x03690b` family) stores the segment's bg-map address (seg 0 map is
   320000 bytes, so the block is easy to spot in the heap), and which room-record field keeps it;
2. from that field's readers find the per-frame copy that walks 200 rows against the camera x
   (expect a source stride of `x1-x0` = 1600 and a destination stride of `DAT_63e64` / 0x140);
3. port or repair that function, then re-run the correlation test above — the match fraction should jump
   from 0.004 to ≈1 for the visible slice.

The offline `.wal` decoder in `scripts/decode_stg.py` plus the dumps written by the host
(`vidscr<frame>.bin`, `pal<frame>.bin`, `palsrc<frame>.bin`, `palrot<frame>.bin`, `/tmp/palfade*.bin`) are
already sufficient to verify that fix without touching DOSBox.

## Diagnostics added (all host-side, gated)

* `dbg_pal_load(...)` in the four screen loaders: table non-zero count + first entries at the load instant.
* `fn_040660` fade-target log + `/tmp/palfade###.bin` dumps of every fade target table.
* Bounded `board_out` push trace with `dladdr`-resolved call sites (`NIGHT_DBG=1`, cap raised to 4000).
* `fn_038098` pacing/branch log (`n`, `DAT_53104`, `DAT_63e24`, draw-and-flip vs skip).
* Host dumps: `vidscr<frame>.bin` (back screen, 64000 B) and `frame<frame>_scr.bmp` (320x200 render of it),
  next to the existing `pal/vid/palsrc/palrot` bins.
* `heap_chain_check` (from the previous session) still attached at `fn_03a829`; it reported no corruption in
  a 2124-frame run.

Suite unchanged and green: **2787 PASS / 0 FAIL**, ASan clean, `make selftest` ALL PASS.
