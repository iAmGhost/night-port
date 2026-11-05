# 362 — packing confirmed on hardware, and what the core ships with

0.1.52 was the experiment: three menu entries declared at one bridge address `0xF700000C` with disjoint masks, and no
`writeonly` flag (docs/360 predicted the two possible outcomes).  The card log (`scratch/pocket/log_0152.sav`, 4 096
lines, `DIAG=11`) answers it in one word.

## 1. One word carried all three settings at the same time

```
menu: Music Volume slot CLAIMED @0x4000006c the word hardware carried it in on (word 00000305, field 5 = 5/8 at bits 0-3)
menu: Screen Fit  slot CLAIMED @0x4000006c the word hardware carried it in on (word 00000305, field 3 = scaler 320x200 at bits 8-9)
menu: SFX Volume  slot CLAIMED @0x4000006c the word hardware carried it in on (word 00000388, field 8 = Full at bits 4-7)

menu: Screen Fit = stretch (field 2 of word 00000288) applied: panel fit      <- same word: music Full, sfx Full, fit stretch
menu: Music Volume = Full  (field 8 of word 00000288) applied: music volume
menu: SFX Volume  ...        word 00000288
census t=45.4s: 10 candidate(s) of 96 registers; claimed: Music Volume, SFX Volume, Screen Fit
```

`0x288` = `music 8 | sfx 8 | fit 2`, read back as three independent fields, and `0x281` (music Off, sfx Full, fit
stretch) shows a field changing while the others stayed.  So:

- **the OS does read-modify-write at a bridge address the bitstream never decodes**; the read half is served by the OS's
  own shadow copy, not by the core, which is why `of_interact_get()` being dead (docs/342) never mattered;
- **`writeonly` was the whole cause of 0.1.47's whole-word writes** (docs/358 §1 measured it, this build disproves the
  conclusion drawn from it);
- **packing is the documented shape after all** — openfpgaOS's own `interact.json` packs six variables into
  `0xF7000000` exactly this way, and now our core does too.

`LAYOUT = "packed"` in `scripts/pocket/mk_dist.py` is the shipping shape.  `"proven"` (docs/356's one-setting-per-word
layout) is kept in the file as the fallback if a future OS build stops composing.

## 2. Persistence, which was still unproven, is proven by the same log

At `t=9.3 s` — before the player touched anything — `0x4000006c` already read `0x305`, the *previous* session's picks.
The firmware's own store shows the end of this session:

```
Settings/iamghost.night/Interact/_core/interact_persist.json
  id 130 (Music Volume) val 7   -> option index 7 = Full   (0x08)
  id 131 (SFX Volume)   val 7   -> option index 7 = Full   (0x80)
  id 132 (Screen Fit)   val 1   -> option index 1 = stretch(0x200)
                                     composed: 0x288, which is the last word the log saw  ✓
```

The store holds the option **index**, not the wire value, which is why the decode is `field → option` and not a raw
number.  Settings survive an eject and a reboot without the menu being opened.

## 3. The second address is free

`0xF7000010 → 0x40000068` carried a setting only while a knob needed it (docs/356).  With three knobs in one word it is
unused, and `0x40000068` was seen carrying nothing in this run.  A fourth menu setting is one `APP_MENU_SETTINGS` line
and a mask away — the standing candidate is the framebuffer mode (`FB_MODE`, the only unmeasured performance lever:
native 640×480 presents cost mean 21 344 µs vs 3 608 µs at 320×200, docs/343).  Not added here: this commit wraps up,
it does not extend.

## 4. What ships in 0.1.53

| night.ini | value | why |
|---|---|---|
| `MUSIC_VOL` `SFX_VOL` `MASTER_VOL` | 255 | the chain fix and the measurement (docs/361) |
| `SFX_GAIN=200` `CDDA_GAIN=150` | knee gain | the game's own samples are quiet; the disc is not (docs/361 §3) |
| **`FIT=1`** | **stretch** | fill the 320×240 panel by doubling 40 of the 200 rows.  `0` is the letterboxed 320×200 with 20+20 black bars, `2` asks the core scaler for its 320×200 mode.  The menu knob and L1+B change it live; `0x200` became the menu entry's own `defaultval` too, so the menu and the ini no longer disagree about the default |
| **`LOG=0`** | **file logging off** | `Saves/night/common/nightlog.sav` is a 256 KB diagnostic file rewritten every eight lines.  It is how this port was measured (docs/337 → docs/362), and a player has no use for it. `LOG=1` restores the exact same trail |
| `DIAG=0` | no MMIO probes | unchanged since docs/348 |
| `SCREEN=0` `FB_MODE=1` `TICK_HZ=490` | as measured | unchanged |

The desktop build (`app_pc`, `-DOF_PC`) **keeps logging on**: the rig and the differential tests read
`scratch/pocket/night.log` as their witness, and `NIGHT_LOG` overrides either default.  Verified on the PC build:

```
settings: music_vol=255 sfx_vol=255 cdda=1 pad=1 tick_hz=0 screen=0 file_log=1
fit=1 (0 = 1:1 with 20+20 black bars, 1 = host row stretch, 2 = ask the core scaler for 320x200)
```

The stale log was archived to `scratch/pocket/log_0152.sav` and removed from the card, so the deployed tree is a player
build: 0.1.53, `MUSIC_VOL=255 SFX_VOL=255 MASTER_VOL=255 SFX_GAIN=200 CDDA_GAIN=150 FIT=1 LOG=0 DIAG=0 SCREEN=0
FB_MODE=1`, elf md5 `12cdd9543dd99ad1323f7d9386594808` byte-identical to the build.  `check_interact.py` ALL PASS, the
fixture suite ALL PASS, `src/f` untouched.

## 5. Where the port stands

Working on hardware, measured in the logs above: boot and logo, the `.fli` intro, the title, the 640×480 slides, the
stage and gameplay modes, SFX through the game's own DSP transfer census (`sfx=350/0/0`, no dropped blocks), CDDA
through 14 repacked track slots (`cdda=7797/19639/6`, gapless), the game's own save slots, the fixed-step tick
(`ticks=130013 (+501/s)` against a 490 Hz program), and three live menu settings that persist across boots.

Open, in the order I would take them:

1. `FB_MODE=0` has never been measured on hardware, and it is now *menu-able* (the freed `0xF7000010`);
2. DSP commands `0xe1` and `0xf2`, one sighting each in the whole census, still unexplained;
3. the SDK's `os.bin` (134 424 B) is older than the one openfpgaOS-Doom ships (135 400 B) — a newer OS might make
   `of_interact_get()` answer, which would retire the peripheral-mirror detour entirely;
4. the analogizer/SCNN route for a TV (docs/336's reserved variables are declared, never exercised on a TV).

The settings thread that ran 0.1.27 → 0.1.53 is closed: **Core Options work, all three, in one bridge word, with
persistence.**

## 6. Playability status, from the device (reported by the user, 2026-11-06)

While this goal was paused the port got to the point where **it is very playable on the Pocket** — the game is played
on hardware, start to the stage modes, with the menu settings behaving.  That is the outcome the objective asks for, and
it is consistent with everything the card logs measured independently: `view=1 320x200` with the frame signature
advancing on every sample, `pres=3656us` present cost, `pit=490 Hz` with `ticks=130013`, `sfx=350` blocks delivered and
`cdda=7797/19639/6` sectors consumed, arena `3968768/16777216`, no crash and no watchdog reset across a 4 096-line run.

What that does *not* yet prove, and what the remaining tasks are for: that **all nine episodes** are reachable on this
code path (task-8, in progress), the level-up/continue screens on the Pocket path, `FB_MODE=0`'s cost on hardware, and
the stage palette divergence docs/333 recorded (the port's stage frames read `255/97/97` on both hosts while the DOSBox
original reads a grey-blue station scene — a shared-`src/f` defect inherited, not introduced).
