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
