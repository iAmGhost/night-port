# 349 — an app may not read the APF status page: `0xF7000000` never returns

Core 0.1.38.  Card log, 0.1.37 with `DIAG=15` — 22 lines, and this is all of it:

```
log: opened "save:7" (the SDK save alias)
root: "os.bin" ino=2 slot=1 … root: "nightlog.sav" ino=18 slot=17
root: 26 entries; slot 17 found
core 0.1.37: boot marker
phase: boot start
phase: display claimed
night pocket: cpu=100000000 Hz sdram=65536 KB heap=35986 KB fb=320x240 stride=320 voices=32 variant=0
caps: version=4 hw=01a7e35b os=00000001 (sdk OF_CAPS_VERSION=4)
probe: control-word read begin
```

That is the last line the core ever wrote.  No `probe: control-word read done`, no `phase: clock source installed`,
no `board_boot`, no game — and the player watched a frozen Pocket.  The bracket did its job: the log names the
statement that never returned.  (It also outran the `diag=…` summary, which `0.1.36` printed *after*
`of_settings_load()` — the probe was inside `of_settings_load()`.  In 0.1.38 nothing probes during settings load, so
the summary necessarily precedes every probe.)

## What it means

`0xF7000000` is the APF **status** page: the core writes it for the host (Analogue firmware) to read.  Reading it
from the SoC side — from an openfpgaOS app — is not a supported operation on this bitstream, and it does not fail
cleanly: it hangs.  That is why no openfpgaOS app reads it.  Note what the SDK's `interactdemo` reads instead:
`0x400000xx`, **openfpgaOS's own periph page**, which is a different page and demonstrably app-readable.

This also quietly retires the theory that motivated the read.  The theory was: "Pocket LCD moves the panel, so the
value must be sitting in a word the app can read."  0.1.36 disproved the need: **Pocket LCD works with the host
reading nothing at all.**  The setting is applied by the core; the app was never in the loop, and it does not need
to be.  The only thing the host ever used that word for is `SCREEN=-1` (follow the menu), which is a nicety.

## 0.1.38

* `bridge_control_word()` is **deleted**, not gated.  There is no `DIAG` bit and no ini line that can bring it back.
  Its epitaph sits in `of_save.c` where the function was, so the next reader does not reintroduce it.
* `of_settings_poll()` returns 0 and says why: every setting this host applies comes from `night.ini` at load time;
  the interact ecall is not a substitute, because three builds of card logs showed it returning a fixed table of
  junk for every index — including the index whose menu entry demonstrably moves the panel.
* The `menu word=…` per-second field is gone (it could only ever print zero now).
* `OF_DIAG_BRIDGE` (bit 8) is marked **retired** in `of_save.h` rather than removed, so an old `night.ini` that
  still carries it is accepted and ignored instead of meaning something else.
* The old poll body is preserved under `#if 0` with the reason attached, so the shape of the abandoned experiment
  is still readable.

Settings a player can change, all of it independent of the bridge: `night.ini` (`MUSIC_VOL=`, `SFX_VOL=`, `FIT=`,
`SCREEN=`, `CDDA_OFF=`, `PAD_OFF=`, `TICK_HZ=`, `DIAG=`) and **L1 + B** for panel fit, live.  That is now the
documented surface, not a fallback.

## What is still open, and it is one boot wide

The periph page at `0x400000xx` is the only channel proven app-readable on this core: 0.1.35's 96-word scan
completed, and `0x40000098` read `01a7e35b`, matching `caps->hw_features` from the same run exactly.  Whether any
*menu* value lands there is still unmeasured, and the card is set to `DIAG=11` (periph scan + analogizer +
interact census, no control-word read) for exactly that question:

```
boot → Core Options → move Pocket LCD a few times, move Video H/V Offset a couple of steps → play ~30 s → eject
grep -nE "phase: probe|periph: baseline|periph move" nightlog.sav
```

| outcome | meaning |
|---|---|
| a `periph move @0x4000xxxx` after the baseline line, timed to a menu change | openfpgaOS mirrors menu state into a page the app can read; NIGHT settings can be tracked there without declaring any interact element |
| the scan completes with no movement | no menu state reaches the app by any route; `interact.json` app elements are fatal (the control word) or inert (everywhere else), `of_interact_get()` answers junk, and the control word is unreadable — which closes the menu question completely |
| it hangs on some other `begin` | that probe is retired the same way, and the periph measurement is retried with `DIAG=1` alone |

## The rule this buys, written down

Three hardware facts, each bought with a dead or frozen core, now encoded in the port instead of remembered:

1. **An app element in `interact.json` at `0xF7000000` stops the core from booting** (0.1.27, 0.1.34) — enforced by
   `check_interact.py`.
2. **An app read of `0xF7000000` hangs the core** (0.1.37) — the read no longer exists in the source.
3. **`of_interact_get()` carries nothing** on this OS build (0.1.28, 0.1.29, 0.1.32, 0.1.36) — so no setting in this
   port depends on it.

Everything the port now reads about the platform comes from `of_get_caps()`, `of_video_get_mode()`, and —
diagnostically — the periph page.
