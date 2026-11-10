# 439 — the fade ramp works: 42 steps, armed on the stage-entry fade-in — and its length depended on the frame rate

`0.2.13` swapped the byte-count discriminator for a content one (docs/438). It armed, immediately and twice:

```
fade ramp: n=42 (entries moved=255 max level delta=255 dac bytes=768) from black=yes     [t≈80.5 s, 320x200]
fade ramp: n=42 (entries moved=254 max level delta=255 dac bytes=765) from black=no      [res=title1.pcx, 640x480]
```

## 1. Which arm is which, and the one piece of luck

The first is the case docs/436/438 were about: `320x200` geometry, all 255 entries moved the full range,
**from black** — the fade into stage 1, which the port had been cutting straight through since docs/426.
It played 41 of its 42 steps.

The second armed at the title card (`res=title1.pcx`, and one line later `fb mode -> native 640x480`), i.e. a
full-table move that is probably a *cut* rather than a fade. It was cancelled a line later by
`applied_refresh()` — the mode-change rule "after re-programming the display path, `from` no longer describes
the panel, so drop the ramp" (docs/436 §3) — so it likely never reached the panel. That is the right outcome
for the wrong reason, and it is worth naming: a cancel that fires because a mode change happened next door is
not a policy about cuts, and the next boot could well arm a title fade that *does* run. The honest position is
still that content detection cannot tell a cut from a fade; if the title melt starts looking like soup, the
off switch is `FADE_RAMP=0`, and the real answer is a signal inside `src/f` (the fade functions themselves).

The counter that would make this a measurement instead of an inference — ramps cancelled, with the reason — is
not in `0.2.13`. It is cheap and it is the last piece of this thread I would add before calling it settled.

## 2. What "42 steps" turned out to mean

`0.2.13` advanced **one step per present**, so the ramp's length in seconds was whatever the frame rate
happened to be. This boot's `320x200` phase measured 24 f/s, so the fade on the panel was ~**1.75 s** —
against the original's measured **0.6–0.83 s** (docs/436). In the 41-f/s phase the same fade would have been
1.0 s. A fade whose duration is a function of the frame rate is calibrated against nothing, and I had picked
`n = mx/6` believing it was a duration.

`0.2.14` makes progress wall-clock:

```c
kw = (elapsed_us * n) / (FADE_RAMP_MS * 1000)      /* FADE_RAMP_MS default 700 */
if (kw <= k) return 0;                              /* this present carries no new step */
```

so the ramp finishes in ~0.7 s regardless of whether the machine is managing 24 or 41 frames a second, steps
arrive in whole numbers, and the last one lands exactly. The log now closes each ramp with its measured
length, so "how long is our fade" is a lookup instead of an arithmetic argument:

```
fade ramp done: %.2f s over %u steps (target %u ms, %u present(s) carried no step)
```

`fade runs=.. arms=.. steps=.. n=.. chg=last/max mx=last/max w=.. fr=..` still reports the discrimination
itself, and this boot showed the other half of the picture in it:

```
fade runs=24  arms=0  chg=246/248 mx=8/16     ← 248 entries moved, but only 8-16 levels
```

That is a fade the game is *already* spreading across presents — 248 entries moving a little every frame —
which the rule correctly left alone. So some fades were visible all along; the ones that were not are the ones
that arrive as a single large move, which is exactly what the ramp now catches.
