# 168 — The game clock rate, measured on both sides from the dumps

Extracted from the docs/167 investigation; it stands on its own because it is a *global* time-base measurement
that every other timing claim depends on.

## 1. What was measured

`DS 0x63360` is the game's frame counter (docs/167 §11 established the 1:1 coupling with its increment site).
Counting the positive steps of that cell between consecutive dump samples gives a quantum per sample, and with a
known sample cadence, a tick rate — no witnesses, no instrumentation, just the dumps:

| side | run | samples | cadence | quantum per sample | implied ticks/s |
|---|---|---|---|---|---|
| shipped | `g15` | 1846 | 10 Hz | **9 ×933, 10 ×789** | ~95 |
| port, default (`route.sh` wall-clock pump) | `clkfree` | 1297 | 10 Hz | **7 ×677** | ~70 |
| port, fixed step 490 PIT Hz | `q4` | 2000 | 10 Hz | **7 ×1285** | ~70 |
| port, fixed step 665 PIT Hz | `clk665` | 1167 | 10 Hz | **9 ×346, 10 ×346** | ~95 |
| port, fixed step 700 PIT Hz | `clk700` | 1167 | 10 Hz | 10 ×703 | ~100 |
| port, fixed step 665, inside a real route | `g20` | 1500 | 10 Hz | **9 ×483, 10 ×483** | ~95 |
| shipped, same route, slower sampler | `g20/orig` | 424 | 2.8 Hz | 19 ×191, 20 ×60 | ~95 |

The shipped distribution is bimodal 9/10 because a fixed ~95 Hz clock sampled on a 100 ms grid alternates 9 and
10 — a signature of a clock paced against wall time, and it is reproduced *quantitatively* by the port at
`NIGHT_RIG_STEP_CALLS=665`, not at 490 (the value docs/107 chose) and not at 700.

## 2. Two separate findings

**(a) Port defect.** The port's real-time loop — the mode a player runs in, and the mode every oracle run used
until now — ticks at ~70 Hz while the shipped game ticks at ~95 Hz.  The port is **26 % slow**, so every timed
threshold in the game is reached late: the 500-tick idle timer (docs/167) needs 7.1 s instead of 5.3 s, animation
durations are 1.36× longer, and the number of per-frame events (sound triggers, picture changes) diverges by the
same factor.  This is a pacing defect in the port's own clock path (`main.c:1379-1391` pumping
`sys_pit_hz(0)`, which the game programs to 490 Hz at `0x982`), not a comparison artifact.

**(b) Rig calibration.** The oracle must compare equal clocks.  `scripts/rig/route.sh` now runs the port at
`NIGHT_RIG_STEP_CALLS=665` by default, chosen by measurement (above) rather than by the 490/≈60 fps arithmetic
of docs/107, and `g20` confirms 9/10 quanta on a real route.

## 3. Negative result, stated honestly

Re-running the full route at the calibrated cadence (`/tmp/rig/g20`, 150 s, both sides) did **not** shrink the
state-oracle residual:

```
g20 tally:  DIFF 38   DIFF-WIDTH 6   DIFF-SIZE 13   REC-DIFF 35   REC-SAME 15   SAME 64   REC-NULL 11
            NON-GAMEPLAY: device_plumbing 27, dos_extender 10, host_address_space 2, arena_state 3
            ENTROPY 14   SIZE? 35   PTR 52
activity-profile oracle: 269/269 SAME
pacing check: picture changes/s orig=5.02 port=27.39  ratio 0.18
idle-timer cell DS 0x6339c max: 147 (old cadence) -> 277 (calibrated) ; shipped g15 reached 498
```

So the clock quantum was a real skew — the idle timer visibly moved 147 → 277 toward the shipped 498 — but it is
not the explanation for the 38 same-width gameplay divergences, whose count is unchanged.  The pacing check also
did not improve (5.02 vs 27.39), so the port's picture-change rate is fast while its tick rate is slow, which
rules out "one global slow clock explains the screen oracle".

## 4. Rules and tools this produced

- `scripts/rig/cells_reader.py` is the only sanctioned way to read a port `cells.bin`: 16-byte file header, then
  per record a 40-byte packed header (`t_ms, frame, audio_bytes, screen_hash, tick, n`) and the payload
  (`rig_sink.c:304-307`).  It raises if the payload does not divide evenly.  Reading with a `record_bytes` stride
  drifts 56 bytes per record and fabricated a false "the port's globals are corrupted" finding, retracted in
  docs/167 §19.
- A quantum-per-sample census of a known monotone counter is the cheapest cross-side clock check available and
  should precede any dwell-time, animation-length, or trigger-rate comparison.
