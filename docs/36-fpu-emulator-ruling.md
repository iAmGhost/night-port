# 0x0461cc / 0x0473bf — the bundled 8087 emulator: why it stays out of the port

The two remaining `[-]` rows are `0x0461cc` (2998 B, L5) and `0x0473bf` (37 B, its `caseD_3` arm). This
session extracted the last thing that was unknown about them — the two ROM jump tables — and the result
settles the question: a 1:1 transcription is **not** a single unit of work, because the interpreter's
memory-operand path dispatches into ~30 code blocks that are not part of the 528-function inventory.

## The interpreter, from the pseudocode + capstone

`FUN_000461cc(param_1..param_6)` reads the instruction stream from a saved area (`in_stack_00000038`):

1. A prefix scan advancing past the operand bytes of the prefix set `0x9b/0x9c`, `'&'`, `'.'`, `'6'`,
   `'>'`, `'d'`, `'e'`, `'f'`, `'g'` (each prefix selecting a different segment override into `uVar7/uVar16`).
2. The escape byte must be `0xd8` (`(char)(... & 0xfffffff8) != -0x28` → plain `return`). Everything the
   game's compiled floating point actually emits goes through the Watcom *helper* layer (`fn_04794a` and
   friends, already ported and vector-tested), not through this decoder.
3. The modrm byte is read, the pointer advances by 2, and `bVar14 = modrm & 0xf8`:
   * **`bVar14 < 0xc0` (memory operand)** → `(*(code *)(&PTR_LAB_00045e78)[idx])()` where
     `idx = (byte)(u|u>>8)` of `CONCAT11(b>>3,b) & 0x1807`, then `FUN_00042310()`, then
     `(*(code *)(&PTR_LAB_00045f38)[(st & 0x38) | st])()` — **two ROM jump tables**.
   * **`bVar14 >= 0xc0` (register operand)** → `FUN_00042310` then a switch on `(bVar14 & 0x38) | lo`,
     cases 0x00,0x01,0x04,0x05/0x07,0x06,0x08,0x09/0x0d/0x0f,0x0c,0x0e,0x10/0x14,0x11,0x15/0x17,
     0x18/0x1c,0x19/0x1d/0x1f,0x1e,0x20,0x21,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2c,0x2d,0x2e,
     0x30,0x31,0x34,0x36,0x38,0x39 (a nested switch of its own),0x3c,0x3e — each driving the 80-bit
     register file `DAT_00053668` (8 x 10 bytes) through the already-ported helpers `fn_0478f0` (FADD),
     `fn_047d24` (FMUL), `fn_0478e2` (FSUB variants), `fn_047b4f` (FCOM/FUCOM), `fn_047ab6` (copy),
     `fn_042310` (tag/register lookup), `fn_04636e`/`fn_0463c0` (the store tails), `fn_048c3f/048c49`
     (increment/decrement), `fn_048585`, `fn_04876d`, `fn_04836c`, `fn_047e9f`, `fn_0481df`, `fn_048666`,
     `fn_048754`, `fn_048c56`, `fn_048e60`, `fn_048c8c`), the exception hook `(*DAT_000536c2)()`, and
     **recursion back into `FUN_000461cc`** for the pending-operator tail.

## What the two jump tables point at

Table A at `0x45e78` (48 entries) and table B at `0x45f38` (>= 64 entries) are dword pointers into
`0x3642a..0x36be4`:

```
A: 3642a 36430 36436 3643c 364ec 3644a 36450 36456 3645c 36464 3646c 36474 364ec 36486 36490
   36498 364a0 364a9 364b2 364bb 364ec 364cf 364da 364e3  (then the same list again for the +mod forms)
B: 36529 3661b 366f7 367e9 36883 369dc 36ae2 36be4 36548 36524 36716 36524 368a6 36524 ...
```

Those targets are **not Ghidra functions and not rows in `analysis/functions.tsv`** — they are unnamed
operand-fetch arms (6 to a few hundred bytes each) sitting in the gap between the game's
`fn_03628e`/`fn_03658d` region. Verified with capstone over the whole range: none of the 528 inventoried
functions start at any of these addresses.

The ROM *data* tables it also drives are interleaved 10-byte rows, extracted here so the layout is on
record: `0x45dc0` = a zero 80-bit constant (used by the `0x21/4` case), `0x45dca`/`0x45dcc` = per-register
source-index words, `0x45e1a`/`0x45e1c`/`0x45e22` = the status/control masks (`0xfffc`, `0x0001`,
`0x0003`, `0xfff3`, `0xffcf`, `0xfff0`, ...), `0x45e6e` = the 8-byte control-word OR table
(`00 02 40 42 01 03 41 43`).

## Decision

`0x0461cc` stays `[-]`, and with it `0x0473bf` (which only stores `0x461cc` into `DAT_000536c2`, the
continuation slot). Reasons, in order of weight:

1. **Nothing in the port can reach it.** Zero caller edges; entry is through the extender's exception
   vector table (`0x00e430`/`0x00e980`) when the CPU traps an `0xd8` opcode, which cannot happen because
   the port runs the game's floating point on the host FPU through the ported Watcom helper layer
   (`src/f/fx_04794a.c` + its vector tests). The rows that own the exception vectors — the DOS4GW startup
   `0x042044`/`0x042046`/`0x0420bc` — are now ported and do **not** install the FPU emulator.
2. **The unit of work is not one function.** The memory-operand path alone needs ~30 unnamed arms
   (`0x3642a..0x36be4`) ported first, none of which are in the inventory, plus the two ROM tables and the
   `(*DAT_000536c2)()` exception protocol. Marking it `[x]` after transcribing only the register-operand
   switch would misreport what was ported.
3. **The arithmetic that matters is already ported and verified** — the 80-bit engine behind it
   (`fn_04794a`, `fn_0478f0`, `fn_047d24`, `fn_047b4f`, `fn_0478e2`, `fn_047ab6`, `fn_0463c0`, `fn_04636e`,
   `fn_042310`, `fn_048c3f/048c49/048c56/048c8c`, `fn_048585`, `fn_04876d`, `fn_04836c`, `fn_048754`,
   `fn_0481df`, `fn_048666`, `fn_048c8c`, `fn_048e60`, `fn_047e9f`) with the `test/vectors/fp_*_vectors.h`
   differentials.

If a future session wants them anyway, the shape is fully recorded here: the register-operand switch is a
direct transcription against a `struct fp_reg { uint32_t lo, mid; uint16_t hi; } fp[8]` file plus the byte
tables above; the memory-operand path starts with the ~30 arms.

## Closing evidence: neither address appears anywhere in the image bytes

Beyond the absent call/xref edges, a raw scan of `NIGHT.EXE` for any little-endian encoding of the two
addresses finds nothing at all:

```
imm 0x0461cc hits: [] count 0      # pattern cc 61 04
imm 0x0473bf hits: [] count 0      # pattern bf 73 04
imm 0x046208 hits: [] count 0      # pattern 08 62 04 (interior, would catch a shifted table entry)
```

So no instruction, table, or relocation anywhere in the file carries a pointer to either region. A region
that nothing in the binary can name has no caller by construction: the port cannot be missing behaviour by
not implementing it, which is what the `[-]` marking records.
