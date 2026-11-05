# 364 — the frame budget, instrumented: what a frame costs, what streaming the disc costs, and whether the mixer ever ran dry

Task-9a's contract is the four numbers a Pocket run is judged on -- ticks, present ms, SD read ms, mixer
underruns.  Three of them existed in fragments and one did not exist at all, and the first version of the fourth
was reported under a name that meant the opposite of what it measured.

## 1. What the host already reported, and what it could not

| quantity | before | after |
|---|---|---|
| tick rate | `memstat ticks=…(+491/s) pit=490 Hz` ✓ already proven (docs/335) | unchanged |
| present cost | `perf pres=<last frame> max=<worst frame>` -- **no mean**: 3.6 ms typical with a 75 ms worst looked the same as 3.6/7.5 | `budget present n= mean= max=` (`g_present_us_sum / g_present_n`) |
| present work split | `stages last m= p= c= f= x= \| max all= copy= flip=` ✓ | unchanged |
| SD / slot read cost | **absent**: a sector read was a black box | `of_disc_stats()`: reads, total µs, worst µs, bytes, seeks, failures |
| mixer underrun | **absent**: `ring=`/`head=` were printed but nothing counted the failure they describe | `of_cdda_stats()`: fills, fill µs, `audio_low`, `room_hold`, `clock_behind`, `dry`, `ahead_min`, pump visits |

## 2. The counters, and the one that was misnamed at first

`of_cdda.c` now counts, per pump visit:

- **`dry`** -- the real underrun.  `clock_consume()` computes how many frames the looping voices must have eaten
  since they started (`(now - g_play_us) * 44100`); when that exceeds `g_done_frames`, the host has written less
  audio than the mixer has played, and the voices are looping over stale or silent samples.  Counted **before** the
  clamp, because the clamp is what hides it.  This is the failure mode the whole CDDA engine exists to avoid; its
  mirror image (the write cursor lapping the voice) is the buzz docs/338 traced.
- **`audio_low`** -- less than the 0.1 s fill guard (`CD_GUARD_FRAMES` = 4410 frames) buffered *with sectors still
  to read*.  The tail exclusion matters: every track the game re-requests ends with the ring draining before
  `fn_010b1d` is called again, and the original's single MSCDEX pickup does the same.  Counted without that
  exclusion, stage 1 scored `audio_low=20, ahead_min=0` at track boundaries and looked broken while it was fine.
- **`room_hold`** -- the disc still had sectors and the pump was ready, but the ring had no room in front of the
  voice: the reads are faster than the voice eats them, which is the good direction.
- **`clock_behind`** -- the 75 sectors/s disc clock had passed `g_pos`, i.e. the host fell behind the disc's own schedule.
- **`ahead_min`** -- the smallest buffered slack the run held, in ring frames (44 100 a second).

The first version named `room_hold` as `blocked` and lumped it with underrun risk.  A stage-1 run reported
`blocked=310 030` and `ahead_min=1.59 s` at the same time -- a true count under a misleading name.  The reason is
structural: `of_cdda_play()` pre-fills the **whole** 176 400-frame ring (4 s), so for the first seconds of every
track the write cursor is a full lap ahead and the pump is right to refuse.  Split into separate counters, the
same run reports `dry=0 audio_low=0 room_hold=26`.

## 3. The finding: the CDDA pump ran 150 000 times a frame

`of_cdda_pump` is called from the pump, which the game's own spin loops re-enter (`fn_00035ec2` and friends call
`sys_port_spin`).  Added a visit counter to get a denominator for `room_hold`, and the denominator was the story:

```
before:  cdda visits=295 921 377 in 1964 frames        ~150 000 visits per frame, 0.1 % of which had anything to do
after:   cdda visits=16 430       in 1964 frames        one visit per tick (490 Hz)
```

The pump now runs **once per tick** instead of once per spin (`main_of.c`'s `cdda_pump()`), gated on `g_timer_calls`
so the gate costs no clock read.  The safety argument, in the numbers the engine itself is built on: its finest
action is one 12-sector chunk = 160 ms of audio, its lookahead is 120 sectors = 1.6 s, its ring is 4 s, and every
decision it makes is derived from elapsed time (`g_budget += elapsed × 75`), not from visit count.  A 2.04 ms gate
is 78× finer than the finest thing it does.

Verified against the unthrottled run of the same shape (stage 1, 2500 frames, same seed, same CD staging dir):

```
throttled   : audio cdda=2737/19639/4 plays=4 stops=4 sfx=8/0/0 ring=70630 head=70630   refusals/failures: 0
unthrottled : audio cdda=2738/19639/4 plays=4 stops=4 sfx=8/0/0 ring=70853 head=70853   refusals/failures: 0
budget      : dry=0  audio_low=0  clock_behind=0  ahead_min=70057 (1.59 s)
```

One sector of difference (13 ms of a 4.4-minute track, at a track boundary) and no underrun in either.

### 3.1 How the change was actually tested -- and what `frames.bin` cannot prove

The obvious test -- diff the 40x30 luma signatures in `frames.bin` -- looked alarming and is not a determinism
proof on a stage with moving content.  Four stage-1 runs (`--warp-stage 1 --frames 2500`, seed 1234):

| pair | frames | differing signatures |
|---|---|---|
| two throttled runs, same binary, same config | 2501 | 152 (6.1 %) |
| another throttled run vs those two | 2501 | 1215 / 1284 (48.6 % / 51.3 %) |
| throttled vs unthrottled | 2501 | 1739 / 1761 (69.5 % / 70.4 %) |

So run-to-run noise in stage 1 ranges from 6 % to about half the frames depending on which parts of the scene are
animated, and a signature comparison cannot separate "the throttle changed the game" from "the animation was one
tick further along when this frame was sampled".  Stage 9, whose warp run holds one live entity and two SFX blocks,
repeats at 26/2001 differing (1.3 %) -- the same instrument is tight where the scene is static.

What does hold across all four runs, throttled and not, is the structural timeline:

```
stage loop reached      : yes in all four
disc transfers (xfer)   : 8      sfx plays/drops/steals: 8/0/0     arena high-water: 3 903 168
view segments           : 640x480 for 10 sampled seconds, then 320x200 for 35   (identical in all four)
cdda.jsonl              : 4 play records, payloads identical; t_ms offsets differ by a constant boot offset,
                          their mutual spacing by <= 18 ms
pcm_cd.wav              : 6 585 644 bytes in both
```

That is the evidence used for "no behaviour change": the game's requests, the host's transfers, the view timeline,
the arena high-water and the delivered PCM are the same, and `dry`/`audio_low` stayed at zero.  Per-frame pixel
equality is not available as a test on this host for an animated stage, which is the same reason docs/333 compared
mean RGB between the two hosts rather than bytes.

## 4. Measured budget, desktop shim (`app_pc`, `-DNIGHT_PC`, dummy video/audio, `NIGHT_CD_DIR=scratch/pocket/cd`)

`--warp-stage N --frames 2000`, `NIGHT_SEED=1234`, `FB_MODE=1` (the 640×480 view presented as-is), post-throttle:

| stage | present mean | present worst | disc reads | read mean | read worst | fills | fill mean | fill worst | frames of PCM | visits | dry | audio_low | ahead_min |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 493 µs | 4 518 µs | 3 263 | 5 µs | 806 µs | 2 067 | 37 µs | 5 161 µs | 1 918 644 | 16 430 | 0 | 0 | 70 051 (1.59 s) |
| 5 | 446 µs | 3 138 µs | 2 915 | 9 µs | 1 511 µs | 2 018 | 47 µs | 6 761 µs | 1 714 020 | 14 932 | 0 | 0 | 69 971 (1.59 s) |
| 9 (run 1) | 475 µs | 2 484 µs | 3 262 | 1 µs | 329 µs | 2 066 | 33 µs | 5 741 µs | 1 918 056 | 16 424 | 0 | 0 | 70 073 (1.59 s) |
| 9 (run 2) | 467 µs | 2 977 µs | 3 263 | 2 µs | 800 µs | 2 067 | 32 µs | 6 223 µs | 1 918 644 | 16 432 | 0 | 0 | 70 078 (1.59 s) |

Nine-stage sweep, 4500 frames each, presented-frame maxima (docs/363): 2 467 / 2 535 / 2 930 / 3 185 / 4 866 /
8 977 / 13 053 / 16 666 / 2 898 µs, tick rate +491/s against `pit=490 Hz` in all nine, arena high-water
3.90-5.25 MB of the 16 MiB budget.

The worst-case fill (`max=5 161 … 6 223 µs`) is the 300-sector whole-ring prefill: 705 KB read, de-interleaved,
written into the mixer's SDRAM pool and cache-flushed in one call.  A steady-state chunk (12 sectors, 7 056 B)
costs tens of microseconds.

## 5. The SD number, and why this machine cannot produce it

The desktop `disc reads` column is *file* reads from `scratch/pocket/cd`, not SD reads: mean 1-9 µs is a page
cache, and it is deliberately reported as such rather than dressed up as a device number.

Trying to measure the card from macOS fails honestly rather than convincingly:

```
/Volumes/Pocket/Assets/night/common/track03.bin, 2352-byte unbuffered reads:
  warm region   : 1700 reads -> 0.5 us/read  (4 574 MB/s)
  cold region   : 1700 reads -> 0.7 us/read  (3 490 MB/s)   (offset 42 MB, never read this session)
  1 MiB sequential: 40 MiB in 0.478 s -> 87.7 MB/s
```

3.5 GB/s from an SD card is the mount's readahead, not the media -- the Mac's filing layer reads ahead of every
small request, so per-read cost is unmeasurable this way, and it is a different file service from
`of_file`/`slot:N` over the Pocket's SPI anyway.  **The real SD read number has to come from the core's own
budget line on the device.**  That is why the core on the card is 0.1.54 with `LOG=1`: its `budget …` line reports
`disc reads=… mean=…us max=…us seeks=… fails=…` for the Pocket's own slot reads, in `Saves/night/common/nightlog.sav`.

## 6. What the numbers say about task-9's tuning options

- **Tick-locking the CDDA pump is the cheapest win found so far** (~18 000× fewer visits, no audio change) and it is
  already in the 0.1.54 build on the card.  The same argument applies to the other spin-driven work; the pad pump
  was already gated this way (one poll per ~8 ms, docs/333).
- **The presentation half is the half that is large.**  640×480 presents measured 21 344 µs mean / 75 304 µs worst
  against 3 608 µs for 320×200 (docs/343); `FB_MODE=0` (host-side downscale to the panel) is the one-line change
  that trades that for a resample, and it is still unmeasured on hardware.
- **The disc half is not the bottleneck on desktop**, and the ring never held less than 1.59 s of audio.  An async
  `of_file` read is therefore a *hardware* question, which is exactly what the 0.1.54 budget line exists to answer.
- Build flags: the device build already carries `-O3 -flto=auto` (Makefile:52) on top of the SDK's `-O2` base, so
  the flag lever is already pulled; what remains is where the cycles go.

## 7. Reproduce

```sh
cd pocket/src/night && make test                     # app_pc with the budget line
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234 \
NIGHT_CD_DIR=scratch/pocket/cd NIGHT_LOG=1 \
./app_pc --warp-stage 1 --frames 2000 --rig-dump /tmp/b --rig-witness 0 | grep '^budget'
```

On the device: `night.ini` `LOG=1` (measurement cycle only; the shipping default is `LOG=0`, docs/362), boot 0.1.54,
then read `Saves/night/common/nightlog.sav` and grep `^budget`.

Fixture suite re-run after these changes: `make test` at the repo root -> `== ALL PASS ==`.  `src/f` untouched.
