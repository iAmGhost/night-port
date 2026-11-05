# 369 — how often the pump is really entered, and what that says about the per-spin work

Task-9's next measurement.  docs/364 counted the CDDA pump's visits (295 921 377 in a 1 964-frame run) but the pump
*itself* had no counter: `memstat pumps=22 075` counts only the visits where a tick was due, which reads exactly like
the tick rate (490 Hz × 45 s) and therefore says nothing about how often the game re-enters the host.

## 1. The number

`g_pump_enters` counts every entry of `tick_pump`, including the ones from the game's own spin loops
(`fn_00035ec2` and friends call `sys_port_spin`, which calls it):

```
budget pump enters=655 466 013 per-spin (ticked=22 069) frames=2455
```

**267 000 pump entries per presented frame, against 22 069 tick-bearing ones -- a 30 000 : 1 ratio.**  Each entry
does a `sys_now_us()` clock read at the top, plus `audio_pump()`, the CDDA gate test, `of_mixer_pump()` on the
device, the depth bookkeeping and the present/tick scheduling arithmetic.  On a 100 MHz core that is the kind of
cost that only shows up as "the whole thing is slow", which is why it needed a counter rather than an argument.

## 2. What was gated, and what was deliberately left alone

Gated to the game's own tick (the same rule as the CDDA pump in docs/364 and the pad pump in docs/333):

- `warp_entry_pump()` -- its finest decision is a 250 ms confirm press = ~120 ticks.
- `save_pump()` -- it fires on a frame match, and a tick runs ~8× a frame, so a gated match is the same frame.
  Regression-checked, not assumed: `save: slot 2 -> /tmp/sv2/night_2.sav (192 B) stage=1 gauge=0/0 state=00/00/00
  resolver_calls=0`, and `warp entry: stage loop reached` still fires in the gated build.

Left running on every spin, on purpose: **`audio_pump()`**.  It drains the byte ring the game's own DMA transfers
write into, and "drain whenever the game spins" is the contract that ring is written against -- servicing it 490
times a second instead of 267 000 is a 2 ms latency change *and* an overflow question, and an audio glitch is a
worse price than some bookkeeping.  It is now the measurable candidate rather than an assumed one: the same
`budget` line reports SFX plays/drops/steals, so a gated experiment can be judged on `sfx=…/…/…` staying flat.

## 3. Desktop before/after, and the honest reading of it

| build | present mean | present worst | cdda visits | `dry` | `audio_low` |
|---|---|---|---|---|---|
| pre-gate (0.1.55, stage 1, 2 500 frames) | 477 µs | 4 490 µs | 20 842 | 0 | 0 |
| post-gate (0.1.56, same route) | 491 µs | 9 423 µs | 20 832 | 0 | 0 |

No improvement on the desktop, and the worst-case got worse -- which is the correct reading of this experiment rather
than a failure to notice: on the shim the present cost is dominated by the blit and the desktop's own cheap
`sys_now_us()`, so removing bookkeeping from a path that is nearly free changes nothing measurable, while the run
still has animation jitter of the size docs/364 §3.1 measured.  **The gating only pays where a pump entry is
expensive, and that is the device** -- where `sys_now_us()` is a register read and the core runs at 100 MHz.  That is
why the counter is shipped rather than kept in a branch: 0.1.56 on the card prints its own `pump enters=…`, which is
the device's spin rate, and the device is the experiment.

## 4. Card state, and the version trap hit and recorded again

Deployed: **0.1.56**, `night.elf` md5 `383968b8aedd1f169331e2386ccd7226` byte-identical to `pocket/.obj/night/app.elf`,
`core.json` 0.1.56, `night.ini` `SCREEN=-1 DIAG=1 LOG=1`, no `._` sidecars.

The trap: bumping `VERSION` in `mk_dist.py` and running `make deploy` once put `core.json` 0.1.56 over an ELF that
still said 0.1.55 -- docs/366 §3's exact failure, reproduced within one session of writing the rule.  The working
order is `make dist` (or `make`, whose `-MMD -MP` dependencies make the regenerated `of_interact_idx.h` force the
recompile) *then* `make deploy`, and then `strings … | grep ^0.1.` on the card next to `md5` against the build.  A
deploy that reports success is not evidence; the two strings matching is.

Fixture suite after the change: `== ALL PASS ==`.  `src/f` untouched.
