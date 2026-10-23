# Why the port is silent: the shipped data leaves the sound board unconfigured

The audio model (DSP + 8237 DMA + SDL stream, `docs/39`) never produced samples in the host run
(`pushed=0 xfer=0`). Measuring the two gates the game itself checks explains it, and the answer is in the
shipped image rather than in the port.

## Gate 1 — `DAT_00053334`, the device port base

`fn_040d09` starts with `g_dat_53334 = fn_040f4f()` and bails on `if (g_dat_53334 == 0x210) return 0;`.
The shipped data segment initialises **DS 0x53334 = 0x210** (NIGHT.EXE file offset `0x4bd34`,
`bytes 10 02 00 00`), i.e. the value `fn_040f4f` returns when the controller is "not ours". The port had
initialised that global to 0, which only changed *where* the scan failed (eight probes of ports 0x00-0x70,
since the scan advances `g_dat_53334 += 0x10` at most eight times and never reaches 0x220).

Fixed: `g_dat_53334 = 0x210` with the provenance comment, matching the image. Consequence, stated plainly:
**with the data as shipped the board path is never initialised** (`DAT_00063f90` stays 0) and
`fn_04185e` returns immediately, so this sample path is inactive in a default run.

## Gate 2 — `DAT_00053338`, the sound enable byte

Written only from the title-menu sound option inside `fn_039c03` (`f.c:24611` seeds
`f[3] = 1 - DAT_00053338`, `f.c:24655` assigns `DAT_00053338 = 1 - f[3]`), so with the shipped default the
option is **off** until toggled.

## Host switches that exercise both paths

* `NIGHT_BOARD=1` — `board_boot()` models the controller the way the unit fixture does (base 0x220, the
  0xaa discovery signature on `base+0xa`, status `0x80`/`0x00` on `base+0xe`/`base+0xc`, the game's own
  vector-0x0a handler bound, and the device interrupt fired from a command hook when the `0xf2` probe is
  written). The whole chain then runs in the host: `base=0x220 -> irq probe=2 -> id=43690 (0xAABA) ->
  buf=0x106a... -> initialised`, and the pump line reports `devinit=1`.
* `NIGHT_SOUND=1` — sets `DAT_00053338 = 1`, mirroring the menu toggle.

With both, `snden=1 devinit=1` but still `plays=0`: the sample player is entered only from the room loop's
element event (`DAT_00063364 != 0`, `f.c` room loop), which the headless autokey run does not produce
before it dies in the room loop. Next step for sound is therefore the element-event side (what sets
`DAT_00063364`, i.e. the `.stg`/script trigger path), not the audio backend.

Suite unchanged and green: **2792 PASS / 0 FAIL**.
