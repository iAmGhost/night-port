# 402 — the priming ramp underran, and the card log's tail was a previous run

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* the ramp it measures was turned off (`DISC_RAMP=0`).


Two findings. The second one changes how every log in this project must be read.

## 1. A method error first: `nightlog.sav` is a fixed 256 KB slot, and its tail is stale

The 0.1.81 log opened with `core 0.1.81: boot marker` at **line 17**, and its tail reported:

```
disc ops=9703 sectors=18899  cdda fills max=418281us  audio_low=0  dry=0  ahead_min=19715(0.45s)
```

Those are **0.1.75's numbers** — identical to the file I read days earlier. The log writer starts at the beginning
of the slot on each boot; a shorter run leaves the previous, longer run's bytes past the end of what it wrote. The
file size stays 262 144 because the slot is fixed-size, so nothing about the file says "this tail is old".

Rule going forward: read from the boot marker forward, and stop where the counters stop advancing monotonically.
`grep … | tail` reads the oldest data in the file. Several of my earlier "this build got worse/better" readings were
comparisons against a stale tail, and the 0.45 s `ahead_min` above is *not* a 0.1.81 measurement.

## 2. What 0.1.81 actually measured

From the live region (line 78 onward), per-second `budget present` samples:

| sample | `ahead_min` | `dry` | `audio_low` | `room_hold` | `fills` |
|---|---|---|---|---|---|
| first nine (before any track) | 0.00 s | 0 | 0 | 0 | 0 |
| first track switch | **0.16 s** | 0 | 0 | 0 | 8 → 96 |
| then, monotonically | **0.00 s** | **5 → 49** | **5 → 64** | 164 → 898 | 128 → 374 |

`cdda fills … max=118 260us`.

So: the ring was **never ahead** (0.16 s at the first switch, 0.00 s after), and the underrun counters climbed
throughout the run. 0.1.75's own live region measured `dry=0`, `audio_low=0`, `ahead_min 19 715 frames (0.45 s)`
with a `418 281 µs` fill. The ramp traded the fill for underruns — the same mistake as 0.1.74, in a subtler form.

### Why that produces "skips the early portion of the music"
The mixer voice loops over the whole 176 400-frame ring (`of_cdda.c`, `voice_start()`/wrap). The ramp starts the
voice after 24 sectors and fills the rest *while it plays*. If the write cursor is behind the voice head, the head
reaches ring territory that was either never written or written for the *previous* track — and because the voice
wraps, it then reads that territory from the beginning again. Audibly: the opening bars are missing, or the same
phrase repeats. `dry` and `audio_low` climbing monotonically is that mechanism being counted.

### The deeper constraint, also from this log
Across the live span:

```
sectors/op = 1.95        (DISC_BATCH is 12: the batching is not happening on the device)
mean_op    = 3 878 us    (a fixed ~3.9 ms cost per slot read, whatever it delivers)
delivered  = 172 KB/s    (a CD-DA stream needs 176.4 KB/s)
```

The feed is **marginal** — 98% of the required rate — so there is no headroom for any stall, and `sys_cd_da_plan`
is granting ~2 sectors per call, paying the full 3.9 ms for each. This is why "prime more at track start" keeps
appearing to help and keeps breaking something else: the steady state itself is at the edge.

## 3. What 0.1.82 ships

- **The ramp is off by default** (`DISC_RAMP=0`): priming happens in the visit that requested the track,
  `DISC_PREFILL_FIRST=150` — the 0.1.75 shape, the one that measured `dry=0 audio_low=0 ahead 0.45 s`. The ramp
  code stays, switchable, because it *did* remove the 418 ms fill and that problem is real.
- Picture shape unchanged from 0.1.81 (`FB_MODE=1`, `PRESENT_SYNC=0`, `BLIT_DIRTY=0`), so the picture thread is not
  disturbed while the audio thread is reverted.

Card: core **0.1.82**, ELF md5 `5bbabf47f6f0c0f8a294807bea096e36` == build, `core.json` == ELF string, 28 ini keys
including `DISC_RAMP=0 DISC_PREFILL_FIRST=150 DISC_MIN_WANT=12 DISC_BATCH=12 BLIT_DIRTY=0 FB_MODE=1 PRESENT_SYNC=0`.
Repo `make test` = `== ALL PASS ==`. `src/f` untouched.

## 4. The remaining fix is structural, and the numbers say what it must be
Scheduling tweaks around a 3.9 ms-per-op, ~2-sector feed cannot win. The device facts (docs/378, docs/396) that a
real fix must use:

| fact | value | consequence |
|---|---|---|
| `async_max_read` | 65 536 B = **27 sectors** | an op can carry 27 sectors, not 2: 507 KB per 3.9 ms = 130 MB/s of capability against a 176 KB/s need |
| `dma_stage_size` | 1 048 576 B | enough to stage 5.9 s of audio outside the mixer ring |
| `mean_op` on device | 3 878 µs | the cost is per **operation**, so the lever is fewer, bigger operations |

So: raise the granted chunk toward 27 sectors (`CD_CHUNK_SECTORS`/`DISC_BATCH`/`DISC_MIN_WANT`, and the batch
buffer), and issue the next read while the mixer plays the current one. That turns a 98%-of-rate feed into one with
an order of magnitude of slack, and it makes the 418 ms fill disappear without a priming ramp, because the priming
itself becomes one or two big asynchronous operations. That is a change in `of_disc.c` and `src/sys`'s grant policy,
not a knob — and it should be built and measured against `sectors/op` and `dry`/`audio_low`, not argued.
