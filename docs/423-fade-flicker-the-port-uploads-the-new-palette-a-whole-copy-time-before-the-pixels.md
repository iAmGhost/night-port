# 423 — fade flicker: the port uploads the new palette up to a whole copy-time before the pixels that need it

Reported from the hardware: "flicker while starting stage-1 (fade-in)", on `0.2.0` with `CDDA_STOP_FIX=0`.
Two things came out of the card log before any code changed, and one of them exonerates the thing the last
release turned on.

## 1. The log clears `PRESENT_SKIP` for this symptom

`boot C` (`0.2.0`, `CDDA_STOP_FIX=0`, 1393 log lines). The stage-1 entry window, sampled at 1 Hz:

```
   t_s  view       pal_lit  d_pal  sig_chg  pushes  skips
  16.86  640x480       255    +0        9      247     501
  17.87  640x480       255    +0        9      247     556
  19.04  320x200         0  -255       10      298     556      <- stage 1: black, palette ramp starting
  20.22  320x200       255  +255       10      341     558      <- the fade's whole second: 43 pushes, 2 skips
  21.23  320x200       255    +0       11      342     604
```

During the fade, `skips` moved by **2** while `pushes` moved by **43**. The copy path was essentially never
skipped while the fade ran: the picture is changing enough that the surface hash never repeats three times.
So the flicker is not the surface skip. It is in the palette path, which this log says was running ~43 times a
second in that window.

Same log, the audio decision from the previous boot series:

```
cdda EVENT play idx=1  tick=2920    cdda EVENT play idx=9   tick=4560
cdda EVENT play idx=11 tick=5549    cdda EVENT stop tick=6245 by=card-rule
cdda EVENT play idx=2  tick=9202
```

`by=port-bookkeeping` appears **zero** times, where boots A/B had three or four. `CDDA_STOP_FIX=0` is doing
what docs/421 said it should on real hardware: the invented stops at `fn_000405a7` are gone. The one stop that
remains is the intentional card rule (tick 6245 → 9202 = 6.03 s on this route; boot A's was 2.01 s, and the
difference is that this run's stage track started later, not that the rule changed). No asserts, no read
failures; device disc cost is its usual self (`mean_op=33 ms`, `max_op=111 ms`).

## 2. Bug: the palette-change hash sampled 37 of 256 entries

```c
for (int i = 0; i < 256; i += 7) { h ^= g_pal32[i]; h *= 16777619u; }
if (g_pal_force || h != g_pal_hash || (g_frames & 4095u) == 0u) of_video_palette_bulk(g_pal32, 256);
```

To avoid paying for a 256-entry upload on every frame, the device only uploads when a hash of the palette
changes — and it hashed indices 0, 7, 14 … 252. **37 entries out of 256.** A ramp that touches entries the
sampler misses leaves the hash untouched, the upload is skipped, and the panel holds the old colours until the
forced refresh every 4096 frames — about **78 s** at 52 frames/s. The full 256-entry hash costs ~3 µs at
100 MHz next to a 2–19 ms copy, so there was never a trade to be had here. Fixed unconditionally in `0.2.1`.

## 3. The prime suspect for the flicker, and the knob that tests it

`palette_push()` runs at the **start** of `present()` — before the copy, before `of_video_flush()`, before
`of_video_flip()`. So the sequence per frame is:

```
new palette -> panel   ...   [copy 2-19 ms]   ...   flush   ...   flip -> new pixels
```

For the whole duration of the copy, the panel is showing the **previous** picture in the **next** frame's
palette. On a static screen that is invisible. On a fade it is a wrong-colours window **every single frame**,
43 times in stage 1's fade second — which is what "flicker while starting stage-1" looks like from the couch.

`PALETTE_AFTER_FLIP=1` moves the upload to immediately after `of_video_flip()`, so the palette lands with the
pixels it belongs to instead of a copy-time ahead of them. Default stays 0 so the boot measures exactly one
change. Note the shim is deliberately unaffected: `OF_PC` always pushes at the original site, because moving
it there changes what a present costs on the host that every cross-host diff is measured against
(docs/365, docs/382).

## 4. If the flicker survives that change

Then the ordering was not it, and the remaining candidates in order of likelihood:

- The bulk upload itself is not atomic against scanout, so any palette change mid-frame bands. The lever is
  `of_video_wait_flip()` on frames whose palette changed — paying the (expensive, docs/400) sync only during
  fades instead of every frame.
- The stage-1 entrance draws as well as ramps, and the flicker is the game's own — which would mean comparing
  against a container capture of stage-1 entry, the way docs/415/417 did for the holds.

Worth saying plainly: this ordering has been in the port since the palette gate existed, so the flicker
predates `PRESENT_SKIP` and predates the audio change. The two recent changes were the first things to look at
because they were the only things that had changed — the log said otherwise.
