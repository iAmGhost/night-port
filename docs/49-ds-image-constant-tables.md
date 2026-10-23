# 49 — two silent data slips behind "enemies wear the hero's face" and "the player is missing"

Both reported sprite problems were *data*, not draw code.

## 1. the per-type sprite/action name tables were shifted by one

`fn_037b03` loads, for every entity type a stage's records mention, `kTypeSprNames[type]` /
`kTypeActNames[type]` (image constants: DS dword arrays at `0x51d88` and `0x52198`, 101 entries).
Those dwords are **not far pointers** — they are offsets into a packed string pool based at DS `0x50000`
(`gun.spr\0` = 8 bytes, `ena-1.spr\0` = 10), so decoding entry *i* is
`str(DS 0x50000 + dword[i])`:

```
DS 0x501c0: .spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0
            ena-1.spr\0 ena-2.spr\0 ena-3.spr\0 gun.spr\0 gun.spr\0 enb-1.spr\0 ...
entry 30 = gun.spr   entry 31 = ena-1.spr   entry 36 = enb-1.spr   entry 41 = enc-1.spr
```

Each enemy family occupies a block of ten whose head is `gun` (`{gun, X-1, X-2, X-3[, X-4], gun, …}`),
so the family heads sit at 30/35/40/45… The port's transcription had **one extra `gun.spr`** before the
`ena` group, shifting every entry from index 31 onward by one. Stage 1 spawns types
`1, 11, 31, 32, 36, 37, 41, 42, 46` (verified by `scripts/decode_stg.py` over the real `stage1.stg`), so
the type-31/32/36/37/41/42/46 enemies all resolved to the *hero's* `gun.spr`/`gun.act`.

Fixed: both arrays re-transcribed straight from the image (`31 = ena-1 … 94 = enl-3`, 101 entries).
The stage-loader tests now use a type-31 record and pin that slot 31 receives `ena-1` while slot 32
(`ena-2`) stays empty.

## 2. four DS image-constant tables were still zero stubs

`DAT_00052c3c` is the **walk-line Y** the player and the room-edge spawns place entities on (10 dwords
per stage, 90 used: 172 almost everywhere, 162/160 on the raised platforms). The port had
`g_tab_52c3c[0x104] = {0}`, so the player spawned at world Y 0 — off the walk line, hidden behind the
HUD. Also still zero: `DAT_00052738` (per-ship table: 100..500 then an IEEE-float half, `0x3f000000` =
0.5f), `DAT_00053200` (61-entry timed-event tick table 229 … 1086) and `DAT_00052ffc` (65 dwords). All
four are now initialised from the image bytes; `DAT_00053d08` was checked and really is zero in the
image (runtime-filled), so it stays as it was.

Entity diagnostic (`NIGHT_ENTDBG=1`) before / after in stage 1:

```
before: player world=90,0    enemies world=35,0   (all three drawn from gun.spr)
after:  player world=118,139 enemies world=210,162 / 72,184   (hero vs ena-1 thugs)
```

Screenshots: `docs/manual-playtest/02-shots/01-playing-episode1-sprites-fixed.bmp` (hero in the middle,
two shirtless `ena-1` thugs) versus `00-screenshots/enemy_sprite_health_bar.png` (both "heroes", no
player).

## Regression test

`test_ds_constant_tables` (6 checks, registered first because other tests legitimately write these
tables afterwards) pins `52c3c` (172 / 162 / 160, 90 entries then zeros), `52738`, `53200`, `52ffc`.
Also new: `stage_slots_quarantine()/stage_slots_restore()` around the stage-loader fixtures — the
per-type teardown walks **every** slot 1..0x103 and frees what it finds, so a slot still holding an
offset from another test's arena faults.
