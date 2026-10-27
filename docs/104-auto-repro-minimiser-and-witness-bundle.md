# 104 — Auto-repro minimiser and witness bundle

`rig-runtime` → `rig-minimise`. Continues docs/101 (determinism), docs/102 (state oracle),
docs/103 (media oracles).

## 1. What the minimiser does

`scripts/rig/minimise.py` takes a drive that diverges and shrinks it by delta-debugging:

1. **probe a candidate** — a *fresh* route: the port runs in the host, the shipped game boots in a new
   container (`bringup.sh` + `calibrate_ds.py` + `replay_original.py`), and only the state oracle runs
   (the cheapest oracle with a per-cell verdict).
2. **ddmin over the event lines** — drop interleaved chunks of events (interleaving keeps the timeline
   spread instead of truncating it), keep any subset whose score still exceeds `--min-diff`; then a
   single-line pass.
3. **shrink the route length** — halve the duration while the divergence survives.
4. **build the witness bundle** — re-run the *minimal* drive through `route.sh` (all four oracles) and
   assemble everything needed to root-cause without re-running anything.

Two predicates, both measured:

- **default — divergence from the shipped game.** Score = `DIFF + REC-DIFF` cells from the state
  oracle. This is the literal meaning of "still diverges", but on the current build the port diverges
  almost everywhere, so the shrink is allowed to keep drives that do not exercise the injected fault.
- **`--baseline` — injected build vs clean build on the same drive.** Score = DS cells that differ
  between two port dumps of the *same* drive (clean vs `NIGHT_RIG_CONTROL=…`), entropy cells excluded,
  pointer cells counted separately because heap layout differs between processes for meaningless
  reasons. No container per probe (≈40 s instead of ≈70 s), and an empty drive cannot reproduce an
  input-dependent fault, so the shrink is *forced* to keep the key presses that matter.

## 2. Measured runs

| run | fault injected | predicate | start | minimal | probes | score kept |
|---|---|---|---|---|---|---|
| `/tmp/rig/min1` | `no_timer` (fn_03ee88 never called) | shipped game | 6 events / 20 s | **1 event / 2 s** | 2 + 3 duration | 62 cells |
| `/tmp/rig/min2` | `no_input` (keys never reach key state) | `--baseline` | 6 events / 20 s | **1 event / 10 s** | 2 + 2 duration | 14 cells |

The `--baseline` run is the informative one: at 5 s the score collapses to 2 cells, so the surviving
event (`10060 key enter up`) is *required* — the minimal repro is genuinely input-dependent, and the
bundle's full route reproduces it: `divergent runs: 1`, pacing `orig=0.10 port=13.69/s`,
audio `original 9.4 s audible vs port 0.2 s`.

Honest limitation: ddmin stopped at one event because removing it leaves an empty drive, which the
shrink refuses; and the surviving event can be temporally artificial (an `up` without its `down`).
Both are recorded rather than hidden — the bundle always carries the full probe log.

## 3. Witness bundle (what root-causing gets)

`$OUT/witness/`:

| file | content |
|---|---|
| `minimal.txt` | the minimal drive, with the shrink statistics in its header |
| `DIVERGENCE.md` | all four oracle reports for the minimal route, with timestamps |
| `report_state.md` | per-cell diff table: address, both values, port symbol, verdict |
| `report_screen.md` | route score, divergence windows, DS attribution per window |
| `report_audio.md` | per-window onset / audible-fraction comparison |
| `port.log` | the port's own log (script events, pump counters, board state) |
| `orig_frames.bin`, `port_frames.bin` | the full per-frame witness streams |
| `frames_side_by_side.pgm` | the two 40×30 luminance grids nearest the window, upscaled 6×, side by side (PGM: no image library needed) |
| `orig_audio.wav`, `audio_window.wav` | the container's capture and the slice for the divergent window |
| `manifest.json` | minimal events, duration, injected fault, finding window, every probe's score/tally |
| `repro.sh` | re-runs the whole route unattended and prints the finding lines |

Verified: `bash witness/repro.sh` (fresh containers, nobody involved) reproduced the same finding —
`divergent runs: 1`, `pacing … ratio=0.06`, `port 0 buckets` of audio — matching the bundle's
`DIVERGENCE.md`.

## 4. The port-side faults (the rig has to catch a broken build)

`NIGHT_RIG_CONTROL` now implements three stubs in the port host (docs/103 §4 has the measured
detection table): `no_timer` (the guest's installed timer handler is never called — the state oracle
sees the record stream freeze: `REC-DIFF` → `REC-SAME`/`REC-NULL`), `no_input` (scripted keys are
logged and marked but never applied — pacing rate doubles), `no_draw` (composed, never presented —
**not** observable through the port's own screen witness, recorded as a hole).

## 5. Reproduce

```sh
python3 scripts/rig/minimise.py --out /tmp/rig/min2 --inject NIGHT_RIG_CONTROL=no_input \
        --baseline --seconds 20 --max-runs 12
bash /tmp/rig/min2/witness/repro.sh
```
