# 393 — the diagnostic log overwrote a player's save slot; and one command now reads everything a boot leaves on the card

`scripts/pocket/cardlog.sh` was written to make the hardware loop cheap (boot, play, eject, one command).  Running it
for the first time surfaced a data-loss bug.

## 1. What the card showed

```
== diagnostic trail ==
  --- /Volumes/Pocket/Saves/night/common/night_6.sav (17 B)
  | NIGHT-PROBE-0001
== save slots ==
  night_6.sav      17 B  (text or partial)
```

`night_6.sav` is **save slot 6** -- `of_save.c` maps instance slot ids 10..16 to `night_0.sav .. night_6.sav` -- and it
holds a diagnostic probe string from the docs/351 experiment.  A measuring boot wrote its log over a player's slot.

`of_log.c`'s own names explain it: `LOG_NAME_3` was `"night_6.sav"`, and `LOG_FALLBACKS` was `{ 16, 15, 14 }` with the
comment "game save slots stand in if none of those open".  The intended slot is 17 (`LOG_SLOT`, `save:7` /
`nightlog.sav`), which is separate from the game saves; the fallback was the hazard, and hardware took it.

## 2. The fix

- `LOG_NAME_3` is `"log.sav"`: a third bare name that is not one of the seven game-save names.
- `LOG_FALLBACKS` is `{ -1 }`, and the fallback loop now refuses any slot outside 10..16-with-guard and returns the
  documented no-slot path: `log: no writable slot; diagnostics stay on stdout only`.

So the diagnostic trail either gets a name of its own or stays on the terminal.  A save slot is the player's data and
is never a log target.  The stale `NIGHT-PROBE-0001` file stays on the card as the record of the old behaviour; it is
not a save blob (no `NGSV` magic) and `cardlog.sh` labels it "text or partial".

## 3. `scripts/pocket/cardlog.sh`

Everything a boot leaves behind, in one command, so a hardware answer needs no photograph:

- **core identity:** `core.json` version, the version string embedded in `night.elf`, and the card's ELF md5 against
  `pocket/build/.../night.elf` (the docs/366 consistency check, now automatic);
- **the ini the core ran with:** the shipping keys plus the A/B levers (`DIAG`, `LOG`, `SCREEN`, `FB_MODE`,
  `PUMP_AUDIO_TICK`, `BRAM_SET`, `PERIPH`, `DISC_*`, gains, `TICK_HZ`, `FIT`, `RESTORE`);
- **the diagnostic trail:** every log file found, printed inline (and a `NGSV` blob is called a blob, not a log);
- **save slots:** size and stage byte per slot, `NGSV`-checked;
- **screenshots:** the newest five the Pocket captured.

Its first run on the card read core 0.1.70 consistently (`0.1.70` in `core.json` == the ELF's string, md5
`502c664be5a41210e77fb552bcb07cb3` == the build), the 26-key shipping ini, one 17-byte trail, one slot, five
screenshots.

## 4. Card after the fix

Core **0.1.71**: `core.json` version == ELF string, card md5 `878ad3e385f96bfafdd1be9505827e40` == `make dist` output.
`make -C pocket/src/night test` (both halves) clean, repo `make test` `== ALL PASS ==`, `src/f` untouched.

## 5. Both halves of this verified, and the artifact moved off the card

`ab.sh diag`, `ab.sh show` and `ab.sh player` were each run (they had never been executed since being written -- the
same class of gap as the docs/388 sweep wiring): `diag` sets `LOG=1 DIAG=1`, `show` prints the card's version/md5/ini,
`player` restores `SCREEN=0 PUMP_AUDIO_TICK=0 RESTORE=1 DIAG=0 LOG=0`.  `cardlog.sh` reads all of it back.

The stale `NIGHT-PROBE-0001` file is kept as evidence at `analysis/legacy/night_6-slot-clobbered-by-probe-log.txt` and
removed from the card, so a future `cardlog.sh` run reports only trails the current core actually wrote.  The card ends
in the player shape: core 0.1.71, `core.json` == ELF string, card md5 `878ad3e385f96bfafdd1be9505827e40` == build, no
save slots, no log file.

