# 391 — the 720x400 "boot" window in the original capture is the emulator's console, not the game

Docs/382 and docs/367 6 carried an unexplained line: the port matches only 8 of 24 frames in the original's `boot_`
window at 720x400, and "the port has no mode-3 presentation" was listed as a known gap.  Looking at the frames settles
it: there is no game content in that window.

## What the frames actually contain

The window is 24 frames, 1.30 s to 5.05 s.  Modes per frame: **15 frames at INT 10 mode 3** (text), one at mode 19,
then eight at **mode 257** (0x101, VESA 640x480).  The text frames read:

```
dosbox-automation 0.85.1 (7567ec4) [Webserver enabled]
https://dosbox-automation.org - command list: HELP, instructions: MANUAL
Cheat Workbench: type WORKBENCH or press Ctrl+Alt+W
Z:\>mount C /work/game
Local directory /work/game/ mounted as C drive
Z:\>imgmount D /work/game/TG_NIGHT.CUE -t iso
The IMG MOUNT command is deprecated; please use MOUNT instead.
MSCDEX installed.
CD-ROM image /work/game/TG_NIGHT.CUE mounted as D drive
Z:\>C:
C:>NIGHT
DOS/4GW Protected Mode Run-time   Version 1.97
Copyright (c) Rational Systems, Inc. 1990-1994
```

That is the capture harness's own autoexec echo (docs/377's container recipe, verbatim) followed by the **DOS/4GW
protected-mode loader** NIGHT.EXE runs under.  By 3.53 s the console has scrolled to two banner lines; at 3.69 s the
mode changes to 19 and at 3.87 s to 257, which is the game taking over the display.

## What that means for the objective's "boot" requirement

- The port does not present a text mode because **the original's text mode is not the game**: it is the emulator's
  shell and the DOS extender's banner.  Nothing in NIGHT's own assets draws at 720x400 -- the game's first presentation
  is the 640x480 logo card, which the port shows at 0.01 s (docs/389 2).
- The 8/24 "match" in that window was therefore always going to be junk: eight frames of a black-and-text console
  happened to correlate with the port's own black pre-logo frames.  It should not be quoted as accuracy in either
  direction.
- The remaining, honest scope statement about boot is: **the port starts where the game starts.**  The DOS-layer
  preamble (mounting a CD image, loading a 1994 DOS extender) is what the Pocket core replaces with data slots, not
  something to reproduce on a 320x240 LCD.
