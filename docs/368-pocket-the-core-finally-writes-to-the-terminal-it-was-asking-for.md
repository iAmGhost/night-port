# 368 — the core finally writes to the terminal it was already asking for

Task-12a's code half.  The host has long since derived a display mode from the menu's *Pocket LCD* option and the
`SCREEN=` key (`docs/350`, and the per-present re-assertion that stopped the port overriding the user 60 times a
second, `docs/336`), and `OF_DISPLAY_TERMINAL` is documented as "Terminal only" (`of_video.h:43`) with the console
being the thing `printf` writes to.  **The core never wrote a line to it.**  Every per-second fact went to
`NLOG` -- the card log -- and to `printf` only when `g_console_live` was set, which is `g_log_ok ? 0 : 1`: on the
device, where the log slot opens, console output is switched off by design.  So `SCREEN=1` produced an empty 40×30
console while the game carried on behind it.  That is the whole of what "Terminal was not honoured" could mean at the
code level, and it is why the earlier overlay experiments (0.1.7's `SCREEN=2`) were read as visual confusion: text was
appearing over the game from a mode nobody asked for, while the mode that *should* have had text had none.

## What ships now (0.1.55)

In the 1 Hz diagnostics block, after the `budget` line: a fixed **nine-line panel** drawn with `printf`, cleared and
homed each second (`\033[2J\033[H`) so the console shows *state* rather than the last 30 lines of a 3 000-line trail.
Rendered only when `g_display_mode == OF_DISPLAY_TERMINAL`, so the game screen is untouched in every other mode.

```
NIGHT 0.1.55  frame 872
view 320x200  sig b41d49f7 chg 12
ticks 490/s  frames 53/s  pit 490 Hz
present 721 us  max 1776 us
pal 255 lit  arena 3903168/16777216
cdda 558/19639 trk 4  sfx 2
disc 1760 reads 3 us avg  fails 0
ring ahead 1.59 s  dry 0  low 0
clock 1001640 us/s  pump 0
```

That is task-12a's readout list exactly: view, framebuffer hash (`sig`, plus the change counter), palette, present
cost -- and the budget numbers docs/364 added, because on a 320×240 LCD the interesting ones are the ones that fit.

The same panel is emitted on the desktop shim as a single `term:` line (`NLOG`, so it lands in the log too), which is
how the content and the 1 Hz cadence were verified without hardware.  Measured: a 900-frame stage-1 run prints 16
`term:` lines, one per second, e.g.

```
term: NIGHT 0.1.55 frame=872 | view 320x200 sig=b41d49f7 chg=12 | ticks=490/s frames=53/s pres=721us max=1776us
      | pal=255 arena=3903168/16777216 | cdda=558/19639 trk=4 sfx=2 | disc=1760 r 3us ring=1.59s
```

## What the next boot has to prove

The contract line is *Terminal text visible on the Pocket LCD during a run*, and only hardware can settle it.  The
card is staged for the test rather than for play: `SCREEN=-1` (follow the menu), `DIAG=1` (the peripheral read that
lets `SCREEN=-1` see the menu's Pocket LCD field, docs/350), `LOG=1`.

1. Boot 0.1.55 (elf md5 `69b189d701a46a5b329ea9df1e740d8f`, matching the build byte for byte).
2. In **Core Options**, set *Pocket LCD* → **Terminal** during a run and watch for the panel.
3. If nothing appears, two things are distinguishable from the log: `screen: ini/menu=-1 display_mode=0` (the mode
   was chosen) versus `pump ... dm=` staying at framebuffer (the peripheral field was not read), and `menu scr=-1`
   showing whether the mirror answered at all.

What remains unproven about terminal mode on this OS: whether the game keeps running behind it (the pump does, the
picture does not reach the LCD), and whether `OF_DISPLAY_OVERLAY` is worth keeping at all now that the terminal mode
has content of its own.
