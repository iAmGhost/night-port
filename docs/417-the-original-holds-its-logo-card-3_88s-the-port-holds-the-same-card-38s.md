# 417 — the original holds its logo card 3.88 s; the port holds the same card 38 s with a signature that never moves

docs/416 said the Pocket's long holds are the present, not the clock read, and named the stage card as its one
measured case. Here is the second case, and it is far worse: the **boot logo card**, where the port spends
**38 s** against the original's **3.88 s** — a ~10× gap, not 2.4× — and the port's own log proves the picture
never changed once during it.

## The original, measured twice on the calibrated container route

The route runs at realtime factor 1.000 (docs/414), so the emulator log's own timestamps are seconds. The logo
card is the mode-`101h` (VESA 640x480) window between the first `13h` and the attract screen:

| run | 720x400 text | -> 13h | -> 101h (logo card) | -> 13h (attract) | **hold** |
|---|---|---|---|---|---|
| `run11` | 56.978 | 59.982 | 59.982+0.118 | +3.992 | **3.87 s** |
| `run12` | 02:10.089 | 02:12.578 | 02:12.709 | 02:16.591 | **3.88 s** |

No keypress was sent in either run (`presses sent: []`). The card is the TQ Entertainment mark — witness
`scratch/pocket/orig_logo2/0004_640x480.png`. The picture ID by NCC is weak there (best `1s3` at 0.36; the
control, a real title frame, scores 0.93 per docs/415), so the card is identified by eye, not by score.

## The port, from its own card log (0.1.93, ticks / 490 Hz)

```
t=  0.00s  video CHANGED: view=0 640x480 (was -1 -1x-1)      <-- logo card up
t= 37.16s  fb mode -> game 320x240 rc=0 in 42206us
t= 38.08s  video CHANGED: view=1 320x200 (was 0 640x480)     <-- attract
```

**38.08 s on the same card.** Inside that window:

- `frames` runs 103 at t=2 s to 2097 at t=37.2 s → **56.5 presents/s**, 2097 presents in the window.
- `view=0 640x480 sig=ecd967f7 chg=1` — the frame signature is **one value with one change**, the first frame.
  The palette goes 1 → 255 in the first second and then holds, i.e. it fades in and then nothing moves.
- `cdda=0/0` (no audio yet), `arena` static, `pumps == frames` (one pump per present).

```
2097 presents x ~17 ms of 640x480 surface copy  = ~36 s of a 38 s window
```

Same shape as the stage card in docs/416 (398 presents x ~17 ms = ~6.4 s of a 7.95 s window against the
original's 3.91 s), on a second phase, with the added proof that the surface signature never moved at all.
Both are the same defect: **a `delay_units` wait during which the host re-copies an unchanging surface as fast
as it can, and the copy costs more than the wait it is padding.**

## The third host, for free: the desktop shim

The shim runs the same `src/f` with presents that cost nothing (SDL2 dummy driver). Run and capture:

```sh
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234 NIGHT_CD_DIR=scratch/pocket/cd NIGHT_FILELOG=0
./pocket/src/night/app_pc --frames 4000 --rig-witness 0 2>&1 | perl -pe 'BEGIN{$|=1; use Time::HiRes qw(time); $t=time}
  $_ = sprintf("%.3f ", time-$t).$_'            # wall-stamped log
```

```
t= 1.02s  video CHANGED: view=0 640x480 (was -1 -1x-1)
t= 3.02s  video CHANGED: view=1 320x200 (was 0 640x480)      -> 2.00 s on the logo card
t= 1.02   view=0 640x480 sig=ecd967f7 chg=1 pal=255          -> the SAME signature as the card
memstat: 54.5 frames/s throughout
```

Three things fall out:

- the logo card's signature is **`ecd967f7` on the card and on the shim** -- the same picture, byte for byte in
  the arena, and static on both;
- the shim spends **2.00 s** there, the device **38.08 s**: a factor **19**, not 2.4;
- and the arithmetic closes without a free parameter:

```
device 38.08 s  ~  shim floor 2.00 s  +  2097 presents x 17 ms (35.7 s)  =  37.7 s   -- 1% off
```

That is the model, on a second phase, to within one percent: the presents are counted from the log and the 17 ms
was measured in the same log (docs/416). It is not a fit -- there is nothing left in the window to attribute.

## What this predicts, and what would falsify it

`PRESENT_SKIP=1` (docs/416) removes exactly the copies that carry these windows, so on the next boot:

- the logo phase should fall from **38.08 s to ~2.0 s** -- not toward a theory, but to the 2.00 s the shim
  already spends on the same card with the same signature. A 19x collapse, or nothing: that is the test;
- the stage-card hold should fall from 7.95 s toward the shim's 3.01 s (docs/416 measured that window too);
- in the log, `perf ... skips=` should climb through both windows and `stages ... copy=` should go to ~0.

If the windows do not fall while `skips` climbs, the model is wrong and the copy was never the cost. And note the
direction of the leftover: the shim's floor (2.00 s) is *below* the original's 3.88 s, so a fully working lever
makes these phases **shorter than the original**. The knob after this one is `DELAY_US` (night.ini, 0 = leave
f.c's 500 us alone): scaling the shim's 2.00 s by the original's 3.88 s puts the target near **950-1000 us**, and
that is a measurement to make in its own boot, *after* the skip is proven -- never in the same boot as the skip.

## Transport notes for the container route (add to docs/414)

- The slim image has **no `pkill`, `pgrep`, or `ps`**. Scan `/proc/[0-9]*/cmdline` — but match on
  `readlink /proc/$pid/exe`, **not** on the cmdline string: a cmdline pattern for `dosbox` also matches the
  `bash -c` shell running that very scan, which SIGKILLs itself (measured: exit 137, empty output). PID 1 in the
  container is `sleep 7200`; skip it explicitly.
- `docker exec dbga curl -o file.png` writes into the **container's** /tmp. Copy it out with `docker cp`.
- The emulator log timestamps are real wall-clock, so mode-change durations come straight off the log — cheaper
  and more precise than differencing frame-capture clocks.
