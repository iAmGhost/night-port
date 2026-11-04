# Port vs original: phase timings, measured on both sides

This is the first document that compares the port and the ORIGINAL on the same axis: moments captured from the
original through the dosbox-automation REST capture path (docs/46 route 8, AGENTS.md recipe) against the port's
named-event shots from the same scripted route.

## Tooling added this cycle

| file | role |
|---|---|
| `scripts/playtest_orig_drive.py` | drives the original: polls frames, classifies them, waits on *geometry* (`wait_mode 480/200`), injects keys through `/api/v1/input/sequence`, saves `NNNN_<event>.png` + `events.tsv` (label, similarity, emulated mode, time) |
| `scripts/playtest_orig_capture_api.py` | plain paced capture of the original (`--every`, `--hz`) |
| `scripts/playtest_compare_orig_port.py` | pairs port shots with original frames and reports per-pair NCC, the strictest pixel agreement, MAE and geometry |
| `scripts/playtest/drive_orig_episode1.txt` | the original's drive file (logo → .fli → title melt → slides → gameplay → pause probing) |
| `scripts/playtest/drive_orig_pacing.txt`, `drive_orig_play.txt`, `drive_orig_keyprobe.txt` | narrower probes |
| `scripts/playtest/route_slide_pacing.txt` | port route that samples the logo fade envelope and the slide edges |
| `scripts/playtest/dosbox_key_names.txt` | the emulator's accepted key names (extracted from its binary: `KBD_esc`, not `KBD_escape`) |

Gotchas that cost time and are now pinned down:
- one emulator instance per capture: the old instance kept the API port, the new one logged `failed to bind to
  0.0.0.0:8386 ... the API is off`, so polling silently read a **stale, idle** machine;
- the container image has no `pkill`/`pgrep`, so instances are reaped by scanning `/proc/*/exe`;
- `wait_mode 200` for the .fli and `wait_mode 480` for cards is a far steadier anchor than image matching,
  because animation frames never match a single reference well;
- an extra Enter during the slide show *skips* it (the `fn_00035dfc() == 1` -> `goto hold` branch at 0x03b0f1),
  which is what cut the original's slide show short in the first run;
- `/api/v1/input/sequence` answers 409 when a previously injected sequence is still in flight.

## The two clocks, and which one to compare

The port's shot manifest carries `t_ms` derived from **pump frames** (~18 ms of modelled game time each), which
does not advance while the game burns time inside a single main-loop step. The original's `events.tsv` times are
**wall** time. Comparing them directly is wrong; wall time is the common basis, taken from the port's shot file
mtimes.

Measured in wall time for the port, with the DOS-clock-read cost set to 0 (the old behaviour) and to the measured
500 us:

| phase | unit cost 0 us | unit cost 500 us | original |
|---|---|---|---|
| company logo screen: logo load -> first `opening.fli` lookup | **1.45 s** | **2.60 s** | ~2.5 s of fades+holds (below) |
| run length to reach the title | 42.0 s | 43.2 s | 46.6 s (with the attract demo) |

## The original's company-logo screen, measured

Captured at 6 Hz from boot (`cpu_cycles = 20000`), mean RGB per frame:

```
   7.74  30.73  56.20  78.94 | 92.18 92.18 92.18 92.18 92.18 92.18 | 84.78 61.54 38.39 15.09 | 0.00
   fade-in (5 steps ~0.83 s)      plateau at full brightness ~1.0 s        fade-out ~0.83 s      black
```

Structure and content match the port exactly: pairing the port's logo card with the original's logo frames gives
NCC 0.997-0.998, and after fitting a single scalar gain the mean channel error falls to 3.6-4.5 (best gain
0.171 -> MAE 1.65 for a frame the original had faded to 17 % brightness). So the card is the same image with the
same palette; the residual is the emulator's capture of a frame that was mid-fade.

The port reproduced the same envelope once the clock-read cost was modelled: 50+51 retrace-paced fade frames
(1.45 s) plus two 0x400-iteration `delay_units(1)` holds (2 x 1024 x 0.5 ms = 1.02 s) = 2.47 s, against the
original's ~2.5 s.

## `delay_units` was free: the defect and its measured basis

`delay_units` (0x0004564f) converts its argument into a number of DOS clock reads
(`(arg*ticks + 500) / 1000`) and then performs that many `INT 21h/2Ch` reads. The port's stand-in counted them and
returned, so every hold built out of `delay_units` -- the logo screen's two idle loops (fn_038a20), the ending
slide show's 12 000-iteration hold, the stage card's 0xc00-iteration hold, and the key-repeat gates -- ran in
essentially zero time.

Two independent original-side measurements fix the cost at ~0.5 ms per read:

- the logo screen: 2 x 1024 reads account for the ~1.05 s of hold between the measured fade pair;
- the ending/stage card hold is 12 000 reads, and the original's per-card hold measured ~6 s.

The port now paces each read by `g_delay_unit_us` (default 500 us, override `NIGHT_DELAY_US`, 0 disables). The
wait runs `sys_port_spin()` so the cooperative tick/input pumps keep running, and with no pump installed -- every
unit test -- it stays a no-op, which is why `make test` (2873 checks) and `make selftest` still
pass unchanged.

The cost proved insensitive to the emulated CPU speed: the logo fade curve measured at `cycles = max` and at
`cpu_cycles = 20000` is the same to within a sample, which is what makes it a usable calibration rather than an
artifact of one emulator setting.

## Still open after this cycle

- the per-card slide hold is **not yet pinned**: in the original run the show was cut short by the drive's own
  Enter press (the skip branch), so the original only showed one card; the port's `wait res` anchor spacing (4.0 s
  per card) is an artifact of the route's schedule, not a measurement. Both sides need a clean, key-free slide
  window.
- the title melt has 28 original frames (`title_melt_01..28`, 46.59-53.58 s) but the port's card shots in that
  window pair at NCC 0.07, i.e. the phases have not been lined up yet.
- the pause menu now has an original frame (pink panel: LEVEL UP MODE / COMMAND LIST / BGM MODE / SOUND MODE /
  EXIT TO DOS) against the port's `pause_m0/m4/m3`; the whole-frame NCC is 0.87 because the underlying gameplay
  differs, so the menu panel region has to be compared separately.
  **[re-scored, docs/79]** that 0.87 was measured against a channel-mirrored 320x200 capture; the corrected-basis
  number is **86.18 % of the 64,000 pixels within +-3, NCC 0.860, MAE 9.04** with the residual confined to the live
  gameplay backdrop behind the window (the window itself is byte-verified against `pause.spr`) -- docs/75.

## Clean slide window on the original: one more thing to learn, and the title card needs the press to land late

`scripts/playtest/drive_orig_slides.txt` presses Enter only while the picture still matches the title reference
(new drive step `key_until_not_label`, because one Enter pressed ~10 s into the card leaves the original sitting
on the title -- it later falls into the attract demo by itself -- while a press after the show has started skips
it). The title gave way after **two** presses.

What the clean window shows (154 frames of 640x480, 59.66-99.19 s): the slide content appears by 60.7 s and the
picture then stays unchanged for 38.5 s. The frame at 60.7 s is legible -- the episode-1 narration card ("내 이름은
남궁건, 17세…", a bus interior) -- but its mean RGB is 193, i.e. strongly blown out, and pairing it with the port's
`1s1/1s2/1s3/stage1` shots gives NCC ~0 with MAE 193. That is not the content mismatch it looks like: in the earlier
capture the original's slide frames paired with the port's slide-era cards at **NCC 0.996**, so the same artwork is
involved; the difference is the palette level the card is held at.

Open, and deliberately not papered over:
- the original's per-card cadence in the episode slide show (the drive's second Enter may still have skipped
  ahead, and one card held 38 s does not match any port-side constant);
- whether the original holds a slide at a near-white palette level by design, which would make the port's
  fade handling for these cards wrong in a way the index-level comparison in docs/58 cannot see.

## Title melt: the original's duration is now measured; the port's is not yet isolated

Original, from the captured frames in the `title_melt` window (35 frames of 640x480): content changes begin at
49.92 s and the last change is at 56.66 s, so **the melt animation spans 6.74 s** of real time, after which the
card is still.

The port side is not yet a number. Two measurement facts had to be discarded on the way:
- shot-file mtimes are useless on the port side -- the harness writes shots in bursts inside one game step, so 103
  shots came back with ~1 ms spacing;
- the manifest's `t_ms`/`frames` clock is the port's modelled clock (about 18 ms per pump frame), which is the
  right basis for the port but is not comparable with the original's wall clock without the `delay_units` pacing
  discussed above.

On the modelled clock the port's title-era 640x480 shots change continuously from the `title1.pcx` lookup until
the first >=0.8 s static run, which lands 44 s after the lookup -- but that static point is the attract menu after
the title, not the end of the melt, so the melt itself still needs a phase definition (first card frame -> first
static run *before* the attract menu's sprites are looked up). Until then no port/original melt ratio is claimed.
