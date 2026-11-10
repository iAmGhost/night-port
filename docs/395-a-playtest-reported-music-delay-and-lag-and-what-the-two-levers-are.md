# 395 — a hardware playtest reported music delay and lag; the two levers, and a build switch that silently did nothing

> **Part of the CD-DA thread, 0.1.73 → 0.1.91.** Contemporaneous log of one step, kept cited by code.
> [**docs/412**](412-the-cdda-thread-in-one-record-from-0-1-73-to-0-1-91-what-the-music-pause-actually-was.md) is the consolidated record: causal chain, measurements, and the claims here that did not survive.


Reported after playing the 0.1.71/0.1.72 builds on the Pocket: **music delay after a few seconds, slightly laggy**.  Not
intended.  Nothing in the port aims at either, and this is the first time the two changes most likely to cause them have
been testable against hardware at all.

## 1. The two suspects, and why each is plausible

**BRAM placement (since 0.1.59, docs/371/372).**  `BRAM_SET=present` is the default and puts 9 376 B of the hot host
path in `APP_BRAM`.  It has **never been measured on hardware** -- docs/372's own words: "a placement that cannot be
measured on hardware is a preference, not an optimisation", and the plan was to revert if the device showed no gain.  It
is also not only a placement: `OF_FASTTEXT` (of_fastram.h:37) is

```c
__attribute__((section(".app_fasttext"), noinline, optimize("align-functions=4", "align-loops=4")))
```

`noinline` on ten hot functions -- `present`, `downscale_2x2_frame`, `palette_push`, `audio_pump`, `cdda_pump`,
`ring_write`, `fill_sectors`, `clock_consume`, `drain_audio`, `ring_ahead` -- defeats the `-flto=auto` inlining the rest
of the build relies on, and GCC documents the `optimize` attribute as intended for debugging, applying per-function
option changes that inhibit cross-function inlining.  "Move it to fast RAM" therefore also changed *how the code is
compiled*, which is exactly the shape of a "slightly laggy" report.

**The disc read policy (since 0.1.62-0.1.65, docs/375/376).**  `DISC_BATCH=12 DISC_MIN_WANT=12 DISC_PREFILL_FIRST=150`:
12 sectors (28 224 B) per read, a chunk gate before each read, and a 353 KB synchronous slice at track start.  Every one
of those numbers was measured on a Mac's SSD.  A "music delay after a few seconds" is the signature of the CD-DA ring
draining -- the ring is 4 s with a 1.6 s lookahead, so a device read slow enough to fall behind shows up exactly like
that, after a delay rather than immediately.

## 2. The levers, now actually working

| command | what it changes | rebuild? |
|---|---|---|
| `scripts/pocket/ab.sh bram-off` | `BRAM_SET=none`: nothing in `APP_BRAM`, no `noinline`/`optimize` on the host path, `LOG=1` | yes |
| `scripts/pocket/ab.sh bram-pump` | the docs/372 alternative set | yes |
| `scripts/pocket/ab.sh disc-simple` | `DISC_BATCH=1 DISC_MIN_WANT=1 DISC_PREFILL_FIRST=0` -- the pre-docs/375 shape every earlier build used, `LOG=1` | no |
| `scripts/pocket/ab.sh disc-fast` | back to the shipping 12/12/150 | no |
| `scripts/pocket/ab.sh player` | silent shipping shape | no |

Card as left here: **0.1.73 with BRAM placement removed** (`.app_fasttext` absent, `.text` 530 760 B), shipping disc
shape, `LOG=1 DIAG=0` so the next playtest is measurable, md5 `a2fe1c070bd369e7d379e6910f5d0e36` identical to the build.

## 3. Two build-integrity defects found on the way, both fixed

**`BRAM_SET` was not a build dependency.**  make does not track compiler flags, so changing the switch left the previous
objects in place.  My first stamp rule named `$(OBJ_DIR)/night/main_of.o`, which is not the object path (`sdk.mk:173`
builds `$(OBJ_DIR)/main_of.o`), so it did nothing: three consecutive builds with `pump`, `none` and `present` produced
byte-identical `.text`/`.app_fasttext` sizes for pump and none.  Fixed by removing the two objects that carry
`HOTTEXT` in the ab.sh variants that change the switch.  Verified by the section table, not by the flag: `present` puts
9 376 B in `.app_fasttext`, `none` has no such section.

**`ab.sh` wrote the ini before deploying.**  `deploy.sh` rewrites `night.ini` from the generated dist tree (docs/362), so
`LOG=1` set beforehand was silently discarded -- the card came back `LOG=0` and a "measuring" boot would have measured
nothing.  The rebuild variants now set the ini *after* the deploy.  (This is the trap docs/366 already warned about for
`DIAG`; ab.sh had walked straight into it.)

## 4. What the next playtest can already answer

With `LOG=1` the core writes its own lines to `nightlog.sav`; `scripts/pocket/cardlog.sh` prints them.  The two
symptoms have distinct witnesses:

- **music delay** -> `cdda`/`audio` lines: `audio_low`, `dry`, `room_hold` (underrun counters, docs/364) and
  `ahead_min` (the lookahead high-water, docs/376).  Any `dry` or `audio_low` above 0 says the reads are not keeping up,
  and `disc-simple` is the cross-check.
- **lag** -> `present mean=/max=`, `stages ... copy=`, `budget pump enters=` per frame.  Compare a `bram-off` boot with a
  `bram-pump`/default boot on the same stretch of the same stage; if the copy path does not get cheaper with BRAM, the
  placement is reverted permanently and the default `BRAM_SET` becomes `none`.
