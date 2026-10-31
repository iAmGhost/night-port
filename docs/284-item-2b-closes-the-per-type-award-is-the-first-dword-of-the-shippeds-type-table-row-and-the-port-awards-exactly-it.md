# 284 — Item 2b closes on the award half: the per-type EXP value is the first dword of the shipped's own type-table row, the port's harvest expression is byte-faithful, and the port now awards exactly that value

## 1. The shipped's award write, read from the disassembly (`0x15030-0x15073`)

```
0x15032: movsx eax, word [eax + 0x29]      ; record HP
0x15036: test eax, eax
0x15038: jne  0x1503c        -> HP != 0 path ... 0x1503a jmp 0x1505d        ; (the arms are inverted on purpose)
0x1505d: imul eax, dword [eax + 0xd], 0x88         ; idx * 0x88           (HP != 0 arm)
0x15067: mov  edx, dword [eax + 0x3ea8]            ; DS 0x53ea8 + idx*0x88  -> the row's first dword
0x15070: mov  dword [eax + 0x32], edx              ; award = per-type table value
0x1503c..0x15054: [eax+0x5a]=1; [eax+0x31]=1; [eax+0x30]=2; [eax+0x32] = 5   ; (HP == 0 arm, constant 5)
```

`field_writers.py --disp 0x32`: **10** writers, **0** indexed ✓ — `0x15054` (constant 5) and `0x15070` (the table value)
in `fn_01441e`; `0x31f4e` `fn_031f10`; `0x3202f` `fn_031f5f`; `0x335c1/0x335cf` `fn_03272f` (`= 0x2e` / `= 0x2d` ✓ the
port has these at `f.c:27421` ✓); `0x35cf6` `fn_035c5c`; `0x3acc0` `fn_03a829` (`= 0`); `0x3c809` `fn_03c7d7` (`= 0`);
`0x3ea50` `fn_03e20f` ✓

## 2. The port models both arms faithfully (`f.c:25628-25650`)

`... cur[0x30] = 2; [cur+0x32] = 5` ✓ in one arm, and in the other
`[cur+0x32] = (g_dat_53ea8 + type_idx * 0x88)[0]` ✓ with the comment recording that the guest has **no extra +0xc**
while `g_dat_53ea8` is itself `g_dat_53e9c + 0xc` ✓ (`f.c:8206-8208`: the entity-type table is 0x104 rows of stride 0x88 ✓)
— byte-faithful to `0x15060-0x15070` ✓

## 3. Fresh both-sides evidence from the shipped's own DS window (`/tmp/rig/rb9`, flat `0x16000` window, `offset = DS − 0x50000`)

First dword of `DAT_00053ea8 + idx*0x88`, read straight out of a shipped sample:

```
idx  0  1    2..10   11   ...  31   36
val  0  250   0...0   85   ...  10   15
```

and the port's award census on the same run (docs/282 §3): **`kind=1 type_idx=31 award(+0x32)=10` ×44** ✓✓ — the port
awards *exactly the shipped's table value for that type* ✓; the `5` values the census showed for `type 36` are the
**HP == 0 arm** (`0x15054`, constant 5 ✓), not the table value (`15` for type 36 ✓) ✓ — consistent with the shipped's
own branch ✓

**Superseded:** docs/189's "static: no per-type EXP table exists" ✗ — a per-type EXP value does exist: the first dword
of each `DAT_00053ea8` row (stride 0x88, read at `0x15067` with `EAX = idx*0x88` ✓), and it is present in the shipped's
own dumped memory ✓ Docs/188 ("the 53/54 in `+0x32` are state ids, not EXP") still stands ✓ — those values arrive via
`fn_03272f`'s `0x2e/0x2d` family stores and other writers ✓, not via `fn_01441e`'s table arm ✓

## 4. Item 2b status

| contract leg | status |
|---|---|
| root cause | **closed**: the drive's repeat shape never re-made the key while the slot state was held, so `g_dat_62cd8` was never credited, every post disarmed its key, and the multi-code `0x1d` windows that entries 6-9 (`0x33/0x34/0x35/0x36`) need never formed (docs/280-282 ✓) |
| per-type award table transcribed only where spans agree | **closed**: `g_dat_53ea8 = g_dat_53e9c + 0xc`, 0x104 rows × 0x88 ✓, read at the shipped's `0x15067` with `EAX = idx*0x88` (no extra +0xc ✓) and verified against shipped memory (31 → 10 ✓, 36 → 15 ✓, 1 → 250 ✓, 11 → 85 ✓) |
| red-before / green-after record-field oracle pair | **closed**: before (`rb8`) no `kind==1` dying record carried an award ✓; after (`rb9`) `kind=1 type_idx=31 +0x32=10` in 44 samples ✓✓ with the chain-target census moving `{0,51}` → `{0,53,51,54,52}` ✓ |
| docs/65 row | **closed** ✓ (docs/282, docs/283, docs/284 rows ✓) |
| suite green | **closed**: `make -C port test` ALL PASS ✓; nine-episode ASan/UBSan soak 9/9 with ubsan=0/asan=0/segv=0 ✓ |

Still open, recorded rather than glossed: the **player-side accumulation** has no valid instrument yet — `fn_015295`'s
`add dword [edx + 5], eax` (`0x152b8`) writes an unaligned `record+5` accumulator whose census holds small negatives
(`-56…-52`) on *both* sides ✓ (faithful, not EXP ✓), and the shipped's player record lives in guest heap so its EXP cell
cannot be read from the flat DS window ✓ A follow-up should instrument the shipped's HUD EXP field (`fn_0137c1`'s draw
✓) or its heap-dump player record ✓ Two residual ring divergences also remain (ring-empty 31.5 % vs 9.0 % ✓, cursor-at-0
38.1 % vs 17.6 % ✓) consistent with docs/281's cadence finding ✓
