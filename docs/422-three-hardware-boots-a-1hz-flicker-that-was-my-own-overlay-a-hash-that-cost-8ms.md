# 422 — three hardware boots: a 1 Hz flicker that was my own overlay, a hash that cost 8.3 ms and skipped nothing, and the line that clobbered it

User offered hands-on device time, so this doc is measured on hardware rather than argued from the shim.
Three boots of `0.1.9x` on the card, one variable each. Build/version pairs: A = `0.1.98` with
`PRESENT_SKIP=0`, B = `0.1.98` with `PRESENT_SKIP=1` (same ELF, ini only), fix = `0.1.99`.

## 1. The flicker was the diagnostic, and it is now doubly gated

Reported as "flickers every 1 s ish". `night.ini` had `SCREEN=2` (`OF_DISPLAY_OVERLAY`, `of_video.h:45`
"white terminal text over framebuffer") because I had left it set for the task-12a check. In overlay mode the
1 Hz panel starts with `printf("\033[2J\033[H")` — a full clear of the text layer composited over the game —
which is the flicker, at exactly that rate.

Two changes, both in `0.1.98`:

- the card's `night.ini` → `SCREEN=0`. Nothing draws over the picture any more, and the log confirms it
  (`settings: ... screen=0 file_log=1`).
- `main_of.c`: overlay text now additionally requires `DIAG & OF_DIAG_PANEL` (bit 64). A player who picks
  Pocket LCD → Overlay cannot flicker their own game. `OF_DISPLAY_TERMINAL` still prints unconditionally —
  the game picture is not on the panel there, and that is the route that was confirmed working.

**task-12a closes on this.** Its contract was "terminal text visible on the Pocket LCD during a run", and the
user has seen it from Core Options → Pocket LCD → Terminal. The overlay route was my invention to avoid asking
anyone to switch the LCD away from the game; it turned out to cost the game. Contract satisfied by the simpler
route, so the overlay's default stays off. The docs/411 lesson repeats — a diagnostic can be the pathology,
this time at 1 Hz and this time over the picture.

## 2. `surfhash` was measuring a quarter of the surface and calling it throughput

`DIAG=32` times a pass over the surface with `for (i = 0; i < n; i += 4u) h = (h ^ p[i]) * 16777619u;`. That
reads **one byte every four**, and prints `n / us` as if it had read them all. It is not byte-throughput bound,
it is iteration bound (load + xor + mul at 100 MHz), so the printed MB/s was 4× the real coverage rate.

Rewritten to word reads — the loop `PRESENT_SKIP` actually runs — and re-measured on hardware:

```
surfhash: 76800 B read in 2235 us (34.4 MB/s) hash=c93264c3
```

That gives the cost table that decided the rest of this session's plan:

| surface | full-surface hash | copy | net when static |
|---|---|---|---|
| 640x480 native (logo, slides) | ~8.9 ms | ~18.4 ms | ~9.5 ms/frame |
| 320x240 (gameplay) | ~2.2 ms | ~2.9 ms | ~0.7 ms, on the ~24% of frames that are identical |

So the change-skip is a **holds-only** win of roughly 2×, not the ~19× I predicted in docs/417 — that
prediction assumed hashing was free and the whole copy disappeared. Boot B would have shown that on its own;
it showed something else instead.

## 3. Case A — `PRESENT_SKIP=0`, a clean 123 s run

```
0.0-  41.0  640x480  40.97 s  sig_chg 1   52.8 pres/s  copy 18991 us  2162 frames   16.2 MB/s
42.7-  52.4  640x480   9.68 s  sig_chg 7   31.2 pres/s  copy 23918 us   302 frames   12.8 MB/s
52.4- 123.0 320x200  70.65 s  sig_chg 70  45.4 pres/s   copy  2934 us  3210 frames   26.2 MB/s
```

- No asserts, no disc read failures; the CDDA voice tracked at `rate=0.999x` with the ring ~2 s ahead at the end.
- The logo hold is 41.0 s with `sig` changing **once** — 2162 presents of one picture. This is the phase the
  skip exists for, and it is static by measurement, not by assumption.
- The card-silence rule fired exactly once and for the right length:
  `cdda EVENT stop tick=24559 by=card-rule` → next play at `tick=25544` = **2.01 s**.
- Three `by=port-bookkeeping` stops (ticks 19869, 23418, 25544) each landed **on the same tick as a new
  `play idx=`**. That is why they are inaudible on this route: the game restarts a track immediately after
  each one, so the invented stop is only a rewind, not a silence. docs/421's 2.5 s deletion needs a case where
  the track is meant to *continue*, and this route does not contain one. So the audio listen test is still
  worth a boot, for a reason I did not expect when I scheduled it.
- Worst single frames: 76 ms in the slides phase, 48 ms in gameplay. Those are the slide blits and PCX decode,
  not the copy — a different lever from the one this doc has been pulling.

## 4. Case B — `PRESENT_SKIP=1` on the same ELF: the hash paid, and skipped nothing

Only the ini changed, so the A/B is exact. Result:

```
perf pres=27435us max=83910us pump=29 ticks=498 skips=0     ...  skips=0, all 118 samples
stages last m=15 p=147 c=26746 f=213 x=236 | max all=83874 copy=82853
view=0 640x480 sig=ecd967f7 chg=1 pal=255
cdda EVENT play idx=1 lba=37262 sectors=3153 tick=29394      (= 60.0 s; case A: tick 19869 = 40.5 s)
```

`skips=0` in every sample, `copy` up from 18.4 ms to 26.7 ms, logo hold up from 41.0 s to ~60 s. The extra
8.3 ms is exactly `surfhash`'s 307200-byte word pass. The mechanism ran, cost the money, and was discarded.

The cause is a line written for an older feature:

```c
skip = of_setting_blit_skip() ? 1 : 0;   /* BLIT_SKIP=0 on the card keeps every blit */
```

Six lines after `PRESENT_SKIP` had decided `skip`, this **assigned** to it — not OR'd, not guarded. With the
shipping `BLIT_SKIP=0` it reset the decision to 0 on every frame. Worse, `skip` was the only counter the log
reported, so "the hold got longer" was the only symptom, and the log line that named the cause was the one
reading `skips=0` — which reads as "nothing to skip" rather than "the decision was thrown away".

Fix in `0.1.99`: the sampled path fills a separate `smp` and is OR'd in, so it can add a candidate but cannot
erase one; and the log now splits the attribution — `skips=N(hash X smp Y)` — so a future run says *which*
mechanism decided, and a zero is ambiguous no more. The two mechanisms stay independent, including their
staleness rules: the sampled skip is bounded at 8 consecutive frames, the hash skip is bounded by the
three-display-buffer proof instead.

Root `make test` and `pocket/src/night make test` (both halves) green; `verify_card` OK at 0.1.99.

## 5. What is still open

- Whether B2 (`0.1.99`, `PRESENT_SKIP=1`) lands the predicted logo hold 41.0 s → ~20 s with `hash` skipping,
  and whether any picture is ever stale on hardware. The user's eye is the only instrument for staleness.
- Choppiness during animated 640x480 phases: the skip only helps static frames, and slides/gameplay animate.
  With the hash costing ~8.9 ms and the copy ~18.4 ms, the remaining lever is a **free** change signal — there
  is no arena-write counter in shared code today (`grep vga_write|video_write|write_count src/sys src/f` comes
  back empty), so that would be a port-side addition, not a knob.
- The audio listen test (`CDDA_STOP_FIX=0`), and a save round-trip on hardware.
