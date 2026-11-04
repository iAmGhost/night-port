# 278 — Item 2b: per-entry chain rejection census — entries 6-9 want `0x1d` at their first window slot and the ring holds `0x48`/`0x00`

`chain_why()` added next to `ACTWHY` (`src/f/f.c`, armed by `NIGHT_ACTTRACE=1`, capped at 60 lines ✓ trace-only,
no behavioural change ✓). Canonical drive `ep1_kill3.txt` 150 s ✓, distinct (entry, stage, want, got) triples:

| entry | len | stage | want (entry's code) | got (ring slot) |
|---|---|---|---|---|
| 0 | 3 | code0 | `100`=0x64 | `72`=0x48 |
| 1 | 4 | code0 | `56`=0x38 | `29`=0x1d |
| 2 | 3 | code0 | `56`=0x38 | `72`=0x48 |
| 3 | 2 | code0 | `56`=0x38 | `29`=0x1d |
| 4 | 2 | code0 | `100`=0x64 | `29`=0x1d |
| 5 | 1 | code0 | `56`=0x38 | `0` |
| **6** | 4 | code0 → codek | `56`=0x38 then `29`=0x1d | `72`=0x48 then `0` |
| **7** | 3 | code0 | `56`=0x38 | `72`=0x48 |
| **8** | 2 | codek | `29`=0x1d | `0` |
| **9** | 1 | code0 | `29`=0x1d | `0` |

All samples were taken with `cursor = 4` ✓ (`g_dat_63f50`, so the window read is slots `3,2,1,0` backwards from the
cursor ✓ `s1 = cursor − len` with the `+10` wrap ✓ modelled at `f.c:27688-27691` ✓ faithful ✓).

## What this says

* The chain **tables are present and being walked**: entries 0-9 have lengths 3,4,3,2,2,1,4,3,2,1 ✓ and their first-code
  requirements are real `0x38`/`0x64`/`0x1d` values ✓ — this is the same container docs/264 §5 proved carries the
  `0x33/0x34/0x35/0x36` targets ✓
* The failure is a **phase problem, not a table problem**: the ring at the window slots holds `0x48`/`0x1d`/`0x00` where
  the entry wants `0x38` first ✓. Combined with docs/277 (post and clear *event counts identical on both sides* ✓), the
  divergence is *which codes sit in which slots when the resolver looks*, i.e. the ordering/phase of posted codes
  relative to the cursor ✓ — not posting volume ✓, not the reaction table (docs/259 ✓), not the arms (docs/271 §5 ✓),
  not the clamp (docs/254 ✓ landed), not the slot window (docs/263 ✓ landed) ✓
* Chain matches still 44 in 150 s ✓ (docs/266's 53 ✓ same order ✓) and `DAT_00062d58` remains `{0, 0x33}` ✓ — the
  matches are all single/short entries ✓

## Next bounded step (unchanged shape, sharper target)

Compare the shipped's ring **sequence** (the ordered code list its own dump shows per cursor position ✓) against the
port's, at matched cursor values, offline in `/tmp/rig/rb8` ✓ — if the shipped holds `0x38` immediately before the
cursor where the port holds `0x1d`, the defect is in *what the port posts per key* (`fn_034353`'s key-code mapping ✓
`keymap.c`'s `NIGHT_SC_*` codes ✓) rather than in `fn_03272f` at all ✓ Gate any change on: entry 6-9 matching ✓,
`DAT_00062d58` producing `0x34/0x35/0x36` ✓, settled `kind==1` ✓, `+0x32 = 10` ✓, `player[+0x52]` rising ✓, suite ALL
PASS ✓, soak 9/9 ✓. Suite re-run after this instrumentation: **ALL PASS** ✓
