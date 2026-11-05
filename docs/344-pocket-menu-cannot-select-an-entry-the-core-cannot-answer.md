# 344 — the menu cannot select an entry the core cannot answer, and interactdemo does not prove otherwise

Core 0.1.33.  Report: Music Volume and Screen Fit show as unselected and selecting them does not change the
selected state, while **Pocket LCD works** — it switched the display to Terminal dynamically.  That contrast is
the whole finding.

## 1. What the 0.1.32 card log says

```
core 0.1.32: boot marker
caps: version=4 hw=01a7e35b os=00000001 (sdk OF_CAPS_VERSION=4)
analogizer: enabled=0 state rc=0 en=0 video=0 snac=0/0 off=0,0

interact[0]: err=-419071091   val=00000000     interact[5]: err=0 val=45d1f560
interact[1]: err=0            val=4f40446d     interact[6]: err=-277204179 val=00000000
interact[2]: err=0            val=29138574     interact[7]: err=0 val=73e157d5   (Screen Fit @ 0x03FE0000)
interact[3]: err=0            val=52a1cde1     interact[8]: err=0 val=5986c511   (Music Volume @ 0x03FE0008)
interact[4]: err=-1819641228  val=00000000

interact2[0..8]   -- three seconds later --    BYTE-FOR-BYTE IDENTICAL to interact[0..8]
bridge: lcd index 6 err=-277204179 val=00000000 field=00000000
bridge decoded: music=0 fit=-1 lcd=00000000
grep -c "^bridge"  ->  2        (both are the boot decode; no raw word ever moved during the run)
menu lcd=00000000 scr=0        x 43 samples, including the moments the panel was in Terminal
```

Four things follow.

**The interact table is never filled.**  Boot and +3 s read the same values, and they are the *same values the
0.1.29 run read* (`4f40446d`, `29138574`, `52a1cde1`, `45d1f560`, `73e157d5`, `5986c511`).  Values that survive a
reboot and a rebuild are not uninitialised SDRAM and not bus reads — they are a fixed table in the OS image that
nothing writes to.

**Not one value reached the app, including the one the player demonstrably changed.**  Pocket LCD moved, and the
core acted on it (Terminal came up), yet index 6 read `err=-277204179 val=00000000` on all 43 samples.  The app
and the core are not connected on this path at all.

**The errors are not errors.**  `of_error.h` codes are small negatives; `-419071091`, `-277204179` and
`-1819641228` are not among them.  `err=0` alongside `val=4f40446d` is a handler answering "success" from a table
it never populated.

**`os_features` is 0x00000001** — `OF_OS_FEAT_MOUSE_COUNTS`, the only OS feature flag that exists in this SDK
(`of_caps.h:38-41`).  There is no interact capability bit to gate on, which is consistent with interact never
having been finished on this OS rather than being conditionally available.

## 2. Why the UI behaves exactly as reported

Analogue's own rule for `interact.json`: the firmware performs a **read-modify-write at each declared address
each frame**, and *"this read-modify-write cycle requires the core to have both read and write support at each
desired address."*  The Core Options `list` draws its selected state from the value it read back **from the core**.

* **Pocket LCD** sits at `0xF7000000` bits 16-17 — a word the openfpgaOS bitstream decodes.  Read-back works, the
  selection moves, the core acts on it.  It works *at the core level*, and the app still reads nothing.
* **Screen Fit / Music Volume** sit at addresses the prebuilt openfpgaOS bitstream does not decode (`0xF700000C`,
  `0xF7000010`, and in 0.1.32 `0x03FE0000` / `0x03FE0008`).  Nothing comes back, so the UI has no state to draw:
  "shown as unselected, and selecting doesn't change the selected state".  The symptom *is* the mechanism.

Doom is the tell.  Its `interact.json` declares App Option 1/2/3 at `0x03FE0000/04/08` — the address I copied —
and its `interact_persist.json` on the card stores ids 20, 40, 50, 60, 70 (its **reserved** entries) and none of
the App Option ids, while no line of Doom's or Quake's source ever calls `of_interact_get`.  The app options are a
template placeholder, not a demonstrated feature.

## 3. `interactdemo` read properly

The demo was worth reading in full before concluding, and it does not demonstrate what it is often cited for.

* `src/apps/interactdemo/main.c:374-388` — `update_interact()` prints **`v0-3` only**: the four *reserved*
  variables (Analogizer, SNAC Adapter, SNAC Assignment, Analogizer Video Out).  Its comment is honest about the
  model: *"a variable reads 0 until the core declares it and the user changes it from the menu."*
* `src/apps/interactdemo/` contains **no `interact.json`** — only `main.c` and `Makefile`.  The demo has no
  app-declared variable to read, and the SDK's own cores declare only reserved entries
  (`dist/sdk/Cores/ThinkElastic.openfpgaOS/interact.json`: eight variables, all reserved, `0xF7000000/04/08`).
* The SDK's test for it is a smoke test — `src/apps/testdemo/test_interact.c`:
  ```c
  uint32_t v0 = of_interact_get(0); ... test_pass("read[0-2]");
  ASSERT_EQ("oob", (int)of_interact_get(999), 0);
  ```
  It asserts only that calls return and that an out-of-range index reads 0.  It never asserts that a value the
  menu set reaches the app.  Nothing in the SDK does.
* What the demo **does** give, and what 0.1.33 uses: `update_raw()` (main.c:396-436) reads live platform state
  straight off the periph page — `RAW32(0x40000134)` input slots, `RAW32(0x40000098)` HW_FEATURES,
  `RAW32(0x4000006C)` **MOUSE_SPEED_PCT, a setting**, `RAW32(0x400000E4)` FB_MODE_SIZE, `RAW32(0x400000EC)`
  VIDEO_SCALER_MODE — and explains that `out_width` is not a register but the scaler slot's width, decoded
  "the same way core_top's scaler_slot_width() does".  Hardcoded MMIO is the SDK's own working technique for
  platform state on this OS.

So: interact is *dispatched* (the OOB assertion implies index validation) and *empty*.  Live values on this OS
come from the periph page, not from the menu.

## 4. 0.1.33

**The knobs are out of the menu** (`APP_INTERACT_KNOBS = False` in `mk_dist.py`, with this evidence in the
comment).  A control that cannot be moved is worse than no control, and the two of them made a working control —
Pocket LCD — hard to find.  What a player can change today, all of it independent of the bridge:

| setting | route |
|---------|-------|
| panel fit | **L1 + B**, live (Letterbox → Stretch → Scaler), or `night.ini FIT=0\|1\|2` |
| music / sfx volume | `night.ini MUSIC_VOL=`, `SFX_VOL=` |
| display | `night.ini SCREEN=0` game, `SCREEN=2` game + log overlay |
| CD-DA / pad | `night.ini CDDA_OFF=`, `PAD_OFF=` |

**The periph-page scan** (`periph_scan()`, main.c): 96 words at `0x40000000..0x4000017C` — only the range
`interactdemo` itself walks, so every address is one a shipped app already reads.  Boot prints a full snapshot;
after that it logs *only words that move*, plus a snapshot every tenth pass.  Pocket LCD is the calibration: it is
the one setting the core answers, so **whatever word changes when the player moves it is a live settings word**,
and a decoded neighbour is somewhere a NIGHT setting could live.  The log line printed before the first read says
"if the log stops on this line, the read itself is the answer".

**Boundary probes**, because the SDK's test asserts something specific: `interact[63]` and `interact[999]` are
read alongside the declared indices.  If the edges read 0 while 0..6 read fixed garbage, the ecall is dispatched
and validating, and the accurate statement is "its table is never filled" — not "it does not exist".

## 5. Reproduce

```sh
cd pocket/src/night && make && make test
cd ../../ && bash scripts/pocket/deploy.sh pocket/build/pocket/night scratch/pocket /Volumes/Pocket
# boot, open Core Options, move POCKET LCD a few times (Terminal <-> On), play ~30 s, eject
grep -E "^caps:|^analogizer:|^interact|periph" /Volumes/Pocket/Saves/night/common/nightlog.sav
grep -c "^periph move" /Volumes/Pocket/Saves/night/common/nightlog.sav     # >0 means a live settings word exists
```

| outcome | meaning |
|---------|---------|
| `periph move @0x4000xxxx` lines appear when Pocket LCD moves | there is a live, app-readable settings channel; NIGHT's own knobs get mounted there instead of in `interact.json`, and the menu becomes real |
| no movement, and `interact[63]/[999]` read 0 while 0..6 read the same fixed garbage | interact is dispatched and unfilled; menu settings are not available to a custom core on this bitstream, and `night.ini` + L1+B stay the settings surface |
| no movement, and the edges read garbage too | the handler does not validate indices either, which makes the OS's own `test_interact` assertion false on this build — worth reporting upstream |
| the log ends on the `periph:` line | raw reads at that page are not safe for this variant, and the scan comes back out |

## 6. Open

* Whether the newer openfpgaOS pair your Doom cores ship (os.bin 135400 B / os25.rbf_r 2123992 B, 2026-09-13,
  against my vendored 134424 / 2135596 from SDK HEAD 2026-08-18) changes any of this.  `deploy.sh`'s 4th argument
  swaps the pair; it is deliberately not the default, and it puts the bitstream in `Cores/` and `os.bin` in
  `Assets/` where each actually lives.
* If the scan finds nothing, the honest options are: ship `night.ini` as the settings surface and say so in the
  core's README; or build a custom openfpgaOS variant with an app-option register, which is an FPGA project
  rather than a port change (docs/330 §budget rejected it for good reason).
* Pocket LCD remains the one menu setting that works — through the core, not the app — and `SCREEN=0` in
  `night.ini` is what the host pins.  A player choosing Terminal while the host pins the framebuffer is a state
  the port should describe, not paper over: the core wins that argument, and the log now shows both sides.
