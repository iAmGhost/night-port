# 326 — Item B's runtime half: the rig does leave room 1 of stage 2 on arm 2, and the boss gate cannot be reached there

Goal task-20.  Item B (docs/manual-playtest/06-2026-11-03.md): stage 2 could be passed **without defeating its
boss**.  Contract: leave room 1 of stage 2 and reach the boss gate on a `DAT_0006336c = 2` arm (never 3), or
record the item as not-reproducible with the exact playtester run asked for; and make no stage-clear claim from
a warp install.  Basis: `--warp-stage 2` (map 2, `DAT_00063370 = 2`) with
`scripts/rig/drives/ep1_walk_st2.txt`, `NIGHT_ROOMDBG=1`, `--frames 9000 --rig-hz 2` → `/tmp/rig/b326`.

## 1. The rig leaves room 1 of stage 2, on arm 2, and nothing advances the stage

The request witness at `fn_03a829`'s loop bottom (`src/f/f.c:24323`) fired twice in the run:

```
[request] seen #1 req=2 advances=0 room=+0x18d6ec stage=2
[request] seen #2 req=1 advances=1 room=+0x18da4c stage=2
```

so the room cell moved `+0x18d6ec -> +0x18da4c` (`g_room_advances = 1`) while **`DAT_00063370` stayed 2** ✓ a
**room** advance inside map 2, not a stage clear ✓ The camera witness agrees: 55 `[vgate]` visits, all
`stage=2 rooms=4`, `inc_would_fire=0`, `camX` 0 -> 2354.

Arm accounting, from the script census: `poke ds 0x6336c = 2` x **81**, `= 0` x 1, **never 3** ✓ — the arm the
contract allows.

## 2. The boss gate cannot be reached on this basis

| measured on `/tmp/rig/b326` | value |
|---|---|
| `show: entry` (fn_03a70e arming, docs/323) | **0** |
| `reveal:` calls (fn_03a5f7, docs/325) | **0** |
| records constructed, type-index census | `{0: 36, 1: 72}` — **type 2 (the stage-2 guardian type): 0** |

`fn_010efc` latches `DAT_00053c80` only when a *constructed* record's type index equals `DAT_00063370`
(docs/311).  Nothing with type 2 is ever constructed on the stage-2 map basis, so the guardian latch stays null:
the boss record, its extra HP bar, and everything gated on the latch are unreachable from a map-warp install.
This is the same wall docs/317, docs/325 §5 and item C all ran into.

## 3. What the shipped's request cell actually means (this is what settles the item)

`DAT_0006336c` has ten writers and one read site.  Sets (`imm_scan`): **1** at 0x000129ed, **3** at 0x00012a04,
**4** at 0x00012a5d, **2** at 0x00012eba and 0x00012ee7.  Clears: 0x0003a976, 0x0003a9dc, 0x0003abb7,
0x0003ac14, 0x0003ac67.  Read only from **0x0003abae**, the bottom of `fn_03a829`'s room loop.  Read the two
arms the report is about:

**Request 1 (0x000129d1-0x000129ed)** — not a kill test:
```
0x129d1  mov  al, [rec + 0x30]        ; the record's own finished flag (same byte fn_03272f gates on)
0x129d9  cmp  eax, 1 / jne 0x12a7b    ; only a finished record gets here
0x129e2  mov  eax, [rec] / cmp eax, [0x53c7c] / jne 0x129f9
0x129ed  mov  [0x6336c], 1            ; the record latched at DAT_53c7c finished
0x129fc  cmp  eax, [0x53c80] / jne 0x12a10
0x12a04  mov  [0x6336c], 3            ; the guardian at DAT_53c80 finished  <-- the only stage-ending arm
```
`fn_03a829` takes request 1 as `break` (leave the room loop) and request 3 as the stage tail (docs/311): the
guardian arm is what advances a stage, and the *room* arm does not need a kill at all.

**Request 2 (0x00012e80-0x00012ee7)** — a *walk* request, no kill term anywhere in it:
```
0x12e83  mov  eax, [rec + 9] / cmp eax, [0x5d0ac] / jle 0x12e90   ; player Y against the room's upper limit
0x12e90  cmp  [0x5d0b0], 0 / jne 0x12ec6                          ; which vertical arm
0x12e99  cmp  byte [0x63ed5], 0 / je 0x12ec4                      ; Down held
0x12ea2  mov  eax, [0x63354] / cmp [eax + 0x34c], 0 / je 0x12eba  ; chain terminator: skip the orientation store
0x12eb0  mov  [0x63398], 0                                        ; seat on side 0
0x12eba  mov  [0x6336c], 2
0x12ec6  cmp  byte [0x63ed3], 0 / je 0x12ef1                      ; Up held
0x12edd  mov  [0x63398], 1                                        ; seat on side 1
0x12ee7  mov  [0x6336c], 2
```
Request 2 is "walk Up/Down at the room's vertical limit, and swap which side the player is seated on"
(`DAT_63398`), with the terminator node (`node[+0x34c] == 0`) deliberately skipping the orientation store ✓
which is exactly the shape the rig observed on `/tmp/rig/b326`.

## 4. Verdict on item B

- **"A room can be left without killing anything" is shipped behaviour.**  Request 2 (a walk) and request 1 (the
  `DAT_53c7c` latch finishing) both advance rooms with no kill term, and the port reproduces them: the rig
  advanced a room of map 2 on arm 2 with zero kills.
- **"A stage can be passed without the boss" needs request 3** (0x00012a04), i.e. the record latched at
  `DAT_00053c80` reaching `+0x30 == 1`.  The port cannot reach it on stage 2 at all, because no record with
  type index 2 is ever constructed there (§2) — the direction of the defect is the *opposite* of the report: in
  the port stage 2 cannot be cleared even by killing something.
- The reported moment is therefore recorded **not reproducible on any basis the rig can build**, and no
  stage-clear claim is made from a warp install (`DAT_00063370` stayed 2 throughout).

**Specific run requested from the playtester** (hand playthrough, no warp): kill stage 1's guardian in map 1's
last room so the game advances to stage 2 on its own; in stage 2 note which room the boss stands in and whether
its extra HP bar is visible on entry; then walk to the room's vertical limit (hold Up at the top edge, Down at
the bottom edge) and record whether the game moves to the next **room** or jumps to the next **stage**, and
whether any kill happened first.  Those two questions — room advance vs stage advance, and whether the boss
record exists in the room at all — are what the rig cannot decide from a map-warp install.

## 5. What would close it in code

The latch condition (`fn_010efc` 0x10f53 area: construct a record whose type index equals `DAT_00063370`) needs
a stage-2 spawn path that actually builds a type-2 record.  The census says the constructed records on map 2
carry type 0 or 1 (36 and 72 sightings), so either the spawn table's type column is being read with the wrong
stride/index for stage 2, or the guardian is meant to arrive through a path the port has not modelled.  That is
spawn-queue work, and it is the same wall that keeps item C's stage-2 clear and item F's stage-3 reveal out of
reach (docs/312 §"spawn queue", docs/325 §5).
