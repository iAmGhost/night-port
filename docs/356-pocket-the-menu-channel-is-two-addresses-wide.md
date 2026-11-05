# 356 — the menu channel is two addresses wide: `0xF700000C` and `0xF7000010`, and nothing else

Core 0.1.45 ran the experiment, core 0.1.46 ships what it decided.

## 1. The experiment and its answer

0.1.45 declared three entries **out of address order** so that declaration order and address could be told apart:

| declared | entry | bridge address | mirrored into |
|---|---|---|---|
| 1st | Music Volume | `0xF700000C` | `0x4000006c` ✓ claimed at t=45.7 s |
| 2nd | SFX Volume | `0xF7000014` | **nothing** |
| 3rd | Screen Fit | `0xF7000010` | `0x40000068` ✓ claimed at t=89.4 s |

```
menu: Music Volume slot CLAIMED t=45.7s @0x4000006c the word hardware carried it in on (value 47 = 7/8)
menu: Screen Fit has no peripheral slot after 60 s -- this OS mirrors only some of the app's entries ...
menu: SFX Volume has no peripheral slot after 60 s -- this OS mirrors only some of the app's entries ...
menu: Screen Fit slot CLAIMED t=89.4s @0x40000068 the word hardware carried it in on (value 52 = stretch)
census t=106.2s: 2 candidate(s) of 96 registers; claimed: Music Volume, Screen Fit
```

Screen Fit, declared **last**, was mirrored.  SFX Volume, declared **second**, was not mirrored anywhere at all — the
whole page at the end of the run:

```
periph t=118.2s 0060: 00000000 00000000 00000053 00000046 00000000 00000000 00000000 00000000
                     0x60    0x64    0x68    0x6c    0x70    0x74    0x78    0x7c
                               fit=scaler  music=3/4      ^ and 0x64, where a descending slot table would have put it, is 0
```

So the rule is **the address**, not declaration order, not a slot table: exactly two bridge words are mirrored, and the
word below them in the page (`0x40000064`) is not one of them.

## 2. The channel map, as shipped

| bridge address (`interact.json`) | peripheral word the app reads | status |
|---|---|---|
| `0xF700000C` | `0x4000006c` | mirrored — seen in 0.1.42 (`1..4`), 0.1.43 (`0x41..0x48`), 0.1.45 (`0x41..0x48`) |
| `0xF7000010` | `0x40000068` | mirrored — seen in 0.1.42 (`5..8`), 0.1.43 (`0x51..0x53`), 0.1.45 (`0x51..0x53`) |
| `0xF7000014` | — | **not mirrored**, twice confirmed |
| `0xF7000000` | — | unreadable from an app (0.1.37 hangs the core) and fatal to mount an element on (0.1.27, 0.1.34) |

`0xF7000004`/`0xF7000008` (the reserved Video H/V offset sliders) are mirrored into `0x40000084`/`0x40000088` — 0.1.38
saw them move — but those belong to openfpgaOS's scaler and moving them moves the picture, so they are not settings
this port may borrow.

**Two addresses means two menu settings, and the port has to choose which two.**

## 3. Core 0.1.46's choice

| setting | Core Options | `night.ini` | during play |
|---|---|---|---|
| **Music Volume** (8 levels, `Off` silences the CDDA) | `0xF700000C` → `0x4000006c` ✓ proven | `MUSIC_VOL` | — |
| **SFX Volume** (8 levels) | `0xF7000010` → `0x40000068` ✓ proven | `SFX_VOL` | — |
| Screen Fit (letterbox / stretch / scaler) | — | `FIT` | **L1+B cycles it** |

Sound effects had no route at all before this: `night.ini` only, which a player never sees.  Panel fit already has a
control the player can reach *while playing* — L1+B, verified on hardware in docs/334 and again by the user in this
session — so it is the one of the three that can leave the menu without becoming unreachable.  The swap is one entry
in `mk_dist.py`'s `APP_MENU_SETTINGS` table: put Screen Fit on `0xF7000010` and drop SFX Volume, and the menu becomes
Music + Fit again.

An entry that is not declared is also now inert in the host: `FIT_LIVE` is 0 when `interact.json` has no `Screen Fit`,
its row is skipped by `menu_slots_scan()`, and it can never hunt for a word and mistake a counter for a menu that does
not exist.

## 4. What is finished here, and what is not

Finished: the port has two live, verified, hardware-visible settings, applied through the same
`OF_SET_VOL` / `OF_SET_SFX` path that the SDL host's own volume plumbing uses, with `census` lines naming what the core
believes it controls.  The settings thread that cost 0.1.27 → 0.1.46 — two cores that never booted, one core that hung
on a register read, one that answered junk for six builds — is closed with a working surface and a measured reason for
its size.

Still open, and unchanged by any of this:

* `of_interact_get()` is unusable on this OS build (fixed junk at every index, seven builds).  The peripheral page is
  the channel; the SDK's documented settings API is not.
* *Why* those two words specifically is unexplained.  Three runs pin down the map, none explains it.
* `FB_MODE` (native 640×480 vs host-downscaled) is still unmeasured on hardware, and it is the last performance lever
  in the port: 640×480 presents mean 21,344 µs against 320×200 mean 3,608 µs (docs/352).  It cannot become a menu
  setting — both mirrored words are taken — so it stays `night.ini`, and measuring it needs one card cycle with
  `FB_MODE=0` set.
* `DIAG` is again the developer's instrument: the shipping value 0 switches off the 96-word scans, the analogizer
  probe and the interact census.  The settings path is not part of that switch — it reads its two claimed words every
  100 ms, and runs a 96-word sweep at 2 Hz only while some entry still has no slot, which is why the sweep stops once
  the map is known (docs/354 §4).
