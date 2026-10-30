# 209 — Item 4 measured on the unfixed build: the port releases the "EXP not enough" card on key-up

## 1. The menu cursor does NOT auto-advance (a negative that removes one hypothesis)

`scripts/rig/drives/ep1_lu_hold.txt` = ep1_kill3 to 94 s, then only the record pokes `play 0x27 1`, `play 0x28 2`
and **no key events at all**; the level-up screen is entered by `NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=5600`.

```
witnessed (frame,row) rows: 10356   distinct menu frames: 1726   page histogram: {0: 10356}
page changes with no key in the drive after the warp: 0
```

The cursor stays on row 0 for the whole 1 726 menu frames (the run ends on the frame budget, not on the menu
leaving).  So item 4's "advances automatically" is not the row cursor moving by itself.

## 2. The card itself is replaced at key release

`scripts/rig/drives/ep1_lu_confirm.txt` = the same drive plus two confirm presses (`100000 key enter down` /
`100600 up`, `104000` / `104600`).  With the player's EXP at 0 the confirm path is the rejection branch
(`P[+0x2b] < cost` at 0x000394d2-0x00039520), which is what loads the "a cheaper option exists / EXP not enough"
card, and the resource markers show one card load per press:

```
t= 99084  levelup.spr  (x2), levelup2.pcx (x2), help1.pcx      <- fn_038f3d entry
t=100017  help8.pcx                                             <- first confirm, EXP insufficient
t=104000  help8.pcx                                             <- second confirm
```

Frame stream around the first load (`/tmp/rig/l7/frames.bin`):

```
level-up entry frame 5638 t=99100 320x200
help8.pcx load frame   5693 t=100017
frames in 100017..104200: 507   distinct pictures: 33
  frames identical to the help8 frame:                       97   (~1.6 s, exactly the press-held interval)
  frames identical to the level-up entry picture:             0
  first frame differing from the help8 picture: 5729 t=100617  -- 600 ms after the load, at the drive's
                                                                 key-UP event, with no further key press
```

So the port holds the card only while the key is physically down and moves on at **release**; a shipped picture
is replaced by a *press*, not by a release.

## 3. Shipped-side key-wait basis (measured, with its limits)

* docs/85 already measured `fn_038f3d` waiting one key per picture.
* `/tmp/dbgauto/orig_lu6` (drive `drive_orig_levelup.txt`: one `key enter 150` then a 2500 ms key-free window per
  phase) -- per-phase picture census over 15-frame windows:

```
phase p01: distinct= 3 longest-held=13      phase p05: distinct= 2 longest-held=14
phase p03: distinct= 2 longest-held=14      phase p07: distinct= 2 longest-held=14
phase p09: distinct= 2 longest-held=14      phase p11: distinct= 2 longest-held=14
phase p02/p04/p06/p10: distinct=15 (a transition/animation phase between cards)
```

* `/tmp/dbgauto/orig_lu7` (`drive_orig_levelup_hold.txt`: the exit latch written, then recorded windows with no
  keys) -- `p00_01`, `p02_01` and `p04_01`, sampled ~6 s apart in three different windows, are **byte-identical**
  PNGs (`5e63cc468ffeae173a2026a4bfc37bf6`), and `readmem 0x6336c = 0` shows the latch was consumed.

Limit to record honestly: a purpose-built shipped key-wait run (`scripts/playtest/drive_orig_keywait.txt`,
`/tmp/dbgauto/orig_kw1`, one press then three key-free windows) did **not** reach `fn_038f3d` this time --
`readmem 0x63370 = 1` and its frames are labelled `title3` at sim 0.09 -- and the dosbox-automation container had
to be re-brought-up (`scripts/rig/bringup.sh`, cycles pin 120000) before it ran.  The shipped half of the contrast
therefore currently rests on docs/85 plus the two runs above, not on a fresh same-state capture.

## 4. What is localized, and the next measurement

The divergence is in the wait condition on the card, not in the cursor logic: the port's loop continues (33 distinct
pictures in 4.2 s) once the confirm key is released, while the shipped's picture persists until the next press.  In
the model that is the release-drain family around `f.c:25783-25797` (`0x000396a0-0x00039767`, the `up_ctr` /
`down_ctr` 0x13→0x14 guards) plus the card display path `fn_035f1b("help8.pcx", 1)` at `f.c:25750`, which returns
instead of holding a picture until a key arrives (docs/97's slideshow key-wait mechanism).  Next step: re-run the
shipped key-wait drive with the stage genuinely live before `writemem 0x6336c 2` (the docs/85 sequence uses
`key_until_mode_stable 200 enter 30 1200 4` and `writemem 0x63370 2` first) and take the shipped picture-stability
numbers after a single press+release, so both sides are measured in the same screen state.
