# The port reaches the menu and loads a stage (SDL host, 2026-07-13)

Follows docs/32. Running the SDL host with a scripted key press now gets **past the whole attract chain
into a live stage**: the room play loop runs with entities on the list, and it does so under ASan with no
memory errors.

```
$ SDL_VIDEODRIVER=dummy ./port/test/night --frames 6000 --autokey --dump /tmp/night
disc: 15 tracks, 269453 sectors, lead-out MSF-derived
display mode 640x480
frame 240:  640x480 colours=61  dac lit=255 ... stage=1 W=655360 pitch=640  room=0x0        entities=0
frame 600:  640x480 colours=256 dac lit=255 ... stage=1 W=655360 pitch=640  room=0x100ebd848 entities=3
frame 1200: 640x480 colours=256 ... room=0x100ebd848 entities=3            <- room loop running
```

`--autokey` pulses the confirm key (DAT_63ea4 + scan-code slot 0x1c) every 5 s: the attract chain
(logo hold → opening .fli → title wipe + release drain → the menu itself) each waits for a key, and the
menu's own selection times out to the attract value (3), so a single press gets consumed by whichever
phase is up. The pulse is clocked off the clock, not off presented frames, because a key wait runs only
the input pump.

## Four real port bugs found getting here

1. **The DS sprite-record array had no storage.** `g_dat_5d0b4` (DS 0x5d0b4) was declared as a NULL
   pointer; in the original it is a *static data-segment array* running to the next object (the resource
   index at 0x5d884), i.e. exactly `0x7d0` bytes = **100 records of 0x14**. Stage entry fills it from
   `stage<N>.stg`, the sprite blitters (`fn_03df96`, `fn_03d75c`) read it. NULL → write to zero page.
   Fixed with `static uint8_t g_tab_5d0b4[0x7d0]`.
2. **The resource index table was sized for 128 entries; the shipped disc has 444.** `fn_0366a4` expands
   `count * 0x15` bytes into `g_tab_5d884`, so loading the real index wrote ~6.6 KB past the array and
   corrupted the globals next to it. Now `512 * 21` bytes (0x5d884 + 0x2a00).
3. **`struct hud_font` lied about the sprite/HUD-font block.** Its table fields were modelled as widened
   host pointers, but the loaders (`fn_03d8d4`/`fn_03d9cf`, and `fn_03de85`/`fn_03dcbd` which free them
   with `heap_free_host`) store **heap offsets** there — which is how `fn_03d75c` already reads the same
   block. Writing dwords into 8-byte pointer slots left garbage in the upper half (`0x1d2f00051cab` was
   the observed "pointer"), so `fn_03df96` read wild addresses. The struct is now the honest 0x3c-byte
   layout with `uint32_t` offsets (`+0x04/+0x08/+0x0c/+0x10` tables, `+0x34` char table, `+0x38` RLE base)
   and every consumer resolves them against `g_heap_base`. This unifies the convention noted earlier for
   `font+0x34/+0x38` — see docs/12-functions/03d75c-record-float-decay.md, which flagged the discrepancy.
4. **The clock stopped during key waits.** The busy-waits were patched to call the pump (docs/32), but
   `sys_key_spin()` only ran the *input* pump, so the tick-driven counters a key wait depends on
   (`DAT_63f08`, the paced pumps) froze for the whole wait — the attract menu could never time out and a
   release drain never let go. `sys_key_spin()` now runs the tick pump too, matching the original, which
   kept taking timer interrupts while it waited for a key.

## Video geometry, settled as far as the data allows

`fn_03f15f` sets VBE mode **0x101** and takes `DAT_63e64` (the pitch `fn_03f462` blits with) from the mode
info; the title/attract path then blits a full **640x480** image (`fn_03f462(0, 0, 0x280, 0x1e0, buf)`
over `0x4b000`-byte buffers), so the mode is 640 wide and `DAT_531f4` (its wrap width) is a 640 KiB linear
region based at 0xa0000. The arena therefore had to grow from 64 KiB to `0x160000` — that was the write
past the end of `g_video_ram` reported in docs/32, and the model's screens move to `SCREEN_BASE = 0x200000`.
Gameplay content is 320 wide (screens are `0xfa00` = 320x200, and the RLE picture loader writes pitch
`0x140`), so a 320x200 dump sits next to each full-mode dump (`frameNNNNN_320.bmp`). No PCX headers exist
in `NIGHT.RES` or the ISO image (the `.pcx` streams are the game's own mangled RLE with a leading 0x300-byte
palette), so the geometry is read off the loaders rather than off file headers.

## Suite state

`make -C port test`: **2746 PASS, 0 FAIL**; the ASan suite is clean; `night` under ASan runs 8000+ frames
across the stage boundary with no report. `test_fn_03df96` was reworked to build its font fixture in the
heap (offsets), which is what the fixed struct now expects.

## Still open for playability

- The stage loop runs but the on-screen result has not been eyeballed against the original: compare
  `frameNNNNN_320.bmp` with a dosbox capture of the same room.
- Room-to-room progression (the transition tables `g_tab_52be0`/`g_tab_52da4` have no producer in the EXE
  — they are image data from the disc; docs/24 notes this).
- Sound: the device command stream is modelled but silent.

## Visual fidelity: what the stage dump actually shows

Two character figures, a HUD frame box top-left and the room's plane structure are laid down correctly
(`--autokey` run, `frame01200_320.bmp`), and the DAC cache confirms the game programs it with **6-bit
values** (measured: no byte above 63 is ever written), which the host now scales to 8 bits like a real DAC.
But the colours are wrong for the scene -- pink/green speckle over the background plane -- so the palette
in effect is not the one belonging to this room's art, and/or the background plane indices are not being
filled from the right stream. That is the next thing to chase, and the way to chase it is a side-by-side:
capture the same room from the original under dosbox-staging and diff the two 320x200 frames, which will
separate "wrong palette pushed at the wrong time" from "background plane decoded from the wrong offset".
