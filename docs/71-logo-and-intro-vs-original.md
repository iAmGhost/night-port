# 71. Logo card and opening.fli, measured against the original's own frames

Closes the `logo-intro` playtest task: the company logo at launch and the intro video, now compared with frames
captured from the original game (not only with shipped-asset decodes). Tools added:
`scripts/playtest_fli_index_match.py` (logo fade-envelope + .fli frame-index matching) and the channel-order fix in
`scripts/playtest_orig_drive.py` / raw capture through `scripts/playtest_orig_capture_api.py` (docs/70).

## Capture basis

Clean boot of the original in dosbox-automation (`docker restart dbga`, then a fresh `dosbox-automation -conf
/work/night.conf`), raw payloads at 6 Hz for 60 s into `/tmp/dbgauto/orig_intro_raw` (344 frames). Shape timeline of
the original (payload width x height, from the payload header, not the reported DOS mode):

    720x400    53 ..  1632      DOS text
    320x200  1744 ..  2542      mode 13h, black
    640x480  2690 ..  6991      company logo card (fade in, hold, fade out)
    320x200  7085 .. 48210      opening.fli intro video
    640x480 48044 .. 77104      title card + melt

The port run used for the pairs is `/tmp/nightport/st1fix` (`NIGHT_DENSE=1`, `NIGHT_DENSEHZ=20`, 1670 shots); its
timeline: 640x480 to 2613 (logo), 320x200 2625..43293 (.fli, 903 shots), 640x480 43301..51701 (title + slides).
So the intro window is **40.7 s on the original and 40.7 s in the port** - the earlier suspicion that the original
cut the intro short after ~8 s was a capture dropout in `playtest_orig_drive.py` (60 frames in 33 s), not a game
behaviour; the dedicated raw capture at a steady 6 Hz removed it.

## A. Company logo

Compared with an independent PIL decode of the shipped `logo.pcx` card (640x480) using a single least-squares
brightness gain per frame (the game fades whole screens through the DAC, so a capture differs from the asset by that
gain).

Original, logo era, one row per captured frame:

    t=4014 NCC 0.9944 gain 0.122 | 4189 0.365 | 4365 0.629 | 4542 0.850 | 4719..5597 0.970 (6 frames plateau)
    t=5769 0.869 | 5943 0.629 | 6120 0.365 | 6297 0.122 | black from 6469

Port, logo era:

    t=206 gain 0.239 | 406 0.521 | 606 0.784 | 806..1206 0.962 (plateau) | 1406 0.861 | 1606 0.584 | 1807 0.320 | 2006 0.044

Findings:
- **The logo card is shown at launch on both sides, and it is `logo.pcx`.** Every non-black frame on both sides fits
  the asset with NCC 0.994-0.9999 and a monotonically rising-then-falling brightness envelope; the fade ladder
  shape (5-6 steps, symmetric in/out) is the same on both sides.
- The port is *closer* to the asset than the original capture is: port brightest frame MAE 2.38 (gain 0.962,
  within +-8 on 100 % of pixels), original brightest frame MAE 9.30 (gain 0.970, within +-8 on 51 %). The
  residual grows with brightness on the original side, which is a capture/emulator output characteristic (the
  original never reaches gain 1.0 and carries a contrast error the gain fit cannot remove), not a port difference.
- **One real difference: the hold.** The original holds the plateau for 6 captured frames (4719-5597 ms, 878 ms);
  the port for 4 shots spanning 400 ms. Visible-card span: original 2.28 s, port 1.80 s. Caveat stated honestly:
  the port's shots in this window are 200 ms apart (`NIGHT_DENSEHZ` did not engage for the first ~15 shots), so the
  plateau length is quantised to 200 ms; the ~0.5 s gap is well beyond that quantisation, so the shortfall is real
  and is filed as item 19 in the playtest log for the `title-pacing` task (same hold/fade machinery).

## B. opening.fli intro video

Reference: `ffmpeg -fps_mode passthrough` decode of the shipped `scratch/res/opening.fli` -> 1169 PNGs (the file's
own header declares 320 macroframes; ffmpeg resamples to 35 fps, spacing 0.028571 s, last pts 33.37 s, so the
content duration is 33.4 s - and both the port and the original occupy a 40.7 s window including the fades, which is
consistent).

Method: each captured frame is matched to a .fli frame index by mean-normalised 80x50 thumbnail distance
(mean-normalising is what makes a faded capture match the same frame), 511 distinct reference thumbnails after
collapsing identical ones; a pair is used only when both sides match confidently (distance < 0.10 and a > 0.01 margin
to the runner-up), which is what kept the first attempt's bogus pairs out.

    original: 200 sampled frames, 154 confident matches (77 %), median thumbnail distance 0.011,
              index range 127..1111, playback rate 28.65 .fli frames/s
    port:     763 sampled frames,  585 confident matches (77 %), median thumbnail distance 0.012,
              index range 124..1133, playback rate 28.56 .fli frames/s

    pairs at identical .fli index: n=277
      within +-3 on all channels: median 1.0000 ; >= 0.99 in 170 pairs ; within +-8 median 1.0000
      NCC median 1.0000 ; best pairs MAE 0.06-0.08 (e.g. idx 136, 142, 181, 188, 223)

Findings:
- **No artifact anywhere in the port's .fli playback**: its frames sit a median 0.012 thumbnail distance from the
  ffmpeg reference, the same as the original's 0.011 - the shear that used to be there (fixed by the
  `g_view_320x200` bracketing around `fn_03f6bc`, log item 2) is gone and stayed gone.
- **The two sides show the same frame at the same .fli index**: 277 index-matched pairs, median 100 % of pixels
  within +-3, NCC 1.0000, and 170 pairs with >= 99 % of pixels within +-3 (MAE 0.06-0.08 on the best ones). Side by
  side: `docs/manual-playtest/08-shots/fli_idxNNNN_port_vs_original.png` (port left, original right).
- **Playback rate matches**: 28.65 (original) vs 28.56 (port) .fli frames/s -> ratio 0.997. This is the first
  independent pacing measurement that does not rely on the port's own clock.
- Why 277 pairs and not more: the original is sampled at 6 Hz against a ~35 fps video, so only every ~6th frame is
  available, and frames inside a static stretch are rejected by the confidence margin (an index estimate with no
  margin is not evidence). 154 of 200 original frames were usable.

## What this does not claim

- The logo hold difference (0.5 s) is measured but not yet fixed or root-caused.
- The .fli comparison covers indices 124..1133; the very first and last ~120 ffmpeg samples are dominated by
  static/fade frames that the confidence filter rejects.
- The original-side logo residual (MAE 9.3 at plateau) is not explained; it is one-sided (the port matches the
  asset better), so it is attributed to the emulator/capture output path, which docs/70 already showed to be
  mode-dependent in its channel order.
