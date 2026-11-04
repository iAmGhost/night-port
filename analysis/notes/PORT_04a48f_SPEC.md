# 0x04a48f SPEC — double → decimal-string formatter (L4/L3, 192 insn, 485 B)

Deferred-to-port spec. All 5 callees are PORTED; this is a clean win but 192 insn +
reg+stack callee ABI + FPU => port from a FRESH context using this spec (pattern:
defer 150+ insn C mirrors; cf. 0x044ab5/0x0487b0).

## Identity / caller
- **0x04a48f** = a double → fixed/exponent decimal-ASCII string formatter.
- Sole caller: **FUN_00049e80** @0x00049fe0 (L4, 217 insn — itself unported/deferred;
  part of the same FPU cluster). So 0x04a48f is a LEAF-ish callee of 0x049e80.
- "self-contained, 0 abs-mem-ops" (journal 2026-07-08): it only reads/writes through
  the 7 pointer/arg slots + its own stack locals; no global memory ops.

## ABI (all 7 args on the STACK)
Prologue: `PUSH EBP; MOV EBP,ESP; PUSH EBX/ECX/EDX/ESI/EDI; SUB ESP,0x2c`. Epilogue:
`LEA ESP,[EBP-0x14]; POP EDI/ESI/EDX/ECX/EBX/EBP; RET 0x1c`.
- 7 stack args: **A1=[EBP+8], A2=[EBP+0xc], A3=[EBP+0x10], A4=[EBP+0x14], A5=[EBP+0x18], A6=[EBP+0x1c], A7=[EBP+0x20]** (RET 0x1c = 28 B = 7 dwords).
- All read from the stack (no register-arg inputs to 0x04a48f itself).
- **Return = A7 (the out buffer pointer)** in EVERY path:
  - inf/nan path: `EAX=EDX(=A7); JMP epilogue`.
  - normal path: final copy loop then `EAX=EDI(=A7)`.
  So the C signature returns the out buffer (`char *`).

## Arg semantics (from disasm trace)
- **A1:A2 = the input double** (lo32:A1, hi32:A2). `d = (uint64)A2<<32 | (uint32)A1`.
  A1 also read as [EBP+8]; the double occupies [EBP+8..EBP+0xf]; **[EBP+0xf] = A2's high
  byte = the double's SIGN byte**.
- **A3 (→EBX) = output WIDTH** (digit-slot count). Clamped: `if EBX<0 skip; EBX>0x10→0x10`
  (0x04a552-0x04a55f), later `EBX<1→1; EBX>0x11→0x11` (0x04a622-0x04a633); passed as
  the `width` arg to fmt_u64_fixed.
- **A4 (→ECX) = a pointer to a 32-bit slot** that is: initialized 0 ([A4]=0 @0x04a4b5),
  WRITTEN by fx_double_exp (the biased-unbiased exponent) as its `uint32_t *out`,
  then READ (*A4 → EDX @0x04a51c) and reused/incremented as a running "written count"
  (ADD [ECX],EAX @0x04a5ab; INC [ECX] @0x04a5ec; *A4 read as EAX for fn_04a367 @0x04a59b).
  DUAL ROLE: exp-out first, then a counter. Model as one `uint32_t *`.
- **A5 (→EDI) = pointer to a 32-bit flag/length out**: =0 init @0x04a4ae; =0xffffffff
  if d<0 (negative flag) @0x04a505; =EAX (a small byte-count / high-bytes-OR value)
  @0x04a614. Model as `uint32_t *`.
- **A6 = format selector** compared to 0x46 ('F') at 3 sites (0x04a54b, 0x04a5ad, 0x04a61b):
  'F' = fixed-point. (Other values = other formats; the 'F' gates take the fixed path.)
  Model as `uint32_t format`.
- **A7 (→EDX/EDI) = the output string buffer** (`char *`). Passed to fx_double_naninf as
  `out`, and receives the final rendered digits. Returned.

Suggested C signature:
  `char *fn_04a48f(double d, uint32_t width, uint32_t *p_expcount, uint32_t *p_flag, uint32_t format, char *out);`
  (return out). Verify each arg's role by cross-checking the diff test outputs.

## Callee calls (all PORTED; C sigs from f.h)
1. `int fx_double_naninf(double d, char *out)` (0x04a2a7): called with (A1,A2) as d, out=A7.
   If it returns !=0 (d is inf/nan) → early return A7 (buffer holds "inf"/"nan" w/ sign).
2. `void fx_double_exp(double d, uint32_t *out)` (0x04a9da): called with d=(A1,A2), out=A4.
   *A4 = (hi>>20)&0x7ff - 0x3fe (unbiased exp; 0 if d==0).
3. `void fn_04a7fa(double a, double *out)` (0x04a7fa): *out = a - fpu_rndint(a) = fractional part.
   Called @0x04a575 with d=(A1,A2) [absolute value], out=&[EBP-0x24] (a stack double).
   Then `FLDZ; FCOMPP; FNSTSW; SAHF; JNZ` compares frac vs 0.
4. `int32_t fn_04a367(uint32_t arg0, uint32_t arg1, int32_t arg2, const uint32_t *arg3, uint32_t eax_in)` (0x04a367):
   FP cluster; return = fn_04a814(arg2,arg3). Called TWICE (@0x04a59d, @0x04a5df) with
   EAX=[ECX] (a register arg) + 4 stack pushes. Return = a digit-group count (0 if none).
5. `void fmt_u64_fixed(const uint64_t *val, char *buf, int width)` (0x04a8b9):
   Renders a 64-bit unsigned as a fixed-width right-justified ASCII field. Called @0x04a644
   with val=&[EBP-0x2c] (the accumulated 64-bit integer), buf=&[EBP-0x40], width=EBX(A3).

## Main-algorithm skeleton
1. `r = fx_double_naninf(d, out=A7); if (r) return A7;`  (inf/nan → done)
2. locals: 4 words at [EBP-0x2e/-0x2c/-0x2a/-0x28] = 0 (the 64-bit accumulator, LE
   lo:[EBP-0x2c] hi:[EBP-0x2e]... — verify endianness; SAR-0x10 OR-chain tests hi bytes).
   [EBP-0x24] double (frac), [EBP-0x40] char buffer, [EBP-0x1c] dword, [EBP-0x18] byte.
3. **Zero test:** if A2&0x7fffffff==0 && A1==0 → d==0 → jump to 0x04a622 (render "0" path).
4. **Sign:** `FLDZ; FCOMP d; SAHF;` if 0.0 > d (i.e. d<0): [A5]=-1; `[EBP+0xf] ^= 0x80`
   (clear the double's sign bit → d:=|d| in memory). (Note: modifies the double in place.)
5. **Exponent → digit estimate:** `fx_double_exp(|d|, A4); e=*A4; e=abs(e);`
   `EDX = (3*abs(e)+5); e = (EDX*0x80000000... signed) / 10` → `n_digits_est = (3*abs_e+5)/10`.
   (IDIV by 10 of the signed (3e+5).)
6. **Width clamp:** `EBX(A3): if <0 skip-all; else clamp [1,16].` (0x04a552-0x04a55f)
7. **Fractional part:** `fn_04a7fa(|d|, &frac); FLDZ;FCOMPP;... if frac==0 && EBX<=EDX: AH=1,
   EBX=EDX, [EBP-0x18]=1` (integer-valued fast path) (0x04a581-0x04a58b).
8. **Two fn_04a367 calls** to pull digit groups from |d| into the [EBP-0x2c] 64-bit
   accumulator, updating [A4] (count) and EBX; guarded by the 'F' format check + [EBP-0x18].
9. **High-bytes-OR test** (0x04a5ee-0x04a614): OR the 4 high bytes (each SAR 0x10) of the
   accumulator; if all zero → [A5]=that value (0) (means accumulator underflow/empty).
   Else if A6=='F': EBX -= EDX.
10. **Final render:** clamp EBX to [1,0x11]; `fmt_u64_fixed(&[EBP-0x2c], &[EBP-0x40], EBX)`;
    copy [EBP-0x40] string → A7 (out); return A7.

## Key formulas
- digits estimate: `(3*abs(unbiased_exp) + 5) / 10`  (signed).
- 0x46 = 'F' (fixed format gate).
- width clamp: [1,16] (early) then [1,17] (final, for fmt_u64_fixed).

## Risks / open questions (resolve during port)
- A4 DUAL role (exp-out then counter) — model as one `uint32_t *`; confirm *A4's value flow.
- The 64-bit accumulator layout ([EBP-0x2e] hi / [EBP-0x2c] lo? or 4 independent groups) —
  the SAR-0x10 OR-chain + fmt_u64_fixed(val=&[EBP-0x2c]) implies a 64-bit LE integer with
  lo at [EBP-0x2c] and hi at [EBP-0x2e]; BUT fmt_u64_fixed takes `const uint64_t *val` at
  [EBP-0x2c] (8 bytes = [EBP-0x2c..0x33]) — confirm val reads [EBP-0x2c]..[EBP-0x33].
- Sign handling writes `[EBP+0xf]^=0x80` (the in-memory double's sign byte); the C port
  can just take `d = fabs(d)` after recording the negative flag (equivalent, cleaner).
- fn_04a367's exact digit-group semantics (it's the "extract next 10^k group" op) — read
  its port (f.c:8532) + fn_04a814 to mirror the accumulation.
- The final string-copy loop (0x04a64a-0x04a664) copies [EBP-0x40]→A7 2 bytes/iter; it
  looks like a word copy but writes AL (1 byte) — verify it's a full memcpy of the NUL-
  terminated field (likely just `strcpy(out, buf)` in C).

## Test plan
- Wiring test (test_fn_04a48f): a handful of doubles × width × format, asserting:
  - inf/nan: out contains "inf"/"nan" (±), return==out.
  - 0.0: out=="0" (or per-width).
  - finite: out is the fixed/exponent string; *flag, *p_expcount sane.
- Prefer KNOWN-VALUE vectors (format 1.0→"1", 3.14→"3.14"-ish, 123456 with width N,
  1e30, -2.5) over a pure Python mirror, to catch shared-understanding bugs. If a Python
  ref is used, cross-check against at least 3 hand-computed known values.
- Register in test_f1.c main() after test_fn_044c23.

## Full disassembly (192 insn, 0x04a48f-0x04a672)
See the block captured this session; the mnemonic list:
PUSH EBP; MOV EBP,ESP; PUSH EBX; PUSH ECX; PUSH EDX; PUSH ESI; PUSH EDI; SUB ESP,0x2c;
MOV EBX,[EBP+0x10]; MOV ECX,[EBP+0x14]; MOV EDI,[EBP+0x18]; MOV EDX,[EBP+0x20]; PUSH EDX;
MOV ESI,[EBP+0xc]; PUSH ESI; MOV EAX,[EBP+8]; MOV [EDI],0; PUSH EAX; MOV [ECX],0;
CALL 04a2a7; TEST EAX,EAX; JZ 04a4cb; MOV EAX,EDX; JMP 04a669; XOR EDX,EDX; XOR ESI,ESI;
MOV [EBP-0x2c],DX; MOV [EBP-0x2a],SI; MOV [EBP-0x28],DX; MOV [EBP-0x26],DX; MOV EDX,[EBP+0xc];
MOV ESI,[EBP+8]; TEST EDX,0x7fffffff; JNZ 04a4f5; TEST ESI,ESI; JZ 04a622; FLDZ;
FCOMP qword [EBP+8]; FNSTSW AX; SAHF; JBE 04a4e...; MOV AH,[EBP+0xf]; XOR AH,0x80; MOV [EDI],
-1; MOV [EBP+0xf],AH; PUSH ECX; MOV EAX,[EBP+0xc]; PUSH EAX; MOV EDX,[EBP+8]; PUSH EDX;
CALL 04a9da; MOV EDX,[ECX]; FSTP ST0; TEST EDX,EDX; JGE 04a526; NEG EDX; MOV EAX,EDX;
SHL EDX,2; SUB EDX,EAX; ADD EDX,5; MOV ESI,10; MOV EAX,EDX; SAR EDX,0x1f; IDIV ESI;
MOV ESI,[ECX]; MOV EDX,EAX; TEST ESI,ESI; JGE 04a546; NEG EDX; MOV EAX,[EBP+0x1c]; MOV [ECX],
EDX; CMP EAX,0x46; JNZ 04a552; ADD EBX,EDX; TEST EBX,EBX; JL 04a5ee; CMP EBX,0x10; JLE
04a564; MOV EBX,0x10; XOR AL,AL; MOV [EBP-0x18],AL; LEA EAX,[EBP-0x24]; PUSH EAX; MOV ESI,
[EBP+0xc]; PUSH ESI; MOV EAX,[EBP+8]; PUSH EAX; CALL 04a7fa; FLDZ; FCOMPP; FNSTSW AX; SAHF;
JNZ 04a58e; CMP EBX,EDX; JLE 04a58e; MOV AH,1; MOV EBX,EDX; MOV [EBP-0x18],AH; LEA EAX,
[EBP-0x2c]; PUSH EAX; PUSH EBX; MOV ESI,[EBP+0xc]; PUSH ESI; MOV EAX,[EBP+8]; PUSH EAX;
MOV EAX,[ECX]; CALL 04a367; MOV ESI,EAX; TEST EAX,EAX; JZ 04a5ee; MOV ESI,[EBP+0x1c]; ADD [ECX],
EAX; CMP ESI,0x46; JZ 04a5b8; CMP BYTE [EBP-0x18],0; JZ 04a5ba; ADD EBX,EAX; CMP EBX,1; JGE
04a5c6; MOV EBX,1; JMP 04a5d0; CMP EBX,0x10; JLE 04a5d0; MOV EBX,0x10; LEA EAX,[EBP-0x2c];
PUSH EAX; PUSH EBX; MOV EAX,[EBP+0xc]; PUSH EAX; MOV ESI,[EBP+8]; PUSH ESI; MOV EAX,[ECX];
CALL 04a367; TEST EAX,EAX; JLE 04a5ee; MOV EAX,[ECX]; INC EAX; INC EBX; MOV [ECX],EAX;
MOV EAX,[EBP-0x2c]; SAR EAX,0x10; MOV [EBP-0x1c],EAX; MOV EAX,[EBP-0x2e]; MOV ESI,[EBP-0x1c];
SAR EAX,0x10; OR EAX,ESI; MOV ESI,[EBP-0x2a]; SAR ESI,0x10; OR EAX,ESI; MOV ESI,[EBP-0x28];
SAR ESI,0x10; OR EAX,ESI; JNZ 04a622; MOV [EDI],EAX; MOV EDI,[EBP+0x1c]; MOV [ECX],EAX; CMP
EDI,0x46; JNZ 04a622; SUB EBX,EDX; CMP EBX,1; JGE 04a62e; MOV EBX,1; JMP 04a638; CMP EBX,0x11;
JLE 04a638; MOV EBX,0x11; LEA EAX,[EBP-0x2c]; LEA EDX,[EBP-0x40]; MOV EDI,[EBP+0x20]; LEA ESI,
[EBP-0x40]; CALL 04a8b9; PUSH EDI; MOV AL,[ESI]; MOV [EDI],AL; CMP AL,0; JZ 04a666; MOV AL,
[ESI+1]; ADD ESI,2; MOV [EDI+1],AL; ADD EDI,2; CMP AL,0; JNZ 04a64a; POP EDI; MOV EAX,EDI;
LEA ESP,[EBP-0x14]; POP EDI; POP ESI; POP EDX; POP ECX; POP EBX; POP EBP; RET 0x1c.