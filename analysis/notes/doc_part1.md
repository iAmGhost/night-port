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
