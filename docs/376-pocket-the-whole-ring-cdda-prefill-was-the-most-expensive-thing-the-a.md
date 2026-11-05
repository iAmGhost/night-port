# 376 — the whole-ring prefill was the most expensive thing the audio path did, so it became a slice

docs/375 §6 ended with the next candidate named from its own measurement line: `cdda fills=614 mean=253us max=5747us`.
That 5.7 ms is `of_cdda_play()`'s prefill: **300 sectors (705 KB) read inside the single frame that requested a
track**.  On the desktop that is a bad frame; on the card it is 300 SPI reads in a row, and the card has never been
measured.

## 1. What the prefill is actually for

Two things, and the whole ring is not one of them:

- the looping MUSIC voice needs *something real* under it the moment it starts (docs/332: a voice over an unwritten
  ring plays whatever the allocator left there, which is the buzz docs/338 chased);
- a readable slot must be proven before sound is expected (docs/337: requests with `sectors=0` while two voices looped
  at the logo and through the .fli).

Everything after the slice keeps its meaning: zero sectors read is still a refusal, a short read is still logged as
partial, and `ring_silence()` still guarantees the un-read remainder is silence rather than leftovers.  The pump then
tops the ring up in chunks, and it does that comfortably — it holds `g_pos` at `budget + 120` sectors while the voice
sits at the `budget`, so a slice is overtaken within a few ticks.

`DISC_PREFILL_FIRST` (12..300, env `NIGHT_DISC_PREFILL_FIRST`) is the slice, so the old shape is one key away.

## 2. The slice, measured on one binary (stage 1, 2 400 frames, seed 1234)

| slice | sectors before the voices start | worst read | worst fill | present mean | present worst | room_hold | ahead_min |
|---|---|---|---|---|---|---|---|
| 300 (the old whole ring) | 300 | 968 µs | 1 781 µs | 404 µs | 1 905 µs | **29** | 1.44 s |
| 150 | 150 | 819 µs | 1 935 µs | 396 µs | 1 022 µs | **0** | 1.44 s |
| 90 | 90 | 851 µs | 1 760 µs | 395 µs | 937 µs | **0** | 1.20 s |
| 45 | 45 | 844 µs | 1 362 µs | 394 µs | 926 µs | **0** | 0.60 s |

The structural finding is `room_hold 29 -> 0`: the whole-ring prefill was *refusing* writes because the ring was full
of audio the voice had not consumed yet.  A slice removes a class of refusals rather than shaving a few microseconds.

## 3. The sweep decides it: slack is worth more than microseconds

The nine-episode sweep (`FRAMES=6000`) at two slice values, same core otherwise:

| slice | ahead_min (all nine) | room_hold | dry | present mean | present worst | cards identical | episodes clean |
|---|---|---|---|---|---|---|---|
| 90 | 0.59–0.60 s | 0 | 0 | 351–397 µs | 1 936–16 668 µs | 27 | 9/9 |
| **150** | 1.44 s (0.96 s in episode 8) | 0 | 0 | 347–398 µs | 2 506–16 669 µs | 27 | 9/9 |

Frame cost is statistically identical — the worst frames (episode 8's 16.7 ms, the number docs/363 already carries)
are not audio reads at all.  Slack differs by 2.4×.  Since the device's per-read cost is unmeasured and likely far
above the desktop's, the shipping choice is the one that buys cushion at equal frame cost: **150 sectors = 2 s**.

## 4. Shipping state

Card **0.1.65**: `night.elf` md5 `aba7381a68408ec8d57a3f1061dd0351` == the build, `core.json` 0.1.65, `night.ini`
`DISC_BATCH=12 DISC_MIN_WANT=12 DISC_PREFILL_FIRST=150` with `SCREEN=0 DIAG=0 LOG=0 RESTORE=1`, `check_slots.py
== ALL PASS ==`, fixtures 3 089 `[PASS]`, `src/f` untouched.  Nine-episode regression on the shipping shape: 9/9
entered and clean, 27 shipped cards byte-identical, `audio_low=0 room_hold=0 dry=0` in all nine.

## 5. What this makes cheap to test on the card

Three of the four audio levers are now ini keys a player's card can be flipped to without a rebuild, and they are all
measurable from the same `budget` line: `DISC_BATCH` (bytes per read), `DISC_MIN_WANT` (how long to wait for a chunk),
`DISC_PREFILL_FIRST` (what a track switch costs).  The device numbers that would settle the remaining question — what
a sector read actually costs on the card, and whether 2 s of slack is enough there — come from one boot with `LOG=1`
(`scripts/pocket/ab.sh diag`).
