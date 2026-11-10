# 409 — The music pause was one assignment that moved the writer back on top of the voice

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


Core **0.1.90**, card md5 `534400b7fec53f03d79dc18f536eb732`.

The symptom, at last stated precisely: **the pause happens once per track start — logo music, title
screen music, episode-introduction music.** Three places, three track requests. And before it was
located, "plays 1–2 seconds, then pause a while, then continues."

## 1. What 0.1.89's probe measured

```
first sounding sample:  live=6754   w = 10        e_live=850    e_wtail=0     starts=1
next sample:            live=49031  w = 44109     e_wtail=10819 laps=1
steady:                 behind≈170 500   vroom≈500–1700 frames   e_live=0 on ~40% of samples
                        dpos≈44 150 frames per ~1 001 300 us     starts=2/3, stops=1/2
```

Read line by line:

- **`dpos ≈ 44 150 frames / 1 001 342 µs` → the mixer consumes at exactly 44 100 frames/s.** The rate
  hypothesis (the 48 kHz default Doom warns of, 1.088×) is dead. The mixer was doing the right thing
  at the right speed all along. (`rate=0.000x` on the card was a display bug of mine — the scale was
  off by 1000, so the factor printed as its own integer part. Fixed; the raw `dpos`/`dt` in the same
  line is what carried the reading.)
- **`e_wtail=0` on the first sounding sample, with the write cursor `w = 10`.** The voice was already
  playing, and nothing had been written behind the writer. The prefill had put 150 sectors (88 200
  frames, 2.0 s) into the ring moments earlier.
- **`behind ≈ 170 500` of a 176 400-frame ring, `vroom ≈ 500–1700` frames = 0.01–0.04 s.** The mixer's
  view says: the writer is 0.02 s behind the voice, not 2 s ahead of it. `allowed` is clamped to
  `vroom / 588` = **1–2 sectors per read** — which is the "1.6 sectors/op" that docs/406 §3 built a
  theory on. That throttle was a symptom.
- **`e_live=0` on ~40 % of samples while `e_wtail` is loud.** The voice is reading *silence* while
  real audio exists elsewhere in the ring. This is the audible pause, caught.
- `silent=0ms` — because the voice never *stops*; it runs over zeros. A looping voice is always
  "sounding". The probe still needed that reading to make the distinction.

## 2. The line

```c
uint32_t got_frames = fill_sectors(prefill_sectors);   /* writes 88 200 frames, g_write -> 88 200 */
...
g_write = 0;                    /* the voice starts at the first prefilled frame */
g_consumed_frames = 0;
voice_start();
```

The comment is true and the code is wrong. The voice does start at ring index 0 — but zeroing
`g_write` did not move the voice, it moved the **writer** back to index 0, where the voice was
already reading, and left frames `[88 200, 176 400)` as the zeros `ring_silence()` had just put
there. So:

- the writer chased the voice from *behind* at the real-time rate (75 sectors/s) instead of leading
  it by the 2 s the prefill had bought;
- the voice walked off the end of the written region into those zeros — **silence, for as long as it
  takes a 4 s ring to lap**;
- then it wrapped into the band the chaser had since written — **"and continues to play"**;
- the cycle repeats once per ring lap, and re-arms at every track start, which is exactly the three
  places the user named.

`g_done_frames` was *not* reset, so the model believed in a ring the hardware had not filled. That
is the arithmetic docs/408 §2 measured and could not name:

```
of_cdda_head()  170 500 frames   (mixer)
ring_ahead()     83 400 frames   (model)
difference        87 100 frames   = the 150-sector prefill, 88 200 frames
```

The origin error I had measured, quantified, and written down one build earlier was this assignment.
The invariant it violates is one line wide and was staring at me: `ring_write()` advances `g_write`,
`fill_sectors()` advances `g_done_frames` — **both counters are "frames written" and must be equal**.
Every other `g_write = 0` in the file is paired with `g_done_frames = 0` (audited: lines 725, 746,
804, 950, 968). Line 810 was the only unpaired one, and it was the only one reached on the normal
track-start path.

The fix is to delete the assignment. With it: the writer's frontier stays at 88 200, the voice starts
at 0, `behind` = 88 200 = a real 2 s of slack, `vroom` = 83 790 frames = 142 sectors, and a 27-sector
read is finally possible for the reason docs/407 wanted and could not get.

## 3. Why no previous build could have seen this

Three of my own instruments were structurally incapable of it, which is worth recording:

1. `clock_consume()` clamps `want` to `g_done_frames`, so `dry` can never report the voice running
   ahead of the model (docs/408 §2a). It read 0 for five builds while the player heard silence.
2. `room_hold` counts refusals in the *pump*; the throttle that mattered was the `allowed = room/588`
   clamp in the fill, which is silent.
3. `audio_low` asks `ring_ahead()`, i.e. the model that the bug had desynchronised.

Every one of them asked the model. That is the reason 0.1.89 existed as an instrument-only build,
and it is what found this in one boot.

## 4. What 0.1.90 changes, and what it does not

- **The fix**: `g_write` is not reset after the prefill.
- **Probe honesty**: `rate` now carries three decimals (`1.000x`, not `0.000x`), and the line adds
  `ahead` — the frames from the write frontier *forward* to the voice. `behind` near `RING` is not
  "plenty buffered", it is "the voice is about to read audio nobody wrote", and reading that number as
  a depth is the mistake this port made for several builds. 18 specifiers, 18 arguments, counted
  (docs/404).
- **Still open, deliberately**: `night.ini DISC_MIN_WANT=27` is silently clamped to 12 by
  `CD_MIN_WANT_MAX`. The boot line prints the effective value (`cdda policy: min_want=12`), so the
  log is honest even though the ini knob lies. Raising the clamp changes the shim's fill sizes and
  therefore may move the rig basis, so it is not bundled with this fix — one variable per boot.

Verification: `pocket/src/night` `make test` builds both halves; the rig basis is unchanged
(`cdda.raw` = `ad81a3e0f5567c210c358123accfe2e9` at 1 200 frames and `712b6c128a2f631a4dc1ad023f8c8cb6`
at 2 400, `pcm_cd.wav` `c0a447ea`/`6455ae7f`, all identical to the pre-fix values) — which is the
expected result, because the rig witnesses *sector reads* and this change moves only the writer's
position inside the ring, not the sector sequence; repo `make test` = `== ALL PASS ==`; `src/f`
unmodified. The nine-episode sweep was not re-run: it scores arena/video cards from `src/f`, and
nothing in this change reaches that path — the audio byte stream, which is what it could touch, is
md5-identical.

## 5. What the next boot should show

1. `e_live` **never 0** while `sounding=1` — the direct test of whether the pause is gone.
2. `behind` settling near **88 200** (2 s) and `ahead` near **88 200**, both far from the edges,
   instead of 170 500 / 5 900.
3. `vroom` in the tens of thousands of frames instead of 500–1700, and `sectors/op` finally able to
   reach 12 (and 27 once the clamp is lifted).
4. `laps` no longer climbing once a second.
5. The three named moments — logo, title, episode introduction — with music running through them.

If `e_live=0` survives with `behind` healthy, then a second silence source exists and the next thing
to look at is `ring_silence()` coverage on partial prefills (the code zeroes the whole ring and then
fills a slice, so a slice shorter than the ring's depth is still silence the voice can reach).
