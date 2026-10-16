# 09 · Music (per-intro / per-stage) & Title screen

Port work for two of the four remaining stage systems: the **music selection**
(CD-DA per-episode tracks + boss) and the **title screen** (bounded scroll +
restored "START GAME / EXIT TO DOS" text). Each is grounded in the Ghidra
reverse-engineering in `docs/06-cdda.md` and verified with headless `--log` +
screenshots in `scratch/shots/`.

---

## 1. Music

### 1.1 Ghidra reverse-engineering

- **Per-episode track** — `FUN_0003a829` (the stage frame) computes
  `music = *(RAM 0x52da4 + epData[0x40]*4 + (episode-1)*0x28)` (decompiled via
  `FUN_00010b1d` / `FUN_00010d47`; the 1..9 episode counter `DAT_00063370` is
  set at 0x3b578 in `FUN_0003af18`). The table lives at **file offset 0x4b7a4**
  in `NIGHT.EXE` (RAM 0x52da4): **9 rows × 10 wave-slots of u32 tracks**.
- **Fixed tracks** (`docs/06-cdda.md` §2, `FUN_0003af18` main + the play
  functions `FUN_00038f3d`/`FUN_00038445`):

  | game track | CD track | role |
  |---|---|---|
  | 1 | 2 | opening |
  | 9 | 10 | menu |
  | 10 | 11 | stage (generic) |
  | 11 | 12 | combat |
  | 12 | 13 | **boss** |

### 1.2 The music table (read from `NIGHT.EXE` file offset 0x4b7a4)

Each row is one episode; the **dominant** value is the base track that plays for
the intro + stage, and a single wave-slot in most rows overrides to **track 8**
(a transient the position-based port does not model):

| ep | base track | CD | wave→8 override |
|---|---|---|---|
| 1 | **2** | 3 | wave 3 |
| 2 | **3** | 4 | wave 3 |
| 3 | **9** | 10 | wave 6 |
| 4 | **4** | 5 | wave 1 |
| 5 | **5** | 6 | — |
| 6 | **6** | 7 | wave 3 |
| 7 | **7** | 8 | wave 2 |
| 8 | **8** | 9 | — |
| 9 | **2** | 3 | wave 7 |

### 1.3 Port implementation (`stage.c` `episode_music_track` / `stage_music_track`)

`episode_music_track` returns the base track above (`{2,3,9,4,5,6,7,8,2}` by
episode). `stage_music_track`:
- **Intro (slides) + stage** play the per-episode base track.
- **Boss** (`phase==1 && boss_active`) takes over track **12**.
- **Menu** = 9, **opening** = 1 (wired in `game.c`).

### 1.4 Verification (`SDL_VIDEODRIVER=offscreen` + `--log`)

| ep | `--log` (intro+stage) | WAV played |
|---|---|---|
| 1 | `intro+stage music cd track 2` | `scratch/cdda/track03.wav` |
| 2 | `… cd track 3` | `track04.wav` |
| 3 | `… cd track 9` | `track10.wav` |
| 4 | `… cd track 4` | `track05.wav` |
| 5 | `… cd track 5` | `track06.wav` |
| 6 | `… cd track 6` | `track07.wav` |
| 7 | `… cd track 7` | `track08.wav` |
| 8 | `… cd track 8` | `track09.wav` |
| 9 | `… cd track 2` | `track03.wav` |

Boss transition (ep1 demo): `[f01454] music: cd track 12 (track13.wav …)` then
`music -> game track 2` (back to ep1's base). Menu: `[menu] music: cd track 9
(track10.wav …)`. Every value matches the table at file 0x4b7a4.

---

## 2. Title screen

### 2.1 What the original does

- `title1.pcx` (640×480) from `NIGHT.RES` is the **static full-screen title
  image** (dusk skyline + the Korean title + "1997 T.G.ENTERTAINMENT /
  ORIGINAL COMIC BY MYUNG JIN LEE"). It does **not** scroll infinitely.
- The menu items **"START GAME" / "EXIT TO DOS"** are drawn by the game **over**
  the title (a text overlay) — they are *not* baked into `title1.pcx`
  (confirmed: rendering `title1.pcx` alone shows no menu text).

### 2.2 Port fixes (`game.c` `present_menu`, `to_menu`)

1. **Bounded settle** — `to_menu` sets `menu_t = 100`; the menu state decrements
   `menu_t -= 2` per frame until it clamps at `0`. `present_menu` blits
   `title1.pcx` at y-offset `-menu_t` and clips rows outside the window to
   black, so the title settles into a **bounded, non-infinite** scroll.
2. **Restored text** — after blitting the title, `draw_text` (the 5×7 font from
   `stage.c`, exposed via `stage.h`) draws:
   - `START GAME` at (310, 230), scale 3, cream `0xFFFFE8C0`;
   - `EXIT TO DOS` at (310, 274), scale 3, dim `0xFFB0A090`.

### 2.3 Verification

- Screenshot `scratch/shots/menu_title.png` (headless menu at frame 120, after
  the settle) shows the full static title with **START GAME / EXIT TO DOS**
  present in the mid-right area — matching the original layout.
- `--log`: `[menu] enter menu (keys: 1=exit 3=replay else=play)` +
  `[menu] music: cd track 9 (track10.wav …)`.

---

## 3. Build / selftest

`make` is clean (0 warnings) and `./night --selftest` = ALL PASS after these
changes.
