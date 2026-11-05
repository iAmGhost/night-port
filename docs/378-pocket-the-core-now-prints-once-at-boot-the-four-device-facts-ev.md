# 378 — the core now prints, once at boot, the four device facts every remaining decision waits on

Everything still open on this port is gated on a number the chip has and the desktop shim does not.  Rather than a
probe build per question, the host asks the OS the whole set once at boot and writes it to the log.

## 1. Why these four

| question still open | the value that answers it |
|---|---|
| Is async slot prefetch buildable at all, and at what chunk size? (docs/375 §4 named this as the precondition) | `of_file_async_max_read()`, `of_file_dma_stage_size()` |
| Did the display claim land, and is the stride the blit copies with the stride the OS reports? (docs/343, docs/331 §7) | `of_video_get_caps()`, `of_video_get_mode()`, `of_video_get_mode_count()` |
| Is the 32-voice mixer pool being exhausted mid-stage -- the shape of docs/339's double-voice fault? | `of_mixer_voice_active()` across all 32 |
| Did the BRAM placement survive the link, on the device? (docs/371, docs/372) | the runtime address of `present`, against `app.ld`'s `APP_BRAM` window `0x4000..0x7800` |

The desktop shim cannot stand in for any of them.  Its own header says so: `of_mixer_alloc_samples()` is `malloc`
(`of_mixer.h:426`) and `of_mixer_voice_active()` returns 0 (`of_mixer.h:391`).  So the probe is compiled only for the
device (`#ifndef OF_PC`), and it is written with `NLOG`, which a player's card does not put in a file at all
(`LOG=0` ships, docs/362).

## 2. What it prints

```
sdk video: caps max=WxH stride<=S frame<=F B modes=0x… physical=320x240 default=320x240 | applied WxH stride=S color=C mode_count=N
sdk limits: async_max_read=A B dma_stage_size=D B | mixer voices active=V | BRAM present@0x… in_bram=1
```

The last field is already known on the link side -- the shipped ELF carries code in the region:

```
$ riscv64-elf-objdump -d --start-address=0x4000 --stop-address=0x4010 pocket/.obj/night/app.elf
    400a:  86ba        mv   a3,a4
    400c:  60070613    addi a2,a4,1536
```

but the runtime line is what makes a `BRAM_SET=present` vs `BRAM_SET=pump` A/B interpretable: if the copy path is not
at an address inside the window, the experiment measured nothing.

## 3. Shipping state

Card **0.1.67**: `night.elf` md5 `67c4a199bad0694d9a74a85bcb5c2ec6` == the build, both targets compile clean
(RISC-V and the SDL2 shim), fixture suite 3 089 `[PASS] == ALL PASS ==`, `src/f` untouched.  `scripts/pocket/ab.sh`
now greps `^sdk` alongside the perf lines, so the recommended hardware session is:

```sh
scripts/pocket/ab.sh diag      # LOG=1, and it prints the grep to run after the boot
```

and that one boot answers the geometry question, the mixer-pool question, the async-feasibility question and the
"was the BRAM experiment real" question, in addition to the `disc ops/sectors/max_op` and `cdda fills` read costs the
previous two docs were written to obtain.
