# docs/309 — the skip-intro shortcut: what the attract chain costs a run, and what the shortcut keeps

Commit `0e79e44`. Request: a skip-intro function, so a debugging run does not spend its time on the intro.

## 1. What the intro costs, measured

`NIGHT_RESLOG=1` on a plain `ep1_kill3` boot (no warp flags), resource lookups in order with wall stamps:

| lookup | wall time from run start |
|---|---|
| `logo.pcx` (the company-logo card, `fn_038a20`) | **+0.16 s** |
| `opening.fli` (the opening movie, `fn_038ae0` → `fn_03f6bc`) | **+3.82 s** |
| `title1.pcx` (the title menu's pages, `fn_03a224`) | **+35.7 s** |
| `stage1.wal` (the first room's art) | **+39.1 s** |

so **~39 s of every run happened before the game was on screen**. The two steps are:

- `fn_038a20` (0x038a20): `fn_03f15f` (VESA mode 0x101, 640×480×8), palette clear, `fn_041e20` fill,
  `board_out` of `logo.pcx`, then a DAC fade pair (`fn_040660` → fade-out) bracketed by **two
  `delay_units(1)` holds of 0x400 iterations each** (0x38a80–a9e and 0x38ab3–ad5), then `fn_0406f5`
  (100 steps), `fn_00010dde`, `fn_10b1d(1)` (start CD track 1).
- `fn_038ae0` (0x038ae0): `fn_10b1d(1)` again, `fn_03f6bc("opening.fli")` — **900 frames, 1,224,852 bytes**
  (docs/71) — between two more `fn_040660`/`fn_0406f5` fades.

## 2. Where the shortcut hooks in

`fn_03af18`'s menu loop calls the pair at 0x03af6e and 0x03af73. Those two calls are now one call to
`fn_03af18_attract_steps()` (the same host-side extraction pattern the port already uses for the ending
slides, `fn_03af18_ending_slideshow`). `fn_038a20` and `fn_038ae0` themselves are unchanged.

```c
void fn_03af18_attract_steps(void)
{
    if (g_skip_intro) {
        fn_03f15f();  fn_10b1d(1);  fn_00010dde();  fn_03f15f();      /* the pair's bookkeeping only */
        g_intro_steps_skipped++;  return;
    }
    fn_038a20();   /* 0x03af6e */
    fn_038ae0();   /* 0x03af73 */
    g_intro_steps_run++;
}
```

Enabled by `--skip-intro` or `NIGHT_SKIP_INTRO=1` (any value but `0`). **Default off**: with the flag absent
the shipped's attract chain runs exactly as before.

## 3. What the shortcut keeps, and why those three

The shortcut runs the bookkeeping the two steps leave behind, not their presentation:

| kept | shipped origin | why it must survive |
|---|---|---|
| `fn_03f15f()` ×2 | 0x38a38 (opens `fn_038a20`) and 0x38b24 (closes `fn_038ae0`) | it fills `DAT_531f4` (row width 640<<10), `DAT_63e64` (pitch) and the mode cells the whole software renderer reads; drop it and the title sequence and every room composite run against unset geometry |
| `fn_10b1d(1)` | 0x38b02, inside `fn_038ae0` | it is the CD track/TOC read that **starts track 1**; the music-schedule gates (docs/232, docs/235, docs/307) read that schedule, so skipping it would move audio measurements |
| `fn_00010dde()` | 0x38b1f | the shipped runs it between the track start and the mode set; it stays in position |

What it drops is presentation only: `fn_041e20`'s fill, the `board_out` of the logo, the palette clear, the
four DAC fades (`fn_040660` ×2, `fn_0406f5` ×2), the two 0x400-iteration holds, and the 900 `.fli` frames.

## 4. Measured effect

Same binary, same drive (`ep1_kill3.txt`), same pinned env (`NIGHT_SEED=13579`,
`NIGHT_RIG_STEP_CALLS=665`, `NIGHT_RIG_FPS=60`), same `--frames 4200`, `--rig-hz 5`:

| | flag off | flag on |
|---|---|---|
| `logo.pcx` requested | +0.16 s | **not requested** |
| `opening.fli` requested | +3.82 s | **not requested** |
| `stage1.wal` requested | +39.1 s | +17.6 s |
| first frame with `memstat stage=1 room=1` | **frame 2197** | **frame 981** |
| exit code | 0 | 0 |

**1216 frames and 21.6 s of wall time per run** saved, and the run still reaches the stage: the title menu,
the stage-1 art and gameplay all appear, `rc = 0`.

## 5. Witness and fixture

`f.c` exports four counters, and `test_f1.c` has `test_skip_intro_shortcut` (part of `make -C port test`,
`ALL PASS`):

- flag off by default — the shipped's attract chain is what runs;
- with the flag: one shortcut recorded, `g_intro_steps_run == 0` (neither `fn_038a20` nor `fn_038ae0` ran);
- `g_intro_skip_mode_sets == 2` — **both** mode sets the pair carries are still executed;
- `g_intro_skip_cd_track == 1` — `fn_10b1d` is still asked for **track 1**;
- with the flag cleared the same call runs `fn_038a20 + fn_038ae0` and records no shortcut.

`NIGHT_SKIP_INTRO=1` also prints one `skip-intro: ACTIVE ...` line at boot and one line when the
substitution happens, so a log always says which mode produced it.

## 6. Rules this shortcut creates

1. **A skipped run is a different measurement basis.** The `.fli` consumes logic steps too, so frame numbers
   shift with it: 2197 → 981 above. Never compare a frame-anchored number from a skipped run against one from
   an unskipped run — that is the docs/277 cadence rule applied to the intro.
2. **Never use it on an audio-schedule route.** Music measurements are wall-time statements (docs/302,
   docs/307), and this flag moves every wall stamp downstream of it.
3. **Any drive with a `wait res opening.fli` or `wait res logo.pcx` line stalls under it** — that resource is
   never requested, so the drive's anchor never fires. Check a drive's `wait res` lines before reusing it
   with the flag.
4. **Say which mode a run used.** A run header or a report line should state `skip-intro: on/off`, the same
   way it states the pinned clock and the dump cadence.
5. The flag is a **host** affordance, not a game behaviour and not a cheat: it is off unless the harness asks,
   it does not touch `fn_038a20`/`fn_038ae0`, and it cannot be reached from a drive script or a key.
