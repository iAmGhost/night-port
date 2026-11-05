# 370 — the readout goes over the game picture: `OF_DISPLAY_OVERLAY`, and the SDK's own demo is the precedent

task-12a's remaining half is "Terminal text visible on the Pocket LCD during a run".  Until now the only way to get
it was *away* from the game: `SCREEN=1`, or Core Options → Pocket LCD → **Terminal**, which means switching the panel
on mid-run and losing the picture you were checking.  `of_video.h:45` has a third mode:

```c
#define OF_DISPLAY_OVERLAY     2  /* White terminal text over framebuffer */
```

The host already mapped `night.ini SCREEN=2` to it at boot (`main_of.c:2089`), but the 1 Hz panel was gated on
`g_display_mode == OF_DISPLAY_TERMINAL` only, so overlay mode selected the mode and then drew nothing -- the same
"mode chosen, nothing drawn" defect docs/368 fixed for the terminal, in the other mode.

## 1. The change, and the reference it copies

`main_of.c` now prints the nine-line panel in **either** text mode:

```c
if (g_display_mode == OF_DISPLAY_TERMINAL || g_display_mode == OF_DISPLAY_OVERLAY) {
    printf("\033[2J\033[H");            /* clear + home: a panel, not a scroll */
    ... nine lines ...
```

That shape is not invented here -- it is the SDK's own framebuffer demo: `openfpgaSDK/src/apps/fbdemo/main.c:48`
sets `OF_DISPLAY_OVERLAY`, `:89-90` and `:111-112` clear with `ESC[2J ESC[H` and reprint a short panel every cycle,
and `README.md:484` documents the console that printf reaches as a 40×30 CP437 text surface.  A panel that is
redrawn rather than appended, in a 40×30 console, over a live framebuffer: all three choices are the SDK's.

Why it matters for the hardware run: with `SCREEN=2` the game keeps moving and the diagnostics sit on top of it, so
one boot answers both halves of task-12a -- the picture proves the run left the logo, the panel proves the core is
writing to the text surface -- with no Core Options fiddling while the game is running.

## 2. What is verified here, and what is not

Verified on the desktop shim (`NIGHT_SCREEN=2`): the setting reaches the mode selection (`settings: ... screen=2`),
the panel's content is produced every second (`term: NIGHT 0.1.58 frame=382 | view 640x480 sig=83e2b683 chg=5 |
ticks=490/s frames=54/s pres=943us max=1265us | pal=256 arena=3903168/16777216 | cdda=365/17856 trk=3 sfx=2 |
disc=965 r 3us ring=1.59s`), and the run is normal (fixtures `== ALL PASS ==`).

Not verified, because the shim has no display-mode emulation at all (`pocket/src/sdk/pc/of_sdl2.c` never mentions
`of_video_set_display_mode`): whether openfpgaOS composites the text over the picture, where it places it, and
whether white CP437 stays legible over a bright slide.  Those are the device's answers, and they are the last thing
standing between task-12a and done.

## 3. The generated-ini trap, and the guard that now catches it

Editing `mk_dist.py`'s ini block by line insertion put **`SCREEN` twice** and then **`FB_MODE` twice** into the
shipped `night.ini` (my insert landed inside a Python string literal, then its repair duplicated a line).  A
duplicated key is a coin-flip about which value the parser honours -- an invisible setting bug, and the third
self-inflicted `mk_dist.py` breakage in two sessions (docs/369 §4 was a `SyntaxError`).

`scripts/pocket/check_slots.py` now refuses it:

```
  [PASS] night.ini names each of its 22 keys exactly once (SCREEN, FB_MODE, PUMP_AUDIO_TICK, DIAG, LOG tracked)
```

The rule it encodes: when a generator's output is a config file, check the *generated* file, not the generator's
source.

## 4. Card state

**0.1.58**, `night.elf` md5 `fa2641b12971cd6354944d2ed4e5471f` byte-identical to `pocket/.obj/night/app.elf`,
`core.json` 0.1.58.  `night.ini`: `SCREEN=2`, `DIAG=1`, `LOG=1`, `FB_MODE=1`, `PUMP_AUDIO_TICK=0`, 22 keys, no
duplicates, no `._` sidecars.  Fixture suite `== ALL PASS ==`; `src/f` untouched.

What a boot now needs to do: press Start, play for a minute, eject.  If white text appears over the picture, the
overlay half of task-12a is answered without any menu switch; `Saves/night/common/nightlog.sav` answers the numbers.
`SCREEN=1` remains available if the overlay turns out illegible over a bright slide.
