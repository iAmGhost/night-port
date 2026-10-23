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
| 8 | **was a port deadlock**, now plays | 2 | stage8.pcx + the picture show `na80001.pcx` |
| 9 | plays | 1 | stage9.pcx |
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
rather than in the stage path itself. The two are **separate**: with `SDL_AUDIODRIVER=dummy` the crash reproduces at the same point (after
926 presented frames), and the faulting site is exact:

```
* thread #1, stop reason = EXC_BAD_ACCESS (code=1, address=0x0)
  frame #0: night`fn_03cc6b(a0="") at f.c:8480:28
```

`f.c:8480` is `if (g_dat_639dc == *(uint32_t *)c)` with `c = rec_ptr_get(a0, 0x34c)` — the room-chain
`next` link is NULL and the state machine dereferences it (the original's unguarded read, kept). Stage 5
is a **one-segment** stage (`ep 5: segs 1, recs 28, exit NONE (kill-clear)`), so at the room boundary the
`next` link of the only room is 0 — which means the port reaches this state with a NULL next-room where
the original does not. Next step: at the fault, compare `g_dat_6334c` / `g_dat_63354`, `g_dat_639e4` and
the room count `g_dat_639d8` for stage 5 against the `.wal` (1 segment) to find which of the chain/serial
state differs; `NIGHT_ENTDBG` already dumps the entity list and camera.

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

## Stage 5 resolved: a latent original quirk that is only fatal on the host

With the NULL link logged instead of dereferenced, stage 5 plays to the end of the scripted route
(`episode_start` / `stage_play` / `playing` at 4 entities, 8166 frames presented) and the diagnostic fires
once:

```
[room-chain] fn_03cc6b got a NULL next link: a0=head=cur=0x1026984d8 state639e4=0 roomCount=1 stage=5 camX=960 camY=0
```

`a0` **is** the chain head and the current room (`roomCount == 1`), so `a0+0x34c` — the room-chain `next`
link — is legitimately 0 for a one-segment stage, and `fn_03cc6b` dereferences that link in *both* arms
(`*(uint32_t *)c` for the world-X test and `c->0xc` for the camera-Y bump). On real DOS that pointer would
be whatever the allocator left there — mapped memory, so the two comparisons simply fail and the state
machine is inert. On the host it is address zero, hence SIGSEGV. So this is not a port logic slip: it is a
latent original quirk whose consequence is host-only.

Port deviation, marked in the code: when the link is NULL, log once and return (the inert outcome the
original would have had). The alternative — feeding a self-reference — would invent comparison operands the
original never had, so it was rejected.


## Stage 8 was a missing cooperative pump in fn_03a70e (fixed, 2026-10-24)

Stage 8 did not crash and was not gated by content: it **deadlocked**. A `sample` of the stalled process
showed the main thread inside `fn_03a70e` (the picture / CG slide-show driver), whose first wait was

```c
for (;;) {                                    /* 0x0003a789-9f key wait */
    if (g_dat_63ec1 != 0 || g_dat_63ea4 != 0) break;
    if (g_dat_63e89 != 0) break;
}
```

with no `sys_port_spin()`. On DOS those bytes are updated asynchronously by the keyboard IRQ and the
pump; in the single-threaded port nothing else runs, so the wait can never end -- the frame counter
freezes at the first picture (`na80001.pcx` never even appeared in the resource log, which is why the
anchored route looked like it was "waiting too long"). The key-*release* wait a few lines below already
pumps; this one had been missed.

After adding the pump, `scripts/playtest/route_show_keyed.txt` (anchored on the show's own resource
names, then paced key presses) takes stage 8 through the show and into gameplay:

```
shot 0090 na80001.pcx    frame=2497 res=na80001.pcx entities=2
script: anchored on res 'na80001.pcx' at t=46012ms (rebased -6000ms)
shot 0095 episode_start  frame=4139 ... shot 0096 stage_play ... shot 0097 playing  entities=2
```

Stage 9 plays too (`episode_start` -> `stage_play` -> `playing`, 1 live entity). So all nine episodes now
enter, render and accept control through the warp. Note the show family is `na<stage>NNNN.pcx`, a
different asset family from the `<stage>s<N>/<stage>e<N>.pcx` cards of docs/58 -- two separate
intermission mechanisms.
