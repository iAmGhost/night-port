# 400 — partial update is invalid on a triple-buffered display, and what was actually wrong with it

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.
> *Superseded in part:* settles the question docs/399 opened; `BLIT_DIRTY=0` ships.


Reported after 0.1.79 on hardware: the intro is **still choppy and now glitchy** — "uncleared images and
flickering video". The user's conclusion ("partial-update is nada") is correct, and there is a mechanism that
made the failure certain rather than incidental.

## 1. Why a shadow comparison cannot work against this display path

```c
/* main_of.c, the comment that was already in front of the flip wait:
 * The API is triple-buffered (of_video.h:10), and of_video_flip() does not wait: "It does not wait for the
 * vsync swap to complete, so rendering can overlap scanout". */
```

`stage_commit` skipped rows by comparing against `g_shadow` — the frame *we last wrote*. That is only sound if the
buffer we are writing now is the one we wrote last frame. With three buffers and a flip that does not wait, the
buffer this frame writes was **last written three frames ago**. So every "unchanged" row we skipped left a
three-frame-old picture sitting in the buffer that is about to be displayed.

That is precisely the reported artifact: leftover picture where a row happened to match three frames ago, and
flicker as different rows come from different generations. Nothing about the comparison was buggy; the *premise*
was false. A correct partial-update scheme has to either

- keep **one shadow per display buffer** (three shadows, rotating with the flip), or
- write whole frames into a buffer the core owns for the duration of the write (which is what `PRESENT_SYNC=1`
  approximates), or
- get the OS to do the dirty work.

## 2. What 0.1.80 ships instead

| knob | 0.1.79 | 0.1.80 | why |
|---|---|---|---|
| `BLIT_DIRTY` | 1 | **0** | whole-frame writes: the picture is written every frame into whichever buffer is next, so no generation can leak through. The code stays, counted and switchable, because the *cost* it measured is real |
| `FB_MODE` | 1 (native 640×480 surface) | **0** (OS 320×240 surface, software 2×2 downscale) | 76 800 bytes written per frame instead of 307 200 — four times less into the aperture, and the downscale reads cached SDRAM. This attacks the measured chopiness (`perf pres=15771µs` of an ~18 ms frame) without touching frame-completeness semantics |
| `PRESENT_SYNC` | 0 | **1** | stop letting a write overlap the scanout of a buffer that is mid-frame |

The picture shape (whole frames, every frame) is the 0.1.76 shape the user reported as playable; the change is in
how many bytes each whole frame writes and when the write is allowed to start.

Card: core **0.1.80**, ELF md5 `2892b603deeaf94fa0691265d65d790f` == build, `core.json` == ELF string, ini
`BLIT_DIRTY=0 FB_MODE=0 PRESENT_SYNC=1 SCREEN=0 FIT=1 TICK_HZ=490 DIAG=0 LOG=1`. Repo `make test` = `== ALL PASS ==`.
`src/f` untouched.

## 3. What the next log should show

- `blit dirty staged_frames=0 too_big=… too_small=…` — staging disengaged, as intended; `rows_written` stays 0,
  and that is the expected shape of a disabled feature, not a fault.
- `perf pres=` should fall well below 15 771 µs if the surface size was the cost. If it does not, the aperture
  write rate is not proportional to bytes, and the next lever is word-at-a-time stores instead of `memcpy`'s byte
  loop.
- The mode line should now read `surface=320x240 stride=320`, and `fb mode ->` lines should stay at three (boot)
  rather than growing.
- Chopiness and glitchiness are now two separate claims with two separate causes, so each can be answered on its
  own: completeness no longer depends on a comparison, and cost depends on bytes written.

## 4. The honest summary of this thread

Three attempts at the disc/audio problem produced one fix (the priming ramp, and the music pause is gone) and one
regression that measurement caught (priming 24 sectors starved the ring, docs/397). Two attempts at the picture
problem produced **a new defect** (partial update on a rotating buffer set) and one measurement that was mine to
misread (`malloc` failing, docs/399 §5 — it was guard arithmetic). What has held every time is the rule that a
device-side counter, not a plausible story, decides whether a change ships: `staged_frames=0 fallback=1387` is what
told me the optimization never ran, and the user's eyes are what told me the "fix" for chopiness broke the picture.
