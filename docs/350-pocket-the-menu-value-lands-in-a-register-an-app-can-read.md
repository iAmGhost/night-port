# 350 — the menu value is readable after all: openfpgaOS publishes it at periph `0x40000080`

Core 0.1.39.  A 1423-line card log from 0.1.38 with `DIAG=11` (periph scan + analogizer probe + the retired bit)
answered the question three builds and two bricked cores were chasing.

## 1. The register

Watching which of the 96 scanned words moved gave a clean table.  Grouped by how often they moved:

| register | moves | values seen | what it is |
|---|---|---|---|
| `0x40000080` | 3 | `00010000`, `00020000`, `00000000` | **Pocket LCD.**  All three option values from `interact.json` — On `0x00000000`, Off `0x00010000`, Terminal `0x00020000` — appeared in one run, in **bits 16-17**, exactly as the JSON declares them.  The player moved the menu; the word moved. |
| `0x400000e4` | 4 | `00f00140` → `01e00280` | applied framebuffer mode as `(height<<16)\|width`: 240/320 → 480/640, i.e. 320×240 → 640×480 |
| `0x400000e8` | 4 | `00000140` → `00000280` | applied framebuffer width: 320 → 640 |
| `0x400000ec` | 3 | `0` → `7` | the scaler slot in use (the SDK's `VIDEO_SCALER_MODE`): 0 at 320×240, 7 at 640×480 |
| `0x40000004`, `0x40000024`, `0x400000c0`, `0x400000dc`, `0x40000040`, `0x40000018` | 10-48 each | free-running / geometry chatter | counters and mode-dependent timing words, moved on their own |
| `0x40000098` | 1 | `01a7e35b` | `HW_FEATURES`, identical to `caps->hw_features` |

So the three negative findings all stand **and** the channel exists:

* `of_interact_get()` — carries nothing (0.1.28/29/32/36).
* Reading `0xF7000000` from an app — hangs the core (0.1.37).
* An `interact.json` app element on `0xF7000000` — prevents boot (0.1.27/0.1.34).

But openfpgaOS keeps its own copy of menu state in **its own periph page**, at an address an app may read — which is
exactly the page `interactdemo` reads (`HW_FEATURES`, `MOUSE_SPEED_PCT`, `FB_MODE_SIZE`, `VIDEO_SCALER_MODE`).  The
interact *ecall* was never the channel; the OS's peripheral registers were.

## 2. 0.1.39 turns the scan into a named witness

`periph_named()` reads those four registers and logs only on change:

```
periph reg: lcd=00020000(Terminal) applied=640x480 scaler=7
```

* `applied=` / `scaler=` answer the question docs/343 had to answer by inference: **what the core actually applied**,
  read straight off the bus, alongside the host's own `geo view: applied … | asked …` witness.
* `SCREEN=-1` now means something: the pump follows `0x40000080` and switches to `OF_DISPLAY_TERMINAL` when the menu
  says Terminal.  `of_settings_load()` no longer resolves `-1` to `0` at boot, which is what silently killed the
  follow for several builds.  With the periph probe off (`DIAG=0`, the shipping value) `-1` behaves as `0`.
* The 96-word per-second watch now stops itself at 90 KB of log with a line saying so, because a scan must not
  crowd out the game's own lines in a 256 KB save slot.  The named four keep running.

Card is set to `DIAG=11` for the next boot.

## 3. The other thing that log measured: the 640×480 present is expensive

Split by which framebuffer the game was drawing into, from the same run:

| source | presents | mean | worst |
|---|---|---|---|
| 640×480 (native mode, `FB_MODE=1`) | 17 | **20,165 µs** | 62,739 µs |
| 320×200 (1:1 into 320×240) | 29 | **3,224 µs** | 62,739 µs |

20 ms per present on a 640×480 source is a sixth of a second — that is 307,200 index pixels copied through the
bridge, and it is the same shape as docs/343's measurement (a 640×480 copy is a bandwidth cost, not a bug in the
blit).  The 62.7 ms worst case is a mode switch, and a stall that long is what starves the CDDA ring.

The alternative route already exists in the port (`night.ini FB_MODE=0`: keep the 320×240 framebuffer and let the
host downscale 640×480 → 320×240 in software, 153,600 source pixels read and 76,800 writes instead of 307,200
copies).  That has never been measured on hardware.  The next boot after this one should carry `FB_MODE=0` and the
same `perf pres=` lines will settle which route the Pocket prefers — and the `periph reg: applied=…` line will say
which mode the core was actually in, so the comparison cannot be misread.

## 4. What is *not* claimed

Only **one** menu value has been observed on a readable register: Pocket LCD.  Whether the other reserved entries
(Analogizer, SNAC, the two offset sliders) land in neighbouring registers — `0x40000084` moved six times to small
values (`1, 3, 6, 0xc, 0x10, 0xb`) and `0x40000088` twice (`6 → 0xa`) — is unmeasured: the offsets were nudged in
this run but nothing was logged against a timestamp, and correlating a slider to a register needs the move marked in
time.  That is a cheap follow-up: add a seconds-since-boot stamp to the `periph move` lines and have the player move
one slider at a known moment.

And a *usable* NIGHT setting (volume, fit) would still need the OS to mirror a value the core can't be asked to own —
`interact.json` app elements are fatal on the control word and inert elsewhere.  So the settings surface stays
`night.ini` plus the L1+B pad combo (the user confirmed L1+B cycling fit works on hardware), and `SCREEN=-1` is now
the one menu-following setting that genuinely works.
