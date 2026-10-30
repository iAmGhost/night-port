# 188 — Self-correction: the 53/54 in `+0x32` are state ids, not EXP amounts

docs/186 §2/§8 and docs/187 measured a real difference but labelled it wrongly.  This document supersedes the
label, keeps the measurements, and adds a divergence that needs no record layout at all.

## 1. What `+0x32` actually holds

`fn_031f10` (guest `0x31f10`, port `f.c:8215`) is a **push into the saved-state queue**, and the six shipped call
sites (`branch_xrefs.py --target 0x31f10`) push *state ids*, not point values:

```
from fn_01441e : 0x149c5, 0x14b08, 0x14d90
from fn_03272f : 0x33bbb   0x33bb1 mov edx, 0x71                      ; push 0x71 (113)
                 0x34049   0x34037 mov dx,  word [edx + 0x388]        ; push a word from a table
                 0x340e5   0x340d3 mov dx,  word [edx + 0x388]        ; (same, other branch)
```

with `edx = [ebp-0x1c]*2 + [ebp-4]` (a 2-byte-stride table walk; `fn_03272f`'s `[ebp-4]` is its `tbl` argument, set
at `0x32747` from the incoming `EAX`).  The port models **all six** sites (`f.c:25086`, `25108`, `25175`, `27352`
`fn_031f10(pl, 0x71)`, `27398`, `27411` `fn_031f10(pl, target)`) — an earlier reading of mine that the port was
missing `fn_03272f`'s three pushes was wrong: the grep was truncated by `head -5`.

`fn_031f5f` then shifts that queue (`slot[k] = slot[k+1]`, docs/187 §2) and copies the head into `rec[+0x17]` (the
**state** field).  So `+0x32` is the head of a queue of *states to retarget to*.

## 2. The 53/54 match the chain-target census exactly

`DS 0x62d58` is the chain-target cell.  docs/167 §14 reported the shipped census `0x33 x1453 / 0x35 x198 / 0x34
x108 / 0x0 x78 / 0x36 x9`.  Re-measured on `g33` straight from `state_NNNN.ds.bin` (one byte per DS address from
`DS_LO = 0x50000`, so no record layout is involved):

| cell | shipped (413 states @4 Hz) | port (1 500 samples @10 Hz) |
|---|---|---|
| `DS 0x62d58` chain target | `0x33` x328, `0x0` x42, **`0x34` x20, `0x35` x19, `0x36` x4** | `0x33` x787, `0x0` x713 |
| `DS 0x62d5c` chain level | `0x0` x410, `0x1` x3 | `0x0` x1 500 |
| `DS 0x63370` stage | `0x1` x413 | `0x1` x1 500 |

`0x35` = 53 and `0x36` = 54 — the same numbers that appeared in the record `+0x32` census (docs/186 §2: shipped 54
x325 / 53 x2).  They are **chain-target state ids**, and `fn_01441e`'s `+0x32 = 5` writes a state id too.  The
"shipped awards 54, the port awards 5" framing in docs/186/187 is therefore wrong: both sides write state ids into
the same slot.

What survives as a genuine divergence, and is quotable without any record-layout assumption: **the port's chain
matcher only ever selects target `0x33`; shipped also selects `0x34`, `0x35` and `0x36` (43 of 413 states), and the
port's chain-level cell `DS 0x62d5c` is zero everywhere** (docs/179 already traced that cell to seven unwritten
stores inside `fn_03272f`).  Items 2, 3 and 4 of docs/manual-playtest/03-2026-10-29.md are all about that
chain/combo-level machinery, so this is the row to chase.

## 3. The player-record EXP fields are NOT yet a trustworthy basis

Reading docs/125's player offsets out of the `g_player_ent` dump block gives implausible, run-constant values:

```
port   g_player_ent (1 500 samples): display +0x2b = 256000 (distinct 1), pending +0x52 = 327 680 000 (1),
       accumulator +0x56 = 327 680 000 (1), score +0xbb = 260 (1)
shipped g_player_ent block (413 states): +0x2b max 29 338 282 (2 distinct), +0x52 max 114 623, +0xbb 2 881 470 213
```

and the shipped block at `state_0380` / `state_0412` is a fill pattern, not a record
(`+0x20..0x3f: bf a9 a9 bf 01 00 07 aa aa aa aa aa aa bf 01 00 07 aa ...`), while the port's block at the same
time is a real record (`+0x2c: 2c 01 00 00  e8 03 00 00  a0 0f 00 00  88 13 00 00` = 300, 1000, 4000, 5000).  The
shipped `ent00..ent11` blocks are entirely zero at `state_0380`.

Consequence for the rig: a shipped-side block is only informative when the pointer cell actually points at a live
record, and "passes `looks_like_entity()`" is not sufficient to prove that the two sides are looking at the *same*
structure.  Until that identity is established, the per-type EXP amounts in item 2b cannot be quoted from record
bytes — which is why `gate3-exp-values` is still open even though a divergence has been localized.

Next concrete rig step for that identity: pick a kill that both sides perform (the `NIGHT_HITTRACE` witness names the
victim on the port side; the shipped side names it through `DS 0x62d58`/the entity list), and require the shipped
block to be the one whose pointer cell value changed at that step.  If the shipped blocks cannot be tied to named
entities, item 2b's per-type comparison has to come from the **screen oracle** (the number the HUD prints) instead of
record bytes.

## 4. Where the port's chain targets go: computed, written, then invisible

`NIGHT_STTRACE=1` on the port (`port/test/night --script drives/ep1_kill3.txt --frames 9000`, i.e. the same 150 s
route as `g33`) prints the chain-match witness at the five sites `f.c:27370, 27379, 27391, 27400, 27413`, each of
which is `g_dat_62d58 = target; g_dat_62d5c = lvls[k-1]; chain_target_log(...)`:

```
94 witness lines, all at site "match"
  55 x target=51 (0x33) lv=1
  21 x target=52 (0x34) lv=1
  18 x target=53 (0x35) lv=1
```

So the port **does** compute `0x34` and `0x35` (39 of 94 matches), the same state-id family shipped selects, and it
does write them into the cell.  Yet the dumped cell on the identical route holds only `0x33` (787 samples) and `0x0`
(713), and `DS 0x62d5c` is 0 in all 1 500 samples even though every witness line carries `lv=1`.

The dumper is not reading a different object: `NIGHT_RIGDBG=1` prints

```
cellmap g_dat_62d58 ds=00062d58 host=0x106bbd48c host_size=4 ptr=0
cellmap g_dat_62d5c ds=00062d5c host=0x106bbd404 host_size=4 ptr=0
```

(one 4-byte cell each, `dupes: 1` in `scripts/audit/gen/rig_cells.json`, no alias).  The difference is therefore
**persistence**, not observation: on the shipped side `0x34`/`0x35`/`0x36` survive long enough to be sampled at 4 Hz
(20/19/4 states of 413), while on the port side they are overwritten before the next 10 Hz sample.  That is the row
to chase for items 3 and 4, both of which are about the combo/technical-level state the chain machinery feeds.
