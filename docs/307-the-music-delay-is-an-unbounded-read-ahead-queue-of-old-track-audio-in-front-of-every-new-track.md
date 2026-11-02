# 307 — The music delay is an unbounded read-ahead queue: 4.2-10.0 s of old-track audio sat in front of every new track

Playtest item (docs/manual-playtest/04-2026-11-02.md item C): tracks do change on the recent build, but with an
audible delay.  docs/302 fixed the *rate* (3.64x real time -> 1.04x); this note is about the *latency*, which the
rate gate could not see.

## 1. The instrument

`port/src/app/main.c` now writes `cdda_pipe.jsonl` when `NIGHT_CDDADBG=1` is set with `--rig-dump`:

| line | what it names |
|---|---|
| `device` | `freq` / `channels` of the opened device (avail is counted in *device* bytes) |
| `play` | `cdda_play()` ran -- the game's track request reached the board; `avail = SDL_GetAudioStreamAvailable()` |
| `flush` | the branch that flushes the stream, with the `avail` it found |
| `first_feed` | the first pump batch after a play, `pos` = sectors handed to SDL |
| `stop` | `cdda_stop()` |
| `pipe` | throttled 40 ms sample: `avail` / `pushed` / `pulled` (= pushed - avail) / `pos` / `budget` |

`scripts/rig/cdda_delay.py` reads it and reports, per switch, the bytes queued ahead of the audible head, the
feed and drain rates in sectors/s against real time (75 sectors/s), the frame coupling `t(first_feed) - t(play)`,
and the count of underrun samples (a live track with `avail == 0`).

Canonical drive `scripts/rig/drives/ep1_kill3.txt`, `--frames 9000 --rig-hz 10`, pinned env
`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`.

## 2. Before the fix (`/tmp/rig/mt1`, `/tmp/rig/mt2`)

```
run      switch t(ms)  index  flush  queued ahead B   delay s   coupling
mt1          41100      9     yes        878,592       4.58      0 ms
mt1          54566     11     NO       1,309,696       6.82      0 ms
mt1          74716      2     NO       1,911,808       9.96     17 ms
mt2          40100      9     yes        799,744       4.17      0 ms
mt2          55616     11     NO       1,299,456       6.77      0 ms
mt2          74133      2     NO       1,878,016       9.78      0 ms
```

Plus, for both runs: **peak queue 3.75 MB = 19.5 s of audio**, feed 82.9 sectors/s (**1.10-1.11x** real time),
device drain 71.5 sectors/s (0.95x), 0 underruns in 2771 / 2753 pipe lines.

Two defects are visible in those numbers, and they compound:

1. **The reader is paced by wall time alone.** `allowed = budget + 30` bounds how far the reader may run ahead
   of *elapsed time*, never how far it runs ahead of what the **device has consumed**.  Feeding at 1.10x while
   the device drains at 0.95x makes the queue grow for the whole route: 19.5 s of audio sitting in the stream at
   the end.  (The 0.95x drain is itself partly a consequence of the over-fed queue -- after the fix the same
   dummy device drains at 1.03x.)
2. **The flush at a track switch was conditional.** `cdda_play` only flushed when `g_cdda_sectors != 0`.  Three
   of the four switches arrived with the old track's sector count already exhausted, so nothing was flushed and
   0.80-1.91 MB of *old* track sat in front of the new track's samples.  The new track was therefore audible
   4.2-10.0 s after the game asked for it.

Frame coupling is not a component: 0-17 ms, i.e. under one frame.  The cushion is 30 sectors = 0.4 s, which is
also what the bounded queue ends up holding (below).

docs/302's rate oracle could not see any of this: it inspects *sectors pushed per second*, and 1.04x is not the
same statement as "audio reaches the device when the game asks for it".  Standing rule from this: an oracle that
inspects commands rather than the media they produce leaves the audible half of a bug undetectable.

## 3. The fix

`port/src/app/main.c`:

- `CDDA_CUSHION_SECTORS 30` is now both the allowance granted at PLAY and the **queue ceiling**: the pump reads
  `SDL_GetAudioStreamAvailable()` and shrinks (or zeroes) `want` so the stream never holds more than 30 sectors'
  worth of device bytes.  The ceiling is converted to device bytes by `cdda_dev_bytes_per_sector()`
  (`2352 * dst.freq / 44100`) because the stream is created at 44100 Hz and resampled to the device's rate
  (48000 Hz here), while `avail` counts device bytes.
- `cdda_play` flushes **unconditionally**, before installing the new track, so a queue left by a track that
  finished on its own cannot delay its successor.

## 4. After the fix (`/tmp/rig/mt4`, `/tmp/rig/mt5`)

```
run      switch t(ms)  index  flush  queued ahead B   delay s   coupling
mt4          39200      9     yes         67,584       0.35      0 ms
mt4          54783     11     yes         75,776       0.39     33 ms
mt4          74983      2     yes         74,752       0.39     33 ms
mt5          39400      9     yes         73,728       0.38      0 ms
mt5          55000     11     yes         76,800       0.40     33 ms
mt5          74450      2     yes         84,992       0.44     33 ms
```

| | before | after |
|---|---|---|
| switch delay (old audio queued ahead) | 4.17 - 9.96 s | **0.35 - 0.44 s** |
| peak queue in the stream | 3.75 MB = 19.5 s | **114.6-115.7 kB = 0.60 s** |
| feed rate | 1.10 - 1.11x real time | **1.04x** |
| device drain | 0.95x | 1.03x |
| underrun samples | 0 / 2771 | **0 / 2775** |
| switches that flushed | 1 of 4 | **3 of 3 late switches** (the first track has nothing to flush) |

The remaining 0.4 s is the cushion itself -- 30 sectors granted at PLAY and held as the ceiling -- and it is
deliberate: it is what keeps the device from starving between pump batches.  0.4 s is the design figure, not a
residual.

## 5. The gates docs/302 and docs/304 set, re-checked on these dumps

- **Stream rate**: 1.04x real time in both fixed runs, identical to `/tmp/rig/rb15`'s 1.04x -- no regression, and
  the 3.64x failure mode of docs/302 does not return.
- **Track schedule**: the `(index, lba, sectors)` chain is identical in all four runs --
  `(1,37262,3153) (9,185120,27697) (11,223146,17856) (2,40415,19639)`.
- **Audio onsets**: `scripts/rig/diff_audio.py` run port-vs-port (`/tmp/rig/mt1/cdda.wav` against
  `/tmp/rig/mt4/cdda.wav`, same drive and windows) gives **273 `[SAME]` window lines** and 4 content lines,
  which are the per-track correlation rows at `corr=0.22-0.23` -- those are the instrument floor documented in
  docs/304 (the container capture contains no CD-DA), not port defects.
- **Disc-byte identity**: `scripts/rig/disc_search.py --wav <port cdda.wav> --at 3 --len 6` returns NCC **+1.000
  at disc sector 37337 (absolute LBA ~37487)** for the pre-fix and the post-fix stream alike; the requested track
  1 begins at LBA 37262, so 225 sectors (3.0 s) into the track is exactly where the excerpt matches the image.
- `make -C port test` = `== ALL PASS ==` with the fix in place.

## 6. Basis limits recorded

- A real-device basis is not available on this machine in a headless session: with `SDL_AUDIODRIVER=coreaudio` the
  port logs `audio: default refused (CoreAudio error (AudioQueueStart): 2003329396)` and every alternative, so
  `/tmp/rig/mt3` ran with **no stream at all** -- `avail` reads 0 there because `g_cdda_stream` is null, and its
  "2735 underrun samples" is an artifact of that, not a finding.  The delay measured above is the port's own
  pipeline depth (`SDL_GetAudioStreamAvailable`), which is what the device sits behind.
- The canonical route's shipped half could not be run for this note: the OrbStack docker engine does not answer
  (`docker ps` and `docker version` both time out at 200 s; `curl --unix-socket ~/.orbstack/run/docker.sock
  /_ping` exits 28), so the pre/post comparison above is port-side against the pre-fix port dumps, which is the
  same-basis comparison the fix needs.  The shipped-side state/screen oracles are re-run in the regression sweep.
