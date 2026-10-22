# 0x03d75c — the stage-record float decay loop (L10, 306 B) — PORTED

Capstone listing source: `vad.py dis 0x3d75c 0x133`. Clean win in the disasm scan — its only callee is
`fpu_rndint` (`0x041f7e`, already ported at `f.c:1856`). Earlier deferred as "float/physics"; the loop
is small and fully tractable. **Ported 2026-07-13 with 8 test checks; the ABI section below corrects the
first reading of the prologue (the multiplier is NOT the frame constant).**

## Body (exactly as the machine code reads)

```
canary: PUSH 0x34; CALL 0x418ee            ; EAX out = the CALLER's EAX -> implicit arg (see ABI)
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
        rec[+0x10] = rec[+0x10] - (float)EAX_in * *(float *)(0x5274c + roomSerial*4 + (DAT_00063370 - 1)*0x28)
}
```

* `0x041f7e` is `PUSH EAX; FNSTCW; ...; FRNDINT` — the round-to-nearest-even helper, ported as
  `fpu_rndint`.

## ABI — the multiplier is an implicit register argument (correction)

`0x0418ee` (the Watcom stack probe `__CHK`) is:

```
0418ee  xchg dword ptr [esp + 4], eax   ; frame size -> EAX for the checker; caller EAX saved on stack
0418f2  call 0x41901
0418f7  mov  eax, dword ptr [esp + 4]   ; caller EAX RESTORED
0418fb  ret  4
```

so `MOV [EBP-4], EAX` at `0x3d774` keeps the **caller's EAX**, not the pushed `0x34`. Ghidra's
`iVar1 = FUN_000418ee()` is an unmodelled incoming register — and that register is what `FILD`/`FMUL`
use. (The port's own `canary_id(eax, frame_size) -> eax` convention already encodes this correctly.)

**Both call sites pin what the register holds** (found by scanning the text for `E8 rel32` to 0x3d75c):

```
0003cbc3  mov eax, [0x139dc]      ; DAT_000639dc = camera x scroll (current)
0003cbc8  sub eax, [ebp - 8]      ; minus the previous frame's value
0003cbcb  call 0x3d75c
...
0003cc55  mov eax, [0x139dc]
0003cc5a  sub eax, [ebp - 8]
0003cc5d  call 0x3d75c
```

Both are inside `fn_03cb38` (the camera/scroll updater, which writes `DAT_639dc` / `DAT_639e0` /
`DAT_639e4` from a room struct). So the decay is **per pixel of camera travel**, at a per-stage /
per-room rate: a record loses its float while crossing the field, and scrolling back restores it (the
product is signed — `FILD` of a negative delta turns `FSUBR` into an add). Port signature:
`void fn_03d75c(int32_t cam_delta)`.
* `FSUBR` at `0x3d875` means `mem - ST(0)`, i.e. `rec[+0x10] - (delta * table)`, and the `FSTP` at
  `0x3d87b` writes the **unrounded** original minus the product — while the range gate used the
  *rounded* float. Pinned by a test (`f = -1.0` pulls `a = 0x140` into range; stored `-1.0 - delta*rate`).
* Range gate: `a` in `0..0x13f` **or** `b` in `0..0x13f` (the two are `x` and `x + width`-like, both
  derived from the same `A[idx]`), i.e. the record is decayed only while it is at least partly inside
  the 0x140-pixel field, with `DAT_000639dc` (camera scroll) subtracted.

## Table added

`DAT_0005274c` — a **float** table, `roomSerial`-indexed within a per-stage block of 40 bytes (10
floats): effective address `0x5274c + serial*4 + (stage-1)*0x28`.
Layout bound: `0x5274c .. 0x52c3c` (the next known global `DAT_00052c3c`) = **0x4f0 bytes = 316
floats ≈ 31 stage blocks**, so model it as `float g_tab_5274c[0x4f0/4]`. Like `DAT_52c3c`, `DAT_52738`,
`DAT_52f08`, `DAT_53200`, `DAT_532f4` it has **no producer inside the EXE** → seedable, awaits the CD
backend.

## Settled: `DAT_63dfc+0x04 / +0x0c` hold HEAP OFFSETS

`fn_03de85` fills the struct with `s[i] = heap_malloc(size)` (fields `+0x04 .. +0x38` = `s[1..14]`) and
`fn_03dcbd` frees them with `heap_free_host(s[k])`, so the fields are **heap offsets**: this function must
add `g_heap_base` before indexing `+0x0c` (array A) and `+0x04` (array B).

**Discrepancy flagged for a follow-up audit (deliberately not changed here):** the older `fn_03df96` port
reads the same global through `struct hud_font`, whose fields are declared as host `uint32_t *` at
*re-packed* offsets (+0, +8, +0x10, +0x18, +0x20, +0x28) instead of the original
+0x04/+0x08/+0x0c/+0x10/+0x34/+0x38, and dereferences them directly. Only one of the two views can match
what the loaders actually store; this function follows the loader evidence.

## Test (implemented — 8 checks, `test_fn_03d75c`)

Five seeded records over heap-resident `A`/`B` dword arrays:

1. in-field record decays by exactly `delta * rate`;
2. both projections outside `0..0x13f` -> float untouched;
3. `a` out but `b = a + B[idx]` in -> the OR gate still decays;
4. the gate uses the **rounded** float, the store uses the unrounded original;
5. `A[idx]` (`spr+0x0c`) is the value subtracted for `a`;
6. the rate index follows `serial + (stage-1)*10` (stage 1 vs 2 give different rates);
7. a negative camera delta **increases** the float (signed `FILD`);
8. record count 0 -> the loop never runs.
