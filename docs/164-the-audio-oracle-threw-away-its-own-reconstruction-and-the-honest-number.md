# 164 — the audio oracle threw away its own reconstruction, and the honest number behind it is worse

The closeout sweep was run on the current tree (route `/tmp/rig/g16`, drive `ep1_kill3`, 150 s, port built by the
route itself).  The audio half reported the port as silent in every fight window.  Before believing that I
checked the oracle, and the oracle was broken.

## 1. Rig defect: the reconstructed envelope was overwritten two lines after being built

`scripts/rig/diff_audio.py` reconstructs the port's board envelope from the per-record PCM byte counters
(the docs/129 §2 fix) because the WAV header carries a *nominal* 22050 Hz while the board rate is whatever DSP
command `0x40` last programmed.  It then did this:

```python
            ep, rp = envelope_from_windows(out, rate_fallback=board), rp   # reconstruction
            ...
    eo, ep = envelope(so, ro), envelope(sp, rp)                             # <- throws it away
```

so every number the report printed for the port came from the naive fixed-rate path anyway.  The tell was in
the report itself: `port: 17.7 s @22050 Hz -> 177 buckets` while the reconstruction had just said
`389471/389471 samples placed into 100/1706 record windows at the board rate 10989 Hz`.

Fixed: the reconstruction is kept when it exists, and the report line now states which basis it used.

## 2. With the reconstruction actually used, the port's board stream is *near-silence*, not merely short

```
port PCM: 389471/389471 samples placed into 100/1706 record windows at 10989 Hz
          (35.4 s of sound in 247.7 s of route)
original: 133.4 s @44100 Hz -> 1334 buckets   onsets=8  active=0.961  rms=1294.1  zcr=0.068
port    : 35.4 s @10989 Hz  -> 2113 buckets   onsets=5  active=0.003  rms=  21.0  zcr=0.362
  P0@0      orig onsets=5 active=0.90 rms= 928.9 | port onsets=5 active=0.01 rms=  94.0
  P1..P16   orig onsets=1-2 active=0.95-1.00 rms=1100-2000 | port onsets=0 active=0.00 rms=0.0
```

Two separate statements, and they must not be merged:

* **Coverage**: PCM is produced in 100 of 1706 record windows — 14 % of the route.
* **Level**: the samples that *are* produced average **RMS 21** against the shipped mixer's **1294**, i.e. they
  are essentially silence with a little noise (the zcr 0.362 vs 0.068 says the little that is there is high
  frequency, not signal).  The old basis reported RMS 3524.9 for the port, which was not the port's audio at
  all — it was 35 s of samples stretched over the whole route and averaged against silence buckets.

## 3. What the port-side wiring is known to be doing

Read while checking the coverage claim, so the next measurement starts from facts rather than hypothesis:

* the DMA source the model reads is `sys_audio_set_source(g_dat_63fb4)` (`port/src/app/main.c:399`), and
  `g_dat_63fb4` **is live in the port**: allocated `fn_00040788(&g_dat_63fb0, 0xffff)` at `f.c:8305` (a 0x10000
  buffer) and filled by `mem_copy(g_dat_63fb4, g_dat_63fa8, g_dat_63f98)` at `f.c:8417` and `f.c:8457`.  So this
  is not docs/112/113's "unfilled array for a live DS address" family.
* the shipped dump carries **no cell** for `0x63fb4` (`ORIG idx -1 cell absent`) while the port dumps it as
  `guest_size=4, host_size=8` — the port's own 0x10000-byte host buffer is being compared as a 4-byte DS
  global, which is a `host_address_space` case under docs/134's families and should be reclassified so it stops
  consuming a DIFF slot.
* the board counters exist and are printed in the retracing build (`sys_audio_transfers()`, `sys_audio_stops()`,
  `sys_audio_rate()`, `sys_audio_width()`, `sys_audio_pending()`, `sys_audio_pushed()` at `sys.c:161-166`); the
  plain run's `pump:` line reports `pit=490 Hz` and the DS timer cells but not the transfer counters, so the
  transfer/stop census needs the debug switch that enables `audiodbg` (`main.c:346`) or the shutdown line at
  `main.c:423`.

## 4. Next measurement, in order

1. **Transfer census against play census**: enable the `audiodbg` counters and compare `transfers`/`stops` with
   the `NIGHT_ACTTRACE` SFX witness (`played=74` on this drive, docs/113).  If plays fire but transfers are
   ~100, the board is *not being programmed per play* — that is the missing-Punch-SFX item's real mechanism.
2. **Level check inside a known play**: the docs/129 §4 probe (`NIGHT_PLAYVOC=7`) showed the slot's VOC block
   decodes with PCM peak 13568.  Take one play, dump the bytes the model pushes for that transfer, and compare
   the peak against 13568 — that separates "the samples are wrong" from "the samples are never pushed".
3. **Music masking for the SFX oracle**: `orig/audio.wav` is the container's *mixer* capture (board **and**
   CD-DA), while `port/pcm.wav` is board-only.  Comparing them overstates the shipped side.  The shipped DS cell
   `0x53c44` (current track index) is sampled at 10 Hz in the same session, so the shipped CD-armed intervals
   are known; the SFX oracle should score shipped activity only in the CD-silent intervals.

## 5. Closeout sweep, same route (for the record)

| gate | result |
|---|---|
| `make -C port test` | `== ALL PASS ==` |
| `make -C port selftest` | `ALL PASS (0 failures)` |
| nine-episode ASan/UBSan soak (`/tmp/rig/soak_final`) | 9/9 stages `rc=0 ubsan=0 asan=0 segv=0`, all reach 4000 frames |
| state oracle, activity-profile half | 269/269 SAME |
| state oracle, value half | 57 divergent cells, **38 same-width value divergences**, 42 classified NON-GAMEPLAY, 13 REC-NULL |
| screen oracle | 6 runs under the 0.88 score floor; pacing orig 5.07 vs port 28.15 changes/s |
| audio oracle | board half unreliable until §4 is done; CD-DA half reported separately |

The 7 manual-playtest items are all checked with ROOT CAUSE / DOCS / EVIDENCE in
`docs/manual-playtest/02-2026-10-28.md`, but **the state and audio diffs are not green**, so the goal is not
complete and no completion claim is being made.

## 6. The board path, measured with witnesses instead of inferred

`NIGHT_AUDIODUMP` (already in `sys.c:184`) dumps the exact bytes each DMA arm puts on the wire.  60 transfers on
`ep1_kill3`, 6000 frames: **all 60 carry `n=3607`, `width=1`, `rate=10989`, `ch=1` and byte-identical content**
(`7f 80 80 81 83 89 8f 90 ...`).  That looked like "the port sends the same samples for every sound", so the
feed side got a witness too, and the picture is different:

```
sfxfeed (fn_04097e)   73 calls      audiodump (DMA transfers)  73   <- one feed per transfer
sfxcopy (fn_040b83)    0 calls      -- the duplicated feeder is never used on this route
sfxplay (element-event path, f.c:23207)  10
distinct feed sizes:   3609 x70, 5348 x1, 5031 x1, 4916 x1
```

* **The transfer path is not dropping the sound identity.**  Three feeds carry different block sizes, so different
  sounds do reach the wire; 70 of the 73 feeds are the *same* 3609-byte block, and `n = 3607 = 3609 - 2` matches
  the header stripping in `fn_04097e` (`total - 2` at `0x40a23-29`, the device is told one byte fewer at
  `0x40a9a`).  The open question is now behavioural: *why is one 3609-byte sample fed 70 times* on this drive.
* **No address bug in `au_start()` after all.**  It reads `au_src[0 .. n-1]`, which is right, because the feeder
  programs the DMA address *from the buffer base*: `g_dat_63fac = g_dat_63fb4 (+ ds_lin)` at `f.c:8308/8312` and
  `outb(0x02/0x03)` plus page `0x83` from that value (`fn_040b83`'s documented burst).  The recorded
  `dma_addr` (0x381c on one boot, 0x781c on another -- varies per boot, never per transfer) is that buffer's own
  address.  A real candidate was checked and not filed.
* **`AUDIO_RING` is not the constraint** (1 << 17 bytes vs a 3607-byte block), and the 8-bit conversion in
  `sys_audio_fill` (`(b - 128) * 256`) is correct, so the low level is not a ring overflow or a signedness slip.
* The witnesses are env-gated (`NIGHT_AUDIODUMP`) and cost nothing when unset.

## 7. Consequence for the audio gate

The SFX verdicts in §2 still compare a *mixed* container capture (board + CD-DA) against a board-only stream, so
they cannot clear or fail the gate.  With the feed census in hand, the comparable statement is now countable on
both sides: **one feed per transfer, 73 feeds on this route, 70 of them the same block** -- and the shipped side
has to answer the same three numbers before "missing punch SFX" can be called fixed or open by the rig alone.
