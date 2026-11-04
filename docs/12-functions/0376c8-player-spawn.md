# 0x0376c8 — THE PLAYER (HERO) SPAWN (L15, 601 B)

Ported as `fn_0376c8(void)` in `src/f/f.c`. Together with `0x0374ad` (stage reset/reload) and
`0x037b03` (stage loader) this completes the stage-entry sequence:

| function | role |
|---|---|
| `fn_037b03` | load one stage's four resources (room chain, stage .spr, .stg records, night%d.chr type table) |
| `fn_0374ad` | full stage change: tear the stage down, reload it, seed the player entity's position |
| `fn_0376c8` | spawn the hero entity itself, choose the active key/catch/action tables, resume-load the rest |

## Sequence (capstone `scripts/vad.py dis 0x376c8 0x25a` + decomp agree)

```
fn_03d88f ()                    ; reset the clamp-bounds list (DAT_00063350, links at +0x34c)
fn_03237b (); fn_03ee0a ()      ; drain the two entity lists
block  = fn_00041bdc(0x147)     ; the actor block (0x14f in the port, widened links)
player = block ? fn_010efc(block, 0xffffffe2, 0,
                              DAT_00052c3c[roomSerial*4 + (stage-1)*0x28], 0, 0) : 0
             ; EDX = -30 (spawn kind), EBX = 0, ECX = the stage spawn-offset table entry
             ; the two stack args are both pushed as 0 -> entity type 0 ("gun"), kind 0
DAT_00053c7c = player           ; ** DAT_53c7c IS THE PLAYER ENTITY POINTER **
word[player+0x23] = 0
byte [player+0x28] = DAT_00063384
byte[player+0x28] == 0 ? dword[player+0x56] = (word)DAT_00053e9c        ; type-0 table entry +0
                       : dword[player+0x56] = (word)DAT_00052738[b*4]   ; per-ship table
word[player+0x29] = (word)DAT_00053ea4                                 ; type-0 table entry +8
DAT_00053c88 = 0 ; DAT_00053c84 = player
fn_037b03(1)                    ; EAX = 1 -> resume load (skips the room chain + stage .spr)
DAT_00063354 = DAT_00063358 ? DAT_00063358 : DAT_0006334c
DAT_00062a98 = (&DAT_00062aa0)[DAT_00063378]      ; active .key table
DAT_00062a9c = (&DAT_00062ab4)[DAT_00063378]      ; active catch table (same region, index 5+)
DAT_00053818   = (&DAT_00053c28)[DAT_0006337c]    ; active .act container
dword[player+0x123] = DAT_00053818 ; dword[player+0x2b] = DAT_00063374
byte [player+0x27] = DAT_00063378 ; byte[player+0x26] = DAT_0006337c ; byte[player+0x25] = DAT_00063380
fn_03cad3(DAT_00063354)
DAT_00063398 == 0 ? ( dword[player+1] = DAT_000639dc - 0x1e , dword[player+0x1b] = 0 )
                  : ( dword[player+1] = DAT_000639dc + 0x15e, dword[player+0x1b] = 1 )
dword[player+9]  = DAT_000639e0 + DAT_00052c3c[DAT_00063354[0x40]*4 + (DAT_00063370-1)*0x28]
dword[player+0x17] = 1 ; dword[player+0x11] = 0
```

The `+0x1b` field is the horizontal/vertical orientation flag that `fn_034635` (the keyboard-state
handler) also reads — the same flag `fn_0374ad` derives from the room node's `+0x11` byte.

## Port-model notes

1. **`DAT_00053c7c` is the player entity pointer**, not a "config block". Earlier ports treated it as
   an opaque kb-config block because `fn_034635` writes `[+0x1b]`; here it is assigned the freshly
   constructed actor block, and every field written (`+1`, `+5`, `+9`, `+0x11`, `+0x17`, `+0x23`,
   `+0x25..+0x29`, `+0x56`, `+0x123`) is an entity field from `fn_010efc`. Tests must give it a block
   of at least `0x14f` bytes (the older `0x20`-byte stand-ins still work because only low offsets
   are touched).
2. `DAT_00053e9c` and `DAT_00053ea4` are read as scalars here, but they are entry 0 of the
   entity-type table; the port reads them through the `g_dat_53e9c` array view (`+0`, `+8`) instead of
   adding duplicate globals.
3. `DAT_00062aa0` / `DAT_00062ab4` are indexed as one array (`(stage weapon set) * 4`); the port keeps
   the ten named `.key`/`.catch` globals, so it rebuilds the DS order locally — `0x62aa0..0x62ac8` is
   exactly ten slots, and `DAT_00062ab4` is slot 5. Same for the three `.act` containers
   `DAT_00053c28..0x53c30` selected by `DAT_0006337c`.
4. `DAT_00052c3c` (per-stage spawn offsets, 10 dwords/stage indexed by room serial) and
   `DAT_00052738` (per-ship dword table indexed by entity `+0x28`) have **no producer anywhere in the
   EXE** — they are read in only four functions (0x0374ad, 0x0376c8, 0x03af18, 0x03d244) and both live
   above the file-backed image (VA > 0x4e8d7). They are modelled as zero-initialised seedable globals
   (`g_tab_52c3c`, `g_tab_52738`) with the open question recorded: the values must come from the CD
   overlay/`.IMG` data, so wiring the real backend is what will fill them.
5. `DAT_00053818` (now the per-type action-table array) gets element 0 = the selected `.act`
   container, stored as a heap offset, and mirrored into `player+0x123` — matching `fn_010efc`.

## Test (`test_fn_0376c8`, 10 checks)

Runs on the same synthetic `NIGHT.RES` stage set as `test_fn_037b03` (stage1.wal / st1s.spr /
stage1.stg with **zero** records / night1.chr), after `fn_037b03(0)` has built the room chain and the
entity-type table. Added setup for this path: a `0x3c` stand-in for `DAT_639ec[0]` ("gun.spr", which
`fn_010efc` dereferences for type 0), three `.act` container blocks in `DAT_53c28/2c/30` so
`DAT_6337c = 1` has something to select, `DAT_63374/78/7c/80/84` seeded, and `g_tab_52c3c[0]` /
`g_tab_52738[3]` seeded.

Coverage: the entity pointer lands in both `DAT_53c7c` and `DAT_53c84`; `+0x28` = `DAT_63384` and
`+0x56` = the **low half** of `DAT_52738[+0x28]` (`and edx,0xffff`); `+0x23` cleared and `+0x29` =
`DAT_53ea4`; `DAT_53c88` cleared and `DAT_63354 = DAT_6334c` when `DAT_63358` is empty; the key/catch
table selection (index 2 -> `DAT_62aa8` / `DAT_62abc`); `DAT_53818[0]` = the chosen container offset
mirrored at entity `+0x123`; the `+0x2b/+0x27/+0x26/+0x25` mirrors; the orientation branch (`+1`,
`+0x1b`); `+9 = DAT_639e0 + DAT_52c3c[...]`; `+0x17 = 1`, `+0x11 = 0`.

Test-authoring traps hit here (worth remembering):
* the `.wal` room header must satisfy `node[0x1a] = (+8) - (+0)` — with the field values swapped the
  geometry size became `0xffffffff * 0xc8` and `heap_malloc` aborted inside `fn_03c853`;
* `(word)DAT_52738[x]` after `and edx,0xffff` is the **low** half: `0x123456 -> 0x3456`.

## Playability status

With these three functions ported, the code path "stage number -> rooms + type table + hero entity"
exists and is tested against synthetic resources. What is still missing for the "a stage loads"
milestone is data, not code: the CD/`.IMG` backend (`TG_NIGHT.IMG`) that supplies `NIGHT.RES` and the
real `DAT_52c3c` / `DAT_52738` tables.
