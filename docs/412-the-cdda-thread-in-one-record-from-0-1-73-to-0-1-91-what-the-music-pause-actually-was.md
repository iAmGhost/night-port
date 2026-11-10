# 412 — The CD-DA thread in one record: 0.1.73 → 0.1.91, what the music pause actually was

The Pocket port's audio problem took nineteen builds and seventeen documents. This is the one file to
read: the causal chain, the measurements that settled it, the seven things that were tried and made it
worse, and the rules the thread produced. Each numbered document below still exists and is still cited
by name from the code — this file does not replace them, it puts them in order and states which claims
survived.

Start here, then read any of them as needed. `docs/332` is the original engine design (ring buffer,
byte-exact against the shipped disc); `docs/367` is the whole-port record.

## 1. The two complaints, and what each turned out to be

A hardware playtest at 0.1.73 reported two things: **the music arrives late / pauses** after a few
seconds, and **the picture lags** (docs/395). They share a root only in the sense that both are measured
on the same device, and they were investigated almost entirely separately:

- **Music pause.** Root-caused in 0.1.90 to one assignment. The whole path is §2.
- **Choppy picture.** Not an audio problem: the present path writes a whole 640x480 surface at ~19.5 MB/s
  (docs/401). Partial-row optimisation is structurally invalid on the SDK's triple-buffered display
  (docs/400). This remains open as an optimisation, not a defect.

## 2. The music pause, in order of causation

`of_cdda.c` keeps a 4 s ring (`CD_RING_FRAMES=176400` frames) written by the disc pump and read by two
looping mixer voices. Two cursors describe it, and the port's whole bug class lives in their relation:

- `g_write` / `g_done_frames` — how far the **writer** has got. `ring_write()` advances one, and
  `fill_sectors()` advances the other. **These two mean the same thing and must always be moved
  together.**
- The mixer's voice cursor — where the **hardware** is reading. The voices are armed with
  `of_mixer_set_loop_h(0, CD_RING_FRAMES)`, so they run forward forever and never stall; they simply play
  whatever is in the ring, including samples nobody has written yet.

The defect, 0.1.73 through 0.1.89: after the prefill of a track, the code executed

```c
fill_sectors(prefill_sectors);   /* writes 88 200 frames; leaves g_write = 88 200 */
g_write = 0;                     /* BUG: puts the writer back under the voice */
```

That single line moved the writer from 88 200 back to 0 — right where the voice was — and left
`[88 200, 176 400)` as zeros. `g_done_frames` was *not* reset, so the two cursors disagreed by 87 100
frames. The writer then chased from behind at real-time rate, the voice walked off the written frontier
into silence, and only came out of it when it lapped the ring into freshly written audio. Hence: **the
pause happened at track starts** — logo, title, episode introduction — exactly where the player reported
it.

The instrument that proved it (0.1.89's voice probe, docs/408) printed, while every other counter read
healthy:

```
live=6754 w=10 e_live=0            voice is sounding, reading sample 0
behind≈170500  vroom=500-1700      "behind" near RING means the voice is about to read unwritten audio
dpos≈44150 / 1001 ms               the mixer eats at exactly 44 100 frames/s — the rate theory was dead
e_live=0 on ~40 % of samples       the voice is reading silence
```

The fix is deleting the assignment (0.1.90). `docs/409` has the full argument; `docs/408` is why a
probe of the *mixer* was needed when every port-side counter was green.

Two counting rules came with it:

- **`behind` near `RING` is not "plenty buffered".** It means the voice is about to read audio nobody
  wrote. Watch `ahead = RING - behind` alongside it; `ahead` is the one that must not be near zero.
- **A looping DMA voice cannot underrun.** The `dry` counter is structurally blind to this failure,
  because `clock_consume()` clamps `want` to `g_done_frames`. Reading stale or unwritten samples is not a
  dropout; no dropout counter sees it.

## 3. The disc feed: what was actually binding

For most of the thread the disc was blamed for being slow, and every fix aimed at *how fast bytes arrive*.
The measurements that ended that (docs/406, from the user's own 0.1.86 card log):

| quantity | value |
|---|---|
| ring state during gameplay | `head=170 086 / 176 400` — **96.5 % full** |
| room left per read | ~3 sectors, so the pump did ~2-sector reads ~50×/s |
| per-read cost | 5–26 ms each |
| wall time inside `fread` | **60–65 % of the run** |
| throughput needed vs achieved | 176.4 KB/s needed, 172 KB/s achieved |

**The operation costs; the sector does not.** Throughput was never binding and the ring was never
starved. The port was paying 5–26 ms fifty times a second for data it already had. Burst throughput via
`of_file_read` is ~2.3 MB/s (measured in 0.1.91: 529 calls, 2 302 464 B, largest 49 664 B), two orders
above the per-sector CDDA cost, and device SD reads cost ~2 095 µs/sector against ~2 µs/sector batched
on desktop (docs/396, docs/375).

What the shipping build does about it:

| lever | value | why |
|---|---|---|
| `DISC_BATCH` | 27 sectors (63 504 B) | biggest single op this OS moves; makes each ask count |
| `DISC_LOOKAHEAD` | 240 sectors (3.2 s) device, **120 pinned on the shim** | a ceiling *smaller than the ring it feeds* causes a 1.5-sector trickle (docs/407) |
| `CD_SAFE_AHEAD_FRAMES` | 88 200 (2 s), device-only | defer a tiny read when the voice is deep; the condition is being *deep*, so it cannot manufacture an underrun |
| `DISC_MIN_WANT` | 27 **but clamped to 12** by `CD_MIN_WANT_MAX` | the boot line honestly prints `cdda policy: min_want=12`; lifting the clamp is a separate build |
| `DISC_PREFILL_FIRST` | 150 sectors (2 s) | one-shot priming before voices start; `DISC_RAMP=0` because ramping underran (docs/402) |
| `DISC_ASYNC` | 0 | `of_file_read_async()` completions never fired on hardware (docs/404); sync fallback is mandatory |

## 4. Seven things that were tried and made it worse

Recording these is the point of this file. Each was a reasonable inference and each was wrong:

1. **Cut the prefill (0.1.74, docs/397).** 150 → 24 sectors to reduce the startup stall. Starved the
   ring: `audio_low` 39, `dry` 33. Reverted in 0.1.75.
2. **Ramp the priming (0.1.76, docs/398).** Spread the priming over ticks. `dry` 5→49, `audio_low`
   5→64. `DISC_RAMP=0` ships (docs/402). Priming cannot be cut, only spread — and spreading it underruns.
3. **Dirty-row present (0.1.77–79, docs/399).** Correct idea, twice broken: `STAGE_MAX_BYTES` was
   computed from the *game* size (64 000) instead of the *surface* size (307 200), so staging never
   engaged for two releases. Then proved **invalid** regardless: the display is triple-buffered and
   `of_video_flip()` does not wait, so a skipped "unchanged" row is a 3-frame-old picture (docs/400).
   `BLIT_DIRTY=0` ships.
4. **`FB_MODE=0` — OS 320x240 + software 2×2 downscale (0.1.80, docs/401).** Four times fewer bytes,
   **five times slower**: 76 800 B in 82 657 µs (0.93 MB/s) vs 307 200 B in 15 771 µs (19.5 MB/s). A
   per-pixel gather costs ~1 µs/output pixel. It also brought the logo freeze back **twice** (0.1.80, and
   again in 0.1.84/85 which re-shipped it — docs/405). `FB_MODE=1` ships, permanently.
5. **`PRESENT_SYNC=1` (wait for flip, docs/401).** Flip stage 18 294 → 33 436 µs. `PRESENT_SYNC=0` ships.
6. **Async SD prefetch (0.1.83, docs/403/404).** On hardware the completion never fired
   (`mean_op=0µs`), and the design returned without a sync read when async failed — so it **muted the
   music entirely**. Code retained with a mandatory sync fallback; `DISC_ASYNC=0`.
7. **My own read-deferral (0.1.87, docs/407).** Deferring tiny reads on the mixer cursor alone starved
   the pump's ring: `audio_low` 0→7 553, `dry` 0→353, `ahead_min` 0.35 s→0.00 s, `deferred=10 657`.
   The deferral now requires **both** depth measures to agree (`of_cdda_head()` *and* `ring_ahead()`).

## 5. Method results, which cost more than the fixes

- **The audio md5 is not a determinism test** (docs/410). Three runs of one binary, one seed, one command:
  two distinct `cdda.raw` md5s and two lengths, first mismatch at byte **2 215 585** — 24 s of audio in,
  not the tail. `cdda.raw` is a wall-clock sampling log of the ring. Therefore the "rig basis unchanged"
  inferences in docs/405–409 were luck, including the docs/407 bisection that blamed the shared lookahead
  setter; the host-pinning fix is still right but that causal claim needs re-deriving. The robust gate is
  **per-request bytes against the shipped disc**. `make test` (differential against the python reference
  models) stayed `ALL PASS` throughout and is unaffected.
- **A knob that exists to fix a device number must be inert on the basis host** (docs/407 §3). Threading
  `CD_LOOKAHEAD_SECTORS` through a shared `of_cdda_set_lookahead()` called from both hosts moved the
  shim's audio while `make test` stayed green. Hence `#ifdef OF_PC 120 #else 240`, and the setter is a
  no-op under `OF_PC`. The same rule is why 0.1.91's counters compile out on the desktop.
- **The card log's tail is a previous run** (docs/402). `nightlog.sav` is a fixed 262 144 B slot; a fresh
  boot writes from the beginning, so `grep | tail` reads the *oldest* data. Read from the boot marker
  forward and stop where the counters stop advancing monotonically.
- **Count printf arguments from the ELF, not from memory** (docs/404). `blit dirty` had 12 format
  specifiers and 10 arguments from 0.1.79 to 0.1.83; palette counters quoted in docs/399 were shifted
  garbage.
- **Check a generator's output, not its source** (docs/405). A long trailing comment in `mk_dist.py`
  silently swallowed `PUMP_AUDIO_TICK=0` from the generated `night.ini`; it now refuses to ship without
  every required key, verified in both directions.
- **One variable per boot.** Bundling an audio fix with a picture revert and an async toggle (0.1.86)
  left the next boot unable to attribute anything.
- **BRAM was never real** (docs/396): `.app_fasttext` at `0x00004000` with `in_bram=0`. The region is not
  recognised as BRAM by this OS build. `BRAM_SET=none` ships permanently.
- **A diagnostic can be the pathology** (docs/411). The 35 s logo hold that docs/410 analysed belonged to
  0.1.89, the instrument-only build; 0.1.88 measured 9.1 s and 0.1.91 8.1 s for the same phase.

## 6. Still open

- **The ~8 s opening phase on hardware** (docs/411): zero disc reads, zero keyboard spins during it, vs
  2.6 s on the desktop. Neither of the two predicted causes occurred. Cause unknown; user-invisible, so
  it is not scheduled.
- **Choppy intro**: 19.5 MB/s surface write is the hardware aperture. Needs either per-display-buffer
  shadows (the only correct form of partial update, docs/400) or fewer presents per second.
- **`DISC_MIN_WANT` clamp** (27 requested, 12 effective): a separate build, since it changes shim fill
  sizes.
- **docs/407's causal claim** about the shared setter, pending re-derivation on a reproducible gate.

## 7. Reading order for the seventeen

| doc | build | one line |
|---|---|---|
| 395 | 0.1.73 | the playtest report that opened the thread; BRAM and disc policy as the two suspects |
| 396 | 0.1.73 | first measurement-bearing card log: 424 ms ring fill, 2 095 µs/sector, BRAM fake |
| 397 | 0.1.74/75 | prefill cut traded a rare stall for real underruns |
| 398 | 0.1.76 | the priming ramp; and what a track restart actually is in `src/f` (faithful game behaviour) |
| 399 | 0.1.77–79 | dirty-row commit and palette gate; the surface-size arithmetic bug that hid it |
| 400 | 0.1.80 | why partial update is invalid on a triple-buffered display |
| 401 | 0.1.80/81 | `FB_MODE=0` wrote 4× fewer bytes 5× slower; the logo-freeze shape |
| 402 | 0.1.82 | the ramp underran; the card log's tail is a previous run; the disc feed constraint |
| 403 | 0.1.83 | double-buffered async reads; and a shim broken silently since 0.1.81 |
| 404 | 0.1.83/84 | async muted the music; doubled picture; the printf arg mismatch |
| 405 | 0.1.86 | shipping the shape docs/401 already condemned; the swallowed ini key |
| 406 | 0.1.87 | **the operation costs, not the sector** — ring 96.5 % full while two-thirds of the CPU read |
| 407 | 0.1.88 | my own deferral starved the ring; a ceiling smaller than the buffer it feeds; the shared-setter lesson |
| 408 | 0.1.89 | every counter healthy while the player heard silence; the voice probe |
| 409 | **0.1.90** | **the fix**: `g_write = 0` after the prefill moved the writer back onto the voice |
| 410 | 0.1.91 | the reads nobody counted, and the audio md5 that was never a determinism test |
| 411 | 0.1.91 | the logo hold was a 0.1.89 pathology; the instrument disproved both branches |
