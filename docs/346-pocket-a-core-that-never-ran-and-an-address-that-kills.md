# 346 — 0.1.34 never ran: the death is in core bring-up, not in the app

Core 0.1.35.  The card answered the question that the screen could not.

## 1. Log forensics

`/Volumes/Pocket/Saves/night/common/nightlog.sav` after the 0.1.34 attempt:

```
-rwx------  28841  nightlog.sav
17:core 0.1.32: boot marker        <- the newest (only) boot marker in the file
tail:
audio: dsp commands with no case in the port = 2: 0xe1=1 0xf2=1
memstat frames=1587 ticks=20166(+494/s) pumps=2846 arena=3903168/16777216 pit=490 Hz xfer=2 plays=2 cdda=519/17856/3 clock=1008797
view=0 640x480 sig=2890181e chg=9 pal=256
perf pres=16039us max=75567us pump=2846 ticks=20166 skips=0
```

The file is byte-for-byte the **0.1.32** run.  0.1.33 and 0.1.34 wrote nothing.  Since the app opens its log slot
(`fopen("save:7","wb")`) as the first thing it does, an absent log means the ELF never executed: the core died
during bring-up, while the APF was reading `core.json` / `interact.json` and bringing the SoC up.

That exonerates everything 0.1.34 added to the *app*:

* `bridge_control_word()` — `*(volatile unsigned *)0xF7000000`, the bus read — never ran.  It is not implicated,
  and 0.1.35 keeps it.
* The periph-page scan — never ran (0.1.33 was replaced before it was booted, so that measurement is still owed).

What is left standing between the two dead cores is only the menu definition.

## 2. The pattern, now with the failure stage attached

| build | app element declared at | log written? | outcome |
|---|---|---|---|
| 0.1.25 / 0.1.26 | `0xF700000C`, `0xF7000010` | yes | booted, ran for hours; values unreadable |
| 0.1.32 | `0x03FE0000`, `0x03FE0008` | yes | booted and played (the log above is it); mask `0xFFFFFFFF` → the elements could write nothing |
| 0.1.27 | **`0xF7000000`** bits 20-23 | **no** | core never came up |
| 0.1.34 | **`0xF7000000`** bits 24-26 + 27-28 | **no** | core never came up |

0.1.34 avoided bits 20-23 entirely, put every option value inside its own field, used no zeros and no overlap with
a reserved field — and the core still never came up.  So the variable is the **address**, and the failure is at the
stage where the firmware applies the interact definition, not where the app reads it.  The firmware does its
read-modify-write over that word every frame and `openfpgaOS`'s `core_top` is the consumer; declaring an app field
inside it is a request to write core-owned state every frame, and this core will not come up when asked.

Encoded in `check_interact.py` as a hard rule: an app element may not be declared at `0xF7000000`, whatever its
mask.  Re-enabling app knobs therefore means editing the rule that records the measurement — which is the point of
having it.

## 3. What 0.1.35 installs

* Menu = `dist/sdk/Cores/ThinkElastic.openfpgaOS/interact.json` **verbatim** — 8 entries, no NIGHT element.  The
  base the user asked for, kept for good: it is the interact definition for the bitstream this port runs, and it
  carries `Analogizer 15kHz Timing` (id 61, bits 18-19) that the SDK template omits.
* 0.1.34's well-formed pair stays in `mk_dist.py` as the retry shape, moved to `0xF700001C` / `0xF7000020` so a
  retry cannot repeat the fatal shape.
* The bus read stays: `bridge_control_word()` logged at boot with its fields decoded, and every change logged with
  what the interact ecall reports for the same index in the same frame.  Pocket LCD is the calibration — it is the
  control the player can move and the core demonstrably acts on.
* Player settings, unchanged and all independent of the bridge: `night.ini` (`FIT=`, `MUSIC_VOL=`, `SFX_VOL=`,
  `SCREEN=`, `CDDA_OFF=`, `PAD_OFF=`, `TICK_HZ=`) and **L1 + B** to cycle panel fit live.

## 4. The measurement 0.1.35 is for

```
bridge: control word 0xf7000000 = <word> | fields music=<n> fit=<n> lcd=<n> free 5,8-9,14,20-23,29-31
bridge word=<word> music=.. fit=.. lcd=..            (only when the word changes)
  cross-check interact[5]: err=.. val=..
```

| outcome | meaning |
|---|---|
| the word is non-zero and **changes when Pocket LCD moves** | an app can read the menu the core already answers.  The host can then follow Pocket LCD (game / off / Terminal) and the video-offset sliders live — a real settings surface without owning a single menu element |
| the word reads 0 or never changes | even the bus keeps app-facing state to itself; the interact surface is closed for custom cores on this bitstream, and `night.ini` + pad combos are the documented answer rather than a workaround |
| the log appears at all | the menu is no longer the killer: 0.1.35 boots with the shipped definition, so the 0.1.27 / 0.1.34 deaths are attributable to the app elements and to nothing else |
| still no log | the killer is not the app elements, and the suspects become the 8-entry base itself (a reserved entry this core's slot map does not support) or something in `video.json` / `data.json` that changed alongside |

## 5. Housekeeping found on the card

`Saves/night/common/` had `._night_6.sav` and `._nightlog.sav` — macOS AppleDouble sidecars created when the Mac
writes to the card *after* the core created those files, not by `deploy.sh` (which sets `COPYFILE_DISABLE=1` and
checks the tree it copies).  Removed.  Worth remembering after any card-side file surgery from macOS: a `._` file
sitting in a save directory is noise the Pocket has to ignore.

## 6. Still owed, from this same log

The 0.1.32 run records `perf pres=16039us max=75567us` with `view=0 640x480`: a 16 ms average present on the native
640x480 source (the measured copy cost is ~15.7 ms) with 75 ms spikes, which is where the CDDA ring gets starved.
That is the next thing to attack after the menu question closes — the present cost on a 640x480 source is a
bandwidth problem, not a blit-bug problem, and docs/343's stride fix already removed the shearing.
