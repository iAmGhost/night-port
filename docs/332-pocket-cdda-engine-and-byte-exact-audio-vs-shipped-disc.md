# 332 — CD-DA on the Pocket host: the ring/voice engine, and what its witness proves against the shipped disc

Docs/331 built the host and proved the picture.  This doc is the music side: how a CD-DA track is sounded
through the openfpgaOS mixer from per-track data slots, and what its desktop witness says when it is
compared against the shipped `TG_NIGHT.IMG`.

Files: `pocket/src/night/of_cdda.c` + `of_cdda.h` (engine), `pocket/src/night/of_disc.c` (sector reads),
`pocket/src/night/main_of.c` (the seam the game asks for), `scripts/pocket/repack.py` (the slots),
`scripts/pocket/gen_disc.py` (the TOC the game reads).

## 1. What the game asks the host for

`fn_010b1d` resolves a *game track index* through the TOC it read from the disc-identity records
(`of_disc_root_header` / `of_disc_root_records`, docs/331 §5) and hands the host an LBA and a sector count;
`sys_cd_set_audio_backend()` (src/sys/sys.h) installs the host's play/stop pair, so the request surface is
exactly

```
play(index, lba, sectors)      /* "seek here, play this many sectors" */
stop()                         /* track end, or the game switching tracks */
pump()                         /* once per host pass */
```

Measured requests on the Pocket host (2600-frame run, `NIGHT_SEED=1234`, `NIGHT_CD_DIR=scratch/pocket/cd`):

```
cdda: index 1 lba=37262  sectors=3153  (42.0 s)   opening
cdda: index 9 lba=185120 sectors=27697 (369.3 s)  title
```

identical to the anchored schedule oracle in docs/96 §1 (`index 1 lba=37262 sectors=3153` opening,
`index 9 lba=185120 sectors=27697` title) and to the SDL host's own log line in the same run
(`cdda: track index 1 lba=37262 sectors=3153`).  The selection schedule is therefore reproduced, not
reimplemented.  docs/96 §3's `ep1..9 → 2,3,9,4,5,6,7,8,2` table lives in `src/f` and is shared, so the
stage-side selections come from the same code the SDL host runs.

## 2. Engine: two mono rings, two looping MUSIC voices

Pattern taken from openfpgaOS-Quake's `src/quake/engine/cd_of.c`, adapted to NIGHT's disc.

```
one mono ring per channel      88200 frames = 2.0 s, +1 guard frame past the loop end
allocated in the mixer pool    of_mixer_alloc_samples()  (2 * 88201 * 2 B = 352 804 B of the 8 MB pool)
two voices                     of_mixer_alloc_for_group_h(OF_MIXER_GROUP_MUSIC, ring, 88200, 44100, 0, 0)
                               of_mixer_set_loop_h(0, 88200)          /* looping, never restarted */
                               of_mixer_set_vol_lr_h(255,0) / (0,255) /* hard-panned pair = stereo */
music level                    of_mixer_set_group_volume(OF_MIXER_GROUP_MUSIC, vol)   /* one call = a fade */
```

Why a ring and not back-to-back chunks: the host runs on a pump (the spin loop at ~490 Hz = 2 ms).  Starting
the next voice at a chunk boundary can be up to 2 ms late, i.e. ~88 samples missing per second at 44.1 kHz —
a click every second.  A voice that loops a ring never stops, so the host only has to keep the written
region ahead of the voice's own cursor.  Hence:

* the ring is **prefilled with the full 2 s** before the voices start (a looping voice needs something to
  loop over, and prefilling proves the slot is readable before any sound is expected);
* afterwards the refill is capped by `room = CD_RING_FRAMES - ring_ahead()`, so the write cursor can never
  lap the play cursor;
* every written span is handed to `of_cache_flush_range()`, because the mixer's DMA reads memory the CPU
  wrote through its own (still dirty) 32 KB D-cache — the same requirement Quake's `cd_of.c` documents;
* the seam frame at index `CD_RING_FRAMES` mirrors frame 0 so a read that touches the loop end is valid.

The clock is the disc's own, kept even when nothing is being sounded:

```
budget += (now - clock_us) * 75 / 1e6           /* 75 sectors/s, the CD's own rate */
allowed = sys_cd_da_plan(pos, sectors, budget - pos, 4)     /* the shared cushion from src/sys/sys.c */
consumed = (now - play_us) * 44100 / 1e6        /* frames the voice side has spent */
```

Keeping the *clock* honest on the desktop matters because the game compares its tick counter against
`DAT_00053c48` (track length in seconds) when deciding that a track is done: a host that consumes a track at
the wrong rate changes the game's decisions, not just its audio.

Measured: 1013 sectors consumed in 18 s of wall time (900 frames), 1839 sectors in 24.5 s (1500 frames) —
75 sectors/s in both, matching the SDL host's `cdda=… pos=` counter growth in the same-length run.

## 3. Desktop witness

`OF_PC` (`make test` → `app_pc`) has no mixer service in the SDL2 shim, so the same engine runs against
ordinary memory and the samples leave as `<rig dir>/pcm_cd.wav` (44.1 kHz stereo s16 — exactly what the
mixer would have played).  `NIGHT_OFF=cdda` silences the whole path for fault-bisect (docs/331 §7).

Post-mortem worth keeping: the header fix-up (`fseek` to offset 4 and 40 to write the RIFF/data sizes) left
the file cursor at 44, so every later append **overwrote the span it had just written** — the file stayed at
`44 + 2352` bytes while the byte counter said 2.1 MB.  `wav_write_patch()` now saves `ftell()` and seeks
back; the symptom (a valid 44-byte header, a 2396-byte file) is the signature to look for.

## 4. Fidelity against the shipped disc

`app_pc --frames 900` with the repacked slots, witness 1050 sectors (1050 · 2352 = 2 469 600 B):

```
port == TG_NIGHT.IMG read from sector 37112 (track 2 INDEX 01):  True   (all 1050 sectors, byte-identical)
```

i.e. every byte the Pocket host would push into the mixer is the byte that sits in the shipped image at the
sector the game asked for, with the mapping `disc LBA → slot → byte` unbroken through
`fn_010b1d → sys_cd → of_cdda → of_disc → slot file`.

**Finding about the older audio basis.**  docs/06 §4 maps `game track k → scratch/cdda/track{k+1:02d}.wav`,
and those WAVs are *not* the disc bytes: `scripts/cdda2wav.py` slices `img[off+16 : off+2340]` per sector
("2324 usable PCM bytes per sector"), dropping 16 + 12 bytes of every CD-DA sector.  CD-DA sectors are raw
PCM with no sector framing, so that extraction loses 28 bytes per sector (7 samples per channel per sector)
and the loss accumulates as drift.  Measured against the port witness: sector 0 identical, then first
difference at sector 1 offset 1588, 1 identical sector of 1050, mean sample delta 0.9 over the first 2000
samples — the classic "right audio, wrong length" signature.  Neither host plays those WAVs (both read raw
sectors: `src/app/main.c`'s `g_cdda_raw[2352]`, `of_disc_read_audio()`'s 2352-byte reads), so nothing was
built on the lossy basis; docs/44's audio checks only read its header fields.  The Pocket path is the
faithful one, and this doc is the place that is stated.

## 5. SFX side (docs/98) in the same run

The board bring-up is the host's own, unchanged from `src/app/main.c`'s `board_boot()`: 0xaa signature
write, 0xf2 probe IRQ, 0xe1 id read, and the counters the game checks land as `snden=1 devinit=1` in the
host line.  DMA transfers drain into the mixer pool as mono voices (2-block ping-pong, 1024-sample chunks):
`xfer=18 plays=85` after 900 frames, `xfer=61 plays=284` after 2400 frames, `xfer=0 plays=0` before the game
brings the board up — the same gating the SDL host shows.

## 6. Budget check

| thing | size |
|---|---|
| two CD rings (mixer pool) | 352 804 B |
| mixer pool total | 8 MiB |
| voices used by music | 2 of 32 |
| voices used by SFX | 1 per active DMA block (2 blocks max) |
| per-sector staging | 2352 B static |
| heap arena peak during the opening (Pocket build, 16 MiB cap) | 2 366 912 B → 3 903 168 B |

## 7. Still open on this task

* The mixer calls themselves (`of_mixer_alloc_for_group_h`, `of_mixer_set_loop_h`, `of_mixer_get_position_h`,
  `of_mixer_pump`) compile into the RISC-V ELF but have never been executed — the desktop shim does not
  implement them.  Hardware confirmation of the ring/loop seam and of `of_mixer_get_position_h` as the
  refill governor belongs to the on-device pass (task-8/9).
* The `room`/`consumed` governor currently trusts wall time for the voice's position on the device; the
  first hardware run should cross-check it against `of_mixer_get_position_h` and report any drift as
  underrun (a repeating 1-sample click) rather than assume them equal.
* Music fades: the game's own volume changes reach the mixer only through `of_cdda_set_volume()`; whether
  NIGHT actually modulates music volume (rather than only starting/stopping tracks) is a docs/131 question
  that needs a scripted playtest, not a code read.
