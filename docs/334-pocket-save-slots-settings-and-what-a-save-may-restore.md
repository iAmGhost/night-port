# 334 — Save slots and settings on the Pocket host: what a NIGHT save can honestly contain

NIGHT has no save feature.  Nothing in the shipped code writes progress to disk (docs/06 §4, docs/65), so
this is a port-side feature: a **stage checkpoint** in the openfpgaOS sense -- a bound data slot the OS
creates on first write, plus the instance's `night.ini` as the settings file.
Files: `pocket/src/night/of_save.c` + `of_save.h`, wired into `main_of.c`; the slot bindings come from
`scripts/pocket/mk_dist.py`.

## 1. The slots

`mk_dist.py` writes them into both halves of the core, so they cannot drift:

| slot | file | bound where |
|---|---|---|
| 2 | `night.ini` | instance `NIGHT.json` + core `data.json` (OS config) |
| 4 | `NIGHT.RES` | instance + core |
| 8 | `bank.ofsf` | core (sample bank) |
| 10..19 | `night_0.sav` .. `night_9.sav` | instance `NIGHT.json`; `data.json` gives each `address 0x20100000`, `address_stride 0x40000`, `size_maximum 0x40000` = **10 × 256 KiB** |
| 20..33 | `trackNN.bin` | instance + core, one per CDDA track (docs/331 §5) |

Opening a slot is the rule openfpgaOS-Quake's `sys_of.c` records and `of_disc.c` already follows: bound
slots appear as flat names in the kernel's virtual root and are only identifiable through `readdir("/")`
with `d_ino - 1 == slot id`; opening a guessed `<dir>/night_3.sav` path can wedge the slot bridge, so it is
never tried.  The desktop build has no slot bridge, so its slots are ordinary files under
`NIGHT_SAVE_DIR` (default `scratch/pocket/saves`).

## 2. What the blob holds, and what may be written back

192-byte fixed record: magic/version/timestamp, `stage` (DAT_00063370, the game's own stage counter), the
DAT_63374..63384 mirrors, a 16-byte window of the player block (0x20..0x2f), the DAT_00063e88 held-byte
table, the fn_03272f invocation count at save time (proof the stage was live), and an FNV-1a checksum over
everything before it.

The **restore writes back the stage only** by default.  That is a measured decision, not caution:

```
NIGHT_RESTORE=1 (the two gauge words)  -> rc=138 Bus error: 10
NIGHT_RESTORE=3 (+ state bytes)        -> rc=138 Bus error: 10
NIGHT_RESTORE=5 (+ DAT_63e88 table)    -> rc=138 Bus error: 10
NIGHT_RESTORE=0 (stage checkpoint)     -> rc=0, 6000 frames, no fault
```

The reason is visible in the record head the save logs:

```
save: player record 0x00..0x3f: 01 01 05 a9 2d 2d 2d 2d 01 00 07 a9 a9 a9 bf a9
                                  a9 bf 01 00 07 aa aa aa bf aa aa bf 01 00 07 aa ...
```

That is a block of sprite/frame indices and arena-relative references, not the HUD gauge record:
`fn_01348f`'s "int16 gauge words at entity+0x23/+0x29" belongs to the HUD slot it draws, not to the head of
`g_player_ent`.  Writing those bytes into a live stage makes the game dereference out of range (bus error,
not a graceful mismatch), and a first version that copied 64 raw bytes died the same way on the arena
pointers the head carries.  A faithful mid-stage state injection needs the entity layout resolved first --
recorded as an open item, not papered over.

What *is* safe and useful is the stage counter, because the port already has a warp basis for it (docs/52,
docs/322): `g_warp_stage` forces DAT_00063370, and `fn_037b03` only runs once a game has started, so the
host pulses Enter -- 250 ms on, 250 ms off, exactly `src/app/main.c`'s `warp_entry_key` rule -- until
`f_act_resolver_calls() > 0`, i.e. the stage loop owns frames.  The Pocket host's pulse stops there, so a
restore never injects a key into gameplay.

## 3. Round-trip, measured on the desktop shim build

```
$ ./app_pc --frames 6000 --warp-stage 3 --save 0 --save-at 4500
warp entry: stage loop reached after 31 confirm pulses (fn_03272f calls 1)
save: slot 0 -> scratch/pocket/saves/night_0.sav (192 B) stage=3 resolver_calls=1978      rc=0

$ ./app_pc --frames 6000 --load 0
load: slot 0 <- scratch/pocket/saves/night_0.sav stage=3 saved_at_ms=200580737
warp entry: stage loop reached after 31 confirm pulses (fn_03272f calls 1)
frames presented: 6000  ticks: 53990  cdda tracks: 5  sfx voices: 105                      rc=0
```

The load side reads the blob, arms the warp from `blob.stage`, gets into the stage through the same entry
path, and plays on: 5 music selections and 105 SFX voices after the restore, arena 4 463 232 / 16 777 216.

In-game (device) the same goes through a held-L1 combo: `L1+Select` saves the current slot, `L1+Start`
loads it, `L1+A` cycles slots 0-9 (the slot is printed to the log).  L1 is F1 to the game and Select/Start
are Esc/Enter, so a save also reaches the shipped key state -- unavoidable on 11 buttons, and the reason
the combo is documented rather than hidden.

## 4. Settings

`night.ini` (instance slot 2) is what the chip32 loader reads for `[os]` and what this host reads for
`[night]`:

```ini
[night]
RES=NIGHT.RES
MUSIC_VOL=200        ; OF_MIXER_GROUP_MUSIC volume, 0-255
SFX_VOL=255          ; reserved for the SFX voice gain
CDDA=0               ; 1 mutes the CD music
PAD=0                ; 1 ignores the pad
```

`of_config_get("night", key)` is tried with the key as written and upper-cased, because the ini spells keys
upper case and the OS lookup is exact.  The desktop build reads the same knobs from `NIGHT_MUSIC_VOL`,
`NIGHT_SFX_VOL`, `NIGHT_CDDA_OFF`, `NIGHT_PAD_OFF`:

```
settings: music_vol=255 sfx_vol=255 cdda=1 pad=1
```

(`cdda`/`pad` name what to turn *off*, so their unset default is 0 -- an earlier version defaulted to 1 and
silently muted both.)

## 5. Packaging

`make dist` regenerates the core tree from the CUE (28 instance slots, 33 data slots), `make` assembles
`pocket/build/pocket/night/`, and the slot-bearing deployment is `scripts/pocket/deploy.sh <core> <data> <sd>`
-- the SDK's own `platforms/pocket/copy.sh` only copies the flat core tree and its SD detection needs a
mounted card, so `make copy` fails without one (`SD card not found`).  Verified with an explicit mount point:

```
$ bash scripts/pocket/deploy.sh pocket/build/pocket/night scratch/pocket_stub /tmp/sd_test
  + Cores  + Assets  + Platforms
  + NIGHT.RES -> common/ (slot 4)
  + track02.bin -> common/ (slot 20)
Assets/night/common/{night.elf,os.bin,bank.ofsf,night.ini,NIGHT.RES,track02.bin,README.md}
Assets/night/iamghost.night/NIGHT.json
Cores/iamghost.night/{core,variants,data,audio,video,input,interact}.json + os25.rbf_r + loader.bin
Platforms/night.json
```

The `.sav` files are not pre-created: the instance JSON binds them and the OS materialises them on the
first write -- the same arrangement openfpgaOS-Quake ships (its dist binds `s0..s9.sav` without shipping
them).

## 6. Card install, and the menu knobs that go with it

Installed on a real card (`/Volumes/Pocket`, 128 GB NTFS, 103 GB free, firmware 2.6 which clears the
`version_required: 2.2` the core.json asks for):

```bash
bash scripts/pocket/deploy.sh pocket/build/pocket/night scratch/pocket /Volumes/Pocket
```

The first run exposed a bug worth keeping in the record: deploy.sh located the target folder by scanning
the card for *any* `Assets/*/common`, and the card already had an Atari 2600 core -- so 600 MB of NIGHT
slots landed in `Assets/2600/common/`.  It now derives the platform directory from the tree being deployed
(`$OUT/Assets/*/common`) and refuses to continue if the core tree did not land.  After the move the card
carries, byte for byte:

```
md5 NIGHT.RES   09383a0c6e6b144ca93de2cd14c4e38b   (repo == card, 50 964 894 B)
md5 track15.bin 2b73bda1bee96c359bd361e2e85d81fe   (repo == card)
md5 night.elf   (repo == card)
Assets/night/common/{night.elf, night.ini, os.bin, bank.ofsf, NIGHT.RES, track02..15.bin, README.md}
Assets/night/iamghost.night/NIGHT.json              (slot 0 "Game", the Pocket's game-select file)
Cores/iamghost.night/{core,variants,data,audio,video,input,interact}.json + os25.rbf_r + loader.bin
Platforms/night.json                                (category "Computer", name NIGHT, year 1996)
```

`core.json` names one bitstream -- `{"name":"os25","filename":"os25.rbf_r","chip32_vm":"loader.bin"}` --
and `data.json` binds 33 core slots (0 Game, 1 os.bin, 2 night.ini, 3 night.elf, 4 NIGHT.RES, 8 bank.ofsf,
10-19 saves with `size_maximum 0x40000`, 20-33 the audio tracks), with the instance `NIGHT.json` binding
the same 28 files by name.

The Pocket menu's own settings page for this core is `interact.json`, and the app reads it through
`of_interact_get(index)` (index = definition order): index 0 `Music Volume` (slider 0-255, default 200)
and index 1 `Tick Pump Hz` (slider 60-490, default 490).  The menu values win over `night.ini` on the
device, except when every index answers 0 -- which is what the ecall returns when the interact bridge is
not wired -- and then the ini stands:

```c
int mv = (int)of_interact_get(0), tk = (int)of_interact_get(1);
if (mv != 0 || tk != 0) { g_music_vol = clamp_vol(mv); if (tk >= 60 && tk <= 490) g_tick_hz = tk; }
```

`Tick Pump Hz` may only *slow* the clock below the rate the game programmed (`sys_pit_hz(0)` = 490 Hz):
the tick pump takes `min(hz, cap)`.  That makes it the hardware tuning knob for task-9 rather than a way
to speed the game up.

`interact.json` index 2 is the `Pocket LCD` list openfpgaOS-Quake ships too (`On` / `Off` / `Terminal`,
address `0xF7000000`, mask `0xFFFCFFFF`).  `Terminal` switches the screen to the OS's 40x30 CP437 console,
which is where this core's `printf` diagnostics land on the device (settings, disc/track selection,
memstat, save/load reports) -- the way to read a hardware run without a card reader (of_terminal.h).

## 7. The first hardware attempt, and the 32-slot ceiling

The first card run stopped before the core started:

```
Instance Can't match slot ID
Error in framework instance load error
```

The cause was the slot plan, not the files.  Analogue's own data-json page states it flatly --
*"A maximum of 32 data slots is allowed"* (each slot takes a unique **16-bit** id, so large ids are legal,
the *count* is what is capped) -- and `mk_dist.py` was declaring **33**: the OS quartet (1-3), NIGHT.RES
(4), three never-bound spares (`Data 2` = 5, `Data 3` = 6, `Shared Config` = 8), `Sound Bank` (7), ten saves
(10-19) and fourteen tracks (20-33).  The instance binds 28 of them, so the core manifest was one slot over
the limit and the firmware refused to match the instance against it.

The spares are gone (settings live in `night.ini`, slot 2; a save needs no cfg slot), the plan is now 30 core
slots / 28 instance slots, and `mk_dist.py` refuses to generate a plan over 32 rather than letting it be
found on hardware again:

```python
if len(data_slots) > 32:
    sys.exit(f"data.json would declare {len(data_slots)} data slots; Analogue allows a maximum of 32 ...")
```

Then the plan was tightened one step further, because "32 slots" and "slot id 32" are easy to read the wrong
way -- and `id: 33` in a manifest looks like a bug even when it is legal (the id is a 16-bit label, not an
index: cores on this very card use ids 101, 103 and 330).  Every id now sits inside 0..31 as well, so the
manifest satisfies the strictest reading of the rule and the count has four slots of headroom:

| ids | what |
|---|---|
| 0 | `NIGHT.json` (the Pocket's game-select file, `required`) |
| 1-3 | `os.bin`, `night.ini`, `night.elf` |
| 4 | `NIGHT.RES` |
| 7 | `bank.ofsf` |
| 10-17 | `night_0.sav` .. `night_7.sav` (8 saves; `OF_SAVE_SLOTS = 8`) |
| 18-31 | `track02.bin` .. `track15.bin` (`of_disc.c`: `DISC_SLOT_BASE 18`, slot = track + 16) |

= **28 core slots, 26 instance slots, max id 31**, and `mk_dist.py` enforces both halves:

```python
if len(data_slots) > 32: sys.exit("... maximum of 32 ...")
if max(s["id"] for s in data_slots) > 31: sys.exit("a data slot id exceeds 31; ...")
```

Two other suspicions were checked and cleared, both against the working openfpgaOS-Quake install already on
the same card:

* **Missing `.sav` files are not the fault.**  Quake binds `s0..s9.sav` and ships none, and it boots; the
  same is true here (`inst ids not in core: []`, saves are `required: false`).
* **Subfolders and flat files under `common/` both resolve.**  Quake's track slots bind
  `cd/Quake (USA) (Track 11).bin` and the card really has `Assets/quake/common/cd/…bin`; our
  `track02.bin` sits flat in `Assets/night/common/`, which the OS also accepts.
* Key-for-key, our seven core JSONs now match Quake's JSON shape exactly (`core/data/video/audio/input/
  interact/variants`: no missing keys, no extras).
