# 232 — Item 1 root cause located: the CD-age counter is aged by the shipped's *timer IRQ* (`fn_03ee88`), which the port fires from its frame pump

Fix task: `fix-music`.  Red evidence was committed first (docs/183 `94a74ab`, docs/184 `743d3be`, docs/185
`ec382a7`, gate report docs/226 `4ea273d`).  This document closes docs/184 §4's remaining question and names the
one measurement the fix needs.

## 1. What ages the counter, in the shipped bytes

`DS 0x63e38` is the counter `fn_010d47` bounds a track by (`q = DS 0x63e38 / 490`, `if (q <= DS 0x53c48) return;`,
then `DS 0x63e38 = 0` and `fn_10b1d(cd_track_for_room(DS 0x63354))`).  Its **only** shipped writer is inside
0x0003ee88-0x0003ef1c, which is not a game routine at all:

```
0x3ee88  pushal ; push ds/es/fs/gs ; mov ebp,esp ; sub esp,0 ; cld
0x3ee98  call 0x42310 ; call 0x4231b                       ; seg/irq bookkeeping
0x3eea2  inc  dword [0x63e28]                              ;   /7  -> inc [0x63f08]   (0x3eeb1)
0x3eebe  inc  dword [0x63e24]
0x3eec4  inc  dword [0x63e3c]                              ;   /0x11 -> inc [0x63e40] (0x3eed9)
0x3eeca  inc  dword [0x63e38]                              ; <-- THE ONLY WRITER of the CD-age counter
0x3eee6  [0x63e30] += [0x63e2c]                            ;   /0xffff -> inc [0x63e34]
0x3ef0d  sti ; 0x3ef12  mov al,0x20 ; out 0x20,al          ; re-enable IRQs, EOI the PIC
```

`pushal` + segment pushes + `sti` + `out 0x20, 0x20` is an installed **hardware timer interrupt handler**.  So in
the shipped game the CD age advances on the clock, asynchronously, *whatever the game loop is doing* -- including
while the title/menu screen is up, while a card waits for a key, and while a slide-show holds a picture.

## 2. What the port does instead

The handler is ported (`fn_03ee88`, `port/src/f/f.c:2535-2548`, every increment cited to its guest address ✓
faithful) -- but nothing calls it asynchronously.  Its only call sites are in the rig's frame pump:

```c
/* port/src/app/main.c */
1408:   ... fn_03ee88(); g_timer_calls++;        /* pump bookkeeping */
1412:   while (calls--) { fn_03ee88(); g_timer_calls++; }   /* `calls` = NIGHT_RIG_STEP_CALLS (665) */
```

so the counter advances **665 times per pumped game step** and **not at all while a screen blocks inside one
step**.  Both halves of docs/183 §2-§3 follow from that:

* the port held one track's 42 s request for 166.7 s and never selected `0x9ddf`, because
  `DS 0x63e38 / 490 > DS 0x53c48` only becomes true after ~31 s of *port ticking*, and 42 s of game time is worth
  a different number of port ticks depending on how many steps the pump actually ran;
* the cell dump shows a **65 s hole** (no samples between 40 s and 105 s on the title route) -- the same window in
  which the title/menu screen blocks in one game step, during which *nothing* ages the counter and nothing calls
  `fn_010d47`.

`main.c` already knows about the distinction (`main.c:10` "fn_03ee88 is the game's installed timer handler",
`main.c:145` a `no_timer` mode, `f.c:15633` noting that a build without the handler changes no counter).

## 3. Closed as causes: the title request, the table, the arithmetic

* The shipped title screen *does* request its own track, and the port models it:
  0x0003a43d-42 `mov eax, 9; call 0x10b1d` -> `fn_10b1d(9)` at `f.c:15826` (cited `0x3a43d-42`) ✓ and the shipped
  title teardown 0x0003a578 `call 0x10dde` -> `fn_00010dde()` at `f.c:15851` ✓ (fn_03a224's whole body requests
  only 9; a scan of its 879 bytes finds no other `fn_10b1d` and no `fn_010d47`).
* `fn_010d47`'s arithmetic and its room-table lookup are faithful, and the 100-entry room table is byte-identical
  to NIGHT.EXE (docs/185 §1-§2): shipped's title selection is `table[room+0x40] = table[0] = 2` -> LBA `0x9ddf`
  (261 s), the only track the shipped mix fingerprints (+0.837, docs/183 §3).
* `fn_010d47` has exactly one shipped caller (`branch_xrefs.py --target 0x10d47`: 1 edge, CALL from `fn_03a829` at
  0x3acd3) and the port wires it (`f.c:23493`, cited `0x0003ace6`) ✓ so the miss is not a missing edge
  (docs/184 §5), it is that the age counter feeding the bound is not aged like a clock.

## 4. The fix, and the one measurement it needs before being written

Fix shape (presentation/harness side, not the CD layer, as docs/184 §5 predicted): fire `fn_03ee88` on a
**real-time schedule** at the shipped IRQ rate -- i.e. driven by `sys_now_us()` in the pump/present path and in the
blocking spin paths (`sys_key_spin`, `delay_units`) -- so the CD age ages during title/menu/card/slide blocks
instead of only when the game loop happens to pump.  Which rate to use is the last open number:

* the divisor 490 (`0x1ea`) compared against a value the rig witnesses as 42 and 369 (`DS 0x53c48`, i.e. track
  *seconds*) implies the shipped counter advances ~490 per second; the port's current rate is
  `NIGHT_RIG_STEP_CALLS = 665` per game step (docs/184 §3 measured 665/s under the pinned clock) -- close enough to
  be plausible and far enough to be wrong, so it must be *measured*, not inferred.
* First attempt, negative: reading `DS 0x63e38` out of the shipped-side DS blobs of the canonical dump
  (`/tmp/rig/g33/orig/state_*.ds.bin`, 90 112 B = a 0x16000 window) at offset `0x63e38 - 0x52000` gives **0 in all
  413 states** (as do 0x63e24, 0x63e40, 0x63f08) -- the assumed window base is wrong, so this is not a shipped
  value.  `timeline.json` in that dump carries `events` (drive key timeline) but no per-state wall stamps.
* Second attempt, also negative, and the reason: `state_*.ds.bin` is **not** a flat DS window.  Scanning the two
  end states of `/tmp/rig/g33/orig` for dwords that grew by > 10 000 found 72 candidates whose implied window
  bases are ragged (`off 0x0f1f -> base 0x62f19`, `off 0x0fd0 -> base 0x62e68`, `off 0x1124 -> base 0x62d14`),
  i.e. the blob is a per-cell stream, not `[window_start, window_start+0x16000)` -- so raw offset reads cannot
  name `DS 0x63e38` on the shipped side.
* The measurement to run instead (the rig already reads arbitrary DS dwords on the shipped side): a short shipped
  drive that brackets the counter with ascending `wait ds` probes, e.g.

  ```
  anchor static 3.0 90
  47000 wait res title1.pcx
  47200 key enter down
  47350 key enter up
  60000 wait ds 0x63e38 >=490
  60000 wait ds 0x63e38 >=980
  60000 wait ds 0x63e38 >=1470
  60000 wait ds 0x63e38 >=2450
  ```

  `timeline.json`'s `ds_waits` records `{t_ms, ds, op, val, observed, hit}` for each (it already did for
  `0x53e9c`: `t_ms 859, observed 100, hit true`), so consecutive `observed` values against their `t_ms` give the
  shipped counter's rate as ticks per wall second directly, with the sample count quoted.  The expected order of
  magnitude is 490 ticks per second (the divisor in `fn_010d47`), against the port's current
  `NIGHT_RIG_STEP_CALLS = 665` per game step.

Then the green measurement is already scripted: the canonical 150 s route
(`scripts/rig/route.sh scripts/rig/drives/ep1_title_music.txt`, pinned `NIGHT_RIG_STEP_CALLS=665
NIGHT_RIG_FPS=60`) with the CD-play witness -- the port must select the title track (`0x9ddf`, 261 s) inside the
first ~31 s like the shipped does, with no teardown-without-stop regression (docs/183 §2's 166.7 s hold), and the
suite plus the nine-episode ASan soak stay green.
