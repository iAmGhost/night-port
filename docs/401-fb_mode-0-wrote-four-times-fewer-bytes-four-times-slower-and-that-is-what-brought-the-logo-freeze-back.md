# 401 — FB_MODE=0 wrote four times fewer bytes four times slower, and that is what brought the logo freeze back

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* `FB_MODE=1` / `PRESENT_SYNC=0` ship; docs/405 records the same shape being re-shipped and freezing the logo again.


0.1.80 on hardware: the picture glitch was the thing to fix, and the run became so slow it read as **stuck on the
logo**. The log prices both of 0.1.80's guesses.

## 1. `FB_MODE=0` was the wrong bet, measured

`stages last … c=82374` and `max all=101062 copy=82657` with `surface=320x240 stride=320`:

| shape | bytes written per frame | copy stage | effective rate |
|---|---|---|---|
| 0.1.79 `FB_MODE=1` (native 640×480, row memcpy) | 307 200 | 15 771 µs | **19.5 MB/s** |
| 0.1.80 `FB_MODE=0` (OS 320×240, software 2×2 downscale) | 76 800 | 82 657 µs | **0.93 MB/s** |

I argued "four times fewer aperture writes, and the downscale reads cached SDRAM". The byte count was right and the
conclusion was wrong: `downscale_2x2_frame` costs about **1 µs per output pixel** — ~100 CPU cycles per written byte
at 100 MHz. Its cost is the per-pixel gather (four source reads, the index-majority vote, the palette tie-break)
thrashing an 8K/32K cache, not the writes. A plain `memcpy` of four times as many bytes is twenty times faster.

`PRESENT_SYNC=1` was the second bad guess: the flip stage went **18 294 µs → 33 436 µs** max, waiting on a swap
while the frame was already paying tens of milliseconds.

Present cost went 15 771 µs → **90 081 … 116 050 µs mean**, which is ~10 frames/s. That is why the logo looked
frozen: the port's waits are wall-clock deadlines (docs/335), so a hold that should take 2.5 s of company-logo
animation is stretched by whatever the frame costs, and at 90 ms a frame the animation is effectively stopped.
The logo code was not stuck — the display path was.

## 2. 0.1.81's shape

| knob | value | reason |
|---|---|---|
| `FB_MODE` | **1** (reverted) | the native 640×480 surface + row memcpy is the measured faster path, 19.5 MB/s vs 0.93 MB/s |
| `PRESENT_SYNC` | **0** (reverted) | 15 ms of flip waiting bought nothing while present cost 90 ms |
| `BLIT_DIRTY` | **0** (kept) | partial update is invalid on the triple-buffered display path — docs/400. The glitch fix stays fixed |

New lever, and the only one left that does not touch frame completeness: **`copy_row()` writes four bytes per
store** where the row length and both pointers are 4-aligned (which they are: strides 320 and 640, row widths 320
and 640), falling back to `memcpy` otherwise.

```c
HOTPRESENT static void copy_row(uint8_t *d, const uint8_t *s, uint32_t n) {
    if ((n & 3u) == 0 && (((uintptr_t)d | (uintptr_t)s) & 3u) == 0) {
        const uint32_t *sp = (const uint32_t *)s; uint32_t *dp = (uint32_t *)d;
        for (uint32_t i = 0, c = n >> 2; i < c; i++) dp[i] = sp[i];
        return;
    }
    memcpy(d, s, n);
}
```

Applied to all four whole-frame blit paths (native 640×480, the 320×200 fit, the unexpected-mode copy, and the
dirty-commit path that now ships disabled). The rationale is the measured 19.5 MB/s = ~5 cycles per byte: if the
aperture is transaction-limited rather than byte-limited, a 32-bit store is one transaction where `memcpy` spent
four. Every row is still written every frame.

Card: core **0.1.81**, ELF md5 `b82d8fccc5de71742eab536c3b0701e1` == build, `core.json` == ELF string, ini
`FB_MODE=1 PRESENT_SYNC=0 BLIT_DIRTY=0 BLIT_SKIP=0 SCREEN=0 FIT=1 TICK_HZ=490 DIAG=0 LOG=1`.
Repo `make test` = `== ALL PASS ==`. `src/f` untouched.

## 3. What the next log decides

- `perf pres=` should fall from 15 771 µs if 32-bit stores are cheaper per transaction. If it barely moves, the
  aperture is not transaction-limited and the remaining options are structural, not micro: draw the game's picture
  into the surface once per *changed* frame with a per-buffer shadow (three shadows rotating with the flip), or ask
  the OS to composite (`OF_DISPLAY_OVERLAY` semantics), or accept ~19 MB/s as the panel feed rate and reduce the
  number of presents per second instead of the bytes per present.
- `stages last … c=` is the number to compare against 15 771, and `f=` should return to the ~18 ms ceiling.
- Chopiness and the logo hold are the same measurement here: the hold is wall-clock, so `pres` decides whether it
  animates.

## 4. The counting of this thread

Three shipped changes aimed at the picture: partial update (created a defect, reverted by reasoning, docs/400),
`FB_MODE=0` (created a 5× slowdown, reverted, this doc), word-at-a-time stores (untested on hardware at the time
of writing). The two that were wrong share one shape: both were argued from a model of the cost rather than from a
measurement of it, and both were wrong in the direction the model predicted. The measurements that were right —
the priming ramp fixing the music pause, `staged_frames=0` exposing a feature that never ran — all came from
counters the device wrote.
