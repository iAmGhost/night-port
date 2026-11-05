# 371 — the port used none of the 14 KB of zero-wait-state BRAM; here is what fits, and what does not

task-9's second item ("BRAM hot paths") had never been touched.  `pocket/src/sdk/app.ld:29` reserves

```
APP_BRAM (rwx) : ORIGIN = 0x00004000, LENGTH = 0x3800      /* 14 336 bytes, zero wait state */
```

and the device ELF's sections were `.text 503 376` in SDRAM with **no `.app_fasttext` at all** -- every hot loop was
being fetched from SDRAM through an 8 KiB instruction cache, which is exactly the situation `OF_FASTTEXT`
(`of_fastram.h`) exists to fix.  `of_fastram.h` also explains why the region is easy to waste: the SDK builds with
`-falign-functions=64`, fast RAM is *uncached*, so 64-byte alignment inside `.app_fasttext` is pure NOP padding
("Duke3D overflowed the region by 80 bytes on nothing but padding") and the linker script forces `SUBALIGN(8)`.

## 1. First attempt: the linker says no, loudly

Marking `present`, `tick_pump`, `palette_push`, `downscale_2x2_frame`, `drain_audio`, `cdda_pump`, `ring_write`,
`clock_consume`, `ring_ahead`:

```
ld: section `.app_fasttext' will not fit in region `APP_BRAM'
ld: region `APP_BRAM' overflowed by 6996 bytes
```

There is no partial BRAM: either the placement fits or the core does not link.  That is the good kind of failure -- a
BRAM experiment cannot half-work and quietly look like a win.

## 2. The number that decided the candidate list

Sizes measured from a `-O3`, no-LTO compile of the host (`nm -S`):

| function | size | note |
|---|---|---|
| `tick_pump` | 0x29f0 = **10 736 B** | 3.6× `present` -- because `audio_pump()`, `save_pump()`, `warp_entry_pump()` **and the whole 1 Hz diagnostic block inline into it** |
| `present` | 2 958 B | the copy + palette push + flip |
| `downscale_2x2_frame` | 1 852 B | 76 800 output pixels per present (docs/343) |
| `fill_sectors_raw` / `fill_sectors` | 600 B / 344 B | the 2352-byte slot reads |
| `ring_write` | 650 B | sample copy into the 4 s ring |
| `of_cdda_pump` | 1 428 B | |
| `palette_push` | 152 B, `clock_consume` 122 B, `drain_audio` 64 B, `cdda_pump` 56 B, `ring_ahead` 34 B | small and hot |

`tick_pump` is the port's hottest entry point -- docs/369 measured ~267 000 entries a frame -- and it is precisely
the one function that **cannot** be placed: 10 736 B is 75 % of the region, spent on a body whose bulk is
diagnostics rather than scheduling.  So the measurement turns into the next step rather than being forced now:
split the 1 Hz diagnostic block out of `tick_pump` (it is a well-delimited region) so the scheduling core is small
enough for BRAM.  Marking it as-is would have evicted everything else.

## 3. What ships: 11 380 of 14 336 bytes, ten functions

Verified by address in the linked ELF (`nm -S`, LTO build -- LTO changes sizes, so this is the shape that ships):

```
00004000  130  palette_push            00004b38  4692  present
00004084  1620  downscale_2x2_frame    000062e0   440  fill_sectors
000046d8   232  drain_audio            00006498  2012  cdda_pump
000047c0    26  ring_ahead             00005d8c  1364  fill_sectors_raw
000047dc   628  ring_write             00004a50   232  clock_consume
                            .app_fasttext 11380 of 14336 (79%), .text 493 072
```

The host layer only: `main_of.c` and `of_cdda.c`.  `src/f` is untouched -- an attribute in the shared engine would
have to exist in the SDL host build too, and the whole point of the guard below is that it must not.

## 4. The guard trap, recorded

`#ifdef NIGHT_POCKET` is not the right test: `sdk.mk` builds the **PC shim** with the same flag set *and* `-DOF_PC`,
so the section attribute reached the Mach-O assembler and every desktop build died with

```
error: argument to 'section' attribute is not valid for this target:
       mach-o section specifier requires a segment and section separated by a comma
```

in nine places.  The guard is now `#if defined(NIGHT_POCKET) && !defined(OF_PC)`, with `HOTTEXT` empty otherwise --
and the empty expansion is what makes the desktop path provably unchanged.

## 5. Inertness, and what cannot be claimed

Structural comparison against the pre-change run (stage 1, 2 500 frames, seed 1234): `meta.json` md5 **identical**,
417 rig records both, same record key sets (`ents/player/refused/tick`), `frames.bin` **3 081 244 bytes both**.  The
fixture suite is `== ALL PASS ==`.

What is *not* claimed: byte-equality of `frames.bin`/`records.jsonl` between runs.  docs/365 measured that animated
content differs between identical runs (6–48 % frame-signature differences), so those hashes differing here is not
evidence of anything.  The basis for "this change is inert" is that `HOTTEXT` expands to nothing on the desktop, the
link places code rather than changing it, and the structural witnesses above agree.

## 6. What only the device can answer

BRAM is a **no-op on the desktop build**, so no desktop number can show this working.  The decision is a hardware
comparison of the same route on 0.1.58 vs 0.1.59 -- the `stages`/`perf`/`budget present mean=` lines in
`nightlog.sav` -- specifically whether the per-present copy (`present`, `downscale_2x2_frame`, `palette_push`) and
the per-tick audio path (`drain_audio`, `fill_sectors*`, `ring_write`) get cheaper.  If they do not, BRAM placement
gets reverted rather than kept because it looks principled.

Card: **0.1.59**, `night.elf` md5 `8c969feed18ce9cfb222d08f9d031aaa` == the build, `.app_fasttext` 11 380 B at
0x4000, `night.ini` `SCREEN=2 DIAG=1 LOG=1 FB_MODE=1 PUMP_AUDIO_TICK=0`, no `._` sidecars.
