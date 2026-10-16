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

TOTALS: 114 / 528 done

| Level | Total | Done | Analyzed | Skipped | Pending |
|---|---|---|---|---|---|
| L0 | 112 | 108 | 0 | 0 | 4 |
| L1 | 47 | 3 | 0 | 0 | 44 |
| L2 | 32 | 0 | 0 | 0 | 32 |
| L3 | 21 | 0 | 0 | 0 | 21 |
| L4 | 13 | 0 | 0 | 0 | 13 |
| L5 | 10 | 1 | 0 | 0 | 9 |
| L6 | 10 | 0 | 0 | 0 | 10 |
| L7 | 8 | 0 | 0 | 0 | 8 |
| L8 | 7 | 1 | 0 | 0 | 6 |
| L9 | 2 | 1 | 0 | 0 | 1 |
| L10 | 81 | 0 | 0 | 0 | 81 |
| L11 | 77 | 0 | 0 | 0 | 77 |
| L12 | 58 | 0 | 0 | 0 | 58 |
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
| [ ] | 0x04794a | 363 | FUN_0004794a | | |
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
| [ ] | 0x0485eb | 122 | FUN_000485eb | | |
| [x] | 0x048666 | 166 | fx_classify_status | fixed-point value classifier (register ABI EAX=fx ptr, EBP=ctx ptr, EDI=class offset). If (mask22&flags)==mask22 -> status 0x41. Else clear mask22, classify by mag=exp&0x7fff: mag0->(hi|lo)!=0?0x44:(flags|=mask1c,0x40); mag==0x7fff->flags|=mask20,(lo==0&&hi==0x80000000)?0x05:0x01; else->(hi sign)?0x04:(flags|=mask20,0x44). status high byte=(code|signflag) via &0xb8ff. Masks from 0x45e1c table | fx_classify_status (f.c) |
| [ ] | 0x048e60 | 203 | FUN_00048e60 | | |
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
| [ ] | 0x03ee88 | 160 | FUN_0003ee88 | | |
| [ ] | 0x041439 | 76 | FUN_00041439 | | |
| [ ] | 0x041c28 | 8 | FUN_00041c28 | | |
| [x] | 0x041c31 | 93 | heap_free_host | heap | heap.md; port+test PASS (batch 7-8) |
| [ ] | 0x041e9f | 35 | FUN_00041e9f | | |
| [ ] | 0x041ec3 | 15 | FUN_00041ec3 | | |
| [ ] | 0x041f42 | 26 | FUN_00041f42 | | |
| [ ] | 0x0422d5 | 58 | FUN_000422d5 | | |
| [ ] | 0x0423cf | 21 | FUN_000423cf | | |
| [ ] | 0x0427f3 | 104 | FUN_000427f3 | | |
| [ ] | 0x042958 | 4 | thunk_FUN_0004564f | | |
| [ ] | 0x042a08 | 4 | FUN_00042a08 | | |
| [ ] | 0x042a0d | 11 | FUN_00042a0d | | |
| [ ] | 0x042a30 | 11 | FUN_00042a30 | | |
| [ ] | 0x042a3c | 6 | FUN_00042a3c | | |
| [ ] | 0x042ccc | 93 | FUN_00042ccc | | |
| [ ] | 0x0431b5 | 118 | FUN_000431b5 | | |
| [ ] | 0x043280 | 48 | FUN_00043280 | | |
| [ ] | 0x043324 | 55 | FUN_00043324 | | |
| [ ] | 0x043381 | 76 | FUN_00043381 | | |
| [ ] | 0x04391d | 58 | FUN_0004391d | | |
| [ ] | 0x043e34 | 237 | FUN_00043e34 | | |
| [ ] | 0x0440a9 | 733 | FUN_000440a9 | | |
| [x] | 0x04457f | 38 | hex_digit_value | string | port+test PASS (hex digit: 0-9, a-f->10-15, else 16; via to_lower 0x44859) |
| [ ] | 0x044bb4 | 48 | FUN_00044bb4 | | |
| [ ] | 0x044ea4 | 306 | FUN_00044ea4 | | |
| [x] | 0x045605 | 23 | string_upper | string | port+test PASS (in-place upper-case via to_upper 0x494bd) |
| [ ] | 0x045bb7 | 193 | FUN_00045bb7 | | |
| [ ] | 0x0478e2 | 13 | FUN_000478e2 | | |
| [ ] | 0x0478f0 | 41 | FUN_000478f0 | | |
| [ ] | 0x04791a | 47 | FUN_0004791a | | |
| [ ] | 0x047b79 | 426 | FUN_00047b79 | | |
| [ ] | 0x047d24 | 41 | FUN_00047d24 | | |
| [ ] | 0x047f54 | 127 | FUN_00047f54 | | |
| [ ] | 0x048053 | 198 | FUN_00048053 | | |
| [ ] | 0x048585 | 101 | FUN_00048585 | | |
| [ ] | 0x048f4c | 81 | FUN_00048f4c | | |
| [ ] | 0x048fe6 | 201 | FUN_00048fe6 | | |
| [ ] | 0x0492d0 | 84 | FUN_000492d0 | | |
| [ ] | 0x0494a2 | 26 | FUN_000494a2 | | |
| [ ] | 0x049506 | 228 | FUN_00049506 | | |
| [ ] | 0x0498f6 | 276 | FUN_000498f6 | | |
| [ ] | 0x049a6e | 277 | FUN_00049a6e | | |
| [ ] | 0x049b84 | 241 | FUN_00049b84 | | |
| [ ] | 0x04a24d | 89 | FUN_0004a24d | | |
| [ ] | 0x04a7fa | 24 | FUN_0004a7fa | | |
| [ ] | 0x04a814 | 57 | FUN_0004a814 | | |
## L2 (32)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x041e20 | 23 | FUN_00041e20 | | |
| [ ] | 0x041e38 | 24 | FUN_00041e38 | | |
| [ ] | 0x041fa1 | 0 | FUN_00041fa1 | | |
| [ ] | 0x0423b7 | 23 | FUN_000423b7 | | |
| [ ] | 0x042576 | 179 | FUN_00042576 | | |
| [ ] | 0x0429c8 | 28 | FUN_000429c8 | | |
| [ ] | 0x0429e5 | 4 | thunk_FUN_000422d5 | | |
| [ ] | 0x0429ea | 28 | FUN_000429ea | | |
| [ ] | 0x042a19 | 6 | FUN_00042a19 | | |
| [ ] | 0x042d2a | 258 | FUN_00042d2a | | |
| [ ] | 0x042eef | 210 | FUN_00042eef | | |
| [ ] | 0x044387 | 503 | FUN_00044387 | | |
| [ ] | 0x044b96 | 29 | FUN_00044b96 | | |
| [ ] | 0x044bec | 49 | FUN_00044bec | | |
| [ ] | 0x045079 | 96 | FUN_00045079 | | |
| [ ] | 0x0450da | 218 | FUN_000450da | | |
| [ ] | 0x045a76 | 11 | FUN_00045a76 | | |
| [ ] | 0x045a82 | 101 | FUN_00045a82 | | |
| [ ] | 0x046d32 | 20 | caseD_0 | | |
| [ ] | 0x046d47 | 20 | caseD_8 | | |
| [ ] | 0x0473e5 | 20 | caseD_4 | | |
| [ ] | 0x047b4f | 41 | FUN_00047b4f | | |
| [ ] | 0x0487b0 | 464 | FUN_000487b0 | | |
| [ ] | 0x048c8c | 467 | FUN_00048c8c | | |
| [ ] | 0x048f2c | 31 | FUN_00048f2c | | |
| [ ] | 0x048f9e | 71 | FUN_00048f9e | | |
| [ ] | 0x0495eb | 39 | FUN_000495eb | | |
| [ ] | 0x049a0b | 27 | FUN_00049a0b | | |
| [ ] | 0x049a35 | 719 | FUN_00049a35 | | |
| [ ] | 0x049c76 | 142 | FUN_00049c76 | | |
| [ ] | 0x04a32b | 59 | FUN_0004a32b | | |
| [ ] | 0x04a367 | 64 | FUN_0004a367 | | |
## L3 (21)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x03f084 | 218 | FUN_0003f084 | | |
| [ ] | 0x0418c6 | 11 | FUN_000418c6 | | |
| [ ] | 0x041e51 | 26 | FUN_00041e51 | | |
| [ ] | 0x042aba | 30 | FUN_00042aba | | |
| [ ] | 0x042fc2 | 174 | FUN_00042fc2 | | |
| [ ] | 0x043f80 | 66 | FUN_00043f80 | | |
| [ ] | 0x044807 | 64 | FUN_00044807 | | |
| [ ] | 0x04488b | 452 | FUN_0004488b | | |
| [ ] | 0x044a50 | 100 | FUN_00044a50 | | |
| [ ] | 0x0451bc | 1096 | FUN_000451bc | | |
| [ ] | 0x045dac | 0 | FUN_00045dac | | |
| [ ] | 0x045dad | 16 | FUN_00045dad | | |
| [ ] | 0x048422 | 354 | FUN_00048422 | | |
| [ ] | 0x048754 | 24 | FUN_00048754 | | |
| [ ] | 0x04876d | 66 | FUN_0004876d | | |
| [ ] | 0x048a48 | 502 | FUN_00048a48 | | |
| [ ] | 0x049236 | 123 | FUN_00049236 | | |
| [ ] | 0x049415 | 30 | FUN_00049415 | | |
| [ ] | 0x049613 | 690 | FUN_00049613 | | |
| [ ] | 0x049e15 | 106 | FUN_00049e15 | | |
| [ ] | 0x04a48f | 485 | FUN_0004a48f | | |
## L4 (13)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x043071 | 323 | FUN_00043071 | | |
| [ ] | 0x043fc3 | 229 | FUN_00043fc3 | | |
| [ ] | 0x044748 | 190 | FUN_00044748 | | |
| [ ] | 0x044848 | 16 | FUN_00044848 | | |
| [ ] | 0x044867 | 35 | FUN_00044867 | | |
| [ ] | 0x044c1e | 4 | thunk_FUN_00049415 | | |
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
| [ ] | 0x04322f | 4 | FUN_0004322f | | |
| [ ] | 0x043234 | 46 | FUN_00043234 | | |
| [ ] | 0x043a7f | 544 | FUN_00043a7f | | |
| [ ] | 0x045c79 | 220 | FUN_00045c79 | | |
| [ ] | 0x0461cc | 5909 | FUN_000461cc | | |
## L6 (10)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x041bdc | 74 | FUN_00041bdc | | |
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
| [ ] | 0x041e94 | 4 | thunk_FUN_00041bdc | | |
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
| [ ] | 0x010376 | 105 | FUN_00010376 | | |
| [ ] | 0x0103e0 | 230 | FUN_000103e0 | | |
| [ ] | 0x0104c7 | 119 | FUN_000104c7 | | |
| [ ] | 0x01053f | 48 | FUN_0001053f | | |
| [ ] | 0x010e15 | 43 | FUN_00010e15 | | |
| [ ] | 0x010efc | 1024 | FUN_00010efc | | |
| [ ] | 0x0112fd | 233 | FUN_000112fd | | |
| [ ] | 0x013aad | 415 | FUN_00013aad | | |
| [ ] | 0x01435f | 190 | FUN_0001435f | | |
| [ ] | 0x01519f | 53 | FUN_0001519f | | |
| [ ] | 0x0151d5 | 53 | FUN_000151d5 | | |
| [ ] | 0x01520b | 47 | FUN_0001520b | | |
| [ ] | 0x01545e | 1036 | FUN_0001545e | | |
| [ ] | 0x01586b | 435 | FUN_0001586b | | |
| [ ] | 0x017218 | 242 | FUN_00017218 | | |
| [ ] | 0x031f10 | 78 | FUN_00031f10 | | |
| [ ] | 0x031f5f | 221 | FUN_00031f5f | | |
| [ ] | 0x0326c1 | 109 | FUN_000326c1 | | |
| [ ] | 0x034353 | 125 | FUN_00034353 | | |
| [ ] | 0x0343d1 | 92 | FUN_000343d1 | | |
| [ ] | 0x0348e2 | 91 | FUN_000348e2 | | |
| [ ] | 0x03493e | 72 | FUN_0003493e | | |
| [ ] | 0x034a87 | 50 | FUN_00034a87 | | |
| [ ] | 0x0350aa | 96 | FUN_000350aa | | |
| [ ] | 0x03510b | 126 | FUN_0003510b | | |
| [ ] | 0x03518a | 81 | FUN_0003518a | | |
| [ ] | 0x0351dc | 134 | FUN_000351dc | | |
| [ ] | 0x035329 | 166 | FUN_00035329 | | |
| [ ] | 0x03575d | 148 | FUN_0003575d | | |
| [ ] | 0x0357f2 | 212 | FUN_000357f2 | | |
| [ ] | 0x0358c7 | 121 | FUN_000358c7 | | |
| [ ] | 0x035c5c | 178 | FUN_00035c5c | | |
| [ ] | 0x035d0f | 77 | FUN_00035d0f | | |
| [ ] | 0x035d5d | 158 | FUN_00035d5d | | |
| [ ] | 0x035dfc | 197 | FUN_00035dfc | | |
| [ ] | 0x035ec2 | 88 | FUN_00035ec2 | | |
| [ ] | 0x036626 | 125 | FUN_00036626 | | |
| [ ] | 0x0366a4 | 292 | FUN_000366a4 | | |
| [ ] | 0x0367c9 | 137 | FUN_000367c9 | | |
| [ ] | 0x036853 | 183 | FUN_00036853 | | |
| [ ] | 0x03a594 | 98 | FUN_0003a594 | | |
| [ ] | 0x03a5f7 | 278 | FUN_0003a5f7 | | |
| [ ] | 0x03b5c0 | 189 | FUN_0003b5c0 | | |
| [ ] | 0x03b67e | 153 | FUN_0003b67e | | |
| [ ] | 0x03bb42 | 888 | FUN_0003bb42 | | |
| [ ] | 0x03bebb | 885 | FUN_0003bebb | | |
| [ ] | 0x03c231 | 1065 | FUN_0003c231 | | |
| [ ] | 0x03c65b | 143 | FUN_0003c65b | | |
| [ ] | 0x03c72b | 171 | FUN_0003c72b | | |
| [ ] | 0x03c7d7 | 123 | FUN_0003c7d7 | | |
| [ ] | 0x03cad3 | 100 | FUN_0003cad3 | | |
| [ ] | 0x03cc6b | 415 | FUN_0003cc6b | | |
| [ ] | 0x03ce0b | 692 | FUN_0003ce0b | | |
| [ ] | 0x03d67c | 223 | FUN_0003d67c | | |
| [ ] | 0x03d75c | 306 | FUN_0003d75c | | |
| [ ] | 0x03dcbd | 455 | FUN_0003dcbd | | |
| [ ] | 0x03e0a0 | 171 | FUN_0003e0a0 | | |
| [ ] | 0x03e14c | 194 | FUN_0003e14c | | |
| [ ] | 0x03ef29 | 200 | FUN_0003ef29 | | |
| [ ] | 0x03eff2 | 132 | FUN_0003eff2 | | |
| [ ] | 0x03f3be | 81 | FUN_0003f3be | | |
| [ ] | 0x03f410 | 81 | FUN_0003f410 | | |
| [ ] | 0x03fa3b | 162 | FUN_0003fa3b | | |
| [ ] | 0x03fade | 47 | FUN_0003fade | | |
| [ ] | 0x03fb0e | 100 | FUN_0003fb0e | | |
| [ ] | 0x03fb73 | 169 | FUN_0003fb73 | | |
| [ ] | 0x03fc1d | 159 | FUN_0003fc1d | | |
| [ ] | 0x03fcbd | 72 | FUN_0003fcbd | | |
| [ ] | 0x03feb8 | 242 | FUN_0003feb8 | | |
| [ ] | 0x03ffab | 97 | FUN_0003ffab | | |
| [ ] | 0x040570 | 54 | FUN_00040570 | | |
| [ ] | 0x0405a7 | 54 | FUN_000405a7 | | |
| [ ] | 0x0405de | 82 | FUN_000405de | | |
| [ ] | 0x040631 | 46 | FUN_00040631 | | |
| [ ] | 0x040788 | 111 | FUN_00040788 | | |
| [ ] | 0x0407f8 | 64 | FUN_000407f8 | | |
| [ ] | 0x0408c5 | 130 | FUN_000408c5 | | |
| [ ] | 0x04123d | 101 | FUN_0004123d | | |
| [ ] | 0x0412e9 | 86 | FUN_000412e9 | | |
| [ ] | 0x041390 | 86 | FUN_00041390 | | |
## L11 (77)

| Status | Addr | Size | Name | Subsystem | Notes |
|---|---|---|---|---|---|
| [ ] | 0x010010 | 200 | FUN_00010010 | | |
| [ ] | 0x0100d9 | 167 | FUN_000100d9 | | |
| [ ] | 0x010570 | 134 | FUN_00010570 | | |
| [ ] | 0x011827 | 292 | FUN_00011827 | | |
| [ ] | 0x012efa | 1428 | FUN_00012efa | | |
| [ ] | 0x01348f | 817 | FUN_0001348f | | |
| [ ] | 0x0137c1 | 326 | FUN_000137c1 | | |
| [ ] | 0x013c4d | 1809 | FUN_00013c4d | | |
| [ ] | 0x015295 | 456 | FUN_00015295 | | |
| [ ] | 0x015a1f | 315 | FUN_00015a1f | | |
| [ ] | 0x015b5b | 442 | FUN_00015b5b | | |
| [ ] | 0x015d16 | 442 | FUN_00015d16 | | |
| [ ] | 0x015ed1 | 364 | FUN_00015ed1 | | |
| [ ] | 0x01603e | 442 | FUN_0001603e | | |
| [ ] | 0x0161f9 | 442 | FUN_000161f9 | | |
| [ ] | 0x0163b4 | 442 | FUN_000163b4 | | |
| [ ] | 0x01656f | 480 | FUN_0001656f | | |
| [ ] | 0x016750 | 520 | FUN_00016750 | | |
| [ ] | 0x016959 | 262 | FUN_00016959 | | |
| [ ] | 0x016a60 | 302 | FUN_00016a60 | | |
| [ ] | 0x016b8f | 1116 | FUN_00016b8f | | |
| [ ] | 0x016fec | 555 | FUN_00016fec | | |
| [ ] | 0x01730b | 406 | FUN_0001730b | | |
| [ ] | 0x0174a2 | 141 | FUN_000174a2 | | |
| [ ] | 0x017530 | 262 | FUN_00017530 | | |
| [ ] | 0x017637 | 311 | FUN_00017637 | | |
| [ ] | 0x01776f | 439 | FUN_0001776f | | |
| [ ] | 0x017927 | 894 | FUN_00017927 | | |
| [ ] | 0x017ca6 | 620 | FUN_00017ca6 | | |
| [ ] | 0x032158 | 412 | FUN_00032158 | | |
| [ ] | 0x03237b | 124 | FUN_0003237b | | |
| [ ] | 0x0323f8 | 105 | FUN_000323f8 | | |
| [ ] | 0x032464 | 604 | FUN_00032464 | | |
| [ ] | 0x034295 | 105 | FUN_00034295 | | |
| [ ] | 0x0342ff | 83 | FUN_000342ff | | |
| [ ] | 0x03442e | 518 | FUN_0003442e | | |
| [ ] | 0x034635 | 255 | FUN_00034635 | | |
| [ ] | 0x034990 | 126 | FUN_00034990 | | |
| [ ] | 0x034aba | 387 | FUN_00034aba | | |
| [ ] | 0x034f31 | 376 | FUN_00034f31 | | |
| [ ] | 0x0356ba | 76 | FUN_000356ba | | |
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
| [ ] | 0x03f592 | 178 | FUN_0003f592 | | |
| [ ] | 0x03f645 | 118 | FUN_0003f645 | | |
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
| [ ] | 0x010664 | 202 | FUN_00010664 | | |
| [ ] | 0x01072f | 137 | FUN_0001072f | | |
| [ ] | 0x0107b9 | 188 | FUN_000107b9 | | |
| [ ] | 0x010876 | 176 | FUN_00010876 | | |
| [ ] | 0x010a56 | 131 | FUN_00010a56 | | |
| [ ] | 0x010b1d | 288 | FUN_00010b1d | | |
| [ ] | 0x010cbd | 137 | FUN_00010cbd | | |
| [ ] | 0x010dde | 54 | FUN_00010dde | | |
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
| [ ] | 0x034735 | 173 | FUN_00034735 | | |
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
| [ ] | 0x03ba2c | 89 | FUN_0003ba2c | | |
| [ ] | 0x03ba86 | 187 | FUN_0003ba86 | | |
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
