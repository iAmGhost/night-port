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

## What a working device exposed (the actual remaining bug)

With `SDL_AUDIODRIVER=dummy` the device opens and the game **stops making progress**: the run sits in a
busy-wait (pump calls ~4.2e8, presented frames stuck around 2700-3200, `DAT_63e40` frozen, `retrace`
frozen, `pushed=0 xfer=0 stops=0 snden=0 devinit=0 plays=5`). Without a device the same route reaches
`playing` normally.

This is not a regression from the ladder: the pre-change binary stalls identically with the dummy driver
(`frames=2668`, same frozen counters), so the trigger is *sound init succeeding*, not the new code. The
sound path the game enters once audio exists ends in a wait the port never satisfies -- no DMA transfer
is ever armed (`xfer=0`), so it is not a DMA-completion problem but a wait that is entered before any
transfer, on hardware the model does not answer. That is the next thing to find for this task: which
busy-wait it is (the pump diagnostic's `devinit=0 plays=5` is the breadcrumb), and what the original's
equivalent wait is released by.

Also still missing for the contract: CDDA. The sys layer models only the MSCDEX *stop audio* request
(`sys_cd_audio_stop`); there is no CD-DA play path, so no Red Book bytes can reach the host stream yet.
