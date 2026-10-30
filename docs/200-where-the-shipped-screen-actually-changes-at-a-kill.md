# 200 — Item 2a screen basis: the shipped kill changes a segmented row at y=26-27, and the panel-text hypothesis is superseded

## 1. Shipped: the kill produces a screen spike, and it decomposes into a periodic row, not text at the panel coordinates

`/tmp/rig/ents-orig/orig/state_NNNN.frame.bin` (15-byte header, 320x200 indexed, pitch 320 bytes/row, docs/181 §6).
Consecutive-state changed-pixel counts around shipped's first counted kill (kill counter increments at state 371):

```
state:  360  361  362  363  364  365  366  367  368  369  370  371  372  373  374  375  376
changed: 56   84  114  734    0   56    0   56    0   56 5358   84   56   56   84   84  596
```

5 358 pixels change at state 370 against a 0-84 baseline.  Connected-component decomposition of that change:
**1 644 components, every one 10x10 or smaller**, and the largest are a regular row of 3x2 blocks:

```
pixels=6 x=88..90   y=26..27      pixels=6 x=92..94   y=26..27      pixels=6 x=96..98  y=26..27 ...
(one block every 4 columns, continuing across the playfield)
```

So the visible kill-time change is a **periodic segmented row at y=26..27** (and neighbours in the x=56..198
bbox), not a string of 5x7/6x7 digits.

## 2. The coordinates I had guessed are static map tiles in the shipped frames

`fn_038c23` is called with `col0 = 0x68` (104), `start_row = 0xac` (172), and `fn_0383a4`'s two sites with
`0x118` (280), `0xae` (174) (docs/195 §1b).  Rendering the shipped frame as ink/no-ink over x=100..148,
y=168..184 at states 369, 371, 373 and 399 gives an identical static pattern (`###.` repeating every 4 columns)
at all four states — that region is background tiling and never changes across the kill.  The draw sites are real
(`branch_xrefs` edge sets, docs/195 §1b) but they are not where the kill's visible text is; docs/196 §2's sprite-record
candidate and this doc's row are better leads, and docs/182 row 2a's "region" column must be read as unresolved.

## 3. Rig gap found while doing this (quoted, not worked around)

The port's `frames.bin` (NGFR) carries a per-frame `changed` field, but on `/tmp/rig/ents-port` every record in the
kill window reads 0 or 1: 395 frames in t=[183 000, 186 500] ms, `max changed = 1`, none >= 100 — while the same
window on the shipped side spikes to 5 358 changed pixels.  `changed` is therefore a *flag*, not a pixel count, so a
region-level port-vs-shipped comparison cannot be made from `frames.bin`; it needs the saved frame bins
(700 of them in `/tmp/rig/ents-port/frames/`, mixed `640x480` and `320x200` sizes).  Closing that — emitting a
changed-pixel count (and a changed-region bbox) per frame record — is the concrete rig work that item 2a needs
before its screen gate can be called.

## 4. What to compare once the region basis exists

Compare *glyph ink patterns* rather than regions: decode the glyphs the port loads from `exp.spr`
(`char_table = [font+0x34]`, `rle_base = [font+0x38]`, docs/192) with the independent .spr decoder in
`scripts/playtest_verify_spr.py`, rasterise each 5x7 / 6x7 glyph, and check which of them actually appear as
ink clusters in the shipped frames near a kill.  If a glyph the port rasterises does not match the shipped ink,
that is the byte-level answer item 2a asks for; if all of them match, the garble is positional (where the port
blits them), which docs/196 §1 already narrows to the value/draw-invocation path (`adds=0`, `commits=0`,
`fn_0383a4 = 0`, `fn_038c23 = 0`).
