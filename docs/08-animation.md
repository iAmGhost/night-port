# 08 — Character animation (record→action mapping, player + enemies)

Goal: give the player (건 / Gun) and every enemy a full **idle / walk / punch /
kick / jump / hit** animation set, driven by the current action, and verified
with an input-driven headless render + `--log` frame indices.

## 1. Data & binary grounding

- **`.act` format** (byte-exact, all 55 files): loader `FUN_00010010 →
  FUN_00010181` (record reader). Per record: `[u32 n]` + 109-byte header +
  `[u16 × n] frame[i]` (sprite-index to show at beat `i`) + 2×`[u32 × n]`
  per-beat params. See `docs/03-formats.md` §3.
- **Animation driver** (binary): the gameplay update loop `FUN_000376c8` and the
  xrefs of the per-entity record array `&DAT_00053818[entity]`
  (`FUN_00038f3d`, `FUN_00010efc`, `FUN_00044663`) advance the beat and select
  a record by the character's current action.
- **Controls** (README.TXT): arrows = move (up/down = change lane), SPACE =
  punch, ALT = kick, SHIFT = jump, Enter = run. These define the *actions* the
  animation set must cover.

The record→action **semantics** (which record is the walk vs the punch) are
data-driven; they are established empirically below by decoding `gun.act` and
sheet-rendering each record, then matching each cycle to a control/pose.

## 2. `gun.act` record→action mapping (verified)

`gun.act` = 192 records. Decoding each record (Python, byte-exact vs the C
`act_decode`) and rendering its frames on a grid identifies the action cycles:

| action | rec | n (beats) | sprite frame@mid | pose |
|--------|-----|-----------|------------------|------|
| idle   | 0   | 1         | 0                | standing, guard |
| hit    | 10  | 27        | 32               | recoil → fall to ground |
| punch  | 52  | 17        | 217              | arm extended forward |
| jump   | 57  | 25        | 151              | tucked / airborne |
| walk   | 41  | 42        | 57               | leg stride (42-frame cycle) |
| kick   | 66  | 55        | 88               | front leg reach |

Evidence: `scratch/shots/animtest.png` renders all six poses side-by-side from
the exact records above (labels show `rec`/`frame`). The `--animtest` log:

```
animtest: idle  rec=0  n=1  beat=0  frame=0
animtest: walk  rec=41 n=42 beat=21 frame=57
animtest: punch rec=52 n=17 beat=8  frame=217
animtest: kick  rec=66 n=55 beat=27 frame=88
animtest: jump  rec=57 n=25 beat=12 frame=151
animtest: hit   rec=10 n=27 beat=13 frame=32
```

## 3. Enemy record selection (heuristic)

Enemy `.act` files vary (51–64 records) with no shared fixed layout, so each
foe's records are chosen at load (`pick_anim_records`, `stage.c`) by shape:

- **walk** — first record (i≥1) with `20 ≤ n ≤ 150` (a substantial cycle).
- **atk**  — the *widest* middle-frame sprite among records after walk with
  `8 ≤ n ≤ 80` and aspect `w/h < 1.4` (an extended limb = punch/kick).
- **hit**  — first record with `n ≥ 10` and middle-frame aspect `w/h > 1.7`
  (a fallen/lying pose).

Per-character results (Python, `anim_map2.py`): `ena walk=2 atk=43 hit=5`,
`enb walk=2 atk=11 hit=5`, `enc walk=2 atk=42 hit=21`, `boss1 walk=1 atk=3
hit=5`, … — each foe gets a distinct walk, an extended-limb attack, and a fall.

## 4. Implementation (`port/src/stage.c`)

- `AnimAction { A_IDLE,A_WALK,A_PUNCH,A_KICK,A_JUMP,A_HIT }`; `Ent.action` holds
  the current action; `EntState` gains `S_JUMP`.
- `anim_rec_for(e, action)` → record index (player = fixed gun map; enemies =
  cached heuristic `e_walk/e_atk/e_hit`). `anim_frame(e)` → `(beat/spd) % n`
  (spd = 2 for walk, 3 else) → the sprite index to draw. `draw_sprite` uses it.
- **Player** update sets `action` from state + movement: `S_HIT→hit`,
  `S_ATK→(kick?kick:punch)`, `S_JUMP→jump`, else `moving?walk:idle`. Jump
  (`K_JUMP`/SHIFT) starts a 16-frame `S_JUMP`.
- **Enemies** (`enemy_ai`) set `action`: `S_HIT→hit`, `S_ATK→punch`, moving
  `→walk`, else `idle`.
- **`--log`** prints the action on change and the frame index every 8 frames:
  `anim: player walk -> rec 41 (n=42)`, `anim: player punch frame=217`, …
- **`--animtest`** (`stage_animtest`, `game.c` `--animtest`) renders the six
  player poses as a sheet and logs `rec`/`n`/`beat`/`frame` per action.

## 5. Evidence

- `scratch/shots/animtest.png` — six distinct player poses (idle/walk/punch/
  kick/jump/hit), each labelled with its record + frame index.
- `scratch/shots/anim_live_f0680.png` — live play: player + an enemy in dynamic
  combat poses on the subway floor (both animating, not static).
- `--log` (live, ep1 demo): 109 `anim: player …` lines cycling
  walk(rec 41)/punch(rec 52)/kick(rec 66)/hit(rec 10)/idle(rec 0) with per-frame
  sprite indices.
