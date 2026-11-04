# Stage system analysis (work-in-progress) — NIGHT.EXE

Goal: fix SDL port stage behavior (drip spawn, grounded actors, sub-stage bounds).
Corrected model (user): each episode = chain of sub-stages (ep1: subway → street → mall),
intermission upgrade menu between sub-stages; street sub-stage has Y-axis camera scroll
(vertical camY ramp on transition). .wal segments = separate sub-stage arenas stacked in
world Y, NOT floors of one arena.

## 1. Spawn/event record queue (verified by decompile)

- `FUN_0003ed6f` (per-frame drain): walk linked list `DAT_00063e18` (head) / `DAT_00063e20`
  (cursor); next ptr at rec+0x51; call `FUN_0003e20f(rec)` on each; when `rec[0]==0`
  (consumed) → unlink (`FUN_0003e14c`) + free (`FUN_00041c28`).
  ⇒ Records persist in the queue each frame until their tag byte is zeroed.
- Record = 81 B: `[u8 tag][80B payload]`.

## 2. Tag dispatcher `FUN_0003e20f` (full decompile obtained, see §2.1)

Globals:
- `DAT_000639dc` = camX, `DAT_000639e0` = camY (full-res world px)
- `DAT_00053c68` = active entity count (player + alive foes)
- `DAT_00063370` = episode (1..9)
- `DAT_00063354` = S, pointer to current segment record (int*)
- `DAT_000639dc + -0x1e` = camX-30 (off-screen left), `+0x15e` = camX+350 (off-screen right,
  screen is 320 wide)

### 2.1 TAG 1 (spawn) — the drip spawner

Record fields: type=+1, side=+5 (0=left,1=right), X=u16 +9, Y=u16 +0xd, zone=+0x11, posmode=+0x15.

Outer gate: `*(char*)(S+0x11) == 0` (byte 0x11 of segment record; for ep1 all segments it is
0 → first branch; second branch is the rare variant, see note below).

A) **zone == 1** (special zone spawn):
   fires when `5d08c <= X <= 5d094 && 5d090 <= Y <= 5d098` (rect written by TAG 4).
   Spawn at camX-0x1e (side0) or camX+0x15e (side1). NO cap check, no X-window.
   `ent+0x1b = side` (facing). Record consumed.

B) **zone == 0** (normal): gated by cap:
   `DAT_00053c68 < u32[0x528b4 + S[0x40]*4 + (ep-1)*0x28]`
   (S[0x40] = segment tech-set index; table = 9 ep × 10 u32).
   - side == 0 (from left/behind): requires `camY <= Y <= camY+200` AND `S[0] <= X <= S[8]`
     (within segment left..right).
     * posmode==1 → spawnX = X (authored, on-screen)
     * posmode==0 → requires `X <= camX-0x32` (camera already 0x32 past trigger) →
       spawnX = camX-0x1e (just off-screen LEFT).
     * facing = side. Consumed.
   - side == 1 (from right/ahead): requires `camY <= Y <= camY+200` AND `S[0] <= X <= S[8]`
     AND `camX <= X <= camX+0x172` (trigger within current view+50 px).
     * posmode==1 → spawnX = X (authored) AND `ent+0x17 = 3` (initial action 3)
     * posmode==0 → spawnX = camX+0x15e (just off-screen RIGHT).
     * facing = side. Consumed.

   Note: the rare outer-branch variant (S+0x11 != 0) differs only in the X-window:
   side0 uses `camX-0x32 <= X <= camX+0x140`, side1 uses `X <= camX+0x172` (no camX<=X).

Spawn call: `FUN_00041bdc(0x147)` → character template; `FUN_00010efc(tmpl, spawnX, 0, Y,
type, 1)` → entity (decompile pending). If template missing → 0 (no spawn, NOT consumed?
— in the code local stays 0 and `*puVar2=0` is still executed only in the success path of
some branches; treat as: record consumed on successful spawn).

### 2.2 TAG 2 (camera gates) — per subtype, all require `camY < Y < camY+200` (Y in view):
- sub1: `X < camX` → `S+0x2a = X` (left clamp set when camera is right of it)
- sub2: `camX < X`  → `S+0x2e = X` (right clamp)
- sub3: `camX < X < camX+0x140` → `S+0x32 = X`
- sub4: `camX < X < camX+0x140` → `S+0x36 = X`
- sub5: `camX < X` → `S+0x3a = 1` (lock flag)
- sub6: `camX < X` → `S+0x3b = 1` (lock flag)
Camera gate (docs §10.1, FUN_0003d4e4): right scroll needs `S+0x3b==0`,
`camX+0x140 <= S+0x2e (if set)` and `<= S+0x36 (if set)`; left scroll needs `S+0x3a==0`,
`S+0x2a <= camX (if set)` and `S+0x32 <= camX (if set)`.

### 2.3 TAG 3: append (X,Y,X2,Y2) to zone table `0x5c8bc + DAT_00062a8c*0x14`, count++.
TAG 4 (gate rect writer): only when `DAT_00053c68 == 1` (ONLY player active) and
`camX <= X <= camX+0x13f && camY <= Y <= camY+199` (record under camera) →
`5d08c=X, 5d090=Y, 5d094=X2, 5d098=Y2` (the rect gating zone==1 spawns).
TAG 5: unused in dispatcher (default: break — record never consumed!).
TAG 6 (exit zone): when `camX <= X <= camX+0x13f && camY <= Y <= camY+199` →
`5d0a0=X, 5d0a4=Y, 5d0a8=X2, 5d0ac=Y2, 5d0b0=u32@+0x19`; last one under camera wins.

## 3. Segment init `FUN_0003cad3` (spawn_seg) — decompiled:
```
if (*(char*)(S+0x11) == 0) camX = S[0];   // left
else                       camX = S[2]-0x140;
camY = S[1];                   // top
DAT_000639e4 = (char)S[4];     // = f4 walkline (110-150) stored as transition-flag reg
```
In-memory segment record (int* array, CONFIRMED by spawn X-range check S[0]<=X<=S[2]):
`+0x00 left, +0x04 top, +0x08 right, +0x0c bottom, +0x10 f4 walkline, +0x14 f5 floor,
+0x2a..+0x3b runtime camera gate fields (TAG 2), +0x34c next-segment ptr, +0x40 tech-set idx`.
(orient/flag from the .wal file are NOT at byte 0..1 in memory, or are absorbed into the
u32 layout; byte +0x11 is f4's 2nd byte → 0 for all ep1 segments.)

## 4. ep1 data (scratch/dump_stage_recs.py scratch/res 1)

.wal segments (world coords, full-res px):
```
seg0: orient0 flag0 (0,0)-(1600,200)      f4=139 f5=195   <- subway (stg1)
seg1: orient1 flag0 (3280,0)-(3600,200)   f4=115 f5=195   <- vertical strip (320 wide)
seg2: orient0 flag1 (2000,200)-(3920,400) f4=120 f5=195   <- street (stg2)
seg3: orient0 flag1 (4000,200)-(4320,400) f4=148 f5=195   <- mall (stg3)
```
.stg: 38 tag1 spawns: 26 with Y 149..193 (top band, subway/strip), 12 with Y 341..392
(bottom band, street/mall). Screen Y = Y - camY ⇒ always 141..193 (bottom ~50% of 200 px
screen) when the right segment's camY applies. THIS is why the port (y=Y/2, camY=0 flat)
shows flying enemies: top-band spawns land at screen y≈75-96.
Tag6 exits: rec18 (1528,112)-(1600,198) [subway, extra=0], rec20 (2002,262)-(2052,398)
[street, extra=1]. Per-sub-stage exit zones, activated as the camera passes them.
Tag2 gates: sub1 (left clamp) X 100..1132 top band; sub2 (right clamp) X 2372..3518 bottom
band; sub3 X 2057..2730 bottom; sub4 X 617..1526 top + 3268 bottom.
Tag3 zones: 9 (trigger rects, e.g. (3765,289)-(3920,398) in street).
Sentinel 0xFFFF spawners: none in ep1 (they're in ep6/7 per docs).

## 5. Port bugs (current stage.c)
1. `place_enemies` flattens ALL tag1 records into one world (y = Y/2), activates on
   player proximity (AHEAD_ACT=240) at authored positions ⇒ random-ish pops + flying.
2. `build_floor` composites ALL .wal segments into one world_w×200 strip (later segments
   overwrite earlier at overlapping X) + camera scrolls to raw .wal extent ⇒ next-sub-stage
   bleed (scratch/stage1-broken.png: subway left, street-brick right).
3. No segment chain / camY / intermission at all.

## 6. TODO (next turns)
- [ ] decompile FUN_00010efc (entity spawn: how Y lands on ground)
- [ ] decompile FUN_0003d4e4 (camera gate: X scroll clamps + trig_c Y ramp)
- [ ] decompile loader FUN_0003c6ec / FUN_0003c853 (chain order, in-mem layout, 0x34c, 0x40)
- [ ] decompile FUN_0003d244 (transition handler) + FUN_00038f3d (level-up/intermission screen)
- [ ] read tables: 0x528b4 (cap, 0x180 B), 0x52c3c (player Y), 0x52a1c (kill-clear)
- [ ] verify ep1 chain order = seg0→seg1?→seg2→seg3 (or seg0→seg2→seg3, strip = transition)
- [ ] write docs/11-stage-system.md
