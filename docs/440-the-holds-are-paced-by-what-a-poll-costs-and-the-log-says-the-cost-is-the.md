# 440 — the holds are paced by what a poll costs, and during a hold the whole second is the *decision*, not the copy

The player's report on `0.2.14`: no graphical glitch, fade looks right — and "long logo hold and long intro
load persisted". Both are the same mechanism, and the log has been saying so all along in a line I had been
reading as a copy number.

## 1. What a hold actually is

`delay_units()` (f.c:11395) converts its argument into `es_i` **guest DOS clock reads** and spins them:

```c
while (es_i-- > 0) {
    sys_clock_read();                         /* 0x45679-45680 -- and in this port that is a counter bump */
    if (g_delay_unit_us) { ... sys_port_spin() ... }
}
```

`sys_clock_read()` is a counting stub (sys.c:443). So the guest's wait is a loop of N iterations whose cost
is *whatever the port spends per iteration* — and the hold's wall length is `N × cost_per_iteration`. It is
not paced by the 490 Hz game clock: during the logo hold the log shows `ticks` advancing at the real rate
(~500/s) while `sig=ecd967f7 chg=1` — the picture unchanged for seconds at a time.

That is why `PRESENT_SKIP` cut the logo hold from 41 s to 6.0 s (docs/417 lineage) without changing a single
tick of game time. It did not speed the game up; it made one poll cheaper.

The port already knows the reference figure: `g_delay_unit_us` models the original's **~0.50 ms per clock
read**, measured two independent ways against the original (logo card 2048 reads ≈ 1.05 s; stage card 12 000
reads ≈ 6.0 s). It ships at `DELAY_US=0` because with a 30 ms present per poll, 0.5 ms on top pushed the
holds *past* the original (docs/416/417). So the pacing knob and the poll cost are one knob, and paying the
honest 0.5 ms is only honest once the poll is cheap.

## 2. The line I had been misreading

The boot's logo phase, one second at a time:

```
perf pres=30709us ... pump=24 ticks=499  skips=21(hash 21 smp 0)
perf pres=30713us ... pump=89 ticks=1500 skips=86(...)
perf pres=30708us ... pump=188 ticks=3010 skips=185(...)
split ... pres=5866ms ... frames=188 pump_n=188 ... sig=ecd967f7 chg=1
```

33 presents a second, **every one of them skipped**, each costing ~30.7 ms — so ~1 010 ms of every second is
present-path work while **nothing on the panel changes**. This is the sharpest form of the copy story and it
corrects it: on a build where the copy is skipped, the machine is not the write, it is the *decision* — the
pass over the 307 KB surface that answers "should I copy this?". docs/418/429's "the copy is 66-74 % of the
run" was true of builds that always copied; it is not what a skipped frame costs.

## 3. The one number that decides the fix

`0.2.15` adds it to the split line:

```
| clk=<guest clock reads this second> clkmax=<most reads between two pump visits>
```

- `clkmax ≈ 1` ⇒ the pump is *inside* the wait loop: the hold is poll-paced, so making the present cheap
  shortens the hold directly, and `DELAY_US` becomes the knob that sets its length.
- `clkmax ≫ 1` ⇒ the game spins reads the pump never sees: present cadence is irrelevant to the hold, and
  `DELAY_US` alone is the honest pacing knob.

Either way the plan is the same two steps, in that order: make polls cheap while the game is waiting, then
pay the measured 0.50 ms per read and check the hold lands near the original's 2.5 s logo and ~6.5 s card
instead of near whatever our frame cost happens to be. The intro load is a different lever (disc throughput
at ~2.5 MB/s, docs/434/435) and is not touched here.

## 4. Also, from the same boot

`fade ramp done: 0.70 s over 42 steps (target 700 ms, 1 present(s) carried no step)` — the wall-clock ramp
(docs/439) measured 0.70 s against the original's 0.6-0.83 s, and the player saw no artifact. That thread is
closed; this one is open.
