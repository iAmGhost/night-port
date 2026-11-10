# 406 — the disc was spending two thirds of the CPU moving bytes it did not need yet: 50 two-sector reads a second, while the ring sat 96.5% full

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


Core **0.1.87**, card ELF md5 `bb9ba55faf70983adbe5d691f39c37f1` == build == `core.json` == ELF string, 29 ini
keys, `.bss` 20 088 884, `pocket/src/night make test` green both halves, repo `make test` `== ALL PASS ==`,
`cdda.raw` md5 `ad81a3e0f5567c210c358123accfe2e9` unchanged, `src/f` untouched.
Evidence: the user's own 0.1.86 run, kept at `analysis/legacy/cardlog_0_1_86_slow_not_stuck.sav` (97 KB) — this doc is mostly arithmetic on it.

## 1. The reframe that found it
I asked about a logo freeze. The user answered: **it is not stuck, the loading just takes a long time before the
logo fades out — and the weird music pause is still there.** "Stuck" and "slow" are different failures, and I had
been reasoning about the wrong one. So I measured their run instead of my theory.

Per-second, from `memstat`/`perf`/`budget` (elapsed from `ticks/490`):

| phase | fps | `perf pres` | disc |
|---|---|---|---|
| t = 0 → 38.8 s (logo/intro, `view=0 640x480`) | **55.8 steady** | **17 694 → 17 660 µs** | **ops = 0** |
| t = 38.8 → 54 s (title → gameplay, `view=1 320x200`) | 8.8 … 57 | 3 728 → 18 818 µs | ops begin |
| t = 54 → 80 s (gameplay) | 44 … 49 | ~3 700 µs | ~50 ops/s |

Two things fall out, and they are two different answers.

## 2. The long pre-logo stretch is not a disc problem at all
For the first 38.8 seconds there is **no disc traffic whatsoever** — `disc ops=0`, `cdda fills=0` — at a rock-steady
55.8 fps, and the frame cost is `pres=17.7 ms` of a 17.9 ms frame. That phase is frame-bound by the copy of a
640×480 surface to the panel, at the 19.5 MB/s the aperture sustains (docs/401). Nothing to fix there except the
copy itself.

On *length*: the port reaching title at ~40 s is faithful, not a stall — docs/389 measured time-to-title at
**48.4 s original vs 43.1 s port**. The .fli intro is genuinely long. So the honest statement to the user is: the
duration is the game's, and what the port can still lose is the 55.8 fps (vs ~60/70 Hz of the original's
refresh), which comes from the same 17.7 ms copy.

## 3. The music pause: the pump was thrashing a nearly-full ring
The gameplay window, per second:

```
sec/op    1.3 – 1.7      ← batching not happening (DISC_BATCH=12)
mean_op   5 528 – 26 159 µs   ← one SD operation costs 5-26 ms on the device
ops/s     45 – 59
=> 60 – 65% of wall time inside fread
max_op    82 548 µs, then 96 846 µs
ahead_min 15 553 frames = 0.35 s   (ring is 176 400 frames = 4 s)
room_hold climbing 0 → 2 184
```

And the reason `sec/op` was 1.5 is sitting in the same lines: **`head=170 086` of 176 400 ring frames — the ring
was 96.5% full, 3.86 s of audio already buffered ahead of the voice.** So `room` was about 3 sectors, and the
pump dutifully topped up **2 sectors at a time, ~50 times a second** — paying 5–26 ms per *operation* to move
75 sectors a second it did not need to move yet.

That is a full reversal of the framing I have carried since docs/375 and repeated in docs/402: *"the feed is
marginal, 172 KB/s delivered against 176.4 KB/s needed."* Throughput was never the binding constraint and the
ring was never starved. **The operation costs; the sector does not.** Every earlier attempt (bigger batches,
chunk gates, prefills, and the failed async double-buffer) was aimed at how fast bytes arrive, when the actual
bill was how often we asked.

It also explains the pause the user keeps hearing, without any underrun: `dry=0` and `audio_low=0` throughout,
but a 97 ms `max_op` stall in the same loop that refills the ring, with only 0.35 s of minimum headroom, and the
game re-requesting tracks (idx 1 → 9 → 11 → 2 inside 80 s) so every restart pays priming again.

## 4. The fix: don't read when we're deep, and read a full op when we do
1. **Defer a tiny read while the voice is ≥ 2 s behind** (`CD_SAFE_AHEAD_FRAMES = 88 200`). The condition is
   *being deep*, so it cannot manufacture an underrun: when the ring is low, every read goes through exactly as
   before. New counter `deferred=` on the 1 Hz budget line.
2. **`DISC_BATCH` 12 → 27**: `27 × 2352 = 63 504 B`, the largest single op this OS will move
   (`of_file_async_max_read() = 65 536 B`, docs/403) — so the same bytes that cost ~50 ops/s now cost ~3.
3. **Device-only guard** (`#ifndef OF_PC`) on the deferral, because the desktop shim has no trustworthy mixer
   position and it is the byte-exact basis for docs/365/382. Verified: shim `cdda.raw` md5 unchanged. This is the
   same rule the palette gate learned in docs/400 — a performance experiment must not leak into the host that
   serves as the comparison basis.

## 5. What the next boot decides, and only these numbers count
| measure | 0.1.86 (user's run) | fix working looks like |
|---|---|---|
| `disc sectors/op` | 1.3–1.7 | ≥ 12, ideally ~20 |
| `disc ops` per second | 45–59 | ~3–6 |
| implied blocked time | 60–65% of wall | < 5% |
| `deferred=` | field did not exist | climbing steadily |
| `perf pres` in gameplay | 3 728 µs | unchanged (it was never the problem here) |
| fps floor in gameplay | 8.8 | > 30, and the 8.8/10.1/12.8 dips gone |
| `ahead_min` | 0.35 s | ≥ 1 s |
| `dry` / `audio_low` | 0 / 0 | must stay 0 |

If the dips vanish but the *pause* remains, the remaining suspect is the priming silence at each game-initiated
track restart (docs/398: 315–418 ms before the first sample), and the lever is a deeper prefill or priming under
the fade — not the read policy, which this boot will have settled.

One diagnosability gap recorded while I was in here: `of_cdda.c`'s "cdda: fill policy = read at least N sectors"
line goes to `printf`, i.e. the terminal, not the card log — so the very setting that governs this behaviour was
invisible on the card. It should join the boot block that the log does carry.
