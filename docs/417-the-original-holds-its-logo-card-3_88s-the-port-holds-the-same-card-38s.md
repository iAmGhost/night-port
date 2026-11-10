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

## What this predicts, and what would falsify it

`PRESENT_SKIP=1` (docs/416) removes exactly the copies that carry these windows, so on the next boot:

- the logo phase should fall from 38.08 s toward the game's own pacing floor — docs/411 calibrated the logo hold
  at ~2048 clock reads ≈ **1.05 s**, so expect single-digit seconds with the fades and mode sets on top;
- the stage-card hold should fall from 7.95 s toward ~1.6-1.7 s;
- in the log, `perf ... skips=` should climb through both windows and `stages ... copy=` should go to ~0.

If the windows do not fall while `skips` climbs, the model is wrong and the copy was never the cost. If they fall
but land well above the floor, the remaining gap is the pacing constant — note the floor (1.05 s) is *below* the
original's 3.88 s, so a fully working lever would make these phases **shorter than the original**, and the next
item after it is `g_delay_unit_us`, not more skipping.

## Transport notes for the container route (add to docs/414)

- The slim image has **no `pkill`, `pgrep`, or `ps`**. Scan `/proc/[0-9]*/cmdline` — but match on
  `readlink /proc/$pid/exe`, **not** on the cmdline string: a cmdline pattern for `dosbox` also matches the
  `bash -c` shell running that very scan, which SIGKILLs itself (measured: exit 137, empty output). PID 1 in the
  container is `sleep 7200`; skip it explicitly.
- `docker exec dbga curl -o file.png` writes into the **container's** /tmp. Copy it out with `docker cp`.
- The emulator log timestamps are real wall-clock, so mode-change durations come straight off the log — cheaper
  and more precise than differencing frame-capture clocks.
