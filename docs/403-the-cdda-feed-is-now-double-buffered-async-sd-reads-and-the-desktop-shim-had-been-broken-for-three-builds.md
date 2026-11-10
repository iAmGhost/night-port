# 403 — the CD-DA feed is now double-buffered async SD reads, and the desktop shim had been broken for three builds

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* async never completed on hardware; `DISC_ASYNC=0` ships (docs/404).


Core **0.1.83**, card ELF md5 `91eb3e0452540419412bc1ae40a5d6fb` == build, `core.json` == ELF string, 29 ini keys,
`.bss` 20 053 612. Repo `make test` = `== ALL PASS ==`. `src/f` untouched.

## 1. What the logs said the problem was
From the live region of the 0.1.81 log (docs/402 §2) and 0.1.77's live region:

| | ops | sectors | sectors/op | mean_op | per sector |
|---|---|---|---|---|---|
| small-op shape | 9 666 | 18 824 | **1.95** | 3 878 µs | 1 989 µs |
| bigger-op shape | 397 | 3 453 | 8.7 | 32 518 µs | 3 738 µs |

Two things follow. The feed is **marginal** (172 KB/s delivered against 176.4 KB/s required), and every one of those
reads is **blocking** — the core sits in `fread` on a data slot while the mixer's looping voice eats the ring and
nothing refills it. Bandwidth is not the binding constraint (0.3–0.5 MB/s available against 176 KB/s needed);
**overlap** is. docs/402 §4 framed the lever as "fewer, bigger operations"; that is half right — the operations
should be bigger, but the win is that they stop being stalls.

## 2. The API the OS already provides
`of_file.h`:

```c
int  of_file_read_async(int slot_id, uint32_t offset, void *dest, uint32_t length, void (*callback)(int, int));
int  of_file_async_busy(void);
uint32_t of_file_async_max_read(void);      /* 65 536 B = 27 sectors on this OS */
uint32_t of_file_dma_stage_size(void);      /* 1 048 576 B */
```

*"Start a non-blocking file read from a data slot… Only one async read in flight at a time (bridge limitation)."*
One in flight is exactly the shape of a double buffer: consume one while the other fills.

## 3. What was built
`of_disc.c`: `of_disc_read_audio_async(disc_lba, dest, nsectors)`, `of_disc_async_busy()`,
`of_disc_async_max_sectors()`. Accounting bills the read at the **falling edge of busy** — issue-to-completion —
because billing the issue call would report microseconds and claim a speed-up that never happened.

`of_cdda.c`: two static stages of 27 sectors (`63 504 B` each, 127 KB of `.bss`). `stage_issue()` keeps a read in
flight in every empty stage, bounded by `room_before_voice()` **minus sectors already staged**, so staged sectors
are never double-promised against the ring. `stage_take()` polls the busy flag rather than waiting, and returns
nothing if neither stage has landed. The de-interleave, the ring write and the rig's per-sector witness are the
existing code, so `cdda.raw` still records the same sector bytes in the same order.

Deliberately **not** `of_file_dma_stage_alloc()`: the SDK documents that a destination in normal SDRAM is copied
from its internal CRAM0 bounce buffer on completion, and one copy per 27 sectors is a cost I can see and measure.
A second allocator to get right is not.

**The priming stays synchronous** — that is the only thing that puts the ring ahead of real time, and 0.1.75's
one-shot priming is the shape that measured `dry=0 audio_low=0` (docs/402). Async takes over the steady-state feed,
which is where the 75-blocking-reads-a-second were stalling the pump.

**Two fuses**, because a prefetch that fails would otherwise stop reading the disc entirely — a worse failure than
the stall it was meant to remove:
- 8 refusals with nothing issued → revert to synchronous reads, logged.
- 32 fills in a row finding nothing landed → revert to synchronous reads, logged.

`DISC_ASYNC=1` ships; `DISC_ASYNC=0` is the exact 0.1.82 shape, one ini line.

## 4. The regression this caught: the desktop shim had not built since 0.1.81
```
main_of.c:686: error: call to undeclared function 'copy_row'
```
`copy_row` (the word-at-a-time row copy, 0.1.81) was inserted inside the `#ifndef OF_PC` stage block, but the blit
branches that call it compile for both hosts. The RISC-V ELF linked fine, so every deploy since 0.1.81 shipped
while **the desktop shim was broken** — the host the cross-host diffs (docs/365) and accuracy sweeps (docs/382)
measure against. Cause: I stopped running `pocket/src/night make test` and ran only the repo fixture suite plus the
device build. This is docs/386 §5's rule biting again: a passing RISC-V build is not evidence the core is whole.
`copy_row` moved out of the guard; `pocket/src/night make test` builds both halves green; the shim runs again
(`--warp-stage 1 --frames 1200`: `audio_low=0 dry=0 room_hold=0 ahead_min 1.44 s`, 157 ops / 1 860 sectors).

## 5. What the card log decides — no other evidence counts
New 1 Hz line:
```
cdda async issues=… waits=… falls=… staged_sectors=… stage=63504 B max=27 sectors
```

| measure | 0.1.82 baseline | async working looks like |
|---|---|---|
| `disc sectors/op` | 1.95 | ~27 (one operation per 27 sectors = 0.36 s of audio) |
| `disc mean_op` | 3 878 µs (issue-to-completion, honest) | similar or larger per op, but **overlapped** instead of blocking |
| `cdda fills … mean/max` | 4 787 / 418 281 µs | mean should drop hard: a fill no longer pays for the read |
| `dry`, `audio_low` | 0, 0 | must stay 0 |
| `ahead_min` | 19 715 frames (0.45 s) | should rise toward the ring's 4 s |
| `async falls` / `waits` | — | 0 or a handful; hundreds means prefetch is losing and the fuse reverted the track |

If `waits` and `falls` stay near zero and `fills mean` collapses, the stall is gone and the remaining audio problem
is the game's own track re-requests (docs/398), not the feed. If `waits` climb, the SDK's one-read-in-flight limit
plus a 27-sector read is not enough overlap, and the answer is a bigger staging strategy in `dma_stage_size`
(1 MB = 5.9 s of audio) rather than another scheduling tweak.
