# 341 — the menu knobs only moved at boot, and the game's own Music Mode was never mirrored

Core 0.1.25.  Three reports: Music Volume does nothing (and should be live mid-game), the game's own **Music
Mode → OFF** does nothing, and Tick Pump Hz sits on 60 and cannot be moved.

## 1. What the card had actually stored

`Settings/iamghost.night/Interact/_core/interact_persist.json`, written by the firmware after the previous run:

```json
{ "id": 15,  "type": "slider_u32", "val": 0 },   ; Video H Offset
{ "id": 16,  "type": "slider_u32", "val": 0 },   ; Video V Offset
{ "id": 100, "type": "slider_u32", "val": 0 },   ; Music Volume   (interact.json defaultval: 200)
{ "id": 101, "type": "slider_u32", "val": 60 }   ; Tick Pump Hz   (interact.json defaultval: 490)
```

An untouched slider carries **0** in the interact bridge, and the UI then clamps the display to the slider's
minimum — which is exactly why *Tick Pump Hz reads as "fixed on 60"*: its wire value is 0, its `min` is 60.

That 0 is also the reason Music Volume was unreliable: the host's rule was *"only apply a menu value that
differs from its defaultval"*, because 0 is a legal volume (mute) and the port could not tell "the user asked for
silence" from "nobody ever touched this".  And the whole read happened **once, at boot** — `of_settings_load()` —
so a knob moved inside the game had no path to the host at all.

## 2. Sliders → named lists

Both knobs are now `list` variables, with no 0 in the value set (except the deliberate "Game rate" for the tick
cap), and `of_save.c` accepts **only values it can name**:

```c
static const int MUSIC_OPTS[] = {   1,   64,  128,  192,  255 };   /* Off, Low, Medium, High, Full */
static const int TICK_OPTS[]  = {   0,   60,  122,  245,  490 };   /* Game rate, 1/8, 1/4, 1/2, full */
```

An unwired 0 is now simply not an option, so it cannot mute the music, and a value the menu can show is a value
the host agrees to act on.  `night.ini` ships `MUSIC_VOL=192` so the ini and the menu's default (High) agree
until the menu is moved.  `deploy.sh` deletes the core's own stale `Settings/…/Interact` tree on the way in:
`Tick Pump = 60` was a legal cap (8× slow motion) left behind by the broken sliders.  The owner's input remap
(`Input/_core/`) is untouched.

## 3. Settings are polled, and volume goes onto the voices

`of_settings_poll()` re-reads the bridge; `main_of.c` calls it once every 100 ms from the tick pump and applies
whatever moved:

```
settings: music_vol=128 applied from the menu
settings: tick cap=245 Hz (0 = the rate the game programs)
settings: pocket lcd raw=00020000 scr=1
```

`of_cdda_set_volume()` no longer just stores a number for the next track to pick up — it applies the level
immediately, to the group **and** to each looping voice:

```c
of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, g_vol);
of_mixer_set_vol_lr_h(g_voice[0], g_vol, 0);
of_mixer_set_vol_lr_h(g_voice[1], 0, g_vol);
```

The per-voice call matters: group volume alone did not change what the device played, and
`of_mixer_set_vol_lr_h` is the level API openfpgaOS-Doom actually drives (`i_sdlsound.c:197`).  The two CDDA
voices also stop being born at a hard-coded 255 (`of_cdda.c` voice_start now uses `g_vol`).

## 4. Tick Pump Hz: explained, then taken out of the menu (0.1.26)

The host feeds the game its ticks at the rate the game itself programs — PIT channel 0, 0x982 → **490 Hz**
(`fn_03ef29`, see docs/335).  The knob is a **cap** applied in `tick_pump()`:

```c
int cap = of_setting_tick_hz();
if (cap > 0 && cap < (int)hz) hz = (uint32_t)cap;
```

It can only slow the clock below what the game asked for, never speed it up.  Asked whether that is worth a
place in the player's menu — it is not, and it was removed in 0.1.26, for a reason that is worth writing down:
this port's waits are **clock deadlines**, not tick counts (docs/335 is exactly about that).  So cutting the
tick rate to 60 Hz does not give a faithful slow motion, it gives a *different game*: every wait that expires on
the clock still expires in the same real time, while the animation driving it receives an eighth as many ticks.
The 0.1.17 logo freeze was that failure mode with the host's own blit as the cause.

It survives where a developer reads it, and only there: `night.ini` `TICK_HZ=…` on the card, or
`NIGHT_TICK_HZ=…` on the desktop.  `of_setting_tick_hz()` and the cap in `tick_pump()` are unchanged; the menu
variable, its `TICK_OPTS` whitelist and the poll branch that fed it are gone, and the Core Options menu is now
Pocket LCD + Music Volume.

## 5. Music Mode OFF: the game's flag was never mirrored

The game's own menu item is the byte **DAT_00051a10**.  It gates CD-DA selection (`fn_10b1d` at f.c:2459, and
docs/114's audit of the shipped code: `if (DAT_00051a10 != 0 && idx < DAT_00053c60)`), and the pause/title menu
mirrors it into its option bytes (docs/22).  In the original, turning it off is silence because **the music only
exists as the reads the game issues** — no reads, no sound.  This port plays CD-DA through two *looping* mixer
voices that keep looping until something stops them, so "Music Mode OFF" changed nothing.

Now the host mirrors the flag:

```c
static int cdda_allowed(void) { return !(g_off & OFF_CDDA) && g_music_mode; }
static void music_mode_mirror(void) { ... }        /* off -> of_cdda_stop(); on -> replay the last request */
```

Turning it back on re-plays the same track (`g_cdda_last_index/lba/sectors`) instead of waiting for the next
scene's selection, and the periodic log line carries the byte so a card log can confirm it: `music_mode=1`.

Two separate switches, deliberately: `CDDA_OFF` in `night.ini` (the owner's) and the game's own mode byte.  The
ini switch is not cleared by the game turning music back on.

Worth recording while it is fresh: `f.h` labels `g_dat_51a10` "sound-effect enable flag" and `g_dat_53338`
"sound/music enable byte", while docs/04 and docs/39 have **53338** gating SFX playback (`fn_04185e`) and
docs/114 has **51a10** gating CD track selection.  The two flags are swapped in those header comments; the
behaviour the code shows is the docs'/docs-114 version.  Left as-is in `src/f` (fixture-tested), noted here.

## 6. Screen choice follows the menu

`night.ini` now ships `SCREEN=-1` — follow the Pocket LCD variable (On / Off / Terminal, bits 16-17 of
0xF7000000) — instead of `SCREEN=0`, which silently pinned the display and made the menu's Terminal choice inert.
`SCREEN=0/1/2` still pins it for a card-only diagnostic run.

## 7. Verified / not yet verified

Verified on the desktop shim: builds clean, fixture suite `ALL PASS`, a 2000-frame run starts with
`music_vol=255 cdda=1 tick_hz=0 screen=0` (menu unwired → the ini defaults win, as intended).

Not yet verified on hardware: that a menu volume change is audible **during** a track, that Music Mode OFF goes
silent at once and back on restores the track, and that the named lists move in the Pocket's Core Options menu.
