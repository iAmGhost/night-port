# 100 — Shipped symbol/call-graph DB + static equivalence audit (no runtime, no human judgement)

Goal task: `rig-static` → `audit-symbols`, `audit-checks`.

Everything here runs offline against two inputs only: the Ghidra database for the shipped
`NIGHT.EXE`, and the port's own source (`port/src/f/f.h`, `f.c`). Nothing in it is told where a bug
is; the known families are used only afterwards as a regression check (§4).

## 1. Export: what was pulled out of Ghidra

Script: `~/ghidra_scripts/NightExport.java` (copy kept at `scripts/audit/NightExport.java`), run against project `ppk`, program `/NIGHT.EXE`
(`mcp__ghidra__run_ghidra_script`). Outputs land in `scripts/audit/`:

| file | rows | content |
|---|---|---|
| `symbols.json` | 1032 | 530 `Function` symbols + 502 labels (mostly `caseD_*`/`switch*` — kept, harmless) |
| `functions.json` | 530 | entry, name, body span, **direct callees** (call graph) |
| `defined_data.json` | 2214 | every defined data item: addr, end, size, Ghidra type, label, value-reference count (1638 of them inside DS `0x50000..0x66000`) |
| `mem_access.json` | 4499 | **instruction level**: every instruction with an operand touching DS, with `{to, rtype}` refs → 630 distinct DS cells |
| `export_info.json` | – | coverage counters |

Coverage, measured (not assumed):

- functions: **530 exported / 531 in the DB** — the one not exported is the loader-synthesised
  entry wrapper, not a game routine;
- defined data: **2214 / 2216** (two undefined-hole artefacts);
- DS names: **1043 of 1638** DS items carry a label (`s_*` 670 resource-name literals, `DAT_*` 349,
  `PTR_*` 19, plus renames from earlier sessions);
- DS cell access counts: 630 cells from instruction refs, versus **0** from
  `Data.getValueReferences()` — that API reports nothing for this image, which is why the audit
  reads access counts from the instruction dump instead.

Two structural facts about this database drive every check below:

1. **Flat dwords.** Ghidra applies no arrays and no structs: `DAT_00053d0c` is one `undefined4`, not
   a pool, and a table reached through a register base has **no symbol at all** — the base exists
   only as an immediate operand. So "two overlapping DS intervals" cannot be found by comparing
   symbols; it has to be found from operand text (`c8`).
2. **The loader types DS globals `undefined *`** (600 of them) / `pointer` (16). Reading the
   decompiler literally therefore *manufactures* pointer semantics. `docs/49 / 61 / 94 / 95` are
   consequences of trusting that; the audit treats a shipped `undefined *` as no evidence at all.

Image-initialised state: the LE image loads at linear `DS-0x7600`, so a DS address maps to file
offset `addr-0x7600` in `NIGHT/NIGHT.EXE` (`ghidra_db.Db.image_at`). That is the only ground truth
for "what did this global contain before the game touched it".

## 2. The join table

`scripts/audit/gen_headers.py` joins the three sides per DS cell:

- guest: symbol label, Ghidra type, size, read/write counts (direction recovered from mnemonic +
  which operand holds the address, because Ghidra tags 1079 refs merely `DATA`),
- image: the 4-byte value at `DS-0x7600`,
- port: every port declaration that claims the address, recovered from the name
  (`g_tab_52c3c` → `0x52c3c`) or the declaration comment (`/* DAT_00053338 */`).

Outputs: `gen/ds_join.json` (1829 rows), `gen/ds_symbols.h` (`g_ds_symbols[]` + names for the 447
cells the guest actually touches — the dump table the state oracle will use), `gen/fn_census.h`
(all 530 shipped entries — the coverage census), `gen/gen_info.json`.

Port-side parse: 410 declarations carrying a DS address across 249 distinct cells; 404 port `fn_*`
names vs 530 shipped: **169 shipped names with no port counterpart, 47 port names with no shipped
counterpart**.

## 3. Checks (`scripts/audit/static_audit.py` → `reports/static_findings.{json,md}`)

| id | question | findings |
|---|---|---|
| C1 | DS cells the guest touches that the port never names | 388 |
| C2 | overlapping DS spans / several port names on one cell | 126 |
| C3 | image-initialised cell vs the port's own initializer | 31 |
| C4 | port declares pointer, shipped use says number (and the reverse) | 4 |
| C5 | shipped fn with no port fn / port fn nothing calls | 90 / 181 |
| C6 | runs of non-zero image bytes with **no shipped symbol at all** | 56 |
| C7 | every DS immediate constant (table bases, for the oracle's dump list) | 543 |
| C8 | two DS bases whose access spans overlap → views of one table | 4621 (ranked) |

## 4. Regression check — the historical root causes reappear unaided

- `docs/99` **kill quota at 0x52a1c**: C6 reports the 183-cell image-initialised run
  `0x528a8..0x52b84` that contains it and for which the shipped DB has **no symbol at all**, and C2
  reports that the port's own `g_tab_528b4[0x2d0]` span (`0x528b4..0x52b84`) *swallows* it. So the audit reproduces both halves of that bug: the shipped
  bytes were invisible to the decompiler, and the port's declared array covers the neighbour.
- `docs/91` **entity-type view**: C8 lists `0x53e9c` vs `0x53eb8`, distance `+0x1c`, both
  port-named (`g_dat_53e9c`, `g_dat_53eb8`).
- `docs/98` **SFX slot cells inside a pool**: C8 lists `0x53d0c` (`g_pool_53d0c`) vs `0x53dd4`
  (`g_dat_53dd4`, declared `uint8_t *` in the port), distance `+0xc8`.
- `docs/83` **0x53200 block**: C2's port-span-overlap class is exactly this failure mode; it now
  flags `g_tab_52c3c[0x104]` (`0x52c3c..0x5304c`) swallowing `g_tab_52da4` (0x400 overlap),
  `g_dat_52f08` (0x100), `g_tab_52f2c`, `g_tab_52f90` — one over-declared array hiding five tables.
- `docs/55` **DSP enable byte 0x53338**: present in `ds_join` with image u32 `1`, guest
  `undefined1`, written `MOV byte ptr [0x53338], AH` — the audit's evidence shape for that class;
  C3 no longer reports it because the port now initialises it (fixed), i.e. the check is not
  vacuous in either direction.
- `docs/89`-family **pointer vs number, both directions**: C4 flags `0x53c7c` where the shipped
  symbol is literally `pointer` (`g_pPlayerEntity`, 867 reads) while the port's declaration there is
  `int`, plus three cells the port declares `T *` whose shipped values are small numbers.

## 5. What static cannot see (and why the runtime rig is not optional)

C1–C8 are *presence* tests. Three of the seven open playtest items are phase/device/host-decision
bugs — the music-never-stops gate (a device-state flag, not a symbol), the missing punch SFX
(a mixer bring-up order), the stuck-when-hit-mid-kick (a lock/phase transition) — where every symbol
and initializer already matches and only the *sequence of states* differs. Those need the runtime
oracles (`rig-runtime` and below).

## 6. Triage: every finding gets a verdict (`scripts/audit/triage.py` → `reports/triage.{md,json}`)

442 findings filed with evidence, 86 explained. Nothing here is guesswork dressed as a verdict: the
verdict is decided by a rule, and the rule is printed with the finding.

| verdict | n | rule |
|---|---|---|
| FILED-READONLY | 7 | guest never writes the cell, image bytes non-zero, **and nothing in the port ever writes that global** -> the shipped bytes were dropped |
| FILED-PRODUCER-CHECK | 16 | same, but the port has its own producer -> compare the producer against the shipped bytes |
| FILED-RUNTIME | 107 | initializer differs and the guest does write the cell -> only a replay can say whether the first read sees the shipped value |
| FILED-SPAN | 120 | a port span covers another port global that the guest addresses with its own base constant -> over-declared array hiding a table |
| FILED-VIEW | 63 | two port-named DS cells with overlapping access spans |
| FILED-MODEL | 61 | DS cell with >=3 accesses the port does not name and is not inside any port span |
| FILED-REPR | 4 | pointer/number disagreement, both directions |
| FILED-CALLGRAPH-GAP | 64 | shipped fn with shipped callers, no port counterpart |
| EXPLAINED-DEAD / -RENAMED / -BY-SPAN / -SAME-TABLE / -UNACCESSED / -SCRATCH | 86 | no shipped caller; a port fn shares >=50 % of its callees (renamed); cell lies inside a port span; contiguous one-table split; no instruction-level access |

False-positive controls built into the pass (each one was needed):

- **stack frames**: Ghidra resolves `DS:[EBP+0x4]` to a real DS address, so frame-local accesses were
  being reported as global state (`0x53650`, 6 cells) -> excluded by direction `stack`;
- **port producers**: a placeholder with a loader is not a dropped initializer, so the write scan
  skips declaration statements (otherwise `= {0}` itself counted as a producer);
- **renamed helpers**: a port function sharing half of a shipped routine's callees is treated as the
  same routine rather than a gap.

The filed items are *not* fixed yet, deliberately: `blind-validation` has to observe the current
build. The clearest one is worth naming now, because it is a live bug rather than a hypothesis --
`float g_tab_5274c[0x4f0/4] = {0};` (`f.c:5038`) is read by the shipped camera code
(`0x3d86f FMUL dword [edx+0x5274c]`), nothing in the port ever writes it, and the shipped bytes are
`0.5, 0.5, …`: the port multiplies by zero. `g_tab_52a1c` (kill quota, `0x3e8`) is the same shape,
which is what `docs/99` found by hand.

## 7. Reproduce

```sh
# 1. refresh the Ghidra export (Ghidra MCP: run ~/ghidra_scripts/NightExport.java)
# 2. join + headers
python3 scripts/audit/gen_headers.py
# 3. checks
python3 scripts/audit/static_audit.py        # writes reports/static_findings.{json,md}
```
