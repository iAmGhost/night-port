# 10 — Core Gameplay: Frame Loop & Subsystems

Bottom-up reverse-engineering of `NIGHT.EXE`'s playable-stage core, via Ghidra
MCP decompilation. Decompilation dumps kept in `scratch/ghidra/`:

- `frame_loop_dump.c` — batch 1 (29 fns): frame loop, AI, action update,
  combat, input, camera triggers, wave reset, stage-end music, draw passes,
  player dispatcher, pre-loop fns, wave transition.
- `batch2_dump.c` — batch 2 (30 fns): movement primitives, hitbox test,
  knockdown timelines, action queue, key handlers, `.stg` load, player
  constructor, sample enemy AIs.
- `batch3_dump.c` — batch 3 (10 fns): the six per-frame pipeline wrappers and
  camera triggers.

Notation: `P` = player entity (`DAT_00053c7c`), `S` = current stage record
(`DAT_00063354`), `T` = entity type table base `0x53ea0`,
`camX`/`camY` = `DAT_000639dc`/`DAT_000639e0`. `ep` = `DAT_00063370`
(1..9). Every claim cites an image address.

---

## 1. Frame loop — `FUN_0003a829` (dump: `frame_loop_dump.c` §frame)

### 1.1 Entry / one-time init (before the loop)

1. `DAT_000639bc = 0` — kill counter for the current wave/segment.
2. `DAT_00053104 = 0x46` (70) — frame-throttle scale (§1.2 note).
3. Play stage CD track: `FUN_00010b1d(*(0x52da4 + S[0x40]*4 + (ep-1)*0x28))`.
4. Draw the "pre-stage" entity list (`DAT_000639cc`/`DAT_000639c8`) via
   `FUN_0003b9b7`.
5. `P[0x17] = 1` (idle). Save player stats: `0x63374 = P[0x2b]` (EXP),
   `0x63378 = P[0x27]` (tech-set), `0x6337c = P[0x26]` (style),
   `0x63380 = P[0x25]` (speed class), `0x63384 = P[0x28]` (char id) — these are
   restored on respawn (`0x376c8`).
6. Clear palette work buffer `DAT_000633b8` (768 B); upload DAC via
   `FUN_00040042` (VGA palette ports `0x3c8`/`0x3c9`).
7. Variant flag `*(0x52b84 + S[0x40] + (ep-1)*10)`: `1` → `P.x += ±0x78` (sign
   from `DAT_00063398`, the entry direction); `0` → walk-in
   `FUN_000381d7(buf, 1, 0, 0x78)` (120-tick slide from the screen edge).
8. FX update `FUN_0003ed6f`; full render `FUN_0003801b`; palette cross-fade
   `FUN_00040660(S+0x48, 0x32)` (50-frame blend to the stage palette).
9. `DAT_0006336c = 0` (state code), `DAT_00063360 = 0` (logic frame counter),
   `DAT_00063e28 = DAT_00063e24 = 0` (frame timer / sub-tick counter).
10. Combo-ring reset `FUN_000348e2`; clear key-state arrays `0x62c58` and
    `0x62cd8` (128 B each).

Both `FUN_0003801b` (pre-loop full render) and `FUN_000381d7` (walk-in)
busy-wait in the same tick units as the main loop
(`while (DAT_00063e24 < 0x1ea / DAT_00053104)`).

### 1.2 Per-tick pipeline (the `while(true)` body, in call order)

```
 1. boss intro (once): if boss (0x53c80) on screen
       (camY <= boss.y < camY+200, camX+0x32 <= boss.x <= camX+0x10e)
       -> FUN_0003a70e()
 2. pause: if key 0x63e89 held and P alive -> wait, FUN_00039c03() (pause
       menu); one choice -> return -1 (episode aborted)
 3. FUN_000322f5()      enemy AI pass        (batch3 §pipe_22f5, §6)
 4. FUN_0003209c()      action pass          (batch3 §pipe_209c, §7)
 5. FUN_000347e3()      key poll             (batch3 §keypoll_47e3, §4)
 6. FUN_0003272f(keyTbl) player action dispatcher  (§5)
       keyTbl = 0x62a9c when P[0x17] in {0x66,0x70} (grapple follow-up
       table), else 0x62a98
 7. FUN_000320f5()      combat pass          (batch3 §pipe_20f5, §8)
 8. FUN_0003d4e4(S)     camera gate / stage events  (§10.1)
 9. FUN_0003ed6f()      FX list update
10. FUN_00038098(buf)   z-sort + render + throttle  (batch3 §render_8098)
11. deferred SFX: if 0x63364 != 0: 0x63368 == 0 -> play
       FUN_0004185e(*(0x53d08 + 0x63364*4)), clear; else 0x63368--
12. 0x63360++; idle timer 0x6339c: (P[0x17] == 1) ? ++ : 0
13. palette ripple: if 0x63360 % 10 == 0 -> FUN_0003d606(S)  (§10.5)
14. state machine on DAT_0006336c  (§1.3)
15. FUN_00010d47(S)     stage-end music monitor  (§9.3)
```

The six wrappers are thin list drivers (batch3 dump):

| fn | addr | behavior |
|----|------|----------|
| `pipe_22f5` | `0x322f5` | `FUN_00032158()` pre-step; for each entity ≠ P with `kind != 2`: `FUN_00031a8e(ent)` |
| `pipe_209c` | `0x3209c` | for **all** entities incl. P: `FUN_00012616(ent)` |
| `keypoll_47e3` | `0x347e3` | only while `DAT_00052ff8 != 0` (CD music active); scan 128 key states at `0x62c58`; low-group → `in_lo` `0x34735`, high-group → `in_hi` `0x34635`; decrement repeat delay `0x62d5c` |
| `pipe_20f5` | `0x320f5` | `DAT_00063388 = 0` (last-hit victim); for all entities: `FUN_0001441e(ent)` |
| `stgev_3d4e4` | `0x3d4e4` | camera trigger gating (§10.1) |
| `render_8098` | `0x38098` | `FUN_00013aad` (z-sort); **if** `0x63e24 < 0x1ea / DAT_00053104`: background `FUN_0003ce0b`, entities `FUN_0003203d` (→ `FUN_00012efa` per entity), decorations `FUN_0003df96`, `FUN_0003b953`, boss bar `FUN_0001348f(0x53c84, buf, 0)` + `FUN_000137c1`, damaged-entity bar `FUN_0001348f(0x53c88, buf, 1)`, flip `FUN_000404c6`; then busy-wait until the throttle count is consumed |

Throttle note: the logic pipeline runs once per main-loop iteration; rendering
is gated by `0x63e24 < 0x1ea / DAT_00053104` (494/70 = 7 sub-ticks by default).
The absolute rate of `0x63e24` is measured against DOSBox in task-8; the port
uses a fixed logic tick and renders every tick, so only the *ratios* matter
(7-tick frame, 10-frame ripple, 500-frame crouch, 0x1ea music beat).

### 1.3 State code `DAT_0006336c` and return codes

Set by subsystems, consumed at the end of each tick (batch1 §frame tail):

| code | meaning — setter | handling in `0x3a829` |
|------|------------------|------------------------|
| 1 | player death — `FUN_00012616` when a dead (`0x30==1`) entity is `P` (§7.6) | `break`; if `DAT_00053100 == 0` (overall level unset) → `0x53100 = 3`, **return 0**; else `FUN_00038445()` (end/level screen): 0 → `0x53100 = 3`, **return 0**; non-zero → **return 2** |
| 2 | exit-zone trigger — `FUN_00012616` when P walks (action 2) inside exit rect `0x5d0a0..0x5d0ac` (§7.6) | walk-in `FUN_000381d7(buf, 1, 0, 0x50)` (80 ticks); `0x63398 = *(nextSeg+0x11)`; code = 0; `FUN_0003d244(S)` (§11.2) |
| 3 | boss killed — `FUN_00012616` when `0x53c80` dies | code = 0, **return 1** |
| 4 | all assigned kills done — `FUN_00012616` when kill counter `0x639bc` ≥ `*(0x52a1c + S[0x40]*4 + (ep-1)*0x28)` | `0x63398 = *(nextSeg+0x11)`; code = 0; `FUN_0003d244(S)` (§11.2) |

After state handling: if only the player remains (`DAT_00053c68 == 1`), zero
`S[0x32]` and `S[0x36]` (accumulated player dx/dy offsets used by the camera,
§10.4).

**Return values** (to caller `FUN_0003af18`): `0` = episode finished
(→ title/menu path), `1` = boss defeated (advance), `2` = stage completed via
the `0x38445` screen (continue), `-1` = aborted from the pause menu.

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

### 2.1 Type table `0x53e9c + i*0x88` (101 rows, loaded in `0x37b03` from `night%d.chr`)

**Verified loader asm** (10 sequential reads per row, `4192e`/`41c8f` from the
open stream): disk rows are **116 B packed, unaligned** at `nightN.chr+0`
(101 rows = 11716 B, then 234 B zero padding → 11950 B file). In memory the
same 10 fields land at explicit offsets in a 136 B (0x88) row:

| mem off | size | disk off | observed values (ep1) | notes |
|---------|------|----------|----------------------|-------|
| `+0x00` | u16 | 0  | 0 mostly; 40–50 some | **armor/DEF** — `player_mk`: `ent[+0x23] = f0` (u16), `ent[+0x56]` low byte |
| `+0x04` | u8  | 2  | 35–200 | **body HP** — `ent[+0x29] = u16(T+0x04)`; also reused as ai_t1 alt speed class (§6.4) |
| `+0x08` | u16 | 3  | 256/257/512/768/769 | bit flags (bits unassigned) |
| `+0x0c` | u16 | 5  | 0/250/500/2000 | unknown |
| `+0x10` | u8  | 7  | 0–250 | **AI class** — `FUN_00031a8e` 25-case dispatch; charge patience (≥0x1f → 1000 else 100 frames, §6.1); ai_t0 alert gate (0 → 65×10 px window, §6.4) |
| `+0x14` | u16 | 8  | multiples of 256 | **attack param** — mode 2/4/8 → `ent[+0x97]`/`ent[+0xab] = f5>>1` (§6.4) |
| `+0x18` | u16 | 10 | 180–300 | **patrol param** — mode 4/10 → `ent[+0xab] = f6>>1` or `f6 + rand%0x32` (§6.4) |
| `+0x1c..+0x7b` | u8[100] | 12 | 21–24 non-zero entries | **attack-chain table** — byte[variant(0..9)*10 + step(0..9)] = action id per attack step; read in state 0x0b as `0x53eb8[type*0x88 + ent[0x113]*10 + ent[0x11b]]` (§6.4) |
| `+0x80` | u16 | 112 | – | **speed class** — index into float table `0x51d5c` (enemy walk speed, §6.4) |
| `+0x84` | u16 | 114 | – | **attack-chain length** — state 0x0a: `ent[+0xab] = f9` (target 1) or `f9>>1` (target 2) |

Unwritten padding bytes per row: `+0x02..03, +0x05..07, +0x0b, +0x0e..0f, +0x12..13, +0x7c..7f, +0x82..83, +0x86..87`.
**Loader asm verified** (0x37ea4..0x37fd5): every field load is `IMUL i, 0x88` + base 0x53e9c — the table is ONE contiguous 0x88 row per type (earlier decompiler strides 0x22/0x44 were Ghidra artifacts). Type IDs are global 0..100; each episode's
`.stg` references a subset. Shared types (11, 31, 36, 37, 41, 42, 46) appear
in multiple episodes; rows 31/36/41/42 are byte-identical across episodes,
32/37/46 differ (per-episode palette/variant).

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

Key tables (corrected, task-5): ten 0x4b4-byte **heap tables**, pointers in two
4-byte pointer arrays: `0x62aa0..0x62ab3` = gun.key, gun1..4.key (char 0..4),
`0x62ab4..0x62ac7` = catch.key, catch1..4.key. Active pointers
`0x62a98 = *(0x62aa0 + char*4)`, `0x62a9c = *(0x62ab4 + char*4)` (set at
player spawn, batch1 §wavereset `0x377d5` region; default = table 0).
The frame loop passes `0x62a9c` instead of `0x62a98` while the player is in
grapple actions 0x66/0x70.

`.key` file format (fscanf-style text, parsed by `FUN_00032464` @ 0x32464,
called from the global resource loader `FUN_0003690b`):
```
count
per record: action(u16) n(u8) key1..keyn("%c") [newline] t1..tn(u32) x(u8) y(u8) z(u8)
```
In-memory record slots inside the 0x4b4 table: `+0x000` count u32;
`+0x004` n[i] (stride 1); `+0x68` keys ptr[i] (heap, stride 4);
`+0x1f8` timings ptr[i] (heap, stride 4); `+0x388` action[i] u16;
`+0x450` z[i]. Max 60 records (0x450+60 ≤ 0x4b4). File key chars map to
runtime input codes via the 256-byte table `0x5302c` (measured):
`a='8'(0x38) b=0x66(fwd) c=Enter(0x1d) d=0x50 e=0x69 f=0x64(back) l='K'(0x4b)
p=0x39 q=0x67 r='M'(0x4d) s='*'(0x2a) u='H'(0x48) w=0x68 z=0x2c`
— i.e. sequences are written in fighter-notation (f/b/u = forward/back/up,
a/s/c = the three attack keys). Per-record globals: `0x62ac8[action] = x`
(combo group), `0x62b90[action] = y` (combo step). Decoder + full record
catalog: `scripts/decode_key.py` (verified against all 10 files).

Notable sequences (from the data): dash `fff`(13/25f); a-chains a→aa→aaa→
aaaa (grp 2, steps 1-4); c-chains c→cc→ccc→cccc/cccac (grp 1, steps 1-5);
char supers: `q` (101, grp 100), `fc` (167), `bba` (161), `bbfua` (115),
`fbffc` (181, char 4), `bbwua` (87), `aaabfaua` (83, 8-input, char 4); grabs
103 `bc`, 104 `fc`, 112 `a`, 113 `aaaaa`, 114 `fa`. Timing arrays end in 0
(final input = next-frame latch); `win[0]` = max frames until first input.

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

### 5.1 Technique matcher (dump lines 2040-2193)

After the basic switch, the dispatcher walks the active table records
(`action[i]` at table `+0x388+2i`) and triggers on a matched input sequence:

1. Action 0x70-anim (partner state 5) + record action 0x70 → follow-up 0x71.
2. Current action in ready set `{0xb..0x13} ∪ {0x33..0x50} ∪ {0xa4,0xa6,0xa7,
   0x2f} ∪ {0xab..0xad} ∪ {0xb5..0xb8}` + record action ∈ {0x65, 0x30, 0x31}
   → direct trigger (grab-start / specials).
3. Current action in `0xb..0x14`: only record 0x99 may trigger, else reject
   SFX `FUN_000348e2`.
4. Sequence final timing (`timings[n-1]`) == 0 → reject SFX (no latch window).
5. Chain gates (last-tech id in `DAT_00062d58`):
   - current ∈ {1, 2, 0x66}: fire if `step[rec] < 2` or
     `step[last] == step[rec] - 1` (strict next step).
   - current == 0x70: up to 5 links (`P[0x31] < 5`), last-tech ≠ 0x71.
   - same group: `grp[last] == grp[rec]`, rec ≠ current, require
     `step[rec] > step[last]`, `step[last] == step[rec] - 1`, links < 5.

On trigger: `P[0x17] = action[i]`, `P[0x50] = P[0x11] = 0`, `P[0x31] = 0`,
`DAT_00062d58 = action[i]`, `DAT_00062d5c = timings[n-1]` (the latch window
used by the next-frame re-match). For 0x99/0x71 (targeted techniques) the
victim's action-data row (`P[0x123]` → `+4 + action*4`) supplies flags
`+0x69`/`+0x6a` → `DAT_00063364`/`DAT_00063368`.

---

## 6. Enemy AI

### 6.1 Per-enemy wrapper — `FUN_00031a8e` (batch1 §ai)

For one active enemy (called from the AI pass):

1. **Stuck check**: if the enemy is in a walk action and its screen
   position (`FUN_0001519f`/`0x151d5` = world − camera) has not changed in
   8 consecutive steps (`ent[0x103]`), re-pick a direction.
2. **Charge patience (action 3)**: `ent[0x6b]` (charge timer) increments
   while the player is within ±200 px; when it reaches the patience value —
   `100` frames if `*(T+0x10 + type*0x88) >= 0x1f` is false (i.e. AI class
   < 0x1f → 100, else 1000) — the enemy
   commits to its attack action.
3. **Dispatch** on `*(T+0x10 + type*0x88)` (AI class, 25 cases):
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

### 6.4 AI internal state machine (measured from ai_t0 `0x17f13` / ai_t1 `0x18e88`)

Per enemy: state word `ent[+0x1f..0x22]`, completed-mode word `ent[+0xaf..0xb2]`,
param `ent[+0xab]`, variant `ent[+0x113]`, step `ent[+0x11b]`, alert byte
`ent[+0x8b..0x8e]`, target id `ent[+0x10f]` (1|2).

**Prologue (every frame, both bodies)**
1. Speed class: ai_t0 → `T+0x80`; ai_t1 → `T+0x04` while state ∈
   [0x0a, 0x13], else `T+0x80` (also `T+0x80` when `ent[+0x6f]==1`).
   Float accumulator `ent[+0x63] += *(float*)(0x51d5c + class*4)`; this
   frame's step = rounded increment with carry fixup.
2. Targeting: player action ∉ {0x15,0x16,0x17,0x18,0x1a,0x1d,0x1e,0x20} →
   `FUN_00015a1f` (nearest valid target) → `ent[+0x10f] = 1|2`, state = 0x0a.
3. Gate: only continues when action ∈ {1, 2} and kind == 1.

**State 0 (decision)**
- First-alert (ai_t0 only): while `ent[+0x8b]==0`, an AI-class-0 type alerts
  only when the player is within |dX| ≤ 0x41 (65 px) and |dY| ≤ 10 (with
  `tick%5` hysteresis); other classes alert immediately → `ent[+0x8b]=1`.
- Mode select on completed mode `ent[+0xaf]`:
  - af ≠ 4: param = `T+0x18>>1` (af==10) or `T+0x18 + rand%0x32`; state = 4
    (patrol); variant `ent[+0x113] = rand%10`.
  - af == 4 (attack-capable): `ent[+0x8f]==1` → param = `T+0x14>>1`, state = 3.
    Else if screen X ∉ [0, 0x13f] → state = 7 (reposition). Else param =
    `T+0x14>>1`; on first frame after an attack (`ent[+0xc7]==1`) re-pick a
    target point: ai_t0 dir = rand%2, target X = playerX + rand within a
    per-dir window (`0x51c9c`/`0x51ca4`, 0x10-stride rows), target Y =
    playerY; ai_t1 dir ∈ {0..4} (biased rand%4), `ent[+0x6f] = dir<2`, target
    Y from `0x51ca0..0x51caa` rows; keep old point if too close (dX<10, dY<5).
    Point clamped to screen [0, 0x13f] and the walk band
    [S[+0x12]+1, S[+0x16]−1] (§16.4). Fire tests: dX<5 & dY<5 & player
    straddling → state = 0x14 (or 0x15, with point mirrored around the band
    midline); store point `ent[+0xcb/0xcf]` (dup `0xeb/0xef`) and clear alert.
    Otherwise face: state = 2 (walk) or 8, alert cleared.
  - af == 2: param = `T+0x14>>1`, state = 5. af == 8: same, state = 9.

**Movement states** — shared subs `sub(ent, step, playerSX, playerSY)`,
return non-zero on arrival: state 1 → `0x17218`, 2 → `0x16a60`, 3 → `0x16b8f`,
4 → `0x16fec` (returns 1: mode=4, state=0; 2: state=1, param=0x0a; 3:
mode=4, state=0x0c, param=0x1e), 5 → `0x16959`, 6 → `0x1730b`, 7 → `0x174a2`,
8 → `0x17530`, 9 → `0x17637`; on completion → `ent[+0xaf] = state`, state = 0.

**Attack chain**: state 0x0a (just targeted) — target 1: mode=0x0a, state =
0x0b, step=1, param = `T+0x84`; target 2: same, param = `T+0x84>>1`.
State 0x0b — if param ≥ 0: param--, action = 1 (walk countdown). Else if the
player is in range (|dX| window, dY < 6): action byte =
`0x53eb8[type*0x88 + variant*10 + step]` (§2.1 attack-chain table); 0 or
step > 9 → chain complete (mode = 0x0a, state = 0); else `ent[+0x17] = byte`
(attack action), frame counter `ent[+0x11..0x14] = 0`, step++. If the player
is out of range: 1/20 frames (rand%0x14==2) → mode = 4, state = 0 (chase),
else mode = 0x0a, state = 0. State 0x0c (recovery) — param (0x1e) counts
down; at 0 → action = 2, mode = 0x0a, state = 0.

**Special attacks**: states 0x14/0x15/0x16 → subs `0x1776f` / `0x17927` /
`0x17ca6`; on completion → mode = same state, state = 0, `ent[+0xc7] = 1`,
`ent[+0xc9] = 0`.

**Epilogue (all states)**: X step → `move_dx` (`0x1545e`); Y step → `move_dy2`
(`0x1586b`); facing `ent[+0x1b]` = 1 when playerSX < enemySX else 0.

### 6.5 Per-type differences (where the binary says so)

- The 25 AI bodies differ by (a) alert handling (ai_t0 has the first-alert
  window; ai_t1 does not), (b) speed-class selection (ai_t1 dual class),
  (c) target-point re-derivation (ai_t1 four-direction with Y offset rows),
  (d) the 0x1cf64 family (AI classes 5/9/10) takes an extra arg (2/0/1).
  All bodies run on the SAME 12 shared movement subs (§6.4) — per-body
  state tables are structurally identical; bodies not yet dumped (23 of 25)
  follow the same pattern — confirm per-body deltas in task-6 (runtime).
- Per-type behavior is otherwise data-driven entirely from the type row
  (§2.1): AI class (+0x10), params (+0x14/+0x18), speed (+0x80), attack
  chain table (+0x1c, 10 variants × 10 steps), chain length (+0x84).

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
- **Threshold source (verified)**: `exptab.dat` (RES idx 201, 186 B, text:
  6 lines × 5 ints + blank lines), loaded by `FUN_0003690b` (open @ 0x37251,
  name string @ 0x5156b; `fscanf("%d ")` loop, newline-skip between lines):
  col0 [300,1000,4000,5000,5000] col1 [600,2000,4000,5000,5000]
  col2 [2000,5000,5000,5000,5000] col3 [5000,5000,5000,5000,5000]
  col4/5 [1000 ×5]. Python check: `scripts/decode_key.py` (30 ints ✓).
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
- Spawn/event records: count `DAT_00063e14`; **each record 81 B**
  (`0x51`): `[u8 tag][80B payload]`, queued as a doubly-linked list
  (`FUN_0003e0a0` reader) and drained **per frame** by `FUN_0003ed6f` →
  tag dispatcher `FUN_0003e20f`. Verified tag set (all 9 episodes):

  | tag | name | payload (offsets into record) | effect |
  |-----|------|-------------------------------|--------|
  | 0 | pad | all zeros | no-op filler |
  | 1 | spawn | u8 type @+1, u8 side @+5, u16 X @+9, u16 Y @+0xd, u8 zone @+0x11, u8 posmode @+0x15 | **camera-triggered drip**: each frame, records whose trigger X is at/past the camera are activated; side 0 → `camX − 0x1e`, side 1 → `camX + 0x15e` (Y window `camY ≤ Y ≤ camY+200`); lazy-loads the type's `.spr`/`.act` when `0x639ec + type*4 == 0`. X=Y=0xFFFF → camera-relative special spawner (one per stage in ep6/7, type 36/37). |
  | 2 | camera | u8 subtype @+5 (5/6), u16 X @+9, u16 Y @+0xd | unlock left/right camera scroll bound at (X,Y) |
  | 3 | zone | u16 X0 @+9, Y0 @+0xd, X1 @+0x11, Y1 @+0x15 | **trigger zone**: overlapping kind<2 entity → action `0x20`, resets ent[5..8]/ent[0x11..0x14] |
  | 4 | rect | u16 X @+9, Y @+0xd, X2 @+0x11, Y2 @+0x15 | rect (semantics unconfirmed) |
  | 5 | misc | 4×u32 @+1,+5,+9,+0xd | decoration/misc (→ `0x5d0b4 + idx*5` subset) |
  | 6 | exit | u16 X @+9, Y @+0xd, X2 @+0x11, Y2 @+0x15, u32 @+0x19 | writes `0x5d0a0..0x5d0ac` (exit zone); **last tag6 in file order wins** |

- Stages **without tag6** (ep4/5/8/9) end by **kill-clear** (all spawned
  enemies defeated → wave-clear counter, §11.2/G2). ep8 is a single boss
  room (1 spawn + 1 camera record).
- Type definitions: 101 (0x65) rows → `0x53e9c` (§2.1), read from
  `night%d.chr` right after the `.stg`.

### 12.2 World geometry — `.wal` segments are world-space rects

Segment fields f0..f3 are **world coordinates** (left, top, right, bottom),
not sizes. Stages are multi-floor arenas: segment Y offsets step down
(ep9: 0 → 300 → 900 → 1200), gaps between rects are pits. Segment height is
always 200 (orient 0) or width 320 (orient 1, drop columns).
`f4`/`f5` ≈ 110–140 / 161–195 = walk-line / floor Y offsets inside the
segment (unconfirmed). Tail `n × u16` list semantics unconfirmed.

Verified by `scripts/decode_stg.py` (parses all files to exact EOF):

```
ep  segs recs  tags                     types used                       world X/Y        exit(last tag6)
 1     4   88  0:2 1:38 2:29 3:9 5:8 6:2  1,11,31,32,36,37,41,42,46      0..4320 x 0..400   X 2002..2052 Y 262..398
 2     4  184  0:1 1:86 2:57 3:6 5:32 6:2  2,12,13,51,52,56,57,61,62,63  0..7600 x 0..200   X 1856..1918 Y 84..196
 3     7  128  0:4 1:76 2:33 3:1 5:12 6:2  3,71,72,76,77,81,83           0..6640 x 0..200   X 3574..3735 Y 110..197
 4     2   42  1:21 2:5 3:2 5:14         4,53,54,58                     0..1640 x 0..500   kill-clear
 5     1   28  1:13 2:3 5:12             5,86,87                        0..1280 x 0..200   kill-clear
 6     4  203  1:103 2:63 5:34 6:3       6,11,31,32,33,36,37,41,42,43,46,47,48  0..7320 x 0..500  X 1550..1599 Y 120..197
 7     3  197  1:106 2:44 3:3 5:42 6:2   7,11,31,32,33,36,37,38,39,41,42,43,46,47,48,49  0..7560 x 0..500  X 2500..2556 Y 112..197
 8     1    2  1:1 2:1                   8                              0..640 x 0..200    kill-clear (boss room)
 9     8  207  1:132 2:25 3:4 5:46       9,71,72,73,76,77,78,79,81,82,83,84  0..12320 x 0..1400  kill-clear
```

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

## 14. Port gap analysis (final — task-6)

Port: `port/src/stage.c` (1011 ln), `game.c` (422 ln). Port model before fixes
(docs/05, 06, 07): single-stage walk, tag-1 position-based placement ("the world
is walked left→right; no wave/queue gating", stage.c:79), `level_end_x`
walk-past clear, fixed tick, simplified combat (stage.c:7 "playable
approximation"), direct key mapping.

Evidence key: **D** = disassembly address (this doc), **O** = dosbox-staging
observation (14 Oct / 15 Oct runs, §14.1, files in
`scratch/dosbox/captures/obs_*.png`).

| # | area | port behavior (stage.c / game.c) | original behavior | evidence | gap |
|---|------|----------------------------------|-------------------|----------|-----|
| G1 | spawning | all tag-1 spawns pre-placed at authored (x,y), sorted left→right; enemies walk in from their spot; no queue, no cap (stage.c:79,521,613) | tag-2 spawn queue + tag-3 trigger zones; per-type max-active cap table `0x528b4`; wave clear = kill counter vs `0x52a1c` | D: `FUN_00037b03`, §8.6; O: DOB-5 (staged arrivals: 2 street mobs, then boss+2 in alley) | major |
| G2 | segments | single stage, `level_end_x` clear | segment chain via `S+0x34c`; exit-rect trigger `0x5d0a0..b1` + direction flag | D: §7.6, §10.3; O: DOB-5 (ep1 stage has ≥2 distinct backgrounds) | major |
| G3 | camera | 320-px half-res window, `camx = player_x−160` clamp (stage.c:905) | 640-px full-res window `0x639dc`; dead zone [130,190]; 1/3-px/frame follow; clamps `S[0]`/`S[8]−0x140` | D: §10.1, `spawn_seg 0x3cad3`; O: DOB-5 (camera follows player, clamps at world edge) | major |
| G4 | combat | simplified hit/damage model (stage.c:7) | 6-slot hitbox windows, class table `0x51b60`, ×2/×3 multipliers, armor pools `0x56→0x23→0x29` | D: `FUN_0001441e`, §8 | major |
| G5 | enemy AI | proximity activation, walk-toward-player | 101-type table `0x53e9c`; AI-class dispatch on T+0x10 (`FUN_00031a8e`); full state machine (patrol/wake/charge/attack-chain), per-type attack/patrol params + 10×10 attack-chain table | D: §6.4, §2.1; O: DOB-5 (enemies idle at spawn, approach on sight) | major |
| G6 | player input | direct key mapping | 10 `.key` tables (`FUN_00032464`), 4-step press progression, 10-slot combo ring `0x63f14`, direction remap by facing, technique matcher `FUN_0003272f` | D: §4, §5; keymap `0x5302c` | medium |
| G7 | EXP/HUD | EXP *bar* + "EXP" label (stage.c:979-984); fixed +20 foe / +100 boss (stage.c:673-674) | **6-digit EXP number** ("EXP 000000") next to portrait + red HP bar; thresholds from `exptab.dat` → `0x53c94` (6×5 matrix), overall level `0x53100`, allocation on level-up | D: `0x37251` (exptab loader), §9.1; O: DOB-1 | major |
| G8 | enemy panel | absent (only "BOSS" text top-right, stage.c:986) | top-right panel: enemy **portrait + 2 status bars** per active foe | O: DOB-2 | medium |
| G9 | boss bar | "BOSS" text only, top-right | **"BOSS" label + long blue HP bar, bottom-left**; HP state `0x53c84/0x53c88`, totals `0x73/0x77` | D: §8.6; O: DOB-3 | medium |
| G10 | hit FX | none | orange/white **starburst impact sprite** at hit point (plus white slash marks on whiff) | O: DOB-4 | medium |
| G11 | speed feel | constant per-entity speed | per-type speed class (T+0x80 → float table `0x51d5c`); 4-frame oscillation table `0x52ffc` | D: §5.1 | medium (feel) |
| G12 | palette | static | ripple sweep every 10 frames (`FUN_0003d606`); 50-frame cross-fade on segment change | D: §10.5 | cosmetic |
| G13 | boot/clear flow | logo → opening → title → stage (game.c) | title (START GAME / EXIT TO DOS over `title1.pcx`) → **opening cinematic → stage**; after boss death: back to title, replaying opening on next start | O: DOB-6 (title a→stage b→boss c→title d→opening replay g→title h) | medium |
| G14 | idle behavior | n/a | 500-tick idle → crouch state 0x28 | D: §5 | minor |

(Old G9 "CD key poll gate `0x52ff8`" is port-N/A — no CD-DA dependency in the
port; dropped.)

### 14.1 Dosbox-staging observation log (task-6 evidence)

Method: dosbox-staging 0.83, conf `scratch/dosbox/dosbox-staging.conf`
(1280×960 @ 200,100), `NIGHT.EXE` via autoexec; window captured by CGWindowID
(`screencapture -x -l<WID>`) because full-display `-R` rects were unavailable
on the headless display. Keys sent via System Events key-codes; mouse-capture
released with Cmd+F10 before keying.

| id | capture (t = seconds after launch) | observation |
|----|------------------------------------|-------------|
| DOB-1 | `obs_b_t32.png` | Top-left HUD: player **portrait** + red **HP bar** + white **"EXP 000000"** 6-digit readout (EXP is a number, not a bar). |
| DOB-2 | `obs_b_t32.png` | Top-right: enemy **portrait + two status bars** panel (per-active-foe status). |
| DOB-3 | `obs_c_t46.png` | Bottom-left: **"BOSS"** block label + long **blue** HP bar. |
| DOB-4 | `obs_c_t46.png` | Large orange/white **starburst** impact FX at the hit point; white slash marks on the attacker's whiff arc. |
| DOB-5 | `obs_b_t32.png`, `obs_c_t46.png` | ep1 stage: bright **street** segment (2 dark-shirted mobs walk in from the right) → red **alley** segment (white-haired **boss** + 2 underlings, one armed). Two distinct backgrounds = segment chain (G2). Enemies stand idle at spawn until engaging (G5). |
| DOB-6 | `obs_a_t20…h_t155` (run7 sheet) | Flow: opening cinematic (letterboxed, Korean subs) → title (START GAME / EXIT TO DOS over red-sky `title1.pcx`, animated sun pulse) → stage; after boss fight → **title** again; next start replays the **opening** before the stage. |

Timing notes (60000 cycles/ms, single blind run): opening still playing at
t≈20 s; street gameplay by t≈32 s; boss fight by t≈46 s; title again by
≈62 s. The run was key-driven (Enter-mashing), so exact wave intervals are not
measurable from it — wave *ordering* (mobs → boss) and HUD facts above are the
durable observations.

---

## 15. Open questions (for tasks 2-8)

Resolved in task-2: stage/record/type-table layouts verified against all 9
episodes by `scripts/decode_stg.py` (9/9 parse to EOF, all tag-1 types
defined in the episode's `.chr`, spawn X within world bounds).
Resolved in task-3: movement/collision model, camera-relative X clamps,
segment walk-band (f4/f5 = walk-line/floor-line), trigger strips (§16).

1. Type-row fields: resolved in task-4 (§2.1) — armor, body HP, AI class,
   attack/patrol params, 10×10 attack-chain table, speed class, chain length;
   remaining unknowns: +0x08 bit flags, +0x0c.
2. `.wal` per-segment tail `n × u16` list: consumer unconfirmed (task-3
   closed f4/f5; the u16 tail remains).
3. 0xFFFF/0xFFFF tag1 record (ep6/7): exact runtime placement
   (camera-relative? random?).
4. Tag-4 rect semantics (ep3/4/7 only).
5. `0x63e24` tick rate vs DOSBox real time (task-8 measurement).
6. Per-slot damage/hitstun/sfx byte offsets in the action record (task-5,
   vs `scripts/decode_act.py`).
7. AI bodies `0x1cf64` (arg variants) and `0x2319c` (arg 2) — parameterized
   families; document per-type differences (task-4).

Task-6 note: menu flow after boss death is observed (DOB-6) but the exact
state sequence (title vs episode-select) between "boss death" and "opening
replay" is ambiguous from a single blind key-driven run; treat DOB-6's
ordering as low-confidence beyond "stage → title".

---

## 16. Movement & collision (task-3)

### 16.1 Entity coordinate axes

Entity (0x140 B) spatial fields: `+0x00` kind (u8; 0 = dormant enemy,
1 = active), `+0x01` **X** (i32, world), `+0x05` **lane/depth** (i32),
`+0x09` **Y** (i32, world vertical), `+0x11` wake-state counter, `+0x17`
action. Screen position = X − camX (`walkmap_x @ 0x1519f`).

### 16.2 `move_dx @ 0x1545e` — horizontal move + clamps

1. **Camera-relative X clamp** (camX = `DAT_000639dc`):
   - type 0 (player), not walking: `[camX+0xf, camX+0x131]` (15..305)
   - action 2 (walking, any kind): `[camX−100, camX+0x1a4]`
   - otherwise: `[camX+10, camX+0x136]` (10..310)
   Walk range 320 px = camera width; walking may leave the window
   (−100 / +420).
2. **Trigger strips** (`DAT_0005c8bc`, 20 B rows, count `DAT_00062a8c`;
   rows written from tag-3 records, §12): `[X0@0][Y0@4][X1@8][Y1@0xc]`.
   - Any entity whose Y overlaps `[Y0,Y1]` and whose X+dx would enter
     `[X0,X1]` bounces: moving right → X := X0, moving left → X := X1
     (wall edges).
   - Dormant enemies (kind 0, action 0x16, 0 ≤ ent[0x11] < 0x16, not the
     active player) that enter a strip are **woken**: action → 0x20,
     ent[5..8] = 0, ent[0x11..0x14] = 0, kind → 1, `0x63364 = 0x12`,
     `0x63368 = 1`.
   - Hitting the camera-edge clamp while in that dormant state also wakes
     with a random X jitter `±(rand%41−20)`.

### 16.3 `move_dy @ 0x15295` — lane move

`lane += d`; `lane < 0 → 0` (and ent[6..8] cleared). Death finalization:
when ent[0x30] == 1 (dying) and lane == 0 and ent[0x11] > 0x15 →
ent[0x25] = −1 (speed class dead), kind = 1; boss (`== 0x53c80`) spawns a
25 B FX record via `0x3b5c0`; **EXP**: `player[0x52] += ent[0x32]` (EXP
value at ent+0x32) into `DAT_00053c7c` (active player).

### 16.4 `move_dy2 @ 0x1586b` — world-Y move + walkmap clamp

`Y += d`, then:
1. Strip bounce: same 20 B rows; X inside `[X0,X1]` and Y+d entering
   `[Y0,Y1]` → clamp to the nearer edge (Y := Y1 moving up, Y := Y0
   moving down).
2. **Segment walk-band clamp**: lower = `S[0x12] + 0x639e0`, upper =
   `(S[0x10] == 0 ? S[0x16] : *(S+0x34c)[0x16]) + 0x639e0`. With
   `0x639e0 = S[0x04]` (segment world Y-top, set by `spawn_seg` §16.6)
   this is exactly **[y0+f4, y0+f5]** — the walk-line/floor-line band
   (vertical/drop segments take the *next* segment's f5).
   Out of range → hard clamp.

### 16.5 Per-frame movement source

From the action record (§7): `act[0x6c]` set → horizontal: `move_dx(
act[0x71][frame])` + `move_dy(act[0x75][frame])`; else the frame value
goes to `move_dy2` (vertical/ground move). Actions 0x84/0x85 (directed
dash attacks) use flat 2/3 speed for their first 0x35 frames.
Walk speed itself (action 0/1 → 2 transition) = float at
`0x52ffc + ((tick%4)*3 + P[0x25])*4` — 4-frame oscillation × 3 speed
classes (`P[0x25]`); table values are written during game init (not in
dumped functions) — measure via `--log` in task-6.

### 16.6 `spawn_seg @ 0x3cad3` — camera init per segment

```c
if (S[0x11] == 0)      camX = S[0x00];        // X-left anchor (first/floor segs)
camX = S[0x08] − 0x140;                        // X-right − 320 (drop segs)
0x639e0 = S[0x04];                             // segment world Y-top
0x639e4 = (char)S[0x10];                       // orient
```
Segment record: `[0x00 f0 X-left][0x04 f1 Y-top][0x08 f2 X-right]
[0x0c f3 Y-bottom][0x10 orient][0x11 anchor flag][0x12 f4 walk-line]
[0x16 f5 floor-line] … [0x34c next-seg ptr]`.

### 16.7 Walkmap spot-check (contract evidence)

`scripts/decode_stg.py` + `scratch/walkmap_spotcheck.txt` — **PASS**:
- ep1: 38/38 tag-1 spawns lie inside the `[y0+f4, y0+f5]` band of the
  floor covering their X; 5 tag-3 zones overlap a floor band, 4 sit in the
  vertical gap between floors (drop/pit triggers, e.g. X 1998 = pit at
  1600..2000); both exits overlap their target floor band.
- ep2: 86/86 spawns in-band; 2 band zones + 4 above-walk-line zones;
  exits in-band.
- Multi-floor disambiguation (ep1 X 3280..3600 overlaps upper floor
  seg1 and lower floor seg2) resolved by Y: 4 spawns assigned to the
  lower floor's band 320..395, all in-band.

## 17. Task-7/8: implemented in port & verified (2026-02-06)

Code changes in `port/src/stage.c` (+ generated `port/src/type_tab.h`,
union of `night1..9.chr`: `TYPE_ARMOR`/`TYPE_BODYHP`/`TYPE_SPDCLS`/
`TYPE_CHAINLEN`, 62/101 rows used):

| gap | change | verified by |
|-----|--------|-------------|
| G1 | `MAX_ACTIVE_FOES 3` cap in `activate_enemies` (0x528b4 cap; DOB-5 on-screen count) | `--log` `cam: ... active=3` |
| G2 | last tag-6 → exit zone `(x, y0..y1)`; stage clears when player enters it **after boss death**; no tag6 (ep4/5/8/9, confirmed in .stg) → kill-clear | `--log` `exit=yes (x=1001 y=131..199)`, `STAGE 1 CLEAR (exit zone ...)`, ep8 `BOSS defeated` → stage 9 |
| G3 | dead zone [65,95] world px + 1/6 world px/frame follow (=[130,190] / 1-3 px full-res, §10.3) | `--log` `cam: camx=... player=...` |
| G4 | per-type body HP + armor pool from .chr; damage hits armor first, remainder → body (§8.3) | `--log` `foe killed (ena-1.spr, armor=0)`; no-armor types verified in table |
| G7 | EXP = damage dealt (§8.6 `P[0xbb] += d`); 6-digit `EXP %06d` HUD readout (DOB-1); level thresholds `exptab.dat` col 0 → `EXP_TAB {300,1000,4000,5000,5000}` | `--log` `foe killed ... exp=416`, `LEVEL UP: exp=316 level=1 (threshold 300, exptab col0)`; `scratch/shots/t7_f01500_stage1.png` |
| G8 | top-right 2-bar enemy panel (HP + alert) for nearest foe (DOB-2) | `t7_f01500_stage1.png` |
| G9 | "BOSS" label + blue HP bar bottom-left (DOB-3) | `scratch/shots/t8b_f00150_stage1.png` |
| G10 | orange/white 8-ray starburst impact FX, 10 frames (DOB-4) | combat shots |
| G11 | 4-frame speed oscillation `SPD_OSC` (0x52ffc; values approximated — exact floats not captured) | code + `--log` cadence |

Also fixed a pre-existing port bug: enemies stayed in `S_ATK` forever
(`atk_t` never returned the state to `S_LIVE`) — visible once the demo
holds position to fight.

**Task-8 evidence** (`scratch/shots/`): pairs `pair_{stage,combat,bossbar}_{orig,port}.png`
(orig = `dosbox/captures/obs_4_stage.png` / `obs_5_combat.png`);
`--log` transcripts `task8_s1.log`, `task8_ep{2..9}.log`, `task8_ep8b.log`;
all 9 episodes load and run end-to-end (ep8 fully cleared via boss kill
into stage 9); `--selftest` ALL PASS.

Remaining gaps (deferred): G4 exact 6-slot hitbox windows + class table
`0x51b60` multipliers; G5 full 101-type AI state machine (port keeps
approach/attack heuristic); G6 `.key` combo matcher (port uses direct key
mapping). These are the next porting milestones.
