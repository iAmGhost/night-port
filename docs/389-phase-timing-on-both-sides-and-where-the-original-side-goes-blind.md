# 389 — phase timing on both sides, and where the original capture goes blind for 33 seconds

Docs/381-388 compare *pictures*.  This compares *when each side reaches each phase*, which is the axis that answers
"does the port play the game at the game's pace" -- and it came out of a false alarm: I read the port's intro as six
times too long (40.6 s of `.fli` presentation against a 6.6 s original window), measured it, and the reading was wrong.

## 1. Original side: window start/finish from the capture's own timestamps

| window | frames | starts | ends | span | mode |
|---|---|---|---|---|---|
| `boot_` | 24 | 1.30 s | 5.05 s | 3.75 s | 720x400 (text) |
| `fli` | 54 | 7.73 s | 15.62 s | 7.89 s | 320x200 |
| **nothing captured** | 0 | **15.62 s** | **48.40 s** | **32.8 s** | unobserved |
| `title_melt` | 42 | 48.40 s | 55.29 s | 6.88 s | 640x480 |
| `title_card` | 1 | 55.31 s | 55.31 s | -- | 640x480 |
| `slides` | 133 | 55.60 s | 77.53 s | 21.93 s | 640x480 |
| `gameplay` | 55 | 103.60 s | 111.53 s | 7.93 s | 320x200 |
| `esc_win_` | 41 | 111.73 s | 117.64 s | 5.92 s | 320x200 |

The capture is **event-triggered, not continuous**: 434 frames across 130 s, with a 32.8 s hole between the end of the
`.fli` window and the start of the title sequence.  So the original side can say when a phase was *seen*, not what the
game was doing for those 33 seconds (the movie still playing at 320x200, or the holds between it and the title -- the
capture cannot tell, and nothing here should be read as if it could).

## 2. Port side: the same phases, from `res.jsonl` anchors (`route_mirror_episode1.txt`, ticks at 490 Hz)

| lookup | tick | wall time |
|---|---|---|
| `logo.pcx` | 4 | 0.01 s |
| `opening.fli` | 1239 | 2.53 s |
| `title1.pcx` | 21111 | 43.06 s |
| `1s1.pcx` | 21967 | 44.83 s |
| `1s2.pcx` | 23416 | 47.79 s |
| `1s3.pcx` | 25385 | 51.81 s |
| `stage1.pcx` | 27353 | 55.82 s |

## 3. The comparison, and what it says

- **Time to title: 48.40 s original, 43.06 s port -- 11 % apart.**  Not six times apart.  The whole 40 s of `.fli`
  presentation the port spends is what `src/f` says it should be: `f.c:16783` describes the attract chain as "two
  0x400-iteration `delay_units(1)` holds (0x38a80-a9e, 0x38ab3-ad5) plus a 900-frame .fli", and the shared-code SDL
  host measures the same shape -- `logo.pcx` at 0.16 s, `opening.fli` at 7.33 s, title pages at 39.5 s.  The Pocket
  shim (2.53 s / 43.06 s) tracks that, so `.fli` pacing is shared behaviour, not a Pocket-path defect.
- **The offset is nearly constant: 5.2 s at the movie (7.73 vs 2.53), 5.3 s at the title (48.40 vs 43.06).**  A constant
  offset through a run is the signature of a *skipped phase*, and the skipped phase is the one the port has never
  claimed: the DOS text-mode boot at 720x400 (docs/382), which the original occupies from 1.30 to ~7.7 s and the port
  does not present at all.
- **Downstream, the two sides are not comparable as a clock.**  Every card in this game holds until a key arrives, so
  slides and stage entry measure the *driver's dwell schedule*: the capture pressed on its own cadence,
  `route_mirror_episode1.txt` presses once per card (docs/387).  The port reaches `stage1.pcx` at 55.8 s while the
  capture's gameplay window starts at 103.6 s -- that difference is which keys were pressed when, and no statement
  about the port's speed follows from it.
- **Original-side labels are file-system sightings, not game lookups.**  `1s3` is first seen at 1.30 s, during the
  text-mode boot, before `1s1` at 3.69 s: the DOS layer touched those files during directory enumeration.  Anchoring
  the original side on that column would produce a nonsense ordering, which is why §1 uses window boundaries instead.

## 4. What this adds to the record

A pacing claim that survives measurement: **the port reaches the title sequence 11 % earlier than the original, by an
offset explained by the text-mode boot it does not present, and its `.fli`/hold pacing is the shared `src/f`
behaviour the SDL host also shows.**  And a limit stated where it matters: the original capture goes unobserved for
32.8 s in the middle of the intro, so any claim about what the original did in that hole is out of reach of this rig.

## 5. The `.fli` content itself, measured in the rig's own grid

The picture comparison in docs/388 hesitated on the `.fli` window because the two sides disagree on *geometry* (8
frames at different shapes).  Both captures already share the rig's **40x30 luminance grid** (`frames.bin`, 1200 cells
per frame), which is geometry-free, so the intro's content can be tested directly: pair each distinct original `.fli`
frame against the 676 distinct pictures the port presented between the `opening.fli` and `title1.pcx` lookups.

| measure | value |
|---|---|
| distinct original `.fli` pictures | 25 (from 54 captured frames) |
| distinct pictures the port presented in its `.fli` segment | 676 (from 2207 frames) |
| grid NCC of the best port counterpart | **mean 0.9981, min 0.9835** |
| matched at grid NCC >= 0.90 | **24 / 25** |
| grid mean absolute difference | **0.64 mean, 3.21 max** on a 0-255 luminance scale |

So every distinct frame of the original's intro movie except one has a port counterpart that is the same picture to
within about half a luminance step, and the port shows 27x more distinct frames than the capture sampled -- the capture
sampled 54 frames across 7.9 s of a 900-frame movie, so the extra distinct frames are the movie's own animation that
the original side simply never recorded, not extra content.

The one unmatched frame is the pure-black one (mean luminance 0.0): its grid has zero variance, so NCC is undefined
there and it is counted as blind rather than matched -- the same rule docs/388 2 established.
