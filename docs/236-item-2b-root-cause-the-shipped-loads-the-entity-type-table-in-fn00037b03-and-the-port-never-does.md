# 236 — Item 2b root cause located: the shipped loads the 260-entry entity-type table in `fn_037b03`; the port reads that table but never fills it, so every per-type value -- the EXP award included -- is 0

> **SUPERSEDED in part by docs/237.** §2's "the port never fills the table" is retracted: `f.c:5296-5307` models all
> ten `.chr` accesses, and the new `NIGHT_CHRDBG` witness measures row 31 as (35, 2, 0, 10) -- identical to the
> shipped's measured row.  §3's "the flat-5 store at 0x15054 is unmodelled" is retracted: `((struct ua32 *)((cur +
> 0x32)))->v = 5;` is present (uncited).  What survives: §1's shipped branch structure, and the fact that the award
> chain never runs -- docs/237 §3 localises it to `fn_01441e`'s walk visiting 26 slots, all with `+0x30 == 0`.

Task: `fix-exp-values` (item 2b, "EXP gain must be checked per enemy type").  This is a localization document; the
fix and its red/green pair come next.  It supersedes docs/189's headline ("no per-type EXP table exists") and
completes docs/199's open sub-question.

## 1. The per-type award producer, read out of the shipped bytes

`fn_01441e`'s settle block, 0x00015003-0x00015072 (capstone, contiguous with the 0x14fd1-0x15003 score block):

```
0x15003  mov  eax, [ebp-8]                     ; cur = the entity being settled
0x15006  al   = [cur + 0x30] ; cmp 1 ; jne 0x1501e        ; settle byte must be 1
0x15013  cmp  cur, [DS 0x53c7c] ; jne 0x15020             ; must NOT be the player record
0x15020  al   = [cur + 0x5a] ; test ; jne 0x1503a         ; the "already flagged" byte
0x1502f  movsx eax, word [cur + 0x29] ; test ; jne 0x1503c
0x1503c  [cur + 0x5a] = 1                       ; flag
0x15046  [cur + 0x31] = 1
0x1504d  [cur + 0x30] = 2                       ; settle -> 2
0x15054  [cur + 0x32] = 5                       ; AWARD = the flat constant 5
0x1505b  jmp 0x15073
0x1503a  jmp 0x1505d                                      ; (+0x5a != 0) takes the per-type path
0x1505d  eax = [cur + 0x0d] * 0x88                        ; type index * stride
0x15067  edx = [eax + 0x3ea8]                             ; DS 0x53ea8 + idx*0x88  <- per-type value
0x15070  [cur + 0x32] = edx                               ; AWARD = the per-type table entry
```

So the shipped has **two** award producers in one block: a flat `5` when the victim still carries a non-zero
`word[+0x29]` and has not been flagged, and **the per-type table value** otherwise.  `DS 0x53ea8` is
`DS 0x53e9c + 0xc` (docs/91, docs/108) -- the fourth dword of the 0x88-stride entity-type row -- and docs/199 §2
measured the shipped's row 31 as `(0x23, 0x2, 0, 0xa)` = **(35, 2, 0, 10) in 972 of 974 states**, which is exactly
the `10` the shipped's dying record carries in `+0x32` ✓

`fn_035c5c` (the list settle pass, 0x35ca0-0x35cf6) carries the same table read for every kind-1 record it stamps
(`+0x30 = 1`, `+0x17 = 0x15`, `+0x11 = 0`, `+0x32 = [idx*0x88 + 0x53ea8]`), and its only caller is `fn_01441e` at
0x150f5 (`branch_xrefs.py --target 0x35c5c`: 1 edge) -- a call the port **does** model (`f.c:25456-25462`,
`fn_04185e(...)`, `fn_03ee0a()`, `fn_00035c5c()`, `g_dat_53104 = 0x23`) ✓ so no call edge is missing; the value is.

## 2. The port's reads are faithful; the table's contents are empty

| shipped | port model | state |
|---|---|---|
| 0x15067/0x15070 per-type award | `f.c:25438-25443` (`g_dat_53ea8 + idx*0x88`), cited with the docs/108 no-extra-+0xc correction ✓ | faithful addressing |
| 0x35cf0/0x35cf6 (settle pass award) | `f.c:1255` same addressing ✓ | faithful addressing |
| `uint8_t g_dat_53e9c[0x88 * 0x104]` (`f.c:8156`), `g_dat_53ea8 = g_dat_53e9c + 0xc` (`f.c:8158`), `g_dat_53eb8 = g_dat_53e9c + 0x1c` (`f.c:8247`) | **BSS: all zero** | the array is never written |
| `imm_scan.py --imm 0x3e9c`: **8 hits, all inside `fn_037b03`** -- 0x37ebb, 0x37ef5, 0x37f1b, 0x37f58, 0x37f7e, 0x37fa4, 0x37fca, 0x37ff2 (`mov esi, 0x3e9c` x7, `mov eax, 0x3e9c` x1) | `f.c:5288` models the *read* at 0x37eb4 (`g_dat_53e9c + i*0x88`) | **the eight table-fill stores are not modelled** |

`fn_037b03` is the room/stage loader -- the same routine the rig anchors on for "the stage-data load"
(`DS 0x53e9c` going 0 -> non-zero, the anchor `cd_schedule.py` uses, and the shipped side does flip it: the anchor
was found at 62 300 ms in `/tmp/rig/g33/orig`) ✓✓ The shipped fills the 260-row, 0x88-stride entity-type table
there, column by column, from the stage record data; the port's `fn_037b03` reads the table but has no writer for
it anywhere in `f.c` (grep of `g_dat_53e9c` assignments finds only reads: `f.c:1312`, `f.c:5288`, `f.c:5578`,
`f.c:5583`) ✓✓

Consequence chain, all measured earlier and now explained by one cause:

* the per-type award read returns 0 in the port where the shipped returns 10 (docs/199 §1: 31 shipped dying samples
  carry `+0x32 = 10`, 38 port samples carry `+0x32 = 0`) ✓
* `fn_015295`'s award `player[+0x52] += victim[+0x32]` (0x152b8 for HP; the award gates are settle `+0x30 == 1` at
  0x152d1-0x152dc, `HP(+5) == 0` at 0x152f2-0x152f9, `[+0x11] > 0x15` at 0x152fd-0x15304, kind `+0x00 == 1` at
  0x15315-0x1531f) adds 0 ✓
* every *other* per-type lookup shares the same array -- `fn_010efc`'s constructor reads `+0x88`-stride rows
  (`f.c:1312`), `fn_037eb4` (`f.c:5288`), the player's initial fields at 0x377a1-0x377c1 (`f.c:5578/5583`), and
  `g_dat_53eb8 = g_dat_53e9c + 0x1c` (the per-entry script bytes) -- which is the same family as the "every entity
  lands on type 0 / gun art" and camera-population findings (docs/73, docs/161-162) ✓

## 3. What the fix is, and the gates it must pass

1. Read the shipped's eight `fn_037b03` sites (0x37eb4-0x38010) and model each store: which resource/record stream
   each column comes from, the row count actually written (`branch_xrefs`/contiguity as usual), and the stride
   arithmetic -- transcribing only where the shipped span and the port span agree (contract wording), i.e. the
   values must come from the same stage records the shipped reads, not from a hand-written constant table.
2. Cross-check the filled table against the shipped runtime evidence already in hand: docs/199 §2's row 31 =
   `(35, 2, 0, 10)` in 972/974 states, read out of `/tmp/rig/ents-*` DS dumps, plus `NIGHT_TYPETABLE`
   (`f.c:25429-25436`) printing `row+0xc` and `row+0` on the live port ✓
3. Red before, green after, on the kill-bearing route: `scripts/rig/exp_award.py --orig RUN/orig --port RUN/port`
   (the award census over the 0x160-byte entity record blocks) -- shipped 31 dying samples `+0x32 = 10` vs port 38
   samples `+0x32 = 0` is the committed red (docs/199 §1); the green must show per-type award values appearing in
   the port's dying records ✓
4. `set30_trace` (`f.c:25433`) and `NIGHT_SET30` stay as the settle-flag witness so the `+0x30` chain is visible
   while the table fills ✓
5. Suite `make -C port test` ALL PASS, then the nine-episode ASan/UBSan soak, then the state-oracle residual tally
   re-run (docs/176 §7 cadence rule: quote residual counts only on the canonical 150 s route at `RIG_HZ=10`).

One caveat to carry into the fix: `fn_01441e`'s flat-`5` store at 0x15054 has **no citation anywhere in `f.c`**
(`grep 0x15054` = 0 hits), while the shipped writes it every time the `+0x5a == 0 && word[+0x29] != 0` branch runs.
That store is a second, independent divergence in the same block and has to be restored in the same edit, or the
per-type column will be masked by the flat 5 in exactly the cases docs/199 counted.
