# 398 — the priming ramp, what a track restart actually is, and where the music lag lives

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


0.1.76 (card md5 `8f834106efc71953cec1eaa23256021d`, `core.json` 0.1.76 == the ELF's embedded string).
`of_cdda.c` only; `src/f` untouched; repo `make test` = 3089 `[PASS]`, `== ALL PASS ==`.

## 1. The restart signature in the card log is the game, not the port

0.1.75's log showed the CD position jumping backwards:

```
800 -> 150     369 -> 162     1231 -> 158     8687 -> 150     366 -> 150
```

Those are new play requests (`cdda req idx=2 lba=40415 ... -> disc track 3 slot 6` appears twice in the same
run).  `src/f` issues them, and it has three ways to:

| site | what it is |
|---|---|
| `f.c:24225` `fn_10b1d(cd_track_for_room(room))` | **entering a room starts that room's track** |
| `f.c:5576`, `f.c:27609` | the room's track on other scene paths |
| `f.c:2504-2509`, `26522`, `26929` | the CD **refresh watchdog**: `g_dat_63e38 / 490 > g_dat_53c48` re-issues the same track when its own length in seconds has elapsed (that is how the original loops a track it never re-seeks on a real drive) |

So a restart is faithful behaviour: NIGHT restarts a song when you change room, and re-issues a track when its
length expires.  The desktop shim shows the same signature on the same route (`582 -> 174` in a 2400-frame
`--warp-stage 1` run) — shared `src/f` behaviour, not a device artifact.

## 2. Which means the audible lag is the *start latency of each request*

Every one of those requests lands on the port's `of_cdda_play`, and until 0.1.76 that call primed
`DISC_PREFILL_FIRST=150` sectors **inside the frame that asked for the track**.  On the card a sector read measures
~2.1 ms (docs/396), so 150 sectors is a 315 ms typical / 418 281 µs worst read *before the first sample is
played*.  Each room change therefore cost the music a third of a second of silence, on top of a hitch in the
picture.  That is the lag the player reports, and it is why "no underrun" (`audio_low=0`, `dry=0`) was compatible
with "the music lags".

## 3. Why priming cannot be cut, only spread

The steady-state fill path is gated by the **disc clock**: `sys_cd_da_plan` grants allowance at 75 sectors/s
(the CD's own rate), bounded by `g_budget + CD_LOOKAHEAD_SECTORS`.  It can keep up with playback; it can
**never overtake real time**.  The prefill is the only mechanism that puts the ring ahead.  That is exactly what
0.1.74 got wrong — it primed 24 sectors and expected the pump to catch up, and the ring starved
(`audio_low=39`, `dry=33`, `ahead_min=0.00 s`, docs/397).

The playable build (4e7e267) primed `CD_RING_FRAMES / CD_FRAMES_SECTOR` = 300 sectors, the whole 4 s ring, in one
frame.  Right amount, wrong payment.

## 4. 0.1.76: same depth, paid as a ramp

```c
#define CD_PRIME_SLICE 24          /* sectors of priming per pump visit */
static uint32_t g_prime_left;      /* priming still owed after a track starts */
```

`of_cdda_play` now primes at most `CD_PRIME_SLICE` (2 batches) and starts the voices, leaving
`g_prime_left = prefill_target - primed`.  `of_cdda_pump` runs the debt down, 24 sectors a visit, guarded by
`ring_ahead()` so the ring cannot be overfilled, and falls through to the budget-limited steady state when the
debt is done.  The failed-read resume path uses the same ramp.

300 sectors primed in 13 visits ≈ 27 ms of pump work spread over frames, instead of ~400 ms in one frame.

## 5. Verified on desktop before asking for a boot

Same command, pre-ramp binary vs post-ramp binary (`--warp-stage 1 --frames 2400`, seed 1234):

| | pre-ramp (0.1.75 shape) | post-ramp (0.1.76) |
|---|---|---|
| `cdda.raw` md5 | `712b6c128a2f631a4dc1ad023f8c8cb6` | `712b6c128a2f631a4dc1ad023f8c8cb6` — **identical bytes** |
| sectors read | 3516 | 3516 |
| worst single fill | 18 148 µs | **2 883 µs** |
| `audio_low` / `dry` / `room_hold` | 0 / 0 / 0 | 0 / 0 / 0 |
| `ahead_min` | 1.44 s | 1.44 s |

The audio stream is unchanged — same sectors, same order, byte-for-byte — while the worst single-frame cost drops
6.3× even on a Mac SSD where reads are 2 µs.  On the card, where a sector costs ~1 000× more, the same shape is
what removes the third-of-a-second silence at each track request.

## 6. What a card log should show after this

- `cdda fills … max=` should stop being a six-digit microsecond number (0.1.75: 418 281 µs).
- `ahead_min` should sit at or above the 120-sector lookahead (70 560 frames ≈ 1.44 s) and not decay to 0.45 s.
- `room_hold` should stop climbing (0.1.75 reached 16 020 during one run).
- `audio_low` and `dry` must stay 0, as they were in 0.1.75.
- The position resets in `audio cdda=` lines are **expected** and are not a defect; they mark the game's own
  room-entry and loop re-requests.  What should change is how long the music is silent after each one.
