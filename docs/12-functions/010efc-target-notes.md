# 0x010efc — target prep notes

L10, 1025 B, 248 instructions, **unported**. Highest-value remaining clean lever: it gates the L15
pair `0x0374ad` / `0x0376c8` (the latter calls it as
`fn_00010efc(heap_malloc(0x147), 0xffffffe2, 0, <stage table dword>)` and keeps the result in
`DAT_00053c7c`).

* Listing: `analysis/disasm/FUN_00010efc.txt` (capstone; read in slices).
* Decomp: `analysis/pseudo/021_FUN_00010efc_010efc.c`.
* Full work plan + ABI derivation: `scratch/PORT_010EFC_SPEC.md` (scratch is git-ignored).

## ABI (capstone, from the prologue)

`PUSH 0x24; CALL 0x418ee` guard, then four register arguments and two stack arguments:

| slot | meaning |
|---|---|
| EAX | block being initialised (also the usual return shape) |
| EDX | param2 → stored as a dword at block+1 |
| EBX | param3 → stored as a dword at block+5 |
| ECX | param4 |
| [EBP+0x10] | param5 (dword) — compared against `DAT_00063370`; on equality the block is published to `DAT_0003c80` |
| [EBP+0x14] | param6 (byte) → stored at block[0] |

## Shape (from `scratch/skel.py`, so the listing never had to be read into context)

* **No loops** (zero backward jumps) — a straight-line, branch-heavy initialiser.
* Only three calls: the `0x418ee` guard, `0x41e20` (already ported fill) and `0x41e9f`.
* Only one global pair: read `DAT_00063370`, conditional write of `DAT_0003c80`.
* Jumps: `jne@10f26 jne@11052 jmp@1106a jl@11088 jle@1108e jmp@11090 jne@1115d jmp@11189`
  → four guard/selection regions, no switch table.

The overlapping dword-at-byte-offset stores seen in the first 0x50 bytes (`block[0]` byte, `+1` and
`+5` dwords) continue in the same idiom; port them verbatim, as elsewhere in this code base.
