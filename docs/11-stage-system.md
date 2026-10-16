# 11 — Stage system: sub-stage chain, drip spawn, ground Y, camera clamps

Deep Ghidra analysis of the NIGHT.EXE stage runtime, cross-checked against the
decoded `.wal`/`.stg` data for all 9 episodes. This document is the basis for
the SDL-port fixes (belt-scroll behavior, grounded actors, clean sub-stage ends).

Addresses refer to the Ghidra program `/NIGHT.EXE`. Data globals:

| Global | Meaning |
|---|---|
| `DAT_00063354` | current segment pointer (`S`) |
| `DAT_00063350` | first segment of the episode (chain base) |
| `DAT_000639dc` | camX (camera-left world X) |
| `DAT_000639e0` | camY (camera-top world Y) |
| `DAT_000639e4` | Y-ramp "ramping" flag (0/1) |
| `DAT_00053c7c` | player entity pointer (`P`); `P+1`=X, `P+9`=Y, `P+0x17`=action |
| `DAT_00053c80` | boss/"objective" entity pointer (kill → stage clear) |
| `DAT_00053c88` | secondary boss pointer (cleared on death) |
| `DAT_00053c68` | active entity count (1 = player only) |
| `DAT_0006336c` | stage state flag: 1=player dead, 2=exit-zone sub-stage advance, 3=stage clear (boss), 4=kill-clear sub-stage advance |
| `DAT_000639bc` | kill counter (incremented per killed `*ent==1` entity) |
| `DAT_00063370` | current episode (1..9) |
| `DAT_00063398` | entry direction for next sub-stage (from next seg flag byte) |
| `DAT_00063360` | frame counter |
| `0x5d0a0..0x5d0b1` | tag-6 exit-zone rect (u32 x, y, w?, h?) + flags — single global, set by tag-6 drain |
| `0x528b4` | per-(ep,tech-set) max-active cap table, u32[9][10] |
| `0x52a1c` | per-(ep,tech-set) kill-clear threshold table, u32[9][10] |
| `0x52c3c` | per-(ep,tech-set) player-Y table, u32[9][10] |
| `0x52da4` | per-(ep,tech-set) player-sprite-set table, u32[9][10] |
| `0x52be0` | per-(ep,tech-set) walk-in flag, byte[9][10] |

## 1. Model overview

An **episode is a chain of sub-stages**, one per `.wal` segment. File order is
chain order (the loader `FUN_0003c6ec`/`FUN_0003c7d7`/`FUN_0003c853` reads
segments sequentially and links `S->next` recursively; no explicit next-pointer
in the file — verified by the port's strict-size `.wal` decoder fitting all 9
files byte-exactly).

- Each sub-stage is a belt-scroll arena: the camera follows the player with a
  dead-zone, clamped to the segment rect and tag-2 scroll gates.
- Enemies **drip in** from off-screen as the camera crosses their trigger X
  (tag-1 records), not at authored positions.
- Sub-stage → next sub-stage happens on a **tag-6 exit zone** (state 2) or a
  **kill-clear counter** (state 4), both via `FUN_0003d244`: intermission /
  upgrade screen (`FUN_00038f3d(1)`), segment switch, camera + player re-init,
  wave reset (remaining `.stg` queue re-drained by `FUN_0003ed6f`), music +
  sprite reload.
- The **last sub-stage** is cleared by killing the **boss** (entity type ==
  episode number; sets `DAT_00053c80`; death → state 3 → stage clear →
  level-up check `FUN_00038445` → next episode).
- Player death → state 1 → retry.
- Segments in a **different Y band** (ep4, ep6, ep7, ep9) give Y-axis camera
  scroll via the `FUN_0003cc6b` ramp (§5.3).

## 2. Segment layout

### 2.1 File layout (verified by byte-level decode, all 9 episodes)

```
.wal = [u32 segCount] + segCount × segment
segment =
  u8   a            (mem +4 transient)
  u8   b            (mem +0x11 — "flag": entry-from-right / variant-spawn gate)
  u32  p0 left
  u32  p1 top
  u32  p2 right
  u32  p3 bottom
  u32  p4           (f4: walkline row within band)
  u32  p5           (f5: floor row within band)
  grid  (p1&1) ? (p3-p1)*320 : (p2-p0)*200 bytes   (background tile index)
  pal   768 bytes (256×3)
  u32  nmark + nmark×2 bytes (mark data)
```

### 2.2 In-memory layout (loader `FUN_0003c853`)

```
+0x00 u32 left      +0x04 u32 top      +0x08 u32 right
+0x0c u32 bottom    +0x11 u8 flag(b)   +0x12 u32 f4 walkline
+0x16 u32 f5 floor
+0x1e..             3×0x100 palette (loader copies the 768B pal here)
+0x2a u32 leftScrollGateX      +0x2e u32 rightScrollGateX
+0x32 u32 leftScrollGateX2     +0x36 u32 rightScrollGateX2
+0x3a u8 leftLock              +0x3b u8 rightLock        (tag-2 writers)
+0x3c u32 nmark                +0x44 ptr mark data
+0x348 ptr bg buffer (heap, w*200 or h*320)
+0x34c u32 next segment ptr    +0x350 u32 prev segment ptr
+0x40 u32 tech-set index (never written by the loader path → 0;
        all (ep,tech-set) tables are therefore indexed at ts0 = (ep-1)*10)
```

Note: tag-1 rect check `S[0] <= X <= S[2]` = left..right. Camera init
`FUN_0003cad3`: `camX = S.left`, `camY = S.top`, walkline → `DAT_000639e4`…
(`DAT_000639e4` is reused as the ramp flag at runtime; the walkline value is
carried separately as `DAT_000639e4` at init in the original — port stores it
on the segment struct instead.)

### 2.3 Chain data (decoded from `scratch/res/stageN.wal`)

| ep | segs (left,top)-(right,bottom), f4/f5, flag b |
|---|---|
| 1 | 4: (0,0)-(1600,200) 139/195; (3280,0)-(3600,200) 115/195 a=1; (2000,200)-(3920,400) 120/195 b=1; (4000,200)-(4320,400) 148/195 b=1 |
| 2 | 4, all Y 0-200: X 0-1920, 2000-3920, 4000-5920, 6000-7600 |
| 3 | 7, all Y 0-200: X 0-960, 1000-1320 (f5=168), 1500-3740, 4000-4320 (f5=192), 4500-4820 (b=1, f5=168), 5000-5320 (f5=192), 6000-6640 |
| 4 | 2: (0,0)-(960,200); (1000,300)-(1640,500) ← Y-band drop |
| 5 | 1: (0,0)-(1280,200) b=1 |
| 6 | 4: (0,0)-(1600,200); (2000,300)-(3600,500) b=1; (4000,300)-(6560,500); (7000,300)-(7320,500) |
| 7 | 3: (0,0)-(2560,200); (3000,0)-(4920,200) f5=187; (5000,300)-(7560,500) |
| 8 | 1: (0,0)-(640,200) (boss arena) |
| 9 | 8, Y bands 0-200 → 300-500 → 0-200 → 900-1100 → 1200-1400 (multi-drop) |

Ep1 per the user: subway → street → mall (the 320-px strip seg1 sits between;
see §9 open items).

## 3. Tag-1 drip spawner (`FUN_0003ed6f` drain → `FUN_0003e20f` case 1)

`.stg` is a drained queue: each record fires **once** when the camera crosses
its trigger X (the queue is re-drained after every wave reset, but consumed
records stay consumed).

Tag-1 record fields (81-byte rec, `p[]` words):
type=`p[0]`, side=`p[1]`, X=`p[2]` (trigger), Y=`p[3]`, zone=`p[4]`,
posmode=`p[5]`.

Per-frame, for each pending record with `camX >= X` (and `S.left <= X <=
S.right`):

- **Outer gate**: `S+0x11` (flag byte `b`). `b==0` → standard path.
- **Zone 1** (`zone==1`): spawn at camera edge **without cap check**:
  - side 0 (left):  `x = camX - 0x1e` if posmode==0, else `x = X`
  - side 1 (right): `x = camX + 0x15e` if posmode==0, else `x = X`
  - Y window: `camY <= Y <= camY + 200` (record must lie in the current band).
- **Zone 0**: same placement, but gated by the cap table:
  `DAT_00053c68 (active count incl. player) < u32[0x528b4 + ts0*4 + (ep-1)*0x28]`.
- Spawn call: `FUN_00010efc(type, x, Y, ...)` — the **authored Y is used
  verbatim** as world Y; no ground snapping at spawn time (grounding happens in
  entity physics / player table, §6).

Cap table `0x528b4` (ts0 column, all episodes = 9 except noted): ep1-9 all **9**
at ts0. (Non-ts0 entries: ep1 ts3=5, ep2 ts3=5, ep3 ts6=5, ep4 ts1=5, ep6 ts3=5,
ep7 ts2=5, ep9 ts7=5 — dead data since ts is always 0.)

Sentinels: X >= 0x00FFFFFF (ep6/ep7) → skipped (docs/10 §11.3). Boss: the
type==ep tag-1 record also sets `DAT_00053c80` (`FUN_00010efc`: `if (type ==
DAT_00063370) DAT_00053c80 = ent`).

## 4. Camera: init, gates, scroll

### 4.1 Init (`FUN_0003cad3`)

`camX = S.left`, `camY = S.top`. On sub-stage entry via `FUN_0003d244` the
player is placed at `camX - 0x1e` (flag 0, enter from left) or
`camX + 0x15e` (flag 1, enter from right), minus/plus a 0x78 walk-in offset
when the walk-in table byte is set; `P.y = u32[0x52c3c + ts0*4 + (ep-1)*0x28]`
(= world Y, see §6).

### 4.2 Scroll gates (`FUN_0003d4e4`, called every frame)

```
P.x = player X;  S = current segment;
scroll RIGHT if  S+0x3b == 0  &&  P.x - camX > 0xbe
                 && (S+0x2e == 0 || camX + 0x140 <= S+0x2e)
                 && (S+0x36 == 0 || camX + 0x140 <= S+0x36 || DAT_00053c68 == 1)
    → FUN_0003cb38(S)   (camX += speed)
scroll LEFT  if  S+0x3a == 0  &&  P.x - camX < 0x82
                 && (S+0x2a == 0 || S+0x2a <= camX)
                 && (S+0x32 == 0 || S+0x32 <= camX || DAT_00053c68 == 1)
    → FUN_0003cbd9(S)   (camX -= speed)
Y-ramp if     camY + 0xc2 <= P.y   (player within 6px of the 200px band bottom)
    → FUN_0003cc6b(S)
```

Tag-2 records (29 in ep1) write the gate fields: they clamp how far the camera
may scroll before all foes are cleared (`DAT_00053c68 == 1` bypasses the X2
gates). Net effect: camX ∈ [max(S.left, S+0x2a), min(S.right - 0x140, S+0x2e)],
widenable when the arena is empty.

### 4.3 Y-ramp / Y-axis scroll (`FUN_0003cc6b`)

```
next = S->next (+0x34c);
if (!ramping) {
    if (camX == next.left) {          // camera parked at the next segment's left edge
        S = next;  ramping = 1;       // segment switches at ramp start
        if (camY < next.bottom - 200) camY++;
    }
} else {
    if (camY == next.top - 1) { S = next; ramping = 0; }  // commit
    if (camY < next.bottom - 200) camY++;                 // 1 px/tick ramp
}
```

Used for Y-banded segments (ep4/6/7/9): the player descends (P.y grows via
entity physics on the stairs/elevator at the segment edge), the camera ramps
down 1 px/tick, and the new band's floor scrolls in from the bottom. In-band
segments (ep1/2/3/5/8) never satisfy `camY < next.bottom-200` with a band
change, so the ramp is inert and sub-stage changes go through tag-6/d244.

## 5. Sub-stage end, stage end, intermission

### 5.1 Exit zones (tag 6, writer `FUN_0003e20f` case 6, @0003ece6)

Each tag-6 record (ep1: 2 — (1528,112,1600,198) in subway, (2002,262,2052,398)
at street's left edge) writes the **single global rect** `0x5d0a0..0x5d0b1`
when drained. The entity updater `FUN_00012616` (@00012e58) tests the player
against it; inside → `DAT_0006336c = 2`.

### 5.2 Stage state machine (`FUN_00012616` death block + `FUN_0003a829` loop)

In `FUN_00012616`, when an entity is marked for deletion (`ent[0x30]==1`):

```
if (ent == player)            DAT_0006336c = 1;   // game over / retry
else if (ent == DAT_00053c80) DAT_0006336c = 3;   // BOSS killed → stage clear
else if (ent == DAT_00053c88) DAT_00053c88 = 0;   // (no state)
if (*ent == 1) { DAT_000639bc++;                  // kill counter
    if (u32[0x52a1c + ts0*4 + (ep-1)*0x28] <= DAT_000639bc)
        DAT_0006336c = 4; }                        // kill-clear → sub-stage advance
```

In the stage main loop (`FUN_0003a829`), per frame after camera + record drain:

```
if (6336c == 1) break;                       // → post-stage (retry path)
if (6336c == 3) { 6336c = 0; return 1; }     // stage clear
if (6336c == 2) { SFX 0x50; 63398 = *(next + 0x11); FUN_0003d244(); }  // exit zone
if (6336c == 4) { 63398 = *(next + 0x11); FUN_0003d244(); }            // kill-clear
if (DAT_00053c68 == 1) { S+0x32 = 0; S+0x36 = 0; }   // empty arena → clear X2 gates
```

Post-loop (stage clear): `FUN_00038445()` level-up check → return 2 (level-up
screen shown) or 0.

### 5.3 Transition (`FUN_0003d244`)

```
next = *S->0x34c ? *S->0x34c : DAT_00063350;    // advance (wrap to base)
save player cursor; load palettes;
S = next;
FUN_00038f3d(1);        // intermission / upgrade (status-allocation) screen
FUN_0003cad3-ish: camX = S.left, camY = S.top;
P.x = (63398 == 0) ? camX - 0x1e : camX + 0x15e;   // entry side from next seg flag
      ± 0x78 walk-in offset if walk-in table byte set
P.y = u32[0x52c3c + ts0*4 + (ep-1)*0x28] (+ camY band base);
FUN_0003ed6f();        // re-drain remaining .stg queue (wave reset)
load music; FUN_00010b1d(u32[0x52da4 + ts0*4 + (ep-1)*0x28]);  // player sprite set
```

### 5.4 Kill-clear thresholds (`0x52a1c`, ts0 column)

ep1=1000, ep2=1000, ep3=1000, **ep4=15**, ep5=1000, ep6=1000, ep7=1000,
ep8=1000, **ep9=24** (1000 = disabled → those episodes rely on exit zones and
the final-segment boss).

## 6. Ground Y

- Entity world Y = authored record Y (spawn) / physics thereafter; there is no
  authored-Y→ground snap at spawn.
- Screen Y = world Y − camY. Player world Y = camY + playerY-table value, so
  the player sits at fixed screen row **172** (ep1-4,6-8), **160** (ep5) —
  inside the bottom 50% of the 200-px band. Floor row f5=195 (ep1) → 23 px of
  sprite height below the anchor — actors stand on the platform, not at their
  raw authored Y.
- f4 (walkline) = enemy walk baseline within the band (ep1: 139 subway, 115
  strip, 120 street, 148 mall); f5 = floor surface row. The lower-band ep1
  spawns (Y 341-392) minus camY 200 → screen rows 141-192: again bottom half.
- The original never draws a segment's floor outside its rect; the floor strip
  is per-segment (w×200), which is why the port's "one big strip" compositing
  leaks the next sub-stage's green bricks (scratch/stage1-broken.png).

## 7. Port gap analysis → fixes

Current port (`port/src/stage.c`) treats each episode as ONE arena:

| Port today | Original | Fix task |
|---|---|---|
| `place_enemies`: all tag-1 spawns flattened to `y = Y/2`, activated by player proximity `AHEAD_ACT` | camera-triggered drip at camX±edge, authored world Y, cap-checked | fix-spawn |
| `build_floor`: composites ALL `.wal` nodes into one `world_w×200` strip | per-segment floor, camera clamped to segment rect | fix-substage, fix-stage-end |
| single `world_w`, camX clamp at `world_w - 320` | per-segment clamps [left, right-0x140] + tag-2 gates | fix-stage-end |
| no camY, no intermission, no kill-clear | segment chain, d244 intermission, kill-clear/boss states | fix-substage |
| enemies float at raw Y | screen Y = worldY − camY on the band walkline | fix-ground |

## 8. Evidence index (decompiles obtained this pass)

- `FUN_0003ed6f` / `FUN_0003e20f` — record drain + tag dispatcher (tag-1/2/3/4/6)
- `FUN_0003c6ec` / `FUN_0003c7d7` / `FUN_0003c853` — segment loader (mem layout §2.2)
- `FUN_0003cad3` — camera init
- `FUN_0003d4e4` — per-frame scroll gates (verbatim §4.2)
- `FUN_0003cc6b` — Y-ramp (verbatim §4.3)
- `FUN_0003d244` — sub-stage transition / intermission (§5.3)
- `FUN_0003a829` — stage main loop + state handling (§5.2)
- `FUN_00012616` — entity update; death→state mapping, kill counter, exit-zone test @00012e58 (§5.1-5.2)
- `FUN_00010efc` — spawner (authored Y verbatim; boss slot)
- Tables read: 0x528b4, 0x52a1c, 0x52c3c, 0x52da4, 0x52be0 (9×10 each, §3/§5.4/§6)
- Data: `scratch/stage_analysis/wal_dump.txt` (all 9 chains), `stage1.wal` hexdump (layout §2.1), `scratch/dump_stage_recs.py` output (ep1 tag-1/2/6 records)

## 9. Open items (to confirm at runtime, task `verify`)

1. **Ep1 strip segment** (3280-3600, same band as subway): d244 advances one
   segment per transition, but no tag-6 exit zone covers it. Hypothesis: it is
   a short walk-through segment (elevator shaft) between subway and street, or
   the exit-zone rect persists/behaves differently across the transition.
   Confirm with a dosbox-staging playthrough; port fallback: a segment with no
   exit zone, no boss and kill-clear disabled auto-advances when the player
   reaches its far edge.
2. **Tech-set index S+0x40**: never written by the observed loader path → all
   table lookups use ts0. Non-ts0 table entries exist (cap/kill-clear) but are
   probably dead data. Port uses ts0.
3. `FUN_0003cb38`/`cbd9` exact camX step (port uses 2 px/frame toward the
   player inside the dead-zone bands — cosmetic only).
4. `FUN_00038f3d(1)` intermission screen layout (upgrade/status menu) — port
   shows a simple upgrade menu between sub-stages.
