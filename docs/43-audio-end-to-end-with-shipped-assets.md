# Sound proven end-to-end with the shipped .voc data

Two things had to be separated: whether the **port's** sample pipeline works, and whether the **game**
ever asks it to play anything during a headless run.

## What the shipped data contains

All 100 sound slots have a size in the resource index: the boot loop's `fn_036626("1.voc".."100.voc")`
returns non-zero for **100 of 100** (measured: `voc slots: 100 of 100 have a size in the resource index`).
They are not ISO files — they live inside the resource container, which is why `--resnames` never shows a
`*.voc`. So the sound data *is* on the CD and *is* loaded into `DAT_00053d0c[100]`.

## The in-game trigger is an action opcode, not a key

`fn_04185e` has exactly three call sites:

| site | what it passes | measured in the host |
|---|---|---|
| `fn_03a829 + 0x322` (room loop, `0x3ab4b`) | `DAT_00053d08[event]` | entered 2×, cursor always NULL |
| `fn_0113e7` (`SND` action opcode, plus the per-type sound ids) | `DAT_00053d0c[...]` | never reached by a blind key script |
| `fn_03f6bc(name)` | named resource stream (e.g. `opening.fli`) | not reached in this run |

`DAT_00053d08` — the table the room loop indexes with the element-event id — is **never written anywhere in
NIGHT.EXE**: the image bytes are zero, the only xref is that single read at `0x3ab4b`, no instruction uses
a displacement in DS `0x13c80..0x13dc0`, and nothing takes its address. So the original really does call
`fn_04185e(0)` and survives purely because a DOS extender keeps linear 0 mapped: the first byte it reads is
the record terminator. The port models that page (`sys_linear0()` in `sys.c`, guard in `fn_040839`) instead
of faulting — this was a real SIGSEGV in the port as soon as the sound gates were opened.

## End-to-end measurement

`NIGHT_PLAYVOC=<1..100>` hands one shipped slot to the game's own player once (host scaffolding, not port
logic — the in-game opcode trigger needs real combat, which a scripted key sequence does not reach):

```
playvoc: slot 1 cursor=0x10426c12c first bytes=01 19 0e 00     # kind=1, 24-bit len = 0x000e19 = 3609
audio: playing at 48000 Hz (board samples start at 22050 Hz)
pushed=3607 pend=0 xfer=1        plays=3 real=1 null=2
```

That is the full chain on real assets: `.voc` payload → `fn_040839` (kind 1) → `fn_04097e` → DSP rate
command + the game's own ISA DMA programming → 3607 bytes pushed into the PCM ring → the SDL audio stream.
(The sandbox this was measured in has no usable output device, hence `SDL_AUDIODRIVER=dummy`; CoreAudio
refuses `AudioQueueStart` here. The counters that matter are the port-side DMA ones.)

## New host diagnostics

* pump line now reports `plays=N real=M null=K` (`fn_04185e` entries split by cursor validity);
* `NIGHT_PLAYVOC`, and the boot log line `voc slots: N of 100 have a size in the resource index`;
* the gameplay key script in `--autokey` (arrows/confirm/space pulses after t=12 s), which is what showed
  that keys alone never reach the `SND` opcode.

Suite unchanged and green: **2792 PASS / 0 FAIL**.
