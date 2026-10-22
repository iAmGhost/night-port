# 0x03d75c — the stage-record float decay loop (L10, 307 B) — analysis, ready to port

Capstone listing source: `vad.py dis 0x3d75c 0x133`. Clean win in the disasm scan — its only callee is
`fpu_rndint` (`0x041f7e`, already ported at `f.c:1856`). Earlier deferred as "float/physics"; the loop
is actually small and fully tractable. **Not ported yet only because the session budget ran out.**

## Body (exactly as the machine code reads)

```
canary: PUSH 0x34; CALL 0x418ee            ; EAX = 0x34 is KEPT in [ebp-4] and USED below as a float!
for (i = 0; i < DAT_00062a90; i++) {                 ; record count
    rec  = g_dat_5d0b4 + i*0x14                      ; 0x14-byte records: +0x00 idx, +0x04 val, +0x10 float
    idx  = rec[+0x00]
    spr  = DAT_00063dfc                              ; stage sprite struct (loaded by fn_037b03)
    a    = rec[+0x04] - *(int *)(*(int *)(spr+0x0c) + idx*4) - DAT_000639dc
    b    = rec[+0x04] - *(int *)(*(int *)(spr+0x0c) + idx*4)
                  + *(int *)(*(int *)(spr+0x04) + idx*4) - DAT_000639dc
    a   += (int)fpu_rndint((float)rec[+0x10])         ; FLD dword; FRNDINT; FISTP
    b   += (int)fpu_rndint((float)rec[+0x10])
    if ((-1 < a && a < 0x140) || (-1 < b && b < 0x140))          ; signed compares: 0..0x13f inclusive
        rec[+0x10] = rec[+0x10] - (float)0x34 * *(float *)(0x5274c + roomSerial*4 + (DAT_00063370 - 1)*0x28)
}
```

* `0x041f7e` is `PUSH EAX; FNSTCW; ...; FRNDINT` — the round-to-nearest-even helper, ported as
  `fpu_rndint`.
* The multiplier `(float)iVar1` is the **frame-size constant 0x34 returned by the 0x0418ee stack-check
  prologue** (Watcom keeps EAX from `PUSH imm; CALL 0x418ee`). So the decay per call is `52.0f *
  table[...]`; port it as `canary_id(0, 0x34)`'s return value, not a literal, to stay honest about where
  it comes from.
* `FSUBR` at `0x3d875` means `mem - ST(0)`, i.e. `rec[+0x10] - (52.0f * table)` ✓ (Ghidra agrees here).
* Range gate: `a` in `0..0x13f` **or** `b` in `0..0x13f` (the two are `x` and `x + width`-like, both
  derived from the same `A[idx]`), i.e. the record is decayed only while it is at least partly inside
  the 0x140-pixel field, with `DAT_000639dc` (camera scroll) subtracted.

## Table to add

`DAT_0005274c` — a **float** table, `roomSerial`-indexed within a per-stage block of 40 bytes (10
floats): effective address `0x5274c + serial*4 + (stage-1)*0x28`.
Layout bound: `0x5274c .. 0x52c3c` (the next known global `DAT_00052c3c`) = **0x4f0 bytes = 316
floats ≈ 31 stage blocks**, so model it as `float g_tab_5274c[0x4f0/4]`. Like `DAT_52c3c`, `DAT_52738`,
`DAT_52f08`, `DAT_53200`, `DAT_532f4` it has **no producer inside the EXE** → seedable, awaits the CD
backend.

## One thing to settle when porting

`DAT_00063dfc+0x04 / +0x0c` are used as **dword-array addresses**. In the port, the sprite struct comes
from `fn_03d8d4`/`fn_03de85`, whose 14 sub-objects are stored as **heap offsets** (see `fn_03de85`), so
the port must add `g_heap_base` to those fields (and confirm against `fn_037b03`'s stage `.spr` load
which representation actually lands in `+0x04`/`+0x0c`). Get this wrong and the dereference faults — the
same class of trap as `fn_03e0a0` returning an offset.

## Test sketch (for the porting session)

Seed `g_dat_62a90 = 2` records with `+0x00` idx, `+0x04` val, `+0x10` float; a `g_dat_63dfc` struct whose
`+0x04`/`+0x0c` point at two dword arrays; `g_dat_639dc` camera; `g_dat_63354+0x40` room serial;
`g_dat_63370` stage; `g_tab_5274c[(stage-1)*10 + serial]` rate. Assert: (1) an in-range record's float
decreases by exactly `52.0f * rate`; (2) an out-of-range record's float is untouched; (3) the rounding of
`+0x10` feeds the range test (a float that rounds `a`/`b` into range triggers the decay); (4) `a` uses
`A[idx]` only while `b` adds `B[idx]`.
