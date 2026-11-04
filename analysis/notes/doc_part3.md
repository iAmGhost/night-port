
---

## 8. Combat pass — `FUN_0001441e` (batch1 §combat)

Runs once per entity per tick (attacker pass). For attacker `A`:

### 8.1 Body-bump technique (pre-check)

If A is walking (action 2) into an enemy within 13 px moving the same
direction and `A[0x27] >= 2` (tech-set ≥ 2) → action `0xda` (body-bump
technique). Tech-set < 2: no-op.

### 8.2 Hit testing

For each of the 6 hitbox slots `i` of A's current action:
- Window: slot live when `u16(act+8+4i) <= frame < u16(act+10+4i)`.
- Box: per-frame offsets from A's type row `0x639ec + type*0x14`:
  x-array at slot `+0x1c`, y-array at `+0x20`, indexed by
  `(frame % total)*2 + act[0x6d]`.
- Overlap via `FUN_00013c4d(A, B, act, i)`: action window active,
  lane/tolerance match (`|A.y - B.y|` within tolerance + lane), facing fix
  from lane; spawns hit FX on success. Returns 1 on hit.

### 8.3 Damage

- Base `d = *(act + 0x20 + 16*i)` (per-slot damage int).
- Multipliers: ×2 if the victim is knockdown-capable (`act_B[0x6c]`) and not
  in actions 0x33..0x35; ×3 if victim is in 0x33..0x35 (guard-break /
  recovery crit); ×1 otherwise (a player in a safe state forces ×1).
- **Armor-first absorption**: `d` is taken from victim `0x56` (secondary
  pool), then `0x23` (armor), then `0x29` (body HP). Armor depleted with
  `0x30 == 1` → `0x30 = 2` (armor-broken state).
- Box class `c = *(act + 0x38 + i)`:
  - `1..9` → non-lethal: victim action = `*(0x51b60 + c*4)` (reaction
    table), hitstun = `u16(act + 0x4a + 2*i)`.
  - `10..20` → heavy class: random knockback ±20 px, random dy, extra
    hitstun.
  - other → knockdown (§8.4).

### 8.4 Knockback & victim states

- **Player victim**:
  - action `0x1e` (blowback): dy offset −15, frame = `(yoff + 99)/3`,
    knockback 2 or 3 px/frame (class < 0xb or > 0x14 → 2, else 3), initial
    25 px dx push.
  - action `0x14` (block success): only while guarding (0x97) with class 1-9.
- **Enemy victim, knockdown**: grounded → pushback 10 px (class < 0xb or
  > 0x14) or 20 px; airborne → hitstun 80, knockback 1 or 4 px/frame.
- Victim latches `B[0x46] = A` (attacker), `B[0x4e] = A's action`,
  `B[0x4f] = slot`.
- SFX: per-slot id `*(act + 0x63 + i)` → deferred `0x63364`/`0x63368`.

### 8.5 Grabs & throws

- **Grab onset**: A in action 0x65 (101) or 0x99 (153) and victim not
  knockdown-capable → A → 0x66 (grab-hold), B → 0x1f (grabbed), mutual
  `0x4a` link, B repositioned within 24 px of A with y synced.
- **Release** (per release action of A):
  - `0x67` (103): flips target facing; target `kind 1 → 0` (dormant —
    "knocked out of the fight" throw).
  - `0x68` (104), `0x6f` (111), `0x72` (114), `0x73` (115): throw variants.
  - `0x71` (113): also clears the target's EXP counters.
  - On release, partner `0x4a` is cleared and the combo ring reset
    (`0x348e2`).

### 8.6 EXP & death

- Attacker == P: `P[0xbb] += d`; after the victim loop
  `P[0x52] += P[0xbb]` (pending EXP → level-up screen).
- Death: victim `0x30 = 1`; death anim from `*(u16*)(T+0x08 + type*0x88)`
  into `0x32`. Player death: hitstun −0x38, SFX 0x28; enemy death: hitstun
  120.
- **Boss death** (victim == `0x53c80`): jingle from
  `*(0x53d0c + *(0x51c68 + A_type*4)*4)`, FX reset `0x3ee0a` + `0x35c5c`,
  `DAT_00053104 = 0x23` (slow the throttle).
- Wave-clear bookkeeping (in `FUN_00012616`, §1.3): kill counter
  `0x639bc++` on any enemy death; player death → `0x6336c = 1`; boss death →
  `0x6336c = 3`; counter ≥ `*(0x52a1c + S[0x40]*4 + (ep-1)*0x28)` →
  `0x6336c = 4` (segment complete).

---

## 9. EXP, level-up & music

### 9.1 EXP → level-up screen — `FUN_00038f3d` (batch1 §levelup)

Shown at segment end (from `FUN_0003d244`, §11.2). Six stat columns
(columns 0..5 = tech-set, char, style, speed, reserved, overall level;
current levels seeded from P[0x27], P[0x28], P[0x26], P[0x25]):

- Thresholds: `*(0x53c94 + col*0x14 + level*4)`; max levels
  `*(0x531b0 + col*4)` (6 values).
- Column 1/4 (armor columns): upgrading when the current armor
  `P[0x23]` already equals `*(0x52738 + char*4)` refunds EXP and re-sets the
  armor; `P[0x30]` cleared.
- Confirm: if EXP < threshold → help8.pcx + SFX (insufficient); if level ==
  max → help9.pcx; else EXP −= threshold, level++ (col 5 increments the
  overall level `DAT_00053100`).
- Banked: `P[0x2b] += P[0x52]` at entry; `P[0x52] = 0`.

UI assets: `levelup.pcx`, `number1/2.spr`, `help1..20.pcx`.

### 9.2 EXP sources

Only player-dealt combat damage accrues EXP (`0xbb` → `0x52`, §8.6).

### 9.3 Stage-end music monitor — `FUN_00010d47` (batch1 §music_10d47)

Each tick, with stage `S`: when `DAT_00053c48` (track count) <
`DAT_00063e38 / 0x1ea` (the millisecond counter divided by the 494-unit
beat): reset `0x63e38`, `0x52ff8 = 0`, play
`FUN_00010b1d(*(0x52da4 + S[0x40]*4 + (ep-1)*0x28))` (stage/episode CD
track table — 10 tracks × 9 episodes), `0x52ff8 = 1`. The `0x52ff8` flag
also gates key polling (§4.1).

---

## 10. Camera

### 10.1 Trigger gate — `FUN_0003d4e4` (batch3 §stgev_3d4e4)

Per tick, with `S` and player `P`:

- **Right scroll** (`trig_a` `0x3cb38`) when
  `S[0x3b] == 0` and `P.x − camX > 0xbe` (190) and
  (`S[0x2e] == 0` or `camX + 0x140 <= S[0x2e]`) and
  (`S[0x36] == 0` or `camX + 0x140 <= S[0x36]` or only the player exists):
  `camX += 1` (walk) or `+= 3` (action 3 dash) until `camX` reaches
  `S[8] − 0x140` (snap within 2 px).
- **Left scroll** (`trig_b` `0x3cbd9`) when
  `S[0x3a] == 0` and `P.x − camX < 0x82` (130) and
  (`S[0x2a] == 0` or `S[0x2a] <= camX`) and
  (`S[0x32] == 0` or `S[0x32] <= camX` or only the player exists):
  `camX −= 1 / −= 3` until `camX` reaches `S[0]` (snap within 2 px).
- **Segment chain** (`trig_c` `0x3cc6b`) when `camY + 0xc2 <= P.y`
  (player within 6 px of the screen bottom) and
  `camX == *(S+0x34c)[0]` (camera reached the next segment's start X) →
  §10.3.

So: 320×200 window; dead zone `P.x − camX ∈ [130, 190]`; follow speed
1 px/tick (3 in dash); clamped to `[S[0], S[8] − 0x140]`. `S[8]` is the
segment's right clamp, `S[0]` its left.

### 10.2 Parallax decorations — `FUN_0003d75c(dx)`

On camX change: for each decoration (count `DAT_00062a90`, table
`0x5d0b4`, 5-field rows: x, ?, ?, y, speed-float at `+4`):
`speed -= dx * *(0x5274c + S[0x40]*4 + (ep-1)*0x28 ...)` — per-type parallax
factors from the `0x5274c` table.

### 10.3 Segment transition (`trig_c` details)

- Start: `nextSeg = *(S + 0x34c)`; `S = nextSeg` pending;
  `DAT_000639e4 = 1` (transition flag); save player stats unless ep 1.
- Ramp: `camY += 1/tick` toward `nextSeg[0xc] − 200`.
- Commit: when `camY == nextSeg[4] + 1` → `S = nextSeg`, `0x639e4 = 0`,
  save stats.
- Render rule during transition (`FUN_0003ce0b`): 2-row vs 1-row background
  blend depending on `camY` vs `nextSeg[4]`/`nextSeg[0xc]`.

### 10.4 Segment init — `FUN_0003cad3` (`spawn_seg`)

`camX = (S[0x11] == 0) ? S[0] : S[2] − 0x140`; `camY = S[1]`; transition
flag from `S[4]`. Called after wave reset and at stage open.

### 10.5 Palette ripple — `FUN_0003d606` (batch1 §spawner)

Every 10 logic frames (§1.2/13): rotate `S[0x3c]` palette entries per frame
through the `{lo, hi}` u16 pairs at `S + 0x44` (up to 60 entries), each via
`FUN_0003d67c` (256-color RGB buffer shift + DAC write). This is the
animated background effect (water/pulse), **not** a spawner.

---

## 11. Wave / segment flow

### 11.1 Wave reset — `FUN_000376c8` (batch1 §wavereset)

Called at each segment transition: free resources (`0x3d88f`, `0x3237b`),
FX list reset (`0x3ee0a`), rebuild the player from template
(`player_mk(resource 0x147, x = −30, y = *(0x52c3c + S[0x40]*4 +
(ep-1)*0x28))`), then:

- `0x53c7c` = new player; armor reset `P[0x23] = 0`; char id restored from
  `0x63384`; `P[0x56]` = armor pool (`u16(0x53e9c)` char 0, else
  `u16(0x52738 + char*4)`); `P[0x29]` = `u16(0x53ea4)` (type 0 max HP).
- `0x53c88 = 0` (last-hit), `0x53c84 = P` (active bar).
- `FUN_00037b03(1)` — re-read `.stg` (param 1 skips the background header
  block but re-reads spawn records + type table).
- `S = DAT_00063358` (the saved next segment); key tables re-selected by
  tech-set; `P[0x123]` action table from `0x53c28 + style*4`.
- Stats restored: `P[0x2b] = 0x63374` (EXP), `P[0x27] = 0x63378`,
  `P[0x26] = 0x6337c`, `P[0x25] = 0x63380`.
- `FUN_0003cad3` (camera init, §10.4).
- Player placement: `0x63398 == 0` (entered from right) → `P.x = camX − 30`,
  facing 0; else `P.x = camX + 0x15e` (350, just off the right edge), facing
  1. `P.y = camY + offset`; `P[0x17] = 1` (idle).

### 11.2 Segment transition — `FUN_0003d244` (batch1 §cd_d244)

State 2/4 handler: cleanup non-player entities, wave reset (`0x376c8`),
switch `S` to `*(old S + 0x34c)`, camera init (`0x3cad3`), level-up screen
(`FUN_00038f3d(1)`), palette fade, then return to the loop (state 0).

### 11.3 Boss

`DAT_00053c80` = boss pointer (set when a boss-type enemy is activated);
`DAT_00053c84` = active-bar pointer (normally P); `DAT_00053c88` = last
damaged entity (for the small HP bar). Boss HP bar uses `0x73/0x77` totals.

---

## 12. `.stg` load — `FUN_00037b03` (batch2 §stg_load)

- Param 0 (episode start): clear `0x62a8c/0x62a90`, clear `0x5d08c` (20 B),
  `0x53c80 = 0`; open the stage file (resource id `0x354` in the file table,
  name built by `FUN_000367c9`); `FUN_0003c6ec(stream)` parses the header →
  base record `DAT_0006334c`; `DAT_000639dc` = first u32 (camX0),
  `0x639e8 = 0`; `FUN_0003c853(S, stream)` reads background/tile data;
  `FUN_0003cad3` (camera init); background sprite from file `0x3c` parsed by
  `FUN_0003d8d4` → 5-field decoration table `0x63dfc`.
- Spawn records: count `DAT_00063e14`; each record 0x59 (89) bytes:
  **tag 1** = spawn by enemy type (record+1 = type u32; lazy-loads the type's
  `.spr`/`.act` when `0x639ec + type*4 == 0`); **tag 5** = decoration (5
  fields → `0x5d0b4 + idx*5`: x, ?, ?, y, 0).
- Type definitions: 101 (0x65) rows of 0x88 → `0x53ea0` (§2.1).
- Tag-2/tag-3 records (the port's "spawn wave" interpretation) are consumed
  by the header parse; the binary's *active* spawning is tag-1 placement +
  proximity wake (§6.2) — see task-2 for the episode-by-episode cross-check.

### 12.1 Player constructor — `FUN_00010efc` (`player_mk`)

Builds a 0x144-byte entity: kind 1, type 0, HP/armor pools
(`0x56` → `0x23` → `0x29` from the template), action table from
`DAT_00053818`, inserts into the doubly-linked list.

---

## 13. Function map (playable core)

| address | name | role |
|---------|------|------|
| `0x3af18` | game_main | episode driver; calls frame loop, handles return codes |
| `0x3a829` | frame | the per-tick pipeline (§1) |
| `0x322f5` | pipe_ai | enemy AI pass → `0x31a8e` |
| `0x3209c` | pipe_act | action pass → `0x12616` |
| `0x347e3` | keypoll | key state scan → `0x34735`/`0x34635` |
| `0x3272f` | pdisp | player action dispatcher (§5) |
| `0x320f5` | pipe_hit | combat pass → `0x1441e` |
| `0x3d4e4` | stgev | camera gate → `0x3cb38`/`0x3cbd9`/`0x3cc6b` |
| `0x3ed6f` | fx_pre | FX list update |
| `0x38098` | render | z-sort + draw + throttle (§1.2) |
| `0x10d47` | mus_mon | stage-end music monitor (§9.3) |
| `0x1441e` | combat | per-attacker hit/damage/grab pass (§8) |
| `0x13c4d` | hitbox | overlap + lane test |
| `0x12616` | ent_upd | action/frame update + wave bookkeeping (§7) |
| `0x12efa` | ent_draw | single entity draw |
| `0x13aad` | zsort | entity z-order before render |
| `0x31a8e` | ai_wrap | per-enemy AI wrapper + dispatch (§6.1) |
| `0x17f13..0x2fe1e` | ai_t0..t27 | 25 AI bodies (§6.5) |
| `0x1545e/0x15295` | move_dx/dy | world movement + clamps |
| `0x1586b` | move_dy2 | no-vertical movement variant |
| `0x1519f/0x151d5` | scr_x/scr_y | world → screen |
| `0x15a1f` | find_tgt | target search (nearest valid) |
| `0x1194c` | kndown | knockdown/death reaction timeline |
| `0x31f5f` | adv_act | action queue advance (8 slots) |
| `0x31f10` | hit_fx | hit FX spawn |
| `0x112fd` | ent_free | unlink + free |
| `0x37b03` | stg_load | `.stg` parser (§12) |
| `0x10efc` | player_mk | entity constructor (§12.1) |
| `0x376c8` | wavereset | segment respawn (§11.1) |
| `0x3d244` | stg_trans | segment transition (§11.2) |
| `0x38f3d` | levelup | EXP allocation screen (§9.1) |
| `0x3cad3` | spawn_seg | camera init from stage record (§10.4) |
| `0x3d75c` | parallax | decoration parallax on camX change (§10.2) |
| `0x3d606` | ripple | palette ripple effect (§10.5) |
| `0x381d7` | walkin | screen-edge slide-in |
| `0x3801b` | full_render | pre-loop full render |
| `0x39c03` | pause_menu | pause/quit menu |
| `0x38445` | end_scr | end/continue screen (return-code source) |
| `0x3a70e` | boss_intro | one-shot boss intro |
| `0x10b1d` | cd_play | CD track start |
| `0x4185e` | sfx_play | VOC sfx (table `0x53d08`) |
| `0x404c6` | vflip | double-buffer swap |
| `0x40042` | pal_dac | palette upload (ports 0x3c8/0x3c9) |

### 13.1 Key global state (0x52xxx–0x63xxx)

| addr | meaning |
|------|---------|
| `0x53c70/0x53c74` | entity list head/tail |
| `0x53c68` | entity count |
| `0x53c7c` | player pointer |
| `0x53c80/0x53c84/0x53c88` | boss / active-bar / last-damaged pointers |
| `0x53ea0` | type table (101 × 0x88) |
| `0x639ec` | per-type hitbox table (10 ptrs × 101) |
| `0x53e9c/0x53ea4` | type-0 armor pool / max HP |
| `0x52738` | per-char armor pools (u16 × 4) |
| `0x52ffc` | per-frame speed table (12 float/int entries) |
| `0x51d5c` | per-type movement speed floats |
| `0x51b60` | box-class → victim reaction table |
| `0x53c28` | style → action table |
| `0x53c94` | EXP thresholds (6 × 4 × int) |
| `0x531b0` | max stat levels (6) |
| `0x53100` | overall level (0 = unset) |
| `0x52a1c` | per-stage/episode kill targets (10 × 9) |
| `0x52b84` | per-stage/episode variant flags (10 × 9) |
| `0x52c3c` | per-stage/episode player base-Y (10 × 9) |
| `0x52da4` | per-stage/episode CD tracks (10 × 9) |
| `0x5274c` | parallax factors |
| `0x5d0a0..0x5d0b1` | stage exit rect (x0, x1, y0, y1) + direction flag |
| `0x5d0b4` | decoration table (5-field rows) |
| `0x63354/0x63358` | current / saved-next stage record |
| `0x6336c/0x63360` | state code / logic frame |
| `0x63398/0x6339c` | entry direction / idle timer |
| `0x639bc` | kill counter |
| `0x639dc/0x639e0/0x639e4` | camX / camY / transition flag |
| `0x63364/0x63368` | deferred SFX id / delay |
| `0x62c58/0x62cd8` | key states / repeat timers (128 each) |
| `0x63f14..0x63f28` | combo ring (10 × 2 B) + seen |
| `0x63f50/0x63f0c` | ring index / key-table position |
| `0x62aa0/0x62ab4` | per tech-set key tables (4 × 0x14 each) |
| `0x62a98/0x62a9c` | active key tables (normal / grapple) |
| `0x62a94` | facing at key press |
| `0x63ed0/ed3/ed5/ed8/eb4/ec1` | held direction/technique key flags |
| `0x63374..0x63384` | saved player stats (EXP, tech, style, speed, char) |
| `0x62d58/0x62d5c` | attack latch / repeat delay |
| `0x63e24/0x63e28/0x63e38` | frame timer / sub-tick / ms counter |
| `0x63e89` | pause key flag |
| `0x52ff8` | CD music active flag (gates key poll) |
| `0x633b8` | palette work buffer (768 B) |

---

## 14. Port gap analysis (initial — refined in task-6)

Port: `src/stage.c` (1011 ln), `game.c` (422 ln). Current port model
(docs/05, 06, 07): single-stage walk, tag-1 position-based placement with
proximity wake, `level_end_x` walk-past clear, fixed tick.

| # | original (this doc) | port today | gap |
|---|--------------------|-----------|-----|
| G1 | segment chaining via `S+0x34c`; exit-rect trigger `0x5d0a0..b1` with direction flag (§7.6, §10.3) | single stage, `level_end_x` | **major**: implement segments + exit zone |
| G2 | wave clear = kill counter vs `0x52a1c` per stage/ep (§8.6) | boss-flag based | major |
| G3 | camera dead zone [130, 190], 1/3 px follow, clamps `S[0]`/`S[8]−0x140` (§10.1) | (check stage.c camera) | medium |
| G4 | 6-slot hitbox windows, class table `0x51b60`, ×2/×3 multipliers, armor pools `0x56→0x23→0x29` (§8) | simplified combat model | major (tuning) |
| G5 | dormant→wake AI, alert radius, charge patience 100/1000, 25-case dispatch (§6) | proximity activation only | major |
| G6 | 4-step press progression, combo ring, direction remap, tech-set key tables (§4) | direct key mapping | medium |
| G7 | EXP/level-up allocation screen, thresholds `0x53c94`, overall level `0x53100` (§9.1) | absent/absent? | major (feature) |
| G8 | palette ripple every 10 frames (§10.5), palette cross-fade 50 frames | static palette | cosmetic |
| G9 | key poll gated on CD flag `0x52ff8` (§4.1) | direct | none (no CD in port) |
| G10 | 4-frame oscillating speed table `0x52ffc` (§5.1) | constant speed? | medium (feel) |
| G11 | idle 500 ticks → crouch 0x28 (§5) | ? | minor |
| G12 | boss HP bar (`0x53c84/0x53c88`, `0x73/0x77` totals) + death jingle (§8.6) | partial | medium |

Items marked "?" need port re-read in task-6.

---

## 15. Open questions (for tasks 2-8)

1. `trig_a`/`trig_b` clamps `S[0x2e]/0x36` vs `S[8]`: exact stage-record
   layout for all 10 stages × 9 episodes (Python `.stg` cross-check, task-2).
2. `0x63e24` tick rate vs DOSBox real time (task-8 measurement).
3. Per-slot damage/hitstun/sfx byte offsets in the action record (task-5,
   vs `scripts/decode_act.py`).
4. AI bodies `0x1cf64` (arg variants) and `0x2319c` (arg 2) — parameterized
   families; document per-type differences (task-4).
5. Walkmap `.wal` usage: the port uses `.wal` for world width; the binary's
   `move_dx` clamps come from stage record + static table `0x5c8bc`
   (task-3).
