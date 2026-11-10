# 414 — the container route runs at 1.000× real time, so `cycles=max` runs never carried durations at all

docs/413 left the one number that decides its open question unmeasured: how long the original sits on the stage
card. The port holds ~3.3 s on the desktop and ~8.0 s on the Pocket for the same `0xc00 delay_units(1)` loop, and
only the original can say which is right. Going to get it produced a route that answers nothing, plus one fact
that matters more than the null result.

## The emulation speed was an assumption until it was measured

`[cpu] cycles=auto` in the container settles at `current_max=3000` with `auto_adjust: false`
(`/api/v1/cpu/cycles`) — which sounds static and alarming, and is exactly the kind of thing to measure instead of
reasoning about. So measure it against the clock the emulator itself maintains: type `TIME` at the DOS prompt
(DOS prints hundredths of a second), read the text screen back with `/api/v1/video/text`, wait a known host
interval, repeat.

```
calibration: DOS advanced 20.00 s over 20.00 s host -> realtime factor 1.000
```

Two consequences:

- **`cycles=auto` is real time here**, so this route can carry durations. Every duration read off a
  `cycles=max` capture is not a duration at all — which includes the runs behind docs/377, whose own drive file
  (`scripts/playtest/drive_orig_episode1.txt`) already says as much in its header ("*cycles=max boot timing is
  not*"). Any earlier number of that shape should be read as frame counts, not seconds.
- The hold-length question is answerable with this route after all. It just needs a route that reaches the card.

## The route result: null, and why

The run went boot → attract → 640x480 show, and then:

- **7.4 s of pixel-identical 640×480 frames** (mean 193.5, std 49.5, unchanged across 45 consecutive samples).
- Nothing changed for another ~26 s, then the mode went 320x200.
- The still frame is **slide `1s1`**, not the card: matching the captured frames against the shipped PCXes
  (`scratch/res/*.pcx` decoded by `sweep_report.pcx_indices`, 48×48 luminance-grid NCC) gives `1s1=0.33`,
  `1s3=0.09`, `1s2=-0.13` — `stage1` is not even in the top three.
- The drive then reported `stage_play` and `gameplay`, which proves nothing: `wait_mode 200` cannot tell stage
  gameplay from the attract menu. That is docs/410's geometry-anchor trap, in the original's own route.

Cause is in the drive file's own header — *an extra Enter skips the slide show* — and the route presses Enter
four times. So to time the card the route must press **once** to start the episode and then **never press
again**, and must identify the card by **picture**, not height, because title, slides and card are all
640x480 (mode 101h) and only gameplay is 320x200.

Better card reference than my gray-index proxy: the port's own card dump. docs/363 shows the port holds its
stage cards byte-exactly, so a `--shots` dump from the desktop port gives a same-index picture to correlate
against — an apples-to-apples basis instead of index-as-gray.

## Transport traps, because this setup costs an hour every time it is forgotten

| trap | symptom | route |
|---|---|---|
| host-side `curl` to `:8386` | empty response, no error | everything goes through `docker exec dbga curl` (`capture_container.py:6` knew) |
| `playtest_orig_drive_container.py` default `--name` | `container 03ebbe93… is not running` — a container that died days ago | pass `--name dbga` |
| two drive dialects | `unknown drive step: record boot 5000` | `record/wait_mode/wait_mode_soft/shot` belong to `scripts/playtest_orig_drive.py`, not `capture_original.py` |
| numpy missing | `ModuleNotFoundError` mid-run, after the game already launched | `uv run --with numpy --with pillow python3 …` |
| no `pkill` in the image, and a pid-scan `kill -9` | the **container itself** died and needed `docker start dbga` | use the API's shutdown (`/api/v1/status` carries `shutdown_requested`) or `docker restart dbga`; never pid-scan-kill from the host |

Two of these wasted a launched game each: the emulator boots straight into `NIGHT` now (`night.conf` autoexec),
so a driver that fails on import or on the wrong container name consumes the one clean boot that route had.

## Second attempt: the pre-stage flow, measured, and why the card window is never sampled

A no-press run (`scripts/rig/orig_card_hold2.py`, factor 1.000, `scratch/pocket/orig_hold2`) shows what the game
does when nobody touches it: it alternates, forever --

```
320x200  0.0 .. 40.5 s      attract
640x480  40.5 .. 91.4 s      <- frame 0089 is the title/copyright card ("1997 TG.ENTERTAINMENT /
                               ORIGINAL COMIC BY MYUNG JIN LEE"); frame 0099 is black (the fade out)
320x200  91.4 s ..           attract again
```

and a second run (`orig_hold3`) repeated it exactly: attract 27 s / title 49 s / attract 40 s / title 50 s. So
the pre-stage loop is 320x200 attract then a ~50 s 640x480 title sequence, with no episode in sight. Pressing
Enter *once* at the first 320x200 after a title (the obvious moment) did not start an episode: the loop carried
on unchanged. The pre-stage flow is therefore not one press, and `drive_orig_episode1.txt`'s four Enters are
doing something real.

That also explains why the card window has no samples in any run so far, which is the actual blocker:

- `playtest_orig_drive.py` samples only inside its `record` windows, so between its last `slides` shot (t=42.9 s,
  the `1s1` slide, identical for 7.4 s) and its next shot at t=69.1 s -- while sitting in `wait_mode_soft` /
  `wait_mode` -- it saved nothing. The card, if it appeared anywhere in those 26 s, is unsampled.
- My own sampler covers every 200 ms but never gets that far, because it cannot get an episode started.

And one observation from that unsampled boundary that is worth its own follow-up, unrelated to timing: frame
`0252_stage_play.png` at t=69.114 s is **640x480**, and it is not the card -- it shows the station scene with
both HP bars in the upper corners, very dark (mid-fade), 0.17 s before the mode reads 320x200 (`0253` at
69.275 s). The port's mode model says the switch to 320x200 is the `INT 10h AX=0013` right after the hold. A
640x480 picture carrying *gameplay HUD* just before it is not explained by that, and `analysis/refs/
st1_startup_640_noisy.png` suggests a real 640x480 start rather than a capture artifact. Unproven, but the kind
of thing that should not sit in a scratch directory.

## The measurement that is still owed

One route design, one run: `NIGHT` from autoexec → single Enter on the title → no further keys → capture at 6 Hz
→ find the frame that correlates with the port's card dump → the hold is that frame's static run, bounded by the
fade-in ramp (`fn_040660(50)`) and the fade-out (`fn_0406f5(50)`), and at factor 1.000 the `t_ms` column is in
real seconds. Then compare with the desktop's 3.29 s and the Pocket's 7.95 s and the `delay_units` cost per read
is a number instead of an argument.

`scripts/rig/orig_card_hold.py` is in the tree from this attempt; it does the calibration and the mode-watch, and
its press-on-long-static-title logic is wrong for exactly the reason above — it fires on the text-mode shell and
then skips the show. Keep it for the calibration half.
