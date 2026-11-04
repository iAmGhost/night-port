# fn_047d4e — 64-bit fixed-point multiply (fx_mul64) — READY-TO-PORT trace

108 insns. Ghidra name: **fx_mul64**. Register-ABI (no stack args, RET 0).
This is the FX multiply primitive; unblocks nothing directly but is a core FPU op.
Its only "callee" (0x047e3d) is a Ghidra split of its OWN loop (JMP 0x047e3d at 0x047da1
is the overflow sink; 0x047e3d = `MOV CX,0x7fff; MOV EDX,0x80000000; SUB EAX,EAX; JMP 0x047e33`).
No real unported callee -> PORTABLE.

## Register-ABI
- in : EAX=a_lo, EDX=a_hi  (A = a_hi:a_lo, 64-bit mantissa)
       EBX=b_lo, ECX=b_hi  (B = b_hi:b_lo, 64-bit mantissa)
       ESI=exp, 16-bit REPLICATED: ESI = (exp<<16)|exp   (see 0x047d78 XCHG/ROL/SAR)
- out: EAX=r_lo, EDX=r_hi  (64-bit product mantissa)
       ESI=r_exp  (final 16-bit exponent; 0x7fff on overflow, 0 on zero-result)

## Control flow (labels in disasm)
- 0x047d4e entry: if (a_lo==0 && a_hi==0) && ((ESI&0xffff)*2)==0 -> RET (EAX=0; note 0x047d56 ADD SI,SI / 0x047d59 JNZ 0x047d5c; 0x047d5c RCR SI,1 sign-extend).
  Actually: a==0 short-circuits to the b-check (JNZ 0x047d5f skips the RCR). If a==0 and SI==0 -> RET 0.
- 0x047d5f: if (b_lo==0 && b_hi==0) && ((ESI & 0x7fff0000)==0) -> EAX=EDX=ESI=0, RET.
- 0x047d76 main:
  - 0x047d78 XCHG ESI,ECX (ESI<->ECX); 0x047d7a MOV EDI,ECX(=old ESI=exp);
    0x047d7c ROL EDI,16; 0x047d7f SAR EDI,16  -> EDI = sign-extended high half of exp field
    0x047d82 SAR ECX,16 -> ECX = sign-extended low half
    0x047d85 AND EDI,0x80007fff; 0x047d8b AND ECX,0x80007fff  (keep bit15 + bits0-13, clear bit14)
    0x047d91 ADD ECX,EDI  -> ECX = sum of the two sign-ext halves (both = exp, so = 2*exp)
    0x047d93 SUB CX,0x3fe -> CX = 2*exp - 0x3fe  (de-bias)
  - 0x047d98 JC 0x047da6 (if 2*exp-0x3fe underflowed, i.e. 2*exp < 0x3fe -> CX still big/neg)
    0x047d9a CMP CX,0x7fff; 0x047d9f JC 0x047da6; 0x047da1 JMP 0x047e3d (OVERFLOW -> 0x8000000000000000)
    (i.e. if (2*exp-0x3fe) is in [0, 0x7fff) -> compute; else overflow path at 0x047da6)
  - 0x047da6 CMP CX,-0x40; 0x047daa JGE 0x047db7; else (CX < -0x40) -> EAX=EDX=ECX=0, JMP 0x047e33 (UNDERFLOW -> 0)
  - 0x047db7 64x64 -> 128 multiply:
      push ecx(exp-adj), ebp(0), esi(=b_hi after xchg), edx(=a_hi), eax(=a_lo)
      0x047dbd MUL EBX            # EAX:EDX = a_lo * b_lo
      0x047dbf XCHG EAX,ESI ; 0x047dc0 MOV ECX,EDX  # save
      0x047dc3 MUL EDX            # a_lo * a_hi
      ... accumulate cross terms (a_lo*b_hi, a_hi*b_lo, a_hi*b_hi) with ADC chains ...
      -> 128-bit product in {EBP:EDI:ECX:...} then
  - 0x047de4 rounding/normalize block:
      JS 0x047dee (sign); 0x047de6 ADD EBX,EBX/ADC EAX,EAX/ADC EDX,EDX (double result, carry); DEC CX
      0x047dee ADD EBX,EBX; 0x047df0 JNC 0x047e1a (carry-out?)
      0x047df2 JNZ 0x047e01; 0x047df4 OR ESI,ESI; 0x047df6 SETNZ BL; 0x047df9 SHR EBX,1
      0x047dfd MOV ESI,EAX; 0x047dff SHR ESI,1 ; 0x047e01 ADC EAX,0/ADC EDX,0 (round-up)
      0x047e07 JNC 0x047e1a; 0x047e09 RCR EDX,1; 0x047e0b RCR EAX,1; 0x047e0d INC CX; 0x047e0f CMP CX,0x7fff; JZ 0x047e3d (overflow)
  - 0x047e1a OR CX,CX; 0x047e1d JG 0x047e33 (CX>0 -> done); 0x047e1f JNZ 0x047e25 (CX<0); else (CX==0) CL=1
    0x047e25 NEG CX (CX<0 -> make positive)
    0x047e28 SUB EBX,EBX; 0x047e2a SHRD EAX,EDX,CL; 0x047e2d SHRD EDX,EBX,CL  (right-shift product by |CX|)
    0x047e30 SUB CX,CX
  - 0x047e33 (epilogue): ADD ECX,ECX; RCR CX,1; MOV ESI,ECX  -> ESI = final exp (re-bias + carry); POP EDI/EBP; RET
  - 0x047e3d (overflow sink): MOV CX,0x7fff; MOV EDX,0x80000000; SUB EAX,EAX; JMP 0x047e33  -> returns {EDX:EAX}=0x8000000000000000, ESI=0x7fff

## Decompile (Ghidra) — structural reference (uVar9=2*exp de-biased, uVar8=uVar9+0xc002)
  if a==0 && (esi16*2)==0: return 0
  if b==0 && (unaff_ESI & 0x7fff0000)==0: return 0
  uVar9 = (ushort)( signext(esi>>16)&0x80007fff + signext(swap16(esi)>>16)&0x80007fff )   # = 2*exp
  uVar8 = uVar9 + 0xc002
  if uVar9 >= 0x3fe || uVar8 >= 0x7fff:  return 0x8000000000000000   # overflow
  if (short)uVar8 < -0x40: return 0                                     # underflow
  <128-bit multiply + round + normalize>
  return uVar2 (64-bit)

## Port plan (dedicated fresh pass)
- Mirror the disasm in C with a carry-tracking 128-bit multiply (uint64 product, then the
  JS/double/ADC round block, then SHRD normalize). Port as asm-mirror (goto labels) because
  of the back-edge-free but flag-dense structure; the multiply is a straight-line 128-bit MAC.
- Python ref: mirror the EXACT asm (carry chains, RCR, SHRD, the 0x80007fff masks).
- Differential vectors: zero/under/overflow corners, random a/b mantissas (with bit31 set for
  the normalized path), exp values spanning [0x3fe-..., 0x7fff].
- Returns {lo,hi,exp} via out struct (register-ABI).
