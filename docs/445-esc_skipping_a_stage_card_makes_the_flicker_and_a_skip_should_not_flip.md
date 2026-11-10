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

## 4. Retraction: my caveat about `hold=` was the thing that was wrong

In §2's zero-skip run the hold meter reported `max=2989 ms`, and I wrote that "its own definition cannot
produce" it. It can, and it should. The block does sit after the `PRESENT_CAP_MS` early return, but during a
3-second stage-entry stall the game is **blocked on reads and never calls `present_step()` at all**, so there
is nothing to bypass: 2 989 ms is a real stretch in which nothing reached the panel — exactly the stall
docs/434/435 measured and found the original also holds. The meter reads **"time since the last frame that
actually reached the display"**, which is precisely the quantity a player complains about.

So the original comparison stands, and my caveat was the error: `hold max=7018 ms` (`0.2.16`, skip on) against
the original's ~6.0–6.5 s card (docs/416) is a **~10 % long hold**, not a phantom. For contrast, this `0.2.18`
run shows `hold max=92 ms` — because the player was ESC-skipping cards, so the cards never sat. The fade
durations (612–892 ms against the original's 600–830 ms) are unaffected either way: they come from the `FDUMP`
luminance ramps, not from that meter.

## 5. What the next boot has to show

| expectation | if it fails |
|---|---|
| flicker gone **with `PRESENT_SKIP=1`** | the skip is not the whole story; `smean`/`pmean` say where the black lives |
| logo hold still ~6 s, not back to 41 s | the hold win came from the flip after all, and the two are not separable |
| `noflip=` climbing during static holds | the skip path is not running, and the A/B measured nothing |

## 6. `0.2.18` on hardware: closed, and the numbers that say so

> *"stage entering flicker is gone"* — with `PRESENT_SKIP=1`, i.e. with the win still switched on.

| expectation from §5 | what the log shows |
|---|---|
| flicker gone with the skip on | gone (the player), and `noflip=` proves the new path is the one running |
| logo hold still ~6 s, not 41 s | the `640x480` phase runs `wall=2368ms → 7250ms` (`fb mode -> game 320x240` at 7 250 ms) ≈ **4.9 s** |
| `noflip=` climbing on static holds | `noflip=167` cumulative, non-zero in **17 of 36** seconds, peak 26/s |
| a fade must not stall behind a skipped flip | 16 consecutive-dump palette steps > 500 in the trace — fades still animate while flips are skipped |

`noflip` is the important column: it is the evidence that the frames in question really did skip **both** the
write and the flip. `cap=` is far larger (5 267) — that is `PRESENT_CAP_MS` doing most of the "do nothing"
work, which is fine; they are different mechanisms and only one of them used to present a stale buffer.

The mean across the run is `frames/s mean 21.1`, `hold max=92 ms`, 8 stall witnesses (the stage-entry reads,
docs/434/435).
