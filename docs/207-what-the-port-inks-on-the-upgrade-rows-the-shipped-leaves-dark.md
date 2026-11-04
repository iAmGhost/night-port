# 207 — Item 3, screen half: the bar cells the port actually blits on rows the shipped leaves dark

docs/206 localized item 3 to `src/f/f.c:25818` (`n = sel[page]` where shipped uses `sel[i]`,
`0x00039848-0x000398a9`).  This pass records the *picture* consequence, read out of the port's own blits.

## 1. The blit witness on the unfixed build

A `GLYPH_TRACE("lurow", ...)` was added inside the row loop (before the `fn_03bb42` call) so the trace prints the
sprite cell actually blitted per row, and it was paired with the `NIGHT_LUMENU` row witness from docs/206 on
`scripts/rig/drives/ep1_lumenu.txt` (`play 0x27 1`, `play 0x28 2`, `NIGHT_SHOWSCREEN=levelup`,
`NIGHT_SHOWSCREEN_AT=5600` -> warp at frame 5612):

```
paired (row witness, blit witness) cases: 42
  cases where the row's own sel[i]==0 yet a bar cell was blitted: 28

example frame 0 (page=1, so the cursor sits on row 1):
  rows blitted though sel[i] = 0:   i=2 n=2 barRowY=111 colOff=77  rows=9
                                    i=3 n=2 barRowY=131 colOff=31  rows=13
                                    i=4 n=2 barRowY=151 colOff=31  rows=13
                                    i=5 n=2 barRowY=170 colOff=31  rows=13
  rows the shipped WOULD blit:       i=0 sel=1 n=2 barRowY=69  colOff=151 rows=9
                                    i=1 sel=2 n=2 barRowY=94  colOff=151 rows=9
column-offset census, wrongly-lit rows: {31: 21, 77: 7}   (cells 31 and 77 are never reached by a correct row)
column-offset census, all blitted rows: {151: 14}
```

The blitted streams are filled runs, e.g. `01 00 97 ef ef … d0 d0 d0 …` (one run, advance 0, length 0x97 = 151
literals) -- bar ink, not a background passthrough.  So the port paints sprite cells at screen rows y=111, 131,
151, 170 on rows 2-5 that the shipped loop never reaches: at `0x398b3` the shipped does
`cmp [ebp-0x50], 0 / je 0x39943` for `sel[i] == 0`, skipping the blit entirely.

Ink counts on the port's own warped level-up frame (`/tmp/rig/l5/frames/005669_320x200.bin`, non-modal pixels in
the 9-row band starting at each bar row, x=18..199):

```
y= 69 (row 0, sel=1): 942     y=111 (row 2, sel=0): 928
y= 94 (row 1, sel=2): 942     y=131 (row 3, sel=0): 530
                              y=151 (row 4, sel=0): 852
                              y=170 (row 5, sel=0): 963
```

Every bar row carries ink, including the four whose own selection count is zero.

## 2. Shipped-side capture of the same screen: what worked and what did not

`scripts/playtest/drive_orig_levelup_hold.txt` (the docs/85 spine up to `writemem 0x6336c 2`, then six recorded
windows and **no key presses**) ran through dosbox-automation: `done: 90 saved frames in /tmp/dbgauto/orig_lu7`,
with `readmem 0x63370 = 2` (the shipped really moved to stage 2) and `readmem 0x6336c = 0` (the exit latch was
consumed, which is what calls `fn_03d244` -> `fn_038f3d(1)`).  The captured tail frames are 320x200 mode 0x13 and
match none of the card references the scorer knows (`sim=-0.0014`, best label `stage1`), i.e. the shipped side was
on a picture outside the reference set -- consistent with docs/85's finding that this path settles on a help card.
Correlating those frames against `scratch/res/levelup.pcx` / `levelup2.pcx` failed on my PCX header parse
(decode reported 319x0 dimensions), so **the shipped level-up card frame is not yet in the rig's evidence set**;
the shipped-side half of item 3's screen claim rests on the instruction-level fact above (the shipped blits nothing
when `sel[i] == 0`) plus the port-side ink census.  Closing that gap needs a shipped run that holds
`fn_038f3d`'s first picture (docs/85: one key per picture) and a working PCX decoder for the shipped cards.

Item 5 gets a usable lever from the same run: `writemem 0x63370 2` puts the *shipped* game into stage 2 headlessly,
which is the drive shape `gate3-stage2-crash` has been missing.
