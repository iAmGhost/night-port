# 381 — the Pocket host's pictures scored against the original's frames: 222 of 434 at NCC ≥ 0.90

docs/380 §5 said what was missing: label-decodable images from the Pocket shim, so docs/91's pairing basis could be
applied to the Pocket code path instead of only to the SDL host.  It exists now, and the number is in.

## 1. The shim already had everything needed

`NIGHT_VIDRAW=1` makes the rig dump write 120 arena witness raws spread over the run (`main_of.c:491`): `vw*vh` index
bytes, a `#base_delta=…#ents=N#pal` marker, then **768 palette bytes taken from the game's own DAC write**.  Those two
halves are a paletted image, so `scripts/pocket/arena_raws_to_bmp.py` is a decode rather than an approximation --
index → RGB through the game's palette, out as 24-bit BMP, which is the format
`scripts/playtest_compare_orig_port.py` already reads.

## 2. Two defects in my BMP writer, both found by the comparison itself

- **data offset**: the pixel array starts after the 14-byte file header plus the 40-byte DIB.  I wrote
  `off = 54 + 40`, so PIL read 40 bytes late: `OSError: image file is truncated (1880 bytes not processed)`.
- **row order**: a BMP with positive height is **bottom-up**.  Emitting rows top-down handed the comparator a
  vertically mirrored picture, and every frame scored near 0.1 -- against the very same raws that `sweep_report.py`
  matches *byte-for-byte* against shipped PCX cards.  That contradiction is what caught it: 0/434 → **222/434** from
  the row order alone.

## 3. The run and the score

Pocket host: `--frames 12000 --script scripts/playtest/route_pause_menu.txt` (anchored, docs/380), `NIGHT_VIDRAW=1`,
seed 1234, 120 arena witnesses.  Original side: the 434 labelled frames of docs/379's container capture.

```
$ uv run --with numpy --with pillow python3 scripts/playtest_compare_orig_port.py \
      --port /tmp/port75bmp --orig /tmp/orig71
original frames with a port counterpart at NCC>=0.90: 222/434
original event windows: boot(24) down_win(21) esc_win(41) esc_win2(41) fli(54) gameplay(55)
                        left_win(21) slides(133) stage_play(1) title_card(1) title_melt(42)
```

For reference, docs/91 measured the SDL host the same way at **348 of 517** original frames at NCC ≥ 0.90, with the
`.fli` pairs pixel-exact.  This port's picture is shared `src/f` output, and the Pocket host now has its own number on
that basis rather than inheriting the SDL host's.

## 4. What the number does not cover

- **120 witnessed frames out of 12 000.**  The witness cap is deliberate (a nine-episode sweep stays in tens of MB);
  coverage is spread across the run by a stride derived from the frame limit (docs/373), but a 12 000-frame route is
  sampled every 100 frames.  Scenes shorter than the stride can be missed entirely -- `stage_play(1)` and
  `title_card(1)` in the original's window list are exactly that thin.
- **Best-match pairing, not moment-matching.**  This comparator asks "does some Pocket frame look like this original
  frame" (docs/91's basis); `diff_screen.py`'s 0.559 asks "do the two runs pass through the same sequence in the same
  order" (docs/56's alignment).  Both are reported here because they answer different questions, and neither has been
  made to agree with the other yet.
- **Mode-mismatch pairs are flagged, not hidden**: rows marked `320x200 vs 720x400 MISMATCH` are the original being in
  a different video mode at the matched moment, including its 15 text-mode frames.

## 5. State

Harness-only work: card unchanged at 0.1.67, `src/f` untouched, fixture suite `== ALL PASS ==`.  The pieces are now in
place for a stronger run -- raise the witness cap, anchor both sides, and score the same route on both bases.

## 6. Coverage was not the limit (measured, and it changed almost nothing)

The cap is now a setting (`NIGHT_VIDRAW_MAX`, default 120, ceiling 1000) because it was a literal `120` in three
places and therefore unraiseable.  Same route, same seed, same original capture, only the witness count changed:

| witnesses | stride | dump | originals matched at NCC ≥ 0.90 |
|---|---|---|---|
| 120 | 100 frames | 27 MB | 222 / 434 |
| **400** | 30 frames | 89 MB | **226 / 434** |

Three more matches out of 434 for 3.3× the witnesses.  So the residual in §3 is **not** a sampling hole -- a scene
missed for want of a witness would have shown up as a jump when the stride dropped from 100 frames to 30, and it did
not.  The gap is in what the two sides show at all: the original capture is a live playthrough of
`drive_orig_episode1.txt` (with the emulator's own waits and its 15 text-mode frames), and the Pocket run is
`route_pause_menu.txt` on this host, so the 208 unmatched originals are frames from phases that run, or are entered,
differently between the two -- the same conclusion docs/380 4 reached from the other basis, at the same value.

The honest summary of the picture basis today: the Pocket code path matches the original on **half** the frames of a
54-second route at NCC ≥ 0.90, its stage cards and intermission cards are byte-exact against the shipped assets
(docs/363, docs/373), and the reason the original-frame score is not higher is a route/phase difference that has not
yet been pinned down.  Narrowing it needs the labelled pairing to be filtered window by window (`--window slides`,
`--window gameplay`) rather than scored as one route, which is the next measurement, not this one.

> **Correction (docs/384).**  The 0.5590 quoted above is `diff_screen.py`'s *floor*, not a fidelity score: its lag
> window is ±90 port frames (~1.6 s), the two runs reach the same phases tens of seconds apart, and the DP's mean then
> collapses to a brightness term -- a port stream with every grid zeroed scores 0.559023 against this same 0.559047.
> With `--win 4000` the same tool reports 0.8056 (warp + ESC), 0.7786 (warp), 0.5590 (attract/title route, i.e. pure
> floor) against a floor of 0.5590.  Statements here that compare *content* rest on the label/best-match basis
> (docs/381-383), which is unaffected.
