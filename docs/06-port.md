# 06 — Port: Architecture, Build & Run

SDL3 + C vertical-slice port of *어쩐지 좋은일이 생길것 같은 저녁* (T.G
Entertainment, 1997). Source of truth for assets: `NIGHT/NIGHT.RES`
(container spec: `docs/02-container.md`). CD-DA music: 14 tracks extracted
to WAV by `scripts/cdda2wav.py` (see `docs/06-cdda.md`).

## Layout

```
port/
  Makefile            # SDL3-only dependency (pkg-config or Homebrew prefix)
  src/
    main.c            # entry; --selftest dispatch vs game_run()
    game.c            # state machine + CLI + audio + headless rendering
    res_container.c   # NIGHT.RES container (backward-XOR directory)
    res_pcx.c         # .pcx decode (modified RLE + raw 768B RGB palette, see docs/03 §1)
    res_spr.c         # .spr decode + per-row blit
    fli.c             # opening.fli decode (0x0C patch / 0x0F full-frame)
    voc.c             # .voc decode (Creative Voice tag stream)
    wav.c             # RIFF PCM 16-bit reader
    data.c/.h         # .act / .stg / .wal / .key / .chr / exptab.dat
    png_out.c         # minimal PNG writer (stored-deflate, no zlib)
    selftest.c        # byte-exact verification suite
```

## Build

```sh
cd port
make                 # -> ./night
```

Requirements: C99 compiler, SDL3 (Homebrew: `brew install sdl3`). No other
dependencies.

## Run

```sh
./night                          # full flow: opening (1168f @30fps) -> menu -> stage-1 stub
./night --state opening|menu|stage1
./night --frames N               # stop after N frames (headless capture)
./night --shot PATH              # capture one screenshot
./night --shot-every N PREFIX    # capture every N frames as PREFIX_f%05d_STATE.png
./night --log PATH               # append state/music/shot events
./night --res PATH --cdda DIR    # override asset locations (defaults resolve relative)
./night --selftest               # byte-exact verification suite (no SDL device needed)
```

Headless capture (CI / evidence):

```sh
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ./night --frames 1300 \
    --shot-every 300 scratch/shots/flow --log scratch/shots/flow.log
```

Window/renderer creation is best-effort: if the display driver cannot create
a GL context (offscreen driver, no GUI), the port drops into **headless mode**
and screenshots are captured straight from the frame buffer via
`SDL_CreateSurfaceFrom`.

## FLI approach decision

Decode **in C** (`fli.c`), no pipeline pre-convert: `opening.fli` turned out to
be a custom patch-based container, not Cinepak (spec + byte-exact verification
in `docs/03-formats.md` §11). The C decoder consumes all 1169 chunks to exact
EOF and matches an independent Python reference per pixel.

## Game states (`game.c`)

| state | source (binary) | rendering | music |
|-------|-----------------|-----------|-------|
| `ST_OPENING` | `FUN_00038ae0` | `opening.fli` 320×200, 2× nearest → 640×400 | CD track 1 = `track02.wav` |
| `ST_MENU` | `FUN_0003a224` | `title1.pcx` vertical scroll | CD track 9 = `track10.wav` |
| `ST_STAGE1` | `FUN_00037b03` | stub text (port-stage1 task) | CD track 10 = `track11.wav` (pending) |

Menu keys (matches original `FUN_0003a224`): `1` = quit, `3` = replay
opening, any other key = play (stage 1).

## Verified against the original

- `--selftest`: container (444/444), PCX render, 100/100 VOC, `.key/.stg/
  .wal/.act/.chr` parse checks, FLI setup+1168-chunk exact-EOF + palette and
  frame-39 pixel diff vs `scratch/shots/ref_f39.idx`, WAV header checks.
- C FLI frame 39 == independent Python reference (byte-for-byte).
- Screenshots in `scratch/shots/` (`flow_*`, `opening_*`, `menu_*`).

## Known port deviations / notes

- `SDL_Init` in SDL 3.4.16 (Homebrew) returns `bool` (true = success); the
  port checks `!SDL_Init(...)`.
- Audio streams use the SDL 3.4 `SDL_BindAudioStream` API (3.2-style
  `SDL_AddAudioStream` was renamed).
- Music is streamed from extracted WAV files (track k = file `track%02d.wav`
  of k+1, CD numbering); the original drives the CD-DA track directly.
- Opening is rendered at the original 320×200 then 2× upscaled; the original
  game does the same offscreen→VESA compositing (see
  `docs/05-logic.md` § compositing).

## Stage 1 (`stage1.c` + `stage1.h`)

Implemented (see `docs/05-logic.md` §3, `docs/03-formats.md` §4-5):

- **Background**: `stage1.pcx` (640×480) drawn full-screen, raw 8-bit palette
  (= display DAC bytes). Episode 1's pre-stage slides (`%ds%d.pcx`) are a
  cutscene step not included in the slice.
- **Entities**: `stage1.stg` tag-1 records → entity id → name tables dumped
  from the EXE (VA `0x51d88` .spr / `0x52198` .act, 119 entries; in
  `SPR_NAMES[]` in `stage1.c`). tag1 payload: `p[0]`=entity id,
  `p[1]`=flag, `p[2]`=x half-pixels, `p[3]`=y half-pixels. 10 unique entities
  spawn in stage 1 (gun, boss1, mboss1-1, ena-1, enb-1, enc-1, end-1, ...).
- **Player**: entity 0 (`gun.spr`/`gun.act`, 266 frames / 192 act records),
  spawned at the first tag-5 anchor (1656.5, 199.5).
- **Animation**: each entity loops `.act` record 0 (idle) at 1 beat per
  3 frames; punch plays record 1. (Record semantics beyond idle/attack are
  not yet mapped to the binary's technique system — slice scope.)
- **Screen mapping** (assumption, documented): the 320×200 offscreen game
  space is 2× upscaled to 640×480; the 200-px walk band maps to screen rows
  80..480; world x scales 640/2048 across the screen (no camera scroll in
  the slice; the original scrolls the VESA window per `docs/05-logic.md`
  §10.6). Sprites are 2× nearest-neighbor scaled, feet-anchored.
- **Input**: arrows=move (2.2 px/frame), SHIFT=jump (placeholder),
  CTRL/Z=punch, ALT/X=kick — per README. `--demo` (default when
  `--state stage1`) runs a deterministic input script for headless evidence.
- **SFX**: punch/kick fire `1.voc` (VOCs are event-numbered 1..100; the
  full event→voc table is in the binary, `docs/04-sound.md` — out of slice).
  Played 22050 Hz mono S16 via its own SDL3 audio device/stream.
- **Music**: CD track 10 (`track11.wav`) on stage entry.
- **Evidence**: `scratch/shots/stage1_f0100.png` (background + 10 animated
  sprites) and `scratch/shots/stage1_demo.log` (entity load list, punch SFX
  fire, music start).

## Remaining (full game)

- Pre-stage `%ds%d.pcx` slide sequence before the stage background.
- `.wal` floor rendering (the 320000-tile root grid + node palette) —
  currently loaded/parsed but not drawn (the background PCX carries the
  street artwork for the slice).
- Walkmap collision (grid tile walkability), camera/window scrolling,
  `.key` technique-matching combat, EXP/근성치 HUD, CD track 11/12
  combat/boss switches, enemy AI.
