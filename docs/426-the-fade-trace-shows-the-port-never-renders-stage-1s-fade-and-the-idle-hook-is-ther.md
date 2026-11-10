# 426 — the fade trace's surprise: the port never renders stage 1's fade at all; and the file-service probe hands us the stutter fix

Two things came out of the `0.2.3` boot, and neither was the one I went looking for.

## 1. At present granularity, stage 1 has no fade

`FDUMP=2` auto-armed, 120 traced presents across two stage entries:

```
   f      t_ms  lit  sum        d_sum   d_lit  ndiff src
   314       0     0         0                       0   0      <- black
   315      34     0         0        +0     +0      0   0      <- black
   316      40   255     95413    +95413   +255    255   1      <- FULL, in one frame
   317      57   255     95413        +0     +0      0   0
   ...  sum stays exactly 95413 for the remaining 57 presents
  over 30 frames: sum rising 1, sum DROPPING 0
```

`ndiff = 255` — every palette entry moved at once, 40 ms after the mode change, and then nothing ever moved
again. **The port does not render an intermediate palette for stage 1's entrance: it shows black, then the
stage.** The original ramps this over ~0.83 s (docs/373, `fn_040660`'s 0x19-step ramp).

Why: with `DELAY_US=0`, `delay_units()` reads the clock and does **not** spin (`f.c:11406`, the busy-wait is
inside `if (g_delay_unit_us)`), so the game's fade loop never yields to the host. All 25 steps run between two
presents. The fade is real in the game's model and invisible on the panel.

Two consequences worth writing down before someone re-litigates them:

- **The flicker I had been chasing for four boots cannot have been a rendered palette ramp**, because no
  ramp is rendered. Either it was the black→full step plus the 21 surface-change frames around it (the trace
  shows `src=1` on 21 of 120 frames, mostly early — a reveal, not a fade), or the symptom has another home.
- **`DELAY_US` is not a free calibration knob.** Its spin loop calls `sys_port_spin()`, which re-enters the
  host pump (`main_of.c:1201`), so a nonzero value would make fades visible — but after `PRESENT_SKIP` the
  work per clock read already exceeds the original's measured ~1.27–1.9 ms per read, so any positive
  `DELAY_US` would push the holds *past* the original rather than onto it. docs/417's "~950–1000 µs" target
  was calibrated on pre-skip builds; **`DELAY_US=0` is the right shipping value now**, and the fade gap is the
  price, stated rather than buried.

## 2. "Flicker is gone" is not explained by anything I changed

`0.2.3` ships `PALETTE_AFTER_FLIP=0` — the same ordering that flickered in boot C — and adds no behaviour on
the display path. The only new thing running during the fade is the trace itself. So the fair reading is a
timing perturbation (docs/411's pattern: an instrument changing the thing it measures) or a state the port has
since left behind. I am not calling it fixed. The trace stays armed by default (`FDUMP=2` in the shipped ini),
so if it comes back it is captured automatically instead of depending on a button press at the right moment.

## 3. The file-service probe: the stutter has a door

```
filecaps: svc_count=106 idle_hook=1 async_max=65536 dma_stage_total=1048576 stage64k=0x30500000 busy=0
```

`idle_hook=1` — this firmware has `of_file_set_idle_hook()`. Also useful to know: the async path caps at
65 536 bytes (not our 63 504) and the CRAM0 staging allocator handed out a 64 KB stage from a 1 MB pool, so a
properly staged async retry is viable too. The hook is the cheaper first move, because it needs no protocol
to get right — the OS calls us when the SD's DMA is waiting.

And the stutter is still exactly those waits, measured again on this run:

```
22 of 70 seconds below the frame median
below-median windows: mean 398 ms blocked on the card
at/above-median windows: mean 129 ms
cumulative blocking disc time: 14.9 s of 70 s      worst window: 3 frames, 438 ms
```

## 4. `0.2.4` — present frames during the wait

`present()` was extracted into `present_step(now)` so the main loop and the hook share one body: same
timings, same counters, same cadence advance, so a hook-present is indistinguishable in the log from a loop-
present. The hook:

```c
static void disc_idle_hook(void) {
    if (!g_disc_busy || g_in_present || g_hook_busy) return;
    uint64_t now = sys_now_us();
    if (now < g_present_next_us) return;            /* nothing due; let the wait finish */
    g_hook_busy = 1; g_hook_presents++; present_step(now); g_hook_busy = 0;
}
```

Guards, in order of the failure they prevent: never re-enter the display path (`g_in_present`); one hook-present
per blocking read however long it runs (`g_hook_busy`); and **only while our own music read is the one in
flight** — `of_cdda_set_disc_busy_cb()` flags the CD read specifically, because a resource load can land while
the game is mid-draw, and presenting then would show a half-built card. Fallback is one ini line:
`DISC_IDLE_HOOK=0`. The log carries `idle hook: presents=N`, so the boot says whether the hook fired at all —
which is the lesson of docs/404, where "async enabled" and "async worked" were not the same claim.

Card: `0.2.4`, `DISC_IDLE_HOOK=1`, `FDUMP=2`, `DIAG=160`, `PRESENT_SKIP=1`, `DELAY_US=0`,
`PALETTE_AFTER_FLIP=0`, `CDDA_STOP_FIX=0`.

## 5. And one thing the user settled for me

L1 combos do work on the Pocket body — the load combo restored the HP bars, so docs/334's save/load route is
live. My one-shot instrument logged `combo: first combo-button seen l1=0 select=1 start=0`: it fires on the
first combo-button *event* only, so it proved the call site runs but cannot speak for later L1 presses. If a
combo ever silently does nothing again, that line is the first thing to read.
