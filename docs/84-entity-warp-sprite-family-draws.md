# docs/84 — the host entity warp: every shipped sprite family now provably DRAWN

Goal task `warp-entity-census`. docs/80 established that every `.spr` family the sweep called "missing"
is in fact **loaded** inside its episode, and filed the reason as "scripted play never clears a room".
This pass closes the other half: the families are now **composited** (drawn) in the port, and each
composite stream is matched to a shipped `.spr` by content.

## 1. The host warp (`NIGHT_WARP_ENTITY`)

Three functions in `port/src/f/f.c` (declared in `f.h`):

```c
uint32_t     f_warp_entity_type(const char *name);   /* shipped per-type .spr/.act name -> type index, 0xffffffff if none */
const char  *f_warp_entity_refuse(const char *name); /* NULL = spawnable, else the reason */
uint8_t     *f_warp_entity_spawn(const char *name);  /* the shipped room-record opcode-1 build: fn_00041bdc + fn_010efc */
```

The name -> type map is the game's own `kTypeSprNames` / `kTypeActNames` tables (101 per-type entries),
not a host-side guess. `dust` -> type `0xfa` and `mark` -> type `0xc9` are the two shared types whose
per-type slots alias the DS globals (`DAT_000639ec[0xc9] == DAT_00063d10`,
`DAT_00053818[0xfa] == DAT_00053c00`).

The host hook is in `port/src/app/main.c` (the diagnostic block):

```
NIGHT_WARP_ENTITY=boss1,enc-1,dust   NIGHT_WARP_ENTITY_AT=600
```

Each name is requested once and **deferred with the reason logged** until the shipped preconditions
hold (same convention as the docs/78 `fn_showscreen_gate`): no live room record, or this episode never
loaded that type's `.spr`/`.act` blocks. Measured deferral line:

```
warp-entity: boss1 deferred at frame 603: no live room record (the room loop owns the entity list)
warp-entity: boss1 type=1 frame=2573 spawned
```

**Pacing lesson (repro below):** the room is *not* live at frame 600 — the stage card phase runs to
~frame 2570 — and a scripted route ends at a different frame per episode (ep1 ~6.1k, ep3 ~3.7k), so a
fixed `AT=6000` silently missed whole episodes (episode 3 drew nothing from the warp in the first
attempt). `AT=600` + the refusal-defer loop works everywhere.

## 2. Glyph traces for the four font-style renderers that had no trace

`NIGHT_GLYPHDBG=1` adds `menucell tag=…` traces (same line format as the existing menu/entity traces, so
the census parser reuses `cell_re`) to `fn_03df96` (stage sprite `stNs.spr`, tag `stagespr`),
`fn_0137c1` (countdown numbers + `exp.spr`, tag `expnum`), `fn_038b30` (5-digit number field, tag
`numfield5`) and `fn_03b718` (floating value display, `system.spr`/`system2.spr`, tag `valuemenu`).
Budgets: `NIGHT_ENTBUDGET=N` (total entity traces, default 120) and `NIGHT_ENTQUOTA=N` (per type,
default 40) — a census run tracing a dozen families at once needs a small per-type slice.

## 3. mark / dust were *unspawnable* — fixed by the alias fill

Before this pass the loader left `g_dat_639ec[0xc9]` / `[0xfa]` and the matching `g_dat_53818` slots
empty even though `DAT_00063d10` (mark) and `DAT_00063dd4` (dust) were loaded, so a type-`0xc9`/`0xfa`
actor resolved no sprite block and the warp refused it. The loader now fills the aliased slots
(`g_dat_639ec[0xc9] = g_dat_63d10`, `g_dat_639ec[0xfa] = g_dat_63dd4`, same for the action offsets).
Result measured: 100 `mark.spr` and 100 `dust.spr` composites in one episode-1 run
(`type=201 spr=mark.spr`, `type=250 spr=dust.spr`).

## 4. Content matching no longer invents name mismatches

The census used to attribute a composite to the **first** extracted `.spr` containing the probe and then
complain when the port's own per-type name disagreed. Measured with the full stream instead:
`boss4.spr`, `ene-4.spr`, `enj-4.spr`, `enj-1.spr` all **contain** the same first-frame bytes as an
alphabetically earlier sibling, so those four "mismatches" were an artifact of first-match order.

```sh
python3 - <<'EOF'   # scratch/res = the extracted .spr assets
def rd(n): return open(f"scratch/res/{n}.spr","rb").read()
for a,b in [("boss4","boss1"),("ene-4","ene-3"),("enj-4","enj-2"),("enj-1","enf-1")]:
    print(a,b, rd(a)[:48] in rd(b))
EOF
# enj-4 enj-2 True ... (boss/ene/enj-1 pairs share the frame further inside the file)
```

`scripts/playtest_asset_census.py` now collects **all** candidate files for the 48-byte probe, then
disambiguates with the whole stream inside that candidate set, uses the port's own name as the
tie-break, and counts ambiguity (`AMBIGUOUS(n)`, `AMBIGUOUS-NAMED(...)`) instead of hiding it. It also
reports `loaded` (the `NIGHT_RESLOG=1` lookup trace) beside `drawn`, and runs a phase-2 `ent<n>` pass
per episode whose warp list is that episode's own loaded set (with `EP_ENT` as the standalone fallback).

## 5. Family matrix (phase-2 `ent*` runs only, `scratch/census_ent.log`)

```
ena enb enc end ene enf eng enh eni enj enk   drawn
enl                                          never referenced  (no stage record names it)
st1s st2s st3s st4s st5s st6s st7s st9s       drawn
st8s                                         loaded only      (see §6)
mboss1 mboss2                                 drawn
mboss3 mboss4 mboss5 mboss6                  never referenced
boss1 boss2 boss3 boss4 boss5 boss6 boss7    drawn   (one per episode: ent1..ent9)
dust mark exp gage gun                       drawn in all 9 episodes
face over pause system                       loaded only (setup/status screens, docs/80 §)
```

Per-episode composites attributed to a shipped `.spr`: 580 for episodes 1-7 (episode 9: 576,
episode 8: 500), across 9-18 distinct shipped assets each; e.g. `ent3` draws
`boss3-1, enh-1, enh-2, eni-1, eni-2, enj-1, enj-3, dust, mark, exp, gage, gun, st3s`.
Unattributed streams in the whole phase-2 sweep: `AMBIGUOUS(3)=27, AMBIGUOUS-NAMED(None|2)=12,
stagespr=12, AMBIGUOUS(2)=6` — shared first frames and 12 glyph rows whose bytes live in a different
extracted asset than the font being swept.

## 6. Filed: `st8s` is loaded but never composited, because stage 8 has no sprite records

New one-shot witness in `fn_03df96` under `NIGHT_GLYPHDBG`:

```
ep1: stagespr state=2 stage=1 records=8 font=0x1043b49a4 rle=0x1a9c64 chars=0x18e678
ep8: stagespr state=1 stage=8 records=0 font=0x10036ff38 rle=0x18db28 chars=0x18db1c
```

`state=1` means the `.spr` font block *is* present (the RLE and character tables resolve), but
`g_dat_62a90` — the stage sprite-record count — is **0**. Those records are built by the stage loader
`fn_037b03` from **room-record opcode 5** entries (`port/src/f/f.c` 0x00037dec-0x00037e54: opcode 5
copies glyph/x/y/mode into `g_tab_5d0b4[count++]`). Episode 8's room record contains no opcode-5 entry,
so there is nothing to blit: shipped data, not a port rendering gap. Repro:

```sh
cd port && for s in 1 8; do NIGHT_GLYPHDBG=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout 400 ./test/night --warp-stage $s --script ../scripts/playtest/route_walk_fight.txt \
  --no-audio --shot-limit 0 2>&1 | grep "stagespr state"; done
```

Stage 8 is also the episode whose room chain unwinds early (docs/82 follow-up, task `stage8-early-unwind`);
the opcode-5 absence is a second, independent stage-8 data observation for that investigation.

## 7. Suite test

`test_warp_entity_shared_types` in `port/test/test_f1.c` (10 checks; suite total 2801 -> 2811):

* the DS alias identity (`DAT_639ec[0xc9] == DAT_63d10`, `DAT_53818[0xfa] == DAT_53c00`);
* the name -> type map (`enc-1`=41, `enc-2`=42, `ena-1`=31, `boss1`=1, `eni-1`=76, `enj-4`=84,
  `mark`=0xc9, `dust`=0xfa, unknown -> 0xffffffff);
* refusal before a room record exists, and refusal while the type has no blocks;
* spawn once both exist: type byte `0xc9` stored, `+0x127` sprite base and `+0x123` action table
  resolved **through the aliased slot**, entity count incremented;
* `enc-1` spawns with X inside the camera field and Y on the room's walk line (172 for stage 1).

## Repro

```sh
make -C port app
CENSUS_FRAMES=9000 python3 -u scripts/playtest_asset_census.py        # full sweep (phase 1 + phase 2)
python3 -u scripts/playtest_asset_census.py ent > scratch/census_ent.log   # phase 2 only (~10 min/episode)
./port/test/test_f1 | sed -n '/host entity warp/,/^$/p'               # the 10 new checks
make -C port test && make -C port selftest                            # ALL PASS / ALL PASS
```
