# 234 — Item 1's red measurement was a `--rig-dump` artifact: without the dump the port selects the title track and the room track, exactly as the shipped does

Task: `fix-music`.  This supersedes the measurement half of docs/183, docs/184 §3, docs/185 §3 and the item-1 row
of docs/226, and retracts the mechanism note in docs/232 (its shipped-side facts stand, its port-side diagnosis
does not).  No port behaviour is changed by this document except a new host witness.

## 1. New witness: `NIGHT_TITLEDBG=1`, phase timings inside `fn_03a224`

Added to `fn_03a224` (`src/f/f.c:15786-15800`, phases cited to the shipped addresses the code models:
0x3a2c5-d2 the three `title*.pcx` loads, 0x3a2d7-384 the wipe, 0x3a3bb-407 the `0x400` `delay_units(1)` rounds,
0x3a409-431 the release drain, 0x3a433-42 `DAT_63f08 = 0` + `fn_10b1d(9)`, 0x3a451-522 the menu loop):

| run | build | drive | wipe | delay rounds | drain | `fn_10b1d(9)` reached at | game tick there |
|---|---|---|---|---|---|---|---|
| `/tmp/rig/m1_trace.log` | `build/night` | `ep1_title_music` | **10 759 ms** | 0 ms (a drive key breaks the loop) | 24 ms | **10 783 ms** after title entry | 14 425 |
| `/tmp/rig/m1_asan.log` | `build/night-asan` | `ep1_title_music` | **10 792 ms** | 0 ms | 8 ms | **10 800 ms** | 14 651 |

Two builds agree to 33 ms.  The menu loop then runs 21 353 ms (until the drive's key), and the next selection
(index 11, the stage-start slides) lands 44 s into the run ✓ So the title screen's own CD request
(`mov eax, 9; call 0x10b1d` at 0x3a43d-42) is reached ~11 s after the title pictures load: **it was never late.**

## 2. The selection timeline, run without `--rig-dump`, pinned clock

`env SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`,
`--script scripts/rig/drives/ep1_kill3.txt --frames 9000`, witness = the `cdda: track index N lba=… sectors=…`
line the `fn_10b1d` seam prints (`sys_cd_audio_play`):

| build | index 1 `37262` (42.0 s) | index 9 `185120` = `0x2d320` (369.3 s) | index 11 `223146` = `0x367aa` (238.1 s) | index 2 `40415` = **`0x9ddf`** (261.9 s) | further |
|---|---|---|---|---|---|
| current `build/night` (`/tmp/rig/m1_kill3.log`) | 10 s | 42 s | 72 s | **99 s** | — |
| frozen baseline `4ea273d` binary (`/tmp/rig/m1_base.log`) | 12 s | 45 s | 48 s | **77 s** | 12 `241002` @155 s, 2 again @157 s |
| shipped (docs/183 §3, `/tmp/rig/g36/orig`) | not seen in the sampled window | 185 120 | 223 146 | **40 415** (fingerprint +0.837, the only track its mix contains) | selected inside its first 31 s |

`185120 = 0x2d320`, `223146 = 0x367aa`, `40415 = 0x9ddf` — the port (current **and** pre-fix) requests the same
three LBAs the shipped does, in the same order, through the same three sites (`fn_03a224`'s `fn_10b1d(9)`, the
stage-slide `fn_10b1d(11)` at 0x3b084, and `fn_010d47`'s room-table lookup yielding `g_tab_52da4[0] = 2`).
So the selection oracle does **not** separate the port from the shipped on this route, and item 1's
"the port never requests `0x9ddf`" line is gone.

## 3. What the dump actually did to the earlier numbers

`/tmp/rig/m1_red` (`--rig-dump … --rig-hz 10 --frames 9000`, drive `ep1_title_music`, 1 500 samples, 8 995 frames,
210 s wall) — the run docs/183/docs/184 §3/the docs/226 item-1 row were built on:

| cell | measured |
|---|---|
| `DS 0x53c44` (selected start LBA) | 0 → 37 262 at 13.4 s (frame 805) → **185 120 at 178.4 s (frame 7 111)** |
| `DS 0x53c48` (selected length, s) | 0 → 42 @13.4 s → 369 @178.4 s |
| `DS 0x63e38` (the CD age `fn_010d47` divides by 490) | **changes in all 1 500 samples, +66 per 0.1 s = 660 ticks/s; never frozen** |
| `DS 0x63f08` (title demo counter) | changes in all 1 500 samples, +9 per 0.1 s |
| `DS 0x63354` (current room record) | **0 in all 1 500 samples** — this drive never loads a room, so `fn_010d47` (its only caller is `fn_03a829`, docs/184 §5) cannot run on it ✓ that half of docs/184 stands |
| frame modes | 640×480 frames 0-432, **320×200 frames 910-1190** (`opening.fli`), 640×480 frames 1204-7111 |

The same drive, same binary, no dump: title pictures at 12-15 s, `fn_10b1d(9)` 10.8 s later.  With the dump the
request lands at 178 s.  The dump therefore inflates wait-dominated presentation phases by roughly an order of
magnitude (`--rig-hz` sampling + a 1.2 MB frame bin per presented frame inside the 240-pass wipe), and docs/183's
"166.7 s hold" and docs/184's "no cell samples between 40 s and 105 s" are that inflation, not port behaviour.

**Standing rule taken from this:** a `--rig-dump` run is admissible for *what* happens and for event-anchored
comparisons; it is **not** admissible for absolute pacing/latency claims.  Pacing statements need a dump-free run
(`--frames N`, no `--rig-dump`) or an event anchor.  This is the same class of error as docs/181 §4 (pointer-valued
cells) and docs/191 §1 (run-to-run variance): the measuring rig contributed the quantity being reported.

## 4. The fix that was written and reverted

Implemented during this pass, in `tick_pump`'s fixed-step branch (`src/app/main.c:1395`), plus a
`g_clock_read_calls` counter in `delay_units`: age `fn_03ee88` from a `max()` of two measures of elapsed guest
time — presented frames at `NIGHT_RIG_FPS`, and the cooperative waits (`g_vga_retrace_waits × g_vga_retrace_us` +
DOS-clock reads × `g_delay_unit_us`) — with a bounded catch-up burst.  Motivated by docs/232's claim that a screen
blocking inside one game step freezes `DS 0x63e38`.

**Reverted, unshipped.**  §3 falsifies its premise: in the very dump run that produced the red claim the age
counter advanced at 660 ticks/s (the frame term was never starved), and the plain runs already select the shipped's
LBAs.  A wait-count term would additionally change the tick budget of every route (docs/167 §21's calibrated 9-10
frame-counter quanta per 100 ms, docs/107's "the tick sequence is a function of the presented frame count alone"),
i.e. it risked the pacing basis and the replayability basis in exchange for nothing.  The `NIGHT_TITLEDBG` witness
stays; the clock change does not.

One shipped-side number is worth keeping on the record while it is in hand: the PIT channel 0 program written by
`fn_03ef29` (`0x982` → 490 Hz, read back by `main.c:1382` as `sys_pit_hz(0)`) matches `fn_010d47`'s divisor 490
exactly, so the shipped ages 490 ticks per second, while the rig's pinned `NIGHT_RIG_STEP_CALLS = 665` is 1.36×
that.  That gap is a pacing question of its own (docs/167 §21 calibrated 665 against the frame-counter cell
`DS 0x63360`), not part of item 1, and it is *not* evidence for a music defect.

## 5. What still has to be measured before item 1 can be called fixed or absent

The remaining question is audible content, not selection timing:

1. **Does the port's stream contain the room track's audio?**  Compare the port's captured CD-DA bytes
   (`<dump>/cdda.raw`, `<dump>/pcm.wav`) against the shipped `audio.wav` of the *same* route (`/tmp/rig/g33/orig`
   carries audio.wav for the canonical 150 s `ep1_kill3` run) with the existing fingerprint tools
   (`scripts/rig/audio_ncc.py`, `scripts/rig/cd_track_timeline.py`, `scripts/rig/disc_search.py`), keyed on the four
   selected LBAs 37 262 / 185 120 / 223 146 / 40 415.  Content comparison is immune to the dump's timing skew, which
   is what makes it admissible where docs/183's timing comparison was not.
2. **Does the port stop the old track when it starts the new one?**  `cdstops` in the pump diagnostics counted 2 in a
   plain title run (boot + the switch) while the stream continued with a fresh position (`pos` 352 → 12 496 sectors)
   and 29 MB more CD-DA bytes after the switch ✓ — the discontinuity needs the same check on a run that reaches
   index 2, and docs/183 §2's "no teardown-without-stop" claim needs re-reading under that basis.
3. **The keypress-skip half of the item.**  On the plain runs the gap from the index-1 request to the index-9 request
   is 32-33 s, close to track 1's 42 s extent, which is consistent with the intro *playing out* rather than being
   skipped by the drive's Enter.  The shipped side's own selection timeline for the same drive has to be re-read
   (its `g36` window may simply start after an earlier selection — the shipped's sampled selections begin at
   185 120, with no 37 262 seen at all): a shipped-side `wait ds 0x53c44 >= N` bracketing run, or `cd_schedule.py`
   anchored on the stage-data load, answers it.  Until then the "skip" half is unresolved in *both* directions.

Only if (1) shows the port missing the room/title track audio while the shipped has it is there an audio-layer defect
to fix, and the target is then the `sys_cd_audio_play` seam / `cdda_pump` teardown that docs/183 §2 already named —
not the CD selection layer, which §2 shows is faithful.
