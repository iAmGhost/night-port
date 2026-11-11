# 330 — Analogue Pocket port: the target, the budgets, and the mapping decisions

Goal: run the existing NIGHT port (the function-to-function C port, `src/f` + `src/sys` + `src/res` +
`src/cd`) on the **Analogue Pocket** as an openFPGA core, built with the openfpgaOS SDK at
`~/Git/openfpgaSDK`, packaged as an installable core the user puts on the Pocket SD card.  The port lives in
a new `./pocket/` directory inside this repo; `src/f` and friends stay shared with the SDL host
(`build/night`), so the fixture suite and the differential rig (docs/151) keep guarding both builds.

This document is the pre-implementation analysis: what the target actually is, what the two precedent ports
prove, what the port costs today (measured), what the budgets therefore are, and which subsystem maps to
which mechanism.  Every number below is either read out of a file named in the line, or measured with the
command printed next to it.

## 1. What the target actually is (read from ~/Git/openfpgaSDK)

The SDK at `~/Git/openfpgaSDK` is **not** the classic "write a VHDL core" openfpgaSDK workflow.  It builds
**apps for openfpgaOS**: an operating system for a RISC-V soft SoC that *is* the FPGA core.  One core is
installed once; every game is a `.elf` the core loads from the SD card (`README.md`, `GETTING_STARTED.md`).
No HDL, no Quartus, no bitstream build in this repo — `runtime/pocket/` ships prebuilt `.rbf_r` images.

SoC facts (`README.md` "Hardware", `src/sdk/include/of_caps.h`):

| item | value |
|---|---|
| CPU | VexiiRiscv **rv32imafc @ 100 MHz** (`of_caps.cpu_freq_hz`), in-order single-issue, **8 KiB I-cache + 32 KiB D-cache** |
| SDRAM | 64 MB total, cached + uncached aliases (`sdram_base`, `sdram_uncached_base`) |
| on-chip RAM | BRAM 32 KiB, of which **~24 KiB app BRAM** (`OF_FASTTEXT`, VMA 0x2000-0xFE00) is zero-wait-state hot space (`of_bram.h`) |
| video | **320x240 physical** (`of_video.h`: `OF_SCREEN_W/H`, `OF_VIDEO_PHYSICAL_WIDTH/HEIGHT`), triple-buffered, 6 colour modes (8/4/2-bit indexed, RGB565/555/RGBA5551); 8-bit FB = 76,800 B |
| audio | 48 kHz stereo out; **32-voice hardware PCM mixer** with automatic resampling, mono s16 samples living in an 8 MB SDRAM sample pool (`of_mixer.h`); sample-based MIDI with a `.ofsf` bank (`of_midi.h`) |
| storage | SD card through the APF data-slot bridge; `fopen` by filename, `of_file_read` DMA, **`of_file_read_async`** (one read in flight, CRAM0 staging), `of_file_size`, an idle hook during bridge waits (`of_file.h`); **ISO9660 read-only mount** (`of_mount.h`); 10 nonvolatile **256 KiB** save slots (`data.json` `size_maximum 0x40000`) |
| two bitstream variants | `os25` (2.5D: span groups, GPU alpha, **hardware mixer**, analogizer/SNAC, 4-player) and `os30` (triangles + Z-buffer, **no hardware mixer** → software mixer).  Pick with `VARIANT=` in the app `.ini`; gate features on `of_has_feature()`, never on the variant |
| desktop parity | `make test` builds `app_pc` from the *same sources* with `-DOF_PC` against `src/sdk/pc/of_sdl2.c` — this is the PC verification path (`src/sdk/sdk.mk:281-293`) |
| dev loop on hardware | `make copy` (SD tree) and, with a DevKey cartridge, `make debug`/PHDP UART streaming at 2 Mbaud |

Memory map (`README.md` "Memory Map"): BRAM 0x0000-0x7FFF; kernel ~128 KiB at 0x10300000;
**app code+data 0x10400000-0x13400000 (48 MB, loaded from the ELF)**; app heap/mmap 0x13400000-0x13700000;
mixer sample pool 8 MB at 0x13700000; CRAM0 save window 0x20100000 (10 × 256 KiB stride) and shared config
256 KiB at 0x20380000.  `of_get_caps()` reports `heap_base`, `heap_size`, `fb_*`, `sample_base/size`,
`sdram_size`, `cram_size`, `hw_features`, `mixer_voices/rate`, `platform_id`, `cpu_freq_hz`.

Data slots are *not* limited to the base template's 4-6: a custom core's `data.json` may bind many slots
(Quake binds 4,5,6,7,20,21,22 and **23-31 for its CD tracks**, `~/Git/openfpgaOS-Quake/dist/quake/Cores/
ThinkElastic.quake/data.json`).  App-side filename→slot registrations are capped at **32** (`of_file.h`),
and registry filenames are limited to **23 characters**.

## 2. What the Quake precedent proves (`~/Git/openfpgaOS-Quake`)

The Quake port is the closest shipped example of a 1996 software-rendered PC game on this SoC, and three of
its measurements carry directly to NIGHT:

- **Render size 320x200**: `src/quake/engine/vid_of.c:34-35` (`QUAKE_VIDEO_WIDTH/HEIGHT`), inside the 320x240
  panel framebuffer — i.e. a 320x200 source picture is the house style, not a compromise.
- **CD-DA is streamed, not decoded**: `src/quake/engine/cd_of.c` plays raw CDDA `.bin` tracks (44.1 kHz
  stereo s16, no header) as **two mono mixer voices over a 2-second SDRAM ring** (≈345 KB), refilled
  *behind* the play cursor by `of_file_read_async` 64 KiB DMA reads (≈0.37 s of audio per read, ~3 reads/s),
  with `of_cache_flush_range()` after every write because the mixer is an external AXI master.  It records
  the decisive number: a **blocking `fread` of that 176 KB/s stream measured ~13 % of total CPU**, which is
  why the async DMA path exists.  Music voices are tagged as a *group* so music can never steal an SFX
  voice.
- **Toolchain headroom is small and measurable**: their Makefile reports `-O3 -flto` worth ~13 % over `-O2`
  (113.1 ms → 98.3 ms on a dense scene) at +10 % text, and states maps of 16-35 MB are "out of reach
  against a 24 MB engine heap".  Budget discipline is the whole game here.
- A custom core JSON is normal: `core.json` carries both `os25`/`os30` images plus the `loader.bin` chip32
  VM, and per-game `.ini` files carry `VARIANT=`, `ARGS=`, and app-specific keys read by the app itself.

## 3. What the NIGHT port costs today (measured on this machine)

Original-side sizes (`stat`):

```
NIGHT.EXE    311,831 B
NIGHT.RES   50,964,894 B   (444 assets, docs/02)
TG_NIGHT.IMG 633,400,656 B = 269,303 x 2352-byte sectors
```

`TG_NIGHT.CUE` → 15 tracks; the IMG carries **no** PREGAP, so IMG sectors = disc frames − 150
(the same offset `src/cd/cd_iso.c` uses).  Track layout in sectors/bytes:

| track | start frame | sectors | raw bytes | minutes |
|---|---|---|---|---|
| 01 MODE1/2352 (data) | 0 | 37,112 | 87,287,424 | 8:14.62 |
| 02 AUDIO | 37,112 | 3,153 | 7,415,856 | 0:42.03 |
| 03 | 40,265 | 19,639 | 46,190,928 | 4:21.79 |
| 04 | 59,904 | 17,127 | 40,282,704 | 3:49.69 |
| 05 | 77,031 | 19,357 | 45,527,664 | 4:18.09 |
| 06 | 96,388 | 18,290 | 43,018,080 | 4:03.87 |
| 07 | 114,678 | 21,480 | 50,520,960 | 4:46.40 |
| 08 | 136,158 | 24,839 | 58,421,328 | 5:31.19 |
| 09 | 160,997 | 23,973 | 56,384,496 | 5:19.64 |
| 10 | 184,970 | 27,697 | 65,143,344 | 6:09.29 |
| 11 | 212,667 | 10,329 | 24,293,808 | 2:17.72 |
| 12 | 222,996 | 17,856 | 41,997,312 | 3:58.08 |
| 13 | 240,852 | 17,868 | 42,025,536 | 3:58.24 |
| 14 | 258,720 | 9,751 | 22,934,352 | 2:09.01 |
| 15 | 268,471 | 832 | 1,956,864 | 0:11.07 |

Audio tracks 02-15 total **546,113,232 B**; data track + audio = 633,400,656 B = the IMG exactly.  A
2352-byte raw audio sector is 588 stereo s16 frames, i.e. **the audio sectors of this IMG are already
headerless little-endian PCM**: the per-track extraction is a byte-range copy, no de-interleave, no
header strip, no transcoding.  (Track 02's 3,153 sectors = `0xc51`, the number `src/cd/cd_iso.c:21-22`
requires the disc-present gate to see.)

Port-side cost, measured with the existing host:

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_BOARD=1 \
  ./build/night --script scripts/playtest/route_show_keyed.txt --warp-stage 1 --no-audio --frames 3000
  → real 55.2 s / user 55.0 s  (54.5 fps presented, one core saturated)
  → memstat ... frames=2994 arena=3,903,168/50,331,648 rss_max=27,328 KB
size build/night → __TEXT 409,600   __DATA 69,730,304
```

- `rss_max` 26.8-27.3 MB is *touched* memory (macOS `ru_maxrss`); `arena` is the port's own DPMI
  substitute (`heap_arena_used`), i.e. the game's live allocations are **≈3.9 MB** in stage 1.
- The `__DATA` 66 MB is almost entirely one static array: `s_heap_arena[NIGHT_HEAP_ARENA_MAX]` with
  `NIGHT_HEAP_ARENA_MAX = 64 MiB` (`src/f/f.c:3288-3289`), and the SDL host raises the *usable* cap to
  48 MiB (`src/app/main.c:1956`).  **The 64 MiB static array cannot exist on the Pocket** (48 MB whole
  app region); it is a one-line shrink for the Pocket build.
- `g_video_ram[VIDEO_SIZE]` = 1,441,792 B (`src/f/f.h:1012`): the game's linear A0000 screen model.  The
  two paths `src/app/main.c:166-170` names are what the video mapping has to serve: the **room path
  composites 320x200 (pitch 0x140, 0xfa00 = 64,000 bytes) and flips exactly those 64,000 bytes to the
  framebuffer start (fn_038098)**, while the **slide path fills the whole 640x480 region (fn_03f462,
  count 0x1e0)**.
- Pacing has two sources, both already modelled: the PIT tick pump (`fn_03ee88` at the programmed
  1193182/0x982 = **490 Hz**, `src/app/main.c:1747-1760`) and the **0x3DA retrace wait** `fn_00035ec2`
  (docs/47) which paces fades, holds and slideshow cards.  `delay_units` is free (docs/47).
- Audio is not a stream in the original: the game programs a Sound Blaster (DSP cmd 0x40 time constant per
  sample, DMA ch1) and the sys layer turns each transfer into PCM bytes (`src/sys/sys.c:149-290`,
  docs/04, docs/98).  Per-transfer mono 8-bit PCM at a per-sample rate is *exactly* the shape of
  `of_mixer_play(pcm, count, rate, pri, vol)`.
- The shipped game's own spec is the strongest feasibility argument: `README.TXT:42-43` of the install set (CP949)
  requires **minimum 486DX2-66, recommended Pentium 75 MHz or better**.  A 100 MHz single-issue cached
  RISC-V sits inside that envelope for scalar code, so the game's own per-frame work is expected to fit —
  the risk is our host overhead, not the game.

## 4. Budgets that fall out

**SD / slots.** `NIGHT.RES` (51 MB, 1 slot) + 14 audio track `.bin` files (546 MB, 14 slots) + core
images (a few MB) ≈ 600 MB — trivial for an SD card, and 15 data slots is comfortably inside both the
"many slots in data.json" precedent and the 32 registration cap.  Keep names ≤ 23 chars.

**RAM.** With the arena shrunk to 8-16 MB (8 MB leaves 2× headroom over the 3.9 MB measured in stage 1;
the 9-stage sweep in task-8 must confirm the worst case), the app footprint is roughly: text ~0.4 MB,
`g_video_ram` 1.4 MB, static DAT tables ≈5 MB (66 MB `__DATA` − 64 MB arena), arena 8-16 MB → **well under
the 48 MB app region**.  Note the *shape* of that requirement: the arena must stay a **static array in the
app region**, not `malloc`, because the published heap window is only 0x13400000-0x13700000 (3 MB); the
port's design already matches (it substitutes a static arena for DPMI).  Host-side `malloc` (slide staging,
RES index, mixer staging buffers) has to live inside the reported `heap_size` — measured and asserted at
startup in task-2b.

**CPU.** 100 MHz, 8 KiB I-cache.  The port today saturates one modern core at ~55 fps, which is *not* a
parity number — it is a measurement of a superscalar host running an SDL3 host with an RGBA8888 texture
copy per frame (`src/app/main.c:172-190`).  The Pocket host must be cheaper per frame in the ways that
matter: 8-bit indexed writes into an already-indexed framebuffer (no palette expansion to 32-bit), no
texture upload, the retrace wait mapped to `of_video_vsync`/real time instead of a spin loop, and
`of_file_read_async` instead of blocking `fseek/fread` (the Quake port measured a blocking read of a
176 KB/s stream at 13 % CPU).  Task-9a puts a real number on this by instrumenting the frame budget.

**Saves.** The 10 APF save slots are **256 KiB each, fixed** (`data.json size_maximum 0x40000`).  A
whole-state snapshot (DS image + 3.9 MB arena) does **not** fit, and the shipped game appears to have no
save-file feature: the only INT 21 write path in the port is the Watcom buffered-IO layer
(`src/f/f.c:10712-10810`, `sys_file_write` is deliberately "write to the void", `src/sys/sys.h:114`).  So
"saves" on the Pocket is a **port-side feature, not a faithful-port requirement**: either a compact progress
record (stage, stats, inventory, flags — needs its own spec and verification) or none.  Decision needed
before task-7 closes.

## 5. Subsystem → mechanism mapping (the plan)

| subsystem | Pocket mechanism | notes |
|---|---|---|
| host | new `src/app/host_of.c` in `./pocket`, using `of_*` directly | **not** the SDK's SDL2 shim: the port is written against SDL3 (`#include <SDL3/SDL.h>`, `SDL_PIXELFORMAT_RGBA8888`, `SDL_CreateTexture(..., w, h)`, `SDL_SetTextureScaleMode`), which the shim does not provide.  Keeping the SDL host intact for `build/night` also keeps the rig/fixture harness intact. |
| gameplay video | 8-bit indexed `of_video_surface()`; copy the 64,000-byte 320x200 work screen 1:1 into the 320x240 FB (40 spare rows → 20-row black bars) | indexed→indexed copy; the game already produces palette indices in `g_video_ram` |
| slides / .fli | 640x480 region → 2:1 box-average (or 2x2 pixel-select) downscale to 320x240 | downscale is where the 10x12 Korean glyphs (docs/219) get thin; the alternative (crop/scroll a 640x480 view) is worse for legibility.  Judge on hardware. |
| palette | `of_video_palette_bulk()` (or `of_video_palette_vga6`) from the game's own staging table `g_dat_633b8` / the DAC mirror | the port already distinguishes the two sources (`src/app/main.c:110-122`); the fade path writes 6-bit DAC values (docs/47, docs/77) |
| timing | `clock_us()` / `of_timer` drives the 490 Hz tick pump; `fn_00035ec2`'s retrace wait → real-time/vsync wait; keep the "fell far behind → re-anchor, don't burst" rule | docs/47 + docs/68: visible hold time comes from the retrace waits, so this mapping *is* the pacing fidelity |
| input | `of_input_poll()` + `of_btn_pressed()` → `night_key_apply(scancode, make)` (`src/app/keymap.c`) | must land on the **aliased** slots docs/91/92 measured: punch = 0x1d (CTRL), kick = 0x38 (ALT), jump = 0x2a (SHIFT), arrows 0x48/4b/4d/50, Esc 0x01, Enter 0x1c.  Pad plan: d-pad arrows, A→punch, B→kick, X→jump (L1/R1 as the modifier variants for the same three slots), Start→Enter, Select→Esc.  Measured in task-6. |
| RES | one data slot; reads through the existing DOS-handle layer backed by `of_file_read`/`of_file_read_async` (chunked, docs/13 block buffering) | 32 MiB `malloc`-the-whole-container is off the table: `res_open` (`src/res/res_container.c:11-20`) is a selftest path, the game path opens by name (`sys_file_open`), which is the right seam |
| CD data track | keep `cd_iso.c`'s sector semantics on top of a slot: 2352-byte sectors, user data at +16, TOC header/entry requests reproduced | alternative `of_iso_mount("night.bin","/cd")` exists but would bypass the sector/TOC emulation the disc-present gate depends on |
| CDDA music | per-track `.bin` slots + Quake-style 2 s ring + two mono `OF_MIXER_GROUP_MUSIC` voices fed by `of_file_read_async`, `of_cache_flush_range` after each fill | 44.1 kHz source, hardware resamples to 48 kHz; music-group tagging protects SFX |
| SFX | VOC-era 8-bit mono samples (`src/res/voc.c`, docs/04) loaded into `of_mixer_alloc_samples()` pool, one voice per SB DMA transfer with the DSP 0x40 time-constant rate | maps 1:1 onto the mixer; the pool is 8 MB, plenty for the family census in docs/64 |
| saves / config | 256 KiB save slots (or slot 8 shared config) for a **port-side** progress record only | see §4 "Saves" |
| variant | `VARIANT=os25` in the `.ini` | needs the **hardware mixer** and indexed 2D blits; `os30` would demote audio to the software mixer and buys nothing here |
| build | `-O3 -flto=auto`, `--gc-sections`, `OF_FASTTEXT` for the composite/blit hot path | Quake's measured 13 % for `-O3 -flto` is the free win to take |
| verification | PC: `make test` → `app_pc` (`-DOF_PC`, SDK SDL2 shim) runs the same game sources; repo-root `make test` fixture suite unchanged; frame dumps diffed against `build/night`'s dumps on the same scripted route.  Hardware: the user's SD-card run | docs/151 rig basis, docs/63 shipped-asset basis |

## 6. Open risks (named, with the test that settles each)

1. **CPU headroom (the big one).** No local number can settle it; it needs the device.  Mitigations are
   already listed in §4.  Test: instrumented frame budget on hardware (ticks/s, presents/s, mixer
   underruns) from task-9a, reported back by the user.
2. **Panel scaling of a 320x240 framebuffer onto the Pocket's 480x320 panel.** 1:1 for the 320x200 gameplay
   picture is only 1:1 *inside the framebuffer*; the core-to-panel scale factor (4:3 content on a 3:2 panel)
   decides legibility.  Test: one hardware screenshot of a slide card; if glyph legibility fails, revisit the
   slide path (e.g. re-render slides at 320x240 with the 10x12 font instead of downscaling 640x480).
3. **Whether the whole game's arena peak fits an 8-16 MB arena.** Test: 9-stage sweep (docs/59 route)
   reporting `heap_arena_used` per stage, before the arena size is fixed.
4. **SD read throughput for a 600 MB asset set** (level loads, `stage1.wal` is 835,254 B, docs/14).  Test:
   measured load-time for one stage on hardware; async DMA + the file idle hook (which exists precisely to
   keep audio pumping during bridge waits, `of_file.h`) is the mitigation.
5. **`of_file_read_async` single-in-flight limit** vs the port's interleaved music + stage-load reads.
   Test: the music-vs-load soak in task-5 (underrun census).
6. **No save feature in the original + 256 KiB slots**: needs a user decision (§4), not a technical fix.
7. **Korean glyph coverage in the terminal is irrelevant** (all text is drawn by the game's own bitmap
   fonts from `SYSTEM.SPR`), but any *port-side* UI text (menus we invent for the pad mapping) would need
   the same game fonts — the CP437 `of_terminal` cannot render Korean.
8. **SDL3 → no SDL2 shim reuse**: the rig/sink code (`src/app/rig_sink.c`) is SDL3-based and will not build
   against the SDK's SDL2 headers; the Pocket host needs its own dump path (PNG/PPM via `of_*` or raw
   arena dumps, matching the existing dump format so `scripts/playtest_compare*.py` still work).

## 7. Reproduce the measurements in this document

```sh
cd ~/Git/night-port && make app
stat -f "%z %N" TG_NIGHT.IMG extracted/NIGHT.RES extracted/NIGHT.EXE
iconv -f CP949 -t UTF-8 extracted/README.TXT | sed -n '40,45p'      # CPU requirement
size build/night                                                 # text 409,600 / data 69,730,304
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_BOARD=1 \
  ./build/night --script scripts/playtest/route_show_keyed.txt --warp-stage 1 --no-audio --frames 3000 2>&1 \
  | grep -E "memstat" | tail -3                                  # arena + RSS
time ( ... same command ... )                                    # 54.5 fps, 1 core
```

Track table: convert `TG_NIGHT.CUE` index times with frames = `(mm*60+ss)*75+ff`, sector = 2352 B,
and note IMG = disc frames − 150 (the PREGAP is not in the file).
