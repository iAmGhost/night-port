# 438 — a byte count cannot name a step count: why the fade ramp armed zero times, and what it now detects

`0.2.12` existed to answer one question — which of three premises was false — and it answered it in one line:

```
fade ramp: setting=0                                   (boot banner)
fade runs=4620 arms=0 steps=0 n=0 dwmax=2304 dw=0 w=586806 fr=1     (perf, final second)
```

## 1. Two premises were fine; the third was mine

**"The setting never reached the app" — false.** `fr=1` on every perf line. The banner's `setting=0` is my
own trap: the boot marker is logged *before* the ini is parsed, so a setting printed at that moment is always
0 and looks exactly like the failure it was meant to detect. A diagnostic that is confidently wrong at boot is
worse than no diagnostic, because it ends the investigation. The banner now says nothing; the perf line's
`fr=` is the reading.

**"The DAC counter does not move on device" — false.** `w=586 806` DAC writes over the run, and `runs=4620`
proves the check itself executed on every present. Hypothesis 3 was dead too, three seconds in.

**What failed was my derivation.** `0.2.11` counted DAC bytes between presents, divided by 768 (one full
256-entry table) and required at least 4 such tables. Hardware's largest interval was **2 304 bytes** — under
that arithmetic, "3 tables", below the threshold, no arms. But 2 304 is also exactly **~48 steps of a
16-entry block** (48 B × 48), i.e. the fade I went looking for. The game programmes as many palette entries
per step as it likes; `board_out(start, n, buf)` is `n` entries, and `n` is the game's choice. A byte count
therefore cannot name a step count, and I had built a threshold on the one quantity that was never the step
count. That is a design error wearing a threshold's clothes: it looks tunable, and lowering `FADE_MIN_STEPS`
to 3 would have "fixed" it while leaving the same wrong idea in place.

## 2. What the panel can actually be asked: the move, not the writes

`0.2.13` detects the **move** in the palette the game is asking for: how many entries differ from the table
the previous present saw, and how far the largest one moved.

```
chg = entries differing,  mx = max per-channel level delta (of 255)
arm when  chg >= 8  and  mx >= 24
ramp length n = clamp(mx / 6, 12, 60)   presents        (25 presents ~ 0.6 s at 41 fps == the original's ramp)
```

`g_pal_prev32` is updated *before* any return from `palette_push()` — an early return from the ramp branch
that skipped it would make the next interval measure nothing, and that bug would look like "fades stopped
happening" rather than like a missing memcpy.

Two honest things remain. The ramp length is now derived from the *size* of the move rather than the game's
step count, which is no longer observable from the panel side — so 12..60 presents is my calibration against
docs/436's measured 0.6–0.83 s, not something the game declared. And content detection will arm on a *hard
cut* as readily as on a fade (both are "many entries moved a long way"); if the slide transitions now fade
where the original cut, the honest description is that the discriminator cannot tell them apart, and the
choice is between leaving `FADE_RAMP=0` or finding a signal inside `src/f` that can.

## 3. Also caught: my build order

Running `make` then `make dist` packaged an ELF built before the version bump; `make deploy` refused to ship
it:

```
FAIL version mismatch {'core.json': '0.2.13', 'night.elf': '0.2.12', 'mk_dist.py': '0.2.13'} -- run make dist, then make deploy
```

The correct order is `make dist` (which regenerates and builds) and then `make deploy`. The guard existed and
worked; what it means is that a "version 0.2.x on the card" claim is only worth what the deploy chain that put
it there checked.
