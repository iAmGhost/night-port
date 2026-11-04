# 322 — The two axes of a map: stage and room, and what the warp flags actually select

Measured on HEAD `59ac016` + the warp rework in this task.  Runs: `/tmp/rig/w323b` (stage 3),
`/tmp/rig/w324b` (chain census), `/tmp/rig/w324c` (column 2 walk), `/tmp/rig/w324d` (column 3, the last room).

## 1. DAT_00063370 is a STAGE (map) counter, 1..9 — not an episode counter

Ghidra's name `g_dwEpisodeCounter` was a guess.  The shipped bytes:

| offset | instruction | meaning |
|---|---|---|
| 0x000365eb | `DAT_63370 = 1` | a game starts at stage 1 |
| 0x0003b52b | `DAT_63370 += 1` | the "next stage" step of the stage tail (`fn_03af18`'s tail, before `fn_0374ad` reloads) |
| 0x0003b1d4 | `DAT_63370 == 9` → the ending branch | **the game is through after stage 9** |
| 0x0003b578, 0x0003b3f1 | `DAT_63370 = 1` | game over / ending reset |

And the CD carries exactly nine maps, which are the names `fn_037b03` formats out of this very cell
(`stage%d.wal` 0x37b61-71, `st%ds.spr` 0x37c03-13, `stage%d.stg` 0x37c58-68):

```
stage1.wal … stage9.wal      stage1.stg … stage9.stg      st1s.spr … st9s.spr
```

The slide-show table `g_tab_52f2c[DAT_63370] = {0x16, 3, 2, 6, 0, 0, 3, 0, 1, 3}` matches the CD's
`<stage>s<card>.pcx` files card for card — 1→`1s1..1s3` (3), 2→`2s1..2s2` (2), 3→`3s1..3s6` (6),
6→`6s1..6s3` (3), 8→`8s1` (1), 9→`9s1..9s3` (3) — and stages 4, 5 and 7 have **neither** cards nor a table
row.  That closes the naming: a report about "episode 1, stage 3" is about **stage (map) 3**.

## 2. A stage has several rooms, and the shipped names a room by COLUMN, not by chain order

`fn_037b03` reads the room count at 0x37bda into `DAT_639d8` and builds the chain with `fn_03c853` at 0x37bf4.
Each node carries a camera window, which `fn_03cad3` (0x0003cad3) installs:

```
camX (DAT_639dc) = node[0x11] == 0 ? node[0] : node[8] - 0x140
camY (DAT_639e0) = node[4]
band (DAT_639e4) = node[0x10]              ; fn_03cc6b branches on it at 0x3cc8f / 0x3cd4d
next node        = node[+0x34c]            ; stored at 0x3caa2, NULL at a stage's last node
room column      = node[+0x40]             ; the axis the shipped's tables use
```

The room identity the shipped itself indexes is the **column**: `g_tab_52c3c` (the seat Y),
`g_tab_52be0` ("rooms entered by walking in") and `g_tab_52da4` (the room's CD track) are all addressed as
`column + (stage-1)*10`, i.e. up to **ten rooms per stage**.  Chain order is a different thing.

Stage 1's chain, read whole by the new `[chain]` census (`/tmp/rig/w324b/out.log`):

```
[chain] fn_037b03: stage=1 DAT_639d8=4 rooms, chain order below (node# | col | camera window | flags)
[chain]   node#1  col=0  camX=0      camY=0     band=0 side=0 next=next node
[chain]   node#2  col=1  camX=3280   camY=0     band=1 side=0 next=next node
[chain]   node#3  col=2  camX=3600   camY=200   band=0 side=1 next=next node
[chain]   node#4  col=3  camX=4000   camY=200   band=0 side=1 next=NULL (terminator: fn_03d244 takes g_dat_63350 at 0x3d2a2)
```

**This is the correction the playtester forced.**  My first cut called the chain ordinal a "room"
(`--warp-room 3` → node #3 → column 2), which lands *inside* what a player calls the second room: with
`camX = 3600` the seat `camX - 0x1e` sits at x=3570, i.e. 30 px left of that node's own window.  Node
ordinal and room column are separate axes and must not be conflated (docs/300: naming a cell after a guess
invites misreading it).

Per the playtester's hint, the dialogue-skip report is about **the last section of the episode, the room
that holds the boss (extra HP bar at middle-bottom)** → for stage 1 that is **column 3 = node #4**, the
chain terminator.

## 3. What the warp flags do now (`port/src/f/f.c`, `port/src/f/f.h`, `port/src/app/main.c`)

* `--warp-stage S` forces `DAT_63370 = S` in `fn_037b03`, i.e. selects map S (1..9).
  `--warp-episode N` is kept only as a deprecated alias so docs/52-era scripts still run.
* `--warp-node N` selects the N-th **chain node**; `--warp-room` is its alias (kept so old invocations do
  not silently change meaning — the alias is labelled in the basis line).
* `--warp-column C` selects the room by the shipped's own axis: the first node whose `+0x40` equals C.
  This is the flag to use for a playtest report about a "room".
* `[chain]` census (`NIGHT_CHAINDBG=1`, or any warp run) prints the chain **after `fn_03c853` has linked
  it** — the first version ran it before the link pass and printed one node holding raw `.wal` bytes in its
  window fields (`camX=1508896`), which is a broken instrument, not a finding (docs/289).
* Basis statement, printed before anything is measured:
  `warp basis: DAT_63370 = 1 -> stage1.wal / stage1.stg / st1s.spr room basis: --warp-column … node basis: --warp-node …`
* Warp implies entering the game (`warp_entry_key`): the shipped's start path is a chain of key waits (logo
  hold, opening `.fli`, title wipe + drain, then the menu), each phase consuming one press, so a confirm
  pulse every 500 ms lands on whichever phase reads the latch, and the pulse **stops** the instant
  `fn_03272f` (the player action resolver) starts running — so a warp run never injects a key into gameplay
  and never parks on the title screen.  A warp run also implies `--skip-intro` (docs/309 records that this
  is a basis switch: a warp run is a basis of its own).  Applied only when no `--script` pins a timeline.
* The install mirrors the shipped's room-entry stores: `DAT_63354 = node` (0x3ccb5 / 0x3cd6f / 0x3d29b),
  `DAT_63358` mirrored only past stage 1 (0x3ccc1 gates 0x3cccf, 0x3cd7b gates 0x3cd89), `fn_03cad3(node)`
  (0x3d2b6), `DAT_63398 = node[0x11]` (**0x37633-3b**, the shipped's own "this node is now current" store —
  without it the warp seats the player on the wrong side), then `fn_03d244`'s seating at 0x3d2c1-0x3d38a
  (`+1 = camX ± 0x1e/0x15e`, `+0x78` when `g_tab_52be0` marks the column, `+9 = camY + g_tab_52c3c[col]`,
  `+5 = 0`, `+0x17 = 1`, `+0x11 = 0`, `+0x1b = DAT_63398`) and the room's track `fn_10b1d(g_tab_52da4[col])`
  (0x3d3bb-d7).  It deliberately does **not** call `fn_038f3d` (0x3d2ac): a warp basis carries no
  level-up-menu state and never stands in for a real room clear.
* The seating is applied **once per stage load at the room-loop entry** `fn_03a829`, not inside
  `fn_037b03`: the loader builds the chain before the player record exists (the witness prints
  `player=(0,0)` there), and both `fn_0374ad` (0x3761f) and the hero spawn re-seat `DAT_63354` on the chain
  head afterwards.  It is one-shot, so the shipped's own camera-driven room switch (0x3ccb5 / 0x3cd6f) still
  runs afterwards.

## 4. Measurements

| run | basis | result |
|---|---|---|
| `/tmp/rig/w323b` | `--warp-stage 3`, no script | prints the basis line, requests exactly `stage3.wal`, `stage3.stg`, `st3s.spr` (2 `st3s.spr` lookups), `memstat stage=3 room=1` in 22/27 samples |
| `/tmp/rig/w324b` | `--warp-stage 1 --warp-column 2` | chain census above; `--warp-column 2` resolves to node #3 |
| `/tmp/rig/w324c` | same + `scripts/rig/drives/ep1_st1_room3_walk.txt` (entry chain + 24 Right bursts, no pokes) | install line `stage=1 node#3 col=2 … DAT_63398=1 player=(3950,372) state=1` |
| `/tmp/rig/w324d` | `--warp-stage 1 --warp-column 3` (**the last room, the boss room**) | install `stage=1 node#4 col=3 camX=4000 camY=200 DAT_63398=1 player=(4350,372) state=1`; 121 samples, max 8 records in one sample, records of type 1 with hp 2000 in 84 samples (type 1 = the stage's guardian, the record whose type index matches the stage counter — the record `DAT_00053c80` latches), type 2 records in 15 samples |

So the playtester's "last section with the boss" is reachable and does carry the guardian record
(hp 2000) — enemies are not missing on this basis, which contradicts the earlier
"warp loads wrong background, enemies do not appear" observation in a useful way: that observation belongs
to the **old** `--warp-stage M` chain-hop basis, which installed a mid-chain camera window without seating
the player (see §2).

## 5. Retractions this forces

* **docs/312 §0a** — "all previous stage-2 measurements are episode-2 measurements" is wrong: `stage2.wal`
  *is* stage 2 of the same linear 1..9 sequence.  What was wrong was the *label*, not the asset.
* **docs/317 (item E), docs/313 §8's item-E row, docs/320 §4's item-4 leg** — those censuses counted the
  `<stage>s<card>.pcx` slide show reached from the title/attract path (`--warp-episode 3` + a drive of
  Enter presses), never the overlay inside a stage room.  They are withdrawn as evidence about the
  dialogue-skip report, not merely re-labelled: the report is about an **overlay graphic drawn over the live
  stage in the last room of stage 1**, which is a different routine and a different gate.
* **"6 gates, 0 no-press"** is not a verdict on item E.  It is a verdict on the picture-card gate on map 3's
  opening slide show, which nobody reported as broken.

## 6. Open defect found while building this basis (blocks item E)

On `/tmp/rig/w324d` the player record at heap offset 3831256 (`n = 352` bytes) stops being a record at
sample 58 of 103:

```
tick 22  tag 0  x 3970       y 172          st 0          hp 1000
tick 58  tag 1  x -1414812923 y -1397969915 st 757935365  hp 43690   (offset unchanged: overwritten in place)
```

The span is never re-pointed and the fields become host-pointer values — the same class docs/314 recorded
(`refused (outside arena)`, `fn_03272f`'s `+0x30 == 1` gate then refusing the walking body).  This is a port
defect, it reproduces on a clean stage-1 basis, and until it is fixed any measurement taken after it is a
measurement of a corrupted record.  It is the next thing to root-cause, ahead of the dialogue gate itself.
