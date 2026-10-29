# 127 — Nine-episode ASan/UBSan soak: 9/9 clean (0 reports, 0 SEGV, rc=0)

docs/126 removed the alignment class that made sanitizer runs unusable (878 reports on one route). The soak the
closeout contract asks for is now possible, so it was run: `scripts/rig/asan_soak.sh` (committed).

## 1. How it is driven

```
OUT=/tmp/rig/soak1 JOBS=4 CAP=320 FRAMES=4000 scripts/rig/asan_soak.sh
# per stage s = 1..9:
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 SDL_AUDIODRIVER=dummy \
  timeout 320 ./port/test/night-asan --res-dir ./NIGHT --script scripts/rig/drives/title_basic.txt \
      --warp-stage s --frames 4000
```

`--warp-stage s` is the same episode-reach construction the fuzz sweeps use (docs/120), so each stage is a real
episode timeline rather than stage 1 nine times. Counting is mechanical: `runtime error`, `ERROR: AddressSanitizer`,
`SEGV on unknown address`, plus the exit code and `frames presented` (a run that stops early would not be evidence
of anything).

## 2. Result

| stage | rc | UBSan | ASan | SEGV | frames |
|---|---|---|---|---|---|
| 1 | 0 | 0 | 0 | 0 | 4000 |
| 2 | 0 | 0 | 0 | 0 | 4000 |
| 3 | 0 | 0 | 0 | 0 | 4000 |
| 4 | 0 | 0 | 0 | 0 | 4000 |
| 5 | 0 | 0 | 0 | 0 | 4000 |
| 6 | 0 | 0 | 0 | 0 | 4000 |
| 7 | 0 | 0 | 0 | 0 | 4000 |
| 8 | 0 | 0 | 0 | 0 | 4000 |
| 9 | 0 | 0 | 0 | 0 | 4000 |

Every episode reaches its full frame budget under ASan+UBSan at `CAP=320`, which also settles the docs/123 §6
observation that 4/12 seeds missed 2500 frames in 150 s — the cap, not the sanitizer cost, was the limit.

## 3. Rig defect found while running it

`wait -n` is a bash 4 feature; macOS ships bash 3.2, so the throttle loop in `asan_soak.sh` spammed
`wait: -n: invalid option` and ran the nine stages without the intended concurrency limit (harmless here — the
runs all completed — but it defeats the parallelisation budget the rig is supposed to respect). Replaced with a
`sleep 2` poll of `jobs -rp`. Recorded because a rig that silently ignores its own concurrency cap is a rig
defect, same category as the parser regressions in docs/121 / docs/124.
