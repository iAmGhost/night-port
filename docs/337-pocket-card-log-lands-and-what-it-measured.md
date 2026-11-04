# 337 — the host log lands on the card, and what the first long one measured

Cores 0.1.12 through 0.1.14.  Two card runs, one dead end closed, three mechanisms found.

## 1. Card writes were never broken — two slots shared one file id

The first device log came back as `Saves/night/common/track03.bin` (9 574 B, kept at
`analysis/legacy/20261105-night-first-card-log.txt`) because data.json declared **id 19** nonvolatile (the
"Log Save" I had added) while the instance manifest binds id 19 to `track03.bin`: the firmware committed the
dirty save window under the *other* name.  With id 17 claimed instead (`nightlog.sav`, opened as the SDK alias
`save:7`) the second run wrote `Saves/night/common/nightlog.sav`, 71 008 B, plus `night_6.sav`, 17 B, which is
the boot probe's own payload:

```
log: opened "save:7" (the SDK save alias)
root: "os.bin" ino=2 slot=1 ... "night_6.sav" ino=17 slot=16, "nightlog.sav" ino=18 slot=17,
       "track02.bin" ino=19 slot=18, "track03.bin" ino=20 slot=19
root: 26 entries; slot 17 found
probe wrote 17 then closed
probe readback 16 "NIGHT-PROBE-000"
```

`readdir("/")` reports the *implied slot id* per entry, `of_file_slot_find()` resolves every declared name, and
a write/read round trip returns the bytes.  Slot 8 is no longer declared at all (README:462 reserves it for the
SDK).  Final map: saves 10-16, log 17, tracks 18-31 — 28 data slots, 26 instance bindings, no duplicate ids.

## 2. The terminal was the menu, not the port

```
settings: music_vol=255 sfx_vol=255 cdda=1 pad=1 tick_hz=490 screen=0
screen: ini/menu=0 display_mode=1        (1 = OF_DISPLAY_FRAMEBUFFER)
```

The earlier run read `menu lcd=00020000` — the interact bridge carrying Pocket LCD = **Terminal** — and 0.1.10
let the menu override `night.ini`, so the framebuffer was never shown.  `night.ini` now pins `SCREEN=0`; the
menu only takes the switch if the ini says `SCREEN=-1`.

## 3. What the memory system actually gives, measured at boot

```
bench read=9082us      307 200 bytes read once, cold
bench read2=8907us     the same read again (warm: SDRAM is not cached here, so no help)
bench copy=8627us      memcpy of the same 307 200 bytes
bench write=4776us     memset of 307 200 bytes into the heap
bench downscale=24113us one 640x480 -> 320x240 pass on the logo frame
```

Sequential SDRAM reads run at about 34 MB/s.  The 24 ms downscale is *that* frame — the logo is largely
uniform, and the uniform 2x2 block early-out skips most of the work.  Over varied content the same stage
reports `copy=83100us`, and `blit fast=255 slow=0` proves it is the word loop running, not the fallback.  So
the loop's per-pixel work, not the bus, is what costs 80 ms.

## 4. Skipping an unchanged frame is worth 40x, and it moved the wall to the flip

| | 0.1.10 (skip off) | 0.1.13 (skip on) |
|---|---|---|
| `copy` median / max | 80 342 / 80 391 us | **2 008** / 83 100 us |
| `pres` median / max | ~87 500 / 90 626 us | **13 769** / 93 053 us |
| ticks gained per second | +12 | **+349 median** (max +494) |
| `flip` median / max | ~12 545 us | 9 066 / 33 430 us |
| frames skipped | 0 | 3 072 of 6 529 |

The game's 640x480 framebuffer sat unchanged for whole seconds (`sig=ecd967f7 chg=1`), so re-downscaling it 11
times a second was pure waste.  With the skip on, the remaining cost concentrates in `flip` — which is what
`of_video_wait_flip()` (PRESENT_SYNC) waits for — so 0.1.14 ships `PRESENT_SYNC=0`: the flicker got *worse*
with the wait enabled, and the wait was added on a guess, not a measurement.

## 5. The buzz has a mechanism: 53 ms of slack against an 88 ms stall

```
audio cdda=492/17856/3 plays=2 ring=2352 drift=-7053
```

`ring=2352` is 53 ms of fresh audio in a 2 s ring.  The pump only read as far ahead as the *disc clock*
(`sys_cd_da_plan` capped it at the elapsed sector count), so the ring never held more than the clock cushion —
and one 88 ms video present then drains it and the looping voice runs into data that was never refreshed.
0.1.14: `CD_LOOKAHEAD_SECTORS = 120` (1.6 s read past the disc clock), `CD_CHUNK_SECTORS` 4 -> 12 (fewer,
larger SD reads), `CD_RING_SEC` 2 -> 4, and the pump now takes the *smaller* of the clock-model room and the
mixer voice's own room (`of_cdda_head()` reports that lead in the log) instead of trusting the clock alone.

## 6. SFX were being thrown away, not played quietly

`plays=5` on the card against `plays=193` on the desktop shim over a comparable span, with `xfer=108` RES
transfers queued.  The host had **two** SFX buffers and a rule of "if that block's voice is still active, drop
the sample": the second voice of any overlap lost.  Now eight buffers, and the host searches for any free one
before dropping; the log counts `drops=` and prints the first eight plays with the handle the mixer returned
(`voice=-1` would mean the mixer refused the sample outright).

## 7. What the next run has to answer

- `bench downscale=` against a busy scene, and `copy` median: is 80 ms the loop or the frame?
- `flip` with `PRESENT_SYNC=0`, and whether the flicker goes away with the wait gone.
- `audio ... drops=` and `sfx#N ... voice=`: are the sounds dropped, or refused?
- `head=`: does the write cursor keep its 0.1-2 s lead over the voice through the intro?
