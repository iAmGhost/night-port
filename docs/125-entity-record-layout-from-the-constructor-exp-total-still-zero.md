# 125 — Entity-record layout taken from the shipped constructor, not guessed; field-level census then flags the EXP item again

## 1. Where the layout comes from

`disassemble_function(0x00010efc)` (the shipped entity constructor) is a flat list of stores through the
record pointer. Its store set *is* the record layout, and the widths are explicit:

```
byte   [+0x00]                       dword [+0x01] [+0x05] [+0x09] [+0x0d]
dword  [+0x11]  byte [+0x15] [+0x16] dword [+0x17] [+0x1b] [+0x1f]
word   [+0x23]  byte [+0x25] [+0x26] [+0x27] [+0x28]
word   [+0x29]  dword [+0x2b]  byte [+0x30] [+0x31]
dword  [+0x46] [+0x4a]  byte [+0x4e] [+0x4f]  word [+0x50]  dword [+0x52] [+0x56]
word   [+0x23]  byte [+0x5a]  dword [+0x5f] [+0x63] [+0x6b] [+0x6f] [+0x73] [+0x77] [+0x7b] [+0x7f]
dword  [+0x8b] [+0x8f] [+0xab] [+0xaf] [+0xbb] [+0xc7] [+0xd3] [+0xff] [+0x103] [+0x107] [+0x10b]
dword  [+0x113] [+0x11f] [+0x123] [+0x127] [+0x12b] [+0x12f] [+0x133] [+0x137] [+0x13b] [+0x13f] [+0x143]
```

Two things follow immediately:

* The layout is **packed and unaligned by construction** (`dword [EDX+0x1]`, `[EAX+0x17]`, …). That is the
  origin of the ~390 unaligned-access UBSan sites in docs/123: they are semantically required by the shipped
  layout, so the only correct remediation is host-side safe accessors — never re-aligning the record.
* My earlier `ENTITY_FIELDS` widths were guesses and wrong (state as a word at `+0x17`); they are now set from
  this store set (`state` dword `+0x17`, `hp` word `+0x29`, `exp_total` dword `+0x2b`).

## 2. The layout validates against shipped runtime values

Census over all 1296 live shipped samples of the player record (pointer cell `0x53c7c` → `state_NNNN.heap.bin`),
`/tmp/rig/g2`, route `ep1_kill3`:

| field | offset | shipped values |
|---|---|---|
| `type` | `+0x00` byte | `0x0` ×1291, `0x1` ×9 |
| `state` | `+0x17` dword | `0x1`, `0x2`, `0x28`, `0x33` |
| `hp` | `+0x29` word | `0x3e8` (1000) ×1206, decaying `0x3e5`, `0x3dd` |
| `exp_total` | `+0x2b` dword | `0x0` ×741 → `0xa` ×212 → `0x14` ×332 |
| `pos_y` | `+0x73` dword | `0x44c` ×1287 |

`hp` decaying and `exp_total` stepping 0 → 10 → 20 is exactly the semantics the disassembly predicts, so the
offsets/widths are proven, not fitted.

## 3. What the same read gives on the fixed port build (open item, EXP family)

Same fields on the port's own `g_player_ent` record block (`rec_offset` from `meta.json`, same route):

```
hp         +0x29 word : 0x3e8, 0x3e4                      (shipped: 0x3e8, 0x3e5, 0x3dd)  comparable
exp_total  +0x2b dword: never 0xa / 0x14 -- no EXP ladder in any live sample
exp_award  +0x32 dword: 0x5                                (shipped reads 0x36 at that offset)
```

and the DS side that the port uses to seed the field is **zero on the shipped side too**
(`0x63374` = `0x0` in all 1300 samples), even though the shipped *entity* field climbs to 10 and 20 — so the
shipped total is accumulated at some site other than the constructor copy
(`f.c:5314 *(uint32_t *)(g_player_ent + 0x2b) = g_dat_63374`, read back at `f.c:8978/8993`).

Filed as open: docs/109 fixed the *table alias* (`DAT_00053ea8 = DAT_00053e9c + 0xc`) so the award path is no
longer dead, and `exp_award` is now non-zero on the port; but the **player record's EXP total at `+0x2b` still
never reaches the shipped 10/20 ladder on this route**. Next root-cause step is to enumerate the guest sites
that write `+0x2b` (xref the record offset through the flat-model store, not the DS symbol) and compare against
the port's writes, plus resolve whether `+0x32`'s shipped magnitude (`0x36`) is a per-kill award or a running
value.

## 4. Rig corrections made while getting here (so the record is straight)

* The "shipped side reads zero / heap-relative pointers" blocker was my own probe bug (`DS_LO`), see docs/124.
* `load_port`'s samples are `(t_ms, blob, cells, extras)` — the record region is inside `blob` (`s[1]`), and my
  first probes sliced `s[2]`, which produced the bogus "port has no record region" suspicion. There is no such
  defect: `meta.json` gives `record_bytes=30139` with 61 cells declaring 352-byte record regions, and the port
  blocks read fine.

## 5. Self-correction: `+0x2b` is NOT a cumulative EXP total (read the writer before naming the field)

`fn_0137c1` (guest `0x0137c1`, the HUD number display) is the routine that both **prints** `entity[+0x2b]`
(5-glyph field, global font via `g_dat_63e04[0x34]/[0x38]`) and **ticks it**:

```c
if (entity[+0x52] != 0 && g_dat_63360 % 2 == 0) { entity[+0x2b]++; entity[+0x52]--; }   /* 0x0138ba-dc */
if (entity[+0x56] != 0)                         { *(int16*)entity[+0x23]++; entity[+0x56]--; }  /* 0x0138ea-fd */
```

So `+0x2b` is a **displayed counter that ramps up while the `+0x52` countdown drains** — the shipped
`0 → 0xa → 0x14` ladder is a timed ramp that then persists, not "10 EXP per kill". Naming it `exp_total` in
§3 was an inference from the value pattern, which the writer contradicts. Field names must come from the
writer, not the histogram.

What that leaves as the defensible discriminator for the EXP item:

```
+0x52 (the counter that drives the ramp):  shipped nonzero in 3 of 1300 samples (values 3, 8)
                                          port nonzero in 0 of 2160 samples
+0x32: shipped 0x36 vs port 0x5 at that offset (magnitude, and possibly width, still unresolved)
```

Follow-up (filed, not yet chased): find the guest site that **sets** `entity[+0x52]` (the counter that
`fn_0137c1` drains) and check whether the port ever writes it — that is the real gate for "EXP number does
not move", and it is a *write-site* question in the flat model, not a DS-symbol question.

## 6. Retraction: §3's EXP divergence was NOT reproducible — the port shows the same 0 → 0xa → 0x14 ladder

Re-running the port alone on the same drive with the record dump enabled at 4 Hz
(`./port/test/night --script scripts/rig/drives/ep1_kill3.txt --rig-dump /tmp/rig/xp4 --rig-hz 4`, 876 samples,
676 live player-record samples) and reading the fields at the constructor-derived offsets:

```
port   shown(+0x2b): 0x0 ×317, 0xa ×146, 0x14 ×211      shipped: 0x0 ×741, 0xa ×212, 0x14 ×332
port   acc(+0x52)  : 0x0 ×674, 0x2 ×1, 0x6 ×1           shipped: 0x0 ×1285, 0x3 ×2, 0x8 ×1
port   hp(+0x29)   : 0x3e8 in all live samples          shipped: 0x3e8 with 0x3e5 / 0x3dd dips
```

The ladder is identical. So the §3 claim ("exp_total never reaches the shipped 10/20 ladder") was an artifact
of the `/tmp/rig/g2` run — in that dump the `g_player_ent` record block was filler (`0xaa`) in 434 of 2160
samples and the live samples carried the drive's different fight/damage profile. Lesson recorded as rig
hygiene: a record-field claim must state the run, the sample cadence (`--rig-hz`) and the live-sample count,
because a pointer-cell block can be *unallocated* in some samples and a window with no kills has nothing to
compare.

### 6b. How the EXP number actually moves (write sites, from the port's transcription of the guest)

* kill award: `*(g_player_ent + 0x52) += victim[+0x32]` at guest `0x15453` (port `f.c:7938`),
  plus `+= e[+0xbb]` (`f.c:24734`);
* HUD roll-up: `fn_0137c1` ramps `+0x2b` up while `+0x52` drains, on even ticks (`0x0138ba-dc`);
* status screen: `fn_038f3d` (guest `0x00038f3d`, THE LEVEL-UP / STATUS SCREEN) folds the pending
  experience in one shot: `P[+0x2b] += P[+0x52]; P[+0x52] = 0` (`0x00038fff-0x0003900b`).

Witnesses added for this (`NIGHT_ACTTRACE` block): `exp award path: adds=2 added=20 commits=0 committed=0`,
i.e. on this drive the kill award fires twice for 20 points and the **status-screen fold never runs** —
consistent with `fn_038f3d` being one of the never-entered functions in the fuzz census (docs/120). The
visible ladder therefore comes from the roll-up on both sides, which the census confirms.

## 6c. Two side results from re-running the pre-fix build (`d22e4a7` in `/tmp/prefix`)

* **Independent red for the docs/116 SEGV.** Running the pre-fix binary with `--res-dir NIGHT` from the
  worktree (where `NIGHT/` is not checked out) crashed with `rc=139` right after
  `disc: NIGHT/TG_NIGHT.CUE not readable`, i.e. the missing-resource path still dereferenced a flat-model
  offset as a host pointer. Pointing `--res-dir` at the real `NIGHT/` directory, the same pre-fix binary
  runs to completion (`frames presented: 12106`), and the current build never crashes in that path at all.
  That is a rig-produced before/after pair for that item, no human inspection involved.
* **The EXP ladder cannot be shown red at record level on that build.** Its `meta.json` declares 243 cells
  but none of their record regions carries an entity record (`hp@+0x29 == 0x3e8` matches zero blocks), which
  matches docs/121's note that the pre-fix build predates the entity-slot dumper. So the EXP item's red side
  is documented from the *static* aliasing defect (docs/109) plus the pre-fix award-path zero, not from a
  record census; the green side is the 0 → 0xa → 0x14 ladder above.
