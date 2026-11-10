# 397 — the prefill cut traded a rare stall for real underruns, and what the playable build actually did

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* the 0.1.74 change it describes was reverted in 0.1.75.


0.1.74 (prefill 150 -> 24 sectors, lookahead 120 -> 240) made the device **worse**, as reported.  Its own log says so.

| counter, last budget sample | 0.1.73 | 0.1.74 |
|---|---|---|
| `cdda fills … max=` | 423 998 us | **131 128 us** (the stall did shrink) |
| `cdda fills … mean=` | 3 831 us | 4 250 us |
| `audio_low` | 0 | **39** |
| `dry` | 0 | **33** |
| `ahead_min` | 16 082 (0.36 s) | **0 (0.00 s)** |
| `room_hold` | 9 055 | 3 806 |
| budget samples with `audio_low>0` | 0 | **35** |
| `disc ops / sectors` | 6 177 / 9 179 | 1 777 / 2 883 |

The stall got smaller and the audio started *underrunning*: the ring ran out of buffered audio outright, which is a worse
and more frequent artifact than one 424 ms hitch.  Priming less was the wrong lever.  0.1.75 reverts it (prefill 150,
`CD_LOOKAHEAD_SECTORS` 120, ini `DISC_PREFILL_FIRST=150 DISC_BATCH=12 DISC_MIN_WANT=12`).

## What the build the user remembers as playable actually did

`git show 4e7e267:pocket/src/night/of_cdda.c` (the commit whose log line is "the port is playable on hardware"):

```c
uint32_t prefill_sectors = CD_RING_FRAMES / CD_FRAMES_SECTOR;   /* 300 sectors = 4 s */
#define CD_LOOKAHEAD_SECTORS 120
#define CD_CHUNK_SECTORS  12
```

and `of_disc.c` at that commit has **no `of_disc_read_audio_run` at all** -- every read was one sector.

So the playable shape is *prime the entire 4 s ring before the voices start, then read in small pieces*.  It accepted a
large one-frame hitch at track start in exchange for 4 s of slack for the rest of the track, and nobody heard the hitch
because it happens where the game is changing tracks.  My 0.1.73 (prime 2 s) and 0.1.74 (prime 0.4 s) both moved away
from that shape, and the 0.1.74 end of that slope is the underrun above.

## The fix this points to, which is neither of the two I have tried

Keep the priming deep -- 300 sectors, the whole ring, like the playable build -- and **spread those reads across
frames** (docs/376 already named this as the next candidate).  A priming ramp of e.g. 24 sectors per pump visit reaches
4 s of buffer in about 13 visits (~27 ms of wall time spread over a few frames) instead of one frame paying 600 ms, and
never leaves the ring under-filled while it happens.  That is the shape to measure against these counters: `dry` and
`audio_low` must stay 0, `ahead_min` should rise toward 4 s, and `cdda fills max` should stop being a four-digit
millisecond number.
