# 374 — a loaded save used to restore nothing; the apply path was fine, the default was not

The objective lists "saves" as part of the port.  docs/334 built the slot mechanism, and a card-side save had been
written, but nothing had ever asked *what a load actually changes*.  It changed nothing.

## 1. What a save holds

`save_blob_t` (192 B, magic `NGSV`, version, FNV-1a CRC over everything before the checksum field): the stage number
(`DAT_63370`), the five stage-state words (`DAT_63374..63384`), a 64-byte window of the live player record, the key
table (`DAT_63e88`), and a timestamp.  A real mid-stage save at frame 1 500 of stage 1 captured live values, not
zeros:

```
save: player record 0x00..0x3f: 00 5a 00 00 ... 00 ac ... 00 22 ... 64 ... e8 03 ...
save: slot 3 -> /tmp/svrt/night_3.sav (192 B) stage=1 gauge=100/1000 state=00/00/00 resolver_calls=1049
```

Load checks all three guards (magic, version, CRC) and reads it back faithfully.

## 2. The defect, proven with a save I edited

To tell "restored" apart from "reported", the saved HP was changed out of band -- byte 41 of the blob (the live HP
byte) from 100 to 37, with the CRC recomputed so the load would accept it -- and the run then printed the *live*
player record after applying:

| `NIGHT_RESTORE` | live player record after load | run |
|---|---|---|
| **0 (the shipping default until now)** | `gauge=100/1000` -- the spawn value, the blob said 37 | clean, 873 frames, +491 ticks/s |
| 1 | `gauge=37/1000` -- the blob's value | clean |

So `of_save_apply()` works; `restore_mask()` returned 0 by default, and with mask 0 no branch of the apply runs.  A
player pressing the load combo on the Pocket got a log line and no change.

## 3. Which halves are safe, measured rather than assumed

Stage 1, 900 frames, the patched save, each mask:

| mask | what it applies | result |
|---|---|---|
| 1 | HP bars (`player[0x23..0x24]`, `0x29..0x2a`) | exit 0, `gauge=37/1000` restored, unwound normally |
| 2 | state bytes + `DAT_63374..63384` | exit 0, no visible change (the blob's state words are 0) |
| 4 | the `DAT_63e88` key table | exit 0 |
| 5 | gauges + key table | exit 0, `gauge=37/1000` |
| 7 | all three | exit 0 |

All five completed clean on that route -- but the older measurement in `of_save.c`'s own comment is that forcing the
stage selectors mid-stage makes the game dereference them out of range, and a clean run of one route does not retire
that.  So the shipping default is **mask 1** (the half that is proven to take effect and ran clean), and 2/4 stay
opt-in through `night.ini RESTORE=`.

## 4. Why the port has a save system at all

The original has none: `src/f` names no save file (its only data file is `exptab.dat`) and the shipped `NIGHT`
directory contains none.  The Pocket slots are therefore a port facility rather than a fidelity item, which is exactly
why what they may restore has to be bounded by measurement: the safe part is the player's HP, the risky part is the
stage's own state words, and the honest scope is "restore the player within the stage you saved in".

## 5. Shipping state

`of_setting_restore()` reads `night.ini RESTORE=` (env `NIGHT_RESTORE` on desktop), default **1**.  Verified after the
change, with no environment variable set:

```
load: slot 3 <- /tmp/svrt/night_3.sav stage=1 gauge=37/1000 ...
load: restore mask=1
load: restored player record (stage=1 gauge=37/1000 state=00/00/00)
```

Card **0.1.61**: `night.elf` md5 `73a5c73f96df6b3d5dca23df9ab23a67` == the build, `core.json` 0.1.61,
`night.ini` 23 keys, no duplicates, `SCREEN=0 DIAG=0 LOG=0 RESTORE=1`, README row added.  Fixture suite 3 089
`[PASS]`, `check_slots.py == ALL PASS ==`, `src/f` untouched.

## 6. Still open about saves

- Restoring **across stages** (the stage number is stored; loading a stage-3 save into a stage-1 run is untested).
- Restoring after death / at the save-menu moment the player actually uses.
- `RESTORE=2` and `7` beyond one route: the out-of-range-selector finding needs a multi-stage probe before either is
  offered as anything but a developer setting.
