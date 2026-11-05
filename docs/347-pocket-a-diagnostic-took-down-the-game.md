# 347 — 0.1.35 died mid-boot, and two of my readings were wrong

Core 0.1.36.  The user report was *"only shows terminal, regardless of Pocket LCD setting"*.  Reading the card log
carefully corrected two things I had written down an hour earlier, and produced the first genuinely good news in
this whole interact investigation.

## 1. docs/346's "0.1.34 never ran" was wrong about *which build* ran

`nightlog.sav` on the card: 70 lines, 3619 bytes, last marker `core 0.1.34: boot marker`.  That marker is a lie.  The
ELF on the card is the **0.1.36-predecessor 0.1.35** build — later mtime than the 0.1.35 deploy, and the log it wrote
contains the `periph:` scan, which only 0.1.33 and later emit.  The version *string* is stale because
`OF_CORE_VERSION` lives in `of_interact_idx.h`, a generated header that `make` was not tracking as a dependency, so
the ELF kept printing the previous build's marker.

Fixed properly: `CFLAGS += -MMD -MP` plus `-include $(wildcard $(OBJ_DIR)/*.d)` in `pocket/src/night/Makefile`.  The
0.1.36 ELF prints `0.1.36` (verified with `strings` on the card copy).  A version marker that lies is worse than no
marker: it is what made docs/346 attribute a run to the wrong build.

## 2. The periph scan works, and unlike `of_interact_get()` it says something true

0.1.35's log contains the whole 96-word scan of `0x40000000+`.  Two words match the SDK's own names exactly:

| address | value read | what it is |
|---|---|---|
| `0x40000098` | `01a7e35b` | `HW_FEATURES` — identical to `caps: hw=01a7e35b` from `of_get_caps()` in the same run |
| `0x400000e4` | `00f00140` | `FB_MODE_SIZE` — `0x140`=320, `0xf0`=240, i.e. the framebuffer mode the core is in |
| `0x400000e8` | `00000140` | 320 |

So hardcoded MMIO on this page is readable and coherent for this core — the technique `interactdemo` uses is real.
That is the opposite of the interact ecall, whose table has never carried one true value across three builds.
Whether any of it is *useful* for settings is still open: the Pocket LCD calibration never got a chance to move,
because of what happened next.

## 3. Where the run stopped

`of_log.c` writes unbuffered (`setvbuf(…, _IONBF, …)` and `fflush` per line, committing the save slot every 8
lines), so the last line in the file is the last line the app actually wrote:

```
69  periph 0160: 00000000 0000ffff 00000000 00000000 80008000 80008000 000000ff 00000000
70  analogizer: enabled=0 state rc=0 en=0 video=0 snac=0/0 off=0,0
```

and the next statement in `main_of.c` is `NLOG_STEP("clock source installed")` — absent.  The run never reached
`of_settings_load()`, never reached the interact census, never reached the pump: had the pump run, the log would
have grown by ~4 lines per second (`perf`, `stages`, `pump reanchor`, `menu word`) and it did not.  So the app died
or stalled inside the diagnostic block — after a completed `periph_scan(1)` and a completed `of_analogizer_state()`
— and with no display mode ever requested, the panel showed openfpgaOS's terminal, which is exactly what the player
saw, and why moving Pocket LCD appeared to do nothing.

This is not attributable further from this artifact, and that is the real finding: **I shipped four unproven
probes in the boot path unconditionally, and a diagnostic took down the game.**  Four in one build also means the
failure cannot be pinned to one of them.

## 4. 0.1.36 — probes are opt-in, and the shipping build does none of them

`night.ini` carries one switch, editable on the card with no rebuild:

```ini
DIAG=0     ; 1 = periph scan 0x40000000+   2 = analogizer state probe   4 = interact census   8 = read 0xF7000000
```

`of_setting_diag()` gates every one of them, at boot *and* in the pump.  The boot log now prints the mask:

```
diag=0x0 (1 periph scan 0x40000000+, 2 analogizer probe, 4 interact census, 8 control-word read)
       every probe below is gated on this; night.ini DIAG= is the switch, 0 is the shipping value
```

Two more changes, both about not being able to hurt the player:

* **Out-of-range interact probes deleted.**  They were a reasonable idea — the SDK's `test_interact.c` asserts
  `of_interact_get(999) == 0`, which only means something if the handler size-checks its index — but the only way
  to learn that on hardware is to ask the OS to index a table it may not bounds-check.  0.1.32 read only declared
  indices and ran to completion; 0.1.34/0.1.35 read `OF_INTERACT_MAX_VARS-1` and 999.  The boundary question is now
  asked on the PC shim, where an out-of-range read hurts nobody.
* **The host claims the display at the top of boot.**  `of_video_set_display_mode(OF_DISPLAY_FRAMEBUFFER)` right
  after `of_video_init()`, before anything slow.  The mode was previously only asserted from the pump, so a run
  that never reached the pump never asked for a screen at all.

`of_settings_load()` moved above the diagnostic block so `DIAG=` is known before any probe can run.

## 5. Bisection, on the card, without me

Boot with `DIAG=0` first.  If the game plays, the fault was one of the probes, and each is separable by one digit:

| `DIAG=` | adds | if the run dies here |
|---|---|---|
| 0 | nothing | a probe is not the cause; look at `interact.json` (the 8-entry base) and the early display claim |
| 1 | periph scan | the MMIO scan of `0x40000000+` is fatal for this variant — the scan comes out of the port for good |
| 3 | + analogizer | `of_analogizer_state()` is the fatal call |
| 7 | + interact census | the in-range census is fatal, which would mean `of_interact_get()` is not merely uninformative but dangerous |
| 15 | + control-word read | reading `0xF7000000` from the app is fatal, and the only live channel the port has is closed |

Each step is one number in `night.ini` and one boot.  Read-out:

```sh
grep -nE "boot marker|diag=|phase:|periph 0160|analogizer|interact\[|bridge:" /Volumes/Pocket/Saves/night/common/nightlog.sav
```

The `phase:` lines bracket the stages, so a stalled run is visible as the last `phase:` it reached.

## 6. What is still owed

* The Pocket LCD calibration on the periph page: with `DIAG=1`, move Pocket LCD in Core Options and see whether any
  word at `0x400000xx` moves.  That single measurement decides whether app-facing settings exist at all on this
  bitstream — and it is the only route left, since `interact.json` elements are either fatal (the control word) or
  inert (everywhere else), and the interact ecall answers nothing.
* Whether `DIAG=0` alone restores the picture and sound 0.1.31/0.1.32 produced — the 0.1.32 log's
  `perf pres=16039us max=75567us` at `view=0 640x480` is still the next real bug after this one.
