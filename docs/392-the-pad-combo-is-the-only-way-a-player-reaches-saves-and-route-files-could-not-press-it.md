# 392 — the pad combo is the only way a player reaches saves, and route files could not press the key that does it

Docs/374 and docs/390 tested saves through command-line `--save/--load`, which is a harness affordance.  A player on the
Pocket has one route: **hold L1 and press Select** to save, **L1 + Start** to load, **L1 + A** to cycle slots,
**L1 + B** to cycle screen fit.  That path had never been run end to end.  It works -- and testing it exposed that the
route-file dialect could not express it.

## 1. The missing key

`pad_combo()` (main_of.c:1863) gates on `pad_script_held(OF_BTN_L1) || of_btn(OF_BTN_L1)`, and the scripted pad maps
route-file key names through `g_script_keys` (main_of.c:1515).  That table had enter, esc, arrows, modifiers, letters
and digits -- and **no F1**, which is the pad's L1 (`g_pad_keys`, main_of.c:1436: `{ OF_BTN_L1, 0x3b }`).  Numeric
scan codes are not accepted in the file dialect (`script_sc_for` matches names only), so `key 0x3b down` was silently
dropped and the whole combo was unreachable from a route file.

`{ "f1", 0x3b }, { "f2", 0x3c }` added.  The first version of this test wrote the file with numeric codes and reported
`0 keys, 7 lines skipped` with no save -- the honest record of why it looked broken before the table was fixed.

## 2. The combo works, and the save it writes is a live one

`scripts/playtest/route_combo_save.txt` (the docs/387 whole-episode route plus two overlapping spans):

```
39500  key f1 down
39650  key esc down        ; Select is Esc to the game, so the combo also opens the pause menu (docs/334)
39850  key esc up
40100  key f1 up
...
44000  key f1 down
44150  key enter down      ; Start
44350  key enter up
44600  key f1 up
```

Overlapping spans matter: `pad_combo()` reads L1-held and Select-pressed **in the same poll**, and the sequential
dialect cannot express that, which is why this test is written in the timed dialect.

```
save: slot 1 -> /tmp/svrt6/night_1.sav (192 B) stage=1 gauge=100/1000 state=00/00/00 resolver_calls=64
load: slot 1 <- /tmp/svrt6/night_1.sav stage=1 gauge=100/1000 state=00/00/00 saved_at_ms=320046115
load: restore mask=1
load: restored player record (stage=1 gauge=100/1000 state=00/00/00)
```

`resolver_calls=64` is the point: unlike the docs/390 save (`resolver_calls=0`, taken while the stage card was still up),
this blob captured a stage the game was actually running.  So the full player-facing chain -- save mid-stage, load it
back from the same pad, values applied under the shipping `RESTORE=1` mask -- runs on the Pocket code path, driven
through pad buttons rather than through `night_key_apply()`.

## 3. Card

The route-table edit changes the ELF, so the card was rebuilt and redeployed rather than left mismatched: core
**0.1.70**, `core.json` version == the version string in the ELF, card ELF md5 `502c664be5a41210e77fb552bcb07cb3`
identical to `make dist` output, `night.ini` at the shipping shape (26 keys; `DIAG=0 LOG=0 SCREEN=0 FIT=1 RESTORE=1
FB_MODE=1 PUMP_AUDIO_TICK=0 DISC_BATCH=12 DISC_MIN_WANT=12 DISC_PREFILL_FIRST=150 TICK_HZ=490 MASTER_VOL=255
SFX_GAIN=200 CDDA_GAIN=150 PRESENT_SYNC=0`), 14 track slots + `NIGHT.RES` deployed, no `.sav` files on the card.
`make -C pocket/src/night all` (both halves, docs/386 5) clean; repo `make test` `== ALL PASS ==`; `src/f` untouched.
