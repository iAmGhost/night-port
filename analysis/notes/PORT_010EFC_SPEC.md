# 0x010efc — port spec (prepared 2026-07-13, for the next session)

**Status:** unported (L10, 1025 B / 248 instructions). It is the single biggest *clean* lever left:
it gates the L15 pair `0x0374ad` (538 B) and `0x0376c8` (601 B) — `0x0376c8` calls it as
`fn_00010efc(heap_malloc(0x147), 0xffffffe2, 0, <stage table dword>)` and stores the result into
`DAT_00053c7c`.

Full listing saved to: **`analysis/disasm/FUN_00010efc.txt`** (capstone, 248 lines — read it in
slices, do NOT dump it all at once).
Ghidra decomp: `analysis/pseudo/021_FUN_00010efc_010efc.c`.

## ABI (from the prologue, verified by capstone)

```
010efc  PUSH 0x24 ; CALL 0x418ee      ; stack guard, EAX/EDX/EBX/ECX pass through
010f06  PUSH ESI EDI EBP ; MOV EBP,ESP ; SUB ESP,0x14
        [EBP-0x10] = EAX   block          (the object being initialised)
        [EBP-0x0C] = EDX   param2
        [EBP-0x08] = EBX   param3
        [EBP-0x04] = ECX   param4
        [EBP+0x10] = param5 (dword stack arg, RET cleans it -> cdecl-ish)
        [EBP+0x14] = param6 (byte stack arg)
```
Suggested C ABI: `uint8_t *fn_010efc(uint8_t *block, int32_t p2, int32_t p3, int32_t p4,
uint32_t p5, uint8_t p6);` (return = EAX, the block/derived pointer — confirm from the epilogue).

## Structure (cheap skeleton scan — `scratch/skel.py <va> <len>`)

* **No loops at all** (zero backward jumps) — it is a straight-line, branch-heavy initialiser.
* Only **3 calls**: `0x418ee` (guard), `0x41e20` (the memsetw-style fill, ported), `0x41e9f`.
* Only **1 global read/write pair**: `DAT_00063370` (compared with param5 at `0x10f20`) and
  `DAT_0003c80` written with the block when that compare is equal.
* Jumps: `jne@10f26 jne@11052 jmp@1106a jl@11088 jle@1108e jmp@11090 jne@1115d jmp@11189`
  → roughly four guard/selection regions; no switch table.

## Confirmed first stores (the object header)

```
0x10f36  block[0]      = param6 (byte)
0x10f3e  block[+1].dword = param2
0x10f47  block[+5].dword = param3
0x10f4a  ...            = param4        (header continues; overlapping dword stores at byte
                                         offsets are the usual idiom here — port them as-is)
```

## Porting plan for the next session

1. Read `analysis/disasm/FUN_00010efc.txt` in 3 slices (~80 lines each); build a C skeleton that
   mirrors the guard regions in order, keeping the overlapping/byte-offset stores verbatim.
2. Declare the two globals in `f.h` (`g_dat_63370` already exists per earlier work — check;
   `g_dat_3c80` is new).
3. Keep the suite green after **each** slice (`cc -std=c11 -O1 -g -o /tmp/pl test/test_f1.c
   src/f/f.c src/f/fx_04794a.c src/sys/sys.c -lm`, run with a timeout).
4. Test through the real caller shape (`0x0376c8` style): allocate a 0x147-byte block, call with
   p2 = 0xffffffe2, p5 equal / unequal to `g_dat_63370`, and assert the header fields plus the
   `DAT_0003c80` side effect.
5. Then L15 `0x0374ad` (needs `0x037b03` too) and `0x0376c8` become reachable.

Note: `scratch/skel.py` now exists — reuse it for any large function (calls / globals / loops only,
so the listing never has to enter context).
