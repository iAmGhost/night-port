# 375 — CD-DA reads went from one sector at a time to twelve, and the sector stream is byte-identical

task-9's remaining levers were "async SD reads" and "GPU span/blit".  Before reaching for the async API there was a
dumber question on the table, visible in `of_cdda.c` since docs/364: the fill loop asked the disc for **one 2352-byte
sector per call**, so a 12-sector chunk cost 12 read operations and a 300-sector prefill cost 300 — each with its own
seek decision, OS call and buffer hand-off, for 1/75 s of audio.

## 1. The change

`of_disc.c` gains `of_disc_read_audio_run(disc_lba, out, nsectors)`: `nsectors` consecutive sectors in **one**
`fread` on the same per-track slot handle, seeking only when the read is not the next sector in file order (the
handle discipline from docs/338 is unchanged).  `of_disc_read_audio()` is now a wrapper with `nsectors = 1`.

`of_cdda.c` reads into a `CD_BATCH_MAX * 2352` buffer (28 224 B) and empties it sector by sector, so the byte order,
the per-sector rig witness and the ring's write pattern are untouched — only the count of reads changes.  The batch
size is a setting (`night.ini DISC_BATCH=`, 1..12, env `NIGHT_DISC_BATCH` on desktop), which is what makes the
measurement below an A/B inside **one binary** rather than a comparison of two builds.

Counters were split, because the old `reads=` column silently changed meaning: `ops` (file read operations) and
`sectors` (sectors delivered) are now reported separately, with `mean_op` and `mean_sector` alongside.

## 2. What it bought (same binary, same route, `NIGHT_DISC_BATCH=1` vs `12`)

Stage 1, 2 400 frames, seed 1234, one boot each:

| | batch 1 (the old shape) | batch 12 |
|---|---|---|
| disc ops | 3 789 | **2 689** |
| sectors delivered | 3 789 | 3 789 (identical) |
| bytes moved | 8 702 kB | 8 702 kB (identical) |
| mean µs per **sector** | 11 | **2** |
| worst single read | 2 459 µs | **823 µs** |
| cdda fill mean | 16 µs | **4 µs** |
| cdda fill worst | 5 991 µs | **2 369 µs** |
| present mean | 406 µs | 398 µs |
| present worst | 1 666 µs | **915 µs** |
| seeks / fails | 4 / 0 | 4 / 0 |
| audio_low / room_hold / dry | 0 / 29 / 0 | 0 / 27 / 0 |
| ring ahead_min | 69 891 (1.58 s) | 69 884 (1.58 s) |

Five-and-a-half times cheaper per sector, and the **worst-case** read got cheaper by more than the mean — which is the
case that matters, since a long read inside a frame is what steals audio.  The worst *present* halved too, which is
the same effect seen from the other side: less time per read inside the frame means a shorter tail on the frame.

## 3. The bytes are the same (proof, not assertion)

Both runs dumped the rig witness.  The two files that record the disc and the audio:

```
IDENTICAL cdda.raw    9f8513b674c05ee133ece0861f0380ae     <- every raw 2352-byte sector, in order
IDENTICAL pcm_cd.wav  cd07f0d4444470b9d0e7bbd00e5f40c8     <- the mixed audio the host produced
IDENTICAL meta.json   9ea6110e219e3942db376fb4adc8df4e
```

`frames.bin`, `records.jsonl`, `player_recs.jsonl`, `markers.jsonl` and `cells.bin` differ — expected and already
characterised in docs/363 §5: those are animated, timing-dependent content and a signature difference between two runs
is not a determinism test.  Where the change could only have shown up as corruption — the sector stream and the audio
built from it — it shows up as nothing.

## 4. Why the async API is still the next step, not this one

`of_file.h` offers `of_file_read_async(slot, offset, dest, len, callback)` with an IRQ-driven completion, a DMA staging
allocator (`of_file_dma_stage_alloc`, zero-copy when the destination is CRAM0), and two limits that are queries rather
than constants — `of_file_async_max_read()` and `of_file_dma_stage_size()` — plus "only one async read in flight at a
time (bridge limitation)".  Those two limits cannot be read on the desktop shim, and a prefetch design that assumes
them is exactly the kind of thing that has bitten this port before (docs/342: an SDK call that answers `0x55555555` and
the shim hides the error).

So the safe half ships now, and the device measurement comes first: query `async_max_read`/`dma_stage_size` on the
Pocket, print them in the boot block, and only then decide whether a double-buffered prefetch (issue the next batch
while the mixer plays the current one) is worth its complexity.  Batching is also the precondition for that: an async
read of one sector would be a per-sector IRQ, which is worse than what it replaces.

## 5. The sweep showed the reads were still one sector each

Shipping the batch buffer alone did not batch much.  The nine-episode sweep (docs/363's rig, `FRAMES=6000`, on the
0.1.62 core path) reported, per episode:

```
ep stage1: disc ops=7647 sectors=8747 mean_sector=2us max_op=1516us
ep stage2: disc ops=7566 sectors=8941 mean_sector=3us max_op=1776us
...        disc ops ~= sectors, in all nine
```

`ops ~= sectors` means the fill loop was still asking for one sector at a time, and the reason is in the fill
decision: the disc clock advances **75 sectors/s while the pump runs at the 490 Hz tick**, so the headroom
`ceiling - g_pos` (where `ceiling = g_budget + CD_LOOKAHEAD_SECTORS`, 120 sectors = 1.6 s) grows by about **0.15
sectors per visit**.  Truncated to an integer it is 0 or 1, so `sys_cd_da_plan(..., want, cap=12)` was handed `want=1`
in steady state.  The 12-sector cap was never reached outside prefills: reads were happening at exactly the disc's
own rate, one 2 352-byte sector per OS call.

## 6. Waiting for a chunk to be due (the bigger half)

`DISC_MIN_WANT` (1..12, default 12, `night.ini` / env `NIGHT_DISC_MIN_WANT`): hold the read until 12 sectors of
allowance have accumulated, then take them in one read.  Two cases must not wait, and the code says so: the track is
running out (take what is left), and the ring is within twice the 0.1 s guard of the voice (take what is due now --
an underrun is audible, a slightly early read is not).

Same binary, stage 1, 2 400 frames, `DISC_MIN_WANT=1` vs `12`:

| | min_want 1 (the old shape) | min_want 12 |
|---|---|---|
| disc ops | 2 690 | **314** |
| sectors | 3 790 | 3 768 |
| mean µs per op | 6 | 7 |
| **worst single read** | 2 002 µs | **61 µs** |
| mean µs per sector | 4 | **0** |
| cdda fills | 2 594 | **218** |
| worst fill | 6 153 µs | **174 µs** |
| present mean / worst | 397 / 1 629 µs | 394 / **923 µs** |
| audio_low / dry | 0 / 0 | 0 / 0 |
| room_hold | 28 | 26 |
| ring ahead_min | 69 905 (1.59 s) | 63 426 (1.44 s) |

The worst *read* fell 33-fold and the worst *fill* 35-fold: the tail of a frame is no longer a slot read at all.
`ahead_min` drops from 1.59 s to 1.44 s -- chunkier reads leave slightly less slack -- still fourteen times the
0.1 s guard, and `audio_low` and `dry` stay 0 in both shapes.

Nine-episode regression on the shipping shape: **9/9 episodes entered and clean, the same 27 shipped cards
byte-identical**, tick rate +491/s, arena 4.89 MB of 16 MiB, and per-episode disc shape now

```
disc ops 708..751   sectors 8 496..9 012   mean_sector 1us   max_op 856..3 388us
```

ten times fewer operations than the same sweep on 0.1.62, at the same sector counts.

What that leaves on the table is visible in the same line: `cdda fills=614 mean=253us max=5747us`.  A track's
**prefill is measured as one fill** -- 300 sectors read inside the frame that requested the track -- and that is now
the single most expensive thing the audio path does.  Spreading a prefill across frames is the next candidate, and on
the device it is likely to matter far more than on the desktop.

One honesty note about the A/B: with `DISC_MIN_WANT` the two runs' `cdda.raw` streams share their first 1 200 sectors
and then part ways, because a chunkier read changes *when* a track ends relative to the game's own timing (the play
markers are identical -- same indices, LBAs and sector counts in both runs: 3 153 / 27 697 / 19 639 / 17 856 sectors
at lba 37 262 and 40 415).  The batching A/B in §3, where the reads are the same size and only the packing differs,
is the one that shows byte-identical `cdda.raw` and `pcm_cd.wav`.

## 7. Shipping state

Card **0.1.63**: `night.elf` md5 `8b5463965d12be26e73cb2b11a9ea4b4` == the build, `core.json` 0.1.63,
`.app_fasttext` 9 376 B (the present set, docs/372 — unchanged by this work), `night.ini` 25 unique keys with
`DISC_BATCH=12` and `DISC_MIN_WANT=12`, `SCREEN=0 DIAG=0 LOG=0 RESTORE=1`.  `.text` 494 248 B (0x78aa8) + `.app_fasttext` 9 376 B (0x24a0), `.bss` 19.3 MB (the
16 MiB arena, the two 4 s audio rings, and the 28 224 B batch buffer) in 64 MB SDRAM.  Fixture suite 3 089 `[PASS]`
`== ALL PASS ==`, `check_slots.py == ALL PASS ==`, `src/f` untouched.
