# The SDL host: the port boots, loads the real data and paints (src/app/main.c)

Added 2026-07-13. `make app` builds `build/night` against SDL3; `SDL_VIDEODRIVER=dummy
./build/night --frames 1200 --dump /tmp/night` runs headless and writes frames. State reached:
`DAT_633a0 = 444` (the real `NIGHT.RES` index, 444 entries), `DAT_63370 = 1` (the master init ran), the
disc gate satisfied by the real TOC (docs/31), the timer programmed at **490 Hz**, and **212 distinct
palette indices on screen** at frame 600 -- the slide-show/title art is being decoded and displayed.

## Architecture (cooperative, single-threaded)

The original drove the frame with hardware interrupts; the port delivers them at the points where the
original *waited*, which keeps the semantics and stays deterministic:

| original | port |
|---|---|
| the installed timer handler (`fn_03ee88`, INT 1Ch) | `sys_set_tick_pump()`: elapsed real time -> N handler calls at the rate the game programmed the PIT for |
| `INT 31h` PIT programming (`outb 0x43/0x40`) | a PIT model in `sys` (`sys_pit_hz(ch)`): fn_03ef29's `0x982` -> **490 Hz**, which is also the `0x1ea` (=490) frame budget the paced pumps divide by |
| the keyboard IRQ + pump `fn_0347e3` | `sys_set_input_pump()`: SDL key events -> the DS key bytes (`DAT_63ed3/63ed5` held, `DAT_63eb2/63ebe` any-key, `DAT_62c58[]` scan-code slots), then `fn_0347e3()` |
| VGA DAC (`board_out` -> `outb 0x3C8/0x3C9`) | a DAC model in `sys` (`sys_vga_palette()`, write counter) |
| the flat A0000 video window | `g_video_ram` sampled as 320x200 indexed and scaled x2 into an SDL texture |

Every original busy-wait in f.c is written

```c
while (cond) { if (!sys_port_spin()) break; }
```

`sys_port_spin()` runs the installed tick/input pumps and reports whether any exists -- so with a host
(the SDL app) the wait is a real wait, while the unit tests (no pumps) keep exactly the ported no-op
behaviour they were verified against (suite unchanged: 2746 PASS).

## Boot recipe used by the host (same as the real-data test, docs/15)

heap arena init + `heap_region_floor_set(0x10000)` + one 64 KiB floor allocation, `g_ndpool` on a static
pool, the per-type tables (`g_dat_639ec` / `g_dat_53818`) zeroed for a fresh boot,
`sys_file_real_dir(<NIGHT dir>)`, `cd_iso_open(<dir>/TG_NIGHT.CUE)` + `sys_cd_set_backend(...)`,
`g_dpmi_free_dos_paragraphs` above the `0x17c` startup-gate threshold, `g_dat_52ff8 = 1`, terminate
callbacks installed, then `fn_03af18()`.

## Fixed on the way: the CD far-pointer buffer was never allocated

`fn_000104c7` (the master bring-up's DPMI step) was a no-op, so `DAT_00053c5c` stayed NULL and
`fn_010927`'s `mem_copy(*g_dat_53c5c, bufA, 7)` wrote to address 0 -- a crash the moment the real game
main ran (the CD-family tests always supplied their own far buffer, which hid it). Its disassembly
shows what it really does: INT 2F AX=0x1500 (the MSCDEX presence check, whose reply's drive number goes
to `DAT_00053c58`), then `INT 31h AX=0x100 DX=0x11` -- allocate a selector over **real-mode paragraph
0x11** (the Watcom real-mode-call parameter block at 0000:0110), write the paragraph through it, and set
`DAT_00053c5c = base + 2`. The port now models that with a static block (`sys_rm_low_block()`), which is
also where the 24/26-byte real-mode call blocks and the 9-byte round-trips live.

## Open issue (next)

At ~frame 1300-2400 the host crashes inside `mem_copy` (size 640) writing just past a mapped page -- a
blit whose destination runs off the end of a screen buffer. The row length 640 also matches the VESA
640-wide geometry (`DAT_531f4`), so the suspect is a 640x480-mode blit against the 64 KiB video window
model (the port maps the window as 0xa0000..0xb0000 and screens at 0xb0000..+2*0xfa00), i.e. a place
where the original relied on the real LFB being 640x480 = 307200 bytes.
