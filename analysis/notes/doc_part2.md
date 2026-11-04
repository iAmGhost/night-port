
---

## 2. Entity model

Entity records are 0x144 bytes, doubly linked: `+0x13f` next, `+0x143` prev;
list head `DAT_00053c70`, count `DAT_00053c68`. Constructor `FUN_00010efc`
(batch2 §player_mk); free = `FUN_000112fd(ent, 0)` + `FUN_00041c28`.

| offset | field (evidence) |
|--------|------------------|
| `+0x00` | kind/active: `0` dormant (enemy: no AI until woken, §6.3), `1` active, `2` skipped by AI pass and non-targetable in combat (batch3 §pipe_22f5) |
| `+0x01..0x04` | world X (int) |
| `+0x05..0x08` | vertical offset from base line (int; 0 = grounded; `move_dy` §7.5) |
| `+0x09..0x0c` | world Y (int) |
| `+0x0d..0x10` | type (int; `0` = player; indexes `T` and hitbox table `0x639ec`) |
| `+0x11..0x14` | frame counter within current action |
| `+0x16` | (byte) airborne/drop counter — set by hits (slot class 5/7/0xb), decremented in `FUN_00012616` while action has `0x6c` flag |
| `+0x17..0x1a` | action ID. Known values: 1 idle, 2 walk, 3 dash/charge, 4 jump, 5 airborne, 0x14 block-success, 0x15 death, 0x16 appear (wake), 0x1d appear-2, 0x1e hit/blowback, 0x1f grabbed, 0x28 crouch (auto after 500 idle ticks), 0x29 hold/freeze, 0x65/0x99 grab-attack, 0x66 grab-hold, 0x67-0x73 release throws, 0x6f/0x70/0x71 grabs, 0x83-0x85 attacks/dashes, 0x8c/0x8d per-frame-speed moves, 0x97 guard, 0x98/0x9b/0x9c/0xa3 techniques, 0xaa-0xad / 0xb4-0xb8 / 0xbe-0xc7 auto-advancing chains, 0xda body-bump technique |
| `+0x1b..0x1e` | facing (`0` = right, `1` = left) |
| `+0x1f..0x22` | AI substate (`0x1f`: 0 approach, 0x0a alert/chase, …; §6.4) |
| `+0x23..0x24` | armor HP (u16); damage hits armor first (§8.3) |
| `+0x25..0x28` | sprite-data pointer (copy of `T + type*0x88`) |
| `+0x25` (byte) | speed class — indexes per-frame speed table `0x52ffc + ((frame%4)*3 + class)*4` (§5.1) |
| `+0x26` (byte) | style — `.act` table select: `DAT_00053818 = *(0x53c28 + n*4)` → `ent[0x123]` |
| `+0x27` (byte) | tech-set (0..3) — key tables `0x62aa0`/`0x62ab4 + n*0x14`; `< 2` disables body-bump (§8.1) |
| `+0x28` (byte) | char id — armor pool `*(u16*)(0x52738 + n*4)` |
| `+0x29..0x2a` | body HP (u16); init from `*(u16*)(T+0x04 + type*0x44)` |
| `+0x2b..0x2e` | EXP total (int, player) |
| `+0x30` | death state: `1` dead, `2` armor broken (both terminal at hit resolution end) |
| `+0x31` | action-queue/grapple latch (8-slot queue, `FUN_00031f5f` batch2 §adv_act) |
| `+0x32..0x33` | death-animation action id (from `*(u16*)(T+0x08 + type*0x88)`) |
| `+0x3e..0x41` | queued next action: `0x3e` id, `0x42` its frame counter |
| `+0x42..0x45` | (u32) per-frame knockback step (px/frame) + scratch — set by hits (§8.4) |
| `+0x46..0x49` | victim: attacker pointer (grab latch); `0x4e` attacker's action id, `0x4f` hitbox slot |
| `+0x4a..0x4d` | partner/grapple pointer (mutual link) |
| `+0x50..0x51` | hitstun (u16); while > 0 the action frame counter is frozen and decremented instead (§7.4) |
| `+0x52..0x55` | pending EXP — per hit `P[0xbb] += dmg`; after each combat pass `P[0x52] += P[0xbb]`; consumed at level-up (§9.1) |
| `+0x56..0x59` | secondary armor pool (int). Player: `u16(0x53e9c)` (char 0) or `u16(0x52738 + char*4)`; enemies type 1-9: `0x30c` (780). Damage order `0x56` → `0x23` → `0x29` (§8.3) |
| `+0x5a` | death-FX one-shot flag |
| `+0x63..0x66` | (float) fractional movement accumulator (ai_t0 sub-pixel speed, §6.4) |
| `+0x6b` | (u16) charge timer (action 3) — increments while player within ±200 px; attacks at patience (§6.2) |
| `+0x73..0x7a` | total HP (`0x56+0x23+0x29`) and current total — boss HP bar source |
| `+0x8b..0x8e` | alert flags (ai_t0: `0x8b` alerted) |
| `+0xaf` | init 4 — AI patience class |
| `+0xbb..0xbe` | damage-dealt accumulator (attacker == player) |
| `+0x103` | walk step counter (→ 8; stuck check §6.1) |
| `+0x107/0x10b` | last-verified walk position (screen x/y) |
| `+0x10f` | AI target pointer |
| `+0x11f` | `rand() % 0x14` init latch |
| `+0x123..0x126` | action-table pointer (`.act`) |
| `+0x127,0x12b,0x12f,0x133,0x137,0x13b` | per-type pointer copies from `0x639ec + type*0x14` slots `+0x38,+4,+8,+0x34,+0xc,+0x10` |

### 2.1 Type table `T = 0x53ea0` (101 rows × 0x88 B, loaded in `0x37b03`)

| row off | field |
|---------|-------|
| `+0x00` | sprite data ptr (lazy `.spr` via `0x51d88 + type`) |
| `+0x04` | max body HP (u16) |
| `+0x08` | death-animation action id (u16) |
| `+0x0c` | **AI id** (0..0x27) → dispatch in `FUN_00031a8e` (§6.5); also charge-patience class: `< 0x1f` → 1000 frames, else 100 (§6.2) |
| `+0x10..0x17` | u16, u16 params |
| `+0x18..0x21` | u8[10] per-type params |
| `+0x3c` | movement-speed float index (u16) → `*(float*)(0x51d5c + n*4)` |
| `+0x40` | u16 param |

### 2.2 Per-type hitbox table `0x639ec + type*0x14` (10 ptr slots)

`+0x04` box-x offset array, `+0x08` box-y offset array, `+0x1c`/`+0x20`
per-frame box x/y arrays (used directly by combat, §8.2). Rows lazy-loaded in
`FUN_00037b03` when a `.stg` tag-1 record first references the type (batch2
§stg_load).

---

## 3. Action records (`.act`)

Per action id (table base `ent[0x123] + 4`, 4-byte entries pointing to
records). Round-trip-verified layout in `docs/08-animation.md` +
`scripts/decode_act.py`; offsets below are as read by the decompiler.

| off | usage |
|-----|-------|
| `+0x00` | total frames (u16) |
| `+0x04/+0x06` | primary active window (u16) — `FUN_00013c4d` |
| `+0x08..+0x19` | 6 hitbox slots {u16 start, u16 end} — combat slot `i` live when `start <= frame < end` (§8.2) |
| `+0x17` (byte) | auto-advance length: `frame >= it` and action ∈ {0xaa..0xad, 0xb4..0xb8, 0xbe..0xc7} → `action++` (§7.2) |
| `+0x20+16i` | per-slot damage (int) |
| `+0x38+i` | per-slot box class (byte) → victim reaction table `0x51b60` (§8.3) |
| `+0x4a+2i` | per-slot hitstun (u16), non-knockdown hits |
| `+0x63+i` | per-slot hit SFX id (byte) → deferred SFX `0x63364` |
| `+0x69/+0x6a` | action-entry SFX id (byte) / delay (u16) |
| `+0x6c` | (byte) per-frame vertical component flag: 1 → per-frame speed arrays + `move_dy`; 0 → `FUN_0001586b` (§7.5) |
| `+0x6d` | (byte) box phase offset: per-frame box index `(frame % total)*2 + 0x6d` |
| `+0x71` | (int*) per-frame X speed array |
| `+0x75` | (int*) per-frame Y speed array |

---

## 4. Input pipeline

### 4.1 Key poll — `FUN_000347e3` (batch3 §keypoll_47e3)

Runs only while `DAT_00052ff8 != 0` (CD music active — the flag is set by
`FUN_00010d47` after each `FUN_00010b1d` track start). Scans 128 key states at
`0x62c58` (repeat timers at `0x62cd8`):

- **low group** → `in_lo` `0x34735`: `{0x1d (Enter)}` ∪ `{0x2a ('*')}` ∪
  `{0x2d..0x35}` ∪ `{0x36 ('6')}`.
- **high group** → `in_hi` `0x34635`: `{0x38 ('8'), 0x48 ('H'), 0x4b ('K'),
  0x4d ('M'), 0x4e..0x50}`. `0x4b`/`0x4d` are the two direction keys.

### 4.2 Press progression

`in_hi` (0x34635): a queued press advances one step per frame (state in
`0x62c58[code]`): at step 1 it captures the player facing into `DAT_00062a94`;
at step 4 it records the key via `key_disp` `0x34353`, then self-resets
(state 1 with repeat timer `0x62cd8` decrement, or 0 when the timer is 0).
So a high-group key is recorded **5 frames after** its press event, with the
facing at *press time*.

`in_lo` (0x34735): at step 4 records via `key_disp`; while < 4 it advances and
calls `key_hold` `0x3442e` (synthesizes held pseudo-keys for the
6/8/Enter hold-combos).

Direction remap in `in_hi`: keys `0x4b`/`0x4d` are converted to pseudo-codes
before recording: facing 0 (right) → `code - 0x4f` (0x4b→`0x66`, 0x4d→`0x64`);
facing 1 (left) → `code + 0x19` (0x4b→`0x64`, 0x4d→`0x66`). `0x64`/`0x66`
therefore mean "back"/"forward" relative to facing.

### 4.3 Combo ring & key tables

`key_disp` `0x34353` maintains the ring buffer `0x63f14..0x63f27` (10 slots ×
2 B: key code + table delta), write index `0x63f50` (wraps at 10), seen flag
`0x63f28`, and current key-table position `0x63f0c`. The technique-matching
engine walks the ring against the active key table.

Key tables: `0x62aa0` (4×0x14) and `0x62ab4` (4×0x14) — per tech-set maps,
selected in the level-up screen: `0x62a98 = 0x62aa0 + techset*0x14`,
`0x62a9c = 0x62ab4 + techset*0x14` (batch1 §wavereset path, also `0x38f3d`).
The frame loop passes `0x62a9c` instead of `0x62a98` while the player is in
grapple actions 0x66/0x70.

`key_get` `FUN_0003493e` returns the most recently recorded key as a char —
the dispatcher (§5) compares against `'*'` (0x2a), `'6'` (0x36), `'8'` (0x38),
and 0x1d (Enter). Direction-key state lives in four flag bytes:
`0x63ed0`/`0x63ed8` (vertical lanes) and `0x63ed3`/`0x63ed5` (horizontal:
`0x63ed5` = forward, `0x63ed3` = back).

---

## 5. Player action dispatcher — `FUN_0003272f` (batch1 §music_272f)

Called each tick with the active key table. `key` = `FUN_0003493e()`.

- If `P[0x30] == 1` (dead) → return.
- **Idle timeout**: `DAT_0006339c == 500` → action `0x28` (crouch), frame 0.
  (0x6339c is the idle-tick counter maintained in the frame loop §1.2/12.)
- **Action 0x66 (grab-hold)**: `*` or `6` → release: partner (at `P[0x4a]`)
  → idle, its hitstun/`0x4a`/`0x50` cleared; player → idle; `FUN_000348e2()`
  (combo reset). Actions 0x66/0x70 then fall to the grapple section
  (LAB_0x339c5, not re-decoded here — grab-finish transitions).
- **Action < 0x47** (basic/technique states), per action:
  - `0..1` (idle/walk start): if any direction flag set, speed =
    `*(0x52ffc + ((0x63360 % 4)*3 + P[0x25])*4)` — a **4-frame oscillating**
    per-frame speed table indexed by speed class `P[0x25]`;
    `0x63ed8` → `move_dy(+s)`, `0x63ed0` → `move_dy(-s)` (lane change);
    `0x63ed5` → `move_dx(+s)` + facing 0; `0x63ed3` → `move_dx(-s)` + facing 1;
    then action → 2 (walk). If `0x63ec1`/`0x63eb4` pressed **and**
    `P[0x27] != 0` → action `0x97` (guard/low technique), frame/hitstun reset,
    combo reset. `*`/`6`: if no horizontal direction held → action `0x83`
    (forward strike), else `0x84` (directed strike); latch `DAT_00062d58`.
  - `2` (walk): `'8'`/Enter with tech-set → `0xa3`; `*`/`6` → `0x85` (dash
    attack); forward key release at frame 0x13/0x2d → action 4 (jump/step-off).
  - `3` (dash): frame 8 with `'8'` + tech-set → `0x9c`; frame 0x11 → clear
    queue latch.
  - `0x15` (21, down/hitstun): frame < 0x32 with `*`/`6`, HP > 0, tech-set →
    `0x98` (getup counter); frame < 0x56 with attack → clear hitstun.
  - `0x16` (22): frame < 0x1e with attack + HP → `0x9b`; frame < 0x44 →
    clear hitstun.
  - `0x17..0x1e`: hitstun-cancel windows (frame 9..0x3a with attack →
    hitstun = 0).
  - `0x29` (41): attack + tech-set → `0x85`.
- **0x47..0x87**: attack + tech-set → `0x85`.
- **0x83..0x83**: (attack steps; rest of the 0x88+ branch continues at
  dump line 1761 — dash-attack/grab resolution, same patterns).

`move_dx`/`move_dy` are `FUN_0001545e`/`FUN_00015295` (batch2 §move_dx/dy).

---

## 6. Enemy AI

### 6.1 Per-enemy wrapper — `FUN_00031a8e` (batch1 §ai)

For one active enemy (called from the AI pass):

1. **Stuck check**: if the enemy is in a walk action and its screen
   position (`FUN_0001519f`/`0x151d5` = world − camera) has not changed in
   8 consecutive steps (`ent[0x103]`), re-pick a direction.
2. **Charge patience (action 3)**: `ent[0x6b]` (charge timer) increments
   while the player is within ±200 px; when it reaches the patience value —
   `100` frames if `*(T+0x0c + type*0x88) >= 0x1f`, else `1000` — the enemy
   commits to its attack action.
3. **Dispatch** on `*(T+0x0c + type*0x88)` (AI id, 25 cases):
   `0→0x17f13, 1→0x18e88, 2→0x19e48, 3→0x1aec8, 4→0x1bea8,
   5/9/10→0x1cf64 (with arg 2/0/1), 6→0x1df8b, 7→0x1f064, 8→0x20170,
   0xb→0x2119c, 0x15→0x221b8, 0x1f→0x2319c(2), 0x20→0x24bf9,
   0x21→0x265ac, 0x22→0x27bec, 0x23→0x29709, 0x24→0x2adf5, 0x25→0x2c838,
   0x26→0x2e2cc, 0x27→0x2fe1e`. Ids 0xc..0x14 and 0x16..0x1e have no AI body
   (passive/decorative types).

### 6.2 Dormant enemies & waking

Enemies spawned by `.stg` records start with `kind = 0` (dormant). The AI pass
skips them; `FUN_00012616` (action pass) runs their action state machine but
they only act when an AI dispatch wakes them: ai_t0 (`0x17f13`, batch2
§ai_t0) sets `kind = 1` when the player enters its **alert radius**
(≈65 px horizontal × 10 px vertical window) and the player is not in a
no-target action (0x15/0x16/0x17/0x18/0x1a/0x1d/0x1e/0x20), via target
search `FUN_00015a1f` (nearest valid target) → `ent[0x10f]` = target,
`ent[0x1f] = 0x0a` (alert).

### 6.3 ai_t0 — representative walker (`FUN_00017f13`)

- Speed: `ent[0x63]` (float accumulator) += `*(float*)(0x51d5c + idx*4)` where
  idx = `*(u16*)(T+0x3c + type*0x88)`; integral part feeds `move_dx`.
- Approach: move toward `ent[0x10f]` until in attack range, then start
  action 3 (charge) subject to §6.2 patience.
- Alert flags at `ent[0x8b]`; per-frame decision is state 0 (approach) vs
  0x0a (alert/attack).

(Other AI bodies share the same skeleton — target via `0x15a1f`, float speed
accumulator, charge patience — differing only in action ids and thresholds;
per-type flows documented in task-4.)

---

## 7. Per-frame entity update — `FUN_00012616` (batch1 §ent_upd)

Runs for every entity (action pass). Order of effects:

1. **Action table lookup**: `act = *(ent[0x123] + 4*action)`.
2. **Hitstun** (`ent[0x50] > 0`): freeze the frame counter, decrement
   hitstun; skip normal frame advance.
3. **Frame advance**: `frame++`; auto-advance chains: when
   `frame >= *(u16*)(act+0x17)` (byte `+0x17`) and action ∈
   {0xaa..0xad, 0xb4..0xb8, 0xbe..0xc7} → `action++` (continuation move).
   If `frame >= total (act+0x00)` → action ends: per-action exit rules
   (walk → idle, attack → idle/queued next from `ent[0x3e]` via
   `FUN_00031f5f` action queue).
4. **Airborne counter**: if `ent[0x16] > 0` and action has `0x6c` flag →
   decrement.
5. **Per-frame movement**: if `act[0x6c]`: `move_dx(act[0x71][frame])`,
   `move_dy(act[0x75][frame])`; special: actions 0x84/0x85 use a flat 2/3
   speed for their first 0x35 frames. Else `FUN_0001586b(ent, act[0x71][frame])`
   (no-vertical variant).
6. **Stage-exit detection** (player only, §7.6).
7. **Death bookkeeping** (any entity whose `0x30` becomes 1 during the
   tick): kill counter, boss check, wave-clear check — see §1.3 setters.

### 7.6 Stage-exit zone

While P is in walk (action 2) and
`0x5d0a0 <= P.x <= 0x5d0a8` and `0x5d0a4 <= P.y <= 0x5d0ac` (the exit rect
written at stage load; `0x5d0b0` holds the direction flag):
`0x63ed5` (forward key) with `0x5d0b0 == 0` → `0x63398 = 0`, `0x6336c = 2`;
`0x63ed3` (back key) with `0x5d0b0 != 0` → `0x63398 = 1`, `0x6336c = 2`.
The next-segment pointer is `*(S+0x34c)` (0 if the segment is the last — in
which case exit is ignored).
