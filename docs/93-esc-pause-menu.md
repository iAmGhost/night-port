# 93 — The ESC "pause & options" screen: the host quit instead of writing the abort byte; the shipped path is the room-loop latch at 0x0003aa9f -> fn_039c03 (0x00039c03), which was already ported (docs/60)

Playtest log item "ESC key exits game: Where's pause menu?".

## 1. Root cause

The SDL host mapped ESC to BOTH the guest abort byte and its own `g_quit`
(`case SDL_SCANCODE_ESCAPE: sc = 0x01; g_quit = 1; break;` in src/app/main.c). The guest side was already
correct end to end:

- keymap (docs/92) writes the held byte DAT_00063e88[0x01] = **DAT_00063e89** on make and clears it on
  break -- exactly the shipped IRQ9 behaviour (0x0003fe06 / 0x0003fe6e);
- the room loop unwinds it at the shipped latch 0x0003aa9f-0x0003aac6: `g_dat_63e89 != 0 && player[0x30]
  != 1` -> spin while the byte is held (break clears it) -> `fn_039c03()`; returning 0 leaves the room,
  returning 1 resumes it;
- `fn_039c03` (0x00039c03, THE PAUSE & OPTIONS LOOP, drawn from "pause.spr") was ported and traced in
  docs/60: each frame ENTER/SPACE/CTRL/ALT (DAT_00063ea4/0x63ec1/0x63ea5/0x63ec0) confirm the current
  entry; ESC/LSHIFT/RSHIFT (DAT_00063e89/0x63eb2/0x63ebe) spin-until-clear and leave with m = 0x64
  (0x00039ce6-d25); Up/Down walk the five entries by reading DAT_00063ed0/0x63ed8 (0x00039d2a/0x00039d61);
  Left/Right toggle entries 2/3/4. Entry 0 = LEVEL UP MODE (status screen fn_00038f3d v0), 1 = COMMAND
  LIST (help screen fn_00038cde), 2 = SOUND MODE, 3 = BGM MODE, 4 = EXIT TO DOS.

So only the host-side `g_quit` made ESC exit the game. The fix is one line: ESC writes scancode 0x01 and
nothing else.

## 2. Measured on the original (container, live episode-1 room)

- ESC sets DAT_00063e89 while held and clears on break (probe_orig_esc2.txt readbytes at 0x63e80).
- ESC in the live room opens the pause & options panel -- purple overlay, five rows
  "LEVEL UP MODE / COMMAND LIST / BGM MODE / SOUND MODE / EXIT TO DOS", red '>' cursor (probe_orig_esc3
  frames 0012 room_settled -> 0013 esc_after_6s: the panel appears; the room image behind it freezes,
  which IS the pause).
- ENTER confirms the cursor entry on the original too: a later probe frame shows the LEVEL UP MODE status
  screen (TECHNICAL LEVEL / H.P MAX / ... / CONTINUE +08 with "EXIT:ESC HELP:F1" footer) -- fn_00038f3d
  variant 0, reached by Enter with the cursor on entry 0.
- The container session's world had stopped animating before the menu (identical frames) -- the
  dummy-audio dosbox build never services the guest DAC the frame pacer waits on; captured with
  SDL_AUDIODRIVER=alsa + ALSA null sink afterwards as well. Cursor-walking via Up/Down could NOT be
  demonstrated on the original in this container (arrow held bytes 0x63ed8 DO get set while the menu is
  open -- probe_orig_pause3 -- but the frames never redrew). Flagged, not guessed; the port's behaviour
  is the shipped loop itself (docs/60 byte-level trace), and that loop demonstrably reacts to ENTER on
  the original.

## 3. Port comparison (scripts/playtest/route_pause_full.txt + route_pause_space.txt, SDL_VIDEODRIVER=dummy)

ESC in the live room -> `fn_039c03 enter` trace -> panel shot pixel-matches the original capture
(panel crop 12825 px, 346 differing = the cursor/AA, same rows/colors/layout); Down moves the cursor
LEVEL UP MODE -> COMMAND LIST (shot pause_menu_down); Enter on COMMAND LIST opens help10.pcx (the
command-list help page, fn_00038cde); ESC leaves and the room resumes (shot room_after_esc, no panel,
entities live); SHIFT opens/closes the same way (the shipped exit group includes DAT_00063eb2); SPACE at
entry 0 opens the LEVEL UP MODE status screen (res=help1.pcx chain, fn_00038f3d v0).

## 4. Reproduce

```sh
# original: container per AGENTS.md, live ep-1 room (docs/91 nav), then
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_esc3 --drive scripts/playtest/probe_orig_esc3.txt --hz 1   # ESC opens the panel
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_pause3 --drive scripts/playtest/probe_orig_pause3.txt --hz 1  # arrow bytes set
# port:
cd port && SDL_VIDEODRIVER=dummy ./test/night --script ../scripts/playtest/route_pause_full.txt \
  --shots /tmp/dbgauto/port_pause2 --no-audio --frames 13500
SDL_VIDEODRIVER=dummy ./test/night --script ../scripts/playtest/route_pause_space.txt \
  --shots /tmp/dbgauto/port_pause3 --no-audio --frames 13000
```

## 5. Open

- Original cursor-walk under container capture (see §2, arrow bytes set but no redraw observed; most
  plausibly the DAC-paced frame wait starved by the emulated audio path, same subsystem the SFX/music
  tasks touch -- revisit there).
- NUMLOCK: a fresh boot with numlock ON puts the TITLE at mode 19 while the docs/91 probe expects the
  VESA-480 title; README.TXT explicitly warns "NUMLOCK OFF before running". Probe nav files should
  assert the numlock state at boot.
