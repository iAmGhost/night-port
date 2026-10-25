# 80. Draw census widened: every unobserved sprite family is *loaded* by the game, and the gap is position, not omission

Follow-up to docs/75 (which census-reportd 17 shipped `.spr` assets seen being drawn and left the rest "not observed").

## What was added

* `scripts/playtest/route_walk_fight.txt` -- per-stage dwell with the player walking (right, with periodic left
  reversals and up/down steps) and firing continuously (60 shots), plus shot markers, so more entity composites land
  wholly inside the 320x200 room and the player's attacks can land.
* `scripts/playtest_asset_census.py` now reports a second column for every run, from the game's own resource-lookup
  trace (`NIGHT_RESLOG=1` is now set for every census run): **which shipped `.spr` files the game actually loads during
  that run**, and which of those are never composited. It also prints a family matrix (drawn / loaded only / never
  referenced) and gained `walk*`, `setup` and `pausemenu` runs. `CENSUS_FRAMES=<n>` shortens the dwell.

    CENSUS_FRAMES=700 python3 scripts/playtest_asset_census.py walk stage pause pausemenu setup

## Result: walking and firing drew nothing new -- and that is the finding

| episode | composited (content-attributed) | loaded by the loader | loaded but never composited |
|---|---|---|---|
| 1 | ena-1, gage, gun (200 composites) | 20 | 17 -- incl. `boss1`, `enc-1/2`, `enb-1/2`, `end-1`, `dust`, `exp`, `face`, `st1s`, `system` |
| 2 | ene-1, ene-2, gage, gun (240) | 21 | 17 -- incl. `boss2`, `eng-1/2/3`, `enf-1/2`, `dust`, `mark`, `exp`, `face` |
| 3 | enh-1, enh-2, gage, gun (240) | 18 | 14 -- incl. `boss3-1`, `eni-1/2`, `enj-1/3`, `face`, `mark` |
| 4 | ene-4, enf-3, gage, gun (240) | 15 | 11 -- incl. `boss3-2`, `ene-3`, `st4s`, `system`, `dust`, `mark`, `exp` |
| 5 | enk-1, enk-2, gage, gun (240) | 14 | 10 -- incl. `boss3-3`, `st5s`, `system`, `system2`, `dust`, `mark`, `exp` |
| 6 | ena-1, ena-2, gage, gun (240) | 24 | 20 -- incl. `boss4`, `enc-1/2/3`, `enb-1/2`, `end-1/2`, `st6s` |
| 7 | ena-1, ena-2, enb-2, gage, gun (240) | 27 | 22 -- incl. `boss5`, `enc-1/2/3`, `enb-1/3/4`, `end-1..4` |
| 8 | **boss6**, gage, gun (200) | 12 | 9 -- incl. `st8s`, `system`, `system2`, `dust`, `mark`, `exp`, `face` |
| 9 | gage, gun (160) | 23 | 21 -- incl. `boss7`, `enh-1/2/3`, `eni-1..4`, `enj-1..4`, `st9s` |

So for **every** family that docs/75 listed as "not observed" -- `enc`, `eng`, `eni`, `enj`, `st1s..st9s`, `boss1..boss5`,
`boss7`, `dust`, `mark`, `exp`, `face`, `system` -- the shipped asset is *loaded by the game's own loader inside that
episode's run*. The census gap is therefore **not a port omission**: those entity types are resident but their entities
never appear inside the 320x200 viewport during a scripted dwell. Structural reasons, each evidenced:

1. **Rooms are not cleared by scripted play.** The stage's entity set spans many rooms; scripted play (walk + fire,
   60 shots, ~12 s of model time per stage) never finishes a room, so the player never reaches the rooms where the
   other families spawn. The census dumps only composites that lie wholly inside the room (docs/73), and stage 9's
   single entity never landed fully on screen (160 composites, 2 assets).
2. **Bosses spawn at the end of a stage.** `boss1..boss5`, `boss7` are loaded in every applicable episode but only
   `boss6` composites -- because episode 8 is the picture-show episode whose "boss" is on screen from the start
   (docs/52), which is exactly the case where no clearing is needed.
3. **The effect sprites need a hit.** `dust`, `mark`, `exp` load in all nine episodes and composite in none: they are
   spawned by hit/impact events, which the scripted dwell never produces (firing at nothing does not spawn them).
4. **`face` / `system` / `system2` belong to the setup/status screens.** The `setup` census run loads all ten shared
   screen assets (`dust, exp, face, gage, gun, mark, over, pause, system, system2`) but emits **0** traced composites --
   the same process-shape limitation already filed in docs/75 for the help/levelup/ending census runs (the menu screens
   need the route's own keyed interaction, which the census' short process does not reproduce). Their screens are
   reachable in-game (docs/26 drove the setup menu), so this is a harness limitation, not an unrendered screen.

Still filed as shipped-but-unreferenced content: `mboss3..mboss6` -- docs/64's lookup sweep never names them in any
episode, and the entity-type table maps them to types 14..17 which no stage record mentions.

## Census summary after the widening

Drawn and content-attributed (unchanged set, now with the loaded-side evidence): `ena`, `enb`, `ene`, `enf`, `enh`,
`enk` per episode, `boss6`, `gage`, `gun`, `pause`, `over`, `number1`. Loaded-but-not-composited: every other family in
the episodes that reference them (table above). Never referenced: `mboss3..mboss6`.

## Reproduce

    CENSUS_FRAMES=700 python3 -u scripts/playtest_asset_census.py walk        # all nine, with lookup trace
    python3 -u scripts/playtest_asset_census.py stage1                        # the docs/75 dwell
