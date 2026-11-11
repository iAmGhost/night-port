# 99 — Kill-quota table is image data: one kill ending stage 1; plus stage-2 crash re-check and the hurt-state item

Amended/superseded: docs/109 (DAT_00053ea8 is DAT_00053e9c+0xc -- EXP per kill is 10, the port awarded 0
because of a duplicated unfilled array), docs/111 (the reaction-table lookup is unconditional; the port's
player/state gate was invented) and docs/119 (the fallen family is entered by atype 0x15..0x1f plus the
pick-up block at guest 0x00014de1+, not by hit spacing).


Playtest items 8 and 9 (and a re-check of 10), closed alongside docs/92-98.

## 1. "Defeating any single enemy advances to stage 2" — root cause

The shipped room/episode advance fires when `DAT_0006336c == 4` ("quota met"), set in the death
path at 0x00012a55: on a death with `byte[0] == 1` the kill counter `DAT_000639bc` increments
(0x00012a33) and the quota test is

```
quota = [DAT_00052a1c + 4*([DAT_00063354]+0x40) + 0x28*(DAT_00063370-1)]
if (quota <= kills) DAT_0006336c = 4
```

(Ghidra DumpQuota disasm 0x12a27-0x12a55; the port's fn_00012a55 translation matches it).

`DAT_00052a1c` was ported as a **zero-initialized global** ("no producer in the EXE" — wrong):
it is *image-initialised data* inside NIGHT.EXE itself. Evidence: the file bytes at
`0x52a1c - 0x7600 = 0x4b41c` (the data-section delta anchored by DAT_52da4 = file 0x4b7a4,
docs/51) dump to exactly the values read live from the original's memory through the container
API:

```
stage 1: all 1000          stage 2: 1000,25,1000...      stage 3: 1000,4,25,10,4,12,1000...
stage 4: 15,1000...        stage 5-8: all 1000           stage 9: 24,17,5,31,9,32,13,1000,1000,1000
```

(file dump == live read, byte for byte). Quota 1000 is deliberately unreachable, so most rooms
end by the exit rect (DAT_6336c==2) or the all-clear counters, not by quota. With the table all
zero, `quota(0) <= kills(1)` fired on the FIRST kill — one punch and the room/stage advanced.

Fix: the initializer (315 dwords from the shipped file) is copied into g_tab_52a1c at master-data
load (fn_03690b).

Evidence: `route_fight8.txt` with `NIGHT_HITTRACE=1` (`/tmp/dbgauto/pt_quota.log`): **17 kills,
17 HIT witnesses, and memstat stays `stage=1 room=1` for the whole run** (before: one kill left
the stage). test_f1 ALL PASS (3075 checks incl. docs/98 aliases).

Reproduce:

```sh
make -j8 build/night && SDL_VIDEODRIVER=dummy NIGHT_HITTRACE=1 ./build/night \
  --script ../scripts/playtest/route_fight8.txt --no-audio --frames 6000 | grep -E "HIT|stage="
python3 - <<'EOF'   # file side
import struct
exe=open('extracted/NIGHT.EXE','rb').read(); off=0x52a1c-0x7600
print(struct.unpack('<315I', exe[off:off+0x4ec])[:20])
EOF
```

## 2. "Enemy/Player state machine missing (hurt/stagger/fallen)"

The hurt/fallen machinery is ported (hurt-state maps in fn_0113e7/fn_01441e comments at f.c
0x24125, the player hurt branch 0x00033cd8-0x00033d6a at f.c:26474), but it is driven entirely by
successful hit resolution — and until docs/94 **no hit ever resolved** (box arrays indexed by raw
slot instead of the rec+0x6d frame number; DAT_63348 gauge pointer stored as heap offset). With
every attack missing, no entity ever entered hurt/fallen. Post-docs/94 the HIT witnesses show the
victim state/HP moving (e.g. `HIT ... dmg=4 hp 0->0`, docs/94: enemy 31->0 LETHAL, player 99->91)
and the fallen/lethal path removes the entity (fn_0003b67e, docs/95). Root cause = docs/94; no
separate state-machine bug found.

## 3. "Entering stage 2 crashes game (bus error)" — not reproducible on the current build

`./build/night --script route_slide_keyed.txt --warp-stage 2 --frames 12000`: rc=0,
`memstat stage=2 room=1` live from frame ~2630 to run end (5316 frames presented,
arena/RSS flat, no signal). The SIGBUS family (unaligned widened-pointer reads on arm64) was
root-caused and eliminated by docs/94/95 (heap-offset vs host-pointer representations). Item
recorded as fixed-by / not-reproducing on d9a6d4d; if it recurs it will carry a fresh core.

## 4. Note on the "no producer in the EXE" family

docs/98 (DAT_53e24 = slot-array cells) and this doc (DAT_52a1c = image data) share a lesson:
before declaring a DS global NULL-by-default, check `DS-0x7600` in NIGHT.EXE for static
initializers and read it live in the container. g_tab_52a1c was the only other zero-initialized
table involved in these playtest items; other such comments (e.g. g_tab_53200/532f4 seeded by
tests) remain governed by docs/83's contiguous-DS-block finding.
