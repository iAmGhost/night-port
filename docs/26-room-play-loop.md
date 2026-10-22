# The room play loop (0x03a829)

`fn_0003a829` is the loop that plays one stage room. It is called once per room by `fn_0003af18` and it is
where every other gameplay function is hung: the frame pipeline, the exit decision and the CONTINUE tail.

## Frame pipeline (per iteration)

```
fn_03a70e   once per room, when the DAT_00053c80 entity is inside the camera band (CG slide-show)
fn_039c03   if the abort latch DAT_00063e89 is set (title menu); 0 -> the room loop aborts with -1
fn_0322f5   per-type AI sweep
fn_03209c   entity action step (fn_012616 over the entity list)
fn_0347e3   keyboard dispatch
fn_03272f   the player action chain, from g_dat_62a98, or g_dat_62a9c while the player is in 0x66/0x70
fn_0320f5   the actor-vs-actor hit test
fn_03d4e4   camera policy
fn_03ed6f   script command pump
fn_038098   sort, draw, frame pacing
fn_03d606   every tenth frame (the room pair table)
fn_010d47   the CD refresh tick
```

## Exit decision -- the event request DAT_0006336c

| Value | Producer | Effect |
|---|---|---|
| 1 | the player died (fn_012616) | leave the loop -> CONTINUE tail: no credits -> 0, else `fn_038445` -> 0 or 2 |
| 3 | a DAT_00053c80 entity died | return 1 (the room restarts) |
| 2 | the exit rectangle (fn_012616, script opcode 6 box) | 0x50-step wipe, adopt the child room's orientation byte, `fn_03d244` loads the next room |
| 4 | the room kill quota was met | the same reload without the wipe |

The opening clears DAT_0006336c, so a test cannot pre-arm the exit: the exit has to be caused from inside the
frame (a dying entity walked by `fn_012616`).

## What a fixture has to satisfy

The pipeline draws, so the room node must look like a real one:

* `room[+0x1a]` background pitch (0x140), `room[+0x348]` = the background plane (SCREEN_BASE),
* `room[+0x34c]` = the child room as a **host pointer** (widened in this port; `fn_03cc6b` dereferences it),
  `child[+0x11]` its orientation byte,
* `room[+0x3c]` = 0 keeps `fn_03d606` idle,
* the entity list (`DAT_00053c70/74/68`), `DAT_00053c84`, `DAT_00053c88`, `DAT_000639cc` and the script list
  `DAT_00063e18/1c/20` must be empty or they are walked as freed nodes,
* `DAT_00053104` is set to 0x46 by the opening, which makes the draw budget `0x1ea / 0x46 = 10` frames, so
  `DAT_00063e24` decides whether a frame actually draws.

`test_fn_03a829` in `test/test_f1.c` implements exactly this (11 checks) and is `#if 0`-ed out until the full
stage fixture from the integration tasks exists; re-enabling it is a one-line change plus the `main()` call.
