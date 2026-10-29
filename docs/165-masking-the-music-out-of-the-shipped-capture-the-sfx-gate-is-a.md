# 165 — masking the music out of the shipped capture: the SFX gate is a coverage problem, and the DS cell can answer less than assumed

The audio half of the closeout contract compares *container audio* to the *port's board PCM*.  docs/164 §7 said
those are not the same quantity: the container's `audio.wav` is DOSBox's mixer, i.e. board **plus** CD-DA.  The
comparable quantity is the shipped capture scored in the intervals where **no track is playing**, because that is
the only part of it the port's board-only stream can be matched against.  `scripts/rig/sfx_mask.py` implements
that mask.  What it took to get a number worth quoting is mostly rig defects of my own.

## 1. Rig defect: a single clock "shift" is wrong; the capture-to-nominal map is piecewise and stretched

The first version derived one median offset between the session's capture clock and the drive's nominal clock and
used it everywhere.  The state series says otherwise: on `/tmp/rig/g16` the mapping is `+47200 ms` at the first
state and `+134 s` at the last, and consecutive 100 ms capture buckets map to nominal times **793 ms apart** — the
wall clock and the drive's scheduled clock differ by a *scale* as well as an offset, because the replay releases
each drive segment when its `wait ds` fires (docs/158's anchor rebase is a jump, not a slide).  The oracle now
interpolates bucket by bucket between neighbouring state samples, and reports the coverage it actually has
(`nominal covered [47222..284034]`) instead of assuming it spans the route.

## 2. Rig defect: masking by dropping buckets destroys the time axis

`buckets_sfx = [b for b in buckets if not armed(b)]` compresses time, so slicing the mask by a phase window
meanings nothing (every phase after the first looked empty).  Fixed: pairs carry their nominal time, and adjacency
for onset counting is taken from **capture** order, because nominal time jumps at a rebase and a nominal jump is
not a sound starting.

## 3. Rig defect: "a sample within ±200 ms" invented 180 silent windows

The first mask called a bucket CD-armed only if a state sample fell within ±200 ms of it.  At the rebase gaps no
sample falls there, so 180 buckets were called CD-silent, and the oracle reported shipped active against a silent
port in one phase (`SILENT`) and a port excess in another (`EXTRA`).  Both were artifacts of the mask, not of the
game.  Fixed by interval semantics: a selection persists until replaced or stopped, so the armed state is the one
held by the last sample at or before the bucket.

## 4. What DS 0x53c44 actually holds, and what it cannot answer

```
nominal interval        cell value   states   capture buckets, mean / peak RMS
47222-47904               185120        4     n=7    mean  892.7  peak 1281.8
48094-72094               223146       57     n=122  mean  535.2  peak 1743.1
72370-284034               40415      782     n=1200 mean 1376.8  peak 4004.8
```

* The cell holds **track LBAs**, not track indices: 185120 = 0x2d320, 223146 = 0x367aa, 40415 = 0x9ddf, the three
  LBAs docs/106 measured for this route's tracks.  (docs/131 called them "track indices"; the values are the
  sector addresses.)
* **The cell is never cleared on this route**, even though the teardown invariant says every replacement is
  preceded by a stop (docs/106, re-confirmed on g16: 6 plays, 7 stops).  A stop leaves the last LBA in the cell,
  so the cell can say *which track was last selected* but **cannot** say *whether the drive is currently silent*.
  Any mask built from "cell == 0" is therefore blind to stop gaps.

## 5. Honest result on ep1_kill3

```
capture: 1334 buckets @44100 -> CD-armed 1333, CD-silent 0
sanity:  buckets over RMS 600: 1158, of which CD-armed: 1158 (100% of the loud capture is music)
port board PCM: 389471 samples in 100/1706 record windows -> 100 buckets, onsets 83, mean RMS 606, zcr 0.503
```

On this drive the shipped side has a track selected for **every captured bucket**.  There is no shipped
board-only evidence anywhere in the route, so the SFX item **cannot be decided on this drive by any oracle** —
the phases are labelled `NO-CD-SILENCE` rather than SILENT/SAME, which is the honest verdict.

Two secondary notes, both about the port's own stream, not verdicts: 100 PCM buckets in 100 of 1706 record windows
(one burst per feed, docs/164 §6) with zero-crossing rate 0.503 against the shipped capture's 0.067 — a board
stream that is noise-like in its sample-to-sample sign pattern while the mixer capture is not.  That is worth a
look, but it is not a comparison the shipped side can currently confirm.

## 6. The drive the gate actually needs

Music selection on this route starts at **47.2 s nominal** (first LBA residence above).  A route that ends inside
the pre-music window therefore has a shipped side with **no track selected at all**, and every sound it makes is
board sound — directly comparable to the port's `pcm.wav`.  `title_basic.txt` for 45 s was launched as
`/tmp/rig/g17` for exactly that reason; the mask reports 0 armed buckets there, and the comparison is clean
without needing any stop-gap detection at all.

## 7. The mask that actually works is content-based

`--mask audio` scores each 2 s window of the shipped mixer capture against the **port's own CD-DA stream**
(`port/cdda.raw`, byte-faithful to the disc per docs/114) with docs/131's semitone peak-set IoU, whose measured
band is 0.966 for a true match against 0.19-0.43 for unrelated material; a window at or above 0.6 is disc music,
everything else is the board's share.

```
/tmp/rig/g17  title_basic.txt, 45 s:   19 windows, 18 at or above 0.6, best IoU 1.00
                capture buckets armed 378, board share 9
                port board PCM: 10375 samples in 2 of 225 record windows

  P0@0      orig 2 onsets active 1.00 rms 972.7 | port 0 onsets active 0.00 rms 0.0   [SILENT]
  P1@10000  orig 0                                    | port 2 onsets active 1.00 rms 473.0 [NO-CD-SILENCE]

/tmp/rig/g16  ep1_kill3.txt, 150 s:    66 windows, 64 at or above 0.6, best IoU 1.00
```

Two things this buys, and one it does not.

* **The shipped side really is playing disc music at the title**, and its melodic peak sets match the port's disc
  stream **exactly** (IoU 1.00 in 18 of 19 windows, and 64 of 66 on the fight route).  The music item's content
  half is confirmed equivalent by a selective test, not by an energy test.
* **The first mask-honest SFX signal**: in the title route's first 10 s the shipped side has board-share activity
  (2 onsets, active in 100 % of its 9 buckets, mean RMS 973) while the port's board stream produced nothing there
  (its whole route has 10 375 samples in 2 record windows).  Filed as a signal, not yet as a defect: a content
  mask can also call a music window "board share" when that window's peak set simply did not match, so this needs
  a second confirmation (disc TRACK 01 data, or the port's own VOC decodes) before it is attributed.
* **It does not make the fight route decidable.**  Even the content mask finds only 2 non-music windows in 150 s of
  `ep1_kill3`, so the SFX gate stays a coverage problem: the drive, not the oracle, is what is missing.

## 8. Rig operation note

The project `.venv` carries PIL but not numpy, so the numpy-dependent oracles (`music_melody.py`,
`music_presence.py`, `audio_ncc.py`, `disc_search.py`, `onset_align.py`, and `sfx_mask.py --mask audio`) run as
`uv run --with numpy python scripts/rig/<name>.py ...`; the non-numpy path (`sfx_mask.py --mask ds`) runs on the
plain interpreter.
