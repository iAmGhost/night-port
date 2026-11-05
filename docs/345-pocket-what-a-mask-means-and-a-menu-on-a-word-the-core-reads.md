# 345 — what a mask actually means, and the menu mounted on the word the core really reads

Core 0.1.34.  Two things the user supplied made the difference: *use the shipped `openfpgaOS` core's
`interact.json` as the base, add settings at a different address/mask*, and the Analogue spec itself
(analogue.co/developer/docs/openfpga/core-definition-files/interact-json).  The spec immediately explained three
failures that had been attributed to "the OS is broken".

## 1. The two sentences that were misread

> **`mask`**: Bits set to '1' will be **left untouched** in the register. Bits set to '0' may be modified by the
> UI element.

> Each frame, all of the UI elements are **read back from the core** at a specified 32-bit BRIDGE address,
> updated with new user-adjusted values, and finally written back.  This read-modify-write cycle **requires the
> core to have both read and write support at each desired address**, unless the element is flagged as
> `writeonly`.

With those in hand:

| version | what was declared | what the spec says that means |
|---|---|---|
| 0.1.32 | knobs at `0x03FE0000/08`, mask `0xFFFFFFFF` (copied from Doom) | mask 1 = untouched, so an all-ones mask lets the element write **nothing**.  An element that may not modify a register has no state to read back — hence "shown as unselected, selecting does not change the selected state".  Doom's App Option 1/2/3 are inert placeholders for the same reason, and its `interact_persist.json` carries ids 20/40/50/60/70 (its reserved entries) and none of theirs |
| 0.1.27 | Music Volume in bits 20-23 (mask `0xFF0FFFFF`) with options `0x00…0x80` | option values lying **outside the field the element writes**.  The core stopped booting — the card kept the 0.1.26 log and wrote nothing new |
| 0.1.25/26 | knobs at `0xF700000C` / `0xF7000010` | an address the os25 bitstream does not decode: the **read** half of the cycle had nothing to read, so the menu could neither show nor move them |

So "unselected and unmovable" was not the OS refusing the menu.  It was a malformed interact definition: an
element whose own mask forbids it from writing anything.

## 2. The base is now the shipped core's file, verbatim

`pocket/dist/sdk/Cores/ThinkElastic.openfpgaOS/interact.json` (vendored by `scripts/pocket/sync_sdk.sh`, md5
`7969c16d…`) is the interact definition for the exact bitstream this port runs, and the only one known to work on
it.  `mk_dist.py` reads it and appends NIGHT's entries, so entries 0-7 are byte-identical to the shipped core's —
including **"Analogizer 15kHz Timing"** (id 61, bits 18-19), which the SDK *template* we had been using omits, so
our menu had been describing a different picture of that word than the core does.

Bit map of `0xF7000000` as `check_interact.py` now computes it (a field = the complement of the mask):

```
Enable Analogizer          bit 15              mask 0xFFFF7FFF
SNAC Adapter               bits 0-4            mask 0xFFFFFFE0
SNAC Controller Assignment bits 6-9            mask 0xFFFFFC3F
Analogizer Video Out       bits 10-13          mask 0xFFFFC3FF
Analogizer 15kHz Timing    bits 18-19          mask 0xFFF3FFFF   (was missing from our file)
Pocket LCD                 bits 16-17          mask 0xFFFCFFFF   <- the one control the player could move
Music Volume   (NIGHT)     bits 24-26          mask 0xF8FFFFFF   values 0x01000000..0x05000000
Screen Fit     (NIGHT)     bits 27-28          mask 0xE7FFFFFF   values 0x08000000..0x18000000
still unwritten            bits 5, 14, 20-23, 29-31
```

Bits 20-23 are deliberately left alone: 0.1.27 wrote there and the core would not boot, so something lives behind
them.  Values are 1..5 and 1..3 shifted *into their own field*, and none of them is 0 — an untouched variable
reads 0 on the wire, and 0 has to keep meaning "nobody chose", because 0 is also a legal volume (mute, docs/341).

## 3. The host now reads the word it is written into

`of_save.c`:

```c
#ifndef OF_PC
#define OF_BRIDGE_CTRL 0xF7000000u
#endif
static unsigned bridge_control_word(void) {
#ifdef OF_PC
    return 0u;            /* the desktop shim has no bridge; every field then reads "not chosen" */
#else
    return *(volatile unsigned *)OF_BRIDGE_CTRL;
#endif
}
```

Fields are then extracted by mask (`MUSIC_SHIFT 24 / MUSIC_MASK 7`, `FIT_SHIFT 27 / FIT_MASK 3`, Pocket LCD
16-17), which is why the old junk-filter (`bridge_usable`) and the "accept a bare volume" fallback are now `#if 0`
— a bounded field cannot carry junk.

Direct MMIO is not an invention here: it is what the SDK's own `interactdemo` does for live platform state
(`RAW32(0x4000006C)` MOUSE_SPEED_PCT — **a setting** — `0x40000098` HW_FEATURES, `0x400000E4` FB_MODE_SIZE,
`0x400000EC` VIDEO_SCALER_MODE, `0x40000134` input slots; `src/apps/interactdemo/main.c:396-436`), and the log
line before the first read says *"if the log stops on this line, the read itself is the answer"*.

`of_interact_get()` stays as the **cross-check**: whenever the word changes, the host logs the word, the decoded
fields, and then what the documented ecall says about indices 8, 9 and 5 in the same frame.  That pair of
readings is the measurement docs/344 could not make — and docs/344's other conclusion still stands on its own
evidence: the interact *ecall* returns a fixed table nobody fills (`interact[0..8]` identical at boot and +3 s,
identical to 0.1.29's values), so the bus is the channel and the ecall is not.

## 4. What `interactdemo` does and does not prove

It is worth being exact, because it is the only app in the SDK that reads interact:

* `update_interact()` (main.c:374-388) prints **`v0-3` only** — the four reserved variables.  Its comment states
  the model: *"a variable reads 0 until the core declares it and the user changes it from the menu."*
* `src/apps/interactdemo/` ships **no `interact.json`** (only `main.c` + `Makefile`), so the demo never reads an
  app-declared variable.
* The SDK's test is `test_interact.c`: calls `of_interact_get(0..2)`, `test_pass("read[0-2]")`, and
  `ASSERT_EQ("oob", of_interact_get(999), 0)`.  Nothing asserts that a menu-set value arrives — and on hardware
  0.1.32 that OOB-ish behaviour did *not* hold (in-range indices returned `0x4f40446d`-style values), which says
  the table is unfilled rather than absent.
* What it does prove is usable, and 0.1.34 uses it: **hardcoded MMIO is how a live platform value reaches an app
  on this OS**.

## 5. Rules `check_interact.py` now enforces

1. Entries 0-7 match the vendored shipped core's file (`name/id/type/address/mask`) — no hand-written
   approximations of a register the core owns.
2. No two elements write the same bit of one word (this is what caught 0.1.27's near-miss on bits 10-13).
3. An app knob writes only bits ≥ 24 of `0xF7000000`, and an all-ones mask is a failure ("the element can write
   nothing"), not a style choice.
4. Every option value lies inside its element's own field, and no option value is 0.
5. `defaultval` is one of the options; the host's `*_SHIFT` / `*_MASK` equal the field the JSON writes; the
   `OF_IDX_*` constants equal the JSON positions — in both directions.

## 6. Test and read-out

```sh
cd pocket/src/night && make && make app_pc && cd ../../.. && make test && python3 scripts/pocket/check_interact.py
bash scripts/pocket/deploy.sh pocket/build/pocket/night scratch/pocket /Volumes/Pocket
# boot; Core Options: Music Volume and Screen Fit should now show a selected state and move;
# move each a few times, also move Pocket LCD, play ~30 s, eject
grep -E "^bridge|^  cross-check|^interact\[|periph" /Volumes/Pocket/Saves/night/common/nightlog.sav
```

| what the log shows | meaning |
|---|---|
| `bridge word=…` changes value when Music Volume moves, `music=2..5` decoded, volume audibly changes | the menu reaches the core and the app can read it: real settings, no `interact.json` read needed |
| the menu now *shows* a selection but `bridge word=` never changes | the firmware writes and the core discards those bits → the app cannot see app fields, only the core's own fields; `night.ini` + L1+B stay the settings surface, and the knobs go back out |
| `interact[8]/[9]` finally answer `err=0` with values matching the field | the ecall works once the entry is well-formed — docs/342/344's "the OS never answers" gets narrowed to "the OS never answers a malformed entry" |
| the core does not boot (no `boot marker`) | bits 24-28 are not free after all; revert `APP_INTERACT_KNOBS` to `False` (one flag) and the card boots the previous known-good menu |
| the log ends on the `bridge: control word` line | an app may not read `0xF7000000` on this bitstream, and the whole "read the bus" idea dies with it |

One risk worth stating plainly: this version changes the menu *and* the read path at once, which is the mistake
docs/343 warned about.  It is acceptable only because both halves test one hypothesis ("a well-formed field in a
decoded word, read off the bus"), and because reverting is a single flag — `APP_INTERACT_KNOBS = False` restores
the exact menu 0.1.33 shipped with.
