# Installing the NIGHT Pocket core

`iamghost.night` is an openfpgaOS core for the Analogue Pocket.  It runs the same `src/f` game layer as the SDL port
(`build/night`), on the Pocket, from the original game's own data repacked into APF data slots.

The game data is yours: nothing in this repo ships NIGHT.RES or the disc audio.  Everything below regenerates it
from the `TG_NIGHT.CUE` you already have, the image its own `FILE` line names, and the `NIGHT.RES` of your
install set. `make extract` puts all three in `extracted/` (docs/455); `scripts/pocket/gen_disc.py`,
`repack.py` and `mk_dist.py` take them by suffix through `scripts/disc_loc.py` -- the repo root, a
`NIGHT/` under it, or the directory you run them from, and `NIGHT.RES` from `extracted/` first -- so a
rip kept anywhere works as long as the CUE is on that search path. `NIGHT_EXE` names a particular
`NIGHT.EXE` when a run has to say which copy it measured.


> **Upgrading clears your menu settings (docs/394 3).**  `scripts/pocket/deploy.sh` deletes
> `Settings/iamghost.night/Interact/` on purpose: a stale `interact_persist.json` once carried "Music Volume 0", which
> the peripheral mirror read as a real choice and the core played as silence.  After a deploy, the in-menu knobs (Music
> Volume, Sound Volume, Screen Fit) return to the `night.ini` values shown above.  Your `Input` and `Video`
> settings are left alone.

## 1. One command, on a mounted SD card

```sh
make -C pocket/src/night all      # the RISC-V core (app.elf) + the core tree + the generated headers
make -C pocket/src/night data     # scratch/pocket/: NIGHT.RES + one .bin per CDDA track (~600 MB, regenerable)
make -C pocket/src/night deploy   # core tree + slot files -> /Volumes/Pocket
```

`deploy` is the install step.  It writes

```
/Volumes/Pocket/Cores/iamghost.night/   core.json  variants.json  video.json  audio.json  input.json
                                         interact.json  data.json  loader.bin  os25.rbf_r
/Volumes/Pocket/Assets/night/common/    night.elf  os.bin  bank.ofsf  night.ini  README.md
                                         NIGHT.RES  track02.bin .. track15.bin
/Volumes/Pocket/Saves/night/common/     (created by the core: nightlog.sav, and only while `LOG=1`)
```

`make copy` is the SDK's own copy step and carries the **core tree only** -- no slot files.  Run `deploy`, and run it
last: `copy.sh` writes macOS AppleDouble sidecars (`._night.elf`) that `deploy` strips again.

Slot bindings are generated, not typed: `scripts/pocket/repack.py` + `scripts/pocket/mk_dist.py` write `data.json`,
`Assets/night/iamghost.night/NIGHT.json` and `src/cd/disc_table.h` from the CUE, and
`scripts/pocket/check_slots.py` proves the three agree and that no audio file sits in the firmware's save range
(slots 10-19).  `data.json` declares 28 slots with ids 0..31, inside Analogue's 32-slot ceiling; every filename is
under the Pocket's 23-character limit.  The table of filenames and slots lives in the deployed
`Assets/night/common/README.md` and is the thing to check a card against.

## 2. Settings

On the device, **Core Options** carries three settings -- Music Volume, SFX Volume, Screen Fit.  They are three
fields of one bridge word (`0xF700000C`, mirrored into the peripheral register `0x4000006c` that the core can read)
and they survive an eject and a reboot.  `L1+B` cycles the panel fit live.  Why the menu reaches the game through
that route, and why not through `of_interact_get()`, is docs/350 - docs/362.

The rest are developer keys in `Assets/night/common/night.ini` under `[night]` -- the deployed README lists every one
with its default.  The ones that matter for a report:

| key | ships as | set it to |
|---|---|---|
| `LOG` | `0` (no log file) | `1` to write `Saves/night/common/nightlog.sav`, the diagnostic trail the port was measured with |
| `DIAG` | `0` (a player's build performs no experimental bus probes) | `1` peripheral scan, `2` analogizer probe, `4` interact census |
| `FB_MODE` | `1` (native 640x480 surface, row memcpy) | Hardware-measured, and `0` is a trap: 307 200 B / 15 771 us = 19.5 MB/s against 76 800 B / 82 657 us = 0.93 MB/s for the host 2x2 downscale (~1 us per output pixel). `0` was shipped for two builds and brought the logo freeze back on hardware (docs/401, docs/404 §5) |
| `FIT` | `1` (stretch) | `0` letterbox, `2` the core scaler's 320x200 mode |
| `SCREEN` | `0` | `0` = the game alone, which is what ships; `2` = a nine-line diagnostic panel in white text over it (version, frame, view + picture signature, ticks/s and frames/s, present cost, palette and arena use, CD position, disc reads, ring headroom), and `1` terminal text only. `2` was the default once and cost a 1 Hz flicker over the game (docs/422), so the overlay is a switch, not the product |
| `TICK_HZ` | `490` | a cap on the game's fixed step rate, for slow-motion experiments |
| `SFX_GAIN` / `CDDA_GAIN` | `200` / `150` | 100-212 %, through a knee that cannot clip and cannot attenuate (docs/361) |

There are **no game saves**, and that is deliberate (docs/449): the shipped game has no save concept at all —
`src/f` names no save file and the shipped game ships none (docs/374) — so saves were the port inventing a feature, and the
masked restore that came with them only ever half-worked (HP bars restored, stage position and enemies did not).
They were removed in `0.2.23`, together with the `L1+Select` / `L1+Start` / `L1+A` combos that carried them; those
combos also pressed Esc and Enter on the game, so every save was a stray key into the shipped pause menu.
`L1+Up` (trace window) and `L1+B` (panel fit cycle) remain.

The data-slot manifest still *declares* save slots `10`–`16` (`night_0.sav … night_6.sav`) next to the
log slot `17`. **Nothing ever opens them.** They are there because the OS does not appear to attach a
save slot the save table never reaches: declaring only `17` made every write to the log succeed, from
the app's side, and land nowhere, for six boots (docs/454). The cores on the card that do write saves
(Quake, Doom) both declare `10`–`19` contiguously, so this is the shape the platform expects. A build
whose log slot sits below a hole in the table is a build that is blind.

The card ships **`LOG=0`**: no file is written, and a boot leaves nothing on the card behind. That is the
player shape and the default. Logging is a switch, not a state -- flip it with
`scripts/pocket/logswitch.sh on` before a diagnostic boot and `off` after (it prints what the card now
says, because `deploy.sh` regenerates `night.ini` from the dist tree and has quietly reverted hand-edits
before: docs/428). `LOG=1` writes `Saves/night/common/nightlog.sav` every 32 lines; a build with a
quarter-megabyte file being committed ~0.6 times a second is not the build a feel judgement is valid on
(docs/452).

## 3. Reporting a hardware run

Eject, boot, play, eject, mount, then

```sh
cp /Volumes/Pocket/Saves/night/common/nightlog.sav /tmp/nightlog.sav
grep -E '^(version|clock|memstat|perf|stages|budget|audio|view|video CHANGED)' /tmp/nightlog.sav | tail -40
strings /Volumes/Pocket/Assets/night/common/night.elf | grep -E '^0\.1\.'   # the version actually on the card
md5 -q /Volumes/Pocket/Assets/night/common/night.elf
```

The `budget` line is what carries the device's own SD-read timings (`disc reads=… mean=…us max=…us seeks=…
fails=…`) and the mixer's underrun counters; the desktop shim cannot produce the SD half of it (docs/364 §5).  A
screenshot from `Memories/Screenshots/` helps when the complaint is about the picture.

**If the complaint is the music, send the whole file rather than the grep.**  The `audio voice live=…` line added
in 0.1.89 is the only thing in the log that asks the mixer instead of the port's own model, and it has to be read
against the `audio cdda=` and `perf` lines around it (docs/408).  Note that `nightlog.sav` is a fixed-size slot:
new data starts at the beginning and the previous boot's tail survives below it, so look for the point where the
cumulative counters go *down* and cut there (docs/402 §1) — everything below that line is an older run.
For a music pause, the two fields that decide it are `e_live` (0 while the voice is sounding = the voice is reading silence) and `behind`/`ahead` (both near half the ring = healthy; `behind` near `RING` = the voice is about to run off the written frontier). See docs/409.

## 4. Measuring performance (two boots, one ini line apart)

The perf questions this port has open can only be answered on the device, because BRAM and SD cost are no-ops on the
desktop shim.  Set `LOG=1` in `Assets/night/common/night.ini` (the `budget`/`stages`/`perf` lines live in the log),
then run the same ~60 seconds of play twice:

```sh
# variant A: the shipping placement -- per-frame copy path in the 14 KB BRAM
cd pocket/src/night && make deploy

# variant B: tick_pump in BRAM instead, copy path back in SDRAM (docs/372 3)
cd pocket/src/night && make BRAM_SET=pump && make deploy
```

Compare `grep -E '^(stages|perf|budget)' nightlog.sav` between the two.  A third axis is `PUMP_AUDIO_TICK=1`, which
gates the board's DMA drain to the 490 Hz tick instead of every pump entry (docs/369 §5) -- judge it on `sfx=…/…/…`
and `dry` staying flat.  Whichever wins is what stays default.

The three disc levers need no rebuild at all, only an ini line (docs/375, docs/376), and they are read from the same
`budget` line's `disc ops=/sectors=/max_op=` and `cdda fills=` columns:

| key | default | what it changes |
|---|---|---|
| `DISC_BATCH` | 12 | sectors per slot read; 1 = one 2 352-byte read at a time |
| `DISC_MIN_WANT` | 12 | how long the pump waits for a chunk to be due; 1 = read whatever the disc clock says is due |
| `DISC_PREFILL_FIRST` | 150 | sectors read before a track's voices start; 300 = the old whole-ring prefill |

`scripts/pocket/ab.sh` sets these variants up for you (`diag`, `bram-pump`, `audio-tick`, `player`, `show`) and prints
the grep to run after each boot.

## 5. Desktop verification (no hardware needed)

The desktop shim now takes the rig's key lines directly (docs/377 §5):

```sh
./pocket/src/night/app_pc --frames 3000 --script scripts/playtest/route_show_dense.txt --rig-dump /tmp/run
```

Only `key` lines are honoured -- the anchored `wait res` lines belong to the SDL host's reader (docs/55) -- and keys
reach the game on the direct key path, so a route file can drive the Pocket code path without a warp.

```sh
make test                                    # the fixture suite against src/f (repo root)
cd pocket/src/night && make test             # app_pc: the same src/f under the SDK's SDL2 shim
NIGHT_CD_DIR=scratch/pocket/cd ./app_pc --warp-stage 1 --frames 2000 --rig-dump /tmp/b --rig-witness 0 | grep '^budget'
FRAMES=4500 scripts/pocket/sweep_episodes.sh && python3 scripts/pocket/sweep_report.py /tmp/pocket_sweep
python3 scripts/pocket/diff_hosts.py /tmp/xpoc/s1 /tmp/xsdl/s1          # Pocket shim vs SDL host, same route
```

Analysis and evidence: `docs/330` (budgets and the mapping) through `docs/372` (the BRAM sets), `scripts/pocket/`
(repack, mk_dist, deploy, check_slots, check_interact, sim_menu, sweep_*, diff_hosts), `scripts/analysis/gain_scan.py`
(the audio measurements behind the gain curve).
