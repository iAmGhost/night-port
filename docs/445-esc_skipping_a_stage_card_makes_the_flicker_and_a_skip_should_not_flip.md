# 445 — ESC-skipping a stage card is the flicker's trigger, so a skip must not flip

## 1. The player found the trigger I could not

Four builds of chasing this (docs/423/424/426/443) had it as "a flicker at stage entry". The actual repro is:

> *"seems stage entering flicker occurs only when I skip stage cards with esc"*

That single qualifier raised the odds on docs/443 §2 (skip, then flip anyway) and is the reason this closes today.
It also explains why it survived `0.2.0`: a skip is invisible **as long as every display buffer eventually
holds the same picture** — which a long, un-skipped hold guarantees. ESC breaks the hold mid-card, the buffers
still disagree about which scene they hold, and the skip-then-flip presents one that does not.

## 2. The A/B, and what the pixel columns showed

`devini.sh PRESENT_SKIP=0` (ini only) on `0.2.17`, same play pattern:

```
skips column: first 0 last 0  -> copies happened ALL
frames where the game was bright but OUR surface was black: 0
  f=433  320x200  sum=54202  smean=43.8  pmean=44.9
  f=434  320x200  sum=58796  smean=43.8  pmean=44.9
  f=435  320x200  sum=63021  smean=43.8  pmean=44.9
```

**The flicker is gone.** And through the same fade the panel surface now tracks the game picture to within one
unit of mean luminance (`smean` 43.8 vs `pmean` 44.9) with not a single frame of "game bright, surface black".
So the black was never content and never the OS: it was us **presenting a buffer we had not written**.

## 3. The fix, and why `PRESENT_SKIP` cannot simply stay off

`PRESENT_SKIP` bought the logo hold — 41.0 s → 6.00 s (docs/417, and it shipped in `0.2.0`). Leaving it off to
cure the flicker trades a real win for a cosmetic one. It is also the wrong conclusion: the win came from
making each poll *cheaper*, and the flip is part of that cost. So `0.2.18` keeps the skip and fixes its
consequence:

```c
    if (!skip || g_pal_pending) {   /* write only when something new must reach the panel */
    ...
    if ((skip || smp) && !g_pal_pending) { g_present_noflip++; }   /* nothing new: not pixels, not colours */
    else { of_video_flush(); of_video_flip(); ... }
```

A skipped frame now neither writes **nor flips** — the panel keeps showing what it was showing, which is
exactly what "identical to last time" means. The one exception is a palette the panel has never seen
(`g_pal_pending`): a fade changes colours while the surface sits still (docs/443 §1 measured precisely that),
and letting the palette stall behind a skipped flip would turn a fixed flicker into a frozen fade. That flag
is recorded *before* `palette_push()` uploads, because under `PALETTE_AFTER_FLIP=1` the upload happens after
the decision, so the function's return value cannot carry it.

## 4. A caveat on `hold=`, which I have been quoting

In the zero-skip run the hold meter reported `max=2989 ms`, which **its own definition cannot produce**: with
`PRESENT_SKIP=0` every present is a visual present, so the gap it measures is ~33 ms. The block sits after the
`PRESENT_CAP_MS` early return (docs/441), so capped visits bypass it and the meter also counts *cap
starvation*. What that means for the record: my earlier "our holds run ~7 s against the original's 6.0–6.5 s"
conflates two quantities, and the hold decision should not be made on it. The **fade durations** do not come
from that meter — they come from the `FDUMP` luminance ramps (612–892 ms against the original's 600–830 ms,
docs/444's table) — and those stand.

## 5. What the next boot has to show

| expectation | if it fails |
|---|---|
| flicker gone **with `PRESENT_SKIP=1`** | the skip is not the whole story; `smean`/`pmean` say where the black lives |
| logo hold still ~6 s, not back to 41 s | the hold win came from the flip after all, and the two are not separable |
| `noflip=` climbing during static holds | the skip path is not running, and the A/B measured nothing |
