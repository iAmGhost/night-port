# 424 — the fade flicker is not the palette ordering; the shared logic fades clean, so the port now traces its own presents

Boot D: `PALETTE_AFTER_FLIP=1` on hardware. **The flicker was unchanged.** So the ordering theory — the new
palette landing a copy-time before its pixels — is not it, and neither is `PRESENT_SKIP` (docs/423 showed the
copy path is barely skipped during the fade at all).

## 1. The shared game logic is not flickering

Before touching the device path again, measure the fade where it can be measured deterministically: the shim's
`frames.bin` carries a 40x30 luminance thumbnail of every presented picture, so the ramp shape is a number.

```
shim, warp to stage 1, first 40 frames of the 320x200 phase:
  rising steps 37, DROPPING steps 0        (0 = black, 255 = fully lit; steps > 1 LSB counted)
```

A strictly monotonic ramp. The port's own game code fades stage 1 in cleanly, so what the eye catches on the
Pocket is something in the *presentation*, not the content. That retires the hypothesis that the game is being
sampled mid-ramp, at least as far as luminance can tell — a half-ramped palette that kept total brightness
monotonic would slip past this measure.

## 2. Why this cannot be answered by dumping pixels

The obvious instrument is "write the frames to the card and look at them". It is not available:

- `nightlog.sav` is a fixed 262 144-byte slot (docs/402), so there is no room for frames in the one write path
  that is proven to reach the card.
- The only other proven path is the save slots — and docs/393 and docs/394 are exactly the record of
  diagnostics landing in `night_0..night_6.sav` and being a problem. Megabytes into game saves is out.

So measure the *thing that changes during a fade* instead of the pixels: the palette. 60 frames of
`~90 B` fits the log slot with room to spare.

## 3. The instrument: L1 + Up traces the next 60 presents

`0.2.2`. Holding L1 and pressing Up arms 60 presents; each one logs what the port actually held at the moment
it presented:

```
fdump f=2213 t=43981 320x200 palh=7c1f9a02 ndiff=  7 lit=132 sum=118842 src=0 pushes=874 skips=604
```

| field | meaning |
|---|---|
| `palh` | hash over **all 256** palette entries (docs/423 fixed the sampler) |
| `ndiff` | how many of the 256 entries differ from the previous armed present |
| `lit` | entries with any colour in them — the fade's progress |
| `sum` | total RGB of the whole table — the ramp's height |
| `src` | 1 = the surface bytes moved this present (from the skip path's hash, which is already paid) |
| `pushes` / `skips` | the cumulative counters, so a per-frame delta can be checked against them |

Note Up is an arrow key to the emulated game, so pressing it also moves a menu entry — the same unavoidable
trade as the save/load combos (docs/334), and said so in the code comment.

## 4. What each outcome will mean

- **`sum`/`lit` move backwards, or `ndiff` is large on many frames.** The palette the port holds at present
  time is not a clean ramp — i.e. `present()` is running at an arbitrary point relative to the game's frame
  construction, catching it mid-update. The lever is then *when* we present, not what we push.
- **`ndiff = 0` for stretches while the panel visibly changes.** Something updates colours behind the back of
  our snapshot; would have been impossible while the sampler covered 37 entries, and is a real finding now
  that it cannot be.
- **The device trace reads as clean as the shim's.** Then the content is provably clean on hardware too, and
  the flicker is the panel's reaction to ~43 palette uploads a second. The levers there are bounded: rate-limit
  palette uploads during a fade, or `of_video_wait_flip()` only on frames whose palette changed — paying the
  docs/400 sync cost for a second instead of a whole run.

Either way the guessing stops here: the next report about this symptom comes from the device's own numbers.
