# 342 — every interact index answers 0x55555555, and `of_interact_get()` hides why

Core 0.1.29.  Two things landed in the same turn: the report that **Music Volume still does nothing** (moved
several times during a run), and the request to put the **panel stretch in the menu with non-stretched as the
default**.  The first produced a finding that changes what the menu is worth at all.

## 1. What the card log said

`Saves/night/common/nightlog.sav`, 0.1.28 run:

```
core 0.1.28: boot marker
settings: music_vol=128 sfx_vol=255 cdda=1 pad=1 tick_hz=490 screen=0
interact: 0=55555555 1=55555555 2=55555555 3=55555555 4=55555555 5=55555555 6=55555555 7=55555555
music knob: raw=55555555 -> vol=128 (index 7, whole word)
menu lcd=00010000 scr=0 blit fast=1 slow=0 skip=0        ; every second, all run long
```

Two things follow.

**Every interact index returns the same word, 0x55555555.**  Not a value, not a menu choice — and not an error
code either: `of_error.h` codes are small negatives (`OF_ERR_NOT_SUPPORTED = -2`).  It is a poison pattern.

**The Pocket LCD reading we have been logging since 0.1.25 was that same poison, masked.**  The host masks bits
16-17 out of whatever the index hands back: `0x55555555 & 0x00030000 = 0x00010000`, which is exactly the value the
log printed as `menu lcd=00010000`, i.e. "Off".  It looked like a real menu read for six versions.  It was not:
the LCD *does* change on the hardware, because the bitstream acts on `0xF7000000` directly — but the app never
saw it.  Every `screen=0` in every log since then is the host's own default, not the user's choice.

## 2. Why the number cannot be interpreted: the SDK helper throws the error away

`of_interact.h:39`:

```c
static inline uint32_t of_interact_get(int index) {
    return (uint32_t)of_ecall1(OF_EID_INTERACT, OF_INTERACT_FID_GET, index).value;
}
```

`of_syscall.h:67-99` returns a pair — `.error` (0 = success, negative = an `of_error.h` code) and `.value` — and
this helper keeps only `.value`.  So 0x55555555 is ambiguous between the two answers that matter:

* the OS answered the call and its interact table holds poison (a wiring problem in our `interact.json`), or
* the OS **refused** the call (`OF_ERR_NOT_SUPPORTED` for this EID/FID) and `.value` is junk it never meant to
  publish (the openfpgaOS kernel has no app-facing interact read, and the menu is decorative for apps).

The SDK's own test plan does not settle it either — `src/apps/testdemo/test_cases.md:310` specifies only
*"Index out of range → returns 0 or garbage (no crash)"*, and here indices 0-7 were all in range for nine
declared variables.  Worth noting: **no shipped openfpgaOS game reads interact.**  `grep -rn of_interact_get
openfpgaOS-Doom/src openfpgaOS-Quake/src` matches only the SDK headers and `interactdemo`.  Doom ships its app
knobs at `0x03FE0000/0x03FE0004/0x03FE0008` ("App Option 1/2/3") and never reads them.

## 3. 0.1.29 keeps both halves of the answer

`of_save.c`:

```c
long of_interact_probe(int index, unsigned *value) {
    struct of_sbiret r = of_ecall1(OF_EID_INTERACT, OF_INTERACT_FID_GET, index);
    if (value) *value = (unsigned)r.value;
    return (long)r.error;
}
```

Every interact read in the host goes through it — boot, the settings load, and the 100 ms poll — and the log now
carries the error and the value side by side:

```
interact[i]: err=<n> val=<xxxxxxxx>          ; one line per declared index, at boot
bridge: music index 8 err=… val=… | fit index 7 err=… val=…
bridge: lcd index 6 err=… val=… field=…
bridge decoded: music=… fit=… lcd=…
bridge: music raw=… err=… decoded=…          ; re-logged whenever the raw word moves
```

`decoded=` is the host's reading of the word: `music` 0 means "no instruction" (so `night.ini MUSIC_VOL` stands),
`fit` -1 means the same for the panel fit.  0x55555555 is named in the code (`BRIDGE_JUNK`) and rejected by
`bridge_usable()` along with 0, so a poison word can never be mistaken for a choice — which is what the first
version of this bug did to `MUSIC_VOL`.

**The one run of 0.1.29 answers the question.**  If the log says `err=-2` at every index, openfpgaOS does not
implement an app-facing interact read, no address would have worked, and the honest statement is that the Core
Options knobs are cosmetic until the OS supports them — `night.ini` being the control that works today.  If it
says `err=0` with a value that moves when the knob moves, then the wiring is ours to fix and the place to try
next is openfpgaOS's own app-option range, `0x03FE0000` (Doom's App Option 1 shape: a plain unmasked `list`).

## 4. The rule 0.1.27 broke, now enforced by a checker

0.1.27 mounted Music Volume *inside* `0xF7000000` (bits 20-23, mask `0xFF0FFFFF`) and **the core stopped
booting**: the card kept the previous run's log and wrote nothing, i.e. the app never reached the point of
opening the log slot.  `0xF7000000` is openfpgaOS's own control word — Analogizer, the SNAC lists and Pocket LCD
are decoded out of it by the bitstream — and Analogue's doc says the firmware reads back, updates and writes back
every declared element's field **each frame**.  A menu knob mounted there is not a knob, it is a periodic write
into core state carrying a default value of its own.

`scripts/pocket/check_interact.py` now refuses that shape outright, and prints the whole bit map it enforces:

```
[PASS] 0xF7000000: 5 elements, no shared bits -- Enable Analogizer 15; SNAC Adapter 0-4;
       SNAC Controller Assignment 6-9; Analogizer Video Out 10-13; Pocket LCD 16-17
       still unwritten at 0xF7000000: 5, 14, 18-31
[PASS] app knobs on their own addresses: Screen Fit@0xF7000010, Music Volume@0xF700000C
[PASS] Screen Fit: options [1, 2, 3] map 1:1 onto the host's choice table
[PASS] Screen Fit: host index 7 matches its position in interact.json
```

Bits 10-13 were the obvious place to put a knob and are **not free**: "Analogizer Video Out" owns them (its mask
`0xFFFFC3FF` is zero exactly at 10-13).  The checker also verifies that each app knob's option values are exactly
the 1..N the host maps, that its `defaultval` is one of its own options, and that the JSON position equals the
`OF_IDX_*` constant the generated header hands the C.

## 5. Screen Fit, and how the fit is applied where it can be seen

`interact.json` gains **Screen Fit** (id 101, `list` at `0xF7000010`, the address 0.1.25 shipped a knob at
without breaking anything): **Letterbox** (default) / **Stretch** / **Scaler 320x200**.  In the host:

| fit | what draws the 320x200 game picture on a 320x240 panel |
|-----|--------------------------------------------------------|
| 0 (default) | 1:1 copy, 20 black rows above and 20 below — what this port has always done |
| 1 | the host shows 40 of the 200 rows twice, one every fifth row: 200 + 40 = 240, no bars |
| 2 | ask the core's scaler for its declared 320x200 framebuffer mode and let the FPGA stretch it |

Letterbox is the default on purpose: it is the known picture, and stretching is the thing to opt into.

Two fixes were needed before the desktop build could even show a difference.  The fit had been cached in
`g_fit_stretch`, set only inside the hardware video probe — which the desktop build skips, so `NIGHT_FIT=`
changed nothing there and a fit comparison produced two identical sets of frames.  The copy path now asks the
setting every frame (`fit_wants_stretch()`), and `fit_apply()` is left owning only the scaler-mode request,
which is a hardware thing (a mode change costs 41-53 ms, docs/338, so it is issued only when the choice changes).

Measured with `NIGHT_PANEL_SHOT` witnesses (`firstrow`/`lastrow` = first and last non-black panel row):

```
fit0 0002..0007:  firstrow=20 lastrow=219   ; 20 bars above, 20 below, 200 rows of game
fit1 0002..0007:  firstrow=0  lastrow=239  duprowpairs=40   ; the whole panel, 40 doubled rows
fit2 0002..0007:  firstrow=0  lastrow=239  duprowpairs=40   ; the shim has no 320x200 scaler mode,
                                                 so the desktop build falls back to the host stretch
```

Because the menu is not yet proven readable, the fit has two routes that need no interact bridge at all:
`night.ini` `FIT=0|1|2` (deployed as `FIT=0`), and **L1 + B** on the pad, which cycles Letterbox → Stretch →
Scaler live and logs `fit: pad cycle -> n` followed by the `fit=… (…)` line.  L1 is already the save-combo
modifier (L1+Select save, L1+Start load, L1+A next slot); B is the kick slot, so the game sees a kick press
during the combo, which is the same unavoidable cost the save combos already pay.

## 6. Reproduce

```sh
cd pocket/src/night && make test && make            # app_pc (desktop shim) + the RISC-V ELF
cd ../../
python3 scripts/pocket/mk_dist.py | tail -1
python3 scripts/pocket/check_interact.py            # menu/host agreement, bit map, no control-word knob
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_SEED=1234 NIGHT_CD_DIR=scratch/pocket/cd
for f in 0 1 2; do NIGHT_FIT=$f NIGHT_PANEL_SHOT=/tmp/fit$f ./pocket/src/night/app_pc --frames 1500 >/tmp/fit$f.log 2>&1; done
# firstrow/lastrow of /tmp/fit{0,1,2}/*.ppm: 20/219 for fit0, 0/239 with 40 doubled rows for fit1
make test                                           # the fixture suite, unchanged: ALL PASS
bash scripts/pocket/deploy.sh pocket/build/pocket/night scratch/pocket /Volumes/Pocket
# on hardware, the question is answered by:  grep -E "interact\[|bridge" Saves/night/common/nightlog.sav
```

## 7. Open

* Whether openfpgaOS answers `OF_EID_INTERACT` at all — 0.1.29's `err=` column is the measurement.  Until it is
  known, the honest description of the Core Options knobs is "declared, not yet proven readable", and
  `night.ini` is the control surface that works.
* If the ecall is refused: `MUSIC_VOL`, `SFX_VOL`, `FIT`, `CDDA_OFF`, `PAD_OFF`, `SCREEN`, `TICK_HZ` in
  `night.ini` are the settings, and the menu entries should be **removed** rather than shipped dead — a control
  that does nothing is worse than no control.
* `SCREEN=-1` ("follow the menu") is currently inert for the same reason; `SCREEN=0/1/2` in the ini is real.
* The stretch question itself is not settled aesthetically: 320x200 on the original's 4:3 CRT *was* shown
  stretched by 1.2, so `FIT=1` is arguably the closer picture.  That is a judgement for whoever plays it, which
  is why it is a choice and not a default.
