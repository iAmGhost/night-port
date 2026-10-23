# 52 — episode sweep (`--warp-stage`), the stage-5 crash, and what the audio failure really is

## Host debug warp

`--warp-stage N` (`g_warp_stage` in `f.c`, applied at the top of `fn_037b03`) forces the stage number that
every per-stage name is built from (`stage%d.wal`, `st%ds.spr`, `stage%d.stg`, `night%d.chr`) and the spawn
tables are indexed by. It is a host-side knob only — the game never sets it — and it exists because real
progression through 9 episodes is not reachable from a short scripted route.

## Sweep, same route script, 1000 frames each

| stage | result | live entities | stage card seen |
|---|---|---|---|
| 1 | plays | 3 | stage1.pcx |
| 2 | plays | 4 | stage2.pcx |
| 3 | plays | 4 | stage3.pcx |
| 4 | plays | 4 | stage4.pcx |
| 5 | **SIGSEGV** after ~926 frames | 4 (reached gameplay) | stage5.pcx |
| 6 | plays | 4 | stage6.pcx |
| 7 | plays | 4 | stage7.pcx |
| 8, 9 | not run yet (sweep loop timed out at 1200 s wall clock) | | |

The per-stage record/type data is known independently from `scripts/decode_stg.py`:

```
ep segs recs   types used
1    4   88    1, 11, 31, 32, 36, 37, 41, 42, 46
2    4  184    2, 12, 13, 51, 52, 56, 57, 61, 62, 63
3    7  128    3, 71, 72, 76, 77, 81, 83
4    2   42    4, 53, 54, 58
5    1   28    5, 86, 87
6    4  203    6, 11, 31..48
7    3  197    7, 11, 31..49
8    1    2    8
9    8  207    9, 71..84
```

Two leads for stage 5 (and possibly 9): the extracted resource set is **missing** `enk-3.spr/.act` and all
of `enl-1..4` (the type table names them for indices 88 and 91-93, so those lookups fail and the loader
stores what the original also stores — NULL — which `fn_010efc` then dereferences, a latent original bug
the port has kept). Stage 9 uses types up to 84, stage 5 uses 86/87 (`enk-1`/`enk-2`, which do exist), so
the crash still needs its own diagnosis: under `lldb` the stage-5 run did **not** crash but stalled inside
CoreAudio (see below), which suggests the SIGSEGV may be downstream of an audio/DMA state difference
rather than in the stage path itself. Next step: run stage 5 with `SDL_AUDIODRIVER=dummy` under lldb to
separate the two.

## The audio failure is the host audio device, not the port's parameters

With a real (non-dummy) audio device the port's stream never gets samples
(`pushed=0 pend=0 xfer=0 snden=0 devinit=0 plays=2`), and CoreAudio's own log explains why:

```
HALC_ProxyIOContext::IOWorkLoop: the server failed to start, Error: 0x10000004
AQMEIO_HAL.cpp:2995  FetchAndLogFrameSize Error 2003332927 trying to fetch buffer frame size on device 76; assuming default of 512
[aqme] AQME Device-com.p5sys.jump.audio.driver_mic_v001: client stopping after failed start
[AMCP] HALC_ProxyIOContext::_StartIO(): Start failed - StartAndWaitForState returned error 268435460
```

i.e. the default output device on this machine is a **third-party virtual audio driver**
(`com.p5sys.jump.audio.driver_mic_v001`) whose CoreAudio server refuses to start, so opening the default
device yields exactly the class of error the report shows (`AudioQueueStart` 2003329396). The fix is not in
the game's DSP/DMA emulation: bring-up must (a) try the default device, (b) on failure enumerate other
devices and retry, and (c) fall back to a null sink that still advances the emulated DMA clock so the game
logic (which spins on the sound-ready flags) is not starved. `--audio-driver <name>` plus a fallback log
line is the plan; `pushed=…`/`plays=…` counters already exist to prove samples move once the device works.
