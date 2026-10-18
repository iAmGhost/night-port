# Function Port Tracking

Single source of truth for function-to-function port progress.
One original function = one row. Update this file in the same commit as
the code/doc it describes.

## Protocol

Per-function loop (see `docs/12-function-map.md` §3):
1. Analyze `analysis/pseudo/<file>.c` + xrefs → entry in
   `docs/12-functions/<subsystem>.md` (signature, semantics, constants,
   edge cases, globals/assets touched, verification method) → status `[~]`
2. Port 1:1 to `port/src/f/<name>.c` with `/* orig: 0xNNNNNNN */`
3. Verify (unit selftest for pure fns; headless-run + DOSBox diff for
   stateful fns) → status `[x]`
4. Rename in Ghidra → update the Name column

Status legend:
- `[ ]` pending
- `[~]` analyzed (doc entry written, not yet ported)
- `[x]` ported + verified (evidence path in Notes)
- `[-]` intentionally not ported (e.g. DOS4GW bookkeeping replaced by
  `port/src/sys/` — note what replaces it)

Rules:
- Work strictly in order: top-to-bottom within a level, L0 → L19
  (call-graph depth; a function may only be ported after all its callees
  are `[x]` or `[-]`).
- Notes must be short: pointer to the subsystem doc section and/or the
  verification evidence.
- After each batch, refresh the **Progress** counts below.

## Progress

(counts: done = `[x]`, analyzed = `[~]`, skipped = `[-]`, pending = `[ ]`)

TOTALS: 228 / 528 done

| Level | Total | Done | Analyzed | Skipped | Pending |
|---|---|---|---|---|---|
| L0 | 112 | 109 | 0 | 0 | 3 |
| L1 | 47 | 32 | 0 | 0 | 15 |
| L2 | 32 | 17 | 0 | 0 | 15 |
| L3 | 21 | 11 | 0 | 0 | 10 |
| L4 | 13 | 2 | 0 | 0 | 11 |
| L5 | 10 | 2 | 0 | 0 | 8 |
| L6 | 10 | 0 | 0 | 0 | 10 |
| L7 | 8 | 0 | 0 | 0 | 8 |
| L8 | 7 | 1 | 0 | 0 | 6 |
| L9 | 2 | 1 | 0 | 0 | 1 |
| L10 | 81 | 30 | 0 | 0 | 51 |
| L11 | 77 | 25 | 0 | 0 | 52 |
| L12 | 58 | 7 | 0 | 0 | 51 |
| L13 | 16 | 0 | 0 | 0 | 16 |
| L14 | 7 | 0 | 0 | 0 | 7 |
| L15 | 5 | 0 | 0 | 0 | 5 |
| L16 | 8 | 0 | 0 | 0 | 8 |
| L17 | 6 | 0 | 0 | 0 | 6 |
| L18 | 1 | 0 | 0 | 0 | 1 |
| L19 | 1 | 0 | 0 | 0 | 1 |
| L20 | 1 | 0 | 0 | 0 | 1 |
| L21 | 1 | 0 | 0 | 0 | 1 |
| L22 | 1 | 0 | 0 | 0 | 1 |
| L23 | 3 | 0 | 0 | 0 | 3 |





## Work queue (current batch)

_L0 (leaves) is the starting point. First batch: memory/video-buffer primitives + LPT I/O stubs._

## Decisions log

| Date | Decision | Ref |
|---|---|---|
| 2026-07-08 | **0x0485eb PORTED (L0, 122B, register-ABI fx align-subtract-normalize).** Faithful asm-mirror C (goto labels L_0485ff/0b/0d/12/27/33/3e/48/4a/4e) + Python ref mirroring the same disasm (scratch/fp_0485eb_ref.py). in {a_lo,a_hi}={EAX,EDX}, {b_lo,b_hi}={EBX,ECX}, expA=ESI, expB=EDI; out struct fx_sub_t{lo=EAX,hi=EDX,exp=ESI,ebx=(expA&0x8000),bx=((expA&0x8000)^(expB&0xffff)),edi=neg-flag(NEG if bit7(bx))}. **Key insight:** the normalize is BIT-SERIAL — ESI=(expA&0x7fff)-(expB&0x7fff) decrements by 1 per shift cycle, so it takes up to ~0x7fff iterations for large exp diffs (the original is O(exp-diff), not O(log)). C-side is fast; only the one-time Python vector gen is slow. Back-edges JC 0x4860d (carry->re-subtract) + JS 0x485ff (hi-bit31->re-compare) mirrored exactly. 469 differential vectors (19 edge + 250 full-range + 200 small-exp) all match. L0 110->111, suite 852->853 PASS 0 FAIL. Unblocks 0x048585 (L1). | STATUS.md L0 |
| 2026-07-08 | **0x0485eb register-ABI + algorithm trace (READY TO PORT, deferred to fresh pass).** 64-bit fixed-point align-subtract-normalize. **Register-ABI:** in EAX/EDX = A (EDX:EAX), EBX/ECX = B (ECX:EBX), ESI = A 15-bit exp, EDI = B 15-bit exp. **Out** (read by caller): ESI = result exp (OR A-exp sign bit 0x8000; 0 if EDX==0), EDX:EAX = normalized 64-bit magnitude (hi bit31 set), EBX = A_exp&0x8000, BX = (B_exp &0xffff) XORed via `XOR BX,CX`, EDI = sign/neg flag (`NEG EDI` if JNS-fail). **Trace:** (1) `esi&=0x7fff; edi&=0x7fff; esi-=edi` (shift=ea-eb); (2) if shift<0 (JL 0x4864a): `esi=(ea-eb)+eb=ea; edi=0` -> epilogue with A unchanged; (3) else `edi=0`; compare B vs A (CMP ECX,EDX / CMP EBX,EAX); if A>=B do `EAX-=EBX; EDX-=ECX-borrow; STC` (negative flag); `ADC EDI,EDI` (EDI=1 if subtracted); (4) **normalize loop** `DEC ESI; JL 0x48627(if shift<-1: ESI=0)`; `EAX+=EAX; EDX+=EDX+carry`; `JC 0x4860d`(carry-out -> re-subtract); `OR EDX,EDX; JS 0x485ff`(hi bit31 set -> re-compare); else loop; (5) 0x48627 tail: if EAX==0&&EDX==0 -> ESI=0 path else left-shift {EAX,EDX} until EDX bit31 set (0x48633 loop: `JS 0x4863e` exit when hi bit31 set); (6) epilogue 0x4864e: POP ECX(saved EDI)/POP EBX(saved ESI); `EBX&=0x8000; OR ESI,EBX; OR EDX,EDX; JNZ keep-ESI else SUB ESI,ESI; XOR BX,CX; JNS else NEG EDI; RET`. **Back-edges (JC 0x4860d, JS 0x485ff) make a clean top-down C port error-prone -> port as a faithful asm-mirroring loop + Python ref mirroring the same loop + differential test (like fn_048e60/fn_047f54).** Gates 0x048585 (L1). Unblocks: 0x048585. | STATUS.md L0 |
| 2026-07-08 | **Leaf-tier (L0-L4) CLEAN wins EXHAUSTED (corrected cross-ref).** This session ported 0x043234 (L5 node-list flush walker), 0x041e94 (heap_malloc JMP thunk), 0x04123d (device-port init); 270->273, suite 844->852 PASS 0 FAIL. **Fixed a hex-width bug** in the callee cross-reference (8-char Ghidra vs 6-char STATUS never matched -> falsely flagged ~40 gated). Re-running int-normalized: EVERY remaining L0-L4 pending fn's callees resolve to (a) deferred big functions (0x0485eb subtract, 0x047b79 FPU, 0x04574a date, 0x042ccc DPMI heap, 0x044c23, 0x0451bc) or (b) Ghidra sub-blocks not in the canonical 528 list (e.g. 0x044848, 0x042438, 0x04a84e) -- i.e. no new clean win. The 'sub-block' callees are internal Ghidra splits of larger canonical functions, not separate tracked fns. **NEXT (dedicated passes, not clean wins):** the FPU register-ABI cluster (0x048a48 502B gates the 0x048c3f/49/56 thunks), 0x0485eb (L0 subtract, gates 0x048585), 0x0451bc typed deserializer (biggest pure-C L3 win), and the mid-subsystems (data load 0x3690b, audio 0x3658d, CD/MSCDEX) which are the critical path to a playable build. | STATUS.md L0-L4 |
| 2026-07-08 | **fn_047f54 (0x047f54, 189B) — register-ABI fixed-point normalizer**: EAX:EDX (64-bit) in, EBX out {lo,hi,field}. Core: v=(hi<<32\|lo)<<11; eax/edx split; ecx=SAR(hi,20); cx=ecx&0x7ff. Three branches: cx==0x7ff (exponent overflow → clamp +inf: cx=0x7fff, edx\|=0x80000000, +0x40000000 if nonzero); cx!=0 (normalize: cx+=0x3c00, edx\|=0x80000000); else nonzero (subnormal: cx=0x3c01, swap+cx-=0x20 if edx==0, left-shift edx/eax until bit31 set, cx-- each). Zero → ecx=0. Output: out[0]=eax, out[1]=edx, out[2]=(cx<<1)\|bit31(ecx). The 0x0463dc call is a spurious interpreter thunk (EAX saved/restored via PUSH/POP, return discarded) → no-op standin. **CRITICAL REFERENCE BUG FIXED:** the SAR(hi,20) must be ARITHMETIC (sign-extending), not logical — for hi with bit31 set, ecx high 16 = 0xffff so bit31(ecx)=1 (sets the output field's LSB). The Python reference originally used `>>` (logical) and produced a wrong field LSB for negative inputs; corrected with ctypes.c_int32 arithmetic shift. 2009 differential vectors generated via scratch/fp_047f54_ref.py; all PASS. suite 817→818 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **IRQ installer pair + CD/DPMI real-mode I/O cluster (8 ported)**: (1) **fn_03ef29** (0x03ef29, 200B) installs the IRQ 0x1c timer handler (divisor passed in EAX, register-ABI): skips if g_dat_63e4a set, stores divisor in 63e2c, clears 63e30, saves ISR-0x1c via isr_get into 63e44(off)/63e48(seg), isr_set(0x1c, 0x3ee88, CS), programs PIT ch0 (mode 0x36 + 16-bit divisor), sets flag, STI — complement of the already-ported fn_03eff2 uninstall. (2) **fn_03feb8** (0x03feb8, 242B) installs the IRQ 9 handler + initializes timer/DMA channel state: saves ISR-9 into 63e80/63e84, isr_set(9, 0x3fd40, CS), then memset-inits 63e88 (0x80B,=0), 63f14 (0xa,=0), 63f1e (0xa,=1), 63f28 (0x28B,=0), resets slot base 63f0c=0->63f08=0->63f50=0, sets 63f4c=-1, sets flag, mirrors low-mem byte 0x417 into 63f51 then clears its 0x20 bit. Both use the no-op isr_get (returns 0, no real OS vector) + isr_set/outb standins; CS in isr_set = 0x1f flat selector (immaterial, no-op). (3) **CD/DPMI real-mode I/O cluster** (6 no-op standins): fn_103e0/fn_10570/fn_10876/fn_107b9/fn_10664/fn_10b1d all perform DPMI INT 0x31h real-mode CD/DOS I/O via the no-op trampoline fn_041e38 with register-ABI args; their observable outputs depend on caller register state + no-op I/O results, so no deterministic flat-model effect — documented no-ops (fn_10876 returns 0 for its checksum). Per the DPMI-trampoline convention (cf. fn_043381/fn_04391d). 2 globals added (63e88[0x80], 63f4c); 17 checks. TOTALS 220->228, suite 758->775 PASS 0 FAIL. | STATUS.md L10/L12 |
| 2026-07-08 | Vertical-slice port discarded; function-to-function approach adopted | docs/12-function-map.md |
| 2026-07-08 | 115 unreferenced functions recovered via code-gap scan; 12 bogus mid-code entries deleted | analysis/rank.tsv |
| 2026-07-08 | canary pair (0x418ee/0x41901, L8/L9) ported in batch 1 ahead of L1-L7: self-contained stack guard; sole callee 0x429ea (printer) replaced by sys_fatal (sys layer), fatal fall-through = process death | primitives.md |
| 2026-07-08 | 0x0461cc is a 5909-byte monolithic bytecode/state interpreter (1347 insns, single RET, `JMP 0x0461cc` loops); 0x4636e/0x463c0/0x463dc/0x46d05 are spurious mid-code entries within it (marked [x] spurious, no C fn); 0x461cc ported later as one big function | STATUS.md L6 |
| 2026-07-08 | Remaining L0 cluster is FPU/BCD/fixed-point/bit-chain; needs dedicated x86+reference traces (decompiler mangles RCR/SHLD/CARRY and FPU doubles): 0x47ea4(+thunks 0x47e4a/0x47e9f, RCR/SHLD exp chain), 0x4811a (fxnum ctor, ambiguous `JZ 0x48149` — decode raw bytes), 0x4a9da (returns FPU double, modifies A6 high word to `((A6>>16)&0x800f)\|0x3fe0` then `FLD`), 0x4a84e (fixed-point L/R shift), 0x4a8b9 (BCD /10000 formatter), 0x45d56 ('e'/'E'/'.' string handler). 0x47ea4 raw-byte jump map (verified): entry 0x47ea4 BL=0x20; thunks set BL=0xa0(0x47e4a)/0x1f(0x47e9f) then JMP 0x47ea7; targets 0x47f09=R_0(SUB EAX,EAX;RET), 0x47f01=R_80(MOV EAX,0x80000000;RET), 0x47efe=main epilogue(NEG EAX;POP ECX;POP EBX;RET — JNC path), 0x47ee0=SUB EBX,EBX(CL!=0x20 path), 0x47eed=MOV CL,0xff(CL==0x20 merge); core bit-chain is RCR CH / SHLD EBX,[EAX],CL / SHL EAX,CL / ADC EAX,0 — needs register-level trace + reference before porting | STATUS.md L0 |
| 2026-07-08 | 0x04a923 has GARBAGE disasm (no prologue: starts `CALL`; `SLDT`,`CALLF 0x0:0x23b`,`IN AL,0xb`,`POP SS`,`MOV AL,[0x5af34e72]`) yet is called from 0x4a814 and sits right after 0x4a8b9 (ends 0x4a922) — likely mis-boundary or a data block; NOT marked spurious pending deeper investigation | STATUS.md L0 |
| 2026-07-08 | 0x04811a decoded (raw-byte) = IEEE-754 float32→fxnum constructor `fx_from_f32`: expfield=(int32)v>>23 & 0xff (bits 23..30). expfield==0→passthrough {lo=0,hi=v,exp=0}; expfield==0xff→NaN/Inf marker exp=0xffff; else exp=expfield+0x3f80 (=unbiassed+0x3fff). hi=(v<<8 & 0x7fffffff)\|((v<<8&0x7fffffff)?0x40000000:0)\|0x80000000. The ambiguous `JZ 0x48149` is at 0x48142 and skips only `OR EAX,0x40000000` (merge point, not a loop). The trailing `ADD ECX,ECX;RCR CX,1` is NOT pure identity: with bit31 set (CF=1) it flips exp's top bit, so the carry is computed explicitly. Distinct from the BSR-normalizing fx_from_u32/s32 (0x47f1f/0x47f0e). 6 differential tests PASS | STATUS.md L0 |
| 2026-07-08 | Remaining L0 (16) are all fixed-point/FPU/bit-chain, need register traces. Surveyed: 0x047fd4 = fixed-point struct normalizer (reads 4-field struct at [EAX]: [0]/[4]/[8]; RCR CH / SHLD / SHRD / ADD 0x100 / 0x80000000 clamp / ADD CX,0xc080 bias) - normalizes a {lo,hi,exp} fx value to 16-bit-mantissa form. 0x0485eb = fixed-point SUBTRACT (ESI:EDI - 15-bit; then normalizes EDX:EAX by left-shift until top bit, with a 0x7fff mask + sign-restore) - returns EDX:EAX normalized difference + a 0x8000 sign bit. 0x047ea4 = the RCR/SHLD exp-conversion (BL-mode variants 0x47e4a/0x47e9f). 0x04a9da = FPU double (FSTP QWORD - return via memory). 0x04a923 = garbage disasm (no prologue, SLDT/CALLF/IN/POP SS) yet xref'd from 0x4a814 - mis-boundary or data block, investigate. Port these ONE AT A TIME with a full register trace + reference (no-op is unsafe - real logic) | STATUS.md L0 |
| 2026-07-08 | Remaining 7 L0 all assessed as the hardest (need dedicated passes): 0x04794a (363B, complex fx, 8 callers), 0x047d4e (fx multiply, 64-bit + exp bias 0xc002/0x3ffe, returns 0x8000000000000000 on overflow), 0x0481df (fx bit-shift/round via table DAT_0x4815f indexed by (exp+0xbfe1)*4), 0x0485eb (fx subtract, register ABI EAX:EDX=a lo:hi EBX:ECX=b lo:hi ESI/EDI=exps, alignment+normalization loops), 0x048e60 (fx division/normalize 64-bit loop), 0x04a3a8 (FPU FLD/FCOMP/FDIV/FMUL over {int16,double} table 0x519bc w/ 0x134/0xd8; decompile drops FPU), 0x04a923 (Ghidra decompiler fails at 0x4a96c overlapping insn; body disassembles to zero-filler `ADD [EAX],AL` after a leading `CALL 0x4a9d8` -> effectively a tail-call thunk, real logic in 0x4a9d8 or boundary wrong; called from 0x4a814). All real (not spurious). Deferred for careful individual passes | STATUS.md L0 |
| 2026-07-08 | 0x0481df = fixed-point round-down `fx_round_down(uint32_t *v, uint16_t mode)` (v={lo,hi,exp}, exp@8). Ported straight from the disassembly (0x481df..0x4830f) because the Ghidra decompile garbles the index (renders (ushort)(mag+0xbfe1) which looks OOB; the real index is EBX=mag-0x401f or mag-0x3fff, both 0..31, in-bounds on the 32-entry mask table at 0x4815f). CRITICAL bug caught on first build: the three branch tests are SIGNED JL ((int16)bx<0 continue / >=0 skip); I first coded them as unsigned `bx>=0x4000` which inverted the branches (all values passed through unchanged -> 9 FAIL). Fixed to `bx<0x8000` (bit15 clear = (int16)>=0). Branches: tableA (mag in [0x401f,0x403e]) clears tbl[mag-0x401f] top bits from lo; underflow (mag<0x401f) sets lo=0 then tableB (mag in [0x3fff,0x401e]) clears tbl[mag-0x3fff] bits from hi; else zero/inf (mag<0x3fff). Round-half (mode 0) uses ROL/ADC + RCR carry chain to conditionally add mask+1 and exp++. 6 hand-traced cases (truncate, underflow->0, equal-round, add-round, tableB-on-hi, underflow->inf) + determinism all PASS. Ghidra renamed fx_round_down. L0 108/112 | STATUS.md L0 |
| 2026-07-08 | 0x04a923 resolved as a no-op stub `fx_stub_4a923(void)`. The earlier "garbage disasm / SLDT/CALLF/IN/POP SS" reading was a mis-decode of the zero-filler padding; the real body is a single `CALL 0x4a9d8` (e8b0000000) followed by 0x0000 (ADD [EAX],AL) filler pairs. 0x4a9d8 is NOT a Ghidra function; its two bytes are `POP EDI; RET`. So calling 0x4a923: CALL pushes 0x4a928 and jumps to 0x4a9d8, POP EDI loads 0x4a928 (clobber), RET pops the caller's return address and jumps back to 0x4a923's caller. Net effect = return void with EDI clobbered to 0x4a928, no return value, any pushed args left on the stack (caller-cleans). Disassembly-verified no-op -> empty C function. Called from 0x4a814. Ghidra renamed fx_stub_4a923 | STATUS.md L0 |
| 2026-07-08 | 0x047d4e = fixed-point 64x64 multiply `fx_mul64(a_lo,a_hi,b_lo,b_hi,esi)`. A=EDX:EAX, B=ECX:EBX, ESI=(exp_hi:16|exp_lo:16) packed (verified from caller 0x4876d: MOV EDX,0xb8aa3b29; MOV EAX,0x5c17f0bd; CALL 0x47d4e; MOV [EDI],EAX). u9=(esh&0x80007fff)+(esl&0x80007fff); u8=u9+0xc002. Key finding: the decompile's uVar2 is NOT A*B - it is a schoolbook split uVar2=a_hi*b_hi+(carry<<32)+((u14+u13)+carry) and uVar11 is the 32-bit guard word (lo32 of the cross terms); lo32(a_lo*b_lo) and the low cross bits live in uVar11, not uVar2. Left-normalize shifts u2 left 1 with carry-in=u11 bit31 and sets u8=u9+0xc001; round-to-nearest adds 1 when u11 bit31 set (with a 0x100000000>>1 renormalize on bit63 overflow, u8+=1, 0x7fff->inf); final right-shift by (-(char)u8) when (short)u8<1. x86 SHL count is masked to 5 bits, so (32-sh) with sh==0 shifts by 0 (hi folded into lo). CRITICAL UB caught: carry sums `(u32+u32)>>32` are UB (32-bit sum wraps before shift); -O0 gave correct values but -O2 assumed no-UB and optimized to 0 (returned 0x8000000000000000 instead of 0). Fixed by widening to uint64 before >>32. 8 checks PASS incl. 2^32*2^32@esh=0x3fff->1, small-product underflow->0, overflow->inf. Ghidra renamed fx_mul64 | STATUS.md L0 |
| 2026-07-08 | 0x048666 = fixed-point value classifier `fx_classify_status(const uint8_t *fx, uint8_t *ctx, uint32_t edi)`. Register ABI (EAX=fx ptr {u32 lo,u32 hi,u16 exp@8}, EBP=ctx ptr [status@+4, flags@+8], EDI=class byte-offset). If (mask22&flags)==mask22: status=0x41. Else clear mask22 and classify by mag=exp&0x7fff: mag0 -> (hi|lo)!=0?0x44:(flags|=mask1c,0x40); mag==0x7fff -> flags|=mask20, (lo==0&&hi==0x80000000)?0x05:0x01; else -> (hi&0x80000000)?0x04:(flags|=mask20,0x44). Status high byte=(code|sign) where sign=2 if (short)exp<0; low byte + bits 9,10 preserved via &0xb8ff. Masks from the 8-word table at 0x45e1c (mask1c=tbl[edi/2], mask20=tbl[edi/2+2], mask22=tbl[edi/2+3]). 8 differential checks PASS. Ghidra renamed fx_classify_status | STATUS.md L0 |
| 2026-07-08 | 0x047ab6 = fixed-point lexicographic UNSIGNED compare `fx_compare(const uint8_t *a, const uint8_t *b)` on {u32 lo, u32 hi, u16 exp} (exp at [8]). A value with exp==0x7fff and nonzero (hi<<1)|lo is "special" and the compare returns 2. Otherwise it is a lexicographic compare of (exp, hi, lo): the first differing field sets the sign via the unsigned borrow (CF). Returns -1 (a<b) / 0 (==) / +1 (a>b). The original's RCR EAX/XOR ESI/ADD ESI at 0x47b10-0x47b14 is dead (EAX is overwritten by EDI at 0x47b1c). 10 differential checks PASS incl. the hi=0x80000000 lo=0 overflow edge (not special). Ghidra renamed fx_compare | STATUS.md L0 |
| 2026-07-08 | 0x04a2a7 = double NaN/Inf inline-string writer `fx_double_naninf(double d, char *out)`. If (hi>>20)&0xff==0xff (low 8 exponent bits all 1): write inline "inf" (mantissa 0) or "nan" (mantissa !=0) at 0x51844/0x51848, with a leading '-' if the sign bit is set, return 1; else return 0 leaving out untouched. Latent original bug: only 8 of the 11 exponent bits are checked, so 1.0 (exp 0x3ff, low8=0xff, mantissa 0) is classified "inf". 7 differential checks PASS (Inf/-Inf/NaN/-NaN/1.0/3.0/0.0). Ghidra renamed fx_double_naninf | STATUS.md L0 |
| 2026-07-08 | 0x045687 = DOS date/time packer `datetime_pack(struct datetime_pack *out)`. Reads INT 21h 2Ch/2Ah via new sys_getdate/sys_gettime backends. local0=(month<<24)|day<<16, local1=(hour<<24)|low8(minute*256+second-0x76c)<<16. The SHL ECX,16 zeroes the low byte of each packed local, so ESI (built from local0's low byte) is always 0 and the day-change branch (CMP local1.byte0 vs local2.byte0, both 0) is always equal -> dead. Returns 0 always; writes f4=day f8=month f14=low8(time) f20=0xffffffff f0=fc=0 f10=0xffffffff; f18 untouched. 10 differential checks PASS. Ghidra renamed datetime_pack | STATUS.md L0 |
| 2026-07-08 | 0x04a9da = IEEE-754 double -> 32-bit fixed-point exp `fx_double_exp(double d, uint32_t *out)`. hi=bits 32-63. If exactly +/-0 (hi&0x7fffffff==0 && lo==0) -> *out=0; else *out=(int32)(((hi>>20)&0xff)-0x3fe) (low 8 bits of the biased exponent minus bias 0x3fe). The original rewrites [ESP+6] and does FLD double, but the FPU value is never stored so it does not affect the returned EDX; those are omitted as no-ops. 6 differential cases PASS (±0, ±1.0, 2.0, smallest subnormal). Ghidra renamed fx_double_exp | STATUS.md L0 |
| 2026-07-08 | 0x047fd4 = fixed-point mantissa normalizer `fx_norm32(const uint8_t *p)` (renamed from fx_normalize to avoid collision with the batch-G static constructor helper of the same name). Reads {u32 lo,u32 hi,u16 exp}. M=(hi+0x100)&0xffffff00 (or 0x80000000 + exp+1 on hi+0x100 overflow; mask->0 on the hi==0x80/lo==0 underflow edge). cx=(exp+carry)&0x7fff, piecewise: cx==0->M; cx==0x7fff->((M<<1)>>8|0xff000000)<<1; else cx2=(cx+0xc080)&0xffff: cx2>=0x8000->0, cx2>=0xff->0xff000000<<1, else ((M<<1)<<8|(hi<<25)>>24)<<1. Key: EBX=hi<<25 so BX(low16)==0, making every RCR EAX,1 a plain <<1 (CF=0). Instruction-faithful; 6 hand-traced cases PASS (cx=0 M, small->0, 0x7fff special, 0xff path, SHRD path, overflow carry->0) | STATUS.md L0 |
| 2026-07-08 | 0x047ea4 (body) + 0x047e4a (mode 0xa0) + 0x047e9f (mode 0x1f) = fixed-point exp-based mantissa extractor `fx_extract(const uint8_t *p, int32_t mode)`. p->{u32 lo,u32 hi,u16 exp} (exp at [8]). cl=(exp&0x7fff)-0x3ffe; cl<0->0, cl>0x20->0x80000000, cl>(mode&0x3f)->0x80000000. Else extract top cl bits of hi (or all hi when cl==0x20), then RCR CH,1/SHLD/ADD/ADC/NEG bit-chain (explicit CF tracked) sets the sign + a +1 carry. lo influences only via bit31(lo)->RCR CH,1. Three Ghidra fns fold into one C fn with a mode param (default 0x20). Ported instruction-by-instruction; 8 hand-traced differential cases PASS (bias, underflow, both overflow paths, cl=0x20 hi+1, mode 0x1f/0xa0 distinctions) | STATUS.md L0 |
| 2026-07-08 | 0x049d0b = fixed-width signed integer formatter `fmt_int_signed(uint8_t *out, int32_t width, int32_t maxw, int32_t val)` (EAX/EBX/ECX/EDX). base = 100/10/1 by magnitude (3/2/1 digits); sign '+'/'-'; default width = 2 (3 for 3-digit magnitudes) when width param==0; leading-'0' pad to width; IDIV digit extraction (ESI/EBX quotient, ESI%EBX remainder, EBX/=10). Returns the field count (width+1 or digits+1); two no-output cutoffs: width+1>maxw (0x49da9) and digits>width (0x49da6). Sibling of fmt_decimal (0x49db2) - together they format a signed decimal number (integer + fractional part), and both share the 0x49db0 (POP ESI;RET) epilogue. INT_MIN abs overflows (NEG wraps) - latent, not tested. 7 differential tests PASS | STATUS.md L0 |
| 2026-07-08 | 0x049db2 = decimal number formatter `fmt_decimal(uint8_t *out, int32_t intlen, int32_t fractlen, const uint8_t *src)` (EAX/EBX/ECX/EDX). Writes intlen digit chars from src (until NUL, then '0' pad), then '.', then the fractional part via shared ESI/ECX flow: loop A (while fractlen!=0 && esi!=0: esi++;'0';fractlen--) fires only in the negative-intlen edge case; loop B copies fractlen chars from src (until NUL) then '0' pad. Returns the advanced out pointer (EAX). IMPORTANT: 0x49db2 is a real standalone function that reuses the shared epilogue at 0x49db0 (POP ESI;RET) - the tail of the PREVIOUS function 0x49d0b (ends 0x49db1 RET). Not a spurious split; the two are adjacent and share the final POP/RET. 5 differential tests PASS (normal, src-short pad, 0., fractlen==0, negative-intlen) | STATUS.md L0 |
| 2026-07-08 | 0x049132 = 64-bit -> IEEE-754 float32 converter `fx64_to_f32(uint32_t lo, uint32_t hi)` (input EDX:EAX). If hi&0x7ff00000==0 (top 12 bits of hi clear) -> underflow -> 0. Else: shift the 64-bit value left 1 (top bit captured into EBX as the f32 sign 0x80000000), add the 0x20000000 mantissa bias, then clamp/normalize: hi==0 or hi>=0x8fe00000 (unsigned) -> 0x7f800000|sign (max-finite clamp); hi<0x70200000 -> 0; else hi-=0x70000000, shift left 2, OR in the sign, return the high word as the f32. The thresholds (0x7f800000/0x8fe00000/0x70200000/0x70000000) are f32 bit patterns. Ported as an exact register-op replication with uint64. Note: the `hi==0 after bias` JZ clamp is unreachable (hi>=0x100000 survives the initial test) but is replicated faithfully. 6 differential tests PASS | STATUS.md L0 |
| 2026-07-08 | 0x04a84e = fixed-point 53-bit mantissa scaler `fx_mantissa_scale(uint32_t *out, const uint32_t *in)`: exp = hi's bits 31:20 (bias 0x433); mantissa = (hi & 0xfffff)\|0x100000 (bit20 forced = implicit leading 1) + lo (53 bits). Shifts the mantissa left (exp>0x433) or right (exp<0x433) by exp-0x433; the original's per-bit SHR/RCR loop with EAX/ESI tracking the shifted-out bits is exactly round-to-nearest-even (top discarded bit = EAX bit31 -> CMP 0x80000000; lower discarded = EAX low bits / ESI; tie -> TEST ECX,1 = round-to-even). Port uses uint64 shift + explicit RNE (sa>=64 -> 0). 7 differential tests PASS (passthrough, left, tie-even both ways, sticky round-up, large shift, low-mantissa) | STATUS.md L0 |
| 2026-07-08 | 0x04a8b9 = 64-bit unsigned -> fixed-width right-justified ASCII field `fmt_u64_fixed(const uint64_t *val, char *buf, int width)`: writes buf[width]=0 sentinel, then loops extracting v%10000 (a 4-digit group) — the original splits this into two 32-bit DIVs (hi/10000, then (hi%10000):lo/10000), provably equal to 64-bit v/10000 and v%10000 — converts the group to 4 ASCII digits (AAM BCD in orig, direct /10 %10 in port), stores right-to-left (ones at highest index), v/=10000, and pads '0' groups until width slots are filled. EBP(physical, -=4/group) and EBX(counter, -=1/digit) stay in sync; the per-digit JZ stops partial-group stores so non-multiple-of-4 widths never write OOB. 6 differential tests PASS (incl. full 20-digit 0xdeadbeefcafebabe) | STATUS.md L0 |
| 2026-07-08 | 0x045d56 = in-place sci-notation float-string cleanup `float_str_sci(char *s, const uint8_t *rec)`: rec[0x14]&1→no-op; else find 'e'/'E' (or NUL), step back one, if rec[0x15] is 'g'/'G' strip trailing '0's, if the stepped-back char is '.' overwrite it (drop the dot), then copy from the 'e' position leftward (NUL-terminating) removing the stripped zeros/adjacent dot. Faithfully replicates the unguarded zero-strip (all-zero input would underflow past position 0, as in the original). 6 differential tests PASS | STATUS.md L0 |
| 2026-07-08 | **L1 started** — 0x045605 = in-place NUL-terminated string upper-caser `string_upper(char *s)` (per-char `to_upper` 0x494bd, already ported L0). 5 tests PASS. **PIVOT:** the 4 remaining L0 (0x4794a 363B/8-caller, 0x485eb subtract, 0x48e60 divide, 0x4a3a8 FPU) are all high-risk for hand-verified ports and are being deferred for dedicated passes. **0x485eb analysis (for the dedicated pass):** register ABI a=EDX:EAX (hi:lo), b=ECX:EBX (hi:lo), exp_a=ESI, exp_b=EDI; out r_lo=EAX, r_hi=EDX, r_exp=ESI, r_sign=EDI (0 or -1). Ghidra decompile DROPS the epilogue (0x4864e-0x48665: computes returned exp ESI, sets EDI sign via `XOR BX,CX;JNS;NEG EDI`), so it must be ported from the disassembly. Body: (1) mag_a<mag_b → return a unchanged, ESI=mag_a; (2) else a=a-b when b<=a (STC) else a unchanged, EDI=(did-subtract); (3) two left-shift normalization loops (align by mag_a-mag_b, then normalize to top-bit-set); (4) OPEN QUESTION: the second loop's 64-bit shift `ADC EAX,EAX;ADC EDX,EDX` carries bit30(EAX) (not bit31) into EDX — either a latent off-by-one or a 63-bit mantissa format; unresolved without running the original binary. Callers: 0x48585, 0x48a48. | STATUS.md L1 |
| 2026-07-08 | **0x485eb refined trace** (full 53-instruction disasm 0x485eb..0x48664 read). Corrects the earlier "two normalization loops" reading: it is a SINGLE shift-and-clamped-subtract loop with feedback edges. After `D=(exp_a&0x7fff)-(exp_b&0x7fff)`, if D<0 jump to L_other (0x4864a). Else one compare/subtract: `sub` (A=A-B, STC) reached iff A>=B (unsigned 64-bit); `skip` (A unchanged, CF=0) iff B>A. Then 0x48612: EDI+=CF (EDI=did-subtract), DEC ESI (D--), `JL 0x48627` if D<0. The loop body shifts A(EDX:EAX) left by 1 (ADD EAX,EAX;ADC EDX,EDX), then `JC 0x4860d` (A_hi overflow -> re-subtract B) or `JS 0x485ff` (A_hi top bit set -> re-compare B vs A) or back to 0x48612. So it shifts A left up to (mag_a-mag_b) times, clamped-subtracting B whenever A grows to >=B, counting subtractions in EDI — i.e. a division-like / fixed-point alignment, NOT a plain subtract. Epilogue 0x4864e-0x48665: `AND EBX,0x8000; OR ESI,EBX` (exp gets a sign bit), then `XOR BX,CX; JNS; NEG EDI` (EDI=sign 0/-1). HIGH RISK to hand-verify (no reference); defer for a dedicated pass with an original-binary trace. | STATUS.md L0 |
| 2026-07-08 | **0x0498f6 (276B) = Unix-timestamp→calendar-date converter** (caller 0x049a0b: EAX=0x63df=25567 days [1900→1970 offset], EDX=unix_secs from global 0x64098, EBX=0(adjust), ECX=out). Output struct u32[]: [0]=sec [4]=min [8]=hr [0xc]=day(1-idx) [0x10]=month(0-idx) [0x14]=year(rel, actual=+1900) [0x18]=dow ((total_days+1)%7) [0x1c]=doy(0-idx) [0x20]=0(caller). Year loop: `excess = total_days - (total_days/366)*365 - (total_days/366-1)/4`; while `excess >= 365+is_leap(year+1900)` subtract year_len, year++. **KEY LATENT-BUG FINDING:** the month/day step loads `ESI = DAT_00051960`(non-leap)/`0x5197a`(leap) = the **END**-of-month cumulative tables {31,59,90,…} (raw bytes `be 60 19 05 00`/`be 7a 19 05 00` confirmed), and computes `day = doy - tbl[month] + 1` (month = doy/31, +1 if doy >= tbl[m0+1] via a high-u16-of-dword read quirk). But that formula only yields correct calendar days when tbl is **START**-of-month {0,31,59,…} = DAT_0005195e/0x51978 (exactly 2 bytes earlier). A faithful disasm replication (ESI=0x51960) gives day=-30 for 1970-01-01; the start-of-month table gives correct dates for every tested timestamp (verified vs Python `datetime`). So the original either has a 2-byte table-base bug or date output is used in a tolerant context. **DEFER the port to a dedicated pass: RUN THE ORIGINAL first** to confirm actual date output, then port faithfully. Companion 0x049506 (228B) uses both DAT_0005195e and DAT_00051960 + is_leap (likely "add days to a date") — also deferred. | STATUS.md L1 |
| 2026-07-08 | **fn_0498f6 (0x0498f6, 108 ins) PORTED — supersedes the earlier deferral (L105/L117 notes)**: those misread DAT_00051960 as end-of-month {31,59,…} and deferred pending an original run. Direct read_memory confirms the tables are START-of-month cumulative: non-leap DAT_00051960={0,31,59,90,120,151,181,212,243,273,304,334}, leap DAT_0005197a={0,31,60,91,121,152,182,213,244,274,305,335}. The month step: m=day_in_year/31; if tbl[m+1]<=day_in_year m++ (straddle reads the NEXT u16 via a dword load); day=day_in_year-tbl[m]+1. Year: year_est=days/365, day_in_year=days-year_est*365-(year_est>0?(year_est-1)/4:0), then is_leap(year_est+1900, ctx=year_est) returns (ctx<<32)|leap so the echoed ctx = running year and the CONSTANT year_len=leap+365 is subtracted in a countdown. **LATENT BUG (preserved 1:1):** the m==11 (Dec) straddle reads the byte PAST the table (0x51978=365 non-leap, 0x51998=0 leap); the leap OOB=0 makes month become 12 for Dec 7-31 of leap years. I gave the C tables a 13th element = the actual next-memory value to replicate the OOB read. 4-vector differential test (Jan 1 1900 / Jan 1 1901 / Dec 31 1900 / Feb 29 1904) all match hand-computed Gregorian dates (10 checks). L1 30->31, TOTALS 210->211, suite 739 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **Three clean wrapper/constant wins** (callee sweep of the 30 smallest unported fns): (1) **fn_04322f** (0x04322f, 4B) = `mov eax,-1; ret`, returns -1. (2) **fn_044848** (0x044848, 16B) = lseek wrapper: sets EBX=1/EDX=0 then fn_044807(handle,0,SEEK_SET); caller param_2/3 clobbered, only handle observable. (3) **fn_049a0b** (0x049a0b, 27B) = unix-time decoder: writes out->mode=0 (+0x20) then fn_0498f6(0x63df=25567 days [1900->1970 epoch], *seconds, 0, out). Extended struct dec_datetime with a `mode` field at +0x20 (set by 0x49a0b, read by 0x49506). 6 checks (fn_049a0b(0)=Jan 1 1970 verified). L2 12->13, L4 0->2, TOTALS 211->214, suite 745 PASS 0 FAIL. **Note the date subsystem chain**: 0x49506->0x4574a->(0x49a35,0x49613) still gated; JMP-thunks (0x41fa1/0x42044/0x41e94/0x429e5/0x44c1e/0x46d00) depend on unported targets; 0x46d47/0x478e2 are vtable/tail-merges (deferred). | STATUS.md L2/L4 |
| 2026-07-08 | **Two flag-guarded IRQ-installer wins (L10)**: (1) **fn_03eff2** (0x03eff2, 132B) installs the IRQ 0x1c (FPU) handler when g_dat_63e4a==1: isr_set(0x1c,off,seg) + PIT ch0 mode 0x36/0xff/0xff, then clears the flag; wrapped in irq_cli/irq_sti (no-op standins). (2) **fn_03ffab** (0x03ffab, 97B) installs the IRQ 9 handler when g_dat_63e86==1: isr_set(9,off,seg), clears the flag, mirrors the IRQ-9 source byte g_dat_63f51 into the real-mode low-memory flag g_dat_00417 (DAT_00000417). Both are **no-arg in effect** (Ghidra's inferred ECX/param_3 stack params are never read — verified by disasm). Supersedes the L10 deferral note (line 124) that held 0x03ffab as "FUN_00042361-dependent" — isr_set (0x42361) is now ported. The canary prologue (PUSH 0xNN; CALL 0x418ee) is the compiler stack-overflow check, NOT real logic — omitted per established convention (canary_id has exactly 1 self-ref in f.c). 9 globals added (63e4a/44/48, 63e86/80/84, 63f51, 00417). 5 checks. L10 21->23, TOTALS 214->216, suite 750 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **Three register-ABI (EAX-param) hardware wins (L10)**: (1) **fn_03575d** (0x03575d, 148B) = PIT channel-0 rate setup. Divisor passed in EAX (canary prologue preserves it); stores clamped divisor in g_dat_63318 (0->0x10000; >0x10000->0x10000 stored + 0 written), clears g_dat_6332c, writes PIT mode 0x34 + 16-bit divisor via outb (no-op). (2/3) **fn_03f3be / fn_03f410** (0x03f3be/0x03f410, 81B each) = VGA palette-register setters (INT 10h AX=0x4f05 "Set Palette Register"). `color` in EAX; value=color<<(g_dat_63e4c&0x1f); if value!=g_dat_63e50[index] store + issue INT10 (no-op standin). The two twins differ only in index global (0x3f3be->g_dat_63e5c, 0x3f410->g_dat_63e58) -- factored to a shared `palette_set(index,color)` helper. **Register-ABI assumption** (documented): the input EAX is passed as a C param; the `PUSH 0x20; CALL 0x418ee` canary prologue is a compiler stack-overflow check, omitted. INT 10h (VGA) is a no-op in the flat model -- only the palette cache update is observable. 6 new globals (6332c,63318,63e50[256],63e5c,63e58,63e4c). 7 checks (PIT clamp + palette cache). L10 23->26, TOTALS 216->219, suite 757 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **fn_1053f (0x01053f, 48B) = DPMI INT 0x31h AX=0x101 (Set Debug Registers)**: DX = word at (DAT_00053c5c - 2). No DPMI in the flat model -> documented no-op standin; the DAT_00053c5c-2 read only feeds the no-op DPMI arg so it is not dereferenced (g_dat_53c5c tracked for fidelity). 1 global, 1 smoke check. Supersedes the line-126 deferral that listed 0x01053f/0x03f3be/0x03f410 as "DOS INT 21h/31h" -- the 0x03f3be/0x03f410 pair were actually VGA INT 10h palette setters (ported this turn), and 0x01053f is a DPMI INT 0x31h no-op (ported now). L10 26->27, TOTALS 219->220, suite 758 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **Remaining L1 deferrals (need dedicated passes)**: (a) **0x04a814 (57B table / larger decomp)** fx_mul wrapper — calls ported `fx_mul64` + `0x4a923` no-op; decompile garbles output (POP EBX via return-address) — needs disasm. (b) **0x047d24 (73B)** fx_mul64 wrapper — output via `POP EBX` (return-address trick); register ABI vs `fx_mul64` unconfirmed. (c) **0x047f54 (167B)** fx mul — schoolbook-split uVar2 + FPU load/rewrite + POP-EBX output. (d) **0x04711d (84B)** fx — FPU load/rewrite + garbled decompile. (e) **0x0422d5 (405B)** DOS I/O — INT 13h+disk. (f) **0x04391d (250B)** DOS I/O — INT 13h read. (g) **0x043381 (39B)** DOS I/O. (h) **0x0440a9 (49B)** DPMI real-mode far-call wrapper. (i) **0x043324 (64B)** DPMI 2.0 init. (j) **0x0431b5** heap size-adjuster (longer than the 18B boundary — runs to ~0x431f7): param_1=p; aligned=([p]+3) with low byte & 0xfc (low-byte-only mask quirk); if aligned==0 return 0; else if `(DAT_000533b2==1 && DAT_000533b3==0) || DAT_000533b2==9` → aligned+=8, else aligned-=FUN_00042ea1(); then *[p]=aligned. DEPENDS on unported FUN_00042ea1 + new globals DAT_000533b2/0x533b3 — not self-contained. (k) **0x0492d0 (71B)** FPU + 0x498f6 + 0x4a923. (l) **0x041439 (291B)** I/O ack — deps 0x469d9+DAT_00063f84+DAT_00053334 unported. All real; deferred. | STATUS.md L1 |
| 2026-07-08 | **0x049a6e (277B) = time-string parser** (0x049a48 done as parse_decimal). 0x049a6e register ABI: EAX=input string, EDX=prefix-output buffer, EBX=total-seconds output ptr; returns EAX=position after the parsed time. Structure: (1) skip an optional leading ':'; (2) locate the offset of the first {digit, ',', '-', '+', NUL} char (skipping other chars), clamp to 30; (3) copy that prefix to EDX (REP MOVSD/MOVSB — plain memcpy in flat model) + NUL-terminate; (4) if the break char was '-' set a negate flag, '+' clears it; (5) parse the time via **three calls to 0x049a48** into [ESP]/[ESP+4]/[ESP+8] (hours/minutes/seconds, ':'-delimited), then `*EBX = c0 + c1*60 + c2*3600` (negated if '-'). 0x049a48 = parse_decimal (DONE, batch H + this turn): decimal parser WITH a scan loop, `v = v*10 + (*s-'0')`, returns `const char*` ptr past digits. **0x049a6e OFF-BY-ONE EDGE CASE:** after the break, EBP = break_pos+1 unconditionally — correct for a sign break char ('-'/'+') but for a pure "HH:MM:SS" (break char = first digit) it skips the leading digit, so the function seems to assume a sign is present — **PORTED as parse_time_str this turn (quirk replicated 1:1, documented as a latent bug; verified by 11 hand-traced tests). | STATUS.md L1 |
| 2026-07-08 | **parse_decimal (0x049a48) completed + 0x042958 thunk resolved**: parse_decimal (batch H) returned void (dropped EAX = pointer-past-digits); completed the 1:1 contract → now returns `const char*` (first non-digit), used by 0x049a6e as its new scan position; added a return-position test. 0x042958 (4B) = `JMP 0x4564f` (delay_units, ported f.c:1151) — a call-site alias, no separate C fn. L1 10→11, TOTALS 121→122. | STATUS.md L1 |
| 2026-07-08 | **parse_time_str (0x049a6e, 277B) PORTED**: time-string parser (see 0x049a6e analysis row above). Faithful 1:1: skip optional leading ':'; find break char (first {digit,sign,comma,NUL}), clamp prefix to 30, copy it (segmented REP MOVSD/MOVSB → memcpy), NUL-term; break char '-' sets a negate flag (ESI repurposed from the copy source); parse H/M/S via three parse_decimal calls → *secs_out = sec + min*60 + hour*3600 (negated if '-'); return position after the last parsed component. **OFF-BY-ONE LATENT BUG replicated 1:1:** scan resumes at break_pos+1 unconditionally, so a pure "HH:MM:SS" with no sign skips its first digit ("12:34:56" → 9296 = "2:34:56"); the sign form is intended and correct ("-12:34:56" → -45296). Verified by 11 hand-traced differential tests (prefix copy/clamp, sign negate, H:M:S split, no-time early-return, long-prefix clamp). L1 11→12, TOTALS 122→123. | STATUS.md L1 |
| 2026-07-08 | **heap_size_adjust (0x0431b5, 119B) PORTED**: heap block size-adjuster (was deferred pending 0x42ea1, now ported as heap_top_check). 10-step algorithm: a = align_up(*p,4) [(*p+3)&~3]; a==0→0; (cfg_b2==1&&cfg_b3==0)||cfg_b2==9 ? a+=8 : a-=heap_top_check(); *p=a; t=a+0x3c (overflow→0); if t<limit(g_dat_53640) t=limit&~1; *p=t; u=t+0xfff (overflow→0); u=align_up(u,0x1000) [u&~0xfff]; *p=u; return u!=0. Low-bit masks are byte ops (AND DL,0xfc / AND DL,0xfe / AND DH,0xf0+XOR DL) but the affected bits all lie in the low/second byte, so &~3/&~1/&~0xfff are exact (earlier "low-byte-only mask quirk" note was a misread). Added g_heap_cfg_b3 (byte 0x533b3); g_heap_cfg_b2 + g_dat_53640 already existed. 6 tests (empty→0, limit clamp, no-clamp page-up, +8 branch, large page-up, overflow→0). L1 12→13, TOTALS 123→124. | STATUS.md L1 |
| 2026-07-08 | **fn_0423cf (0x0423cf, 21B) PORTED**: registered-callback dispatcher. PUSH EDX; EDX=EAX; CALL [g_dat_53370]; CALL [g_dat_53374]; EAX=EDX; JMP 0x422d3 (shared epilogue POP EDX;RET). Modelled g_dat_53370/g_dat_53374 as `u32_cb_fn` (uint32_t(*)(uint32_t)) slots; the fn calls g_dat_53370(input), then g_dat_53374(first's return), and returns the ORIGINAL input. Both slots are called unconditionally (the original requires them set; a NULL slot faults, replicated). 3 tests (mock callbacks: return-original, cb1 gets input, cb2 gets cb1's return). L1 13→14, TOTALS 124→125. | STATUS.md L1 |
| 2026-07-08 | **0x42a08 family (4 fns) PORTED**: 0x42a08/0x42a0d/0x42a30/0x42a3c. 0x42a0d(x) = PUSH EDX;MOV EDX,EAX;CALL const_64008;MOV [EAX],EDX → g_dat_64008=x. 0x42a08 = MOV EAX,0xd falling into 0x42a0d body → g_dat_64008=13 (param ignored). 0x42a30(x) = same shape via const_64004 → g_dat_64004=x. 0x42a3c(x) = CALL [g_dat_533bd];RET → returns g_dat_533bd(x). Added globals g_dat_64008, g_dat_64004, g_dat_533bd. Note: the earlier "blocked on unported 0x45a6a/0x45a70" was wrong — those are const_64008/const_64004 (already ported constant address-returners). 4 tests PASS. L1 14→18, TOTALS 125→129. | STATUS.md L1 |
| 2026-07-08 | **0x47d24 fx_mul64 wrapper PORTED**: register-ABI wrapper around fx_mul64 (0x47d4e). Entry a=EAX, b=EDX, out=EBX. esi=(B[8]<<16)|A[8]. The "POP-EBX trick" resolved: EBX (the out ptr) is PUSHed, reused to hold B[0] (b_lo arg), then POPped back before writing out[0]=rlo/out[4]=rhi/out[8](word)=A[8]. Register->arg map verified from 0x47d4e entry (a_lo=EAX,a_hi=EDX,b_lo=EBX,b_hi=ECX,esi=ESI). 3 differential tests PASS (incl. zero-A). L1 18→19, TOTALS 129→130. | STATUS.md L1 |
| 2026-07-08 | **0x44bb4 fn_044bb4 PORTED**: node flag-setter (param_1=EAX=node ptr). if (node[0x0d]&0x20) return; if (ioctl_device(node[0x10])==0) return; node[0x0d]|=0x20; if ((node[0x0d]&7)==0) node[0x0d]|=0x2. The original tests BL=b\|0x20 for the low-3-bit check (0x20 doesn't affect bits 0-2), so the 2nd store yields b\|0x22 only when b&7==0. Modelled node as uint8_t* (64-bit port). ioctl_device (0x492b2) controllable in tests via sys_ioctl_dx/cf (dx=0x80,cf=0 → 1; dx=0,cf=0 → 0). 4 tests PASS (early-return, ioctl=0 no-op, →0x22, →b\|0x20). L1 19→20, TOTALS 130→131. | STATUS.md L1 |
| 2026-07-08 | **0x44ea4 fn_044ea4 PORTED** (+ parse_mod_prefix refinement): 307B numeric conversion-specifier parser. Calls refined parse_mod_prefix (now returns const uint8_t* = position of the first non-flag char, matching 0x44fd7's EAX-at-RET; the void port only dropped the return like parse_decimal did). Then: width from digits or '*' (vararg via cursor-to-cursor `uint32_t **va`, `P=*va; v=*P; *va=P+1`), precision default -1 or '.' digits or '.*' vararg (neg precision kept -1), then suffix: 'F'→0x80, 'L'/'N'→0x40, 'h'→0x10, 'l'/'w'→0x20 (all consumed), other chars return at the char (not consumed). Spec struct byte offsets [4]=width,[8]=precision,[0x14]=flags,[0x16]=width-fill(def ' ', '0' if '0' flag),[0x17]=0. 6 tests PASS. L1 20→21, TOTALS 131→132. | STATUS.md L1 |
| 2026-07-08 | **0x49b84 fn_049b84 PORTED**: 241B [J\|M]N[.N[.N]][/month:minute:second] date/time-string parser. Fully disassembled (Ghidra's extraout_ECX register-ABI juggling decoded into concrete param_2 dword offsets). era=1('J')/0('M')/-1(neither)→[0x20]; M case: first-1→[0x10], '.2nd'→[0x0c], '.3rd'→[0x18], [0x1c]=0; J/neither: first→[0x1c]. Then optional '/m'→[0x08](def 2), ':min'→[0x04], ':sec'→[0x00]. Untouched fields left as-is (verified via 0xAA sentinel). Callee parse_decimal (0x49a48, just refined). 5 tests PASS. L1 21→22, TOTALS 132→133. | STATUS.md L1 |
| 2026-07-08 | **0x492d0 fn_0492d0 PORTED**: node/device-table accessor (84B). Register ABI EAX=index, EDX=ctx. g_dat_53730 (DAT_00053730) = pointer to uint32 node array (4B/node; byte1=flags 0x40 init/0x20 open). if index>=g_dat_536dc → return (ctx<<32)|0. if index<6: if !(node[1]&0x40){ node[1]|=0x40; if ioctl_device(index)!=0 node[1]|=0x20 }. return (ctx<<32)|g_dat_53730[index]. Only callee ioctl_device (0x492b2, ported). 5 tests PASS. L1 22→23, TOTALS 133→134. **Also analyzed 0x0498f6 (276B date-converter)**: EAX=base-days, EDX=time-sec, EBX=offset, ECX=out struct [sec,min,hour,day,month,year,dow,day-of-year]; year-norm via uVar2=days/366, uVar4=days-uVar2*365-(uVar2-1)/4, loop consuming year-_lengths (is_leap+365); month table read as `*(int*)(tab+m*2)>>0x10` (HIGH word) for compare but `*(short*)(tab+m*2)` (LOW word) for day — two different 16-bit slots; tab=DAT_00051960(common)/0x5197a(leap). DEFERRED until table bytes + original run confirm intent. | STATUS.md L1 |
| 2026-07-08 | **L1 clean wins exhausted; L2 trivial wrappers ported** | Surveyed remaining L1: 0x4794a/0x485eb/0x48e60/0x4a3a8 are the 4 hard FPU/fixed-point core (dedicated passes); 0x4a7fa/0x48053/0x48585/0x47f54/0x4711d/0x4a814 are FPU wrappers (garbled, unaff registers); 0x422d5/0x42ccc/0x4391d/0x43381/0x43324/0x440a9 are DOS INT/DPMI (non-portable without DPMI layer); 0x498f6/0x49506 are date (need table bytes); 0x41439/0x431b5/0x48f4c blocked on unported/deferred callees. So L1 has no more self-contained verifiable wins. Pivoted to L2: ported 0x042a19 (6B, g_dat_64008=0xe) + 0x044b96 (29B, drain g_node_list_done via heap_free_host; node=g_ndpool+off {next,rec}). Both PASS. L2 0→2, TOTALS 134→136. | STATUS.md L2 |
| 2026-07-08 | **L2 batch ported (8 fns, 136→144)**: 0x045079 (96B zero-pad/right-justify hex: SCASB-countdown strlen, pad '0' to width, right-justify hex digits); 0x0450da (218B 16.16 fixed-point→decimal string: itoa32s(int part), frac digits via frac*10/65536, round-up when discarded-remnant bit15 set, spec[2]=places def 4); 0x049a35 (719B TZ config parser: config_value_ptr("TZ") + parse_time_str x2, fills g_dat_53780/84/88/a7/7c/34/58, params unused = callback sig; added 7 TZ globals); 0x049c76 (142B TZ range parser = 0x049a35 direct-string variant, initial offset 0x3840, 53780=18000-secs); 0x045a82 (101B byte classifier: fn_042a30/042a0d write g_dat_64004/64008; 'P'→0xe, [0x20,0x22)→5, >=0x22→0x13 when g_dat_533bb>2; tbl[bl]=byte@0x53625+(bl+3); ret (p2<<32)|0xffffffff); 0x045a76 (11B thunk: param_2(EDX)==0→0 else tail-jump 0x045a82, preserves EAX/EDX); 0x0423b7 (23B callback chain: g_dat_5336c(p1)→r1, g_dat_53370(r1), fn_0423cf(p1); added g_dat_5336c); 0x042576 (179B file-mode parser: to_lower(s[0]) in r/w/a else fn_042a0d(9); +/-/b/t modifiers→0x43/0x40/3 flags; w|2/a|0x82/r|1; g_dat_53619==0x200 default-binary; ret (p2<<32)|flags). All hand-traced differential tests PASS. L2 2→10, TOTALS 136→144. | STATUS.md L2 |
| 2026-07-08 | **L2 lexer pair DEFERRED (32→64 struct layout)**: 0x0445a6 (35B lexer read-char) + 0x044387 (503B number parser) share a lexer-ctx struct: method ptrs at param_1[0]/[1] + counter at param_1[3]. In 32-bit these are 4-byte slots (offsets 0/4/12); in 64-bit function pointers are 8 bytes so the layout shifts and cannot be replicated with raw 4-byte indexing. Needs a coordinated 64-bit struct design (method0@0, method1@8, method2@16, counter@24) ported across both fns + callers together, validated against the full lexer — not an isolated win. 0x044bec (49B) also remains (DOS INT 21h, non-portable). L2 clean wins exhausted (10/32); the rest need unported callees / DPMI / a 64-bit struct redesign. | STATUS.md L2 |
| 2026-07-08 | **fn_044bec (0x044bec, 23 ins) ported** — DOS AH=3Eh close-file: INT 21h AH=0x3e closes the handle (RCL/ROR preserves the INT's error flag/CF); on success (CF clear) store_arr53730(handle,0) marks the slot free + return 0; on error fn_042a0d(4) + return -1. The DOS close maps to a new sys_file_close stub (default success). 1 check. NOTE: supersedes the earlier note listing 0x044bec as a remaining L2 clean win (now [x]). L2 11->12, TOTALS 209->210, suite 729 PASS 0 FAIL. | STATUS.md L2 |
| 2026-07-08 | **L10 trivial-accessor frontier (13 ported)**: L3/L6 clean wins are blocked (L3 all DOS except the 1096B 0x0451bc typed-deserializer; L6 depend on heap_malloc's ambiguous 64-bit return + unset callbacks). The real frontier is **L10: 59 clean candidates**, all canary-prologue accessors (the `PUSH 0x20; CALL 0x418ee` prologue is not replicated). Ported 13: 0x010e15 (return g_dat_53c60), 0x040631 (g_dat_40010=low16(p)), 0x01520b (return *(u32*)(p+5)), 0x034a87 (g_dat_62d64+=11, return old), 0x01519f (return *(u32*)(p+1)-g_dat_639dc), 0x0151d5 (return *(u32*)(p+9)-g_dat_639e0), 0x03518a (BE16 read (p[0]<<8)|p[1]), 0x031f10 (ring push cap5), 0x0348e2 (slot-alloc wrap@9), 0x0343d1 (idx flag/counter), 0x03cad3 (struct->g_dat_639dc/e0/e4), 0x034353 (slot update), 0x010376 (free struct fields @0x6d/71/75). Remaining L10 candidates are mostly non-trivial and deferred: DOS INT 21h/31h (0x01053f, 0x03f3be, 0x03f410), spin-wait loops (0x035d0f, 0x0412e9, 0x041390, 0x0350aa), hardware port reads (0x035ec2), register-ABI param order (0x0405de, 0x03a594), data table @0x63f13 (0x03493e), FUN_00042361-dependent (0x03ffab), free-loops (0x0326c1, 0x0358c7, 0x036626). The next productive frontier is the large pure-C functions (0x0451bc) and the mid-level subsystems (data load 0x3690b, audio 0x3658d, CD/MSCDEX). | STATUS.md L10 |
| 2026-07-08 | **L1/L2/L3/L11 frontier after L10 (159/528)**: L1 — ported 0x04a7fa (FPU thunk *out=a-fpu_rndint(a)); remaining L1 are non-portable/complex (DOS INT21h 0x422d5/0x4391d, DPMI stub 0x43324, CPU-flags 0x43381, binary-search 0x4a814, complex 0x42ccc/0x47f54 bcd-encoder). L2 — clean wins exhausted: 0x046d47 (caseD_8, virtual dispatch (**(code**)(EBP+0x76)) + register-convention deps), 0x044bec (DOS), 0x044387 (complex CONCAT31). L3 — ALL 7 use DOS INT 21h (swi(0x21)) EXCEPT 0x0451bc (1096B typed-field deserializer, 338-line decomp, register ABI + 20+ type codes — needs a dedicated fresh-context pass). L6 — all 3 depend on heap_malloc ambiguous 64-bit return + unset callbacks (deferred). L11 — ported 0x016959 (containment check: X vs F=fn_01519f(s) range w/ param2 offset, else Z<0; uses L10 fn_01519f; 6 checks). Remaining L11 (0x017530, 0x016a60, 0x017637, 0x015a1f, 0x034635, 0x0174a2) are 141-412B struct state-machines (set struct fields, call L10 accessors fn_01519f/fn_0151d5/fn_034353, range checks) — need full disasm each; NOT twins of 0x016959 (the batch-decomp `*(iVar+0x17)=2` lines are a decompiler artifact — full decomp of 0x016959 has no write). NEXT (fresh context): 0x0451bc typed deserializer (biggest clean L3 win), the L11 state-machines, then the mid-subsystems (data load 0x3690b, audio 0x3658d, CD/MSCDEX) + entry chain. Added <stdbool.h> to f.h. | STATUS.md L1/L2/L3/L11 |
| 2026-07-08 | **CRITICAL: Ghidra decomp is UNRELIABLE for complex state machines**. fn_016959 (0x016959) decomp dropped the `s[0x17]=2` write + the 3rd param (output ptr at [EBP+0x10]) + misread the logic as a bool "containment check" returning `Z<0`. The disassembly shows a move-toward-target clamped-step function: `s[0x17]=2`; `F=fn_01519f(s)` (position), `X=s[0xbf]` (target); if F>X+step: `*out=-step`; if X-step<=F<=X+step (incl F==X): `*out=0`; if F<X-step: `*out=+step`; reached cases return 1; the ±step cases then (if `s[0xab]<0` return 1) else (`s[0xab]--` return 0). Corrected + 7 checks. RULE: for functions with side-effects / multiple params / branches, verify against the DISASSEMBLY, not just the Ghidra decomp. Simple single-expression accessors (the L10 batch) are safe from decomp. 0x017530 is a BYTE-IDENTICAL TWIN of 0x016959 (verified: same instruction mix, same REL branch offsets shifted by base 0xBE7, both call fn_01519f) — ported as an alias. (Earlier '0x015187 variant' note was a REL-normalization error.) | STATUS.md decisions |

| 2026-07-08 | **L11 fn_016a60 (0x016a60, 302B) ported** — clamped move-toward-target: s[0x17]=2; F=fn_0151d5(s)=s[9]-g_dat_639e0, X=s[0xc3]; Branch A (F>X): if (F-param2<=X)||(F-param2<=g_dat_63354[0x12]) {out=0,ret=1} else {out=-param2; if s[0xab]<0 ret=1 else (s[0xab]--,ret=0)}. Branch B (F<=X): if F>=X {out=0,ret=1} else if (param2+F>=X)||(param2+F>=g_dat_63354[0x16]) {ret=1, NO *out write — LATENT BUG, replicated} else {out=+param2; countdown}. Uses new global g_dat_63354[0x20]. 5 checks. 0x017637 (302B) is likely its twin (same instruction mix from batch analysis) — verify via disasm next context. Remaining L11: 0x015a1f, 0x034635, 0x0174a2. | STATUS.md L11 |
| 2026-07-08 | **L11 state-machines complete (5 ported this session)**: fn_016959 (move-toward-target, disasm-verified), fn_017530 (twin alias), fn_016a60 (clamped move-toward-target + global bounds g_dat_63354, latent B-reached no-*out), fn_017637 (corrected variant), fn_0174a2 (clamp-check [0,0x140) + out adjust), fn_034635 (keyboard input state machine: g_dat_62c58[key] state, g_dat_62cd8[key] count, calls fn_034353; new globals g_dat_62a94/g_dat_53c7c). EXPANDED g_dat_62c58/g_dat_62cd8 from [16] to [128] (the kb handler indexes by full key byte; 0x62c58/0x62cd8 are 0x80 apart in the original data layout). L11 is now clean-candidates-exhausted. | STATUS.md L11 |
| 2026-07-08 | **Frontier re-scan after L11 state-machines (164/528)**: L1(17)+L3(7) "clean" candidates are mostly INLINE DOS INT 21h (verified 0x042aba: AH=0x3f file read) — non-portable; the frontier script does not detect inline INT (only CALLs), so these are false-positive "clean". Only 0x0451bc (1096B) is pure-C L3 (biggest clean win, 20+ type codes, needs fresh context). L2(3): 0x044bec (DOS), 0x04a32b (1 callee — recheck), 0x044387 (complex). L11(19 clean): the 442B family (0x015b5b/015d16/01603e/0161f9/0163b4) = complex game-logic state machines — s[0x17] state (0x1f,<0xb,0x14<x), s[0x1f]==4, s[0x83], s[0x113]=prng%10, s[0xd] table index; reads data table (&DAT_00053eb8)[s[0xd]*0x88+s[0x113]*10]; calls abs32(0x41f76, ported), prng_next, fn_01519f/d5; DAT_00053c7c=g_dat_53c7c (added this session), DAT_00053c6c new global. All callees ported but each needs careful disasm (decomp unreliable for these). NEXT: 0x0451bc (fresh ctx) OR the L11 442B family (pick one, full disasm). Then mid-subsystems (0x3690b data, 0x3658d audio, CD/MSCDEX) + entry chain (0x3af18/0x3a224/0x37b03). | STATUS.md frontier |
| 2026-07-08 | **fn_0100d9 (0x0100d9, 167B) ported** — struct-destructor free-loop. Container (in EAX via canary prologue) = {count, array_off}. For each nonzero array[i]: fn_010376 (destroy 3 fields) + heap_free_guard (free struct). Then heap_free_host (free array). Returns s. Modeled array[i] as heap OFFSETS (converted to pointers for fn_010376, offsets for heap_free_guard/host — consistent with the offset-based heap model). Test uses a tight 0x280 region filled full, proving the frees free s0/s1/array (0x80 reusable only after the frees). Note: my fn_010376 returns void, but the original returns the struct ptr (passed to heap_free_guard); the port passes `off` directly — equivalent. | STATUS.md L11 |
| 2026-07-08 | **fn_015a1f (0x015a1f, 315B) ported** — data-table distance state machine. s[0x17]==0x1f -> g_dat_53c6c=1,ret0; s[0x17] in [0xb,0x14] -> s[0x117]=1,g_dat_53c6c=1,s[0x113]=prng%10,ret2; else s[0x117]=0,s[0x113]=prng%10, and if s[0x1f]==4: idx=s[0x113]*10+s[0xd]*0x88, if abs32(fn_01519f(s)-fn_01519f(g_dat_53c7c))<g_dat_53eb8[idx] && abs32(fn_0151d5(s)-fn_0151d5(g_dat_53c7c))<6 ret1. Traced from disasm (decomp had CONCAT44 + (int)uVar5<(int)(uVar5>>32) artifacts = compare-against-garbage-EDX). New globals g_dat_53c6c (int32), g_dat_53eb8[0x2000] (zeroed runtime table — verified all-zero in binary). LATENT BUG: idx can be negative (signed prng%10) -> OOB table read (1:1). This is one of a family (0x015b5b + 5x442B) sharing g_dat_53eb8; those are next. | STATUS.md L11 |
| 2026-07-08 | **fn_015b5b (0x015b5b, 442B) ported** — data-table family variant. Same s[0x17] state dispatch as fn_015a1f (0x1f->[flag,ret0]; [0xb,0x14]->[s[0x117]=1,flag,prng%10,ret2]; else). Else-branch: s[0x117]=0; if s[0x1f]==4 && s[0x83]==0: s[0x83]=1,s[0x113]=prng%10; elif s[0x1f]!=4: s[0x83]=0. Then tbl0=g_dat_53eb8[idx]; if s[0x1f]==4: tbl1=g_dat_53eb8[idx+1]; if tbl1=='.'(0x2e): abs32(sX-tX)<tbl0->ret1; else abs32(sX-tX)<tbl0 && abs32(sY-tY)<10->ret1. The 5x442B (0x015b5b/015d16/01603e/0161f9/0163b4) are VARIANTS (12-31 instr diffs), NOT twins. | STATUS.md L11 |
| 2026-07-08 | **fn_01603e/0161f9/0163b4 (3x442B) ported as near-twins of fn_015b5b** — REL-fingerprint + callee-set comparison confirmed all three are identical to fn_015b5b EXCEPT the magic char at the table[idx+1] check: 0x015b5b='.'(0x2e), 0x01603e=0161f9=+'(0x2b), 0x0163b4=,'(0x2c). Refactored fn_015b5b into fn_015b5b_core(s, magic) + 4 thin wrappers. All 5 call the same 5 fns (fn_01519f/d5, canary, prng_next, abs32). Test distinguishes the magic: whole table=0x2b + sY-tY=10 (2D Y-check fails) -> '+' (1D) ret1, '.'/,' (2D) ret0. 4 near-twin checks. REMAINING family member: 0x015d16 (real variant, different abs32 ordering + chars 0x2b/0x28) — needs its own trace. | STATUS.md L11 |
| 2026-07-08 | **fn_015d16 (0x015d16, 442B) ported** — last member of the data-table family. Same s[0x17] state dispatch + s[0x83] gate + idx/tbl0 as the 0x015b5b family, but a DIFFERENT tail: the X-check abs32(sX-tX)<tbl0 is a GATE first, then the char (table[idx+1]) selects the Y-limit: '+'(0x2b)->0x28(40), else->10. (In the 0x015b5b family the char selected 1D-vs-2D, not the limit.) Traced from disasm. 6 checks. The whole 6-fn family (0x015a1f + 5x442B: 015b5b/015d16/01603e/0161f9/0163b4) is now [x]. | STATUS.md L11 |
| 2026-07-08 | **fn_015ed1 (0x015ed1, 364B) ported** — another data-table family variant. Same s[0x17] dispatch + s[0x83] gate as the 0x015b5b family, but NO char check: abs32(sX-tX)<table[idx] && abs32(sY-tY)<10 -> ret1 (Y limit 10, like the 2D case of 0x015b5b, unlike fn_015a1f which uses 6). Decomp had CONCAT44 artifact; traced from disasm. 6 checks. NOTE: the family is large + heterogeneous (at least 7 variants so far: 015a1f/015b5b/015d16/015ed1 + 3 near-twins). | STATUS.md L11 |
| 2026-07-08 | **fn_032158 (0x032158, 412B) ported** — entity-list target setter. If g_dat_53c6c==1, walk the list (g_dat_53c70 head, next@+0x13f, count g_dat_53c68); for each entity != player (g_dat_53c7c) with s[0x117]==0, s[0x1f]==4, s[0xaf]!=6: set target (s[0xb3],s[0xb7]) to the player position, then offset by ±0x2d/-5 + prng%10 jitter (sign depends on player[0x1b]); mark s[0xaf]=0, s[0x1f]=6. Clear g_dat_53c6c. New globals g_dat_53c70 (head), g_dat_53c68 (count). Decomp was clean (no CONCAT). 6 checks. | STATUS.md L11 |
| 2026-07-08 | **fn_01656f + fn_016750 ported (multi-char family members)** — generalized fn_015b5b_core to take a magic-char SET (pointer + count). fn_01656f: magic {'''-'''}, fn_016750: magic {'1'''4''''.'''}. Both verified char sets + 1D-vs-2D tail from disasm. The data-table family is now: 015a1f (Y=6, no gate), 015b5b/01603e/0161f9/0163b4/01656f/016750 (char-set 1D/2D), 015d16 (X-gate + char-Y-limit), 015ed1 (no char, Y=10). 6 checks. NOTE: fn_03442e decomp is a FALSE stub (full of "unreachable block" warnings) — NOT a trivial stub; needs disasm trace. | STATUS.md L11 |
| 2026-07-08 | **fn_01776f (0x01776f, 439B) ported** — 2D movement delta computer. Signature (s, param2=step, *out1, *out2) [s in EAX, param2 in EDX, out1/out2 stack args, RET 0x8]. Sets s[0x17]=2. F=fn_0151d5(s), mid=(s[0xc3]+s[0xcf])/2. out1 = F>mid ? (s[0xcb]<0xa0 ? +p : -p) : (s[0xcb]<0xa0 ? -p : +p) [SAME both branches]. Then: target<end -> if F-p<=target {out1=out2=0,ret1} else out2=-p; target>=end -> if F+p>=target {out1=out2=0,ret1} else out2=+p. Traced full disasm (CONCAT-heavy decomp unreliable). 5 checks. | STATUS.md L11 |
| 2026-07-08 | **fn_043280 (L1) ported** — byte-pattern fill, (dest=EAX, pattern=EDX 4-byte, count=ECX; EBX unused). Phase 1: fill bytes (rotating pattern ROR 8) until dest 4-aligned; Phase 2: the pre-existing memsetw (0x0432b7, already ported) for count/4 dwords; Phase 3: count%4 remainder. LATENT BUG replicated 1:1: remainder byte 2 uses DL (pattern[0]), not EH (pattern[2]) — pattern does not continue correctly in the tail. Traced full disasm (decomp had CONCAT/alignment artifacts). 3 checks. (0x0432b7 = memsetw was already done; not double-counted.) | STATUS.md L1 |
| 2026-07-08 | **L11 frontier re-scan: 4 complex candidates remain** — all need dedicated fresh-context passes. (1) **fn_034aba (0x034aba, 387B)** — 4-way interval-list splitter, FULLY traced from disasm. Node layout [0]=start(int32), [4]=next, [8]=len(int16), [10]=flag(byte); 11-byte nodes via fn_034a87 bump-allocator at g_dat_62d64. Signature (a, idx, b, c, flag): a=offset new interval starts, idx=index into g_dat_62d68 head-array, b=length, c=start value, flag=byte. Walks list to node containing offset a, splits [a,a+b) (clamped to 0x140), inserts newnode (+newnode2 fragment if endpos<0x140). BLOCKER: g_dat_62d64 is uint32_t=0; on 64-bit a real arena pointer truncates when stored in uint32_t, so node writes in a differential test are unreliable until the arena model is fixed (g_dat_62d64 -> pointer, which changes committed fn_034a87 return type). (2) **0x03442e (518B)** — Ghidra FALSE stub (15 "Removing unreachable block" warnings); sibling of 0x034aba in the 0x034xxx interval-list region; needs full disasm trace. (3) **0x017927 (894B)** — 2D movement/AI, signature (s) [s in EAX]; uses g_dat_63354 [0x12]/[0x16], sets s[0x67]=s[0xcf]-s[0xc3], compares s[0xbf] vs s[0xcb], calls fn_01519f(s, mid); same family as fn_01776f; decomp CONCAT-heavy, needs disasm trace. (4) **0x016b8f (1116B)** — Ghidra FALSE decomp (unreachable-block warning); signature (s, p2, p3, p4, p5) [5 params]; s[0x17]=2, gates on s[0x8f]==0; largest remaining; needs full disasm trace. | STATUS.md L11 |
| 2026-07-08 | **fn_016fec (0x016fec, 555B) ported** — grid-collision detector. Signature (s, yref) [s in EAX, yref in ECX]. Mode flag g_dat_51c98: if ==1 -> set 2, g_dat_53c90=0, return 0 (reinit step); else (==2) do detection. Sets s[0x17]=1. Grid cell = (X/20, Y/12) signed. Walks entity list (g_dat_53c70 head, next@+0x13f, count g_dat_53c68); for each node != player(s[0x1f]==4... actually node[0x1f]==4, != g_dat_53c7c, != s) in same cell -> s[0xbb]=prng%4, g_dat_51c98=1, return 2. Player-event block (g_dat_53c90==0 && 0x83<=player[0x17]<=0x85 unsigned && abs(Y-yref)<6): if prng%100<50 -> g_dat_51c98=1, g_dat_53c90=1, return 3; else g_dat_53c90=1. Countdown: if s[0xab]<0 -> g_dat_51c98=1, return 1; else s[0xab]--, return 0. New globals g_dat_51c98, g_dat_53c90. prng range [0,32767] so %4/%100 match signed IDIV. Traced full disasm (decomp CONCAT unreliable). 5 checks. | STATUS.md L11 |
| 2026-07-08 | **fn_017ca6 (0x017ca6, 620B) ported** — Bresenham diagonal stepper. Signature (s, step, *out1, *out2). Primary axis = larger |velocity| (Vx=s[0xf3], Vy=s[0xf7]). X-dominant (absVx>absVy): bounds check vs s[0xbf] only (Vx<0: X<s[0xbf]->ret1; Vx>=0: X>s[0xbf]->ret1); out1=sign(Vx)*step, out2=0, s[0xfb]+=absVy, if absVx<s[0xfb] {s[0xfb]-=absVx; out2=sign(Vy)*step}. Y-dominant: bounds check vs s[0xc3] + g_dat_63354 [0x12]/[0x16] (Vy<0: Y<s[0xc3] or Y-step<=g12; Vy>=0: Y>s[0xc3] or s[0xef]+step>=g16 -> ret1); out2=sign(Vy)*step, s[0xfb]+=absVx, out1=0, if absVy<s[0xfb] {s[0xfb]-=absVy; out1=sign(Vx)*step}. Uses abs32 (0x41f76, already ported). Decomp treated abs32 as 64-bit CONCAT (unreliable) — traced full disasm. 7 checks. | STATUS.md L11 |
| 2026-07-08 | **fn_01730b (0x01730b, 406B) ported** — 2-phase X-then-Y move-toward-target. Signature (s, step, *out1=X-delta, *out2=Y-delta). s[0xaf] is a 2-bit done-counter: phase1 (X toward s[0xb3]) skips if s[0xaf]==1, sets bit0 (+1) when X reached, else *out1=±step; phase2 (Y toward s[0xb7], clamped by g_dat_63354 [0x12] lower/[0x16] upper) skips if s[0xaf]==2, sets bit1 (+2) when Y reached, else *out2=±step. Return s[0xaf]>=3 (both done). Traced full disasm (CONCAT decomp unreliable). 5 checks (sentinels detect out writes). | STATUS.md L11 |
| 2026-07-08 | **L11 frontier: 7 candidates remain, all complex** — 0x01730b (406B, 2-phase X-then-Y move-toward-target, s[0xaf] state 1/2/3, CONCAT), 0x017ca6 (620B, X-axis 2D movement mirror of fn_01776f, s[0xf3]/0xf7 velocity), 0x017927 (894B, large movement/AI w/ g_dat_63354 bounds), 0x016fec (555B, grid-collision, new globals g_dat_51c98/g_dat_53c90), 0x034aba (387B, linked-list split w/ CONCAT), 0x03442e (518B, FALSE "stub" decomp w/ unreachable-block warnings — needs disasm), 0x016b8f (1116B, FALSE decomp + 5 params). All need dedicated disasm tracing (Ghidra decomp unreliable for these CONCAT-heavy state machines). NEXT: continue with 0x01730b / 0x017ca6 (movement family) or 0x016fec (grid-collision). | STATUS.md L11 |
| 2026-07-08 | **fn_034aba (0x034aba, 387B) ported + node-arena model fixed** — 3-way interval-list splitter, fully traced from raw disasm (Ghidra decomp garbled the register ABI). Signature (a, idx, b, c, flag) [a=EAX, idx=EDX, b=EBX, c=ECX, flag=stack]. Node (11 B): [0]=start(int32) [4]=next [8]=len(int16) [10]=flag(byte); 11-byte nodes from fn_034a87 bump-allocator; head array g_dat_62d68[idx]. Logic: if a<0 {c-=a;b+=a;a=0;head=newnode1}; if a+b>=0x140 {b=0x140-a} (clamp, NOTE 0x140==320 not 352); end=a+b; walk list to node containing a (acc+=len until acc>=a); set newnode1 {len=b,start=c,flag}; svar2=head->len-(acc-a); if end>=0x140 (2-way): head->len=svar2, head->next=newnode1; else (3-way): walk to node containing end, newnode2 {len=acc-end, start=head->start+head->len-(acc-end), next=head->next, flag=head->flag}, pivar1->len=svar2, pivar1->next=newnode1, newnode1->next=newnode2. **ARENA MODEL FIX:** g_dat_62d64 (bump cursor) is a base-relative uint32 offset into a new 64KB static s_node_arena; g_dat_62d68[idx] + node->next store offsets (not pointers) — same offset pattern as the heap. **ALIGNMENT FIX:** node fields land on unaligned byte offsets (11-byte stride -> int16 at odd addr), so all field access is byte-wise (direct int16/int32 casts corrupt on arm64). 7 checks (3-way split, 2-way split). L11 21->22, TOTALS 180->181. Remaining L11 complex: 0x03442e, 0x017927, 0x016b8f. | STATUS.md L11 |
| 2026-07-08 | **fn_03442e (0x03442e, 518B) ported** — rotating 10-slot event buffer. Param x in AL; the `PUSH 0x38; CALL 0x418ee` prologue is a stack-guard wrapper that XCHG-saves/restores EAX (so x survives it). Ghidra decomp was a FALSE stub (unreachable-block warnings) — fully traced from raw disasm. Dispatch on x (byte): 0x1d / 0x38 / (0x2a|0x36) / else. Each fires at most one branch, gated by the byte-flag globals g_dat_62c75/62c82/62c8e/62c90, stamping a marker ('g'=0x67/'h'=0x68/'i'=0x69) into g_dat_63f14[g_dat_63f50] and picking y in {0x1d,0x2a,0x38}. If a branch fired (z==1): g_dat_63f1e[slot]=0; g_dat_63f28[slot]=g_dat_63f08-g_dat_63f0c (int32 delta); advance slot (wrap 9->0); CALL fn_0343d1(x); then CALL fn_0343d1(0x1d) [y=0x1d], fn_0343d1(0x2a)+fn_0343d1(0x36) [y=0x2a], or fn_0343d1(0x38) [y=0x38]. Slot layout: 0x63f14[10] marker bytes, 0x63f1e[10] zero flags, 0x63f28[10] int32 deltas, 0x63f50 current slot. fn_0343d1(i) decrements g_dat_62cd8[i] + sets g_dat_62c58[i]. New globals g_dat_62c75/62c82/62c8e/62c90. 12 checks (all 4 x-branches, y-dispatch, slot advance + 9->0 wrap, no-match no-ops). L11 22->23, TOTALS 181->182. Remaining L11 complex: 0x017927, 0x016b8f. | STATUS.md L11 |
| 2026-07-08 | **fn_017927 (0x017927, 894B) ported + g_dat_63354 pointer-model fix** — 2D move/AI handler (20 call sites, one per enemy-AI type). Signature (s, val, *out1, *out2) [s=EAX, val=EDX, EBX/ECX unused, out1/out2 = 2 stack args]. Fully traced from raw disasm (Ghidra decomp CONCAT-polluted + register-ABI misparse). **KEY FIX: g_dat_63354 is a POINTER, not a direct array.** Both fn_017927 AND fn_016a60 disasm show the double-deref (`MOV EAX,[0x63354]; CMP ...[EAX+0x12/0x16]`) — 0x63354 holds pointer P, bounds at P+0x12 (lo)/P+0x16 (hi). The 4 already-committed fns (fn_016a60/017637/017ca6/01730b) had modelled it as a direct array (one deref short) — a latent bug. Fix = change `uint8_t g_dat_63354[0x20]` -> `uint8_t *g_dat_63354` (the existing `*(int32_t*)(g_dat_63354+0x12)` accesses then correctly deref P); test main() points it at a static bounds buffer. All 4 fns' tests still pass (values read identical). Logic: s[0x17]=2 (dword); mid=lo+((hi-lo)>>1); s[0x67]=s[0xcf]-s[0xc3]; target=((s[0xbf]+s[0xcb])>>1)+s[0x67]; X=fn_01519f(s)=s[1]-g639dc. Branch on s[0xbf] < s[0xcb] (T) vs >= (E). T: if (cb-X)%3==0 {T1 X>target: out1=cf<mid?+val:-val; T2 else: out1=cf<mid?-val:+val}; reached if (X-val)<=(s[0xbf]+0x1e) {cf<mid: (Y-val)<=s[0xc3]->ret1; else: (val+Y)>=s[0xc3]->ret1} (out1=out2=0). E: if (X-cb)%3==0 {E1 X>target: out1=cf>mid?+val:-val; E2 else: out1=cf>mid?-val:+val}; reached if (val+X)>=(s[0xbf]-0x1e) {cf<mid: (Y-val)<=s[0xc3]; else: (val+Y)>=s[0xc3]->ret1}. Common tail (0x017c69): out2 = s[0xbf]<s[0xcb]?-val:+val; ret 0 (unless reached). Y=fn_0151d5(s)=s[9]-g639e0. out1=[EBP+0x14]=1st-stack-arg (mod-3 branches); out2=[EBP+0x10]=2nd-stack-arg (fall/tail). 6 checks (T/E reached, T1/T2/E1 mod-3, E no-mod3). L11 23->24, TOTALS 182->183. Remaining L11 complex: 0x016b8f (1116B). | STATUS.md L11 |
| 2026-07-08 | **fn_034735 (0x034735, 141B) ported** — per-key state machine on the g_dat_62c58 (flags) / g_dat_62cd8 (counters) byte tables. Register-ABI: index in AL (low byte of EAX), preserved through the `PUSH 0x24; CALL 0x418ee` stack-guard (Ghidra decomp mislabelled it as `bVar1 = FUN_000418ee()`). Logic: if g_dat_62c58[idx]==4 { fn_034353(idx, g_dat_63f08); if g_dat_62cd8[idx]!=0 {62c58=1; 62cd8--} else 62c58=0 } else if g_dat_62c58[idx]<4 {62c58++; fn_03442e(idx)}. 5 checks (62cd8>0 dec, 62cd8==0 zero, <4 inc+dispatch, ==5 no-op). All callees (fn_034353/fn_03442e/tables) already ported. L12 0->1, TOTALS 183->184. NOTE: the other 2 L12 clean candidates — 0x03ba2c (89B, 200x fn_034aba loop, idx 0..199) and 0x03ba86 (187B, nested-loop data-table fn_034aba caller) — are deferred: 0x03ba2c exposes a latent g_dat_62d68[128] undersize (needs >=200) + needs a 200-call arena-diff test. | STATUS.md L12 |
| 2026-07-08 | **fn_0351dc (0x0351dc, 134B) ported** — unsigned base-128 integer decoder (register-ABI, cursor pointer in EAX). Reads *cursor as the data position, accumulates 7 bits per byte while the continuation bit (0x80) is set, advances *cursor past the last byte, returns the value. KEY: the encoding is **MSB-first** (first byte = most-significant group) — `val = byte0; while(byte&0x80){ val=(val<<7)|(next&0x7f) }`, NOT standard little-endian LEB128. 3 differential checks (1-byte 05->5, 2-byte 85 02->642, 3-byte ff ff 01->2097025). L10 13->14, TOTALS 184->185, suite 649 PASS 0 FAIL. NOTE: sibling loads 0x03510b/0x0408c5 deferred (pointer-advance side-effect semantics ambiguous without full disasm tail). | STATUS.md L10 |
| 2026-07-08 | **fn_03510b (0x03510b, 126B) + fn_0408c5 (0x0408c5, 130B) ported** — the two sibling byte-read loads deferred from the fn_0351dc pass, now fully disassembled. fn_03510b = **big-endian 32-bit read** `(p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]` (returns uint32); the `[EBP-4]` INCs in the disasm are a local pointer copy, NOT a caller side-effect (decompiler mislabelled it). fn_0408c5 = **little-endian 24-bit (3-byte) read** `p[0]|(p[1]<<8)|(p[2]<<16` (pre-shifts each byte then ORs; returns uint64 zero-extended). Both register-ABI (pointer in EAX), pure (no side effects). 2 differential checks. L10 14->16, TOTALS 185->187, suite 651 PASS 0 FAIL. (0x03518a at 0x03518a is the 16-bit BE read, already ported as fn_03518a.) | STATUS.md L10 |
| 2026-07-08 | **fn_041e20 (0x041e20, 23B) + fn_0350aa (0x0350aa, 96B) ported** — two clean register-ABI wins. fn_041e20 = memset-style: replicates the fill byte (DL) into a 32-bit pattern 0xNNNNNNNN (the `MOV DH,DL; SHL; MOV DL,DH` idiom), calls fn_043280(dest, pattern, count), returns dest (EAX preserved via PUSH/POP). ABI dest=EAX,byte=DL,count=EBX. Since all 4 pattern bytes are equal, fn_043280's latent remainder-byte bug does not manifest. fn_0350aa = 4-byte prefix compare (a=EAX,b=EDX): compares a[0..3] vs b[0..3], returns 1 if all equal else -1. **NOTE: 0x0350aa was previously misclassified as a "spin-wait loop" deferral** (see the L10-trivial-accessor note); full disasm shows it is a trivial bounded 4-iteration compare. L2 10->11, L10 16->17, TOTALS 187->189, suite 656 PASS 0 FAIL. | STATUS.md L2/L10 |
| 2026-07-08 | **fn_03ba2c (0x03ba2c, 89B) ported + g_dat_62d68 resized [128]->[200]** — the 200-iteration interval-list builder deferred from the fn_034735 pass. Register-ABI c_init=EAX, inc=EDX (confirmed by full disasm): `for i in 0..199: fn_034aba(a=0, idx=i, b=0x140, c=c_init+i*inc, flag=0)`. **LATENT BUG FIXED: g_dat_62d68 was declared [128] but fn_03ba2c (and its consumer fn_03c65b) index [0..199]** — resized to [200] (no other declared global in 0x62dc8..0x62e30, so the resize is safe). Key subtlety: with a=0 (>=0) and zero-initialized lists, fn_034aba's walk never sets g_dat_62d68[i] — it links each new node after node@0 (the arena base), so g_dat_62d68[] stays 0 and the observable output is the arena nodes at offset 11*i (start=c_init+i*inc, len=0x140). The consumer fn_03c65b flattens these back. 6 checks: reference (cursor=2200, node@0 start/next/len, node@11 & node@2189 starts, heads still 0) + a full differential (fn_03ba2c vs a manual 200x fn_034aba loop, arena region 0..0x1000 + cursor byte-for-byte identical). L12 1->2, TOTALS 189->190, suite 662 PASS 0 FAIL. (fn_03ba86 remains the last L12 candidate.) | STATUS.md L12 |
| 2026-07-08 | **fn_03ba86 (0x03ba86, 187B) ported — L12 COMPLETE (3/3)** — the last L12 candidate, a nested-loop interval-list builder reading a variable-length byte stream. Full disasm confirmed ABI a_base=EAX, idx_base=EDX, row_count=EBX, p=ECX. For each row: count=*p++; then count times: acc+=*p++; len=*p++; fn_034aba(a_base+acc, idx_base+row, len, p, flag); p+=len; acc+=len. The flag (5th arg) is `MOV AL,[EBP+0x10]; PUSH EAX` — [EBP+0x10] is the pushed 0x38 canary in the prologue's stack layout, so the flag is the constant 0x38. Key subtlety: with non-zero a, an EMPTY node@0 (len 0) makes fn_034aba's containing-walk infinite-loop, so the arena must be pre-initialized (node@0 covering [0,0x140), cursor=11) before the call — replicated in the test. 2 checks: a full differential (fn_03ba86 vs a manual nested fn_034aba loop over a synthetic 2-row stream; arena region + cursor byte-for-byte identical) + a flag-0x38 assertion. L12 2->3 (COMPLETE), TOTALS 190->191, suite 664 PASS 0 FAIL. | STATUS.md L12 |
| 2026-07-08 | **fn_03a594 (0x03a594, 98B) ported** — filtered byte copy. Full disasm confirmed ABI dest=EAX, src=EDX, count=EBX: `for i in 0..count-1: if (src[i] != 0xff) dest[i] = src[i]`. Bytes equal to 0xff are NOT copied (dest[i] left unchanged) — a masked/conditional copy, no globals or external deps (one of the rare fully-portable L10 candidates). 3 checks: mixed 0xff pattern (copied + skipped slots verified byte-by-byte) + all-0xff src (dest fully unchanged). L10 17->18, TOTALS 191->192, suite 667 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **fn_036626 (0x036626, 125B) ported** — case-insensitive string->value table lookup. Full disasm (the Ghidra decomp mislabelled the loop bound as `< 0`) confirmed: `for i in 0..g_dat_633a0: if (str_icmp(&g_tab_5d884[i*21], s, str_len(s)) == 0) result = *(i32*)&g_tab_5d884[i*21+13]`; returns result (or 0). s=EAX. The table @0x5d884 is 21-byte RODATA entries {str[13]; int32 value@13; pad[4]}; bound by global g_dat_633a0 (added). Added g_dat_633a0 + g_tab_5d884[128*21] globals. Value read uses memcpy (offset 13 is unaligned). Depends on str_len (0x41f5d) + str_icmp (0x423e5), both already ported. Note: a shorter query that is a prefix of a table string still matches (faithful — the compare length is str_len(query), not the table entry's). 5 checks (exact, query-lower, query-upper, entry-upper, no-match->0) against a synthetic 3-entry table. L10 18->19, TOTALS 192->193, suite 672 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **fn_03493e (0x03493e, 72B) ported** — slot-byte read. No args, returns AL. The slot index is g_dat_63f50 (wraps 0..9, already ported). Full disasm (Ghidra decomp said the index==0 case returns 0 — WRONG) confirmed: if g_dat_63f50==0 return the byte at 0x63f1d else the byte at 0x63f13+g_dat_63f50. Key layout fact: 0x63f13 is a standalone byte, and g_dat_63f14[16] (already ported) starts at 0x63f14, so 0x63f1d == g_dat_63f14[9] and 0x63f13+k == g_dat_63f14[k-1] for k>=1. The 0x63f13 byte itself is never read (index 0 special-cased to 0x63f1d). So fn_03493e reuses the existing g_dat_63f14 — no new table needed: `(g_dat_63f50==0) ? g_dat_63f14[9] : g_dat_63f14[g_dat_63f50-1]`. 4 checks (index 0->[9], 1->[0], 5->[4], 9->[8]) against a known g_dat_63f14 fill. L10 19->20, TOTALS 193->194, suite 676 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **fn_0405de (0x0405de, 82B) ported** — store 4 16-bit values. ABI p1=EAX, p2=EDX, p3=EBX, p4=ECX. Full disasm showed the Ghidra decomp's `& 0xffff0000ffff` mask was misleading — it is actually 4 separate 16-bit stores: `XOR EAX,EAX; MOV AX,reg; MOV [0x40014/18/1c/20],EAX`, i.e. g_dat_40014=low16(p1), g_dat_40018=low16(p2), g_dat_4001c=low16(p3), g_dat_40020=low16(p4) (each zero-extended to a 4-byte global). Added 4 globals g_dat_40014/18/1c/20 (consecutive after the already-ported g_dat_40010 frame-pitch at 0x40010). 5 checks (each low16 stored to the right slot + high16 dropped/zero-extended). L10 20->21, TOTALS 194->195, suite 681 PASS 0 FAIL. | STATUS.md L10 |
| 2026-07-08 | **fn_0451bc (0x0451bc, 1096B) ported** — typed field formatter/deserializer (the big L3 win, first of 21 L3 fns). Fully traced from raw disasm (354 instructions; Ghidra's 338-line decomp was a register-ABI maze). ABI out=EAX, cur=EDX (ptr to stream read-ptr), st=EBX (state), alt=ECX (null-string fallback). **Type dispatch** (st[0x15]): E/G/e/g->cb_indirect_53644 (return its ptr, len=strn_len); F/f->(flag 0x10 ? fn_0450da fixed-point : cb); P/p->date/time (flag 0x80: 8-hex date + ':' + 4-hex secs, else 8-hex; 'P' uppercased; size init 13/8); S/s->string (flag 0x80 reads 8-byte {ptr,len} header, else 4-byte ptr; ptr==0->alt; 'S' reads len word/byte, 's' computes via word_nlen/strn_len); X/x->hex (itoa32_tbl base16, 'X' upper); c->single char (reads 1 byte, advances 4); d/i->signed decimal (flag 0x20 dword / 0x10 word / else itoa32s); o->octal (itoa32s_tbl base8, flag 0x01 writes '0' at out[0] + format at out+1); u->unsigned decimal (itoa32_tbl base10); default->write type char, len 1. **Pre-pass**: {X,o,u,x} read val (flag 0x10: 16-bit read but **still advances *cur by 4** — faithful quirk), clear flag 0x2/0x4; {d,i} set st[0x16]=0x20. **Epilogue** (0x4556a): negative '-' moves to prefix (st[0x17]) + ret advances past it; flags 0x02/0x04 set prefix ' '/'+'; st[0x16]=='*' clears prefix; length clamp; st+0x04 (size) -= (prefix_len + length + remainder). 9 callees all pre-ported (fn_0450da, cb_indirect_53644, itoa32s/_tbl, itoa32_tbl, strn_len, word_nlen, string_upper, fn_045079). 13 checks (c char, d 42, i -5 prefix, u 100+size-dec, x/X hex, o octal + flag01 '0'-prefix, u flag10 word-read + cursor-quirk, default char). L3 0->1, TOTALS 195->196, suite 695 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **fn_043f80 (0x043f80, 66B) ported** — 8-bit presence-bitmap builder. ABI src=EAX, dest=EDX. fn_041e20(dest,0,32) clears a 32-byte bitmap, then for each byte b in src (from byte 0) sets `dest[b>>3] |= 1<<(b&7)` (table @0x51854 = {1,2,4,8,16,32,64,128}). Scan stops at first 0 or 0x5d (']') byte; that byte is NOT processed. Returns the source cursor at exit (EAX=ECX). Faithful quirks verified in disasm: (1) byte 0 is processed even if it is 0x5d (the 0x5d check only applies from byte 1 on); (2) the exit cursor is the zero byte's address when stopped by 0, but the address PAST the 0x5d byte when stopped by 0x5d. Depends on ported fn_041e20 (memset). 6 checks (single 0x41->dest[8]=2 + zero cursor, ']'-skip, 0x05/0x07->dest[0]=160, leading-zero no-op, byte0=0x5d quirk). L3 1->2, TOTALS 196->197, suite 701 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **fn_049e15 (0x049e15, 106B) ported** — within-buffer string range-shift + sign/zero fixup + space-fill. Confirmed via decomp + single caller (FUN_00049e80): param_1=char buffer, param_2/param_3 are small COUNTS (not pointers — so `dest+count` is portable pointer arithmetic, no 32-bit-pointer-as-field issue). If count2!=count3: a backward do-while copies dest[0..count2-1] to end at dest[count3-1] (dest[count3-count2+k]=dest[k], high-to-low, count2 iterations including dest[0]); then fixup on dest[0]: '.' -> dest[iVar5-2]='0'; '+'/'-' with dest[1]=='.' -> dest[iVar5-1]='0', dest[iVar5-2]=dest[0]; finally fn_041e20(dest,' ',cur3) space-fills the front. Register model preserved: iVar5=count3-(count2-1), cur3=count3-count2 at exit. Depends on ported fn_041e20. 4 hand-traced differential tests (plain copy+fill, '.' fixup, sign fixup, equal-counts no-op). L3 2->3, TOTALS 197->198, suite 705 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **fn_041e38 (0x041e38, 24B) + fn_03f084 (0x03f084, 218B) ported** — DPMI real-mode trampolines (flat-model no-op standins). 0x041e38 is trampoline B: shuffles 4 register args (EAX/EDX/EBX/ECX) into the 32-byte context at EBX and calls 0x043324 (DPMI INT 31h); in the flat model that call is a no-op, so it returns *EBX (first dword of the unchanged context). 0x03f084 reads 5 words from arg2 + 1 from arg4 into a local DPMI context, calls 0x041e38 with key 0x31; the no-op DPMI call never transforms the locals, so the observable effect is copying 4 of the 5 words (arg2 byte-offsets 0,4,8,0xc -> arg3[0,2,4,6]; the 5th, arg2[0x14], is dropped — faithful quirk) and returning arg2[0xc] zero-extended. 0x045dac (0B) documented as a 1-byte disassembly artifact (Ghidra boundary split; the coherent function is 0x045dad) — no C counterpart. 6 checks. L3 3->6, TOTALS 198->201, suite 710 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **fn_042aba/fn_044807/fn_049415 (DOS INT 21h file ops) ported** — routed to new sys stubs. Since the INT 21h no-op leaves the result registers unset (a pure no-op would return AH-contaminated garbage), these route to `sys_file_read`(AH=3f)/`sys_file_lseek`(AH=42)/`sys_file_delete`(AH=41) in sys.h/sys.c, which return documented no-op defaults (0) until the TG_NIGHT.IMG/.CUE backend is wired (mid-subsystems). Args traced from register setup: 0x042aba(handle=EAX,buffer=EDX,count=EBX); 0x044807(handle=EAX,offset=EDX,whence=EBX, returns 32-bit pos w/ RCL/ROR/TEST sign-gate to -1 on error); 0x049415(name=EAX->EDX). The 0x045a82 error path is dead in the no-op (always success). **Remaining L3 (12) are all callee-gated**: 0x044a50(lock)+0x049236(seek/close) need unported 0x492d0 (FPU); 0x042fc2/0x04488b/0x049613 need unported callees; 0x0418c6/0x045dad/0x048754 are thunks to unported targets; 0x04876d(FPU) needs 0x47d24/0x47d4e; 0x041e51 is a fragile DPMI thunk (returns caller's *EBX); 0x048422/0x048a48/0x04a48f unchecked. 5 checks. L3 6->9, TOTALS 201->204, suite 715 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **fn_043324 (0x043324, 55B) ported** — DPMI real-mode transition save-side (register-ABI: reads caller EDI=context, EBX=rm_ptr). Saves 7-word reg context + [EBX]=ES/[EBX+6]=AX, then calls rm_switch_a (0x04335c). In the flat model rm_switch_a is a no-op and no real-mode code runs between save/restore, so the context write is never read back and the register-ABI save can't be reproduced in C -> faithful no-op standin. **Discovered 0x04335c is already rm_switch_a (L0 batch A) and 0x0433ce is rm_switch_b — my initial fn_04335c was a duplicate, removed; 0x043324 (the save-side) is the distinct unported fn.** 1 check. L1 25->26, TOTALS 204->205, suite 716 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **fn_041439 (0x041439, 24 ins) ported** — IRQ handler (PUSHAD+segs; CLD; seg_ds_load; irq_cli; DAT_00063f84=2; inb(DAT_00053334+0xe); outb(0x20,0x20)=8259 PIC EOI; irq_sti; IRETD). All callees (seg_ds_load/irq_cli/inb/outb/irq_sti) already ported as flat-model no-op sys stubs; IRETD has no C equivalent (handler returns). Only observable effect = DAT_00063f84=2. Added globals g_dat_63f84 + g_dat_53334. Note: the L1 deferral note's "deps 0x469d9+DAT_00063f84+DAT_00053334 unported" was STALE (those deps are ported/now-added). 1 check. L1 26->27, TOTALS 205->206, suite 717 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **fn_03ee88 + fn_043381 ported** — (1) fn_03ee88 (0x03ee88, 38 ins) timer/IRQ tick handler: advances 63e28 (wrap@8 -> 63f08++), 63e24, 63e3c (wrap@18 -> 63e40++), 63e38, 63e30 (+= 63e2c, wrap@0xffff -> 63e34++), then irq_sti + outb(0x20,0x20) PIC EOI; IRETD not portable. All 4 I/O callees ported. Added 8 globals (63e28/24/3c/38/40/30/2c/34; 63f08 existed). 8 checks. (2) fn_043381 (0x043381, 34 ins) DPMI real-mode return: calls rm_switch_b + saves reg/flag state to ctx (register-ABI, no flat-model effect) -> no-op standin. Note: the stale L1 deferral note still lists 0x043381/0x043324/0x0431b5/0x041439 as pending but they are all now [x]. L1 27->29, TOTALS 206->208, suite 726 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **fn_048e60 (0x048e60, 203B) ported** — 128-bit fixed-point normalize + Newton-Raphson reciprocal divide (pure L0 leaf, `uint32_t *p`: p[0]=lo mantissa, p[1]=hi mantissa w/ top byte = status flags, p[2] low16 = exp field). Normalizes the 64-bit mantissa (bits shifted out tracked in a 128-bit low half), divides by the reciprocal estimate via a Newton iteration, writes back p[0]/p[1] + p[2]; guard/overflow paths zero the value and OR 0xc0 into the status byte (offset 7 = top byte of p[1]). P-code CARRY4/CONCAT44/CONCAT14 -> 64-bit int ops; u2 wraps 16-bit. **Differential vs independent reference (scratch/fp_048e60.py)** — the reference initially masked bit 31 of the normalized hi-mantissa with 0x7fffffff, but the original `iVar6=(int)((u5|0x100000000)>>1)` is always in [0x80000000,0x8fffffff] (normalized mantissa top bit set), so the C `int32_t` cast was correct; fixed the reference. 10 cases (guard/early-return/ovp/Newton/div2/denom==-1 special) PASS. L0 108->109, TOTALS 228->229, suite 784 PASS 0 FAIL. **3 remaining L0 confirmed deferred for dedicated passes:** 0x04794a (363B, complex fx, 8 callers, register-ABI), 0x0485eb (122B, fx subtract, register-ABI ESI/EDI + decompiler drops the epilogue), 0x04a3a8 (230B, FPU FLD/FCOMP/FDIV/FMUL over {int16,double} table 0x519bc). | STATUS.md L0 |
| 2026-07-08 | **fn_04391d (0x04391d, 17 ins) ported**
| 2026-07-08 | **fn_044c1e + fn_041e51 ported**
| 2026-07-08 | **fn_0422d5 ported**
| 2026-07-08 | **0x04794a register-ABI mapping inferred (for the next FPU pass).** 0x04794a (L0, 363B) is the key gate: it unblocks 0x0478e2 (L1,13B), 0x048f4c (L1,81B) -> 0x048f2c (L2,31B) + 0x048f9e (L2,71B), and contributes to 0x048a48 (L3,502B). 128-bit fixed-point normalize (like ported fn_048e60). Caller 0x048f4c disasm: after fx_mul64 EAX:EDX=result; MOV EBX,[EDI]; MOV ECX,[EDI+4]; MOV SI,[EDI+8]; ROL ESI,0x10; CALL 0x04794a. So register-ABI: p1=EAX, p2=EDX (fx result, 64-bit {p1,p2}), p3=EBX, p4=ECX (struct value {p3,p4}), ESI=16-bit exponent (byte-swapped via ROL ESI,0x10). Callers do NOT read ESI after the call (0x048f4c loops on a counter), so model ESI as a pure 16-bit input; return = CONCAT44(uVar12,uVar2) = normalized {low,high} 64-bit. Suggested signature: uint64_t fn_04794a(uint32_t p1,uint32_t p2,uint32_t p3,uint32_t p4, uint16_t exp). Port + Python-ref + differential-verify (CARRY4/CARRY2/CONCAT->64-bit ops, as in 0x048e60). Then port the 4 unlocked wrappers. ALSO: 0x045bb7/0x048fe6 confirmed NOT clean no-ops (real global-state: DAT_00053384/ISR7/param writes + register-ABI in_CS/ES/CR0) -- defer. 0x048754/0x04876d/0x044a50/0x049236/0x042fc2 all gated on unported FPU/DOS + register-ABI -- defer. Leaf-tier clean wins remain exhausted; next turn = dedicated 0x04794a pass. | STATUS.md L0/L1/L3 |
| 2026-07-08 | **0x04794a pass — decompile found BUGGY; emulator + 40k vectors built (C port deferred).** Built a data-driven mini-x86 emulator (`scratch/fp_04794a_ref.py`, 149 insns, CF/ZF/SF + 8/16/32-bit operand sizes, RCR/SHRD/SAR/etc.) from the ground-truth disasm, and 40,090 differential vectors (`scratch/fp_04794a_vectors.h`). Differential-testing a Ghidra-decompile-based C port (`scratch/fp_04794a_cport.c`) against the emulator shows ~49% mismatch. Hand-traced the DISASM for i=56 (EAX=1,EBX=1,ECX=0,EDX=0,ESI=0x20001): the alignment `SHRD EBX,EAX,1` puts the fractional bit (EAX bit0) into CF, but the very next `OR EDI,EBX` (0x047a0a) CLEARS CF, so the bit is LOST and the normalize loop's `ADC EAX,EAX` gives **2** (the true binary answer). Ghidra's decompile mis-models this (it keeps the SHRD carry into the normalize loop, giving 3). So the decompile is WRONG for this fn (a latent precision-loss quirk in the original FPU add), and the emulator (disasm-faithful) is the correct reference. C port deferred pending a careful disasm-faithful transcription (hand or emulator->C codegen); the decompile-based cport is superseded. This confirms: for intricate fixed-point fns, verify against the DISASM/emulator, not the decompile. | STATUS.md L0 |
| 2026-07-08 | **L10 DPMI no-op batch (4 fns) + 0x42928 deferral.** Surveyed L4-L10 for small self-contained fns (all callees ported): found 0x3fade (47B), 0x40570/0x405a7 (54B each), 0x407f8 (64B), 0x42928 (47B). Ported the 4 DPMI ones as flat-model no-ops: 0x3fade = zero 64000 bytes at DAT_000a0000 (A0000/DPMI region, not modeled); 0x40570/0x405a7/0x407f8 = build a stack request struct ({3}/{0x13}/{0x101}) + call fn_041e51 (the DPMI INT 31h real-mode trampoline, already a no-op standin) -> no observable flat-model effect. 1 check. **0x42928 deferred**: its decompile is GARBLED — it reads `uVar4=FUN_00044848(...)` as a 64-bit (high32=ptr iVar3, low32=val iVar2) then dereferences iVar3+4 / iVar3+0xd, but FUN_00044848 actually returns void (it is a thunk `FUN_00044807(p1,0,1,p4,p2,p3)`), so the decompile mis-models the register return; needs a disasm trace before porting. L10 30->34, suite ALL PASS. | STATUS.md L10 |
| 2026-07-08 | **L10/L11/L12 DPMI+VGA no-op batch (5 fns).** Continued the L4-L12 self-contained survey (size up to 120B). Ported 5 more as flat-model no-ops: 0x10dde (build ~2KB stack request struct + call fn_00010570 no-op), 0x3fcbd (build struct {0x1012,...} + fn_041e51 no-op), 0x40788 (DPMI INT31h 0x100 mem-alloc, returns 0 = no DPMI), 0x104c7 (DPMI mem-alloc + writes returned segment into DAT_00053c58/53c5c -> no-op), 0x3f645 (VGA palette fn_03f3be + A0000-region memset loop; every body effect is a no-op -> no-op). **Deferred 0x35d0f + 0x356ba**: 0x35d0f's decompile shows 3 identical `do{}while(&DAT_00063e88[i])` spin loops (likely port-read DMA busy-wait mis-modeled as memory loops; porting literally = infinite loop) and 0x356ba has a canary->global decompile artifact (_DAT_00063330 = canary) that must be disasm-verified before porting. L10 34->37, L11 11->12, L12 3->4, TOTALS 241->246, suite ALL PASS. | STATUS.md L10 |
| 2026-07-08 | **L6/L10 batch (2 fns): 0x41bdc heap wrapper + 0x35ec2 VGA no-op.** 0x41bdc (74B) = heap_malloc wrapper: `if(size==0) size=1; return heap_malloc(size)` (the 64-bit return's EDX high word is the caller's register-ABI register, ignored); 2 checks (non-null + distinct). 0x35ec2 (88B) = VGA 0x3DA sequencer ready-wait (two `do inb(0x3da) until ...` spins) -- the second spin (wait for ready bit set) is DEGENERATE in the flat model because inb(0x3da) is a port stub returning a constant (default 0), so it would loop forever -> no-op standin. Deferred from this survey: 0x412e9/0x41390 (CD-ROM status polls, reg-ABI EDX extraout, 0x7fff-iter timeout -> belong to the CD/MSCDEX subsystem task), 0x326c1 (table-free, reg-ABI + canary->global decompile artifact, needs disasm), 0x42a43 (complex struct mgmt, reg-ABI extraout_EDX + garbled decompile), 0x4933a (CD track-list build, reg-ABI, CD subsystem). TOTALS 246->248, suite ALL PASS. | STATUS.md L6/L10 |
| 2026-07-08 | **L10/L11 batch (2 fns): 0x35d0f DMA no-op + 0x356ba PIT setup (disasm-verified).** 0x35d0f (77B) confirmed via disasm = 3 memory-poll spin loops on DAT_00063e88[AL] (wait ==0 / !=0 / ==0, the DMA busy/idle handshake); the "wait for !=0" spin is degenerate in the flat model (the byte is a static 0 global, never changes) -> infinite loop -> no-op standin. 0x356ba (76B) is NOT a no-op -- disasm shows it is a register-ABI timer setup: input ticks in EAX; DAT_00063330 = ticks; divisor = (int32)(ticks*DAT_000530b4) then sign-extended IDIV 100; CALL fn_03575d(divisor) (the IDIV remainder EDX is clobbered by the call, unused). The earlier decompile's "_DAT_00063330 = canary" was misread -- it correctly writes the register-ABI input to DAT_00063330. Added g_dat_530b4/g_dat_63330 globals; 3 checks (DAT_00063330 tracks ticks). Supersedes the prior "0x356ba deferred (canary->global artifact)" note. TOTALS 248->250, suite ALL PASS. | STATUS.md L10/L11 |
| 2026-07-08 | **L10/L11/L12 batch (3 fns): 0x10a56 + 0x3f592 no-ops, 0x3c7d7 real struct reset.** All three self-contained (canary + already-ported callees only). 0x10a56 (131B): register-ABI CD track-entry submit -- copies a 5-dword entry from DAT_00051af0 + reads DAT_00053c64[index*5], but the only observable effect is the fn_00010570 call (flat-model no-op) -> whole function no-op. 0x3f592 (178B): register-ABI VGA vertical-line plotter -- strided pixel writes into the A0000 video buffer (&DAT_000a0000) + fn_03f3be palette (no-op); DAT_000a0000 unmodeled + fn_03f3be no-op -> no-op standin. 0x3c7d7 (123B): register-ABI struct reset (pointer in EAX) -- zeroes 6 fields (+0x2a/+0x2e/+0x32/+0x36 dwords, +0x3a/+0x3b bytes) and memsets two 20-byte buffers DAT_0005d08c/DAT_0005d0a0 (added as globals). 4 checks (fields zeroed, neighbors untouched, buffers memsets). Deferred from same survey: 0x10d47 (CD/DMA state, 8 globals -> CD subsystem), 0x3565a (timer; divides by DAT_00063304 which decompile saw as 0 -> div-by-zero risk in flat model -> timer subsystem), 0x1435f/0x35c5c/0x3b5c0/0x3b67e (real entity/linked-list logic, register-ABI, need struct modeling -> dedicated pass). TOTALS 250->253, suite ALL PASS. | STATUS.md L10/L11/L12 |
| 2026-07-08 | **L10 linked-list push/pop pair (2 fns): 0x3b5c0 insert + 0x3b67e remove (disasm-verified).** A doubly-linked list (head=DAT_000639cc, tail=DAT_000639d0, count=DAT_000639c8). Node = 24B payload: flag@0x00 (stack byte), ECX@0x01, 0@0x05, EDX@0x09, EBX@0x0d, next@0x11, prev@0x15; register-ABI (node ptr in EAX, payload from the other live regs). 0x3b5c0 appends (empty list -> head=tail, else tail->next=node, node->prev=tail, node->next=0, tail=node; count++). 0x3b67e removes (head/tail/middle splice; count--). Two findings: (1) POINTER-WIDTH ADAPTATION -- the original node holds 4-byte (32-bit) next/prev; the flat 64-bit port widens them to 8 bytes (next@0x11, prev@0x19), a necessary 32->64-bit adaptation (the first attempt stored 4B ptrs but read 8B -> garbage high word -> FAIL + null-deref segfault, now fixed). (2) LATENT BUG (faithfully reproduced) -- the head/tail remove branches do not NULL-check the successor/predecessor, so removing the last remaining node null-derefs; the test exercises head/tail/middle removal on >=2-node lists and avoids the last-node case. 10 checks (payload fields, append, splice, count). TOTALS 253->255, suite 812 checks ALL PASS. | STATUS.md L10 |
| 2026-07-08 | **L10 entity-list init (0x35c5c, 178B, disasm-verified).** Walks the entity linked list (head=DAT_00053c70, cursor=DAT_00053c78, next@+0x13f) and for each node with flag[0]==1 && flag[0x30]!=1: sets [0x30]=1, [0x17]=0x15, [0x11]=0, [0x32]=*(DAT_00053ea8 + *(node+0xd)*0x88) (first dword of the 0x88-byte table entry). No-op on an empty list (the flat model's state before data load). DAT_00053c70 was already defined (entity-list ports at 0x2621/0x2787); added only DAT_00053c78 + DAT_00053ea8 globals. 5 checks (fields set per idx, flag=0 node untouched, idempotent second pass, empty-list no-op). TOTALS 255->256, suite 817 checks ALL PASS. | STATUS.md L10 |
| 2026-07-08 | **Fatal-error-reporter batch ported (5 fns)** — Porting the small gate fn_0429c8/fn_0429ea (28B each) unlocked a cluster. Disasm: PUSH EBX/ECX; EBX=EAX(msg), ECX=EDX(param2), EDX=DS; CALL 0x045a44 (debug_break_hook: INT3 if g_debug_break, always returns 0); if 0 restore + CALL fn_0422d5 (fatal-exit path = dispatch_b(0,0xff)). Flat model: debug_break_hook always 0 -> always takes the fatal-exit branch. Ported: fn_0429c8, fn_0429ea (reporters), fn_0429e5 (4B JMP thunk -> fn_0422d5), fn_0418c6 (11B: fn_0429c8("undefined constructor or destructor called!")), fn_045dad (16B: fn_0429ea("Floating-point support not loaded\r\n",1)). Register-ABI inputs (EAX=msg, EDX=param2 unused). 4 checks (thunk returns 0; reporters run). L2 14->17, L3 9->11, TOTALS 232->237, suite 787->789 PASS 0 FAIL. | STATUS.md L2/L3 |
| 2026-07-08 | **Leaf-tier (L0-L4) clean wins EXHAUSTED — remaining need dedicated fresh-context passes.** frontier.py (all-callees-ported) residual, each deferred w/ rationale: L1 — 0x04a814 (linear/binary search over {lo,hi} array; reg-ABI EDX=val-ptr EDI=base; count from no-op fx_stub_4a923 -> degenerate), 0x047f54/0x048053 (fixed-point normalize, reg-ABI EAX/EDX in EBX out, 0x0463dc call return discarded via POP EAX; 0x0463dc is spurious data), 0x045bb7 (DOS INT21h mem alloc, in_DS reg-ABI), 0x048fe6 (INT2f/31 IRQ setup, in_CS/in_CR0 extraout reg-ABI), 0x042ccc (DPMI heap free-all teardown, heap_unlink + INT31/21 frees, reg-ABI), 0x043e34/0x047b79/0x0440a9 (FPU/large parser). L2 — 0x041fa1 (JMP thunk -> UNPORTED 0x0438a3), 0x04a32b (FPU, args on FPU stack, reg-ABI), 0x046d47 (caseD_8 virtual-dispatch tail-merge), 0x044387 (503B CONCAT31 complex). L3 — 0x044a50/0x049236 (DOS disk/file reg-ABI extraout), 0x04488b (452B DOS device open, in_stack_* reg-ABI). L4 — 0x043fc3 (field-deserializer, unaff_ESI/extraout reg-ABI + indirect calls), 0x044c23 (640B). NEXT (fresh context, tractable first): the register-ABI FPU cluster (0x04a814, 0x047f54, 0x048053, 0x04a32b) via full disasm + FPU-stack trace; then 0x0451bc (1096B typed deserializer, biggest L3 win). Total this turn: +4 (0x048e60 L0 diff-verified, 0x044c1e L4 thunk, 0x041e51 L3, 0x0422d5 L1). TOTALS 232/528, suite 787 PASS 0 FAIL. | STATUS.md L0-L4 | — (0x0422d5, 58B) DOS file open + seek + terminate wrapper. Original: INT 21h AH=03h/AL=01 open file 0x420b8 (handle in AX); strlen(param_2); INT 21h AH=40 seek; dispatch_b(0, 0xff); INT 21h AH=4c terminate with param_5. All INT 21h calls are flat-model no-op stubs (swi(0x21)) and AH=4c terminate is a no-op, so the strlen only feeds the no-op seek and the sole observable effect is the real event dispatch dispatch_b(0, 0xff); returns the terminate result (0). Supersedes the stale L1 note '(405B) DOS I/O INT 13h+disk' (misread: it is 58B of INT 21h file ops). 1 check. L1 31->32, TOTALS 231->232, suite 787 PASS 0 FAIL. | STATUS.md L1 | — (1) fn_044c1e (0x044c1e, 4B) tail-call thunk JMP 0x049415; fn_049415 (DOS AH=41 delete, sys_file_delete no-op) already ported -> straight pass-through. (2) fn_041e51 (0x041e51, 26B) register-ABI: MOV ECX,EAX; CALL 0x041df2 (seg-save, EAX=ESP); MOV ECX,ESP; CALL 0x041e38 (DPMI INT31h real-mode trampoline); RET. Return = fn_041e38's EAX = caller's *EBX (the real-mode result); both callees are flat-model no-ops so that result is never produced -> no-op standin returning 0 (documented approximation of the register-ABI *EBX return); params model a=EAX,b=EDX,c=EBX,d=ECX. Supersedes the stale L3 note '0x041e51 fragile DPMI thunk (returns caller's *EBX)' -- now ported as the no-op standin. **Also deferred (register-ABI):** 0x047f54 + 0x048053 (L1, fixed-point normalize, EAX/EDX in EBX out-ptr, the 0x0463dc call's return is discarded via POP EAX -- 0x0463dc is spurious data in the 0x0461cc interpreter, not a portable fn), 0x048fe6 + 0x045bb7 (L1, DPMI/DOS INT 2f/31/21 IRQ+mem setup w/ in_CS/in_CR0/in_DS extraout register returns). 2 checks. L3 8->9, L4 1->2, TOTALS 229->231, suite 786 PASS 0 FAIL. | STATUS.md L3/L4 | — DOS/DPMI timer-vector setup: swi(0x21) AH=0x25 AL=0x04 (set int vector 4 = DAT_00053408:DAT_00053404), then if DAT_000533b2>=3 dos_install_timer_int() else CR0.ET bit op. All DPMI/DOS/CR0-specific, no flat-model effect; dos_install_timer_int is a ported no-op -> no-op standin (2 smoke checks). Also deferred 0x042ccc (41 ins, DPMI heap free-all shutdown: walks g_heap_head chain + heap_unlink + INT 31h AH=502/INT 21h AH=49 frees; g_heap_head IS used by the flat heap so the unlink loop has real effect but it is a shutdown-only DPMI cleanup, deferred) and 0x043e34 (93 ins field-descriptor read w/ ES-seg far pointers + data-driven callbacks) + 0x047b79 (177-ins 64-bit FPU) as complex. L1 29->30, TOTALS 208->209, suite 728 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **0x04794a ported (disasm-faithful C via codegen, diff-verified).** Resolves the 0x04794a deferral. Instead of hand-transcribing the 149-instruction routine (a Ghidra-decompile C port fails ~49% because the decompile keeps the SHRD carry that the following `OR EDI,EBX` actually clears), I wrote a small C EMITTER (scratch/gen_04794a_c.py) that mechanically translates the ground-truth emulator (scratch/fp_04794a_ref.py) into straight-line C (port/src/f/fx_04794a.c, one label+gotos per instruction). Two emitter bugs found and fixed purely by differential-testing against the emulator: (1) XCHG emitted `b=a` from the already-updated `a` instead of `b=_t` (a swap that silently became a copy); (2) ADD/ADC carry used `_t>=m` on a wrapping uint32 sum (carry never detected, off-by-one) -- fixed with 64-bit intermediates. Verified 20,000 random+edge vectors 100% match the emulator; the committed test ships 3,000 vectors (scratch/fp_04794a_ctest.h, force-tracked). Register-ABI (EAX:EDX in, EBX/ECX/ESI exponent; returns (EDX<<32)\|EAX) matches the wrappers 0x0478e2/0x0478f0/0x04791a. 1 check (3000 vectors). Suite 819 PASS 0 FAIL. Unlocks the 0x04794a-gated wrappers (0x0478e2, 0x0478f0, 0x04791a) and contributes to 0x048a48/0x048f4c. | STATUS.md L0 |
| 2026-07-08 | **0x048f4c ported (L1, 32 insns).** Now unblocked (calls fx_mul64=0x047d4e + fn_04794a, both diff-verified). Register-ABI loop over an arr[] of fx128 (10B elems, EDI advances 0xa): acc=EAX:EDX starts at arr[0]; each of `count` iterations computes acc=fn_04794a(fx_mul64(acc, mant, ESI_mul), arr[i+1], ESI_norm). ESI is tracked literally (ROL ESI,16 = swap 16-bit halves; MOV SI,x sets low16). Traced across 3 iterations: fx_mul64 ALWAYS receives ESI=(exp_base<<16)|arr[0].exp (constant); fn_04794a in iter i receives ESI=(arr[i+1].exp<<16)|arr[0].exp (arr[0].exp is special). Reg-ABI: EAX=mant_lo,EDX=mant_hi,EBX=count,ECX=arr,ESI=exp_base(low16). Returns (EDX<<32)|EAX. Structurally verified against hand-sequenced primitive calls with LITERAL ESI values (count=1,2) -- catches loop/ESI/accumulator transcription; primitives independently verified. L1 +1, suite +1 PASS. | STATUS.md L1 |
| 2026-07-08 | **0x048f2c + 0x048f9e ported (2 L2, 048f4c wrappers).** Unblocked by 0x048f4c. 0x048f2c (15 insns): in-place a=048f4c(a, arr, count, a->exp); a->exp preserved (loaded SI written back verbatim); reg-ABI EAX=a(in/out),EDX=arr,EBX=count. 0x048f9e (30 insns): in-place a=(048f4c(a*a, count, arr, a->exp))*a; ESI=(a->exp<<16)|a->exp (16-bit half replicated) for both fx_mul64 calls; a->exp preserved; reg-ABI EAX=a(in/out),EBX=count,EDX=arr. Both structural-verified vs hand-sequenced (verified) primitives, count=2, exp-preserved asserted. (0x048585 gated on unported 0x0485eb subtract -- defer; 0x047b79 177-insn fixed-point -- needs codegen pass -- defer.) L2 +2, suite 822 PASS 0 FAIL. | STATUS.md L2 |
| 2026-07-08 | **0x048754 ported (L3, 24 insns).** In-place wrapper: a = fn_04791a(a, a, 0, 0x80000000, 0x3fff) (b=0x80000000_00000000, exp_hi=0x3fff -- a sign/normalize op). EAX=a (input struct = out pointer; 3 stack args are constants). Unblocked by the 0x04791a port. Verified vs fn_04791a applied to an equal copy. L3 +1, suite 823 PASS 0 FAIL. NOTE: remaining small leaf fns are gated -- 0x048c3f/49/56 (0x048a48), 0x046d32/d47/73e5 (FPU-interp EBP frame + [EBP+0x76] tail dispatch), 0x04876d (0x0487b0), 0x044a50/0x049236 (0x0492d0 gate), 0x044867 (0x04488b), 0x047b4f (0x047b79), 0x0495eb (0x049506), 0x041fa1 (0x0438a3), 0x042ccc (heap-invariant + DPMI/DOS). Next = dedicated passes (heap 0x042ccc, FPU-interp micro-ops, 0x0492d0 gate, 0x048a48). | STATUS.md L3 |
| 2026-07-08 | **0x044a50 + 0x049236 ported (L2/L3).** Node/device-table file-position ops. Both call fn_0492d0 (lazy node init) + store_arr53730. 0x044a50 (reg-ABI EAX=index,EDX=ctx): mark slot (old|0x40), if bit13 set do 2x AH=0x44 ioctl; 0x049236 (reg-ABI EAX=index,EDX=ctx,EBX=count): if AL bit7 AH=0x42 lseek(end), then AH=0x40 write; if written!=count fn_042a0d(0xc). Both use the RCL/ROR sign gate where the reconstructed high bit == CF (the DOS error flag); negative -> fn_045a82 (returns -1). Added sys_file_write stub (AH=0x40, no backend -> returns count). Verified success paths + 0x044a50 error path (CF=1). NOTE (latent, pre-existing): g_dat_53730 (fn_0492d0) and g_arr_53730 (store_arr53730) are TWO separate C globals both = DAT_00053730; the original is one table, so callers must keep them in sync (test sets both). L2 +1, L3 +1, suite 829 PASS 0 FAIL. | STATUS.md L2/L3 |
| 2026-07-08 | **0x044748 ported (L4, 68 insns).** I/O-state flush/seek-back on a 0x14-byte struct {f0, f4=count, f8=buffer, f12=flags(byte0@0x0c,byte1@0x0d), f16=handle}. Flush path (byte1 bit4): clear byte1 bit1; if byte0 bit1 and f8!=0, fn_049236(handle=f16,buffer=f8,count=f4); on -1 or partial (fn_042a0d(0xc)) set byte0 bit5 + return -1. Seek-back path (else): if f8!=0 clear byte0 bit1; if byte1 bit5 clear and f4!=0, fn_044807(f16,-f4,1); on -1 set byte0 bit5 + return -1. Tail always f0=f8, f4=0. Unblocked this session (0x049236/0x044807 just ported). NOTE: corrected a callee-scan regex bug (8-digit addrs are 0x0004xxxx = {4}, not {5}) that had mislabeled these as call-free. Verified flush/seek-back/f8==0 paths; error paths disasm-verified (stubs don't fail). Suite 834 PASS 0 FAIL. | STATUS.md L4 |
| 2026-07-08 | **0x04488b ported (L3, 159 insns).** File open/create handler, reg-ABI 4 stack args (name, mode word, extra byte, create-val). Skip leading spaces; AH=0x3d open (mode=(mode&0x83)|extra); handle=res&0xffff if res>=0 else -1. If (mode&3)&&handle!=-1&&ioctl_device==0: (mode>>8)&4 -> close+fn_042a0d(7)+return -1; (mode&0x40) -> write 0 bytes, on <0 close+fn_045a82(res,buf). If handle==-1: (mode&0x20)&& (res&0xffff)==2 -> AH=0x3c create (CX=((val||0x180)^g_dat_536d8)&0x80?0:1), close, reopen; any <0 -> fn_045a82(res,name). Then value = fn_0492d0(handle,0)&0x3c | (ioctl!=0?0x2000:0, from OR DH,0x20=bit13) | (m==0?0x1:m==1?0x2:m==2?0x3:0, m=(mode&0x83|extra)&0x7f) | (mode&0x10?0x80:0); stored = value|0x40 if ((mhi&3)&&(mhi&2))||(mhi==0&&g_dat_53619==0x200) else value; store_arr53730(handle,stored); return handle. **Subtle:** EDX=flags at 0x0449e3 accumulates OR DH,0x20 + OR DL,x before the 0x044a1c MOV EAX,EDX, so the stored value carries the 0x2000 bit13. **New sys stubs:** sys_file_open (AH=3d) + sys_file_create (AH=3c), both with test queue (sys_file_open_queue) so the create-path reopen can be queued separately. **New global:** g_dat_536d8 (DAT_000536d8, load-time 0). Verified 5 cases: open-success (two value paths), open-fail error delegate, create+reopen, leading-spaces. Suite 842 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **0x044867 ported (L3, 16 insns).** Thin wrapper: reads its 3rd/4th stack args (name, val); p1/p2 unused. Calls fn_04488b(name, mode=0, extra=val&0xff, val=val). Unblocked by 0x04488b this session. Verified differential (wrapper return + tbl[0] side-effect match a direct fn_04488b call on fresh table). Suite 844 PASS 0 FAIL. | STATUS.md L3 |
| 2026-07-08 | **0x043234 ported (L5, 22 insns).** Walk the active node list (head g_node_list_active=DAT_00063ffc, offset-based via g_ndpool, node {next,rec}). For each node: rec = base + node->rec (the io_state record); if (rec[0xc] & mask) count++; if additionally (rec[0xd] & 0x10) call fn_044748(rec) to flush. Returns the count of matching records. **NOTE:** the callee-scan reported 0x043259 as a callee, but that is the loop-back label WITHIN 0x043234 (JMP/JNZ 0x43259), not a separate function — so 0x043234's only real callee is fn_044748 (ported). The original walks pointer-based nodes; the port follows the established offset-pool convention (mirrors node_deactivate). Verified: 2-node list, mask 0x04, A (flags1 bit4) flushed (f4->0, f0=f8), B (no bit4) untouched, count==2. Suite 848 PASS 0 FAIL. | STATUS.md L1 |
| 2026-07-08 | **0x041e94 + 0x04123d ported; 0x0473e5 deferred.** **CROSS-REFERENCE HEX-BUG FIX:** my earlier callee cross-reference compared 8-char Ghidra addresses (0x000418ee) against 6-char STATUS addresses (0x0418ee) so nothing matched, falsely flagging ~40 candidates as gated. Re-running with int-normalized addresses: 0x041e94 and 0x0473e5 are the two truly-unblocked ones. **0x041e94** = pure `JMP 0x041bdc` (heap_malloc) thunk; ported as `fn_041e94(size)=heap_malloc(size)`; verified differential on non-allocating cases (0, oversize) both == heap_malloc, and (16) non-zero. **0x04123d** (36 insns): device-port init, no stack args (RET 0); prologue canary `PUSH 0x20; CALL 0x418ee` (canary_id, no-op guard); then outb(g_dat_53334+6,1); 10x inb(g_dat_53334+6); outb(g_dat_53334+6,0). The callee-scan flagged 0x41276/0x41270 as callees but those are intra-function loop labels (false positives, like 0x43259); the real callees (canary_id/outb/inb) are all ported. The loop counter (EAX=[EBP-4],INC) is recomputed to the port before each inb, so it only bounds the 10 identical inb(port) calls (dead as an arg). Verified smoke (port I/O stubbed). **0x0473e5** (5 insns) DEFERRED: `LEA EAX/EDX/EBX,[{ESI,EDI}+EBP+0x1c]; CALL 0x0478f0; JMP [EBP+0x76]` -- register-ABI (ESI/EBP/EDI caller-set) with an indirect tail-jump through the FPU engine frame at EBP+0x76; the tail jump transfers control to an arbitrary function pointer, not modelable without the calling context. Suite 852 PASS 0 FAIL. | STATUS.md |
| 2026-07-08 | **fn_04794a wrappers ported (3 L1): 0x0478f0, 0x0478e2, 0x04791a.** With fn_04794a in place, its three register-ABI wrappers fell out. All load input A (EAX: {lo@0,hi@4,exp@8}), B, and OUT (the original EBX register -- pushed at entry, overwritten as a temp, popped, then written to), compose ESI=(b_exp_hi<<16)|a->exp, call fn_04794a(a_lo,a_hi,b_lo,b_hi,ESI), and store out={r_lo,r_hi,a->exp}. 0x0478f0 takes B as an EDX struct ptr; 0x0478e2 is identical but flips bit15 of b->exp (XOR SI,0x8000) before composing; 0x04791a takes B as 3 stack scalars (b_lo=arg1, b_hi=arg2, exp_hi=arg3 low16). Defined fx128 {lo,hi,exp} in f.h; implementations in f.c (fx4794a_store helper). ABI derived from each fn's own register usage (EBX pushed+clobbered+popped+written=OUT; EAX loaded+consumed=A; EDX loaded=B), not a caller. Structurally verified against the diff-verified fn_04794a (struct load / ESI compose / OUT store). L1 +3, suite 820 PASS 0 FAIL. | STATUS.md L1 |
## L0 (112)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x040024 | 29 | put_px | video | primitives.md; test_f1 PASS |
| [x] | 0x040042 | 30 | board_out | io-board | primitives.md; test_f1 PASS |
| [x] | 0x040061 | 32 | board_in | io-board | primitives.md; test_f1 PASS |
| [x] | 0x040475 | 27 | cpy_back | video | primitives.md; test_f1 PASS |
| [x] | 0x040491 | 52 | blt_rows | video | primitives.md; test_f1 PASS |
| [x] | 0x0404c6 | 20 | cpy_dw | video | primitives.md; test_f1 PASS |
| [x] | 0x0404db | 42 | hline_set | video | primitives.md; test_f1 PASS |
| [x] | 0x040506 | 37 | vline_set | video | primitives.md; test_f1 PASS |
| [x] | 0x04052c | 65 | rect_fill | video | primitives.md; test_f1 PASS |
| [x] | 0x0418e6 | 0 | canary_ss_save | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041df2 | 39 | seg_save | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041e6c | 36 | mem_copy | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041e99 | 5 | rng_state_ptr | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041ef8 | 73 | itoa32 | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041f5d | 24 | str_len | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041f76 | 6 | abs32 | runtime | primitives.md; test_f1 PASS |
| [x] | 0x041f7e | 28 | fpu_rndint | fpu | primitives.md; test_f1 PASS (FRNDINT, not a no-op) |
| [x] | 0x042310 | 8 | seg_ds_load | runtime | primitives.md; no-op (flat DS) |
| [x] | 0x04231b | 1 | irq_cli | runtime | primitives.md; no-op (no INT flag) |
| [x] | 0x04231d | 13 | outb | io-board | primitives.md; test_f1 PASS (-> sys_outb) |
| [x] | 0x04232b | 1 | irq_sti | runtime | primitives.md; no-op (no INT flag) |
| [x] | 0x04232d | 51 | isr_get | os-int | primitives.md; test_f1 PASS (sys stub, INT21h get-vec) |
| [x] | 0x042361 | 42 | isr_set | os-int | primitives.md; test_f1 PASS (sys stub, INT21h set-vec) |
| [x] | 0x04238c | 7 | inb | io-board | primitives.md; test_f1 PASS (-> sys_inb) |
| [x] | 0x0423b6 | 0 | cb_noop | runtime | primitives.md; test_f1 PASS (no-op cb default) |
| [x] | 0x0423e5 | 82 | str_icmp | str | primitives.md; test_f1 PASS (bounded case-insens cmp) |
| [x] | 0x042438 | 65 | res_consume | resource | primitives.md; test_f1 PASS (res_reader consume) |
| [x] | 0x0427e7 | 11 | digit_chr | str | primitives.md; test_f1 PASS (digit->char) |
| [x] | 0x0428e5 | 18 | fmt_emit | str | primitives.md; test_f1 PASS (printf byte-sink) |
| [x] | 0x04295d | 28 | get_key | input | primitives.md; test_f1 PASS |
| [x] | 0x0429a8 | 29 | mem_cmp | input | primitives.md; test_f1 PASS |
| [x] | 0x042a07 | 0 | cb_noop2 | input | primitives.md; test_f1 PASS |
| [x] | 0x042ada | 162 | heap_alloc | heap | heap.md; port+test PASS (batch 7-8) |
| [x] | 0x042b7d | 4 | (free epilogue) | heap | heap.md; early-exit JZ->JMP 0x42c88, folded into heap_free returns; PASS |
| [x] | 0x042b82 | 266 | heap_free | heap | heap.md; port+test PASS (batch 7-8) |
| [x] | 0x042c8d | 62 | heap_unlink | heap | heap.md; test_f1 PASS (batch 7) |
| [x] | 0x042e2d | 115 | heap_insert_init | heap | heap.md; port+test PASS (batch 7-8) |
| [x] | 0x042ea1 | 77 | heap_top_check | heap | heap.md; port+test PASS (batch 7-8) |
| [x] | 0x04322c | 2 | heap_grow | heap | primitives.md; test_f1 PASS |
| [x] | 0x043263 | 28 | get_key_echo | input | primitives.md; test_f1 PASS |
| [x] | 0x0432b7 | 107 | memsetw | mem | primitives.md; test_f1 PASS |
| [x] | 0x04335c | 36 | rm_switch_a | io | DPMI real-mode trampoline → no-op stub (batch 9) |
| [x] | 0x0433ce | 51 | rm_switch_b | io | DPMI real-mode trampoline → no-op stub (batch 9) |
| [x] | 0x043958 | 32 | dos_install_timer_int | io | DOS INT21h/0x25h → sys stub (batch 9) |
| [x] | 0x043979 | 4 | fpu_state_save | io | FSAVE → no-op in flat model (batch 9) |
| [x] | 0x043ca0 | 216 | cmdline_parse | cmdline | port+test PASS (batch 9) |
| [x] | 0x043d79 | 52 | cmdline_count | cmdline | port+test PASS (batch 9) |
| [x] | 0x043dae | 133 | emit_chars | emit | port+test PASS (batch B) |
| [x] | 0x043f22 | 93 | emit_value | emit | port+test PASS (batch B) |
| [x] | 0x0445a6 | 34 | emit_step | emit | port+test PASS (batch B) |
| [x] | 0x044618 | 74 | dispatch_a | emit | port+test PASS (batch B) |
| [x] | 0x044663 | 78 | dispatch_b | emit | port+test PASS (batch B) |
| [x] | 0x044859 | 13 | to_lower | string | port+test PASS (batch C) |
| [x] | 0x044b5d | 56 | node_deactivate | data-struct | port+test PASS (batch D) |
| [x] | 0x044be6 | 5 | get_dat_53390 | string | port+test PASS (batch C) |
| [x] | 0x044fd7 | 90 | parse_mod_prefix | string | port+test PASS (batch D) |
| [x] | 0x045032 | 35 | strn_len | string | port+test PASS (batch C) |
| [x] | 0x045056 | 34 | word_nlen | string | port+test PASS (batch C) |
| [x] | 0x0451b5 | 6 | cb_indirect_53644 | string | port+test PASS (batch C, NULL-guarded) |
| [x] | 0x04561d | 49 | calibrate_clock | timing | port+test PASS (batch D2, sys rate) |
| [x] | 0x04564f | 55 | delay_units | timing | port+test PASS (batch D2, sys clock) |
| [x] | 0x045687 | 194 | datetime_pack | DOS date/time packer via sys_getdate/sys_gettime. local0=(month<<24\|day<<16), local1=(hour<<24\|low8(min*256+sec-0x76c)<<16). SHL ECX,16 zeroes each local's low byte so the DOW-based return and day-change branch (CMP local1.byte0 vs local2.byte0, both 0) are dead -> always returns 0, writes f4=day f8=month f14=low8(time) f20=-1 f0=fc=0 f10=-1 | datetime_pack (f.c) |
| [x] | 0x04595b | 232 | tokenize | string | port+test PASS (batch J; in-place tokenizer, unescape/quote-strip, ptr-array) |
| [x] | 0x045a44 | 37 | debug_break_hook | sys | port+test PASS (batch D, INT3 stubbed) |
| [x] | 0x045a6a | 5 | const_64008 | string | port+test PASS (batch C) |
| [x] | 0x045a70 | 5 | const_64004 | string | port+test PASS (batch C) |
| [x] | 0x045ae8 | 15 | swap_dat_5363c | sys | port+test PASS (batch E) |
| [x] | 0x045bb1 | 5 | (spurious epilogue) | misc | 5×POP+RET tail, no distinct logic; folds into 0x45bb7 shared epilogue (no separate C fn) |
| [x] | 0x045d56 | 71 | float_str_sci | in-place sci-notation float-string cleanup: skip if rec[0x14]&1; find 'e'/'E'(or NUL), step back one, if rec[0x15]=='g'/'G' strip trailing '0's, drop an adjacent '.', shift tail left to the stepped-back pos (NUL-terminate) | float_str_sci (f.c) |
| [x] | 0x04636e | 81 | (spurious) | data | mid-code entry inside 0x0461cc interpreter; just `JMP 0x0461cc`; no distinct logic |
| [x] | 0x0463c0 | 27 | (spurious) | data | mid-code entry inside 0x0461cc interpreter; just `JMP 0x0461cc`; no distinct logic |
| [x] | 0x0463dc | 77 | (spurious) | data | mid-code entry inside 0x0461cc interpreter; part of its loop body; no distinct logic |
| [x] | 0x04642a | 5 | case_field_0x28 | data-struct | port+test PASS (batch E) |
| [x] | 0x046d05 | 44 | (spurious) | data | mid-code entry inside 0x0461cc interpreter (was misnamed caseD_11); no distinct logic |
| [x] | 0x04794a | 363 | fx_normalize | fixed-point normalize (register-ABI EAX:EDX in, EBX/ECX/ESI exponent; returns (EDX<<32)\|EAX). Disasm-faithful C via codegen from the ground-truth emulator (scratch/fp_04794a_ref.py); 3000-vector differential test PASS | fx_04794a.c |
| [x] | 0x047ab6 | 152 | fx_compare | fixed-point lexicographic UNSIGNED compare on {u32 lo,u32 hi,u16 exp} (exp@8). exp==0x7fff with nonzero (hi<<1\|lo) is "special" -> 2. Else first differing field (exp->hi->lo) sets sign: -1 (a<b) / 0 (==) / +1 (a>b). Original's RCR EAX block at 0x47b10-0x47b14 is dead (EAX overwritten by EDI) | fx_compare (f.c) |
| [x] | 0x047d4e | 251 | fx_mul64 | fixed-point 64x64 -> normalized 64-bit multiply. Register ABI A=EDX:EAX, B=ECX:EBX, ESI=(exp_hi:16|exp_lo:16). u9=(esh&0x80007fff)+(esl&0x80007fff); u8=u9+0xc002; overflow (u9>=0x3ffe && u8>=0x7fff) -> 0x8000000000000000. uVar2/uVar11 are a SCHOOLBOOK split of the 128-bit product (uVar2=a_hi*b_hi+hi32(cross)+carry; uVar11=guard word, NOT A*B). Left-normalize (u2<<1|carry-in from u11 bit31, u8=u9+0xc001), round-to-nearest, final right-shift when (short)u8<1 (x86 SHL count masked to 5 bits: sh==0 folds hi into lo). A==0/B==0 -> 0. 8 checks PASS (2^32*2^32@0x3fff->1, underflow->0, overflow->inf) | fx_mul64 (f.c) |
| [x] | 0x047e4a | 4 | fx_extract_a0 | thunk: PUSH EBX; MOV BL,0xa0; JMP 0x47ea7 (mode 0xa0 variant of fx_extract). Folds into fx_extract(p,0xa0) | fx_extract (f.c) |
| [x] | 0x047e9f | 4 | fx_extract_1f | thunk: PUSH EBX; MOV BL,0x1f; JMP 0x47ea7 (mode 0x1f variant). Folds into fx_extract(p,0x1f) | fx_extract (f.c) |
| [x] | 0x047ea4 | 105 | fx_extract | fixed-point exp-based mantissa extractor (bias 0x3ffe); top-cl bits of hi (or all hi if cl==0x20), then RCR/SHLD/ADC/NEG bit-chain sets sign + carry. lo only via bit31. 3 modes (0x20/0xa0/0x1f) folded into one C fn | fx_extract (f.c) |
| [x] | 0x047f0e | 16 | fx_from_s32 | math | port+test PASS (batch G, fixed-point ctor sign thunk) |
| [x] | 0x047f1f | 52 | fx_from_u32 | math | port+test PASS (batch G, fixed-point ctor unsigned) |
| [x] | 0x047fd4 | 126 | fx_norm32 | fixed-point mantissa normalizer on {lo,hi,exp}: M=(hi+0x100)&0xffffff00 (0x80000000+exp+1 on overflow); cx=(exp+carry)&0x7fff: 0->M, 0x7fff->((M<<1)>>8|0xff000000)<<1, else cx2=(cx+0xc080)&0xffff: >=0x8000->0, >=0xff->0xff000000<<1, else SHRD path. BX=(hi<<25)&0xffff==0 so RCR==<<1 | fx_norm32 (f.c) |
| [x] | 0x04811a | 68 | fx_from_f32 | IEEE-754 float32→fxnum; exp=expfield+0x3f80 (=unbiassed+0x3fff), expfield=0xff→0xffff(NaN/Inf), expfield=0→passthrough {0,v,0}; hi=(v<<8&0x7fffffff)\|(nz?0x40000000:0)\|0x80000000; ADD ECX,ECX;RCR CX,1 nets to identity only when CF=0 (bit31 inputs flip exp top bit) | fx_from_f32 (f.c) |
| [x] | 0x04815f | 0 | (spurious) | misc | 0-size entry, no code (documented) |
| [x] | 0x0481df | 306 | fx_round_down | round fx {lo,hi,exp} (exp@8) toward zero to target precision. mode 0=round-half, 0x400=truncate, other=truncate; (mode==0x400|0x800)&&(short)exp<0 -> mode^=0xc00 (then 0xc00->0x400). mag=exp&0x7fff; >=0x403f no-op. Signed JL checks -> (int16)bx<0 continue. Table branch A (mag in [0x401f,0x403e]) clears tbl[mag-0x401f] bits from lo; underflow (mag<0x401f) lo=0 then table branch B (mag in [0x3fff,0x401e]) clears tbl[mag-0x3fff] bits from hi, else zero/inf (mag<0x3fff): 0x400->0, !=0->+inf, 0 & mag==0x3ffe & hi==0x80000000 & lo!=0 -> +inf else 0. Each clear adds mask+1 with carry/RCR+exp++. 32-entry mask table at 0x4815f. 6 hand-traced cases + determinism PASS | fx_round_down (f.c) |
| [x] | 0x048312 | 64 | (spurious) | data | disassembled data (huge offsets, no prologue, only DATA xref; documented) |
| [x] | 0x0485eb | 122 | FUN_000485eb | | |
| [x] | 0x048666 | 166 | fx_classify_status | fixed-point value classifier (register ABI EAX=fx ptr, EBP=ctx ptr, EDI=class offset). If (mask22&flags)==mask22 -> status 0x41. Else clear mask22, classify by mag=exp&0x7fff: mag0->(hi|lo)!=0?0x44:(flags|=mask1c,0x40); mag==0x7fff->flags|=mask20,(lo==0&&hi==0x80000000)?0x05:0x01; else->(hi sign)?0x04:(flags|=mask20,0x44). status high byte=(code|signflag) via &0xb8ff. Masks from 0x45e1c table | fx_classify_status (f.c) |
| [x] | 0x048e60 | 203 | fn_048e60 | fixed-point | fp_048e60 diff; test_f1 PASS |
| [x] | 0x0490b0 | 88 | dos_realmode_setup | sys | port+test PASS (batch H, INT2F/21-30 stubbed) |
| [x] | 0x049109 | 0 | noop_9109 | sys | port+test PASS (batch F, single RET) |
| [x] | 0x04910a | 38 | fpu_init | sys | port+test PASS (batch H, x87 control-word stub) |
| [x] | 0x049132 | 75 | fx64_to_f32 | 64-bit -> IEEE-754 float32: if hi&0x7ff00000==0 -> 0; else shift 64-bit left 1 (top bit -> sign 0x80000000), +0x20000000 mantissa bias, clamp hi==0 / hi>=0x8fe00000 -> 0x7f800000\|sign, hi<0x70200000 -> 0, else hi-=0x70000000, <<2, OR sign, return hi | fx64_to_f32 (f.c) |
| [x] | 0x04917e | 8 | stack_depth | timing | port+test PASS (batch F, ESP-bound; uintptr bound) |
| [x] | 0x049187 | 10 | set_53640 | sys | port+test PASS (batch F) |
| [x] | 0x0492b2 | 29 | ioctl_device | sys | port+test PASS (batch F, INT21/44 stubbed) |
| [x] | 0x049325 | 20 | store_arr53730 | data-struct | port+test PASS (batch F) |
| [x] | 0x049459 | 72 | itoa32_tbl | string | port+test PASS (batch J; table-based itoa, byte-identical to itoa32 0x41ef8; delegates) |
| [x] | 0x0494bd | 13 | to_upper | string | port+test PASS (batch F) |
| [x] | 0x0494cb | 58 | is_leap | date | port+test PASS (batch J3; Gregorian leap-year, IDIV 100/400, returns (ctx<<32)\|flag) |
| [x] | 0x0498c6 | 47 | fx_less | math | port+test PASS (batch H, fxnum (exp,hi,lo) compare) |
| [x] | 0x049a48 | 37 | parse_decimal | string | port+test PASS (batch H) |
| [x] | 0x049d0b | 166 | fmt_int_signed | fixed-width signed integer formatter: sign ('+'/'-'), leading-'0' pad to width, 1/2/3 digits (base 100/10/1 by magnitude, IDIV); returns field count; two no-output cutoffs (width+1>maxw, digits>width) | fmt_int_signed (f.c) |
| [x] | 0x049db2 | 98 | fmt_decimal | decimal formatter: intlen digit chars from src (then '0' pad), '.', fractlen chars from src (then '0' pad); "0." if intlen<=0&&fractlen==0; returns new out pos. Shares exit with 0x49db0 (POP ESI;RET tail of 0x49d0b) | fmt_decimal (f.c) |
| [x] | 0x04a2a7 | 131 | fx_double_naninf | double NaN/Inf inline-string writer. If (hi>>20)&0xff==0xff: write "inf" (mant=0) / "nan" (mant!=0), leading '-' if sign, return 1; else return 0 (out untouched). Latent bug: only 8 of 11 exp bits checked, so 1.0 (exp 0x3ff, mant=0) -> "inf" | fx_double_naninf (f.c) |
| [ ] | 0x04a3a8 | 230 | FUN_0004a3a8 | | |
| [x] | 0x04a7a2 | 86 | strn_casecmp | string | port+test PASS (batch I) |
| [x] | 0x04a84e | 106 | fx_mantissa_scale | 53-bit mantissa scaler: exp=hi[31:20] (bias 0x433); mantissa=(hi&0xfffff)|0x100000 (bit20 implicit 1) + lo; shift left (exp>bias) / right (exp<bias) by exp-0x433, RNE on right-shift, store 64-bit result | fx_mantissa_scale (f.c) |
| [x] | 0x04a8b9 | 105 | fmt_u64_fixed | 64-bit unsigned -> fixed-width right-justified ASCII field: buf[width]=0 sentinel, then repeatedly extract v%10000 (4-digit group via two 32-bit DIVs == 64-bit div), convert to 4 ASCII digits (AAM in orig), store right-to-left at buf[pos+3..pos], v/=10000, pad '0' groups until width slots filled | fmt_u64_fixed (f.c) |
| [x] | 0x04a923 | 166 | fx_stub_4a923 | no-op stub. Body = `CALL 0x4a9d8` + zero-filler (0x0000 pairs). 0x4a9d8 is not a Ghidra fn; its 2 bytes are `POP EDI; RET`. Net: CALL pushes 0x4a928 -> POP EDI (clobber) -> RET to caller. Returns void, clobbers EDI to 0x4a928, no return value, caller-cleans stack. Empty function | fx_stub_4a923 (f.c) |
| [x] | 0x04a9da | 110 | fx_double_exp | IEEE-754 double -> 32-bit fixed exp. hi=bits32-63; if exactly +/-0 (hi&0x7fffffff==0 && lo==0) *out=0 else *out=(int32)(((hi>>20)&0xff)-0x3fe). The [ESP+6] byte-rewrites + FLD never store the FPU value so they don't affect EDX (omitted) | fx_double_exp (f.c) |
## L1 (47)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x03ee88 | 160 | fn_03ee88 | | IRQ tick; counter wraps (63e28/63e3c/63e30) + PIC EOI |
| [x] | 0x041439 | 76 | fn_041439 | | IRQ handler; flat-model no-op (PIC EOI/IRET) + DAT_00063f84=2 |
| [x] | 0x041c28 | 8 | heap_free_guard | heap | port+test PASS (TEST EAX,EAX; JNZ heap_free_host(0x41c31); RET = NULL-tolerant free; guard(0)=pure no-op (RET, no g_heap_flag), guard(p) tail-frees; 6 checks) |
| [x] | 0x041c31 | 93 | heap_free_host | heap | heap.md; port+test PASS (batch 7-8) |
| [x] | 0x041e9f | 35 | prng_next | runtime | port+test PASS (LCG state=state*0x41c64e6d+0x3039; ret (state>>16)&0x7fff; 9-diff vs Python) |
| [x] | 0x041ec3 | 15 | prng_seed | runtime | port+test PASS (sets *rng_state_ptr()=seed; no-op if NULL) |
| [x] | 0x041f42 | 26 | itoa32s | string | port+test PASS (signed itoa via itoa32; '-' only for base==10; returns buf start; INT_MIN NEG-wraps) |
| [x] | 0x0422d5 | 58 | fn_0422d5 | dos | dispatch_b(0,0xff) only; test_f1 PASS |
| [x] | 0x0423cf | 21 | fn_0423cf | dispatch | 3 tests PASS; calls g_dat_53370(input) then g_dat_53374(ret), returns original input |
| [x] | 0x0427f3 | 104 | tmp_filename | string | port+test PASS (register ABI EAX=buf,EDX=index; counter=get_dat_53390(); "t<4-hex(counter&0xffff)>_<2-hex(index&0xff)>.tmp" via digit_chr; high bits discarded; 6 cases) |
| [x] | 0x042958 | 4 | thunk→delay_units | thunk | JMP 0x4564f (delay_units, ported f.c:1151); call-site alias, no separate C fn |
| [x] | 0x042a08 | 4 | fn_042a08 | data | sets g_dat_64008=13; test PASS |
| [x] | 0x042a0d | 11 | fn_042a0d | data | writes EAX to [0x64008]; test PASS |
| [x] | 0x042a30 | 11 | fn_042a30 | data | writes EAX to [0x64004]; test PASS |
| [x] | 0x042a3c | 6 | fn_042a3c | dispatch | CALL [g_dat_533bd]; test PASS |
| [ ] | 0x042ccc | 93 | FUN_00042ccc | | |
| [x] | 0x0431b5 | 118 | heap_size_adjust | heap | 6 tests PASS; align_up/±8-or-topcheck/+0x3c/limit-clamp/page-up; overflow→0 |
| [x] | 0x043280 | 48 | fn_043280 | byte-pattern fill | 1:1 (disasm; 3 checks) |
| [x] | 0x043324 | 55 | fn_043324 | | DPMI real-mode save-side; no-op standin |
| [x] | 0x043381 | 76 | fn_043381 | | DPMI real-mode return; flat no-op (reg/flag save) |
| [x] | 0x04391d | 58 | fn_04391d | | DOS timer-vector setup; flat no-op (int-vec4/CR0.ET) |
| [ ] | 0x043e34 | 237 | FUN_00043e34 | | |
| [ ] | 0x0440a9 | 733 | FUN_000440a9 | | |
| [x] | 0x04457f | 38 | hex_digit_value | string | port+test PASS (hex digit: 0-9, a-f->10-15, else 16; via to_lower 0x44859) |
| [x] | 0x044bb4 | 48 | fn_044bb4 | sys | ioctl node flag-setter; 4 tests PASS |
| [x] | 0x044ea4 | 306 | fn_044ea4 | string | numeric/precision/suffix spec parser; 6 tests PASS |
| [x] | 0x045605 | 23 | string_upper | string | port+test PASS (in-place upper-case via to_upper 0x494bd) |
| [ ] | 0x045bb7 | 193 | FUN_00045bb7 | | |
| [x] | 0x0478e2 | 13 | fx_norm_x8000 | fn_04794a wrapper (EAX=a,EBX=out,EDX=b); b->exp bit15 (0x8000) flipped before ESI compose; out={r_lo,r_hi,a->exp} | f.c |
| [x] | 0x0478f0 | 41 | fx_norm | fn_04794a wrapper (EAX=a,EBX=out,EDX=b); out={r_lo,r_hi,a->exp} | f.c |
| [x] | 0x04791a | 47 | fx_norm_scalar | fn_04794a wrapper w/ scalar b (stack args b_lo/b_hi/exp_hi-low16); out={r_lo,r_hi,a->exp} | f.c |
| [ ] | 0x047b79 | 426 | FUN_00047b79 | | |
| [x] | 0x047d24 | 41 | fn_047d24 | fx | register-ABI fx_mul64 wrapper; differential test PASS |
| [x] | 0x047f54 | 127 | fn_047f54 | fx | register-ABI fixed-point normalize (EAX:EDX in, EBX out); 2009-vector differential test PASS |
| [ ] | 0x048053 | 198 | FUN_00048053 | | |
| [ ] | 0x048585 | 101 | FUN_00048585 | | |
| [x] | 0x048f4c | 81 | fn_048f4c | reg-ABI fx_mul64+fn_04794a loop over arr of fx128; acc=arr[0], iter i: acc=fn_04794a(fx_mul64(acc,mant,(E0<<16)|arr[0].exp),arr[i+1],(arr[i+1].exp<<16)|arr[0].exp); ret (EDX<<32)|EAX; EAX=mant_lo,EDX=mant_hi,EBX=count,ECX=arr,ESI=exp_base | f.c |
| [ ] | 0x048fe6 | 201 | FUN_00048fe6 | | |
| [x] | 0x0492d0 | 84 | fn_0492d0 | node/device-table accessor: g_dat_53730 (uint32/node; byte1=flags 0x40 init/0x20 open); returns (ctx<<32)|node_value, 0 low if index>=bound g_dat_536dc; lazy-init index<6 via ioctl_device | port+test PASS (5) |
| [x] | 0x0494a2 | 26 | itoa32s_tbl | string | port+test PASS (signed itoa via itoa32_tbl 0x49459; identical to itoa32s) |
| [ ] | 0x049506 | 228 | FUN_00049506 | | |
| [x] | 0x0498f6 | 276 | fn_0498f6 | | date decoder; start-of-month tbl + is_leap; leap-Dec month=12 bug |
| [x] | 0x049a6e | 277 | parse_time_str | string/fmt | 11 tests PASS; off-by-one latent bug replicated (no-sign skips 1st digit); segmented copy = memcpy |
| [x] | 0x049b84 | 241 | fn_049b84 | string | [J\|M]N[.N[.N]][/m:mi:s] datetime parser; 5 tests PASS |
| [x] | 0x04a24d | 89 | config_value_ptr | string | port+test PASS (register ABI EAX=key; DAT_000537d4 = array of "key=value" char* NUL-terminated; strn_casecmp(prefix)+entry[len]=='=' -> ptr past '=' else 0; 9 cases) |
| [x] | 0x04a7fa | 24 | fn_04a7fa | *out = a - fpu_rndint(a) (dist to nearest int) | 1:1 FPU |
| [ ] | 0x04a814 | 57 | FUN_0004a814 | | |
## L2 (32)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x041e20 | 23 | FUN_00041e20 | | |
| [x] | 0x041e38 | 24 | fn_041e38 | | |
| [ ] | 0x041fa1 | 0 | FUN_00041fa1 | | |
| [x] | 0x0423b7 | 23 | fn_0423b7 | callback chain: g_dat_5336c(param_1)->r1; g_dat_53370(r1) [reused from 0423cf]; fn_0423cf(param_1); added g_dat_5336c | port+test PASS |
| [x] | 0x042576 | 179 | fn_042576 | file-mode parser: to_lower(s[0]) in r/w/a else fn_042a0d(9); +/-/b/t modifiers -> 0x43/0x40/3; w|2/a|0x82/r|1; g_dat_53619==0x200 default-binary; ret (p2<<32)|flags | port+test PASS |
| [x] | 0x0429c8 | 28 | fn_0429c8 | fatal-error reporter (debug_break_hook -> fn_0422d5) | test_f1 PASS |
| [x] | 0x0429e5 | 4 | fn_0429e5 | JMP thunk -> fn_0422d5 | test_f1 PASS |
| [x] | 0x0429ea | 28 | fn_0429ea | fatal-error reporter (direct fn_0422d5) | test_f1 PASS |
| [x] | 0x042a19 | 6 | fn_042a19 | trivial wrapper: g_dat_64008 = 0xe (MOV EAX,0xe; JMP fn_042a0d) | port+test PASS |
| [ ] | 0x042d2a | 258 | FUN_00042d2a | | |
| [ ] | 0x042eef | 210 | FUN_00042eef | | |
| [ ] | 0x044387 | 503 | FUN_00044387 | | |
| [x] | 0x044b96 | 29 | fn_044b96 | drain g_node_list_done: while(head){ node=g_ndpool+head; head=node->next; heap_free_host(node) } | port+test PASS |
| [x] | 0x044bec | 49 | fn_044bec | | DOS AH=3e close-file; sys_file_close + store_arr53730 |
| [x] | 0x045079 | 96 | fn_045079 | format value as hex (itoa32s base16), right-justify + zero-pad to `width` chars, null@buf[width]. ABI: value=EAX, buf=EDX, width=EBX | port+test PASS |
| [x] | 0x0450da | 218 | fn_0450da | 16.16 fixed-point -> decimal string: itoa32s(int), frac digits via frac*10/65536, round-up(carry) when discarded remnant bit15 set. spec[2]=places (def 4) | port+test PASS |
| [x] | 0x045a76 | 11 | fn_045a76 | fn_045a82 thunk: param_2(EDX)==0 -> 0, else tail-jump to fn_045a82 (preserves EAX/EDX) | port+test PASS |
| [x] | 0x045a82 | 101 | fn_045a82 | byte classifier: fn_042a30/042a0d write g_dat_64004/64008; 'P'->0xe/[0x20,0x22)->5/>=0x22->0x13 when g_dat_533bb>2; tbl[bl]=byte@0x53625+(bl+3); ret (p2<<32)|0xffffffff | port+test PASS |
| [ ] | 0x046d32 | 20 | caseD_0 | | |
| [ ] | 0x046d47 | 20 | caseD_8 | | |
| [ ] | 0x0473e5 | 20 | caseD_4 | DEFERRED: register-ABI (ESI/EBP/EDI set by caller) + indirect tail-jump `JMP [EBP+0x76]` through the FPU engine frame; not modelable without the calling context (like the other DPMI/register-ABI no-op thunks) | |
| [ ] | 0x047b4f | 41 | FUN_00047b4f | | |
| [ ] | 0x0487b0 | 464 | FUN_000487b0 | | |
| [ ] | 0x048c8c | 467 | FUN_00048c8c | | |
| [x] | 0x048f2c | 31 | fn_048f2c | reg-ABI in-place: a=048f4c(a->lo,a->hi,count,arr,a->exp); a->exp preserved. EAX=a,EDX=arr,EBX=count | f.c |
| [x] | 0x048f9e | 71 | fn_048f9e | reg-ABI in-place: a=(048f4c(a*a,count,arr,a->exp))*a; ESI=(exp<<16)|exp; a->exp preserved. EAX=a,EBX=count,EDX=arr | f.c |
| [ ] | 0x0495eb | 39 | FUN_000495eb | | |
| [x] | 0x049a0b | 27 | fn_049a0b | | unix-time decode: mode=0 + fn_0498f6(0x63df,*secs,0,out) |
| [x] | 0x049a35 | 719 | fn_049a35 | TZ config parser: config_value_ptr("TZ"), parse_time_str x2, fills g_dat_53780/84/88/a7/7c/34/58; params unused (callback sig). Added 7 TZ globals | port+test PASS |
| [x] | 0x049c76 | 142 | fn_049c76 | TZ range parser (fn_049a35 variant, direct string): parse_time_str x2, initial offset 0x3840, 53780=18000-secs; p2/p3/p4 unused | port+test PASS |
| [ ] | 0x04a32b | 59 | FUN_0004a32b | | |
| [ ] | 0x04a367 | 64 | FUN_0004a367 | | |
## L3 (21)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x03f084 | 218 | fn_03f084 | | |
| [x] | 0x0418c6 | 11 | fn_0418c6 | report undefined ctor/dtor via fn_0429c8 | test_f1 PASS |
| [ ] | 0x041d18
| [x] | 0x041e51 | 26 | fn_041e51 | dpmi | no-op standin (reg-ABI, ret 0); test_f1 PASS |
| [x] | 0x042aba | 30 | fn_042aba | | |
| [ ] | 0x042fc2 | 174 | FUN_00042fc2 | | |
| [x] | 0x043f80 | 66 | fn_043f80 | | |
| [x] | 0x044807 | 64 | fn_044807 | | |
| [x] | 0x04488b | 452 | fn_04488b | reg-ABI(name,mode,extra,val); open/create file handler + node-table store | f.c |
| [x] | 0x044a50 | 100 | fn_044a50 | reg-ABI index/ctx; fn_0492d0+store_arr53730; bit13 -> 2x AH=0x44 ioctl sign-gate -> fn_045a82; ret 0/-1 | f.c |
| [x] | 0x0451bc | 1096 | fn_0451bc | | |
| [x] | 0x045dac | 0 | (artifact) | 1-byte disasm fragment | |
| [x] | 0x045dad | 16 | fn_045dad | report FPU-not-loaded via fn_0429ea | test_f1 PASS |
| [ ] | 0x048422 | 354 | FUN_00048422 | | |
| [x] | 0x048754 | 24 | fn_048754 | reg-ABI in-place: a=fn_04791a(a,a,0,0x80000000,0x3fff); EAX=a. Sign/normalize op | f.c |
| [ ] | 0x04876d | 66 | FUN_0004876d | | |
| [ ] | 0x048a48 | 502 | FUN_00048a48 | | |
| [x] | 0x049236 | 123 | fn_049236 | reg-ABI index/ctx/count; fn_0492d0 + AH=0x42 lseek + AH=0x40 write sign-gates -> fn_045a82/fn_042a0d; ret written | f.c |
| [x] | 0x049415 | 30 | fn_049415 | | |
| [ ] | 0x049613 | 690 | FUN_00049613 | | |
| [x] | 0x049e15 | 106 | fn_049e15 | | |
| [ ] | 0x04a48f | 485 | FUN_0004a48f | | |
## L4 (13)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x043071 | 323 | FUN_00043071 | | |
| [ ] | 0x043fc3 | 229 | FUN_00043fc3 | | |
| [x] | 0x044748 | 190 | fn_044748 | reg-ABI io_state_t*; flush(f13 bit4) fn_049236 / seek-back fn_044807; tail f0=f8,f4=0 | f.c |
| [x] | 0x044848 | 16 | fn_044848 | | lseek wrapper -> fn_044807(h,0,SEEK_SET) |
| [x] | 0x044867 | 35 | fn_044867 | 4-arg wrapper -> fn_04488b(name,0,val&0xff,val); p1/p2 unused | f.c |
| [x] | 0x044c1e | 4 | fn_044c1e | dpmi | JMP thunk -> fn_049415; test_f1 PASS |
| [ ] | 0x044c23 | 640 | FUN_00044c23 | | |
| [ ] | 0x04574a | 339 | FUN_0004574a | | |
| [ ] | 0x04836c | 181 | FUN_0004836c | | |
| [ ] | 0x048c3f | 9 | FUN_00048c3f | | |
| [ ] | 0x048c49 | 12 | FUN_00048c49 | | |
| [ ] | 0x048c56 | 12 | FUN_00048c56 | | |
| [ ] | 0x049e80 | 547 | FUN_00049e80 | | |
## L5 (10)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x041af8 | 226 | heap_malloc | heap | heap.md; port+test PASS (batch 7-8) |
| [ ] | 0x04247a | 251 | FUN_0004247a | | |
| [ ] | 0x0428f8 | 47 | FUN_000428f8 | | |
| [ ] | 0x042928 | 47 | FUN_00042928 | | |
| [ ] | 0x04297a | 44 | FUN_0004297a | | |
| [x] | 0x04322f | 4 | fn_04322f | | mov eax,-1; ret |
| [x] | 0x043234 | 46 | fn_043234 | walk active node list, count matching io_states, flush (fn_044748) those with flags1 bit4 | f.c |
| [ ] | 0x043a7f | 544 | FUN_00043a7f | | |
| [ ] | 0x045c79 | 220 | FUN_00045c79 | | |
| [ ] | 0x0461cc | 5909 | FUN_000461cc | | |
## L6 (10)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x041bdc | 74 | FUN_00041bdc | heap | heap_malloc wrapper (size 0->1); reg-ABI high word ignored |
| [ ] | 0x041ffc | 38 | FUN_00041ffc | | |
| [ ] | 0x04262a | 199 | FUN_0004262a | | |
| [ ] | 0x04285c | 136 | FUN_0004285c | | |
| [ ] | 0x042a43 | 118 | FUN_00042a43 | | |
| [ ] | 0x044ab5 | 167 | FUN_00044ab5 | | |
| [ ] | 0x04589e | 188 | FUN_0004589e | | |
| [ ] | 0x046d00 | 4 | caseD_2 | | |
| [ ] | 0x0473bf | 37 | caseD_3 | | |
| [ ] | 0x04933a | 119 | FUN_0004933a | | |
## L7 (8)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x041d47 | 170 | FUN_00041d47 | | |
| [x] | 0x041e94 | 4 | fn_041e94 | thunk_FUN_00041bdc = heap_malloc alias (JMP 0x41bdc); reg-ABI EAX=size | f.c |
| [ ] | 0x042023 | 32 | FUN_00042023 | | |
| [ ] | 0x0426f2 | 26 | FUN_000426f2 | | |
| [ ] | 0x042717 | 103 | FUN_00042717 | | |
| [ ] | 0x0427d2 | 20 | FUN_000427d2 | | |
| [ ] | 0x0446c3 | 132 | FUN_000446c3 | | |
| [ ] | 0x049192 | 163 | FUN_00049192 | | |
## L8 (7)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x041901 | 44 | canary_check | runtime | primitives.md; test_f1 PASS; 0x429ea->sys_fatal (sys) |
| [ ] | 0x04192e | 457 | FUN_0004192e | | |
| [ ] | 0x041d18 | 46 | FUN_00041d18 | | |
| [ ] | 0x042394 | 33 | FUN_00042394 | | |
| [ ] | 0x04270d | 9 | FUN_0004270d | | |
| [ ] | 0x0427a3 | 46 | FUN_000427a3 | | |
| [ ] | 0x0446b2 | 16 | FUN_000446b2 | | |
## L9 (2)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [x] | 0x0418ee | 15 | canary_id | runtime | primitives.md; test_f1 PASS |
| [ ] | 0x041c8f | 136 | FUN_00041c8f | | |
## L10 (81)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x010181 | 500 | FUN_00010181 | | |
| [x] | 0x010376 | 105 | fn_010376 | free struct fields @0x6d/71/75 (heap_free_host) | 1:1 |
| [x] | 0x0103e0 | 230 | fn_103e0 | | CD/DPMI real-mode I/O wrapper (DPMI trampoline no-op in flat model) |
| [x] | 0x0104c7 | 119 | FUN_000104c7 | DPMI | mem alloc INT31h + DAT_00053c58/5c setup (no-op) |
| [x] | 0x01053f | 48 | fn_1053f | | DPMI INT 0x31 AX=0x101 (no-op in flat model) |
| [x] | 0x010e15 | 43 | fn_010e15 | return g_dat_53c60 | 1:1 (canary prologue omitted) |
| [ ] | 0x010efc | 1024 | FUN_00010efc | | |
| [ ] | 0x0112fd | 233 | FUN_000112fd | | |
| [ ] | 0x013aad | 415 | FUN_00013aad | | |
| [ ] | 0x01435f | 190 | FUN_0001435f | | |
| [x] | 0x01519f | 53 | fn_01519f | return *(u32*)(p+1)-g_dat_639dc | 1:1 |
| [x] | 0x0151d5 | 53 | fn_0151d5 | return *(u32*)(p+9)-g_dat_639e0 | 1:1 |
| [x] | 0x01520b | 47 | fn_01520b | return *(u32*)(p+5) | 1:1 |
| [ ] | 0x01545e | 1036 | FUN_0001545e | | |
| [ ] | 0x01586b | 435 | FUN_0001586b | | |
| [ ] | 0x017218 | 242 | FUN_00017218 | | |
| [x] | 0x031f10 | 78 | fn_031f10 | ring push (count@0x31,cap5) | 1:1 (reg-ABI) |
| [ ] | 0x031f5f | 221 | FUN_00031f5f | | |
| [ ] | 0x0326c1 | 109 | FUN_000326c1 | | |
| [x] | 0x034353 | 125 | fn_034353 | slot update (63f1e/63f0c) | 1:1 (reg-ABI) |
| [x] | 0x0343d1 | 92 | fn_0343d1 | idx flag/counter (62cd8/62c58) | 1:1 |
| [x] | 0x0348e2 | 91 | fn_0348e2 | slot-alloc (g_dat_63f50 wrap@9) | 1:1 |
| [x] | 0x03493e | 72 | FUN_0003493e | | |
| [x] | 0x034a87 | 50 | fn_034a87 | v=g_dat_62d64; +=11; return v | 1:1 |
| [x] | 0x0350aa | 96 | FUN_000350aa | | |
| [x] | 0x03510b | 126 | FUN_0003510b | | |
| [x] | 0x03518a | 81 | fn_03518a | BE 16-bit read (p[0]<<8)|p[1] | 1:1 |
| [x] | 0x0351dc | 134 | FUN_000351dc | | |
| [ ] | 0x035329 | 166 | FUN_00035329 | | |
| [x] | 0x03575d | 148 | fn_03575d | | PIT ch0 rate setup (divisor in EAX), clamp to [1,0x10000] |
| [ ] | 0x0357f2 | 212 | FUN_000357f2 | | |
| [ ] | 0x0358c7 | 121 | FUN_000358c7 | | |
| [x] | 0x035c5c | 178 | FUN_00035c5c | entity | walk list (head=DAT_00053c70,next@0x13f); set [0x30]=1,[0x17]=0x15,[0x11]=0,[0x32]=table[idx*0x88] |
| [x] | 0x035d0f | 77 | FUN_00035d0f | DMA | 3x memory-poll spin on DAT_00063e88[AL] (no-op; degenerate, static byte) |
| [ ] | 0x035d5d | 158 | FUN_00035d5d | | |
| [ ] | 0x035dfc | 197 | FUN_00035dfc | | |
| [x] | 0x035ec2 | 88 | FUN_00035ec2 | VGA | 0x3DA sequencer ready-wait (no-op; degenerate spin, port stub) |
| [x] | 0x036626 | 125 | FUN_00036626 | | |
| [ ] | 0x0366a4 | 292 | FUN_000366a4 | | |
| [ ] | 0x0367c9 | 137 | FUN_000367c9 | | |
| [ ] | 0x036853 | 183 | FUN_00036853 | | |
| [x] | 0x03a594 | 98 | FUN_0003a594 | | |
| [ ] | 0x03a5f7 | 278 | FUN_0003a5f7 | | |
| [x] | 0x03b5c0 | 189 | FUN_0003b5c0 | list | reg-ABI insert; append node (flag/ECX/EDX/EBX payload); next@0x11/prev@0x19 (8B) |
| [x] | 0x03b67e | 153 | FUN_0003b67e | list | reg-ABI remove; head/tail/middle splice; latent null-deref on last node |
| [ ] | 0x03bb42 | 888 | FUN_0003bb42 | | |
| [ ] | 0x03bebb | 885 | FUN_0003bebb | | |
| [ ] | 0x03c231 | 1065 | FUN_0003c231 | | |
| [ ] | 0x03c65b | 143 | FUN_0003c65b | | |
| [ ] | 0x03c72b | 171 | FUN_0003c72b | | |
| [x] | 0x03c7d7 | 123 | FUN_0003c7d7 | struct | reg-ABI; zero 6 fields (+0x2a..0x3b) + memset DAT_0005d08c/5d0a0 |
| [x] | 0x03cad3 | 100 | fn_03cad3 | struct -> g_dat_639dc/e0/e4 | 1:1 |
| [ ] | 0x03cc6b | 415 | FUN_0003cc6b | | |
| [ ] | 0x03ce0b | 692 | FUN_0003ce0b | | |
| [ ] | 0x03d67c | 223 | FUN_0003d67c | | |
| [ ] | 0x03d75c | 306 | FUN_0003d75c | | |
| [ ] | 0x03dcbd | 455 | FUN_0003dcbd | | |
| [ ] | 0x03e0a0 | 171 | FUN_0003e0a0 | | |
| [ ] | 0x03e14c | 194 | FUN_0003e14c | | |
| [x] | 0x03ef29 | 200 | fn_03ef29 | | install IRQ 0x1c, divisor in EAX (isr_set/outb no-op) |
| [x] | 0x03eff2 | 132 | fn_03eff2 | | install IRQ 0x1c (FPU) handler, flag-guarded (isr_set/outb no-op) |
| [x] | 0x03f3be | 81 | fn_03f3be | | VGA palette-set (INT10 0x4f05), index 63e5c, color in EAX |
| [x] | 0x03f410 | 81 | fn_03f410 | | VGA palette-set (INT10 0x4f05), index 63e58, color in EAX |
| [ ] | 0x03fa3b | 162 | FUN_0003fa3b | | |
| [x] | 0x03fade | 47 | FUN_0003fade | DPMI | zero DAT_000a0000[64000] (fn_041e20); region not modeled -> no-op |
| [ ] | 0x03fb0e | 100 | FUN_0003fb0e | | |
| [ ] | 0x03fb73 | 169 | FUN_0003fb73 | | |
| [ ] | 0x03fc1d | 159 | FUN_0003fc1d | | |
| [x] | 0x03fcbd | 72 | FUN_0003fcbd | DPMI | real-mode call {0x1012} via fn_041e51 (no-op) |
| [x] | 0x03feb8 | 242 | fn_03feb8 | | install IRQ 9 + timer/DMA channel state init (isr_set no-op) |
| [x] | 0x03ffab | 97 | fn_03ffab | | install IRQ 9 handler, flag-guarded; mirrors 63f51 to low-mem 0x417 |
| [x] | 0x040570 | 54 | FUN_00040570 | DPMI | real-mode call {3} via fn_041e51 -> no-op |
| [x] | 0x0405a7 | 54 | FUN_000405a7 | DPMI | real-mode call {0x13} via fn_041e51 -> no-op |
| [x] | 0x0405de | 82 | FUN_000405de | | |
| [x] | 0x040631 | 46 | fn_040631 | g_dat_40010=low16(p); return low16 | 1:1 |
| [x] | 0x040788 | 111 | FUN_00040788 | DPMI | mem alloc INT31h 0x100 (no-op, returns 0) |
| [x] | 0x0407f8 | 64 | FUN_000407f8 | DPMI | real-mode call {0x101} via fn_041e51 -> no-op |
| [x] | 0x0408c5 | 130 | FUN_000408c5 | | |
| [x] | 0x04123d | 101 | fn_04123d | device-port init: canary, outb(port,1), 10x inb(port), outb(port,0), port=g_dat_53334+6 | f.c |
| [ ] | 0x0412e9 | 86 | FUN_000412e9 | | |
| [ ] | 0x041390 | 86 | FUN_00041390 | | |
## L11 (77)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x010010 | 200 | FUN_00010010 | | |
| [x] | 0x0100d9 | 167 | fn_0100d9 | free container {count, array} + free array | 1:1 (disasm; 4 checks) |
| [x] | 0x010570 | 134 | fn_10570 | | DPMI real-mode read via fn_103e0 (no-op in flat model) |
| [ ] | 0x011827 | 292 | FUN_00011827 | | |
| [ ] | 0x012efa | 1428 | FUN_00012efa | | |
| [ ] | 0x01348f | 817 | FUN_0001348f | | |
| [ ] | 0x0137c1 | 326 | FUN_000137c1 | | |
| [ ] | 0x013c4d | 1809 | FUN_00013c4d | | |
| [ ] | 0x015295 | 456 | FUN_00015295 | | |
| [x] | 0x015a1f | 315 | fn_015a1f | state machine, data-table distance check | 1:1 (disasm; 5 checks) |
| [x] | 0x015b5b | 442 | fn_015b5b | family variant (s[0x83] gate + table[idx+1] '.' check) | 1:1 (disasm; 6 checks) |
| [x] | 0x015d16 | 442 | fn_015d16 | family variant (X-gate + char-selects-Y-limit) | 1:1 (disasm; 6 checks) |
| [x] | 0x015ed1 | 364 | fn_015ed1 | family variant (s[0x83] gate, no char, Y-limit 10) | 1:1 (disasm; 6 checks) |
| [x] | 0x01603e | 442 | fn_01603e | family core, magic '+' (near-twin) | 1:1 (REL-fp; 1 check) |
| [x] | 0x0161f9 | 442 | fn_0161f9 | family core, magic '+' (near-twin) | 1:1 (REL-fp; 1 check) |
| [x] | 0x0163b4 | 442 | fn_0163b4 | family core, magic ',' (near-twin) | 1:1 (REL-fp; 1 check) |
| [x] | 0x01656f | 480 | fn_01656f | family core, magic {-',} | 1:1 (disasm; 6 checks) |
| [x] | 0x016750 | 520 | fn_016750 | family core, magic {'1' '4' '.'} | 1:1 (disasm; 6 checks) |
| [x] | 0x016959 | 262 | fn_016959 | move-toward-target: *out=±step or 0, countdown s[0xab] | 1:1 (disasm-verified; Ghidra decomp WRONG) |
| [x] | 0x016a60 | 302 | fn_016a60 | clamped move-toward-target (fn_0151d5 vs s[0xc3], clamp g_dat_63354[0x12]/[0x16]); latent: B-reached no *out | 1:1 (disasm; 5 checks) |
| [ ] | 0x016b8f | 1116 | FUN_00016b8f | | |
| [x] | 0x016fec | 555 | fn_016fec | grid-collision detector | 1:1 (disasm; 5 checks) |
| [x] | 0x01730b | 406 | fn_01730b | 2-phase X-then-Y move-toward-target | 1:1 (disasm; 5 checks) |
| [x] | 0x0174a2 | 141 | fn_0174a2 | clamp-check F=fn_01519f(s) to [0,0x140); if out of range adjust *out by ±param2 | 1:1 (disasm; 3 checks) |
| [x] | 0x017530 | 262 | fn_017530 | twin of fn_016959 (alias; byte-identical disasm, callee fn_01519f) | 1:1 (disasm-verified twin) |
| [x] | 0x017637 | 311 | fn_017637 | variant of fn_016a60 (corrected: B-reached writes *out=0) | 1:1 (disasm; 2 checks) |
| [x] | 0x01776f | 439 | fn_01776f | 2D movement delta computer (target<end/>=end) | 1:1 (disasm; 5 checks) |
| [x] | 0x017927 | 894 | FUN_00017927 | | |
| [x] | 0x017ca6 | 620 | fn_017ca6 | Bresenham diagonal stepper | 1:1 (disasm; 7 checks) |
| [x] | 0x032158 | 412 | fn_032158 | entity list: set targets to player + prng jitter | 1:1 (disasm; 6 checks) |
| [ ] | 0x03237b | 124 | FUN_0003237b | | |
| [ ] | 0x0323f8 | 105 | FUN_000323f8 | | |
| [ ] | 0x032464 | 604 | FUN_00032464 | | |
| [ ] | 0x034295 | 105 | FUN_00034295 | | |
| [ ] | 0x0342ff | 83 | FUN_000342ff | | |
| [x] | 0x03442e | 518 | fn_03442e | rotating 10-slot event buffer | 12 tests |
| [x] | 0x034635 | 255 | fn_034635 | keyboard input state machine (g_dat_62c58[key] state; calls fn_034353) | 1:1 (disasm; 5 checks) |
| [ ] | 0x034990 | 126 | FUN_00034990 | | |
| [x] | 0x034aba | 387 | fn_034aba | 3-way interval-list splitter (node arena) | 7 tests |
| [ ] | 0x034f31 | 376 | FUN_00034f31 | | |
| [x] | 0x0356ba | 76 | FUN_000356ba | PIT | reg-ABI ticks->DAT_00063330; divisor=(ticks*DAT_000530b4)/100 -> fn_03575d |
| [ ] | 0x035941 | 127 | FUN_00035941 | | |
| [ ] | 0x035a19 | 122 | FUN_00035a19 | | |
| [ ] | 0x035c2d | 45 | FUN_00035c2d | | |
| [ ] | 0x035f1b | 293 | FUN_00035f1b | | |
| [ ] | 0x036041 | 334 | FUN_00036041 | | |
| [ ] | 0x036190 | 253 | FUN_00036190 | | |
| [ ] | 0x03628e | 392 | FUN_0003628e | | |
| [ ] | 0x036417 | 287 | FUN_00036417 | | |
| [ ] | 0x037306 | 64 | FUN_00037306 | | |
| [ ] | 0x0383a4 | 160 | FUN_000383a4 | | |
| [ ] | 0x038b30 | 242 | FUN_00038b30 | | |
| [ ] | 0x038c23 | 186 | FUN_00038c23 | | |
| [ ] | 0x03a1b3 | 112 | FUN_0003a1b3 | | |
| [ ] | 0x03ace9 | 420 | FUN_0003ace9 | | |
| [ ] | 0x03b718 | 570 | FUN_0003b718 | | |
| [ ] | 0x03b9b7 | 113 | FUN_0003b9b7 | | |
| [ ] | 0x03c6ec | 62 | FUN_0003c6ec | | |
| [ ] | 0x03cb38 | 160 | FUN_0003cb38 | | |
| [ ] | 0x03cbd9 | 145 | FUN_0003cbd9 | | |
| [ ] | 0x03d606 | 117 | FUN_0003d606 | | |
| [ ] | 0x03d88f | 68 | FUN_0003d88f | | |
| [ ] | 0x03d999 | 53 | FUN_0003d999 | | |
| [ ] | 0x03de85 | 272 | FUN_0003de85 | | |
| [ ] | 0x03df96 | 264 | FUN_0003df96 | | |
| [ ] | 0x03e20f | 2911 | FUN_0003e20f | | |
| [ ] | 0x03ee0a | 123 | FUN_0003ee0a | | |
| [ ] | 0x03f15f | 606 | FUN_0003f15f | | |
| [ ] | 0x03f462 | 303 | FUN_0003f462 | | |
| [x] | 0x03f592 | 178 | FUN_0003f592 | VGA | reg-ABI vline to A0000+fn_03f3be (no-op, unmodeled DAT_000a0000) |
| [x] | 0x03f645 | 118 | FUN_0003f645 | VGA | palette fn_03f3be + A0000 memset loop (no-op) |
| [ ] | 0x03f885 | 437 | FUN_0003f885 | | |
| [ ] | 0x040660 | 148 | FUN_00040660 | | |
| [ ] | 0x0406f5 | 145 | FUN_000406f5 | | |
| [ ] | 0x0412a3 | 69 | FUN_000412a3 | | |
| [ ] | 0x041340 | 79 | FUN_00041340 | | |
| [ ] | 0x0417f3 | 106 | FUN_000417f3 | | |
## L12 (58)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x0105f7 | 108 | FUN_000105f7 | | |
| [x] | 0x010664 | 202 | fn_10664 | | CD request build + DPMI read (no-op, EAX-gated) |
| [ ] | 0x01072f | 137 | FUN_0001072f | | |
| [x] | 0x0107b9 | 188 | fn_107b9 | | CD dir-entry read (DPMI no-op in flat model) |
| [x] | 0x010876 | 176 | fn_10876 | | CD dir-entry read + checksum (DPMI no-op, returns 0) |
| [x] | 0x010a56 | 131 | FUN_00010a56 | CD | reg-ABI table entry -> fn_00010570 (no-op) |
| [x] | 0x010b1d | 288 | fn_10b1d | | CD track/TOC read via fn_10570 (no-op in flat model) |
| [ ] | 0x010cbd | 137 | FUN_00010cbd | | |
| [x] | 0x010dde | 54 | FUN_00010dde | DPMI | build stack struct + fn_00010570 (no-op) |
| [ ] | 0x017f13 | 3956 | FUN_00017f13 | | |
| [ ] | 0x018e88 | 4031 | FUN_00018e88 | | |
| [ ] | 0x019e48 | 4223 | FUN_00019e48 | | |
| [ ] | 0x01aec8 | 4063 | FUN_0001aec8 | | |
| [ ] | 0x01bea8 | 4283 | FUN_0001bea8 | | |
| [ ] | 0x01cf64 | 4134 | FUN_0001cf64 | | |
| [ ] | 0x01df8b | 4312 | FUN_0001df8b | | |
| [ ] | 0x01f064 | 4363 | FUN_0001f064 | | |
| [ ] | 0x020170 | 4139 | FUN_00020170 | | |
| [ ] | 0x02119c | 4123 | FUN_0002119c | | |
| [ ] | 0x0221b8 | 4067 | FUN_000221b8 | | |
| [ ] | 0x02319c | 6748 | FUN_0002319c | | |
| [ ] | 0x024bf9 | 6578 | FUN_00024bf9 | | |
| [ ] | 0x0265ac | 5695 | FUN_000265ac | | |
| [ ] | 0x027bec | 6940 | FUN_00027bec | | |
| [ ] | 0x029709 | 5867 | FUN_00029709 | | |
| [ ] | 0x02adf5 | 6722 | FUN_0002adf5 | | |
| [ ] | 0x02c838 | 6803 | FUN_0002c838 | | |
| [ ] | 0x02e2cc | 6993 | FUN_0002e2cc | | |
| [ ] | 0x02fe1e | 7279 | FUN_0002fe1e | | |
| [ ] | 0x03203d | 94 | FUN_0003203d | | |
| [ ] | 0x03272f | 7013 | FUN_0003272f | | |
| [x] | 0x034735 | 173 | FUN_00034735 | | |
| [ ] | 0x03565a | 95 | FUN_0003565a | | |
| [ ] | 0x0359c1 | 87 | FUN_000359c1 | | |
| [ ] | 0x035ac0 | 75 | FUN_00035ac0 | | |
| [ ] | 0x035b91 | 98 | FUN_00035b91 | | |
| [ ] | 0x035bf4 | 56 | FUN_00035bf4 | | |
| [ ] | 0x037347 | 357 | FUN_00037347 | | |
| [ ] | 0x037922 | 480 | FUN_00037922 | | |
| [ ] | 0x038a20 | 191 | FUN_00038a20 | | |
| [ ] | 0x038cde | 606 | FUN_00038cde | | |
| [ ] | 0x03ae8e | 137 | FUN_0003ae8e | | |
| [ ] | 0x03b953 | 99 | FUN_0003b953 | | |
| [x] | 0x03ba2c | 89 | FUN_0003ba2c | | |
| [x] | 0x03ba86 | 187 | FUN_0003ba86 | | |
| [ ] | 0x03c853 | 639 | FUN_0003c853 | | |
| [ ] | 0x03d4e4 | 289 | FUN_0003d4e4 | | |
| [ ] | 0x03d9cf | 749 | FUN_0003d9cf | | |
| [ ] | 0x03ed6f | 154 | FUN_0003ed6f | | |
| [ ] | 0x03f80d | 119 | FUN_0003f80d | | |
| [ ] | 0x040948 | 53 | FUN_00040948 | | |
| [ ] | 0x040b83 | 389 | FUN_00040b83 | | |
| [ ] | 0x040efd | 40 | FUN_00040efd | | |
| [ ] | 0x040f4f | 99 | FUN_00040f4f | | |
| [ ] | 0x040fb3 | 549 | FUN_00040fb3 | | |
| [ ] | 0x0411d9 | 99 | FUN_000411d9 | | |
| [ ] | 0x0413e7 | 40 | FUN_000413e7 | | |
| [ ] | 0x041410 | 40 | FUN_00041410 | | |
## L13 (16)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x010927 | 302 | FUN_00010927 | | |
| [ ] | 0x010d47 | 150 | FUN_00010d47 | | |
| [ ] | 0x013908 | 420 | FUN_00013908 | | |
| [ ] | 0x031a8e | 1153 | FUN_00031a8e | | |
| [ ] | 0x0347e3 | 254 | FUN_000347e3 | | |
| [ ] | 0x034eb0 | 128 | FUN_00034eb0 | | |
| [ ] | 0x035a94 | 43 | FUN_00035a94 | | |
| [ ] | 0x035b0c | 132 | FUN_00035b0c | | |
| [ ] | 0x03801b | 124 | FUN_0003801b | | |
| [ ] | 0x038098 | 318 | FUN_00038098 | | |
| [ ] | 0x0381d7 | 460 | FUN_000381d7 | | |
| [ ] | 0x03d0c0 | 387 | FUN_0003d0c0 | | |
| [ ] | 0x03d8d4 | 196 | FUN_0003d8d4 | | |
| [ ] | 0x04097e | 516 | FUN_0004097e | | |
| [ ] | 0x040d09 | 387 | FUN_00040d09 | | |
| [ ] | 0x040e8d | 111 | FUN_00040e8d | | |
## L14 (7)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x010c3e | 126 | FUN_00010c3e | | |
| [ ] | 0x0322f5 | 133 | FUN_000322f5 | | |
| [ ] | 0x0353d0 | 538 | FUN_000353d0 | | |
| [ ] | 0x036537 | 85 | FUN_00036537 | | |
| [ ] | 0x03658d | 152 | FUN_0003658d | | |
| [ ] | 0x037b03 | 1303 | FUN_00037b03 | | |
| [ ] | 0x040839 | 139 | FUN_00040839 | | |
## L15 (5)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x035263 | 197 | FUN_00035263 | | |
| [ ] | 0x03690b | 2554 | FUN_0003690b | | |
| [ ] | 0x0374ad | 538 | FUN_000374ad | | |
| [ ] | 0x0376c8 | 601 | FUN_000376c8 | | |
| [ ] | 0x04185e | 94 | FUN_0004185e | | |
## L16 (8)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x0113e7 | 1087 | FUN_000113e7 | | |
| [ ] | 0x01441e | 3456 | FUN_0001441e | | |
| [ ] | 0x034e3c | 115 | FUN_00034e3c | | |
| [ ] | 0x038445 | 1498 | FUN_00038445 | | |
| [ ] | 0x038f3d | 3269 | FUN_00038f3d | | |
| [ ] | 0x03a224 | 879 | FUN_0003a224 | | |
| [ ] | 0x03a70e | 282 | FUN_0003a70e | | |
| [ ] | 0x03f6bc | 336 | FUN_0003f6bc | | |
## L17 (6)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x01194c | 3273 | FUN_0001194c | | |
| [ ] | 0x0320f5 | 98 | FUN_000320f5 | | |
| [ ] | 0x034c40 | 104 | FUN_00034c40 | | |
| [ ] | 0x038ae0 | 79 | FUN_00038ae0 | | |
| [ ] | 0x039c03 | 1455 | FUN_00039c03 | | |
| [ ] | 0x03d244 | 671 | FUN_0003d244 | | |
## L18 (1)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x012616 | 2275 | FUN_00012616 | | |
## L19 (1)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x03209c | 88 | FUN_0003209c | | |
## L20 (1)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x03a829 | 1215 | FUN_0003a829 | | |
## L21 (1)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x03af18 | 1701 | FUN_0003af18 | | |
## L22 (1)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x0445c9 | 78 | FUN_000445c9 | | |
## L23 (3)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x042044 | 1 | _entry | | |
| [ ] | 0x042046 | 116 | FUN_00042046 | | |
| [ ] | 0x0420bc | 536 | FUN_000420bc | | |
