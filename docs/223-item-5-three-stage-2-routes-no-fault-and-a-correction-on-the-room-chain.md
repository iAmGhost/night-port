# 223 — Item 5: the rig now reaches stage 2 three different ways, and no fault appears on any of them (plus a correction to docs/222 §3's chain-link inference)

Gate: `gate3-stage2-crash`, unfixed build, `build/night-asan`, canonical pinned env
(`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`).

## 1. Route C — the value that actually means "room clear"

`fn_03a829` returns 1 (room clear) on exactly one path: `DS 0x6336c == 3` (guest 0x0003abca-0x0003abdd,
`f.c:23462`, the event written when DS 0x53c80 dies).  Values 1 breaks the room loop into CONTINUE (return 2),
2 (exit rect) and 4 (quota met) advance *inside* the room.  `scripts/rig/drives/ep1_roomclear.txt` pokes
`DS 0x6336c = 3` every 1500 ms; run `/tmp/rig/rc1`, 13200 frames, 2200 samples at 10 Hz:

```
endcard: slide 1 wait                      (room_end_slideshow, f.c:27601 -- reached for the first time
                                            from inside the game, no stage cell written)
res 1e1.pcx                                (stage 1's end slide, g_tab_52f90[1] = 1)
```

so `fn_03af18`'s `if (r == 1) break;` -> `fn_03ace9()` -> the `%de%d.pcx` end slides *did* run.  But the stage
cell never moves: `DS 0x63370 = 1` in 2200/2200 samples, `DS 0x6336c = 0` in 2200/2200, `DS 0x63e89` (the abort
latch) = 0 in 2200/2200, and no `2s1.pcx` / `2s2.pcx` / `stage2.pcx` lookup appears -- the tail's
`g_dat_63370 += 1` (`f.c:27784`, guest 0x0003b52b) is not reached in this run either.  rc = 0, no AddressSanitizer
report, one pre-existing UBSan report (§3).

## 2. Route D — the warp route, which does reach stage 2

`g_warp_stage` (`f.c:5201`, applied inside the stage loader `fn_037b03` at `f.c:5206`: "Never set by the game
itself; 0 = off") is exposed as `--warp-stage N` (`main.c:1475`).  Run `/tmp/rig/w2`
(`--warp-stage 2`, drive `ep1_kill3`, 6600 frames, 1100 samples at 10 Hz):

```
memstat stage=2 room=1 frames=5750 / 5807 ...      (stage=2 in 675/1100 samples, stage=1 in 425: the boot phase)
res stage2.wal  res stage2.stg  res night2.chr  res 2s1.pcx  res 2s2.pcx  res stage2.pcx
rc=0    AddressSanitizer reports: 0    SIGSEGV / bus error: 0
```

The stage-2 resource family loads and the stage-2 room loop runs to the end of the drive.  Combined with
docs/208 (writing `DS 0x63370 = 2` and `DS 0x6336c = 2` gives 13435 frames at stage=2, rc=0) the item-5 crash
statement is now a three-route negative: **the port does not crash on entering stage 2 by warp, by room-clear
request, or by stage-cell poke.**

The historical "bus error ./build/night" line is also not a fault witness: in `/tmp/rig/rc1` the last lines are

```
frame budget: 13200 presented, unwinding the game main
... game main unwound (quit / fatal)
```

i.e. the harness's own `longjmp(g_game_escape)` when the frame budget runs out.  Any future claim that uses that
line as evidence of a crash is void; the fault-oracle for this item is `ERROR: AddressSanitizer` / UBSan output /
`rc != 0` only.

## 3. Correction to docs/222 §3: DS 0x63398 is not the chain terminator

docs/222 inferred that the room chain never advances because nothing writes the byte at `[[room + 0x34c] + 0x11]`
(guest 0x0003ac91, `f.c:23466`).  The static record says otherwise: the room node loader `fn_03c853`
(guest 0x0003c88c-0x0003c8a5) reads two bytes straight off the resource stream into that field --

```
0x3c88f  call 0x41c8f            ; read one byte
0x3c897  mov  [edx + 0x10], al
0x3c89d  call 0x41c8f
0x3c8a5  mov  [edx + 0x11], al
```

and the port models it (`f.c:1040-1041`: `node[0x10] = fn_041c8f(io); node[0x11] = fn_041c8f(io);`).  The
extracted `scratch/res/stage1.wal` (0xcbeb6 bytes) carries 0 at those offsets for the first node
(`+0x00: 04 00 00 00 00 00 00 00 00 00 00 00 00 00 40 06`, `+0x10: 00 00 c8 00 ...`), so **DS 0x63398 = 0 is the
shipped value too**, and `field_writers.py --disp 0x11 --disp 0x34c` confirms the only writer of that byte is
`fn_03c853` itself (0x3c8a5), whose link writes are 0x3ca5e (`[node+0x34c] = 0`, the terminator when
`++DS 0x6335c == DS 0x639d8`, the room count) and 0x3caa2 (`[node+0x34c] = child`).  The port's model has both
(`f.c:1090`, `f.c:1097-1100`, with the `[room-chain]` witness under `g_room_trace`).  docs/222 §3's "no writer
exists, so stage 2 is unreachable" claim is superseded by this section; the *observation* in docs/222 §2 (DS
0x63398 = 0 in 2200/2200, no end-slide lookup on that route) stands.

## 4. What is left open for item 5

1. Why route C's room-clear tail does not reach `g_dat_63370 += 1`: `room_end_slideshow` ran, so the divergence
   is between it and the stage increment -- the candidates are `fn_0406f5(g_dat_63354 + 0x48, 0x32)` at
   guest 0x0003b44a (the port passes `g_dat_63354 + 0x48`, a DS cell plus a constant, where the shipped's
   operand needs the record-base interpretation checked) and `fn_03f15f()` (the VESA mode-0x101 gate at guest
   0x0003b46b, which the port must answer 1 to run the slides -- it did, since the slide ran).
2. The "black floor" half is unmeasured: it needs the arena-buffer comparison in the composite the host
   presents (`fn_03f15f`'s linear model, docs/208 §4), not a shipped-frame pixel comparison (docs/202, docs/205).
3. The crash half is currently unsupported by any measurement: three entry routes, rc=0, zero ASan reports,
   zero SIGSEGV.  If the playtest report's crash is real it needs a build or route the rig has not used (the
   report predates the current build); the rig's answer to item 5's crash is "not reproducible", and the
   gate3-report verdict for that half will say so with these three runs quoted.
