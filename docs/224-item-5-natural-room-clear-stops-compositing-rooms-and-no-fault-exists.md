# 224 — Item 5: the port reaches stage 2 on a natural room clear, then never composites a room again — and no fault exists on any stage-entry route, HEAD or the frozen baseline

Gate: `gate3-stage2-crash`, unfixed build (`port/test/night-asan`, HEAD `18b36f1`) and the frozen baseline
build from `574c636` (checked out to `/tmp/base574`; `port/test/night`, `port/test/night-asan` are tracked in
git, so the baseline commit's own binaries are what ran).

## 1. Warp sweep, all eight remaining episodes, ASan + UBSan, HEAD

`--warp-stage N` with the pinned clock (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), drive
`ep1_kill3`, dumps `/tmp/rig/sweep/sN` (3000 frames) and `/tmp/rig/sweep2/sN` (6600 frames, for the stages
whose slides need >50 s):

| stage | rc | AddressSanitizer / SIGSEGV | stage-live `memstat` lines |
|---|---|---|---|
| 2 | 0 | none | 59 (`sweep2/s2`), 0 in the 50 s run |
| 3 | 0 | none | 77 |
| 4 | 0 | none | 11 (`sweep/s4`) |
| 5 | 0 | none | 10 |
| 6 | 0 | none | 77 |
| 7 | 0 | none | 10 |
| 8 | 0 | none | 18 (`sweep2/s8`) |
| 9 | 0 | none | 11 |

Zero `ERROR: AddressSanitizer`, zero SIGSEGV, zero bus error in all eight logs.  The only sanitizer output
anywhere is §6.  This also closes the hole docs/52 left open: stages 8 and 9 were "not yet run" there, and
docs/52's *stage 5 SIGSEGV after ~926 frames* no longer reproduces.

## 2. The frozen baseline does not fault either

`/tmp/base574` = the gate's frozen build `574c636`.

* `--warp-stage 2`, drive `ep1_kill3`, 6600 frames: `night` rc=0, 77 `memstat stage=2` lines, `night-asan` rc=0,
  no ASan report.
* drive `ep1_roomclear2.txt` (§3), 13200 frames: `night-asan` rc=0, `night` rc=124 (the 250 s wall timeout fired
  during the harness's own shutdown flood, after `frames presented: 13200` and `frames=13199` -- a slow exit, not
  a fault); stage 2 reached at `memstat stage=2 room=1 frames=4768`.

## 3. Route D: the natural room clear, taken all the way

docs/223's route C stopped at the end-of-stage slide show because the drive stopped pressing keys.
`scripts/rig/drives/ep1_roomclear2.txt` keeps poking DS 0x6336c = 3 (the value that makes `fn_03a829` return 1 at
guest 0x0003abca-0x0003abdd) **and** keeps pressing Enter so the slide show can advance.  Run `/tmp/rig/rc2`
(13200 frames, 2179 samples at 10 Hz, `NIGHT_SLIDEDBG=1`):

```
slide: card 1/3 wait ... 3/3 wait        (the stage-1 start slide show, 1s1/1s2/1s3.pcx)
endcard: slide 1 wait                    (room_end_slideshow, stage 1's 1e1.pcx)
res 1e1.pcx  res help1.pcx  res help8.pcx  res levelup2.pcx     (fn_038f3d(1), the level-up slideshow that only
                                                                  stage completion calls: levelup2.pcx = variant 1)
DS 0x63370: 1 -> 2 at t=94316 ms         (g_dat_63370 += 1 at guest 0x0003b52b executed with no stage-cell poke,
no ERROR: AddressSanitizer, no SIGSEGV, rc=0               no warp flag)
```

Stage 2 is reached from inside the game.  What happens next is the finding:

```
mode segments in /tmp/rig/rc2/frames.bin
  (640,480)  920 entries   t =   0.0 -   7.7 s   changed 102
  (320,200) 3587 entries   t =   7.7 -  37.5 s   changed 680      stage-1 gameplay canvas
  (640,480) 5181 entries   t =  37.6 -  80.7 s   changed 1136
  (320,200) 1502 entries   t =  80.8 -  93.3 s   changed 461      stage-1 gameplay again
  (640,480)15305 entries   t =  93.3 - 220.0 s   changed 1771     <-- the rest of the run, 127 s, 7600 frames
```

**Zero 320x200 entries exist after t = 93.3 s**: after the transition the host never presents a room composite
again.  The last three witness frames (t = 215.7, 216.4, 217.1 s) have byte-identical per-row ink profiles
(`ink = 161, 426, 368, 438, 405, 481, 467, 454, 487, 468, 437, 482` at rows 0..440), modal colour
`(255,247,247)` -- a static card screen.  The stage-2 resource family is never requested: `markers.jsonl`
(165 distinct names) contains `1s1/1s2/1s3.pcx`, `stage1.pcx`, `stage1.stg`, `stage1.wal`, `night1.chr`,
`1e1.pcx`, `help1.pcx`, `help8.pcx`, `levelup2.pcx`, and **no** `stage2.wal`, `stage2.stg`, `night2.chr`,
`2s1.pcx`, `2s2.pcx` or `stage2.pcx` -- while the warp route (§1) loads all six of those.

So, measured without any human playtest input: at stage-2 entry through the game's own room-clear path the port
advances `DS 0x63370`, runs the end slides and the level-up slideshow, and then **stops compositing rooms
entirely**: 127 s and 7600 frames of card canvas with no floor band drawn and no stage-2 data loaded.  That is
the observable content of "the floor renders black when entering stage 2" -- not a black pixel band (the stuck
canvas is near-white, `black px = 0` in the lower half) but the absence of any rendered floor at all.  docs/205's
independent mode census ("port 4 segments, never re-enters arena after 73 s") is the same defect seen on the
canonical route.

## 4. The frozen baseline reproduces the stuck screen

Same drive on `/tmp/base574/port/test/night` (`/tmp/rig/base_rc2`):

```
(640,480)  381 entries   t =   0.0 -   3.7 s   changed 102
(320,200) 4625 entries   t =   3.7 -  47.0 s   changed 682
(640,480) 2580 entries   t =  47.0 -  71.0 s   changed 310
(320,200) 2058 entries   t =  71.0 -  90.3 s   changed 792
(640,480)19137 entries   t =  90.3 - 266.0 s   changed 1803   <-- same 640x480 lock after stage 2 (frame 4768)
```

The defect is present on the unfixed baseline and on HEAD: it is not an artifact of the rig drive, since the
drive is identical in both runs and the warp route with the same drive reaches stage-2 gameplay (§1, `/tmp/rig/w2`:
stage 2 live with `stage2.wal`, `stage2.stg`, `night2.chr`, `2s1.pcx`, `2s2.pcx`, `stage2.pcx` all loaded and
bottom-half ink 22270/22515 px, 100/100 rows with ink, no black rows).

The difference between the two routes is what localizes the fault: warp enters stage 2 through the *stage
loader* `fn_037b03` (`g_warp_stage` writes `g_dat_63370` at `f.c:5206` before the family is read); the room-clear
tail reaches stage 2 *after* the level-up slideshow `fn_038f3d(1)` (guest 0x0003b54c) and `fn_0374ad()`, and in
that order the loader never asks for the stage-2 files and the presentation never returns to the room canvas.
`fn_038f3d` is the same routine item 4 is localized in (docs/217: the card wait at `f.c:25750` /
`fn_035f1b("help8.pcx", 1)` -- and `help8.pcx` appears in this run's lookups) ✓ two items, one blocking site.

## 5. The crash half is a documented negative

Five route/build combinations, all rc=0 with no AddressSanitizer report, no SIGSEGV, no bus error:
HEAD warp (8 stages), HEAD stage-cell poke (docs/208, 13435 frames at stage=2), HEAD quota-request poke
(docs/222), HEAD room clear (§3), baseline warp and baseline room clear (§2).  The historical
`bus error ./test/night` line is the harness's `longjmp(g_game_escape)` unwind (`frame budget: ... unwinding the
game main`, docs/223 §2).  Item 5's crash claim has no reproducible counterpart in the current or frozen build;
the playtest report predates both.

## 6. The only sanitizer output in any of these runs, and its counterfactual

```
src/f/f.c:7360:9: runtime error: store to misaligned address 0x000100e52bb9 for type 'char *',
which requires 8 byte alignment
```

Site: `fn_04589e` (`f.c:7334-7362`), the token array tail store `toks[cnt1 - 1] = NULL;` -- the port widens the
shipped's `(count+1)*4` pointer array to `(count+1)*sizeof(char*)` and places it at `tail = buf + nbytes`, where
`nbytes` is the length of the tokenized string, so the 8-byte store is unaligned whenever `nbytes` is.  It
appears identically in all nine warp stages, in both baseline runs, and in runs that never touch a stage
transition (docs/222's quota-poke route) -- which is the counterfactual that separates it from item 5: it is a
standing host-side UB in the resource/`fn_04589e` path, not a stage-2 fault.  It is the natural first fix in the
fix phase (same family as the earlier `mem_copy` unaligned fix) and is not one of the six reported items.

## 7. Verdict for item 5

* Screen half: **DETECTED-AND-LOCALIZED** (rig-only).  At stage-2 entry through the game's own room-clear path the
  port presents no room composite for the remaining 127 s / 7600 frames on HEAD and 176 s / 8400 frames on the
  frozen baseline, and never requests the stage-2 resource family.  Localized to `fn_03af18`'s tail order --
  `fn_038f3d(1)` (0x0003b54c) then `fn_0374ad()` -- by the contrast with the warp route, which enters the same
  stage and does load the family and does render a floor (bottom-half ink 22270/22515 px).
* Crash half: **NOT REPRODUCIBLE**, five route/build combinations, with the fault oracle defined as
  AddressSanitizer/UBSan output or `rc != 0` and the only such output attributed to §6.
