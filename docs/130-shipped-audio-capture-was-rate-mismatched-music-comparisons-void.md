# 130 — The shipped-side audio capture was rate-mismatched: every music comparison before this is void

## 1. The measurement that exposes it

`scripts/rig/bringup.sh` set `[mixer] rate = 22050`, while `/api/v1/capture/audio/*` writes a WAV whose header
says **44100**. The consequence is visible in the samples themselves:

| capture | consecutive-equal sample fraction | onset corr, disc track 11 template (title) |
|---|---|---|
| before (`rate = 22050`) | **0.547** | +0.115 |
| after (`rate = 44100`) | **0.011** (route capture 0.023) | **+0.649** |

Half the samples were duplicates of their neighbour: the mixer produced 22050 Hz audio and the capture endpoint
stamped it 44100. That invalidates sample-level comparison against 44.1 kHz CD-DA *and* fine-grained spectral
comparison. Every music-content number in docs/96 §, docs/106 §verdict, docs/113 and docs/114 (fingerprint
corr ≈ 0.2, "NOT FOUND in the original mix") was measured on that broken capture, so those numbers are
withdrawn — including docs/129's "corr 0.933 at lag +7.0 s", which was RMS-envelope correlation on a
half-rate mix.

`bringup.sh` now pins `rate = 44100` with the reason recorded inline.

## 2. Four new oracle halves, each with a positive control

* `scripts/rig/port_mix.py` — rebuilds the port's music timeline from `cdda.raw` + `cdda.jsonl` using the
  **cumulative `bytes` markers as the real read cursor** (docs/129's sequential-cursor caveat), so each play
  gets its own byte range: on `ep1_kill3`, idx 1 = 3121 sectors, idx 9 = 2049, idx 11 = 7840, idx 2 = 19 664
  (of 19 639 requested), i.e. the witness really is 436 s of disc audio.
* `scripts/rig/onset_align.py` — path-robust presence test: onset-energy train at 100 ms, FFT cross-correlation,
  with a sharpness statistic. Positive control (disc template against the port's own CD witness, which *is*
  those bytes): corr **0.47 / 0.96** at exactly the right position (best/sd 5–9).
* `scripts/rig/disc_search.py` — reverse search: take an excerpt of the capture and find it anywhere in the
  269 303-sector image. On the broken capture: best NCC **+0.100** over the whole disc (nothing).
* `scripts/rig/music_presence.py` — hypothesis test with **measured controls**: for each CD play the port
  issued on the same drive, use that event's own audio from the port's witness as the template and search the
  shipped capture in a ±20 s window around the expected moment; then run every mismatched pairing as a control.

## 3. Where the music item stands after the fix

With the corrected capture (g5, `ep1_kill3`, 300 s, 923 distinct states):

```
capture 266.2 s @ 44100 Hz | consecutive-equal 0.023
  idx 1  port stream   0.0 s -> shipped near   3.5 s: corr +0.470
  idx 9  port stream  41.6 s -> shipped near  47.4 s: corr +0.313
  idx 11 port stream  68.9 s -> shipped near  50.2 s: corr +0.186
  idx 2  port stream 173.4 s -> shipped near  70.9 s: corr +0.464
controls (mismatched track/moment pairs):      0.186 .. 0.486
```

Honest verdict: the hypothesis values sit **inside the control band**, so an onset fingerprint with a 12 s
template cannot decide "this track is audible at this moment" in a mixed capture. The fingerprint needs to be
sharper (harmonic/spectral peak-train rather than an onset train), or the comparison has to be done after
removing the board path from the shipped mix. What is *not* in doubt, and what the schedule oracle keeps
showing, is that the selection logic and the CD bytes are equivalent (docs/106, docs/114); the audible layer
remains the open gate item.

## 4. New capability found on the way: I/O-port sampling works

The objective's "I/O-port sampling for DSP/mixer state" channel exists and answers:

```
GET /api/v1/io/port?port=0x22c&size=1   -> {"port":556,"value":127,"width":1}     # SB DSP read status
GET /api/v1/mixer                       -> channels CDAUDIO(enabled, app 0.627) OPL(disabled)
                                           PCSPEAKER(disabled) SB(disabled at the title),
                                           master 0.501, sample_rate 44100
PUT /api/v1/mixer/volume {"channel":"CDAUDIO","left":..,"right":..} -> 200
POST /api/v1/mixer/mute|unmute          -> global mute, not per-channel
```

Two rig notes: the mixer's per-channel `enabled` flag is a *config* state, not "this channel made sound", and
the mute endpoint is global — so attribution by muting cannot isolate CDAUDIO. An A/B that set CDAUDIO to 0 and
then to 1 captured 7.8 s of **rms 0.0 / peak 0** both times, because the session had already left the music
moment: attribution experiments must be scheduled against the drive's own timeline (an anchor), not run
freehand. That is the same lesson docs/101 recorded for input replay.
