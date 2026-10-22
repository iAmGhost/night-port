# 0x0461cc (2998 B) — the FPU emulation engine: decode notes for the port

The last two pending functions are `0x0461cc` (2998 B, L5) and `0x0473bf` (38 B, L6, `caseD_3`,
which depends on it). Everything else they call is already ported, so this is a pure transcription
task — the notes below are the decoded shape, to avoid re-deriving it.

## What it is

`FUN_000461cc` is the interpreter loop of the game's bundled **x87/FPU emulator** (Watcom's FP
emulation layer, not the game's own logic). It decodes an instruction stream and dispatches over the
emulated register file.

State it drives (all already declared or trivially addable):

| Global | Role |
|---|---|
| `DAT_00053668 / +0x3c` | the emulated FP register file: 8 x (dword low, dword mid, word high) -> 8 * 10 bytes, indexed by `reg * 10` |
| `DAT_00053650` | FP control word (`& 0x3800) >> 0xb` picks the rounding/control field, `& 0xffffc7ff` clears the status tags |
| `DAT_00053654` | the FP status word; masked through `DAT_00045e1a[...]` and OR-ed with `DAT_00045e22[...]` |
| `DAT_000536b8 / bc / c0` | a scratch FP register (push/pop of the emulator's temp) |
| `DAT_000536c2` | a **function pointer** called after the arithmetic cases — the emulator's "raise FP exception / stall" callback |
| `DAT_00045dcc / dca / dce / e1a / e22` | ROM tables (opcode -> tag/exception masks), see the case bodies |
| `PTR_LAB_00045e78`, `PTR_LAB_00045f38` | two of the emulator's indirect jump tables (the first is indexed by `b | (b >> 8)` of an `0x1807`-masked byte; the second by the callback's return value) |

## Control flow (first 0x1e0 bytes decoded)

1. A byte-scan over the instruction stream advancing past the operand bytes of a prefix set
   (`0x9b/0x9c`, `&`, `.`, `6`, `>`, `d`, `e`, `f`, `g`) — this is the emulator's operand-prefix skip.
2. The decoded opcode byte must be `-0x28` (0xd8) for the engine to continue, i.e. this entry handles
   the `0xd8` escape; otherwise it returns immediately.
3. Low opcodes (`< 0xc0` of the modrm byte) go through `PTR_LAB_00045e78` -> `FUN_00042310` ->
   the second jumptable `PTR_LAB_00045f38` (Ghidra cannot normalise these; the port will need the
   tables read out of the ROM bytes and modelled as C switch/dispatch).
4. High opcodes go through a big switch on `b & 0x38 | b`: cases 0/4/6/8/9/0xc/0xd/0xe/0xf/0x10/0x11/
   0x14/0x15/0x17/0x18/0x1c/0x19/0x1d/0x1e/0x1f, which map onto the **already ported** helpers:
   `fn_0478f0` (FADD), `fn_047d24` (FMUL), `fn_047ab6` (operand fetch/copy), `fn_0463c0` / `fn_04636e`
   (the store/exception tails), `fn_047b4f`, `fn_0478e2`, plus `fn_042310` (register/tag lookup) and
   `fn_048c3f/048c49/048c56/048c8c` etc.
   Several cases *recurse* into `FUN_000461cc` (the emulator re-enters itself for the next instruction
   or for the pending-op tail) — the port must keep that recursion.
5. The register-file swaps (cases 9/0xd/0xf) use `LOCK()`/`UNLOCK()` around three exchanges
   (low, mid, high) — model as plain moves in the flat port.

## Port plan

1. Declare the register file as a **named C struct array** (`struct fp_reg { uint32_t lo, mid; uint16_t hi; }`
   indexed by register, with `reg*10` byte arithmetic kept for the table indices) — the STATUS rule
   "pointer/ dword fields spaced 4 bytes apart must be struct fields, never in-place widening" applies.
2. Extract the ROM tables (`0x45dcc..0x45e78`, `0x45e78..`, `0x45f38..`) with `scripts/extract_ds.py`
   and model the two jumptables as explicit dispatch (the pointers are code addresses in the EXE, so
   build a table of C function pointers in the same order as the ROM entries; the order is recoverable
   from the disassembly of the table region).
3. Keep the `(*DAT_000536c2)()` exception hook as a ported function pointer global.
4. Only then port `0x0473bf` (`caseD_3`, 38 B) which is one of the jump-table arms.

## Verification plan

The engine is deterministic over its register file + ROM tables, so a table-driven differential test
is the right shape: seed `DAT_00053668` with known 80-bit values, run one decoded instruction per case
(the opcode bytes are in the ROM), and pin the resulting register file + `DAT_00053654` status bits.
The already-ported FPU helpers (`fx_04794a.c` and the `fp_*_vectors.h` vector sets in `scratch/`) give
the value model to compare against.
