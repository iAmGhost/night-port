# 279 — Item 2b: the chain table's real contents, the shipped's state-4 climb is an `inc`, and an in-room gated census that makes the red basis comparable

## 1. `fn_03272f` is handed one of two tables by the player's state — and the port only ever gets one

Shipped `fn_03a829` (`--entry 0x3aae5 --end 0x3ab25`):

```
0x3aaea: cmp dword [eax + 0x17], 0x66      ; eax = DS 0x53c7c (player record)
0x3aaee: je 0x3aafb
0x3aaf0: cmp dword [eax + 0x17], 0x70
0x3aaf9: jne 0x3aafd
0x3aafd: mov eax, [0x12a98]   ; DS 0x62a98          0x3ab02: call 0x3272f
0x3aafb: mov eax, [0x12a9c]   ; DS 0x62a9c          0x3ab09/0x3ab0e: call 0x3272f
```

`call_xrefs.py`: exactly **2 direct call sites**, both in `fn_03a829` (`0x3ab02`, `0x3ab0e`) ✓ — the port models it as
one call with the same ternary (`f.c:23619`: `state == 0x66 || state == 0x70 ? g_dat_62a9c : g_dat_62a98`) ✓ faithful ✓
`call_xrefs.py` counts 4503 resolved `E8 rel32` calls ✓ so this is a resolved-edge count, not a pattern grep ✓

Dump-side selection census on `rb8` (both sides, whole run): the ten-table array at `DAT_00062aa0..0x62ac4` ✓, the
selector `DAT_00063378`, and the two live pointers:

| | shipped | port |
|---|---|---|
| `DAT_00063378` | `0` in 428/428 ✓ | `0` in 1500/1500 ✓ |
| active `DAT_62a98` = keytab slot | slot **0** in 428/428 ✓ | slot **0** in 1500/1500 ✓ |
| `DAT_62a9c` | `keytab[5]` in 428/428 ✓ | `keytab[5]` in 1500/1500 ✓ |

and the new `acttab` witness (armed by `NIGHT_ACTTRACE=1`, dumps every table `fn_03272f` is actually handed):

```
acttab #1 tbl=0x104905cb0 sel=62a98(attack) entries=10
entry 0 len=3 codes=64 64 64   target=3     entry 5 len=1 codes=38         target=61 (0x3d)
entry 1 len=4 codes=38 38 38 38 target=64   entry 6 len=4 codes=1d 1d 1d 1d target=54 (0x36)
entry 2 len=3 codes=38 38 38   target=63    entry 7 len=3 codes=1d 1d 1d   target=53 (0x35)
entry 3 len=2 codes=38 38      target=62    entry 8 len=2 codes=1d 1d      target=52 (0x34)
entry 4 len=2 codes=64 38      target=47    entry 9 len=1 codes=1d         target=51 (0x33)
```

**Only `62a98` is ever handed** (the port's player never reaches state `0x66/0x70`) ✓ — and that is *not* the
blocker, because the shipped's chain-target census `DAT_00062d58 = {0x33 ×1453, 0x35 ×198, 0x34 ×108, 0x36 ×9}` ✓ is
**exactly entries 6-9 of this same table** ✓✓ So item 2b's target states are reachable from the table the port already
walks ✓; what is missing is the window that admits them ✓

## 2. Correction to docs/278's prose: entries 6-9 want `0x1d` at *every* position

docs/278's table said "entry 6 wants `0x38` then `0x1d`" ✗ — the dump above shows entry 6 = `1d 1d 1d 1d` ✓; its
rejections were `codek want=29(0x1d) got=72(0x48)` and `len want=4 got=1` ✓ (the `code0` lines belong to entries 0-5,
which really do want `0x38`/`0x64` ✓) ✓

## 3. The shipped's state-4 climb is an `inc`, and the port models it

`field_writers.py --disp 0x12c58`: **8** instructions, **0** indexed ✓ — stores of `1` and `0` only, plus
`0x34726 inc byte [eax+0x12c58]` (fn_034635) and `0x347ca inc byte [eax+0x12c58]` (fn_034735) ✓. **Nothing writes 4** ✓
— state 4 is `1→2→3→4` by repeated `inc` ✓, reachable only while `fn_034735`/`fn_034635` keep being called on an armed
modifier slot ✓ Port equivalents exist: `f.c:8870` and `f.c:9809` (`g_dat_62c58[key]++`) ✓ — so the climb is modelled ✓
and the port's live census says it is entered rarely: `case_hits = 316` dispatches in 150 s ✓ producing 122 posts ✓

## 4. Comparable red basis: gated to in-room samples (`DAT_00063354 != 0`), same drive, same dumps

| in-room sample predicate | shipped (n=425) | port (n=880) |
|---|---|---|
| ring completely empty | **8.9 %** | 32.2 % |
| ≥8 of 10 slots filled | **85.4 %** | 57.3 % |
| `0x1d` window ending at `cursor-1`, L=1 / L=2 / **L=3** / L=4 | **54.6 % / 22.6 % / 12.9 %** / 0 | 37.4 % / 12.6 % / **5.8 %** / 0 |
| cursor at 0 | 17.0 % | **33.6 %** |
| ring *reset* events (≥5 filled → 0) | **0** | **0** ✓ (one `empty → non-empty` each ✓ — the ring is filled once and never reset ✓) |
| ring write *events* (0→code / code→0 / code→other code) | 21 / 11 / **97** = 129 | 21 / 11 / **45** = 77 |

Two refinements of earlier claims: the shipped rewrites *already-filled* slots 97 times against 21 fresh fills ✓, so the
`0→code` equality in docs/277 §1 was the least informative metric ✓ — counting all slot writes gives shipped 129 vs port
77 on the same 150 s ✓ (transition counting under-counts the coarser side, so "≥1.7×" is the honest bound ✓); and
`L=4` (entry 6) never holds on **either** side ✓ — the shipped's nine `0x36` targets arrive between samples or through
`fn_03442e`'s combination path, not from a steady 4-window ✓

## 5. Green gate for the next change (all dump-side, all already instrumented)

in-room `≥8-filled` → 85 % ✓ · `L=3` window → ≥13 % ✓ · `DAT_00062d58` census gaining `0x34/0x35/0x36` ✓ · a settled
`kind==1` enemy with `+0x32 = 10` ✓ · `player[+0x52]` rising ✓ (`scripts/rig/exp_award.py`, repeated ≥2× per side ✓) ·
`make test` ALL PASS ✓ · nine-episode soak 9/9 with 0 UBSan/ASan/SEGV ✓
