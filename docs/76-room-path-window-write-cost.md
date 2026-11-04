# 76. The room path's A0000 window-write cost, and what the room pump's cadence really is

Closes the log item docs/72 filed as OPEN: *"the charge is applied only in `fn_03f462` (slide/title writer). The room
renderer's page flip and the sprite blitter write into the same window and are not charged yet."*

## Which room writes actually touch the A0000 window (measured, not assumed)

The filed item was half right, and the half that was wrong matters: the room renderer writes to the A0000 window
**exactly once per frame**, and the sprite blitter never touches it.

Resolving the two arenas the port uses (`video_map()` in `src/f/f.c`) settles it:

| write in a room frame | destination | through the window? |
|---|---|---|
| `fn_03ce0b` room background plane (`blt_rows`, up to 200 rows x 0x140) | `video_map(DAT_633b0)` -> `g_screen_ram` (the two heap-allocated off-screen 64000 B screens) | **no** -- ordinary RAM |
| `fn_03bb42` every entity sprite (`fn_03203d` -> `fn_012efa`) | same off-screen screens | **no** |
| `fn_03df96` / `fn_03b953` glyphs and number displays | same off-screen screens | **no** |
| `fn_01348f` / `fn_0137c1` gauge panel + countdown | same off-screen screens | **no** |
| `cpy_dw(video_map(base), 0x3e80, g_video_ram)` the page flip | `g_video_ram` = the A0000 window arena | **yes -- 64000 bytes** |

`DAT_000633b0` / `DAT_000633b4` are the two off-screen screens the game `heap_malloc`s (64000 B each); the flip copies
the finished work screen into the window. So charging the blitter would have been wrong (double-counting RAM work as
uncacheable I/O). The charge is now applied at the three flip sites: `fn_03801b` (the room entry composite,
`0x38078-8c`), `fn_038098` (the paced frame pump, `0x38154-68`) and `fn_0381d7` (the player step-animation loop,
`0x3833b-4f`), each `vga_charge(64000)` with the same calibrated constant as docs/72.

## What the room pump's cadence actually is: 70.0 frames/s, exactly

New host diagnostic `NIGHT_FRAMERATE=1` (prints from `fn_038098` once per second: calls / drawn / skipped / charged
window bytes). Per-stage runs (`--warp-stage 1`, `scripts/playtest/route_show_keyed.txt`, `--frames 600`):

```
NIGHT_VGAWS=0     room calls=70.0/s  drawn=70.0/s  skipped=0.0/s     (charge disabled = the state before this change)
NIGHT_VGAWS=63    room calls=70.1/s  drawn=70.1/s  skipped=0.0/s     (host default, tuned in docs/72)
NIGHT_VGAWS=97    room calls=70.0/s  drawn=70.0/s  skipped=0.0/s     (the physical 486 figure from docs/72)
NIGHT_VGAWS=150   room calls=70.0/s  drawn=70.0/s  skipped=0.0/s
NIGHT_VGAWS=223   room calls=70.0/s  drawn=70.0/s  skipped=0.0/s     <-- right at the budget boundary
NIGHT_VGAWS=300   room calls=69.8/s  drawn=50.8/s  skipped=18.9/s    <-- the pump starts shedding frames
```

The cadence is set by the pump's own gate, and the numbers line up exactly:

* `fn_03ef29` reprograms PIT ch0 to divisor `0x982` = 2434 -> **490 Hz** (`1193182/2434 = 490`), which is the tick
  rate the host pump already drives (`src/app/main.c`, and `DAT_53c48`'s threshold is compared against 490).
* `DAT_00053104 = 0x46 = 70`, so the gate value is `n = 0x1ea / 70 = 490/70 = 7` ticks per frame -> the pump waits
  until 7 ticks pass -> **490/7 = 70.0 frames/s**, i.e. a 14.29 ms frame budget.
* 70 Hz is also the VGA mode-13h vertical refresh rate, which is what the game's retrace waits (port `0x3DA`,
  316-623 waits per episode in docs/59) are waiting for. Two independent mechanisms in the shipped code --
  a 490 Hz tick accounting and retrace waits -- both point at 70 Hz.

Cost accounting of one room frame with the charge enabled: 64000 B x 63 ns = **4.03 ms**, or x 97.5 ns (the physical
486 figure) = **6.24 ms**, out of the 14.29 ms budget -- 28 % to 44 % of it. The frame-rate cap is therefore enforced
by the tick gate, not by the window writes, and the flip has real headroom. The shedding branch is real though: at
`>= 223 ns/byte` the flip alone consumes the whole budget (`64000 x 223 ns = 14.27 ms`), and at 300 ns/byte the pump
draws only 50.8 of its 69.8 iterations -- the `CMP/JLE` at `0x380e3` skips the draw when the tick counter has already
run past `n`, which is exactly how the original would degrade on a slower machine or in a heavier room. That is the
mechanism docs/72 suspected; it is now measured at both sides of the boundary.

## No regression in the pacing docs/72 pinned: A/B with a new harness

New script `scripts/playtest_title_melt_span.py` (arena-byte comparison against independent decodes of
`title3.pcx` / `logo.pcx`, docs/63's method, so palette fading cannot distort it):

```
build WITH the room-flip charge    title ramp 43219 -> 49221 ms  span 6002 ms  42 partial frames
build WITHOUT the room-flip charge title ramp 43232 -> 49237 ms  span 6005 ms  42 partial frames
logo plateau 2603 ms vs 2592 ms
```

The room charge moves the title melt by **3 ms out of 6002 (0.05 %)** and the logo plateau by 11 ms -- as expected,
since the melt (`fn_03a224` -> `fn_03f462`) runs entirely before any room composite exists. The match fraction ramps
monotonically 0.0996 -> 1.0000 over 42 sampled frames, which is also direct evidence that the reveal is the
progressive row-pair wipe rather than a fade.

Measurement caveat recorded honestly: this harness's absolute span (6002 ms on the manifest's model clock; 5483 ms on
the presented-frame clock, 329 frames at 60 fps) is *not* the same number docs/72 quoted (7262 ms, ratio 1.010 against
the original's 7190 ms). The two detectors pick different first-partial and first-settled frames and different clocks;
the A/B above compares the harness with itself, so the no-regression claim stands regardless. Reconciling the absolute
melt ratio against the original is filed as a separate measurement item (it is a capture/detector question, not a
room-path pacing question).

## Reproduce

    # room cadence sweep
    for ws in 0 63 97 150 223 300; do NIGHT_FRAMERATE=1 NIGHT_VGAWS=$ws \
      ./build/night --warp-stage 1 --script ../scripts/playtest/route_show_keyed.txt \
      --shots /tmp/fr --no-audio --frames 600 --shot-limit 4 2>&1 | grep framerate | tail -1; done
    # title/logo regression A/B
    python3 scripts/playtest_title_melt_span.py
