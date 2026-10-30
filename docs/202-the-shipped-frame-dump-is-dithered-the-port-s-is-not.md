# 202 — Basis correction: the shipped frame dump is a 3-on/1-off period-4 dither, the port's saved bins are not

While testing whether the port paints solid rows where shipped paints none, the two dumps' row statistics turned out
to be incomparable, and the reason is measurable in the dumps themselves.  Classifying every 4-pixel block of a
shipped state frame (`/tmp/rig/ents-orig/orig/state_0365.frame.bin`, 320x200 indexed) versus a port saved bin
(`/tmp/rig/ents-port/frames/005600_320x200.bin`), on = non-zero index:

```
shipped 4-px block patterns:  (1,1,1,0) x 15196   (1,1,0,0) x 776   (0,1,1,0) x 28     of 16000 blocks
port    4-px block patterns:  (1,1,1,1) x 15537   (1,1,1,0) x 411   (1,1,0,1) x  52     of 16000 blocks
```

95 % of the shipped blocks are exactly three lit pixels and one gap, repeating every 4 columns, while 97 % of the
port's blocks are fully lit.  So:

1. **The shipped dump carries a period-4, 3-on/1-off dither the port's dump does not.**  Row-level run statistics
   therefore disagree by construction: shipped's longest solid run is 3 px in *every* row (ink 75 %), the port's runs
   reach 124-320 px.  The "the port draws solid bars where shipped draws nothing" pattern in this session's first pass
   is an artifact of that basis, **not** a garble finding, and it is recorded here as such rather than used.
2. Docs/201 §1-§2 (the component-shape comparison: shipped 2-px pairs at x=161..170 y=160..168 vs port 3x1 blocks at
   x=113..127 y=10, and shipped's 3x2 kill row at y=26..27) must be read with the same caveat: the shapes are filtered
   by the dither, so they describe the *dumps*, not two renderings of the same frame.
3. De-dithering by taking the max inside each 4-px group gives an 80x200 grid on both sides; comparing those for
   shipped state 365 vs port frame 5600 leaves 14 006 of 16 000 cells differing (87.5 %), which is too large to read
   as a text-region difference and says the two samples are not at a comparable presentation state (docs/47 pacing: the
   port runs its clock fast on ad-hoc runs, docs/168).

Consequence for the six-item goal: `gate3-exp-text` (and the screen halves of `gate3-upgrade-menu`,
`gate3-stage2-crash`, `gate3-side-glitch`) need a **same-stage basis** before any region-level claim is admissible —
either de-dither the shipped capture with the measured pattern and compare against a port frame taken at an
event-anchored instant, or compare the port's *back buffer* (what `NIGHT_GLYPHDBG` prints: the row-RLE stream and the
bytes the blit wrote) against the shipped resource glyph, which is stage-independent.  docs/201 §4 already shows the
latter path works and that the HUD digit the port composites is a well-formed 5x7 glyph.
