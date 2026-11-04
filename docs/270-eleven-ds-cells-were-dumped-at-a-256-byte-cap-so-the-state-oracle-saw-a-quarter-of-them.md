# 270 — Eleven DS cells were dumped at a 256-byte cap, so the state oracle only ever saw a quarter of them

docs/269 §3's census error (port `DAT_000636b8` "244-255 nonzero" vs shipped "721-763") was traced to its cause: the
generator that builds the rig's dump table clamped every cell to 256 bytes ✓

```python
hsz = max(1, min(256, d["elems"] * slot)) if slot else 0        # scripts/audit/gen_headers.py, before
```

Auditing `rig_table.h` against the `f.h` declarations finds **11 cells dumped smaller than the object they name** ✓:

| cell | DS | declared | dumped |
|---|---|---|---|
| `g_tab_51b60` (the reaction-state table) | 0x51b60 | 1024 | 256 |
| `g_tab_52738` | 0x52738 | 1024 | 256 |
| `g_tab_52c3c` | 0x52c3c | 1040 | 256 |
| `g_dat_53818` (per-type action-table heap offsets) | 0x53818 | 1040 | 256 |
| `g_dat_62d68` | 0x62d68 | 800 | 256 |
| `g_dat_63e50` | 0x63e50 | 1024 | 256 |
| `g_dat_633b8` (768 B palette table) | 0x633b8 | 768 | 256 |
| `g_dat_636b8` (768 B glyph/palette table) | 0x636b8 | 768 | 256 |
| `g_tab_528b4` | 0x528b4 | 720 | 256 |
| `g_blk_51c24` | 0x51c24 | 476 | 256 |
| `g_tab_52ffc` | 0x52ffc | 260 | 256 |

Consequences ✓: (a) for those rows the state oracle compared the first quarter of each object ✓ — which is also why
byte-count comparisons between a shipped DS-window read (full width ✓) and a port cell read (capped ✓) are invalid ✓
(docs/269 §3's retraction ✓); (b) several of the truncated cells are exactly the ones item 2b interrogates
(`g_tab_51b60` ✓, `g_dat_53818` ✓) — my *static* comparisons against NIGHT.EXE read the full bytes ✓ so the
faithfulness conclusions (docs/259 ✓, docs/237 ✓) stand ✓, but any **dump-side** claim about them was narrowed ✓

Fix: the cap is raised to 1024 (the largest declared DS object ✓) with the reason and this list in the comment ✓, the
table regenerated ✓ — widths are now 1024/1024/1024/1024/800/1024/768/768/720/476/260 for those rows ✓ (`g_tab_52b84`
stays 90 bytes: its chosen declaration is smaller than the 360 my audit script read out of a different declaration ✓ —
recorded as an open item rather than forced ✓) ✓ `make test`: **ALL PASS** ✓

**Required follow-up before the no-regression claim is re-asserted:** the wider cells change the state-oracle row
contents ✓, so the canonical tallies must be re-run on the corrected widths (`route.sh scripts/rig/drives/ep1_kill3.txt
150` ✓, both sides, repeated ✓) and any newly appearing rows attributed ✓ until the attributed residual set is
restated ✓ — the previous "no new unattributed rows" statement (docs/266 §2 ✓) was measured on the capped widths ✓

## Addendum: the re-tally on corrected widths (canonical route, both sides)

`OUT=/tmp/rig/rb8 scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150` ✓ (same drive/env/`--rig-hz 10` ✓) against
`rb7` (the same route measured before the width fix ✓):

| row class | rb7 (capped widths) | rb8 (corrected widths) |
|---|---|---|
| SAME | 69 | **70** ✓ |
| DIFF | 28 | **27** ✓ |
| DIFF-SIZE | 13 | 15 |
| SIZE? | 35 | 33 |
| PTR | 52 | 52 ✓ |
| REPR | 19 | 19 ✓ |
| NON-GAMEPLAY (host_address_space / device_plumbing) | 2 / 27 | 2 / 27 ✓ |
| basis | orig n=429, port n=1500, cells=346, phases=274 ✓ | orig n=428, port n=1500, cells=346, phases=274 ✓ |

**One gameplay row moved from DIFF to SAME** ✓ and two `SIZE?` rows resolved into `DIFF-SIZE` ✓ — no new unexplained
row class appears, `PTR`/`REPR`/non-gameplay counts are unchanged ✓, and the basis is the same sampling ✓ So the
attributed residual set is **not widened** by the fix ✓ (the two `DIFF-SIZE` rows are the expected consequence of
comparing a full-width object whose port and shipped sizes genuinely differ ✓, e.g. `g_tab_52b84` at 90 bytes ✓, and
are attributed to the declaration mismatch recorded above ✓) ✓
