# 359 — both volumes work on hardware; the settings thread (0.1.27 → 0.1.49) is closed

The card confirms the fix in docs/358.  One boot, both menu entries claimed on the first sample of their known mirror
words, and every applied value matches the option that was picked:

```
menu: Music Volume slot CLAIMED t=12.7s @0x4000006c the word hardware carried it in on (word 00000048, field 72 = Full at bits 0-7)
menu: SFX Volume   slot CLAIMED t=12.7s @0x40000068 the word hardware carried it in on (word 00000065, field 101 = 5/8 at bits 0-7)

menu: Music Volume = 1/4  (field 67 of word 00000043) applied: music volume      settings: music_vol=72
menu: Music Volume = Full (field 72 of word 00000048) applied: music volume      settings: music_vol=255
menu: Music Volume = 1/2  (field 68 of word 00000044) applied: music volume      settings: music_vol=109
menu: SFX Volume = 1/2 (field 100 of word 00000064) applied: sound-effect volume settings: sfx_vol=109
menu: SFX Volume = Full (field 104 of word 00000068) applied: sound-effect volume settings: sfx_vol=255
menu: SFX Volume = 3/4 (field 102 of word 00000066) applied: sound-effect volume settings: sfx_vol=182
menu: SFX Volume = 1/2 (field 100 of word 00000064) applied: sound-effect volume settings: sfx_vol=109

census t=45.1s: 0 candidate(s) of 96 registers; claimed: Music Volume, Screen Fit
```

No `found by field cleanliness`, no `slot lost`, no counter claims, no mute that nobody asked for.  `0 candidate(s)` is
the healthy census: with the hunt closed off for entries whose mirror is known, nothing else in the page is even a
candidate.

## What closed it

The last three things, in order:

1. **docs/356's map** — only `0xF700000C → 0x4000006c` and `0xF7000010 → 0x40000068` are mirrored, and it is the
   *address* that decides, not declaration order.
2. **docs/358 §1** — packing into disjoint mask fields is dead: the OS writes the whole word, so the two wired words
   carry two settings and no more.
3. **docs/358 §3** — an entry whose mirror address is known may only be read there.  The hunt (clean + ever-silent +
   held an option value) is left only for entries whose address is unknown, and a known word that never answers is
   reported rather than replaced by a register that merely looks menu-shaped.

## Final settings surface

| setting | Core Options | `night.ini` | during play | hardware |
|---|---|---|---|---|
| Music Volume (8 levels, `Off` silences CDDA) | `0xF700000C` → `0x4000006c` | `MUSIC_VOL` | — | **verified 0.1.48/0.1.49** |
| SFX Volume (8 levels) | `0xF7000010` → `0x40000068` | `SFX_VOL` | — | **verified 0.1.48/0.1.49** |
| Screen Fit (letterbox / stretch / scaler) | — (no third word exists) | `FIT` | **L1+B cycles it** | verified 0.1.45 |
| Pocket LCD | shipped entry, `0xF7000000` bits 16–17 | `SCREEN` | — | verified 0.1.39 |
| CDDA on/off, `TICK_HZ`, `BLIT_SKIP`, `PRESENT_SYNC`, `FB_MODE`, `DIAG` | — | yes | — | ini path verified |

`night.ini` stops being the player's settings surface and goes back to being the developer's instrument.

## Two small things this run exposed

**The census line was hardcoded.**  `census … claimed: Music Volume, Screen Fit` — names baked into the `NLOG` call for
`g_menu[0]` and `g_menu[1]`, which named a table order that 0.1.48 had already changed (Music, SFX, Fit).  The core was
describing a different core.  It is now built from `g_menu[k].what` in a loop.  A log line that lies about the code is
worse than no log line, and this one sat in front of every census since 0.1.44.

**The simulator reads the deployed declaration now.**  `sim_menu.py`'s replay used to hardcode which entries a log
belonged to; it builds them from `pocket/dist/night/**/interact.json` instead, and prints the claims the log itself
records next to the model's:

```
the card actually claimed: Music Volume@4000006c t=12.7s (the word hardware carried it in on),
                           SFX Volume@40000068 t=12.7s (the word hardware carried it in on)
t=9.3s menu: Music Volume slot CLAIMED @4000006c (word 00000048, field 48 = Full)
t=9.3s menu: SFX Volume   slot CLAIMED @40000068 (word 00000065, field 65 = 5/8)
applied: 3 music, 4 sfx          Music Volume driven to music_vol=0 (mute) 0 times
```

(The model claims at its first sample, 9.3 s, where the card claimed at 12.7 s: the model has no boot delay.  Same
registers, same values.)

## What is still open

* **`FB_MODE` has never been measured on hardware.**  Native 640×480 presents cost mean 21,344 µs / median 16,046 µs /
  worst 75,304 µs, against 320×200 mean 3,608 µs (docs/352).  A 75 ms stall starves the 100,402-byte CDDA ring, so this
  is the port's last real performance lever.  It cannot be a menu entry (both wired words are taken), so it is one
  `night.ini` line: `FB_MODE=0` asks the host to downscale 640×480 → 320×240 instead of presenting a native framebuffer.
  One card cycle decides it.
* `DIAG` should go back to 0 for a shipping build (no MMIO scans at all); 11 is only the development value.
* DSP commands `0xe1` and `0xf2`, seen once each, still unexplained.
* A newer `os.bin` overlay (Doom 1.2.0 ships a 135,400-byte OS against the SDK's 134,424) might implement
  `of_interact_get()` properly; untested, and the peripheral-page route works today.
* Task-8 (wall-clock pad scripts, music schedule on hardware), Task-10 (SD packaging, README), Task-11 (final
  verification and frame dumps).

Core 0.1.49 on the card.  Build clean, `check_interact.py` ALL PASS, fixture suite ALL PASS, `src/f` untouched.
