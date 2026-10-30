# 205 — The port stops entering the gameplay video mode: the mode-segment census, and the screen oracle's pairing fix

## 1. `diff_screen.py`'s pairing saturates on long routes (a rig defect, now measured)

`align()` walks the similarity matrix with a fixed ±90-frame lag window around the proportional position.
On the canonical 300 s route (`/tmp/rig/g37`) the streams are 6 064 shipped frames and 38 943 port frames, so the
proportional position advances 6.4 port frames per shipped frame and any clock drift over ~14 % saturates the window.
The worst reported window ("orig t=101778..142396 ms, 40.6 s, worst score **0.000**") pairs shipped frames 2854-2856
(320x200, grid mean luminance 157-... ) with **port frame 0 — a black 640x480 boot frame**:

```
=== orig t=141350 ms (frame 2854, 320x200)  vs  port t=0 ms (frame 0, 640x480)  score=0.000
   ########################################   ........................................
   differing cells(>60): 1200/1200  bbox x=0..39 y=0..29
```

That is an alignment artifact, not content, and must not be quoted as a divergence.

## 2. `scripts/rig/screen_time.py` (new): pair by nominal time, not by frame index

Both dumps carry millisecond stamps and the shipped replay publishes its rebase table
(`emulator ms -> drive nominal ms`: `[0->47200, 1203->60000, 15861->150000]`), the same mapping `diff_state.py` uses.
Pairing each shipped frame with the port frame nearest in nominal time (tolerance 400 ms) gives:

```
pairs within 400 ms = 4723 of 6064 shipped frames (1341 unmatched)
route score: mean 0.7506  median 0.7565  p10 0.6653  min 0.3979   pairs < 0.88: 4678
same-mode pairs: 2034, mean 0.7492, min 0.3979, pairs<0.88: 1989
cross-mode pairs: 2689, mean 0.7516
```

Read against the DP alignment's "6 divergent runs", this says the honest thing: **under time pairing the two games are
rarely similar at the 40x30 luminance level (mean abs diff ~64/255), everywhere, because the port's clock runs
5x fast** (picture changes per second 5.20 shipped vs 25.08 port).  A time-paired grid score is therefore a pacing
measurement, not a content measurement; the screen halves of `gate3-upgrade-menu`, `gate3-stage2-crash` and
`gate3-side-glitch` have to pair on discrete events (mode switches, room transitions), not on wall time.

## 3. The structural divergence the mode census does show

Splitting each stream into runs of constant frame size (shipped emulator ms / port nominal ms):

```
SHIPPED  8 segments:  640x480 46.2 s | 320x200  1.3 s | 640x480 51.8 s | 320x200 42.9 s |
                     640x480 52.3 s | 320x200 43.0 s | 640x480 52.5 s | 320x200  9.6 s
PORT     4 segments:  640x480 12.1 s | 320x200 29.9 s | 640x480 26.2 s | 320x200 293.6 s (30 751 frames,
                     nominal t=73 250..366 834 ms)
```

`f.c:7404` states what the two modes mean: the 640x480 VBE mode 0x101 (set by `fn_03f15f`) is the game's linear
video arena -- the gameplay screen -- and the 320x200 mode is the PCX slide-show mode.  So:

1. The shipped game leaves and re-enters the gameplay arena **three times** after the first stage clear; the port
   leaves it once (nominal 73 s) and **never returns** for the remaining 294 s of its own clock.
2. Each shipped mode switch coincides with 8-52 changed DS dwords in the 4 Hz snapshots (e.g. snapshot 423 at the
   frame-2877 switch: 52 changed dwords), so they are real screen-state transitions, not capture noise.
3. All nine shipped CALL sites into `fn_03f15f` are modelled in the port -- callers `fn_010c3e` 0x10c8e (f.c:5651),
   `fn_038a20` 0x38a38 (f.c:15868), `fn_038ae0` 0x38b24 (f.c:15890), `fn_03ae8e` 0x3aeef (f.c:15936) and
   `fn_03af18` 0x3b061/0x3b1f7/0x3b2b8/0x3b466 (f.c:27612, 27680, 27537, 27691).  The gates that reach them are
   modelled too, and their tables agree byte-for-byte with the shipped DS: shipped `DS 0x52f90` =
   `00 00 00 00 01 00 00 00 01 00 00 00 02 00 00 00` vs the port's `g_tab_52f90 = {0,1,1,2,...}`, shipped
   `DS 0x52fb4 = 3` vs `g_dat_52fb4 = 0x3`.  **The divergence is state/path, not a missing call.**
4. `gate3-stage2-crash` needs a different drive: on this route the port does *not* fault (no ASan/SIGSEGV/bus-error
   line in `/tmp/rig/g37/port.log`, 18 000 frames presented, ending with "game main unwound (quit / fatal)") -- but it
   is stuck in the slide-show mode exactly where shipped re-enters gameplay.  The stage-2 drive has to reach that
   transition, and the "black floor" claim has to be measured on the arena buffer, not on a time-paired grid.
