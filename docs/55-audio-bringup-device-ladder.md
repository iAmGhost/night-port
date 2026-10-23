# 55 — audio bring-up: the device ladder, and what a *working* device exposed

## What the report says

`docs/manual-playtest/00-2026-10-23.md`: the port logs a CoreAudio failure at launch
(`AudioQueueStart` 2003329396) and no sound is heard.

## The host side, fixed

`audio_boot()` used to open the system default output device and give up on failure. It now walks a
ladder, each rung logged with the refusal reason:

1. `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK`;
2. every other playback device the host lists (`SDL_GetAudioPlaybackDevices`);
3. if none opens: no host stream at all, and `audio_pump` **drains the emulated board's DMA ring into a
   scratch buffer every frame** instead of returning early. The game's sample player waits on the *board*
   (its DMA transfer completing / the DSP accepting a stop), not on the host device, so the emulated DMA
   clock must keep advancing or `fn_04185e` starves the game logic.

A fourth rung -- quit the audio subsystem, set `SDL_AUDIODRIVER=dummy`, re-init -- was implemented and
**removed**: on this SDL3 the re-init never returns (the log stops at the last refusal and the run never
ends). An operator can still ask SDL for the null driver up front, which is how the counters below were
taken.

Measured on this machine (`SDL_VIDEODRIVER=dummy`, episode-1 route):

```
audio: default refused (CoreAudio error (AudioQueueStart): 268435460)
audio: alt 0 MPG321UX OLED refused (CoreAudio error (AudioQueueStart): 2003329396)
audio: alt 1 Mac Studio 스피커 refused (CoreAudio error (AudioQueueStart): 2003329396)
```

Every output device on this Mac is broken at the CoreAudio layer -- the same fault that made
`dosbox-staging` stall in SDL audio init until `SDL_AUDIODRIVER=dummy` was set (docs/46). So "no
CoreAudio error on launch" cannot be demonstrated here; what *is* demonstrated is that the ladder walks
all three devices and that the game keeps running with no device at all.

## Pinned by tests (7 new checks, suite 2819 ALL PASS)

`sys_audio_plan()` in `src/sys/sys.c` is the ladder's ordering as a pure function (default first, then
the listed alternates, otherwise the drain-only sink), and the same test drives the board protocol
through `sys_outb`: unmask DMA ch 1, program count `n-1`, DSP `0x40`+time constant, DSP `0x14`+block
length -> `sys_audio_transfers()==1`, `sys_audio_pushed()==16`, `sys_audio_fill()` returns exactly 16
signed 16-bit frames, DSP `0xd0` counted as a stop.

## What a working device exposed: the scripted route is not game-clock deterministic

With `SDL_AUDIODRIVER=dummy` the same route stops reaching `playing`. It is **not** a hang: `sample`
on the live process shows it spending its time in the host's own `input_pump` (main.c:672-676) and, on
the game side, in `fn_038098` (the frame flip), `fn_0347e3`, `fn_03a829`, `fn_03bb42` and **`fn_03af18`
-- the attract/menu loop** -- presenting at ~59 fps (frames 1471 in 25 s). `--frames 200` does not stop
it because the limit is only checked between game steps, and one menu-loop step keeps flipping frames.

So the difference a working audio device makes is *pacing*: the route script is clocked off the wall
clock, one device-refused run and one audio-live run tick at different rates, and the key events stop
landing where the menu loop expects them. The pre-change binary stalls identically with the dummy driver,
so this predates the ladder work.

Two things follow:

1. The harness must schedule script events off the **game clock** (the VBLANK/tick counters the pump
   already reports: `tick`, `DAT_63e40`, `pit=490 Hz`) instead of host wall time, otherwise every
   port-vs-original comparison is at the mercy of host-side overhead. That is the harness contract's
   "deterministically reaches title -> slide -> stage" clause, and it is the next harness fix.
2. The audio task's remaining real gap is not the host device at all: with audio up the game still never
   programs a transfer (`xfer=0`, `pushed=0`, `snden=0`, `devinit=0`), i.e. the sound path is not entered
   -- the enable byte `DAT_53338` is 0 and the board model is only installed with `NIGHT_BOARD=1`. Getting
   an effect measured means entering `fn_04185e` legitimately (board base + enable + the `0xf2` probe/IRQ
   handshake the port already models in `board_boot`).

Also still missing for the contract: CDDA. The sys layer models only the MSCDEX *stop audio* request
(`sys_cd_audio_stop`); there is no CD-DA play path, so no Red Book bytes can reach the host stream yet.

## The mute was a wrong initialized global (fixed, measured)

`DAT_00053338`, the sound/music enable byte that gates `fn_04185e`, was declared `= 0` in the port. The
shipped image initializes it to **1** (NIGHT.EXE byte at DS `0x53338`; the mapping cross-checks: DS
`0x53334` = 0x210 and DS `0x51a10` = 1 both match the port's defaults, and DS `0x53340` is the
"Stack Overflow!" text). Because `fn_04185e` returns at its `DAT_53338 == 0` guard, the port never reached
the DSP/DMA programming at all -- which is why every counter read zero.

Measured with the board modelled (`NIGHT_BOARD=1`), the anchored episode-1 route, and the pump
diagnostics:

```
audio=10989Hz pushed=220027 pend=0 xfer=61 stops=63 snden=1 devinit=1 plays=63 real=61 null=2
```

i.e. the game programmed **61 DMA transfers** totalling **220027 bytes** of 8-bit PCM at **10989 Hz**
(the DSP 0x40 time constant, not the 22050 Hz default), and the ring drained completely (`pend=0`,
`drained=440054 B` counted at the sink).

With a host stream attached (`SDL_AUDIODRIVER=dummy`, device open at 48 kHz):

```
dev=default submitted=11310 B stream=98684 B transfers=61 stops=63 rate=10989 width=1 pushed=220027
```

bytes do reach the SDL stream; delivery is capped by the latency bound (`stream > 96000 B` pauses the
pump), and the null driver does not consume at the rate the game produces, so only 11310 B of the
220027 B were handed over in this window -- on a real device the stream drains in real time.

A regression check now pins the default (`DAT_53338` starts 1). Worth adding later: a suite check that
cross-reads `NIGHT/NIGHT.EXE` and compares DS-resident defaults against the image (the class of bug this
was), skipping cleanly when the image is absent.

Still open for this task: CD-DA. The sys layer models only MSCDEX *stop audio* (`sys_cd_audio_stop`);
`fn_10b1d` (start track) and the Red Book byte path have no host backing yet.
