# Correction: the R/B "swap" is in the capture harness, not in the port — and what that proves

docs/69 ended by filing a defect: the settled stage-1 frame's colours were the original's with red and blue
exchanged. That was wrong, and the correction turns into the strongest rendering evidence in the project.

## The chain that settled it

1. **The port's room palette is the shipped bytes, verbatim.** The DAC push logged by `NIGHT_DACLOG` for the
   room era (256 triples, pushed by the port's room-draw path from the room record's `+0x48`) is byte-for-byte
   equal to `scratch/res/stage1.wal` at offset `0x4e21e`: **768/768 bytes equal**. The original's node loader
   (`0x0003c853`, disassembled at `0x3c9e1-0x3c9f4`) reads that same field with
   `count=0x100, element size=3` into `node+0x48` through the generic stream reader at `0x4192e`, which is a
   plain copy (`movsd` + `repne movsb`) -- no reordering anywhere in the load path. So both sides hold the same
   palette bytes.
2. **The two sides disagree by an exact channel mirror, and only that.** Per-index colour histograms of the two
   settled stage-1 frames have 109 distinct colours each, identical occurrence counts (6487, 5138, 4270, 3452,
   2567, 2554, 2476, 2384, ...), and every colour on one side is the other's with components 0 and 2 exchanged.
   Taking the port's frame, exchanging its channels and comparing to the capture gives
   **100.0000 % of pixels within +-3, MAE 0.17, 65.7 % exactly equal** (the remainder is entity positions, which
   differ between the two playthroughs).
3. **Which side is wrong is decided by an absolute anchor, not by majority.** The shipped palette's index 1 is
   (45,24,24) in 6-bit, i.e. (182,97,97) after DAC scaling -- a desaturated red. The port shows exactly that
   colour on 378 pixels; the capture shows its blue mirror on the same 378 pixels. The HUD portrait's mean tint
   is warm `(149,119,100)` in the port and cool `(101,119,149)` in the capture. Faces and the palette in the
   shipped file agree; the capture does not.
4. **And the emulator reports the same pixel-format byte for every mode**, so the difference is not declared in
   the header: the raw frame header for the three modes seen is `w/h/pitch` then `20 00 00 00` for 720x400
   (pitch 2880), 320x200 (pitch 2560, double-width) and 640x480 (pitch 2560). Yet the 640x480 payloads reproduce
   the shipped PCX palettes exactly (the title-card pair matched at 100.0000 % within +-3), while 320x200
   payloads come out R/B exchanged.

Conclusion: **mode-13h payloads from the dosbox-automation API are BGRX while VESA 101h payloads are RGBX**
under this decoder/emulator combination. `scripts/playtest_orig_drive.py` now swaps R/B for `h == 200` payloads
(`--swap13 0` disables it), with the measurement above recorded in the code comment.

## What this changes

- The filed "room palette R/B swapped" defect is **withdrawn**: the port's room palette, room indices, entity
  sprites and HUD composite are pixel-correct, to within +-3 on 100 % of the frame, against the original's own
  captured mode-13h frame. That is the first strict per-pixel win for *gameplay* rendering (everything
  previously strict was cards/screens, and everything gameplay-level before this was index-level).
- Every earlier comparison that involved a mode-13h original frame was scored against a channel-mirrored
  reference and must be re-scored: the pause menu pair (NCC 0.87), the gameplay/pause pairs from
  `/tmp/dbgauto/orig_full`, and the `demo`/`fli` entries in `scratch/origcmp/refs_orig`. The VESA-era pairs
  (logo, title card, per-episode slides, ending cards) are unaffected.
- The stage-transition conclusion in docs/69 stands unchanged: it was about geometry (a 320x200 room frame being
  presented at 640x480), and that fix plus the fade-envelope match against the original were measured on luma
  statistics, which a channel mirror does not move much.

## Method note worth keeping

The one-sided mirror was only visible because the comparison was done on *colour histograms with counts*, not on
NCC: NCC of a mirrored pair is still 0.99. Any future port-vs-original scoring should print the histogram
comparison (distinct colours, shared colours, top counts) alongside NCC -- that is what caught both this and, in
docs/69, the geometry problem.
