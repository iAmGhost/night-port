# 441 — `clkmax=1` all run: the pump is inside the wait loop, so I can finally pay the game its real per-read time

`0.2.15` asked one question (docs/440): are the guest's clock reads consumed **between** pump visits or
**within** one? The boot answers it without ambiguity — across the entire run, every second:

```
split wall=5311ms ... frames=89  pump_n=89  res_calls=529 ... | clk=39  clkmax=1
split wall=8392ms ... frames=188 pump_n=188 ...                | clk=138 clkmax=1
split wall=13527ms ... frames=353 pump_n=353 ...               | clk=303 clkmax=1
```

`clkmax=1` and `clk ≈ pump_n`: **one guest clock read per pump visit**. So a hold's wall length is
`reads × poll cost`, and today a poll costs ~30 ms even when the copy is skipped, because the skip *decision*
scans the 307 KB surface (docs/440 §2). That also says the game clock and the picture are decoupled during a
hold: `ticks` advance at the real rate while `sig` does not change.

## The two changes this makes possible

**(1) `PRESENT_CAP_MS` — a display-path cadence cap.** A capped visit does **no display work at all**: no
surface hash, no copy, no palette upload. That is the point, since the hash is what a "skipped" present costs.
Default `0` (today's behaviour).

**(2) `DELAY_US=500` — pay the original's measured per-read cost.** Now that a poll is not charged 30 ms, the
honest figure from docs/416/417 becomes payable: `g_delay_unit_us` models the reference hardware's ~0.50 ms
per DOS clock read, and `delay_units()` waits that much per read while still pumping clock, audio and input.

On the desktop shim, armed together, the shape is what the reference measurements said it should be:

```
split wall=6009ms ... frames=227 pump_n=2938 ... | clk=2048 clkmax=5 cap=901 hold=26ms max=30ms
```

`clk=2048` reads in one second at 500 µs each is **1.02 s**, against the original's measured "2048 reads ≈
1.05 s" for the company-logo screen; the stage card's 12 000 reads follow the same arithmetic to ~6.0 s, which
is docs/416's second independent agreement. The pump runs ~2 900 times a second while only 227 frames are
presented and 901 display-path visits are capped — the waits are no longer paying for the display.

## What the boot has to show, in seconds

| hold | original (docs/416/417) | port, PRESENT_SKIP only | port, + cap 25 ms + DELAY_US 500 |
|---|---|---|---|
| company logo card | ~2.5 s visible (0.83 in, ~1.0 full, 0.83 out) | ~6.0 s | measured by `hold=..ms max=..ms` |
| stage / slide card | ~6.0-6.5 s (12 000 reads) | ~? | ditto |

`hold=%ums` on the split line is the longest stretch since a present that was **not** a skip — i.e. since the
panel last received something new — reset every second so a long hold is localized, with `max=` as the run
high-water mark. This is the first hold number in this project that comes from the log rather than from
counting witness frames or from arithmetic; `docs/417`'s "3.88 s vs 38 s" and `docs/415`'s "3.91 s vs 7.95 s"
were both derived, and both were right about the ordering but nobody had a duration meter to aim at.

## The honest caveats

* The two knobs are **coupled**: the hold length is `reads × (DELAY_US + poll cost)`. Setting `DELAY_US=500`
  without the cap overshoots (that is exactly why it shipped at 0); the cap without it leaves the length set
  by our frame cost. Only the pair is a measurement.
* `DELAY_US` lives in shared `src/f`, so it affects the SDL host too. `make test` passes, but it is a
  timing knob on the shared path and the cross-host diffs (docs/365) would show holds lengthening there as
  well — which is the point, but it should be stated rather than discovered.
* A capped visit skips the palette too. Nothing in the port needs a palette upload on a visit where the
  picture did not change; if a fade ever lands entirely inside a capped gap, `hold=` will show it as a
  longer-than-expected stretch, which is how I would find that bug.
