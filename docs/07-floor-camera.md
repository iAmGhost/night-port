# 07 — Floor (`.wal`) rendering & camera

Goal: render the scrolling street scene from `stage%d.wal` as the gameplay
background and add horizontal camera scrolling, so the world (wider than the
screen) can be traversed. This replaces the earlier vertical-slice
approximation that used `stage%d.pcx` (the red EPISODE title card) as a static
full-screen background.

All claims below were re-verified against `NIGHT.EXE` in Ghidra (project `ppk`,
LE loader, `watcom:LE:32:default:watcom`) and against the extracted assets in
`scratch/res/`.

## 1. The `.wal` is the street scene, not a flat floor

`stage%d.wal` holds **pre-rendered 8bpp street scenes** (one per node), each with
its own 256-colour palette. Rendering every node of `stage1.wal` to PNG
(evidence: `scratch/shots/wal_node{0..3}_*.png`) shows:

| node | a | b   | grid    | content                                   |
|------|---|-----|---------|-------------------------------------------|
| 0    | 0 | 0   | 1600×200| **subway platform** (tracks, graffiti, road) — main street |
| 1    | 1 | 0   | 320×200 | dark alley with shops (R2M / COCO)          |
| 2    | 0 | 1   | 1920×200| street with crosswalk + parked cars         |
| 3    | 0 | 1   | 320×200 | 320-px overlay of the b=1 variant           |

- `a` = background(0) / foreground(1) pass; `b` = variant (0/1). The nodes are
  **not** a simple stacked layer set: they are placed at distinct world x-offsets
  and overlap (see §6 for the resolved composite model). For stage 1 node 0 is the
  widest node (1600 px) but the full street is built by compositing **all** nodes.
- Grid bytes are **palette indices** into the node's 768-byte palette (common
  values 0x40–0x47 are the dark road / platform tones).
- Node layout (see `docs/03-formats.md` §4, reader `FUN_0003c853`):
  `[u8 a][u8 b][u32 p0..p5]` then `grid = (p2-p0)×200` (flag `p1&1==0`),
  then `[768B palette][u32 n][n×u16 markers]`. stage1: 4 nodes, all parse to EOF.

**Correction to earlier docs:** `docs/05-logic.md` §10.6 / `03-formats.md` called
`.wal` a "walkmap / walkable-solid map". The *grid* doubles as the walkable
collision map (1 byte per world pixel), but the same grid is the **visible
street artwork** drawn to screen. The `DAT_00053e9c` collision stride (0x88,
0x65 rows) is a separate derived walkability table; the visible floor comes from
the node grid + palette.

## 2. The other stage images are NOT the gameplay background

Rendering the PCXs (evidence `scratch/shots/cmp_*.png`) shows:

- `stage%d.pcx` (640×480) = the **EPISODE-N title card** (e.g. "EPISODE-1
  쏘가리…" over a portrait). Shown once at stage start, not during play.
- `%ds%d.pcx` (e.g. `1s1.pcx`, 640×480) = **story cutscene** comic panels with
  Korean narration text. Shown as pre-stage slides (already implemented).

So the earlier port's use of `stage1.pcx` as the always-on background was
wrong: it is an intro card. The always-on gameplay background is the `.wal`
street (node 0).

## 3. Game-world buffer & camera (verified)

- **Offscreen game buffer = 320 × 200** (stride `0x140` = 320). Verified in the
  sprite blit `FUN_0003bb42`:
  - y is clamped by `param_3 < 0xc9` (201) and rows are clamped by
    `200 < param_3 + count` → max 200 rows.
  - row advance is `+0x140` (320). So the drawable region is exactly 320×200.
- **2× nearest-neighbour upscale** → **640 × 400** (320→640, 200→400).
- **Camera** = a 320-px-wide × 200-px-tall window into the wider world
  (2920 px for stage 1; see §6 for the per-stage widths).
  - `DAT_000639dc` = camera-left x (half-res world units).
  - Camera width = `0x140` = 320.
  - `FUN_0003d4e4` (position triggers) compares `(DAT_00053c7c+1) − DAT_000639dc`
    against thresholds (0xbe, 0x82, 0xc2) — i.e. entity-position relative to the
    camera-left, confirming `DAT_000639dc` is the camera origin.
  - `FUN_0003a829` (stage frame) advances the scroll position
    `DAT_00053c7c+1` by **±0x78 (120 px)** per scripted step — the slide/camera
    advance quantum.
- **World content is drawn into the 320×200 buffer at `(world − camera)`**,
  then the whole buffer is 2× upscaled to the screen. Camera scrolling is done
  this way, **not** via the VESA window.

## 4. The VESA window is NOT the camera

- `FUN_0003f3be` = VESA `int 0x10 AH=0x4F05` (set window start). It pages the
  64 KB A0000 window through the 307200-byte (640×480) linear framebuffer
  (`DAT_000a0000`), using window table `DAT_00063e50[]`, page `DAT_00063e5c`,
  shift `DAT_00063e4c`. Callers: `FUN_0003628e` (PCX loader), `FUN_0003f462`
  (present), `FUN_0003f645` (menu init).
- `FUN_0003f462` (present/flip of a 640×480 region) is called **only** by
  `FUN_0003a224` (menu/opening). The stage presents via its own draw path.
- Conclusion: the VESA window exists to write a >64 KB framebuffer through a
  64 KB A0000 alias; it is not how the camera moves. (For the port we render
  straight into a 640×480 ARGB buffer, so this is a non-issue — no banking.)

## 5. Compositing onto the 640×480 screen (best-effort, documented)

The 320×200 camera view becomes 640×400 at 2×. The screen is 640×480, leaving
80 px. The original's exact vertical placement of the 640×400 block could not be
observed directly (the original stage cannot be run here — it needs the CD +
the missing `HCODE.EXE` copy-protection wrapper, see `docs/06-port.md`).
Best-effort decision for the port:

- Draw the 2× street+sprite view to **screen rows 80..480** (bottom 400 px).
- Fill the **top 80 px (rows 0..80) with a dark header** — the HUD
  (STAGE / HP / EXP / WAVE) sits here, and the street's top (dark platform)
  meets it with no visible seam.
- Vertical world mapping: world row `wy` (0..199) → screen row `80 + 2*wy`.
  The walkable road (lower rows of the street) lands at the bottom of the
  screen, far background (platform) at the top of the street block — the natural
  beat-'em-up "far top / near bottom" ordering.

## 6. Node compositing model & camera (resolved, port)

The street is built by **compositing all nodes into one `world_w × 200` buffer**,
then windowing that buffer with the camera. Model (verified by rendering each
node and the composite to PNG and matching the on-screen result):

- **x-offset = `p0 / 2`** — the `p0..p5` fields are in **half-pixels**; a node's
  top-left world column is `p0/2`.
- **1 world px per grid cell** — a node of grid width `gw/200` spans
  `[p0/2, p0/2 + gw/200)` world columns.
- **draw order: `b=0` nodes first, `b=1` nodes on top** (background pass then
  foreground pass; within a pass, file order).
- **palette index 0 = transparent** — where a node's grid is 0 the layer behind
  (or the dark ground default) shows through. Non-zero indices are opaque.
- **`world_w` = max over nodes of `(p0/2 + grid_width)`** (the right edge of the
  rightmost node).
- Cells no node covers → dark ground fill.

The composite is stored once as an ARGB buffer (`world_w × 200`); the camera
blit copies a 320-px-wide × 200-px-tall window of it (2× upscaled) to screen
rows 80..479. Camera follow:

- Camera shows 320 world-px (640 screen-px). `camx = clamp(player_wx − 160, 0,
  world_w − 320)` (player centred, clamped to the world).
- Street pixel `(wx,wy)` → screen `(2*(wx−camx), 80 + 2*wy)`, clipped.
- Sprites/entities use the same `(world − camx)` transform, 2× upscaled.

### Per-stage floor (verified: `--state stage1 --episode N --frames 1`)

| ep | .stg recs | .wal nodes | world_w (px) |
|----|-----------|------------|--------------|
| 1  | 88        | 4          | 2920 |
| 2  | 184       | 4          | 4600 |
| 3  | 128       | 7          | 3640 |
| 4  | 42        | 2          | 1600 |
| 5  | 28        | 1          | 1600 |
| 6  | 203       | 4          | 4560 |
| 7  | 197       | 3          | 5060 |
| 8  | 2         | 1          | 1600 |
| 9  | 207       | 8          | 6600 |

Each stage has a distinct street (subway, graffiti alley, building facade, …),
all rendering correctly from the same composite code path.

### Bug fixed: dangling `.wal` grid pointer

The first port of this feature rendered a **garbage/dark street** (grid looked
all-noise; the palette, by contrast, was correct). Root cause: `wal_decode()`
set `node->grid = d + off` (a pointer **into** the caller's input buffer) while
`node->pal` was `memcpy`'d (owned). `stage_open()` did
`wal_decode(d,…); free(d);`, leaving `node->grid` (and `node->marks`) dangling —
`build_floor()` then read freed heap. The palette survived (it was copied); the
grid did not. Fix: `wal_decode()` now `malloc`+`memcpy`s both `grid` and `marks`
(owned by the `Wal`), and `wal_free()` releases them. After the fix the C grid
bytes match the raw `stage%d.wal` extraction byte-for-byte.

> Note: the selftest did **not** catch this because it calls
> `wal_decode(res_blob(…), …)` on a still-live buffer and checks parse-to-EOF,
> so the dangling pointer was never dereferenced there. A byte-compare of the
> rendered grid against a fresh extraction is the reliable check.

## 7. Palette expansion, player & enemy fixes (reference match)

Reference target: `scratch/stage1-screenshot.png` (original ep1 in DOSBox) —
a bright **subway** (train, DANGER railing, red platform edge, grey tiles)
with the player (건, navy jacket / white shirt / jeans) and **shirtless / red-jacket**
foes. Matching it surfaced three port bugs:

### 7.1 The `.wal` / sprite palette is 6-bit, not 8-bit

The 256-entry node palette (768 bytes) stores **native 6-bit DAC values
(0..63 per channel)** — the hardware expands each to 8-bit by `<<2` (×4) when
loading the DAC. Reading the bytes as 8-bit (no expansion) collapses the image
to the bottom quarter of the range → an almost-black scene. Fix: every read of
`node->pal` (floor blit *and* `draw_sprite`) applies `<<2`:
`0xFF000000 | (r<<2)<<16 | (g<<2)<<8 | (b<<2)`. This made the subway bright and
the characters full-colour.

### 7.2 Player spawn is the left edge of the enemy field, not a tag5 anchor

tag5 records are **camera/anchor waypoints** in the *later* part of the world
(ep1: x = 1031..1765, all in the street half). The player actually starts at
the **left edge of the first fight** — the subway. Wave-1/tag1 enemies in ep1
span x = 50..763 (subway), so the spawn is `min(tag1 enemy x) − 30` (clamped ≥
20), y = 185. This is what makes the opening camera show the subway, like the
reference.

### 7.3 Enemies come from tag1 (entity spawns), not tag2

`.stg` tag1 = entity spawns (`p[0]` = entity id, `p[2]/2` = x, `p[3]/2` = y);
tag2 (all `p[0]=0`) are triggers/FX, **not** spawns. The old port read tag2 and
fell back to `gun` (id 0), so every foe looked like the player. Now `start_wave`
queues **tag1** spawns (id ≠ 0, ≠ boss) in the wave's x-band (world split into 4
left→right bands, matching the player's rightward advance); the boss (tag1 id =
`boss_id_for(ep)`) spawns last. ep1 band-1 = subway foes (ena/enb/enc), matching
the reference's shirtless + red-jacket enemies.

### 7.4 `SPR_NAMES` off-by-one (id → sprite)

The id→`.spr` table had **14** `gun.spr` fillers (idx 18..31) where the original
name table (EXE VA `0x51d88`) has **13** (idx 18..30), shifting every foe: id 31
→ gun (should be ena-1), id 36 → gun (should be enb-1). Removed one filler so
ena-1 = idx 31, enb-1 = idx 36, etc. Verified via `--log` spawn lines
(`spawn id=36 spr=enb-1.spr`).

## 8. Evidence

Node renders (Python, `scratch/shots/wal2/`):
- `wal_node0_a0b0.png` — stage1 node0 subway street (1600×200).
- `wal_node1_a1b0.png` — node1 alley (320×200).
- `wal_node2_a0b1.png` — node2 crosswalk street (1920×200).
- `wal_node3_a0b1.png` — node3 overlay (320×200).
- `wal2/composite_world.png` — all 4 nodes composited (2920×200), the target
  street the port must reproduce.

Port renders (headless `SDL_VIDEODRIVER=offscreen`, 640×480):
- `floor_f00560_stage1.png` / `floor_f01120_stage1.png` — stage1 subway street
  with camera scroll + fighters.
- `s2_f01120_stage1.png` — stage2 graffiti-alley street (4600 px).
- `s9_f01120_stage1.png` — stage9 building-facade street (6600 px, 8 nodes).

Reference-match renders (headless, 640×480):
- `sub1_f0420.png` / `fixed_enemies.png` — ep1 subway + gun (navy) + enb-1
  (red jacket / pink hair), matching `stage1-screenshot.png`.
- `ep2_verify.png` / `ep9_verify.png` — ep2 outdoor (trees/scaffold) and ep9
  nighttime road, all full-colour.
- `chars_480.png` — gun | ena-1 (shirtless) | enb-1 (red), node0 palette `<<2`.

Intro cards (NOT the gameplay background):
- `cmp_stage1.pcx.png` — EPISODE-1 title card (640×480).
- `cmp_1s1.pcx.png` — story cutscene panel (640×480).

## 9. Function references (Ghidra)

| role | address |
|------|---------|
| stage frame (camera ±120 step, palette→DAC, draw dispatch) | `FUN_0003a829` |
| sprite blit into 320×200 (clip bounds, stride 0x140)         | `FUN_0003bb42` |
| position triggers vs camera-left (`DAT_000639dc`)           | `FUN_0003d4e4` |
| `.wal` node reader (grid + palette + markers)               | `FUN_0003c853` |
| VESA set-window (`int 0x10 AH=4F05`) — framebuffer paging   | `FUN_0003f3be` |
| 640×480 present (menu only)                                 | `FUN_0003f462` |

Globals: `DAT_000639dc` camera-left x; `DAT_00053c7c+1` scroll/entity x;
`DAT_00053c7c+9` second position; `DAT_00053e9c` derived walkability table;
`DAT_000633b8` global display palette.
