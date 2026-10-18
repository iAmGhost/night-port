#!/usr/bin/env python3
"""Exact instruction-by-instruction mirror of fn_047d4e (fx_mul64), 0x047d4e-0x047e3c.

Register-ABI: in EAX=a_lo, EDX=a_hi, EBX=b_lo, ECX=b_hi, ESI=esi (exp field, 32-bit).
out: EAX=r_lo, EDX=r_hi (64-bit product mantissa), ESI=r_exp (16-bit exponent).

The mirror tracks CF/ZF/SF/OF per instruction because the JS at 0x047de4 reads SF
from the multiply's final ADC, and the round/normalize block (0x047dee-0x047e35) is
flag-dense (CF/ZF/SHR/SHRD/RCR). No shortcuts: the word-by-word 128-bit MAC is mirrored
so the trailing flags match the asm.
"""
M32 = 0xFFFFFFFF
M16 = 0xFFFF

def s16(x):
    x = x & M16
    return x - 0x10000 if (x & 0x8000) else x

def s32(x):
    x = x & M32
    return x - 0x100000000 if (x & 0x80000000) else x

def ref(a_lo, a_hi, b_lo, b_hi, esi):
    # registers
    EAX, EDX, EBX, ECX, ESI, EDI, EBP = a_lo & M32, a_hi & M32, b_lo & M32, b_hi & M32, esi & M32, 0, 0
    CF = ZF = SF = OF = 0
    # --- 0x047d4e OR EAX,EAX ---
    EAX = EAX | EAX
    ZF = 1 if EAX == 0 else 0
    if not ZF:            # 0x047d50 JNZ -> bcheck
        EAX = EAX         # skip a_hi check (a_lo != 0)
        goto_bcheck = True
    else:
        # --- 0x047d52 OR EDX,EDX ---
        EDX = EDX | EDX
        ZF = 1 if EDX == 0 else 0
        if not ZF:        # 0x047d54 JNZ -> bcheck
            goto_bcheck = True
        else:
            # --- 0x047d56 ADD SI,SI (16-bit) ---
            si = (m16_add := (ESI & M16) + (ESI & M16))
            CF = 1 if si > M16 else 0
            si &= M16
            ESI = (ESI & 0xFFFF0000) | si
            ZF = 1 if si == 0 else 0
            if not ZF:    # 0x047d59 JNZ -> 0x047d5c
                # 0x047d5c RCR SI,1 (16-bit)
                old = si
                si = ((si >> 1) | (CF << 15)) & M16
                CF = old & 1
                ESI = (ESI & 0xFFFF0000) | si
                goto_bcheck = True
            else:         # 0x047d5b RET (EAX=0, EDX=a_hi=0, ESI)
                return (0, 0, ESI)
    # --- 0x047d5f OR ECX,ECX ---
    ECX = ECX | ECX
    ZF = 1 if ECX == 0 else 0
    if ZF:                # 0x047d61 JNZ not taken -> 0x047d63
        EBX = EBX | EBX
        ZF = 1 if EBX == 0 else 0
    if ZF:                # 0x047d65 JNZ not taken -> 0x047d67
        ZF = 1 if (ESI & 0x7FFF0000) == 0 else 0
    if ZF:                # 0x047d6d JNZ not taken -> 0x047d6f
        return (0, 0, 0)
    # --- main 0x047d76 ---
    # PUSH EBP; PUSH EDI (saved, restored at RET; values irrelevant to result)
    # --- 0x047d78 XCHG ESI,ECX ---
    ESI, ECX = ECX, ESI
    # --- 0x047d7a MOV EDI,ECX ---
    EDI = ECX
    # --- 0x047d7c ROL EDI,16 ---
    EDI = ((EDI << 16) | (EDI >> 16)) & M32
    # --- 0x047d7f SAR EDI,16 ---
    EDI = s16(EDI >> 16) & M32
    # --- 0x047d82 SAR ECX,16 ---
    ECX = s16(ECX >> 16) & M32
    # --- 0x047d85 AND EDI,0x80007fff ---
    EDI &= 0x80007FFF
    # --- 0x047d8b AND ECX,0x80007fff ---
    ECX &= 0x80007FFF
    # --- 0x047d91 ADD ECX,EDI ---
    r = (ECX & M32) + (EDI & M32)
    CF = 1 if r > M32 else 0
    ECX = r & M32
    # --- 0x047d93 SUB CX,0x3fe (16-bit) ---
    lo = ECX & M16
    r = lo - 0x3FE
    CF = 1 if r < 0 else 0
    CX = r & M16
    ECX = (ECX & 0xFFFF0000) | CX
    # --- 0x047d98 JC 0x047da6 ---
    if not CF:
        # --- 0x047d9a CMP CX,0x7fff ---
        CF = 1 if CX < 0x7FFF else 0
        ZF = 1 if CX == 0x7FFF else 0
        if not CF:        # 0x047d9f JC not taken -> 0x047da1
            # --- OVERFLOW 0x047e3d ---
            # MOV CX,0x7fff; MOV EDX,0x80000000; SUB EAX,EAX; JMP 0x047e33
            ECX = (ECX & 0xFFFF0000) | 0x7FFF
            EDX = 0x80000000
            EAX = 0
            # fall through to epilogue 0x047e33 (below)
            return _epilogue(ECX, EAX, EDX, EBX, ESI)
    # --- underflow check 0x047da6 CMP CX,-0x40 (0xffc0) ---
    # signed compare CX (16-bit) vs -0x40
    if s16(CX) < -0x40:   # 0x047daa JGE not taken
        # 0x047dac: EAX=EDX=ECX=0; JMP 0x047e33
        return _epilogue(0, 0, 0, 0, 0)
    # --- multiply 0x047db7 ---
    # PUSH ECX; SUB EBP,EBP; PUSH ESI; PUSH EDX; PUSH EAX
    exp_adj_ecx = ECX          # saved (32-bit, low16=CX)
    EBP = 0
    push_esi = ESI             # = b_hi
    push_edx = EDX             # = a_hi
    push_eax = EAX             # = a_lo
    # 0x047dbd MUL EBX  -> EDX:EAX = EAX*a_lo? (EAX=a_lo, EBX=b_lo)
    prod = (EAX & M32) * (EBX & M32)
    EAX = prod & M32; EDX = (prod >> 32) & M32
    # 0x047dbf XCHG EAX,ESI
    EAX, ESI = ESI, EAX
    # 0x047dc0 MOV ECX,EDX
    ECX = EDX
    # 0x047dc2 POP EDX  (top of stack = push_eax = a_lo)
    EDX = push_eax
    # 0x047dc3 MUL EDX  -> EDX:EAX = EAX(b_hi) * EDX(a_lo)
    prod = (EAX & M32) * (EDX & M32)
    EAX = prod & M32; EDX = (prod >> 32) & M32
    # 0x047dc5 MOV EDI,EDX
    EDI = EDX
    # 0x047dc7 ADD ECX,EAX
    r = (ECX & M32) + (EAX & M32); CF = 1 if r > M32 else 0; ECX = r & M32
    # 0x047dc9 ADC EDI,EBP
    r = (EDI & M32) + (EBP & M32) + CF; CF = 1 if r > M32 else 0; EDI = r & M32
    # 0x047dcb ADC EBP,EBP
    r = (EBP & M32) + (EBP & M32) + CF; CF = 1 if r > M32 else 0; EBP = r & M32
    # 0x047dcd POP EAX  (next = push_edx = a_hi)
    EAX = push_edx
    # 0x047dce XCHG EAX,EBX
    EAX, EBX = EBX, EAX
    # 0x047dcf MUL EBX  -> EDX:EAX = EAX(b_lo) * EBX(a_hi)
    prod = (EAX & M32) * (EBX & M32)
    EAX = prod & M32; EDX = (prod >> 32) & M32
    # 0x047dd1 ADD ECX,EAX
    r = (ECX & M32) + (EAX & M32); CF = 1 if r > M32 else 0; ECX = r & M32
    # 0x047dd3 ADC EDI,EDX
    r = (EDI & M32) + (EDX & M32) + CF; CF = 1 if r > M32 else 0; EDI = r & M32
    # 0x047dd5 ADC EBP,0
    r = (EBP & M32) + 0 + CF; CF = 1 if r > M32 else 0; EBP = r & M32
    # 0x047dd8 MOV EAX,EBX
    EAX = EBX
    # 0x047dda POP EDX  (next = push_esi = b_hi)
    EDX = push_esi
    # 0x047ddb MUL EDX  -> EDX:EAX = EAX(a_hi) * EDX(b_hi)
    prod = (EAX & M32) * (EDX & M32)
    EAX = prod & M32; EDX = (prod >> 32) & M32
    # 0x047ddd ADD EAX,EDI
    r = (EAX & M32) + (EDI & M32); CF = 1 if r > M32 else 0; EAX = r & M32
    # 0x047ddf ADC EDX,EBP  (SF from here feeds 0x047de4 JS)
    r = (EDX & M32) + (EBP & M32) + CF
    CF = 1 if r > M32 else 0
    EDX = r & M32
    SF = 1 if (EDX & 0x80000000) else 0
    OF = 0
    # 0x047de1 MOV EBX,ECX
    EBX = ECX
    # 0x047de3 POP ECX  (last = exp_adj_ecx)
    ECX = exp_adj_ecx
    # --- round/normalize 0x047de4 JS 0x047dee (SF set) ---
    if not SF:
        # 0x047de6 ADD EBX,EBX
        r = (EBX & M32) + (EBX & M32); CF = 1 if r > M32 else 0; EBX = r & M32
        # 0x047de8 ADC EAX,EAX
        r = (EAX & M32) + (EAX & M32) + CF; CF = 1 if r > M32 else 0; EAX = r & M32
        # 0x047dea ADC EDX,EDX
        r = (EDX & M32) + (EDX & M32) + CF; CF = 1 if r > M32 else 0; EDX = r & M32
        # 0x047dec DEC CX
        CX = (ECX & M16) - 1
        CF = 1 if (ECX & M16) == 0 else 0
        ESI_UNUSED = 0
        ECX = (ECX & 0xFFFF0000) | (CX & M16)
    # --- 0x047dee ADD EBX,EBX ---
    r = (EBX & M32) + (EBX & M32)
    CF = 1 if r > M32 else 0
    EBX = r & M32
    ZF = 1 if EBX == 0 else 0
    if CF:                # 0x047df0 JNC not taken (CF==1)
        if ZF:            # 0x047df2 JNZ not taken (ZF==1, EBX==0) -> execute OR-ESI block
            # 0x047df4 OR ESI,ESI
            ESI = ESI | ESI
            ZF = 1 if ESI == 0 else 0
            # 0x047df6 SETNZ BL
            BL = 0 if ZF else 1
            # 0x047df9 SHR EBX,1
            CF = EBX & 1
            EBX = (EBX >> 1) & M32
            if not CF:    # 0x047dfb JC not taken
                # 0x047dfd MOV ESI,EAX; 0x047dff SHR ESI,1
                ESI = (EAX >> 1) & M32
            # 0x047e01 ADC EAX,0
            r = (EAX & M32) + 0 + CF; CF = 1 if r > M32 else 0; EAX = r & M32
            # 0x047e04 ADC EDX,0
            r = (EDX & M32) + 0 + CF; CF = 1 if r > M32 else 0; EDX = r & M32
        else:
            # JNZ taken -> 0x047e01 ADC EAX,0 (CF from 0x047dee ADD EBX,EBX)
            r = (EAX & M32) + 0 + CF; CF = 1 if r > M32 else 0; EAX = r & M32
            # 0x047e04 ADC EDX,0
            r = (EDX & M32) + 0 + CF; CF = 1 if r > M32 else 0; EDX = r & M32
    if not CF:            # 0x047e07 JNC
        goto_norm_end = True
    else:
        # 0x047e09 RCR EDX,1
        old = EDX; EDX = ((EDX >> 1) | (CF << 31)) & M32; CF = old & 1
        # 0x047e0b RCR EAX,1
        old = EAX; EAX = ((EAX >> 1) | (CF << 31)) & M32; CF = old & 1
        # 0x047e0d INC CX
        old16 = ECX & M16; CF = 1 if old16 == 0xFFFF else 0
        CX = (old16 + 1) & M16
        ECX = (ECX & 0xFFFF0000) | CX
        # 0x047e0f CMP CX,0x7fff
        CF = 1 if CX < 0x7FFF else 0
        ZF = 1 if CX == 0x7FFF else 0
        if ZF:            # 0x047e14 JZ -> 0x047e3d OVERFLOW
            ECX = (ECX & 0xFFFF0000) | 0x7FFF
            EDX = 0x80000000; EAX = 0
            return _epilogue(ECX, EAX, EDX, EBX, ESI)
    # --- 0x047e1a OR CX,CX ---
    CX = ECX & M16
    ZF = 1 if CX == 0 else 0
    SF = 1 if (CX & 0x8000) else 0
    OF = 0
    if not (SF == OF and not ZF):   # 0x047e1d JG (signed greater: SF==OF and !ZF)
        if not ZF:      # 0x047e1f JNZ
            # 0x047e25 NEG CX
            CX = (-CX) & M16
        else:
            # 0x047e21 MOV CL,1
            CX = 1
        # 0x047e28 SUB EBX,EBX
        EBX = 0
        # 0x047e2a SHRD EAX,EDX,CL (x86 masks CL to 5 bits for 32-bit dst)
        CL = CX & 31
        if CL == 0:
            pass
        else:
            EAX = ((EAX >> CL) | (EDX << (32 - CL))) & M32
        # 0x047e2d SHRD EDX,EBX,CL (EBX=0)
        if CL:
            EDX = (EDX >> CL) & M32
        # 0x047e30 SUB CX,CX
        CX = 0
        ECX = (ECX & 0xFFFF0000) | CX
    # --- epilogue 0x047e33 ---
    return _epilogue(ECX, EAX, EDX, EBX, ESI)

def _epilogue(ECX, EAX, EDX, EBX, ESI):
    """0x047e33: ADD ECX,ECX; 0x047e35: RCR CX,1; 0x047e38: MOV ESI,ECX; RET -> (EAX, EDX, ESI)"""
    r = (ECX & M32) + (ECX & M32)
    CF = 1 if r > M32 else 0
    ECX = r & M32
    # RCR CX,1 (16-bit)
    old = ECX & M16
    cx = ((old >> 1) | (CF << 15)) & M16
    ECX = (ECX & 0xFFFF0000) | cx
    ESI = ECX
    return (EAX & M32, EDX & M32, ESI & M32)

if __name__ == "__main__":
    # sanity spot-checks
    print("zero a:", ref(0,0,5,3,0x20000000))
    print("normal:", ref(0x12345678,0x00000001,0x9abc0123,0x00000002,0x007f0000))
    print("done")
