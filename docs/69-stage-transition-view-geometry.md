# Stage-1 startup corruption: root cause, fix, and the paired original-side measurement

Playtest log item 5: *"While starting stage 1 the screen is corrupted (it recovers after a while)"*.
Reproduced, root-caused, fixed, and now measured on both sides.

## What the port was doing (20 Hz capture, `scripts/playtest/route_pause_menu.txt`)

From the `stage1.pcx` hold into gameplay (`/tmp/nightport/st1trans3`, `NIGHT_DENSE=1 NIGHT_DENSEHZ=20`):

| t (ms) | presented geometry | mean luma | edge noise |
|---|---|---|---|
| 52199-52229 | 640x480 | 91.9 | **68.3** |
| 52399-54002 | 640x480 | 111.9 | **81.8-82.0** |
| 54201 onward | 320x200 | 94.3 | 28.9 |

Healthy gameplay frames measure edge noise 28-31 (docs/54's metric); the sheared intro frames measured 58.6.
68-82 is garbage: for ~1.8 s the port presented a **320-pitch, 320x200 room frame read at 640x480 geometry**, so
the frame is sheared 2x over whatever 640x480 card was in the window before it. `scratch/st1_startup_640_noisy.png`
(kept as `docs/manual-playtest/07-shots/stage1_transition_port_before_sheared.png`) shows it: the stage card's
lower part is intact while its top is noise with a HUD strip in it.

## Root cause: a port-only view hint, not the renderer

The renderer is fine. `NIGHT_ROOMBG=<path>` (added this cycle) fingerprints the work screen before/after the room
background blit and the window before/after the copy inside `fn_03801b` (0x03801b), and the single room-entry call
it logs shows:

```
res=stage1.pcx scr_before=43af3fd1 after_bg=9c3e9fa0 win_before=9778f581 win_after=68b05755
```

so `fn_03ce0b` did draw the room background into the work screen and `cpy_dw` did push it into the A0000 window
-- exactly the order the original has (`0x0003a946` blit, then `0x0003a951` palette fade-in).

What is port-only is *how the host decides what geometry to present*. `g_view_320x200` was set only by
`fn_038098`'s page flip, which runs in the room's frame loop, so the room's **entry composite** -- a 0x3e80-dword
(64000 byte = 320x200) push -- was displayed as 640x480 until the first flip. The original cannot show this: it
changes the VGA mode itself (`/api/v1` mode log: mode 257 = VESA 101h for cards, mode 19 = 13h for the room), so a
mode-13h frame is never scanned as 640x480.

Fix: `fn_03801b` sets `g_view_320x200 = 1` after its copy. Pinned by a test
(`test_fn_03801b`: "the room composite marks the window as a 320x200 frame, so the room's entry frame is never
shown at 640x480").

## After the fix, measured against the original

Original (`/tmp/dbgauto/orig_stage`, 8 Hz, `scripts/playtest/drive_orig_episode1.txt`) around its own transition:

```
card fade-out at 640x480: mean 59.6 46.3 34.7 21.2  8.5   edge 13.2 10.4  7.5  5.5  1.8
mode 640x480 -> 320x200 at t=92744 ms
room fade-in at 320x200: mean  0.0 14.9 34.1 51.4 68.3 87.3 95.0   edge 0 4.8 10.6 15.8 20.6 26.2 27.9
steady: mean 95.0, edge 27.9-28.3
```

Port after the fix (`/tmp/nightport/st1fix`, 20 Hz):

```
card fade-out at 640x480: mean 63.1 57.2 38.6 19.4  1.2   edge 14.0 12.7  7.7  3.9  0.5
mode 640x480 -> 320x200 at t=51900 ms
room fade-in at 320x200: mean 18.6 24.5 31.8 37.5 46.1 51.1 58.8 64.6 72.0 75.7 77.6 83.6 94.9
steady: mean 94.5, edge 28.0
```

Same structure: the card fades out *before* the geometry change, the first room frame starts from black, the fade
in is monotone over ~0.6-0.8 s, and both settle at mean luma 94.5-95.0 with edge noise 28.0-28.3. No frame in the
port's transition is sheared any more (`docs/manual-playtest/07-shots/stage1_transition_port_after.png` next to
`stage1_transition_original.png`: the same station platform, HUD panel, DANGER sign and train).

Paired similarity of the settled frames is NCC 0.9928; per-pixel agreement is only 1.26 % within +-3 (MAE 14.6)
because the two runs are at different game states (the player and enemies stand in different places), which is
expected for gameplay frames -- but computing it is what exposed the next defect, below.

## New defect found while scoring that pair: the room palette has R and B exchanged

Colour histograms of the two settled frames:

```
distinct colours: port 109, original 109      shared: only 4
most frequent colours, port      : (109,109,101) x6487, (93,93,85) x5138, (24,36,44) x4270, (77,81,73) x3452, (0,24,32) x2554
most frequent colours, original  : (101,109,109) x6487, (85,93,93) x5138, (45,36,24) x4270, (73,81,77) x3452, (32,24,0) x2554
```

Identical indices, identical occurrence counts, and every port colour is the original's with the first and third
component swapped. So the *pixel indices are right* and the palette bytes pushed for the room are reversed.
`NIGHT_DACLOG=<path>` (added this cycle: every DAC push with the resource lookup in flight, the calling function
and the bytes) shows the bytes are already reversed when `board_out` receives them -- the room-era pushes from
`fn_03ce0b` (which pushes `g_dat_63354 + 0x48`, the room record's palette, every tenth frame) contain (27,27,25),
(23,23,21), (6,9,11), (16,16,15) where the original's displayed colours are those same triples read back as
(25,27,27), (21,23,23), (11,9,6), (15,16,16). Card/PCX-era pushes in the same log are in the correct order, which
is why every index-level card check in docs/58/63 passed and this stayed invisible.

Open questions, deliberately not guessed at: which fill writes the room record's palette (the record is built at
runtime; `stage1.stg` -- 7132 B -- has no plain 768-byte 6-bit table at +0x48), and whether the reversal enters
with the room/stage loader or with a dword-wise copy of a 0x00RRGGBB table. The next step is to trace the room
record's `+0x48` fill, then re-run the histogram test above as the regression check (4 shared colours -> 109).

## Host diagnostics added for this

- `NIGHT_DACLOG=<path>` -- every DAC push: `#n res=<lookup> seq=<n> start= n= from=<fn>` plus the hex bytes;
- `NIGHT_ROOMBG=<path>` (or `=1` for stderr) -- `fn_03801b` fingerprints of the work screen before/after the room
  background blit and of the window before/after the copy;
- `NIGHT_DENSEHZ=<hz>` -- the 320x200 dense sampler's rate (default 10 Hz); a 10 Hz series steps straight over a
  1.8 s transient, and the shot count cap had to be raised with `--shot-limit` (the default 600 truncated the
  series exactly where the transition was).
