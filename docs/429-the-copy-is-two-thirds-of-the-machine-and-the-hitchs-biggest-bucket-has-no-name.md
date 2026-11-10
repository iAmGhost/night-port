# 429 — the 0.2.5 boot: the copy is two-thirds of the machine, the hook is dead for real, and the hitches' biggest bucket still has no name

The accounting line worked. One 168-second run, 4969 frames, `core 0.2.5` stamped in the boot marker, and for
the first time the whole wall second is divided up instead of inferred.

## 1. Where the run's time went

Cumulative over the session:

| bucket | ms | share | what it means |
|---|---|---|---|
| `wall` | 168 500 | 100% | the session |
| `pres` | 112 276 | **66.7%** | inside `present()` — palette, surface copy, flush, flip |
| `rest` | 42 186 | **25.0%** | everything outside the three below |
| `disc` | 13 826 | 8.2% | blocking CD reads on the music path only |
| `pump` | 211 | 0.1% | the game's 490 Hz tick loop, 78 918 ticks |

Frames/s: min 9, median 33, max 48, average 29.5. `skips=3043` of 4969 frames = 61% skipped.

Two things fall straight out of this.

**The tick loop is not the cost.** 211 ms over 78 918 ticks is **2.6 µs per tick**. Whatever `fn_03ee88` is,
it is not where the game's drawing and decoding happen — so the game's real work lives in `rest` or inside
`present()`, and any theory built on "the delay loop is doing the game's work" is looking in the wrong
function.

**A good second is a saturated second.** For the 115 seconds at or above the median frame rate:

```
pres ≈ 1010 ms   disc ≈ 0 ms   rest ≈ 13 ms
```

i.e. essentially the entire second is inside `present()`. That is the frame-rate ceiling: at `pres` mean
22.6 ms, nothing else being asked, the port tops out around 44 fps, and that is what it does. The picture is
the machine.

## 2. The hitches have two partners, and neither is the one I named before

For the 45 seconds more than 5 frames below the median:

```
pres ≈ 461 ms    disc ≈ 155 ms    rest ≈ 408 ms
```

So a hitch is *fewer frames* (461 ms in present instead of 1010) with both `disc` and `rest` climbing — and
**`rest` is the larger of the two.** docs/425's headline ("the stutter is blocking SD reads for the music
feed") is at best half the story: disc reads are present in the bad seconds (median 155 ms, max op 122 ms)
but the bigger moving bucket is the 25%-of-everything bucket I cannot yet name. `rest` is the game's main-loop
compute, resource file reads (PCX/.fli go through stdio and are **not** counted in `disc`, which measures only
the music path), the mixer/SFX pump, and log writes. Naming it is the next instrument, not another theory.

## 3. The idle hook is dead, and now that is a measured fact rather than a knob I forgot

```
disc_idle_hook=1 registered=1     ...    idle hook: presents=0   (all 161 samples)
filecaps: svc_count=106 idle_hook=1 async_max=65536 dma_stage_total=1048576 stage64k=0x30500000
```

`devini.sh` had set the knob before this boot, the settings line proves it reached the core, the service table
says the OS has the hook, and the kernel called us zero times in 168 seconds of music-playing gameplay. Two
builds, armed both times, same answer. The remaining explanation is the boundary one from docs/427: our reads
are stdio (`fopen("slot:N")` + `fseek` + `fread`), and this hook is documented against the kernel's own DMA
wait. **The path forward is not a better hook; it is reading the disc through the OS call the hook wraps, or
giving up on it** — and docs/404 already recorded that the async route never completes on this hardware. So
the honest state is: three read-side mechanisms tried (batch, defer, hook), and the only one that changed
anything was batching.

`CDDA_STOP_FIX=0` held up as predicted too: **zero** `stop by=` lines in the whole run (vs 3-4 when the
bookkeeping stops were allowed), with `play idx=` events intact.

## 4. What `PRESENT_SKIP` actually bought, restated

61% of frames skipped and the median is still 33 — because skipping does not make the *unskipped* frames
cheaper, and a skipped frame still pays the hash:

```
surfhash: 76800 B read in 2207 us (34.8 MB/s)   [a 640x480 copy costs ~17500 us; a skipped frame still pays this]
```

At 640x480 that hash is ~8.9 ms against a ~17.5 ms copy, so a static slide goes from 17.5 ms to 8.9 ms per
frame (the logo/slide win docs/422 measured, 41 s → 6 s) while gameplay, which skips 1.8% of frames, is
copy-bound and unchanged. That is the correct, narrow claim for the feature: it is a holds optimization, not a
frame-rate one.

## 5. Next step, in order of what the data says

1. **Attribute `rest`.** Split it into game compute / resource file reads / audio+SFX pump / log writes. It is
   25% of the machine and the largest swing in a hitching second.
2. Only then decide whether the frame ceiling (present-bound at ~44 fps) is worth attacking, since the
   remaining ideas there are the expensive ones: a per-display-buffer dirty-row map (docs/400's condition for
   partial updates on triple buffering), or living with it.
3. Close the disc-hook thread with the boundary test, not another hook variant.
