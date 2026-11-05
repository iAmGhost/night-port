# 355 — music volume and panel fit work on hardware; the third entry is mute, and a constant nearly fooled the claim

Core 0.1.44 measured this, core 0.1.45 tests what it left open.  After eight builds the settings channel is **working
for two settings**:

```
menu: Music Volume slot CLAIMED t=33.2s @0x4000006c the word hardware carried it in on (value 41 = Off, options 41..48)
menu: Music Volume = Full (48) applied: music volume      settings: music_vol=255 applied from the menu
menu: Music Volume = Off (41) applied: music volume       settings: music_vol=0 applied from the menu
menu: Music Volume = 1/4 (43) applied: music volume       settings: music_vol=72 applied from the menu
menu: Music Volume = 5/8 (45) ... 145 ... 3/4 (46) ... 182 ... 7/8 (47) ... 218 ... Full ... 255 ... Off ... 0
menu: Screen Fit slot CLAIMED t=115.6s @0x40000068 the word hardware carried it in on (value 52 = stretch)
menu: Screen Fit = scaler 320x200 (53) applied: panel fit
menu: Screen Fit = letterbox (51) applied: panel fit      ... and the panel changed on the device
```

`census t=137.1s: 2 candidate(s) of 96 registers; claimed: Music Volume, Screen Fit` — the core naming its own working
settings.  The player's report matches: **volume and fit work.**

## 1. SFX Volume's value never arrived anywhere

The third entry (`0xF7000014`, values `0x61..0x68`) produced nothing the port could use, and the log proves the absence
rather than inferring it.  The words around the two known mirrors stayed silent for the entire run:

Each dump line prints eight consecutive words starting at the offset in its label, so `0060:` covers
`0x60 0x64 0x68 0x6c 0x70 0x74 0x78 0x7c`:

```
periph t= 9.2s 0060: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
periph t=98.9s 0060: 00000000 00000000 00000000 00000045 00000000 00000000 00000000 00000000
                     0x60    0x64    0x68    0x6c    0x70    0x74    0x78    0x7c
                                               ^ Music Volume at 5/8 (0x45); its neighbours still silent
```

`0x40000060`, `0x40000064`, `0x40000070` and every neighbour are zero at the end of a 4-minute run in which the player
moved SFX Volume repeatedly.  Searching every `periph move` in the log for a value in `0x61..0x68`, excluding the false
claim below, finds only two counters passing through on their way up (`0x4000010c → 0x62`, `0x40000150 → 0x62, 0x67`).
The music value (`0x45`) and the fit value (`0x51..0x53`) both arrived.  **The SFX value did not.**

## 2. The claim rule grabbed a constant, and the fix is the silence before the value

`0.1.44` did claim something for SFX Volume:

```
menu: SFX Volume slot CLAIMED t=14.9s @0x4000003c found by range cleanliness (value 63 = 1/4, options 61..68)
cand @0x4000003c: clean for SFX Volume, 174 samples holding an option value
```

`0x4000003c` is a **peripheral constant**.  It sat at `00000063` at the first sample (9.3 s, before the player touched
anything), and never changed again in the whole run.  `0x63` happens to be inside SFX Volume's option band, so it
passed "every sample is 0 or an option value" and "held an option value for at least 5 samples" — with room to spare.
It is not a mirror of anything; it is a register that looks like "1/4" forever.  That is why the claimed slot never
applied a value and the sound effects kept playing at the ini's 255 — which is exactly what the player heard.

Range cleanliness cannot separate a constant mirror from a constant register.  What does separate them is the fact
docs/354 §2 established: **a mirror word is silent (0) until the player moves its entry.**  So a word may only be
claimed if it was *seen* silent at some point:

```c
static uint8_t g_menu_zero[MENU_N][OF_PERIPH_WORDS];    /* was this word ever seen silent? */
...
if (v == 0u) g_menu_zero[k][i] = 1;
...
if (!g_menu_clean[k][i] || !g_menu_zero[k][i] || g_menu_seen[k][i] < MENU_SEEN_MIN || !menu_is_option(m, v)) continue;
```

`0x4000003c` is never 0 in any sample → unclaimable.  `0x4000006c` and `0x40000068` are 0 at 9.2 s and speak later →
claimable.  The same rule is applied to the census, and the known-slot path cannot reach `0x4000003c` at all because
the known addresses are the two measured words.

## 3. A menu control that does nothing is worse than no menu control

An entry the OS never mirrors still *appears* in Core Options, still *persists*, and still *looks* like a setting —
which is how a player discovers it is a lie.  0.1.45 makes an entry that gets no word say so, once, in the log:

```
menu: SFX Volume has no peripheral slot after 60 s -- this OS mirrors only some of the app's entries
      and the value from night.ini stands
```

## 4. The one experiment 0.1.45 runs: why those two words?

Two data points (`0xF700000C → 0x4000006c`, `0xF7000010 → 0x40000068`, in both hardware runs) plus one null
(`0xF7000014 → nothing`) leave two hypotheses, and the **declaration order** in `interact.json` is what separates them.
0.1.45 declares the three entries in a different order while keeping their addresses:

| declared order | name | address | known slot |
|---|---|---|---|
| 1st | Music Volume | `0xF700000C` | `0x4000006c` ✓ proven twice |
| 2nd | **SFX Volume** | `0xF7000014` | none |
| 3rd | Screen Fit | `0xF7000010` | `0x40000068` ✓ proven twice |

* **If the page mirrors the first *two app-declared variables*** — then SFX Volume takes `0x40000068` even though it
  sits at `0xF7000014`, and Screen Fit loses its word and answers to `night.ini` plus the L1+B pad combo.  The rule is
  then known: only the first two app entries are readable, so the port must *choose* which two settings to expose.
* **If the page mirrors only two fixed bridge words** (`0xF700000C` and `0xF7000010`) — then SFX stays mute, Screen Fit
  keeps working at `0x40000068` exactly as before, and the port's menu surface is exactly the two settings that already
  work, with SFX volume staying where it has always been.

Music Volume is first in either hypothesis, so the experiment never costs the setting that matters.  What to read in
the next log: `menu: SFX Volume slot CLAIMED @0x40000068 ...` (order-based) versus `menu: SFX Volume has no peripheral
slot after 60 s` together with `Screen Fit ... CLAIMED @0x40000068` (address-based).

## 5. Where the settings surface stands now

| setting | menu (Core Options) | `night.ini` | pad |
|---|---|---|---|
| Music Volume | **works on hardware** (8 levels, `Off` silences the CDDA) | ✓ | — |
| Screen Fit | **works on hardware** (letterbox / stretch / scaler) | ✓ | L1+B cycles it |
| Pocket LCD | works, and the core answers its own menu entry | `SCREEN` | — |
| SFX Volume | declared, not mirrored (0.1.45 tests why) | ✓ | — |
| CDDA on/off, `TICK_HZ`, `BLIT_SKIP`, `PRESENT_SYNC`, `FB_MODE`, `DIAG` | no menu entry | ✓ | — |

Two things this does not change, both recorded so nobody re-pays for them: an app `interact.json` element at
`0xF7000000` stops the core booting (0.1.27, 0.1.34) and an app read of `0xF7000000` hangs the core (0.1.37).  The
peripheral page is the channel; the interact ecall still answers fixed junk at every index while the same entries move
registers.
