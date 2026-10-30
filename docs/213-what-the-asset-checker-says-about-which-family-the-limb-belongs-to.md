# 213 — Item 6, second probe: the player's own action record resolves to `gun.act` / `gun.spr`, and one enumerated entity resolves to no shipped asset at all

`scripts/playtest_check_entity_sprites.py /tmp/rig/le4.log` (one run of `ep1_leftedge` with `NIGHT_ENTDBG=1
NIGHT_ENTSPR=1`) enumerates the entity list and names the shipped asset each record resolves to:

```
ent idx=0 player=1 type_act=0  type_spr=0  spr=gun.spr   act=gun.act
ent idx=1 player=0 type_act=31 type_spr=31 spr=ena-1.spr act=ena-1.act
ent idx=2 player=0 type_act=31 type_spr=31 spr=ena-1.spr act=ena-1.act
... (every pass in the stage) ...
FAIL: every enumerated entity resolves to a shipped per-type act/spr asset:
      ('3','0','-1','-1','?','?','00224b5c','0x0') ...
```

Two facts that narrow item 6:

1. **The player record's action pointer lands on the type-0 family (`gun.act` / `gun.spr`)** in every pass of the
   stage.  So the sprite the entity drawer (`fn_012efa`) places for the player *is* the weapon family, and a limb/arm
   that belongs to the player's attack animation can only come from the `.act` chain that family walks (docs/110's
   chain-target census), not from a second player "body" family.  `night%d.chr` is not a sprite sheet at all -- it is
   the entity-type table (`f.c:5121`, 0x88-stride `DS 0x53e9c`), so there is no separate character-sheet path that
   could be doing the mirroring.

2. **The asset checker's own gate fails on an unresolved record**: one enumerated entity has
   `type_act = -1, type_spr = -1` (its `+0x123` action pointer matches no `g_dat_53818` slot; its `+0x127` struct
   reads `00224b5c`).  That record is the only entity in the run that no shipped `.act`/`.spr` family explains, and it
   is exactly the kind of record a spawned attack/limb part would be.  Whether that record is the limb is not
   established yet -- its identity needs the `.act` chain census, not the type table.

Combined with docs/212 (facing bit correct at the left edge, both mirror arms and the selector byte-faithful, blitter
clip divergence narrow), the remaining candidate for "the limb is drawn on the right side at the left edge" is a
chain-resolved sprite whose `X` (`+0x137` array) / `W` (`+0x12b` array) pair puts it on the other side of the entity
when the mirror arm is taken -- so the next measurement is the chain: enumerate the targets the player's
attack-action chain resolves (`NIGHT_ACTTRACE` census, docs/110), and for each target dump `spr`, `W`, `H`, `X`, `Y`
and the arm taken, then compare those four arrays against the same values decoded from the shipped `.act`/`.spr`
bytes.
