# 396 — the device log arrives: a 424 ms ring fill, and a BRAM placement that never landed

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


First hardware log that actually contains measurements (128 467 B in `nightlog.sav`, core 0.1.73, `LOG=1 DIAG=0`,
`ab.sh bram-off` shape).  The user's report was "weird music pause persists" -- the log says why, and it also invalidates
docs/371.

## 1. The music pause has a witness

```
budget present n=5368 mean=5213us max=66927us | disc ops=6177 sectors=9179 mean_op=3113us max_op=99103us
       mean_sector=2095us seeks=5 fails=0 kb=21083 | cdda fills=6117 mean=3831us max=423998us
       frames=5397252 visits=26553 audio_low=0 room_hold=9055 clock_behind=2 dry=0 ahead_min=16082(0.36s)
```

- **One CD-DA ring fill took 423 998 us -- 424 ms inside a single frame.**  113 of the budget samples report a fill whose
  worst case exceeded 100 ms.
- **The lookahead bottomed at 16 082 frames = 0.36 s.**  The ring is 4 s and the policy asks for only 120 sectors
  (1.6 s) of slack, so a 424 ms fill lands almost exactly on top of the remaining buffer.  That is the audible pause:
  nothing *underran* in the counters (`audio_low=0 dry=0`), but the slack was eaten while the mixer kept pulling.
- `room_hold` climbed 8 947 -> 9 055 within a minute: the writer is being refused by a full ring, then emptied, then
  refused again.  That oscillation *is* the pause pattern.
- Device SD read cost, first measurement: **mean 3 113 us per 12-sector (28 224 B) batch, 2 095 us per sector, worst
  operation 99 103 us.**  A Mac's SSD was 2 us/sector (docs/375) -- the card is ~1 000x worse per sector, which is why
  every number chosen on desktop was optimistic.

## 2. `in_bram=0`: docs/371's placement never happened

The boot probe (docs/378) answers the question the BRAM thread left open, and not in our favour:

```
sdk limits: async_max_read=65536 B dma_stage_size=1048576 B | mixer voices active=0 | BRAM present@0x10405e00 in_bram=0
```

`app.ld:29` reserves `APP_BRAM` at `0x00004000`, `riscv64-elf-size` reports a `.app_fasttext` of 9 376 B there, and the
SDK still says **`in_bram=0`** with BRAM *at* `0x10405e00`.  So the section we filled was not the fast RAM the OS
recognises: ten hot functions were placed in a region the device does not consider BRAM, and `OF_FASTTEXT`'s `noinline`
+ `optimize(...)` attributes were the only real effect they ever had -- which is a plausible source of the "slightly
laggy" report on its own.  `ab.sh bram-off` (0.1.73, `.app_fasttext` absent) is therefore the honest default: it removes
the attribute damage and nothing real was lost.

## 3. Present cost on hardware, first measurement

`present mean=5 213 us max=66 927 us`, with the copy dominating: `stages … c=2 557 … max all=66 889 copy=65 915`.
Mean present cost is 5.2 ms of an 18.3 ms frame (~28 %), so the port is not frame-bound -- but a 67 ms copy spike is a
visible hitch, and the 65.9 ms max sits in the copy path, not the flip (`f=683`).  `perf pres=3 580us pump=27 874
ticks=57 724 skips=0`: 57 724 ticks over 5 368 frames = 491 tick/s, `skips=0`, so the tick contract held on hardware.

## 4. What async is now worth doing

`async_max_read=65536 B` and `dma_stage_size=1 048 576 B` are the device numbers docs/375 4 said had to exist before
async prefetch could be designed: 65 536 B is 27 sectors per async read, comfortably above the 12-sector batch, and the
DMA stage is 1 MB.  That makes a double-buffered prefetch (issue the next batch while the mixer plays the current one)
buildable rather than speculative.

## 5. The two fixes this log dictates

1. **Cap the sectors read per frame.**  A single frame must not pay 424 ms; the ring should catch up across several
   frames.  A per-visit/per-frame read budget turns a stall into a ramp.
2. **Ask for more slack than 120 sectors.**  `CD_LOOKAHEAD_SECTORS = 120` (1.6 s) is the policy; `ahead_min=16 082`
   (0.36 s) is what the disc actually delivered.  The ring holds 4 s, so the headroom exists and is currently unused.
3. `present max` spikes belong to the copy path (65 915 of 66 889 us), not the flip -- that is the next measurable thing
   after the audio stall.
