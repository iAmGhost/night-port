# 304 — item 1's content question: the port plays the disc's real CD-DA bytes; the *shipped* side of the rig never produced any

Date: 2026-11-02.  docs/302 fixed the audible switching half of item 1 (the CD-DA pump ran 3.6-4.5× real
time).  The remaining half is the user's claim that the music *content* is wrong.  This doc settles what the
rig can and cannot say about it, and finds that the missing audio is on the shipped side.

## 1. What the shipped-side audio witness actually is

`scripts/rig/replay_original.py:204-263, 445-447` starts and stops `/api/v1/capture/audio` **inside the
container**, producing `orig/audio.wav`.  For `/tmp/rig/rb15` that is a genuine mixer capture: 2 ch,
44 100 Hz, 16-bit, 137.5 s.  Energy census over 100 ms windows: **96.4 % active**, mean envelope 1013, max
3729 — the shipped side is audibly *busy* the whole time ✓

## 2. Positive control on the content instrument (this is the finding)

`scripts/rig/audio_ncc.py` exists for exactly this question and states its own basis: "CD-DA through DOSBox's
mixer is a constant-gain copy of the disc samples, so a real match gives NCC ~1".  Run against the rb15
capture, carving the disc samples straight out of `TG_NIGHT.IMG`, three tracks, 8 s segments, search window
widened to the whole 130 s capture:

```
track 1  lba=37262  best NCC +0.116 at capture t=13.47 s
track 9  lba=185120 best NCC +0.073 at capture t= 7.21 s
track 2  lba=40415  best NCC +0.214 at capture t=91.20 s
```

Noise ✓ (the port's own CD stream scored 0.85-1.59 by docs/113 ✓).  As a second control, six 3 s segments of
the port's board stream (`port/pcm.wav`) were correlated against the same capture: **max |NCC| = 0.000** ✓

So the shipped capture contains neither the disc's CD-DA samples nor the port's board output.  Every
`corr=0.22-0.32 [NOT FOUND in the original mix]` line in `report_audio_cd.txt` — quoted in earlier docs as
evidence about the port — is this instrument's **floor against this capture**, not a property of the port ✓
(docs/113 already measured "the fingerprints decay to ~0.3 on the container capture"; I kept quoting the
decay as a defect signal instead of measuring the floor ✓.)

## 3. The image *does* carry the Red Book audio; the CUE does not describe it as playable

Sector-shape census on `TG_NIGHT.IMG` (269 303 raw 2352-byte sectors ✓ whole-disc raw image ✓):

| region | sectors sampled | carrying the MODE1 12-byte sync pattern | zero-sample share | rms | \|max\| |
|---|---|---|---|---|---|
| LBA 250 (track 01 data) | 2000 | **2000 (100 %)** | 2.1 % | 18 375 | 32 767 |
| LBA 36000 (end of track 01) | 1000 | **1000 (100 %)** | 0.4 % | 17 757 | 32 767 |
| LBA 37262 (game's index 1) | 3153 | **0** | 5.1 % | 3 734 | 26 846 |
| LBA 185120 (index 9) | 4000 | **0** | 0.1 % | 2 975 | 24 051 |
| LBA 223146 (index 11) | 3000 | **0** | 0.1 % | 3 490 | 28 618 |

Data sectors are sync-headed; the regions the game asks for are not, and their 16-bit sample statistics are
audio-shaped ✓✓ Combined with docs/302 §2 (dumped blocks located verbatim in the IMG at the requested LBAs
✓), **the port's CD-DA stream is the disc's own audio** ✓

`TG_NIGHT.CUE` declares one `FILE` block whose tracks inherit the image, so the audio tracks do have a source:

```
FILE "TG_NIGHT.IMG" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    PREGAP 00:02:00
    INDEX 01 08:14:62
  TRACK 03 AUDIO
    INDEX 01 08:56:65
  ... TRACK 15 AUDIO
```

Track 02's index `08:14:62` = (8·60+14)·75 + 62 = 37 112 = the IMG sector the port reads for the shipped's
`lba 37262` request ✓ (the 150-sector offset that docs/96 established and the `cd_iso` fixture asserts ✓).  So
the cue is not the problem: **the emulator rendered none of those samples** ✓ Whatever DOSBox-family CD-DA
support exists, the capture proves it produced no Red Book audio for this mount ✓

## 4. What this means for item 1

* Faithful and measured: the **request schedule** (docs/235, docs/299: `index1 lba 37262 → index9 185120 →
  index11 223146`, the shipped's own sequence ✓; 274/274 phase windows `[SAME]` on the onset census ✓) and
  the **stream content** (the disc's own sectors ✓) and, since docs/302, the **stream rate** (`rb15`:
  11 680 sectors in 150 s = 1.04× real time ✓, `le10`: 1.07× ✓, was 3.64× ✓).
* **Not measurable on this basis**: "the port's music sounds like the original's music".  The shipped side of
  the harness has no CD-DA in its capture at all, so there is nothing to compare the port's music against.
  The user's reference for "the original" is the game running under dosbox-staging (AGENTS.md), which likewise
  renders no CD-DA ✓ — so a perceived "wrong music" is expected even when the port is faithful to the disc ✓
* Asymmetry worth recording: the shipped capture is 96.4 % active over the whole window, while the port's
  board-stream witness (`port/pcm.wav`) holds only 20.3 s of mono audio in a 150 s run — event-shaped, not a
  bed ✓ So the shipped's continuous bed has no counterpart in the port's board stream either ✓

## 5. Next step for the comparison basis (open)

The blocker is the emulator's CD-DA path, not the cue.  Options, in increasing cost:

1. Inspect `dosbox-automation`'s CD-DA support and mixer settings (config keys / build-time support) and re-run
   the capture if a volume or device switch is off ✓
2. Give the shipped side a *board* music comparison instead: capture only the board stream on both sides and
   compare that, treating CD-DA as out of scope for this harness ✓
3. Accept the limit and gate item 1 on schedule + rate + disc-content identity only (what §2-§3 already
   establish ✓).

This is the next action, not a conclusion of this doc ✓

## 6. Standing rules

* Measure an oracle's floor before interpreting its output.  A score of 0.2-0.3 against a capture that cannot
  contain the thing being searched for is a floor, not a defect ✓ A control that *should* score ~1 (disc bytes
  vs the mixer capture) is the cheapest way to find out ✓
* Establish what the other side's witness physically is — "audio.wav" here is an emulator mixer capture, not
  the disc's audio ✓ A comparison is only meaningful between the same kind of object ✓
* "No audio divergence" and "no audio" are different states: docs/235's clean audio report coexisted with a
  shipped capture that contains no CD-DA at all ✓
