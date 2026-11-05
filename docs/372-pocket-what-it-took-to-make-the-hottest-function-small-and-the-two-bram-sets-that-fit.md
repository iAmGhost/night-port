# 372 — what it took to make the hottest function small, and the two BRAM sets that fit

docs/371 ended with the region full and the wrong thing in it: `tick_pump`, the function the game re-enters ~267 000
times a frame (docs/369), was 10 736 bytes -- 75 % of the 14 336-byte `APP_BRAM` -- because `-O3` had inlined
`audio_pump()`, `save_pump()`, `warp_entry_pump()` and the entire once-a-second readout into it.  This doc is the
fix, and the measurement that says where the region is better spent.

## 1. The extraction

`main_of.c:1673-1847` was a 174-line `if (now >= g_diag_next_us) { ... }` block: memstat, the view/signature lines,
the optional bus probes, the budget statistics, the terminal panel, the DSP-command census.  A brace-matched scan of
the block found its real dependencies on `tick_pump`'s locals to be exactly two -- `now` and `hz` -- and the 11 hits
on `d` were all `%d` inside format strings.  So it moved out whole:

```c
static void __attribute__((noinline)) diag_report(uint64_t now, uint32_t hz) { /* those 174 lines */ }
...
    if (now >= g_diag_next_us) diag_report(now, hz);      /* the only thing left in the caller */
```

`noinline` is the whole point: the diagnostics run once a second and belong in SDRAM with the cold code.

Sizes measured from a `-O3`, no-LTO compile of the host (`nm -S`), in the order the steps happened:

| step | `tick_pump` | newly separate |
|---|---|---|
| as shipped in docs/371 | **10 736 B** | -- |
| + `diag_report()` extracted | **7 934 B** | `diag_report` 4 418 B |
| + `save_pump` / `warp_entry_pump` marked `noinline`, `audio_pump` moved to its own BRAM section | **3 896 B** | `audio_pump` 2 640 B, `save_pump` 56 B, `warp_entry_pump` 310 B |

`tick_pump` is 2.75× smaller than it was, and nothing was deleted -- it is the same statements with the cold bulk
placed where cold code belongs.

## 2. The rule the region forced: cadence decides placement

14 336 bytes is a budget, so placement is now by **how often a function runs**, not by how important it sounds:

| cadence | rate | in BRAM? |
|---|---|---|
| pump entry (docs/369) | ~267 000 / frame | `audio_pump`, `drain_audio` -- and `tick_pump` itself under `BRAM_SET=pump` |
| present | 59 / s | `present`, `downscale_2x2_frame`, `palette_push` (the shipping set) |
| game tick | 490 / s | **no**: `cdda_pump`, `clock_consume`, `ring_ahead` |
| ring fill | ~1.4 / frame (2 743 fills in a 2 455-frame run) | **no**: `fill_sectors`, `fill_sectors_raw`, `ring_write` |

Dropping the per-tick and per-fill functions is what made room: they were the first thing marked in docs/371 and they
are now out.  Five functions with the shipping set:

```
.app_fasttext  9 376 of 14 336 bytes  (65%),  .text 494 032 -- 4 960 bytes of headroom
```

## 3. Two sets, both verified to link, one of them a build switch

Marking `tick_pump` alongside the present set overflows the region -- `overflowed by 1460 bytes`, which pins its LTO
size at about 6 420 bytes -- so the two ideas cannot both ship:

```
BRAM_SET ?= present          ; the per-frame copy path: present, downscale_2x2_frame, palette_push, audio_pump, drain_audio
make BRAM_SET=pump           ; tick_pump, audio_pump, drain_audio (+ palette_push), present path back in SDRAM
```

Both verified on this machine: **set `present` = 9 376 B, set `pump` = 9 324 B**, both fit with ~5 KB spare, both
build clean.  The mechanism is a compile-time macro pair (`HOTPRESENT` / `HOTPUMP` in `main_of.c`, `-DNIGHT_BRAM_PUMP`
from the Makefile), so the only difference between the two cards is one `make` variable.

Which one is faster is not knowable here: BRAM is a no-op in the desktop build, so no number this machine can
produce ranks them.  The claim is only that both are *measurable* now -- two boots, `LOG=1`, compare the
`stages`/`perf`/`budget present mean=` lines -- and if the pump set does not win, it stays unshipped rather than
default.

## 4. Inertness, checked as a census rather than a hash

The extraction had to preserve the diagnostic stream exactly, so it was compared line-type by line-type against the
pre-extraction run of the same route (stage 1, 2 500 frames, seed 1234):

| line type | before | after |
|---|---|---|
| `memstat` / `stages` / `perf` / `pump` / `term:` | 45 each | 45 each |
| `budget` (2 per stats cycle) | 90 | 90 |
| `audio` + `audio:` | 45 + 45 | 45 + 45 |
| `view=1` / `view=0` | 35 / 10 | 35 / 10 |
| `meta.json` md5 | `9ea6110e…` | same |
| rig records / `frames.bin` bytes | 417 / 3 081 244 | 417 / 3 081 244 |

Fixture suite: **3 089 `[PASS]` lines, `== ALL PASS ==`**.  Slot/menu guards: `== ALL PASS ==`.  `src/f` untouched.

## 5. Shipping state, and how to run the A/B

Card **0.1.60**, a player build again (the diagnostic staging in docs/370 was mine, not a shipping choice):
`SCREEN=0`, `DIAG=0`, `LOG=0`, `FB_MODE=1`, `PUMP_AUDIO_TICK=0`, `FIT=1`, `TICK_HZ=490`, `MASTER_VOL=255
SFX_GAIN=200 CDDA_GAIN=150`; `night.elf` md5 `e075e8e4ee57d63b420c59a1c0bc1d82` == the build; `.app_fasttext` 9 376 B
at `0x4000`; 22 unique ini keys; no `._` sidecars.

To measure the perf questions, in this order, one boot each:

1. `night.ini`: `LOG=1` (that is where the `budget`/`stages`/`perf` lines live).
2. Shipping card, play ~60 s, eject, `grep -E '^(stages|perf|budget|audio|disc)' Saves/night/common/nightlog.sav`.
3. `cd pocket/src/night && make BRAM_SET=pump && make deploy`, same play, same grep.
4. Optionally the same pair with `PUMP_AUDIO_TICK=1` to settle docs/369 §5 on hardware.

`SCREEN=2` (the panel over the picture) and `DIAG=1` remain available for a run that needs them.
