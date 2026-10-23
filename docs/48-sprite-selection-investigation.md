# 48 — why stage-1 enemies are drawn with the protagonist's art (open)

New host diagnostic `NIGHT_ENTDBG=1` (`ent_dump()` in `src/app/main.c`) names each live entity's type by
matching `entity+0x123` (the act-table value the constructor `fn_010efc` copies from
`g_dat_53818[type]`) back against the table, and does the same for the six sprite fields copied from
`g_dat_639ec[type]`.

Measured in stage 1 (episode 1, scripted route):

```
ent: count=3 tables loaded=10
  ent[0] type(act-match)=0  type(spr-match)=0  pos=1100,0      <- the player
  ent[1] type(act-match)=31 type(spr-match)=31 pos=35,0
  ent[2] type(act-match)=31 type(spr-match)=31 pos=35,0
```

`0` is the protagonist (`gun.spr`/`gun.act`). The port's transcribed per-type name arrays
(`kTypeSprNames` / `kTypeActNames`, 101 entries, image constants at DS `0x51d88` / `0x52198`) put
**`gun.spr`/`gun.act` at index 31**, and `ena-1.spr` at index 32 — so the two stage-1 enemies land on the
protagonist's art, exactly as reported. Two candidate causes, not yet separated:

1. the transcribed arrays are off by one (a run of `gun.*` "unused type" fillers shifted into the range
   where `ena-*` should start), or
2. the array is right but the *stage record file* yields type 31 where the original yields 32 (a width or
   stride slip in the record reader), and the same slip would also explain "the player is missing"
   (the player's spawn `fn_0376c8` publishes to `DAT_00053c80` only when `type == DAT_00063370`).

Note: only **10 of the 101** per-type act tables are loaded by stage 1 (the loader loads just the types
the stage's record file names), which is expected but means the loaded set is a direct fingerprint of
what the record reader produced — the `shot` lines of a run log list the exact `.spr`/`.act` names the
loader opened, which is the cheapest way to see whether `ena-1.spr` was opened at all.

Attempted and failed this turn: decoding the DS name arrays straight from `NIGHT.EXE` with the file-offset
mapping `file = DAT - 0x7600` (derived from `DAT_53334` ↔ file `0x4bd34`) produced garbage, so that base
is wrong for these DS addresses — redo it via the container/image base used by `scripts/extract_ds.py`
before trusting any hand-decoded name table.

## Resolved: the name tables are faithful — the type index coming out of the stage record is off by one

The DS name arrays are not far pointers: each entry is a **pool offset from DS `0x50000`**, and the pool
is packed (`gun.spr\0` = 8 bytes, `ena-1.spr\0` = 10). Decoding `0x51d88` (`.spr`) that way gives, in
order, the packed pool bytes

```
.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0 gun.spr\0
ena-1.spr\0 ena-2.spr\0 ena-3.spr\0 gun.spr\0 gun.spr\0 enb-1.spr\0 ...
```

i.e. entry 31 = `gun.spr`, entry 32 = `ena-1.spr`, 37 = `enb-1.spr` — byte-for-byte what
`kTypeSprNames`/`kTypeActNames` in `f.c` already contain. So hypothesis (1) is dead: the transcription is
correct and the port's stage-1 enemies really are spawned with **type 31**, which the original would also
draw from `gun.spr`.

That leaves the spawn side: the type value the port feeds to `fn_010efc` is one lower than the original's
(32 = `ena-1` for stage 1's first enemy). Next step is to trace where that index is produced — the stage
record loop in `fn_037b03` (the `night%d.chr` / record-file reader) into the `fn_010efc(block, type, ...)`
call — and check the width/offset of the field it reads (a byte vs word or a stride slip is exactly the
signature), plus the companion question of why the player entity (`fn_0376c8`, which only publishes to
`DAT_00053c80` when `type == DAT_00063370`) ends up absent.
