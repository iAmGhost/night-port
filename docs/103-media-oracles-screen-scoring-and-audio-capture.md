# 103 — Media oracles: per-frame screen scoring with attribution, and real audio capture

`rig-runtime` → `rig-media-oracles`. Continues docs/101 (determinism) and docs/102 (state oracle).

## 1. Screen oracle: one record layout, two producers, whole-route scores

Both sides emit the same 32-byte record plus a 40×30 mean-luminance grid (1.2 KB/frame), so a route of
thousands of frames costs megabytes instead of 128 MB of bitmaps:

```c
struct { int64_t t_ms; uint64_t frame; int32_t w, height, changed, pad; }   /* 32 B, no padding */
```

Field widths are chosen so every member is naturally aligned — the first version put `uint8_t
changed` after `int32_t`s, the compiler padded the struct to 32 while the Python reader packed 27, and
every frame after the first was garbage. Producers: `port/src/app/rig_sink.c` (`rig_screen_frame`,
called from `present()` with the **explicit** byte offsets `(3,2,1)` because the port's RGBA8888 word
is little-endian, so its memory bytes are A,B,G,R) and `scripts/rig/capture_original.py`
(`/video/frame?format=raw`: a 15-byte header — w, h, pitch, bpp — then BGRX32 or index pixels with a
trailing 768-byte palette; measured 14 ms/frame, so ~25 Hz capture).

`scripts/rig/diff_screen.py` aligns the two streams with a lag-window DP (frame numbers are not
comparable across sides, docs/56) and reports:

```
orig frames=497 distinct_pictures=55 | port frames=3286 distinct_pictures=810
aligned pairs=497 route score=0.3514 (1 = identical)
divergent runs (score < 0.88): 4
  orig t=1082..11573 ms (10.5s) worst score=0.240
      attributed: no dumped DS cell changed at this moment
pacing check (docs/47, docs/82): picture changes per second orig=2.20 port=7.64 ratio=0.29  <- pacing or slide-count divergence
```

**Content attribution** is computed, not eyeballed: for each divergent window the two DS snapshots that
bracket its start are diffed and the changed cells are named through the shipped symbol table; the
measured entropy cells of docs/101 are excluded first, otherwise the clock cells (`0x63e24`,
`0x63e30`, …) appear in every attribution and say nothing. A window where *nothing* changed is reported
as such — that is a real observation (the original is holding a static card while the port redraws).

Two premise checks are built in rather than left to a reviewer:

- **channel order (docs/70, docs/79)**: aligned witness frames are re-scored with R and B swapped;
  `declared=0.976 R/B-swapped=0.976 (chroma 32.1) ok`. When the sampled frames are near-greyscale the
  check prints *inconclusive* with the mean |R−B| instead of pretending to discriminate.
- **pacing / phase pinning (docs/47, docs/82)**: raw capture rate (20/s) and present rate (110/s) are
  explicitly *not* comparable — they are the poll rate and the host's unthrottled loop — so the check
  uses picture-changes-per-second, which is what the game's retrace/timer pacing actually shows. The
  10× divergence at the title is the runtime signal behind the "no slideshow track" playtest item.

## 2. Audio oracle: the container *does* capture audio

Correction to docs/101: `GET /openapi.json` lists only `capture/video/*`, but the binary exposes
`/api/v1/capture/audio/{start,status,stop}` (found with `strings bin/dosbox`, since the tarball's
`dosbox-automation` is a shell wrapper — the first grep looked at the wrapper and found nothing).
Verified live: start/stop answer `{"status":"recording"}` / `{"status":"stopped"}` and write
`/root/.config/dosbox-automation/capture/audioNNNN.wav` inside the container. The capture taps the
mixer, not the host device, so it records real samples even with `SDL_AUDIODRIVER=dummy`: 44100 Hz
stereo, RMS 786, 142 onsets in ~6 s.

Comparison is by event window on activity features (RMS envelope at 10 Hz, audible fraction, onset
count, zero-crossing rate), never waveform identity — the sides run at 44.1 kHz stereo and 22.05 kHz
mono with different clocks. `scripts/rig/diff_audio.py` on a 25 s title route:

```
original: 23.4 s @44100 Hz -> 234 buckets   {onsets 17, active_frac 0.739, mean_rms 476}
port    :  0.5 s @22050 Hz ->   5 buckets   {onsets  1, active_frac 1.000, mean_rms 1785}
  P0@0      orig onsets=12 active=0.45 | port onsets=0 active=0.00   [MISSING]
  P1@10000  orig onsets= 5 active=0.95 | port onsets=0 active=0.00   [MISSING]
  P2@20000  orig onsets= 2 active=0.97 | port onsets=0 active=0.00   [SILENT]
```

The port emitted 0.5 s of PCM across a 22 s run where the original is audible 74 % of the time — the
runtime signal for the music/SFX items. Two rig bugs found on the way, both worth recording because
they would have silently produced this verdict for the wrong reason: the hand-written WAV header was
byte-misaligned (`ata` where `data` should be), and its sizes were never finalised because nothing
called `rig_close()` — the WAV declared zero frames, i.e. the port looked silent. `diff_audio.py` now
re-derives the port's sample rate from its own record timeline and falls back to raw-PCM parsing when a
header is unusable.

## 3. DS-activity oracle (kept as the clock-free audio proxy)

`scripts/rig/diff_activity.py` compares per-cell *transition rates* (transitions/second, not raw
counts — the two sides are sampled at different densities) per drive phase. On a title-only route it
reports 229 SAME: honest but weak, which is why the real audio capture above matters and why the
fuzzing routes must reach gameplay.

## 4. One command, and the injected control

`scripts/rig/route.sh [drive] [seconds]` runs the port (`--rig-dump`), boots the original twice in
fresh containers (state replay with `--audio-capture`, then a screen capture), pulls the captured WAV
out with `docker cp`, and runs the four oracles into `$OUT/DIVERGENCE.md` with timestamps. A 25 s route
takes ~3 minutes; every step is timeout-capped.

Injected controls (rig-runtime contract — the rig must catch a deliberately broken build).
The port host accepts `NIGHT_RIG_CONTROL=<fault>`; each fault stubs a piece of the shipped game rather
than faking a value. Measured on the same 25 s title route against the same original replay
(`/tmp/rig/route-a1/orig`), the rig's own outputs, nobody told which fault was in force:

| build | state-oracle tally | screen route | pacing (changes/s, orig 2.20) |
|---|---|---|---|
| clean | DIFF 77, REC-DIFF 37, REC-NULL 6, SAME 67 | 4 divergent runs | port 6.91 → ratio 0.32 |
| `no_timer` (fn_03ee88 never called) | DIFF 76, **REC-SAME 6, REC-NULL 37**, SAME 67 | 4 divergent runs | port 4.52 → ratio 0.49 |
| `no_input` (scripted keys never reach the key state) | DIFF 78, REC-DIFF 37, SAME 67 | 4 divergent runs | port 12.66 → ratio 0.17 |

The signatures are different and both are machine-visible: `no_timer` freezes *game* time, so the
successive record snapshots stop changing and the state oracle's verdict for the record stream flips
from `REC-DIFF` to `REC-SAME`/`REC-NULL` — that appears in no other build. `no_input` leaves the port
sitting on the attract card, whose redraws push its picture-change rate to *twice* the clean build's,
so the pacing check separates 12.66 / 6.91 / 4.52 without knowing which fault ran.

Correction, recorded because it matters: an earlier draft of this section cited a `no_timer` run with
numbers like "2.85 → 14.52 changes/s" from before that control existed (only `no_draw` was
implemented then, so that run measured nothing injected). The table above is the measurement.

`NIGHT_RIG_CONTROL=no_draw` (compose, never present) is **not** observable through the port's own
screen witness — the witness is the composed buffer, taken before the present decision — and is
recorded as a hole rather than pretended away.

## 5. Reproduce

```sh
OUT=/tmp/rig/route-a1 bash scripts/rig/route.sh scripts/rig/drives/title_basic.txt 25
python3 scripts/rig/diff_screen.py --orig $OUT/orig_screen/frames.bin --port $OUT/port/frames.bin \
        --orig-run $OUT/orig --md $OUT/report_screen.md
python3 scripts/rig/diff_audio.py  --orig $OUT/orig/audio.wav --port $OUT/port/pcm.wav \
        --port-cells $OUT/port/cells.bin --drive scripts/rig/drives/title_basic.txt
NIGHT_RIG_CONTROL=no_timer timeout 90 ./port/test/night --res-dir ./NIGHT \
        --script scripts/rig/drives/title_basic.txt --rig-dump /tmp/rig/ctl-no_timer --rig-hz 10 --frames 1600
```
