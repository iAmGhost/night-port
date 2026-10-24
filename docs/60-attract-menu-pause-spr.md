# 60 — The setup / attract menu (pause.spr): what the port does, measured

The playtest report asks for the "pause menu". In this game that screen is the five-entry setup menu
drawn from `pause.spr` by `fn_039c03` (orig 0x00039c03, 1456 B) -- the same routine the attract loop uses
before the game starts. It loads at boot (`g_dat_63e0c = spr_block("pause.spr")`, `f.c:2417`) and is
composed per frame, not loaded as a card, so it never appears in the resource log: the resource lookups
stay on whatever card was last shown.

## It is reached without any key press

`scripts/playtest/route_attract_wait.txt` (anchored on `title1.pcx`, then no keys at all):

```
shot 0084 title1.pcx      frame=2288 res=title1.pcx
shot 0085 title1.pcx_hold frame=2343 res=title1.pcx
shot 0086 wait_p4         frame=2505 ... shot 0095 wait_p22 frame=3485
```

Frames from `title1.pcx_hold` (2343) to `wait_p22` (3485) -- over 22 s, 1142 frames -- are **byte-identical
to each other and to the frame the earlier route named `menu`** (mean L1 = 0.00, brightness 75.07), and that
frame is clearly a different screen from the title card itself (`title1.pcx` brightness 119.59, L1 to the
menu frame 71.49). So the menu is displayed on its own after the title card, and it is static while nothing
is pressed -- which is expected: the routine composites a frame only when something changes.

Two consequences for the harness: an `Enter` **on the title card** skips the menu entirely and goes straight
into the game (`1s1.pcx -> 1s2.pcx -> 1s3.pcx -> stage1.pcx` in `route_setup_menu.txt`), and a key press
*inside* the menu leaves it -- the routine exits when any of `DAT_00063ea4 / 0x63ec1 / 0x63ea5 / 0x63ec0`
is set (that is why the harness's Enter/space, which the host routes into `DAT_00063ea4`, never stays in
the menu).

## Keys do reach it, and the screen follows them

`scripts/playtest/route_setup_toggles.txt` presses Down / Right with one second between presses (the menu
arms key auto-repeat for 0x13 frames after each press):

| transition | frame delta | changed pixels | bounding box |
|---|---|---|---|
| `menu_m0` -> `menu_down1_m4` | 2342 -> 2614 | 1.90 % | rows 279..444, cols 350..639 |
| `menu_down1_m4` -> `menu_m4_right` | -> 2777 | 1.90 % | same |
| `menu_m4_right` -> `menu_down2_m3` | -> 2941 | 1.90 % | same |
| `menu_down2_m3` -> `menu_m3_right` | -> 3104 | 1.90 % | same |
| `menu_m3_right` -> `menu_down3_m2` | -> 3267 | 1.90 % | same |
| `menu_down3_m2` -> `menu_m2_right` | -> 3322 | **identical** (max diff 0) | -- |
| `menu_m2_right` -> `menu_m2_right2` | -> 3431 | **identical** (max diff 0) | -- |

So direction keys are delivered and the entry region (the lower-right block of the 640x480 host frame --
where the entries and their toggle sub-rows are drawn: entry rows at `g_tab_531e0[m]` with dest 200 and the
toggle row at dest 0x100 / start_row 0xad) is redrawn on every step. The last two steps produce no change
at all.

## Open finding: the SOUND / MUSIC toggles do not change the enable bytes

The host pump line now carries the two enable bytes and the CD stop counter (`... snden=<DAT_00053338>
... cdstops=<sys_cd_audio_stop_calls()> sfx=<DAT_00051a10>`, added in `port/src/app/main.c`). Across the
whole toggle run they stay fixed:

```
snden=1 sfx=1 cdstops=2      (every pump line from frame 55 to frame 3539; cdstops last moved at frame 109)
```

per the routine's own description (0x00039c82-93 / the left-right handling): at `m == 3` a Left/Right press
should write `DAT_00053338` (music) and at `m == 2` write `DAT_00051a10` (effects) and either select the
room's CD track or stop the CD. Three Downs from `m = 0` walk `0 -> 4 -> 3 -> 2`, so the presses above
should have landed on both. Neither byte moved, and the last two steps did not even redraw -- which is a
concrete port-side bug candidate in `fn_039c03`'s toggle branch (the `f[2]` / `f[3]` mirroring at
0x00039c82-93 only *initialises* `f[]` from the bytes; the write-back path on Left/Right is what has to be
re-read against the disassembly, along with why the redraw stops after the third Down).

Filed, not yet fixed. It does not affect gameplay audio (the shipped defaults are on and the audio path is
measured working in docs/55 and docs/57); it means the menu's sound controls are not functional in the port.

---

## Correction and fix (same day): `fn_039c03` is the **in-game pause menu**, and three host input bugs were
## holding it shut

The section above probed the wrong screen. Adding an entry trace to `fn_039c03` and to `fn_038f3d` showed
**neither is ever called** on the attract path, so the static frame after the title card is some other
routine -- the routine's own caller settles the question: the room loop in `fn_03a829` reaches it only under

```c
if (g_dat_63e89 != 0 && pl[0x30] != 1) {          /* 0x0003aa9f-0x0003aac6 */
    while (g_dat_63e89 != 0) { if (!sys_port_spin()) break; }
    if (fn_039c03() == 0) return -1;
}
```

i.e. **ESC during gameplay**. That is the playtest report's "pause menu". Driving it that way
(`scripts/playtest/route_pause_menu.txt` = the anchored episode-1 route, then ESC) exposed three
host-side input defects:

1. **All four arrows wrote the same byte.** `key_apply` mapped `0x48/0x50/0x4b/0x4d` all to
   `DAT_00063ed0`, so pressing Down also triggered the menu's entry-walk branch. Now: `0x4b -> DAT_63ed0`,
   `0x4d -> DAT_63ed8`, `0x48 -> DAT_63ed3`, `0x50 -> DAT_63ed5` -- the pairing the ported consumers imply
   (`fn_038cde` scrolls with ed3/ed5 at 0x00038d7b/0x00038df0; `fn_039c03` walks entries with ed0/ed8 at
   0x00039d2a/0x00039d86 and toggles with ed3/ed5 at 0x00039de2).
2. **Arrow presses latched the "any key" bytes.** They set `DAT_00063eb2 / DAT_00063ebe`, which make
   `fn_039c03` leave on its first frame (its entry check spins until those bytes clear and then exits with
   `m = 0x64`, 0x00039ce6-d25). Arrows no longer touch them.
3. **`DAT_00063eb2 / DAT_00063ebe` were never cleared.** They were set on every key make and never on
   break, so any screen that waits for them to clear was wedged. They now follow the key (set on make,
   cleared on break).

Measured before/after, with a per-frame trace of the menu (`NIGHT_MENUBG=1` prints `m`, the four key bytes,
the auto-repeat counter and the five toggle flags):

| | before the fix | after the fix |
|---|---|---|
| `fn_039c03` entered | 1 (then left at once) | 1 |
| traced menu frames | **0** | **1098** |
| entry index values seen | -- | 0, 1, 2, 3, 4 |
| toggle branch observed | -- | `f[4]` flipping 1 -> 0 -> 1 at m == 4 |

With the presses placed on the right rows, the sound controls move the real enable bytes:

```
@frame 3757: snden=0        (MUSIC off, m == 3)
@frame 3921: snden=1        (MUSIC on again)
@frame 4138: sfx=0          (sound EFFECTS off, m == 2)
```

and the redraws are localized exactly where the routine composites them: entry changes alter rows 109..179 /
cols 200..311, the MUSIC ON/OFF sub-row alters rows 173..178, cols 256..311 (0.23 % of the frame), and the
frame after the final ESC differs from the paused one over 14.11 % of pixels (rows 2..186, cols 24..316) --
the game, unpaused. Screenshots: the run writes `pause_m0 ... pause_m2_hold, resumed`.

Timing note for any scripted navigation: the routine re-arms its auto-repeat counters at 0x13 frames, so a
700 ms hold steps the entry index two or three times (the trace shows `m` walking 0 -> 4 -> 3 -> 2 inside a
single hold). Short presses (~150 ms) step exactly one row and still fire the toggle branch, which only
needs its counter to reach 0 -- that happens on the second frame of a hold.

Still open for this screen: at `m == 2` the byte changes (`sfx=0`) but the row's ON/OFF glyph does not
(`pause_m2 -> pause_m2_off` is pixel-identical while `DAT_00051a10` flips) -- the entry is drawn from the
same sprite with `f[m] + 6 / f[m] + 8`, so the effects row may legitimately have one cell, or the index into
the `pause.spr` cells is off. Filed, not yet resolved. The original-side frame for this screen is still
blocked on the capture path (docs/46).

## Resolved: the EFFECTS row's ON/OFF glyph *does* change (the filing was a measurement artifact)

Filed earlier: at `m == 2` the enable byte `DAT_00051a10` flipped but the row's ON/OFF glyph looked
pixel-identical. The toggle-row composite (capstone over `0x00039fa7-0x0003a012`) shows the cell index is
`(f[m] << 2) + 0x18`, i.e. cells **6 and 7**, shared by both the sound (`m == 2`) and music (`m == 3`) rows --
identical to the port's `k = f[m] + 6` in dword units, and the same dereference convention
(`[spr+0x38] + [[spr+0x34] + idx]`) that was wrong in `fn_038445`.

To decide it by measurement instead of theory, `NIGHT_MENUBG=1` now also prints an FNV hash of the presented
frame over rows `0x64..0xbb` x cols `0xc8..0x13f` after the `0x0003a094-a9` present. Measured over the pause
route (`scripts/playtest/route_pause_menu.txt`, 1141 menu frames):

```
m=2 f=00101  h=16c13ce7e8c26b6d     (f[2] = 1)
m=2 f=00101 -> f=00001  h=c392c2b3c198bb86   ... hash changes with the flag, exactly as for m == 3
m=3 f=00011  h=d6acb1d7a43c5cf8
```

So the row repaints on both rows; there is no port-side defect. Regression pin added in
`test_real_res_container`: `pause.spr +0x38` is a heap offset, cells 6/7 (and the `m == 4` pair 8/9) resolve to
in-block offsets with non-zero width/row-count, and **cells 6 and 7 carry different pixel data** (memcmp over
the first 16 RLE bytes) -- if the ON and OFF glyphs ever collapse to one cell this fails.

## Byte-level proof of the pause-menu composite (docs/63 basis)

`NIGHT_MENUBG=1` now also prints, for each sprite cell the menu composites, the cell's raw row-RLE stream plus
the bytes the blit actually wrote on the back screen. `scripts/playtest_verify_spr.py` decodes that stream with
its own implementation of the shipped format (`fn_03bb42` mode 0 / `0x0003bb42`: per row a run count, per run
`colAdvance, runLen, runLen literal bytes`, column position accumulating `colAdvance + runLen`) and compares it
against what the port drew, at every column the cell covers. Over `scripts/playtest/route_pause_menu.txt`:

```
  entry    compared  179700  matched  179700  mismatched     0
  head     compared  234031  matched  234031  mismatched     0
  toggle   compared  134050  matched  134050  mismatched     0
  TOTAL    compared  547781  matched  547781  mismatched     0  -> 1.0000
```

So the menu header, every entry row and both ON/OFF toggle cells are exactly what the shipped `pause.spr` data
says, at the positions the port computes -- not merely "looks right".
