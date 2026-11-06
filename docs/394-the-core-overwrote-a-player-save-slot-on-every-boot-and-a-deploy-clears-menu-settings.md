# 394 — the core overwrote a player save slot on *every* boot; and a deploy clears the menu settings

A playtest on the real hardware (2026-11-06, core 0.1.71) left exactly the fingerprints a card keeps, and reading them
found a defect that docs/393 only half fixed.

## 1. What the playtest left on the card

Files touched at **14:09:04**:

```
/Volumes/Pocket/Saves/night/common/night_6.sav                       17 B   "NIGHT-PROBE-0001"
/Volumes/Pocket/Settings/iamghost.night/Interact/_core/interact_persist.json   935 B
/Volumes/Pocket/Settings/iamghost.night/Input/_core/input_persist.json         522 B
/Volumes/Pocket/Settings/iamghost.night/Video/_core/video_persist.json         236 B
/Volumes/Pocket/System/{lastcore,laststate,recent,cores_cache}.bin
```

`video_persist.json` records what the OS presented: `display_mode id "0x00"`, `integer_scale_mode: 1`,
`stretch_crt: false`.  No `nightlog.sav`: the boot ran the shipping `LOG=0`, so it recorded nothing about speed -- the
playtest was a playtest, not a measurement (docs/393's `ab.sh diag` is the measuring shape).

And `night_6.sav` was back.  I had deleted it an hour earlier, after docs/393 explained it as a leftover from the
docs/351 probe experiment.  It reappeared with the same 17 bytes at the moment of the playtest.

## 2. The defect: `file_probe()` writes to a player save slot at boot, unconditionally

The string is written at `main_of.c:2334`, in a function docs/393 never looked at -- so 0.1.71, which docs/393
presented as fixed, still did it:

```c
FILE *f = fopen("night_6.sav", "wb");     /* a game save slot, so the probe cannot touch the log */
static const char mark[] = "NIGHT-PROBE-0001";
size_t w = fwrite(mark, 1, sizeof mark, f);
```

The comment reasons backwards.  `night_6.sav` **is** instance slot 16, and `of_save.c` maps instance slots 10..16 to
`night_0.sav .. night_6.sav` -- so every boot truncated one of the seven player save slots and replaced it with a probe
marker.  Not a measuring boot: `file_probe()` was called at boot with no DIAG gate (main_of.c:2565), and it was in
0.1.71, the build the playtest ran.  The "log may fall back onto save slots" route was closed in docs/393; this one was
the actual writer.

Two changes:

- **the write half is deleted.**  What remains of `file_probe()` is read-only: `of_file_slot_find()` on five names plus
  a `readdir("/")` listing capped at 12 -- the file-layer facts the probe was ever useful for.
- **the call is gated**: `if (of_setting_diag() & OF_DIAG_FILEPROBE) file_probe();` with
  `OF_DIAG_FILEPROBE = 16` in `of_save.h`.  A player boot now performs no file probe at all, like every other probe
  since docs/362.

`grep 'fopen("night_'` over `pocket/src/night/*.c` returns nothing: the only writer of a `night_N.sav` name left in the
core is `of_save.c`'s own save path.

## 3. A behaviour worth knowing before upgrading: `deploy.sh` clears the menu settings

`scripts/pocket/deploy.sh:141-142`:

```sh
if [ -d "$SDCARD/Settings/iamghost.night/Interact" ]; then
    rm -rf "$SDCARD/Settings/iamghost.night/Interact"
```

It is deliberate -- the comment above it (line 137) records why: the card's `interact_persist.json` once carried
"Music Volume 0", which the mirror then read as a real choice, and the core behaved as if the player had silenced the
music.  Clearing the persisted interact state is the safe response to that.  The consequence is that **installing a new
core discards the player's menu choices**, which fall back to the `night.ini` values.  `Input` and `Video` settings are
not touched.  That trade is now stated in `pocket/INSTALL.md` rather than being something a reader discovers when their
volume settings are gone.

## 4. Card after the fix

Core **0.1.72**: `core.json` version == the version string in `night.elf`, card md5
`3fd8efc77978c900c4845faf986ef774` == `make dist` output, `night.ini` in the shipping shape (`SCREEN=0 FIT=1 FB_MODE=1
PUMP_AUDIO_TICK=0 RESTORE=1 DIAG=0 LOG=0`), `Saves/night/common/` empty -- the stale `night_6.sav` removed, and nothing
in this build can recreate it.  `make -C pocket/src/night test` (both halves) clean, repo `make test` `== ALL PASS ==`,
`src/f` untouched.
