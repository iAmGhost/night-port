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

## 5. Shipping state

Card **0.1.62**: `night.elf` md5 `e43645444d3b79499a1643b26962c4dd` == the build, `core.json` 0.1.62,
`.app_fasttext` 9 376 B (the present set, docs/372 — unchanged by this work), `night.ini` 24 unique keys with
`DISC_BATCH=12`, `SCREEN=0 DIAG=0 LOG=0 RESTORE=1`.  `.text` 494 248 B (0x78aa8) + `.app_fasttext` 9 376 B (0x24a0), `.bss` 19.3 MB (the
16 MiB arena, the two 4 s audio rings, and the 28 224 B batch buffer) in 64 MB SDRAM.  Fixture suite 3 089 `[PASS]`
`== ALL PASS ==`, `check_slots.py == ALL PASS ==`, `src/f` untouched.
