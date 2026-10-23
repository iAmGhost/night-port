# 0x0461cc / 0x0473bf: what the remaining port work actually needs

The two `[-]` rows are the last thing between the port and a 528/528 inventory. This note records the
facts that were missing (and partly wrong) in `docs/30-fpu-engine-0461cc-plan.md`, so the port itself can
be executed without re-deriving anything.

## The engine is self-contained — its dispatch tables are its own arms

`scripts/extract_fpu_tables.py` dumps both ROM tables as C initializers (output in
`scratch/fpu_tables.txt`). Every entry is a far pointer `offset:3`, and selector 3 maps
`flat = 0x40000 + offset`, so:

* **table1 @ DS-relative ROM `0x45e78`, 48 entries** (`0x45e78..0x45f37`, table2 starts right after) —
  targets `0x4642a, 0x46430, …, 0x464e3, 0x464ec`: the six-byte operand-form stubs **inside** `fn_0461cc`.
* **table2 @ ROM `0x45f38`, 64 entries** (`b & 0x3f`) — targets `0x46524 … 0x46c9d`: the arithmetic case
  bodies, again **inside** `fn_0461cc`.

So the earlier worry ("ROM jumptables dispatch into ~30 unnamed arms elsewhere") was a misread of the
far-pointer pairs (`642a 0003` = offset `0x642a`, seg `3` — not `0x3642a`). Nothing outside the function is
needed; everything it calls (`fn_042310`, `fn_0478f0`, `fn_047d24`, `fn_047ab6`, `fn_0463c0`, `fn_04636e`,
`fn_0478e2`, `fn_047b4f`, the `fn_048c*` thunks) is already ported.

## Frame and globals to model

The arms address the emulator frame through `EBP`, established at `0x46232`:

```
00046232  lea ebp, [0x364c]        ; DS displacement + 0x50000 -> DAT_0005364c
00046238  mov edi, ds:[ebp + 4]    ; DAT_00053650 = FP control word
0004623c  and edi, 0x3800 ; shr edi, 0xb ; lea edi,[edi+edi*4] ; add edi,edi   ; *10 = register stride
0004624a  jmp dword ptr cs:[ebx*4 + 0x35f38]   ; cs displacement + 0x10000 -> ROM 0x45f38 (table2)
```

Required globals (none exist in the port yet): the FP register file `DAT_00053668` (8 × 10 bytes,
`{lo, mid, hi}` — **must be a named struct array**, per the standing rule about dword fields 4 bytes
apart), control word `DAT_00053650`, status word `DAT_00053654`, scratch register `DAT_000536b8/bc/c0`,
the exception hook pointer `DAT_000536c2`, and the ROM mask tables around `0x45dcc..0x45e6c`
(`g_fxmask_tbl` already covers the `0x45e1c/20/22` triplets).

Note the displacement conventions, both of which bit earlier analyses: DS data immediates are
`+0x50000` (`lea ebp,[0x364c]` → `DAT_0005364c`), cs-relative tables are `+0x10000`, and far-pointer
selector 3 is `+0x40000`.

## Shape of the port

1. prologue: the segment/operand-prefix scan (`0x9b/0x9c`, `& . 6 > d e f g`), the `0xd8` escape gate,
   and the `modrm` split (`0x461cc..0x46251`);
2. `modrm < 0xc0` → table1 arm (operand form) → `fn_042310` → table2 arm;
3. `modrm >= 0xc0` → straight to the `b & 0x38 | b` table2 dispatch;
4. table2 has ~30 distinct arms (`0x46524` is the shared "no such opcode" tail, reached by 8 entries);
   several arms tail-jump back into `0x461cc` (re-entrant pending-op handling) and three of them swap
   register-file triples under `LOCK()`;
5. `0x0473bf` (`caseD_3`) is one of those table2 arms: it writes the synthetic operand descriptor
   (`word [ebp]=0x33f`, `[ebp+4]=0`, `[ebp+8]=0xffff`), stores the continuation `0x861cc` into
   `[ebp+0x76]`, clears EDI and tail-jumps into the engine — it falls out of the engine port for free.

## Why it was not attempted blind in a limited session

846 instructions (`scratch/dis_0461cc.txt`, produced by
`uv run --with capstone python scratch/fdis.py 0x461cc 2998`) and ~30 substantial arms: transcribing it in
one pass risks leaving a half-ported engine inside an otherwise fully verified `f.c`. The staged order
above (prologue + tables + one arm per step, each pinned by a register-file differential check as
`docs/30` proposes) is the safe route.
