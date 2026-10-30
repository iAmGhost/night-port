# 201 — Frame-bin decomposition on both sides: the two games animate different rows, and the port's kill window shows no text-shaped change

docs/200 §3 said the port's `frames.bin` `changed` field is a flag, not a count.  Worked around it by reading the
saved frame bins (`/tmp/rig/ents-port/frames/NNNNNN_320x200.bin`, mapped to times through `frames.bin`'s
`frame` column) and decomposing consecutive-frame differences the same way docs/200 did for the shipped side.

```
PORT (saved bins, 320x200, consecutive saved frames)
  baseline pairs (5600->5608, 5624->5633, 5662->5670, 5718->5727 ...):
      changed = 42 .. 136,  components 18 .. 60,  every component 3 px wide 1 row tall
      top: 3 px @ x=113..115 y=10,  x=117..119 y=10,  x=121..123 y=10,  x=125..127 y=10  (every 4 columns)
  the transition at frames 5752->5760->5773: changed = 13 676 then 19 481, tall components (108..178 px) at
      x=117..119 y=164..199, x=257..259 y=144..178, x=137..139 y=140..199   (a full-screen wipe, not text)
  after frame 5797: a steady 270 changed per pair, components 6 px wide 2 rows tall at y=118..119,
      x=57,61,65,... every 4 columns

SHIPPED (state frames, docs/200)
  baseline pairs (360->361, 365->366, 381->382, 382->383): changed = 56, 28 components of 2 px each at
      x=169..170 y=160,  x=161..162 y=168,  x=165..166 y=168   (2 px pairs in the lower playfield)
  the kill pair 369->370: changed = 5 358, 1 644 components, largest a 3x2 block row at y=26..27 from x=88 every 4 columns
```

## What this settles for item 2a

1. **Neither side's kill-time change is a legible digit string in these samples.**  Shipped's kill change is a
   periodic 3x2 block row at y=26..27 plus 1 643 small neighbours; the port's kill-adjacent change is a full-screen
   wipe (13 676 / 19 481 pixels, components 100-178 px tall).  So "garbled EXP text" is not something either dump
   shows as text at this window, and the gate cannot be satisfied by eyeballing these frames.
2. **The idle animations disagree in position and shape** — shipped: 2-px pairs at x=161..170, y=160..168; port:
   3x1 blocks at x=113..127, y=10 — which is a concrete screen-oracle divergence to attribute (it is the same family
   as the pacing row: `docs/182`'s picture-changes-per-second orig 5.20 vs port 25.08 on the g37 route).
3. Combined with docs/196/198/199 (no EXP award made, `adds=0`, `fn_0383a4`/`fn_038c23` never called, award field 0 vs
   10), item 2a's most defensible reading so far is that the port never produces the *content* the text would show:
   there is no award value, no draw invocation, and the only EXP-shaped number the port renders is the permanently
   zero HUD field (docs/197 §1).  `gate3-exp-text` stays open until the rig can point at a glyph-level difference on
   the unfixed build, which needs (a) the per-frame changed-pixel count + bbox the rig does not emit yet
   (docs/200 §3), and (b) the `exp.spr` glyph raster comparison named in docs/200 §4.
