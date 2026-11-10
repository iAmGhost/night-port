# 425 — the stutter is blocking SD reads for the music feed (11.7 s of an 81 s run), and the OS turns out to have a hook for exactly that

Asked whether the ~1 s stutter was normal. It is not, and the log says why with its own numbers.

## 1. The stutter is measured

The log emits `memstat frames=N` once a second, so frames-per-second is a difference of two adjacent lines:

```
min 10   median 46   max 55      (n=81 one-second windows)
9 of 81 seconds sit 5 or more frames below the median
```

Then pair each window with the disc work in it — `budget ... disc ops=.. mean_op=..` is cumulative, so the
blocking time in a window is `ops2*mean2 - ops1*mean1`:

```
 frames   disc_ms   ops      biggest_op
     55         0     0             0     <- no music reads: 55 frames
     55         0     0             0
     48       335     6        105582
     10       581    16        120388     <- 581 ms of blocking reads in one second: 10 frames
     45       556    13        120388
     27       422     9        120388
     52         0     0        120388     <- no reads: straight back to 52-55 frames
```

The correlation is not subtle: **windows with zero disc reads run 52-55 frames, and the window that spent
581 ms inside `read()` delivered 10.** Cumulative blocking disc time for this run: **11.7 s of 81 s**.

So the stutter is the CDDA feed: filling the audio ring means reading the CD image off the card, those reads
are synchronous, and the frame loop is the thing that waits. Per-op cost is 20-36 ms mean, with individual
reads up to 120 ms. It only happens where music is actually reading, which is why some seconds are smooth.

## 2. The sync path has nothing left to give

`DISC_BATCH` is clamped at 27 sectors (`CD_BATCH_MAX`, `of_cdda.c:62`) because 63 504 bytes is the largest
single op this OS moves (docs/375, docs/420). Bigger batches were never available; the per-op overhead cannot
be amortised any further on the synchronous path. Async was tried and is dead on this hardware — `of_file_read_async()`
completions never fired (docs/403, docs/404), so `DISC_ASYNC=0` ships.

## 3. What the header says that we had not read closely enough

```c
/* Register a hook invoked while a BLOCKING file read waits on its DMA, so the
 * app can keep the audio ring fed from already-decoded data and music does not
 * stall during SD access. The hook MUST NOT itself issue a blocking file read
 * (kernel suppresses nested invocation). Pass NULL to clear. No-op on an older
 * OS that predates this service-table entry. */
static inline void of_file_set_idle_hook(void (*hook)(void))
```

Two things fall out of that comment. The kernel's blocking read *waits on a DMA*, so the CPU is free while the
SD is busy — and the OS will hand that time to us. It is offered for audio, but the frame loop is stalled by
the same wait, and a frame does not need the disc. The gate is the service-table check it spells out, so the
first thing to do is find out whether this firmware actually has it.

`0.2.3` logs, under `DIAG=128`:

```
filecaps: svc_count=%u idle_hook=%d async_max=%u dma_stage_total=%u stage64k=%p busy=%d
```

If `idle_hook=1`, the lever is a hook that runs the frame work (blit + flip, guarded against re-entry and
against issuing any nested read, which the kernel suppresses anyway) while a read waits — turning a 120 ms
dead wait into a presented frame. If it is `0`, the honest options shrink to feeding the music at a lower
data rate or living with the stutter, and I will say so rather than invent a fourth thing to try.

## 4. Also: 0.2.2's trace produced zero lines, and that is a design fault of mine

`fdump` fired **0** times on hardware. The trigger was L1 + Up: four deliberate actions, at the exact moment
the eye is on a screen change, on a pad with 11 buttons. So the trace now arms itself — `FDUMP=2` arms the 60
presents at the mode change *into* a stage, which is the same instant and needs nobody.

While at it, a one-shot line so the absence of a combo is distinguishable from the absence of the call site:

```
combo: first combo-button seen l1=%d select=%d start=%d
```

That matters beyond the trace: if the L1 combos never register on the Pocket body, then the save/load and fit
combos in docs/334 have never worked either, and I have been describing buttons the hardware does not give us.

## 5. What the next boot carries, and what it will decide

Card: `0.2.3`, `FDUMP=2`, `DIAG=160` (surfhash + filecaps), `PALETTE_AFTER_FLIP=0` (back to the default
ordering, since docs/424 showed the change made no difference), `PRESENT_SKIP=1`, `CDDA_STOP_FIX=0`. Nothing
in this build changes behaviour that the player sees; it only measures.

One boot answers both open threads: the auto-armed fade trace (docs/424's decision table) and the file
capability probe that decides how the stutter gets attacked.
