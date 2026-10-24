# 73. Stage-1 sprites: every entity draws from its own type's `.spr`, proved headlessly

Closes the playtest contract's `sprites-player` item ("Enemy sprites are main character's sprite. also player is
missing"). The code fix itself landed long ago (docs/49: the per-type name arrays hold pool offsets from DS 0x50000,
not string pointers, and were shifted by one entry, so type 31 loaded `gun.spr` instead of `ena-1.spr`). This
document is the *verification* the contract asked for: a headless check that enumerates the entity types present in
stage 1 and asserts each entity's sprite source is the type-correct asset, plus screenshots against the original's
own frames.

## The check: `scripts/playtest_check_entity_sprites.py`

It runs the port itself (`port/test/night --script scripts/playtest/route_episode1_anchored.txt`, SDL dummy video and
audio, `NIGHT_ENTDBG=1 NIGHT_ENTSPR=1`) and asserts, from what the game prints:

| Claim | Result |
|---|---|
| The player entity exists in stage 1 | `entmap ... player=1 type_act=0 type_spr=0 spr=gun.spr act=gun.act` |
| Enemies exist and are not the player | 2 non-player entities, `type_act=31 type_spr=31 spr=ena-1.spr act=ena-1.act` |
| Every entity resolves to a shipped per-type asset | all records have `type_act`/`type_spr` >= 0, real names |
| Those names exist among the extracted shipped resources | `ena-1.spr`, `gun.spr` present in `scratch/res` |
| No enemy shares the player's sprite struct | distinct struct pointers, distinct asset sets (`{gun.spr}` vs `{ena-1.spr}`) |
| Attribution is content-based, not just the port re-reading its own table | **80/80** composites' row-RLE streams are byte-present in the `.spr` file that *that entity's own type* names |
| The composited pixels are exactly what that asset says | **80/80** composites' blit output equals an independent decode of the same stream, 0 mismatched pixels (both blit modes: `mode=0` left-to-right, `mode=1` right-to-left/mirrored) |

Type resolution happens the way the game does it: `entity+0x123` holds the action-table value copied from
`g_dat_53818[type]` by the constructor, so matching it back against that table names the entity's type; the asset
names come from `kTypeSprNames` / `kTypeActNames`, the 101-entry arrays pinned by FNV hash against NIGHT.EXE
(`11ab4c98d4de6f12` / `b68eb82fdab264eb`, docs/63).

The suite side of the same claim already existed: `test/test_f1.c` "kind-1 record: ena-1.spr loaded as the type-31
sprite struct".

## What the original's own frames add

`/tmp/dbgauto/orig_full/` holds the DOSBox-automation capture of the original's episode-1 run (32 gameplay frames).
Comparing whole 320x200 frames (307,200 pixels), decoding the mode-13h payload as BGRX (docs/70):

    port shot 'stage_play'  best original frame 0177_gameplay_11:  93.86 % of pixels within +-3, MAE 3.62
                            (same pair read as RGB: 1.32 % -- the channel order is settled by this, docs/70)
    port shot 'playing'     best original frame 0170_gameplay_04:  90.88 % of pixels within +-3, MAE 6.04

The room backdrop, the HUD and the sprites all land inside +-3; the residual 6-9 % is where the two runs' entities
stand and which animation cell they are on, because the two capture runs were not input-synced.

A weaker, complementary probe (`scripts/playtest_spr_figure.py`) template-matches the port's own composite patch
(the bytes the blitter wrote, mapped through the room palette) against the original's frames: the enemy patch's best
placement scores meanSqErr 464.7 against 3677.8 averaged over every placement (8x better than chance), the player
patch 917.4 against 3598.7. Same art family at the same size, different animation cell -- which is what you expect
from runs that are not frame-locked, and it is reported as corroboration only.

Figure: `docs/manual-playtest/07-shots/sprites_player_enemy_vs_original.png` -- port stage-1 screenshot and the
original's episode-1 frame side by side, then the player composite (`gun.spr` cell k=9, 30x79) and the enemy
composite (`ena-1.spr` cell k=1, 42x73, drawn mirrored) exactly as the port's blitter wrote them, next to the
closest region in the original's frames. The player (dark jacket, jeans) and the enemies (bare torso, jeans) are
visibly distinct characters, which is the report item's complaint inverted.

## Harness defects found on the way (fixed)

The byte-level entity-sprite claims in docs/60/63/59 were made through `NIGHT_ENTSPR=1`, and that diagnostic had
three coverage bugs that made enemy art *unobservable* rather than wrong:

1. **Only the forward mirror arm dumped.** The dump sat inside arm B of `fn_012efa`, so every entity drawn through
   arm C (`flipw != 0`, the usual case for an enemy facing the other way) produced nothing. Extracted into
   `entspr_dump()` and called from both arms.
2. **One global budget starved every type but the player.** The player composites every frame and consumed all 120
   slots. Now a per-type quota (40 each) plus the global cap.
3. **Dumps were taken for composites the blitter never draws.** `fn_03bb42` returns immediately when
   `dest_off > 0x13f` and clips at the right/bottom edge, so 24 of the first run's 48 dumps were off-screen
   entities whose "output" was untouched background -- which is exactly what produced the first bogus
   "443 pixels wrong" result. The dump now requires the composite to lie wholly inside the 320x200 room, and the
   check separately counts what it skips.
4. The dump also now carries `type=`, `spr=`, `act=`, `mode=` and `tick=` so a composite can be attributed and
   decoded without guessing the blit mode; `ent_dump` emits a machine-readable `entmap` line per entity.

## Reproduce

    python3 scripts/playtest_check_entity_sprites.py          # runs the port itself, prints the assertions
    uv run --with numpy --with pillow python3 scripts/playtest_spr_figure.py   # the figure above

`make -C port test` ALL PASS, `make -C port selftest` ALL PASS with these changes (the diagnostics are env-gated, so
the suite is untouched by them).
