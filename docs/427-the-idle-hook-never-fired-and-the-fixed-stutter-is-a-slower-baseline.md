# 427 — the idle hook never fired once, and the "fixed" stutter is a slower baseline, not a smoother one

User's report on `0.2.4`: "i think stutter is fixed." The card log says something else, and I would rather
have the log.

## 1. The hook registered and never ran

```
disc_idle_hook=1 registered=1
idle hook: presents=0        ... 115 of 115 samples, all the way to the end of a 114-second run
```

`of_file_set_idle_hook()` exists in the service table (`filecaps: ... idle_hook=1`) and we registered, but the
kernel called us zero times. The likely reason is on our side of the boundary: the port reads the disc with
**stdio** — `fopen("slot:N")` + `fseek` + `fread` (`of_disc.c:178-191, 233-243`), the route docs/375 measured
and docs/376 tuned — while the hook is documented against *the kernel's own DMA wait*. A stdio read that the
kernel services without the wait the hook instruments would register and never fire, which is exactly the
signature here. So the claim to test next is not "the hook works" but "does the OS call the hook at all on
this firmware", with an **ungated** counter, before anything else is built on it.

## 2. What the numbers actually did

| | `0.2.3` (docs/425/426) | `0.2.4` (this run) |
|---|---|---|
| frames/s: min / median / max | 3 / 47 / 55 | 8 / **33** / 49 |
| seconds more than 5 below the median | 22 of 70 (31%) | 19 of 114 (17%) |
| cumulative blocking disc time | 14.9 s of 70 s | 7.8 s of 114 s |
| below-median windows, mean blocked | 398 ms | 218 ms |
| idle-hook presents | — | **0** |

Burstiness genuinely dropped, and blocking disc time more than halved — but the **base frame rate fell from
about 47 to about 33**. Fewer, steadier frames is exactly what "smoother" feels like from the couch, and it is
not the same thing as smooth. I am not calling this fixed: the run is calmer because it is slower.

One trap in reading the stage timings, so the next reader does not repeat my first reading: the full-surface
hash runs *inside* the copy stage, so `stages ... c=` on a `PRESENT_SKIP=1` build includes the hash
(640x480: c=30.0 ms = ~18 copy + ~9 hash; 320x240: c=6.5 ms = ~2.9 + ~2.2). Dividing that number by the
surface size and comparing to docs/418 makes the display path look 1.7x slower. It is not; the hash is
in the bucket. I fell for it and corrected it here rather than silently.

## 3. The part I cannot account for, stated as a hole

Adding up what the log *can* see, for a typical late-run second of this build:

```
blocking disc   ~2.6 ops x 26 ms   =  68 ms
presents        ~33 x 7.5 ms        = 248 ms
                                    ---- 316 ms of ~1000 ms
```

On the `0.2.3` run the same accounting came out around 900 ms. So roughly 680 ms per second is now spent
somewhere the log does not name — tick pump, SFX transfers, or the loop's own bookkeeping. **I do not know
where, and the frame-rate drop is probably that, not the disc.** Until it is named, the stutter story has two
unresolved chapters: the hook that did not fire, and a baseline whose cost grew by more than any component I
can measure.

The next build adds an accounting line to `perf` — loop time split into tick pump / present / disc / other —
because "it feels better" plus "the frame rate dropped" is not evidence of a fix, and guessing which of three
suspects ate 680 ms is exactly the kind of thing this project has already been burned by (docs/404's "enabled
is not worked", docs/408's "every counter read healthy").

## 4. What is on the card, and what stays there

`0.2.4`, `DISC_IDLE_HOOK=1`, `FDUMP=2`, `DIAG=160`, `PRESENT_SKIP=1`, `DELAY_US=0`, `PALETTE_AFTER_FLIP=0`,
`CDDA_STOP_FIX=0`. The hook stays registered because its counter is the evidence — its cost is one function
pointer set at boot. `DISC_IDLE_HOOK=0` remains the one-line revert.

Also unchanged and still true: the fade trace armed twice again (122 lines), so stage 1's entrance continues to
present black and then full, never a ramp.
