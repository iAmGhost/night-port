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
| `ST_STAGE1` | `FUN_00037b03` | full stage engine (`stage.c`): pre-stage slides, waves, combat, boss, clear/next-episode, game over | CD tracks 10/11/12 = `track11/12/13.wav` (stage/combat/boss, switched live) |

Menu keys (matches original `FUN_0003a224`): `1` = quit, `3` = replay
opening, any other key = play (starts the full game at episode 1).

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

## Full stage engine (`stage.c` + `stage.h`)

The vertical-slice stage 1 was extended into a complete playable beat-em-up
covering all 9 episodes. Everything data-driven comes from `NIGHT.RES`;
combat tuning is a documented playable approximation (the original's
authoritative combat lives in `FUN_0003a829`, `docs/05-logic.md` §4).

- **Flow** (per episode 1..9): pre-stage slides (`%ds%d.pcx`, 60 frames each,
  any key skips; e.g. ep1 has 3, ep3 has 6) → wave combat → boss →
  `STAGE CLEAR` (150 frames) → next episode. After episode 9:
  `THANK YOU FOR PLAYING NIGHT` (the binary's actual game-over string) →
  menu. Player death: `GAME OVER` (150 frames) → menu. `ESC` returns to menu
  any time.
- **Enemy placement (position-based)** (`.stg` tag-1): every tag-1 spawn is a
  placed foe at its authored world `(x, y)` = `(payload[2]/2, payload[3]/2)`,
  sorted left→right (`place_enemies`). No pre-spawned wave clumps: each foe
  wakes (`activate_enemies`) the moment the player comes within `AHEAD_ACT`
  (240 world px) to its left, then its AI carries it toward the player — the
  classic beat-em-up "walk in as you advance" rhythm. This replaces the
  earlier 4-band wave/queue model. The stage clears once the player walks past
  the rightmost foe (`level_end_x`), which handles every episode (ep5's boss
  sits at the *start*; the rest put the boss near the end). Sentinel tag-1
  records (`payload[2] >= 0x00FFFFFF`, seen in ep6/ep7) are skipped.
  `MAX_ENTS` is 200 (ep9 places 132 foes). **Verified**: for all 9 stages the
  port's placed count + x-span equal an independent Python tag-1 parse (e.g.
  ep1 38 foes x 67..2070, ep9 132 foes x 45..6132); ep1 per-enemy spawn log
  matches tag-1 (x exact, y ±1). Evidence: `scratch/shots/ep1_subway.png`,
  `ep1_s1street.png` (BOSS indicator).
- **Boss**: `boss_id_for(episode)` = id 1..9 (boss1..boss7 sprites), itself a
  placed tag-1 foe (stage 1 boss1 at (2030, 184)). It wakes like any other
  foe but with boss HP/speed/hitbox, and lights the `BOSS` HUD indicator +
  CD track 12 while it is in the fray. Mini-bosses (ids 11..17, mboss*) are
  placed at their own tag-1 spots with normal-foe HP.
- **Combat model** (tuning constants at the top of `stage.c`): player
  150 HP; punch 14 dmg / 70 range / 12-frame anim; kick 22 dmg / 95 range /
  16-frame anim; enemies 30 HP (5 dmg attacks), bosses 130 HP (12 dmg);
  depth-lane hit rule (`|dy| < 30` world px, i.e. beat-em-up front/back
  lanes); 55-frame player i-frames; kill rewards +20 EXP (+100 boss) into
  the 근성치 (jinseong) bar — full bar grants +25% damage for the stage.
- **Enemy AI**: walk toward the player (x + slow y lane-matching), face the
  player, attack in range with per-enemy cooldowns (55f normal / 80f boss),
  hitstun + knockback + white flash when damaged, flicker-out death.
- **Rendering**: `stage%d.pcx` background (raw 8-bit palette), depth-sorted
  (painter's) sprite pass, 2× nearest-neighbour, feet-anchored at
  `80 + y*2` on a 1600×200 world rendered whole into 640×480 (no camera
  scroll). HUD: `STAGE n`, HP bar, EXP/근성 bar, `FOE n` / `BOSS`.
- **Input** (README): arrows = move (left/right + up/down depth),
  SHIFT = jump (reserved), CTRL/Z = punch, ALT/X = kick, SPACE = crouch
  (reserved), ESC = menu.
- **`--demo`**: deterministic chase-the-nearest-enemy bot (faster than the
  player for headless evidence). Without it the stage is fully human-playable.
- **SFX / music**: punch/kick/hit fire `1.voc` (22050 Hz mono, own audio
  device); music switches live 10 (pre-stage/clear) → 11 (waves) → 12
  (boss), logged in `--log`.
- **Evidence** (`scratch/shots/full/`, from one deterministic 7200-frame
  `--demo` run, log in `scratch/shots/game_demo.log`):
  `01_wave_combat.png` (wave 1 vs 4-enemy cap), `02_boss_fight.png`
  (BOSS indicator + track 12), `03_stage_clear.png` (STAGE CLEAR banner),
  `04_stage2_start.png` (EPISODE-2 background, wave 1), `05_game_over.png`
  (GAME OVER in stage 2 wave 4). Log shows: slides, waves 1-4, jinseong
  full (+25%), `BOSS boss1.spr`, `boss defeated`, `STAGE 1 CLEAR`,
  stage 2 open (44-enemy wave 1 queue), `player dead -> GAME OVER`,
  `game over -> menu`.

## Remaining (full game)

- `.wal` floor rendering (the 320000-tile root grid + node palette) —
  loaded/parsed but not drawn (the background PCX carries the street
  artwork).
- Walkmap collision from the `.wal` grid (movement currently unconstrained
  inside the 1600×200 band), camera/VESA-window scrolling for worlds wider
  than the screen.
- `.key` technique-matching combo combat (the original's 60+ techniques),
  dedicated attack sprites per technique (the port reuses `.act` record 1),
  per-event VOC SFX table (the port uses `1.voc` for all hit events),
  crouch/jump mechanics, EXP level-ups and the original EXP/근성치 formulas
  (`exptab.dat`), episode select, pause screen (`pause.spr`).
- Original combat constants (damage, AI, hitboxes) from `FUN_0003a829` —
  the port's values are a playable approximation, not measured from the
  binary.

## Remaining (full game)

- Pre-stage `%ds%d.pcx` slide sequence before the stage background.
- `.wal` floor rendering (the 320000-tile root grid + node palette) —
  currently loaded/parsed but not drawn (the background PCX carries the
  street artwork for the slice).
- Walkmap collision (grid tile walkability), camera/window scrolling,
  `.key` technique-matching combat, EXP/근성치 HUD, CD track 11/12
  combat/boss switches, enemy AI.

## Status update (2026-02-06, task-7/8 gameplay parity pass)

The "Remaining (full game)" lists above are partially stale. Implemented since:
`.wal` floor rendering + camera scroll (docs/07), full 9-episode playable
game (this file, "Full stage engine"), full animation set (docs/08), and the
measured-mechanics pass documented in `docs/10-gameplay.md` §17 (per-type
HP/armor, damage-dealt EXP + exptab thresholds + 6-digit EXP readout,
max-active-3 spawn cap, tag-6 exit-zone / kill-clear stage clearing,
dead-zone camera follow, speed oscillation, enemy panel + boss bar HUD,
starburst hit FX). Remaining: exact 6-slot hitbox combat, full 101-type AI
state machines, `.key` combo matcher.
