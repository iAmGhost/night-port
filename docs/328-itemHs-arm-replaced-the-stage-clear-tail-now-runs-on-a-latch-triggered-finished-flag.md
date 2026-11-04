# 328 — Item H's arm, replaced: the stage-clear tail now runs on a latch-triggered finished flag, and the request comes from the shipped's own instruction

Goal task-22.  Item H (docs/321) was measured on a run that poked `DAT_0006336c = 3`, which asserts the
conclusion instead of producing it.  This document replaces the arm, names every injection the new arm makes, and
re-runs the presentation census on it.  Two things had to be found first, and both are measurements.

## 1. What the shipped's own arm actually tests

`field_writers.py --disp 0x30` (canonical writer census, 523 bodies walked) — record `+0x30` is the *finished*
flag the request arms test:

| writer | offset | value |
|---|---|---|
| `fn_010efc` | 0x00010fe2 | 0 (constructor clears it) |
| `fn_0113e7` | 0x00011651, 0x00011662, 0x00011673 | **1** |
| `fn_011827` | 0x0001191a | **1** |
| `fn_013c4d` (the hit report) | 0x000141ca | **1** |
| `fn_01441e` (the row/reaction selector) | 0x00014913, 0x0001499f(**2**), 0x0001499f, 0x00014c1a, 0x00014cd5, 0x00014d06 | 1 / 2 |
| `fn_03272f` (the player's own stepper) | 0x00032c6e, 0x00032d4c | 2 |
| `fn_035c5c` | 0x00035cc3 | **1** |

and the arms that read it (docs/326 §3, read again here): 0x000129d1 `cmp [rec+0x30], 1` → 0x000129e2 `rec ==
DAT_00053c7c` → request 1; 0x000129fc `rec == DAT_00053c80` → **0x00012a04 request 3**; then 0x00012a24-0x00012a33
increments the kill counter for `byte[0] == 1`.  The port models the whole block at `f.c:27335-27346` (`if
(e[0x30] == 1) ... else if (e == g_latched_ent) g_dat_6336c = 3; ... if (e[0] == 1) g_dat_639bc++`) and consumes it
at `f.c:24343` (0x0003abca-0x0003abdd).

The point for the arm design: **every `+0x30 = 1` writer sits in the hit / reaction / spawn chain.**  Nothing in
the shipped tests HP and sets the flag.  So an HP injection cannot produce the arm.

## 2. The arms that do not work, measured

| run | injection | result |
|---|---|---|
| `/tmp/rig/h328b` (657-line keypress drive, 274 lctrl taps) | none | never left room 1: `DAT_00063354` = one value (0x18d6ec) over 1 910 samples, player x −30..603, stage 1 throughout |
| `/tmp/rig/h329` (`--warp-stage 1 --warp-column 3`) | guardian **HP = 1**, five times, "ok" | latch record non-null in 745/841 samples; that record read `hp = 1` in 301 sightings; **`+0x30` = 0 in 2 980/2 980 type-1 sightings**; its `+0x56` damage pool stayed 780 (the pool census is {0: 2 235, 780: 442, 772: 299, 778: 3}, so whatever absorbed those 8 damage points it was not the latched record); `DAT_639bc` 0; `DAT_6336c` 0 |
| `/tmp/rig/h329b` | guardian **HP = 0** | same shape: `+0x30` = 0 everywhere, pool constant 780, no kill, no request |
| `/tmp/rig/h329c` | `guardian 0x30 1` (the slot-search variant) | reported "ok" but fired nothing: `DAT_639bc` 0, `DAT_6336c` 0, `+0x30` = 0 in every sighting; `DAT_53c80` had already re-pointed (0x18e148→0x18e698 family) between the poke and the walk |

Keypress traversal is not a route (a stage's rooms are not reachable by walking on a fixed key timeline — the
player gets hit, dies, and the continue screen eats the rest of the script), and HP injection is not a route,
because the shipped's arm reads a flag the hit chain writes, not a health comparison.

## 3. The arm that does work: `NIGHT_RIG_GUARDIAN_KILL=1`, latch-triggered

`port/src/app/rig_sink.c:478-498` stamps `+0x30 = 1` on the record currently latched in `DAT_00053c80`, the
first times a latch exists — it is triggered by the latch, not by a clock, because `DAT_53c80` is cleared at room
init (0x00037b43) and re-latched only when a record whose type index equals `DAT_00063370` is constructed
(0x00010f1d-0x00010f2b).  The rig never writes `DAT_0006336c`.

Drive `scripts/rig/drives/ep1_st1_room3_guardiankill_state.txt` (48 lines) is state-anchored rather than timed:
`wait ds 0x63f50 >=1` (room installed) → `wait ds 0x53c80 >=1` (the guardian is latched) → one HP injection →
`wait ds 0x6336c >=1` (the shipped's own arm produced a request) → **one** Enter (advance the tail slide) →
`wait ds 0x63370 >=2` (the stage advanced) → **one** Esc held 900 ms (leave the level-up menu) → a final
`wait ds 0x63370 >=3` to keep the run alive across the tail.  No attack phase, no repeated Enter.

```sh
d=/tmp/rig/h330; rm -rf $d; mkdir -p $d
env SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_GUARDIAN_KILL=1 NIGHT_ROOMDBG=1 \
    NIGHT_DEATHTRACE=1 NIGHT_RIG_SCREEN_EVERY=8 stdbuf -o0 -e0 \
  timeout -k 5 330 ./port/test/night --res-dir ./NIGHT \
    --warp-stage 1 --warp-column 3 \
    --script scripts/rig/drives/ep1_st1_room3_guardiankill_state.txt \
    --frames 14000 --rig-hz 4 --rig-dump $d > $d/out.log 2>$d/err.log
```

Witness and DS census of that run:

```
guardian-kill: tick=98 latch rec=0x18e698 kind=1 type=1 st=0x3 x=4061 y=369 given +0x30 = 1
[request] seen #1 req=3 advances=0 room=+0x18e148 stage=1
DS 0x639bc  0 -> 1   sample 159  frame 1936
DS 0x63370  1 -> 2   sample 192  frame 2348
DS 0x53c80  0 -> 0x18e698   frame 1196      (the guardian, type 1 == stage 1, kind 1)
DS 0x63354  0 -> 0x18d6ec -> 0x18e148   frames 734, 1196   (warp install of column 3)
DS 0x6336c  transient: consumed between samples, final 0
```

Everything after the injected flag is the shipped's own code: `0x00012a04` writes request 3, `fn_03a829` returns
1 at 0x0003ac14-0x0003ac25, `fn_03af18` takes `if (r == 1) break` at 0x0003b1c1, the kill counter increments at
0x00012a24-0x00012a33, and the stage counter advances at 0x0003b52b ✓ **the stage-clear tail ran with the rig
having written one record byte and no request cell.**

## 4. Item H's census re-run on that arm (the green pair preserved)

Presentation segments from the dump's frame witnesses (each file is named by the geometry it was presented in):

| segment | frames | what it is |
|---|---|---|
| 640x480 ×52 | 2 .. 1191 | title / card phase |
| **320x200 ×38** | 1199 .. 2016 | the room phase, mode 13h ✓ |
| **640x480 ×6** | 2023 .. 2332 | the tail's end slides (`fn_03628e` loader at 0x0003b4c8) |
| **320x200 ×6** | 2339 .. 2373 | **back to 320x200 after the tail** ✓ the item-H fix: `call 0x405a7` at 0x0003b52b is a VGA mode-13h set (AX=0x13), modelled as such (docs/321 §1) |

Harness log agrees: `display mode 640x480` 17:13:27 → `display mode 320x200` 17:13:52 → `display mode 640x480`
17:14:10 → back to 320x200 ✓ the same card-in-640x480 / room-in-320x200 / return-after-each-show shape the shipped
shows (docs/321 §2).  Before the fix the tail's last transition stayed at 640x480 and the level-up menu presented
its 320x200 writes into a 640x480 window (64 000 bytes cover 100 of 480 rows); after it, the tail returns to
320x200 ✓ green pair intact, now armed by §3 rather than by a request poke.

## 5. Arm labels, stated per claim

| claim | arm that armed it | injection the rig made |
|---|---|---|
| item H red (docs/321, pre-fix) | `poke 0x6336c 3` | the request cell itself — superseded by §3 as the arm of record |
| item H green (this document) | finished flag on the latched record → shipped `0x00012a04` | one `memcpy` of `1` into `rec+0x30` of the `DAT_53c80` record, latch-triggered |
| item 2a live EXP census (docs/327) | keypress drive `ep1_kill3`, kills produced by the game's own hit path | none |
| item B's room advance (docs/326) | `poke 0x6336c 2` x 81 | the request cell, value 2 — the shipped's own request-2 arm is a walk request (0x00012e80-0x00012ee7) that no drive reproduces, so this one stays poke-armed and is labelled as such |

## 6. Open static question raised by this work (not a defect claim)

The port's request-1 branch reads `if (e == g_player_ent) g_dat_6336c = 1;` (`f.c:27345`), while the shipped
compares the walked record against `DAT_00053c7c` (0x000129e2-0x000129eb).  Those are the same claim only if
`DAT_00053c7c` holds the player record's address.  Runs where the player demonstrably died (`h329c`, `h329`) do
show a `req=1` sighting, which is consistent with either reading.  Establishing `DAT_00053c7c`'s identity is a
separate job; nothing here depends on it.
