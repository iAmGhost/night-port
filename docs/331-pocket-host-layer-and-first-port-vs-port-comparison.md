# 331 — The Pocket host layer (`pocket/src/night`): what it takes from the SDL host, and what the first port-vs-port comparison measured

Docs/330 fixed the budgets and the mapping.  This doc records the host layer that implements it and the
measurements that say it works, because the mapping decisions only become visible once a second host is
written against the same `src/f`.

## 1. Shape of the host

`pocket/src/night/main_of.c` is a twin of `src/app/main.c` over the openfpgaOS `of_*` HAL.  Nothing in
`src/f`, `src/sys` or `src/app/keymap.c` was forked: the Pocket `Makefile` compiles those files **in
place** from the repo (`VPATH = $(REPO)/src/f:$(REPO)/src/sys:$(REPO)/src/app`, basenames so the `.o`
files stay inside `pocket/.obj/`), and the desktop variant (`make test` -> `app_pc`) compiles the same
list plus `src/app/rig_sink.c` and the SDK's `pc/of_sdl2.c` shim.

Only one edit was made in the shared layer, and it is behaviour-neutral for every host that does not ask
for a different value:

```c
/* src/f/f.c:3285, was: #define NIGHT_HEAP_ARENA_MAX (64u << 20) */
#ifndef NIGHT_HEAP_ARENA_MAX
#define NIGHT_HEAP_ARENA_MAX (64u << 20)
#endif
```

The Pocket build passes `-DNIGHT_HEAP_ARENA_MAX=16*1024*1024` because the app region is 48 MB total
(docs/330 §4); measured stage use is 3.9 MB (`arena=3903168`), so 16 MiB is comfortable.  Repo-root
`make test` still prints `== ALL PASS` (2274 fixtures) with the guard in place.

`src/app/main.c` itself is **not** compiled for the Pocket: it is SDL3 (`SDL_CreateWindow`,
`SDL_PIXELFORMAT_RGBA8888`, `SDL_UpdateTexture`) and the SDK shim is SDL2.  What the Pocket host takes
from it is the *contract*, not the code:

| SDL host | Pocket host |
|---|---|
| window + texture + `SDL_UpdateTexture` | `of_video_*` 8-bit indexed surface, `of_video_flush/flip` |
| `palette_rgba()` DAC mirror + 6-bit scale | identical arithmetic, pushed with `of_video_palette_bulk` |
| PIT pump, 490 Hz, re-anchor after 8 periods | same arithmetic in `tick_pump` (docs/68) |
| keyboard pump + `fn_0347e3` | `of_input_poll` -> `night_key_apply` -> `fn_0347e3` |
| `board_boot()` (0xaa signature, 0xf2 -> IRQ, id 0x04 0x05) | same function, copied (docs/98) |
| SDL audio stream from `sys_audio_fill` | `of_mixer_play` into `of_mixer_alloc_samples` pool blocks |
| `cd_iso.c` (CUE + IMG via `fseek/fread`) | `of_disc.c` + compiled `disc_table.h` + one data slot per track |
| `--rig-dump` witness (`rig_sink.c`) | same `rig_sink.c`, linked into the desktop build |

## 2. Two bugs this comparison caught

**(a) The pump must not feed itself.**  First version of `tick_pump` did the arithmetic on every spin
unit.  `sys_port_spin()` is called ~10 million times a second inside f.c's busy waits, so the handler ran
**10,710,482 times per second** instead of 490, the game clock ran ~20 000x fast, and the display mode
flapped 640x480 -> 320x200 -> 640x480 every ~235 frames.  The reason `src/app/main.c` does not have this
problem is one line at `main.c:1746`:

```c
if (now < g_tick_next && !g_fixed_step_calls) return;
```

i.e. the arithmetic — and therefore the handler call — only happens when a tick is actually *due*.
Copied into `tick_pump`, the measured rate became **491 ticks/s, 55 frames/s, pit = 490 Hz**, matching
the SDL host's 457 ticks/s at 52 frames/s.

**(b) A relative data directory is a trap.**  `sys_file_real_dir("../../../NIGHT")` is right from
`pocket/src/night/` and wrong from the repo root, and the failure mode is not a host error message but
the *game's* own path: `fn_03690b` cannot open `night.res`, calls `fn_036537` (banner printf) which ends
in `fn_0423b7(0)` -> the registered `DAT_0005336c/70/74` terminate callbacks — printed as
`terminate requested ... frames presented: 0`.  The host now probes
`NIGHT`, `../NIGHT`, … `../../../NIGHT` for `NIGHT.RES` and prints the directory it chose.

## 3. Port-vs-port frame comparison (desktop shim vs the SDL host)

Both hosts run the shipped data with the same pinned LCG (`NIGHT_SEED=1234`), no injected keys, and the
rig's own witness sink, which stores a 40x30 luma signature per presented frame
(`src/app/rig_sink.c:549`, `frames.bin`, 32-byte record + 1200 bytes).

| | `build/night` (SDL3 host) | `app_pc` (Pocket host on the SDK shim) |
|---|---|---|
| presented frames in the window | 626 (1253 records: main.c presents twice per counted frame, `main.c:1525` + `main.c:1793` + the present hook) | 601 (1 record/frame) |
| tick rate | ~457/s | 491/s |
| mode timeline | 640x480 from frame 0, 320x200 from frame ~185 | 640x480 from frame 0, 320x200 from frame ~137 |
| **signature identity** | — | **distance 0 (byte-identical) at anchors**: pocket#20 = sdl#44, pocket#40 = sdl#92, pocket#80 = sdl#67, pocket#260 = sdl#654, pocket#300 = sdl#674, pocket#420 = sdl#999, pocket#480 = sdl#1127 |

So the same pictures appear, in the same order, at a ~1.1x frame offset.  The offset is a pacing
difference, not a state difference: the two hosts present at different rates, and `main.c`'s fixed-step
mode (`NIGHT_RIG_STEP_CALLS`) cannot align them record-for-record because its dump holds two records per
counted frame — per record it advances half the ticks.  Practical rule for this port: **compare the
Pocket host to the SDL host in wall-clock mode**, which is what the device actually runs; use fixed step
only for routes replayed inside one host.

## 4. Panel geometry, seen

`NIGHT_PANEL_SHOT=<dir>` writes the 320x240 indexed panel as PPM (desktop builds).

- `…/0000_320x240.png` (frame 20, 640x480 path): the intro card, halved by the 2x2 rule — the
  "…ENTERTAINMENT" wordmark and the crack texture are legible, no moiré, no lost strokes.
- `…/0009_320x240.png` (frame 380, 320x200 path): the dialogue/room frame 1:1 in the middle of the panel
  with 20 rows of bar above and below, and the subtitle "아빠, 어떻게 하죠?" crisp — the 10x12 glyph
  test of docs/219 passes at 1:1 without scaling.

The downscale rule is *majority of the four samples, ties broken by brightness* (`downscale_2x2`) rather
than top-left sampling, because averaging palette **indices** is meaningless; nearest-neighbour is one
`#define` away if hardware shows it better (docs/330 risk 2 stays open until real hardware).

## 5. Disc seam

`scripts/pocket/gen_disc.py` compiles `NIGHT/TG_NIGHT.CUE` into `disc_table.h` with the same arithmetic
as `src/cd/cd_iso.c` (LBA = MSF frames + accumulated PREGAP): 15 tracks, first 1 last 15, pregap 150,
`DISC_TOTAL_SECTORS 269453`, track 2 at 37262, track 3 at 40415 — i.e. the values the shipped disc
really reports, and the difference 3153 = 0xc51 that `fn_010c3e` compares (docs/67).  `of_disc.c`
installs that table as the `sys_cd` backend and reads audio sectors from `slot:N` stdio handles, one
data slot per track (`of_disc_track_slot`: track 02..15 -> slot 18..31, the same mapping
`scripts/pocket/mk_dist.py` writes into `data.json`).  Desktop builds read the identical per-track files
from `NIGHT_CD_DIR`, so the same code path is testable before hardware.

## 7. The CDDA read crash, and the one warning that must not be silenced

With the repacked track files in place (`NIGHT_CD_DIR=scratch/pocket/cd`) the desktop run died a few
seconds after the intro music started:

```
EXC_BAD_ACCESS KERN_INVALID_ADDRESS at 0x000000006bdbba10
  _platform_strlen <- __vfprintf <- _vsnprintf <- snprintf <- tick_pump.220 <- fn_03af18 <- main
```

Bisecting with the host's own `NIGHT_OFF=<present,mixer,cdda,pad>` switch (each subsystem turned off in
turn, three runs each) pointed at one place: `crashes=3/3` for every combination **except** any that
switched `cdda` off, which ran to completion 3/3.  A step-wise trace inside `of_disc_read_audio` then put
the fault between its fourth and fifth statement, and printing `getenv("NIGHT_CD_DIR")` there gave the
answer:

```
ofdisc 4 off_sectors=0
ofdisc 4a branch=PC getenv=0x6ddf3a1e        <-- in of_disc.c
env probe (the same variable, in main_of.c) =0x16ddf3a1e
```

`0x6ddf3a1e` is `0x16ddf3a1e` truncated to 32 bits.  `of_disc.c` called `getenv` without including
`<stdlib.h>`, while the Makefile carried `-Wno-implicit-function-declaration` — the warning diet borrowed
from the Quake port — which demotes an implicit declaration from an error to a silence.  An implicitly
declared `getenv` returns `int`, so the pointer was cut in half and `printf("%s", it)` faulted.  Fixed by
including `<stdlib.h>`, and the pocket Makefile no longer allows that class of bug at all:

```make
CFLAGS += -Werror=implicit-function-declaration   # the rest of the 1990s diet stays silenced
```

It matters twice over here: on rv32 a wrong-width return is 32-bit anyway, so the same slip would have
been invisible on the target.  After the fix, three runs of 1500 frames each finished cleanly with the
CDDA track consumed at the disc's own rate — `cdda=1840/3153` after 24.5 s of wall time, exactly 75
sectors/s.

Two smaller findings from the same session, both kept as host behaviour:

- `sys_file_real_dir` on the desktop build now probes `NIGHT`, `../NIGHT`, … for `NIGHT.RES` and prints
  the directory it chose, because a wrong relative directory does not look like a host error — it looks
  like the game deciding to terminate (§2b).
- `of_disc.c` resolves its desktop track directory once into a static buffer rather than carrying a
  `getenv` pointer through the spin -> pump -> read call chain.

## 8. Storage: what is now checked rather than assumed

`scripts/pocket/repack.py` cuts the disc (`NIGHT.RES` verbatim plus one raw 2352-byte-frame file per audio
track) into `scratch/pocket/`, and `scripts/pocket/check_slots.py` verifies the *mapping* the run-time host
uses — for every track, at four boundaries each (first sector, second, middle, last):

```
track02: 3153 sectors, LBA 37262..40414 byte-identical to IMG at 4 sampled boundaries
...
track15:  832 sectors, LBA 268621..269452 byte-identical to IMG at 4 sampled boundaries
[PASS] track3 - track2 = 3153 (0xc51), the value fn_010c3e compares
[PASS] an LBA inside the lead-in belongs to no audio slot
== ALL PASS ==
```

i.e. offset-in-slot = `disc_lba - track.lba` (no −150, because a `.bin` begins at that track's INDEX 01)
lands on the same bytes `cd_iso.c` reaches with `disc_lba - 150` inside the single IMG.  Audio total:
546,113,232 bytes.

`scripts/pocket/deploy.sh` is the piece `image.sh` cannot be: it copies the core tree *and* the slot files
into `Assets/night/common/` — the folder `image.sh` already fills with `night.elf`, `os.bin` and
`bank.ofsf` — checking the Pocket's 23-character name limit per file (`track02.bin` = 11) and the free
space before writing anything.  A dry run against a plain directory lands 15 slot files (NIGHT.RES + 14
tracks, 575 MB) beside the core tree.

## 9. Still open after this step

- CDDA is consumed but not sounded: the track advances on the disc's 75 sectors/s clock (exactly the
  `src/app/main.c` no-device behaviour, and it is what keeps the refresh watchdogs honest) — task-5.
- Saves/config slots exist in `data.json` but nothing writes them — task-7 needs a decision, since the
  shipped game has no save file at all (`sys_file_write` is a write-to-the-void stand-in).
- The pad map is declared (A=0x1d punch, B=0x38 kick, X=0x2a jump via the aliased modifier slots,
  docs/91/92) but not yet confirmed against the live game — task-6.
- Whole-game reachability on the shim, then on hardware — tasks 8..11.
