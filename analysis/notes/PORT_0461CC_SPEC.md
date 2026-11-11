# 0x0461cc — analysis spec (NOT yet ported): the software x87 FPU emulator

Ground truth: `scratch/dis_461cc.txt` (capstone listing of the full 2998-byte body, produced with
`scratch/vad.py`, which maps **Ghidra flat VA = file offset + 0x2600** in `NIGHT.EXE`;
code segment base 0x10000, data segment base 0x50000 — so `cs:[ebx*4+0x35e78]` == VA 0x45e78 and
`ds:[0x3650]` == VA 0x53650).

## What it is

A **protected-mode 8087/287 emulator** (the classic Borland-style em8087/MCE shipped with 16/32-bit
DOS compilers). The game was linked against it because target PCs may lack a numeric coprocessor.
Three DPMI exception thunks immediately before it install it:

| thunk | VA | role |
|---|---|---|
| INT 7 (ESC / coprocessor-not-available) | 0x04613a | `sti; mov ebp,esp; push [ebp+0x40]; push [ebp+0x44]; mov cx,[ebp+0x3c]; mov [ebp-6],cx; mov ecx,[ebp+0x38]; mov [ebp+0xc],ecx; call 0x461cc; pop ds; pop es; popal; iretd` |
| ESC error path | 0x04615c | rebuilds a register frame from `ss:[eax+...]` (eax = DPMI context ptr at `[esp+0xc]`), then `call 0x461cc`, writes EIP back with `pop dword ss:[eax]` / `pop dword ss:[eax+0x10]`, `iretd` |
| third (0x0461a9) | 0x0461a9 | `sub esp,0xc; push eax; ...; push ds; mov [esp+2],ss; call 0x461cc; popal; iretd` |

So 0x0461cc is **entered as an exception handler**, reading the interrupted frame from its own stack:
`[ebp+0x38]` = instruction pointer of the trapped x87 instruction (read with the *caller's* DS loaded
from `[ebp+0x3c]`), `[ebp+4]/[ebp+6]/[ebp+8]` = segment selectors (CS/SS/DS/ES/FS/GS sources), and
the register file it emulates lives in the emulator's data block.

## Emulator state block (DS base 0x50000; code sets `lea ebp,[0x364c]`)

| DS offset | VA | meaning |
|---|---|---|
| 0x364c | 0x5364c | state block base (ebp in the handler tail) |
| 0x3650 | 0x53650 | status/control dword (`(status>>11)&7` = condition-code/operand-class index used to scale offsets by 10; bits cleared with `&=0xffffc7ff`, set from the 0x45dce table) |
| 0x3654 | 0x53654 | mask word, updated with the 0x45e1a (AND) / 0x45e1c,0x45e22 (OR) tables |
| 0x3668 | 0x53668 | **register file**, 10-byte records: `{u32 lo, u32 hi, u16 exp}` (+0x6c/+0x70/+0x74 for the other slots referenced as DAT_5366c/53670/53671 — byte 9 = sign, XOR 0x80 negates) |
| 0x36b8 | 0x536b8 | scratch fx operand (lo), 0x536bc (hi), 0x536c0 (exp word) |
| 0x536c2 | 0x536c2 | continuation function pointer — called after an operation completes (`(*DAT_000536c2)()`) |
| 0x45dc0/4/8 | code area | fx constant (loaded into the scratch slot by case 0x21/4) |

The 10-byte `{lo,hi,exp}` record is exactly the fx format used by the already-ported engine:
`fx_normalize` 0x04794a, `fx_norm` wrapper 0x0478f0, `fn_047d24` (mul wrapper), `fx_mul64` 0x047d4e,
`fx_compare` 0x047ab6, `fn_048c8c` (negate/square/poly composite), plus 0x0478e2 / 0x047b4f / 0x048666
(see STATUS rows; all ported). **Every arithmetic callee of 0x0461cc is already ported.**

## Decode structure (from the disasm)

```
mov  esi, [ebp+0x38]        ; code pointer (in the trapped code's DS)
mov  ds,  [ebp+0x3c]        ; switch DS to that segment
mov  cx,  [ebp+4] ; mov dx, [ebp+6]
loop: al = [esi]
  al > 0x9b -> (al & 0xf8) == 0xd8 ?  -> ESC/x87 instruction
                 ah = [esi+1]; esi += 2; al &= 7
                 ah < 0xc0: idx = ((ah>>3)&0x18)|(ah&7); call cs:[idx*4+0x35e78]   ; reg-operand forms
                 ah >= 0xc0: esi stored, seg tables, ax==0xf801 special (si=1),
                             then jmp cs:[((ah&0x38)|al)*4 + 0x35f38]             ; mem-operand forms
              else -> jmp cs:[((ah&0x38)|al)*4 + 0x35f38]
  al <= 0x9b -> prefix scanner at 0x46252: '&' -> cx=[ebp+0x3c], '.' -> cx=[ebp+8],
                '6' -> cx=[ebp+6], '>' -> cx=[ebp+4], 'd' -> cx=FS, 'e' -> cx=GS, 'f' -> (nop-ish),
                'g' -> not a prefix (fall through); each sets edx=cx and advances esi.
```
After the first table's handler returns: `es = cx`, `[ebp+0x38] = esi` (store the advanced code
pointer), `esi = ebx` (operand address), then `call 0x42310` (seg-load helper, already `[x]`),
`lea ebp,[0x364c]`, `edi = ((ds:[0x3650] & 0x3800) >> 11) * 10`, and finally
`jmp cs:[ebx*4+0x35f38]` — the per-opclass handler table.

### Tables (file VAs; values are *segment offsets*, add 0x10000 for the VA)
* `0x045e78` — 24 entries, register-form handlers -> 0x4642a, 0x46430, 0x46436, 0x4643c, 0x464ec,
  0x4644a, 0x46450, 0x46456, 0x4645c, 0x46464, 0x4646c, 0x46474, ... (dump: `vad.py dwords 0x045e78 24`)
* `0x045f38` — 32 entries, memory/opcode-class handlers -> 0x46529, 0x4661b, 0x466f7, 0x467e9,
  0x46883, 0x469dc, 0x46ae2, 0x46be4, 0x46548, 0x46524, 0x46716, 0x46524, ... (`vad.py dwords 0x045f38 32`)
* operand/status tables: `0x045dca` (operand index), `0x045dcc` (register index), `0x045dce`
  (status OR-word), `0x045e1a` (AND mask word), `0x045e1c`, `0x045e22` (OR mask word) — indexed by the
  *byte offset* of the operand slot (`iVar4 = statusIndex*10`).
* The switch inside the decomp (`case 0 .. 0x31`) is Ghidra's reconstruction of the second table's
  handlers: add/sub/mul/div by fx_norm/fn_047d24/fn_0478e2/fn_047b4f, compare via fn_047ab6 then the
  "return" continuations 0x0463c0 (compare done) / 0x04636e (arithmetic done), register swaps
  (LOCK/unlock triples), load-constant cases (0x29: 0/1/pi/... fx constants such as 0xcd1b8afe
  d49a784b exp 0x4000), `fld`/`fst` stores into DAT_536b8/bc/c0, and status-mask updates.

## Mid-code entries inside the body (already `[x]` in STATUS as spurious)
0x04636e, 0x0463c0, 0x046d00 are `jmp 0x461cc` continuations; 0x046d32 / 0x046d47 / 0x0473e5 are
`caseD` stubs that call fx_norm / fn_047d24 with `fr+0x1c+EDI / ESI` operands — all ported.

## Port plan (multi-turn)
1. Model the state block as a `struct fpu_state` with the 10-byte fx register file (host pointers,
   watch the widened-pointer hazard: keep records as packed 10-byte structs, do NOT widen fields).
2. The exception-frame ABI cannot be reproduced literally on a 64-bit host: model it as
   `void fn_0461cc(struct fpu_frame *f)` where `f->eip` (was [ebp+0x38]), `f->ds_sel`, `f->cs/ss/...`
   and the code stream is a host pointer; document the deviation (segments collapse to the flat model,
   as done for the other seg helpers: `seg_ds_load` 0x042310 is a no-op).
3. Transcribe in slices: (a) prologue + prefix scanner + decode, (b) table 0x45e78 handlers,
   (c) table 0x45f38 handlers (the big switch), (d) status/constant tables + constants,
   (e) the continuations. Run the fx differential vectors (scratch/fp_*.py) per slice.
4. Note for the *playability* goal: the emulator intercepts x87 instructions in the game's own
   machine code. The port's C code uses host FP instead, so 0x0461cc is required for the
   "all 528 ported" contract but is **not** on the critical path for a running game. Keep the port
   as a faithful, testable unit (decode -> fx ops -> status) rather than wiring it into the runtime.
