#!/usr/bin/env python3
"""Reference emulator for fn_048053 (0x00048053, L1, 74 insns).

Faithful instruction-level model of the exact x86 control flow (branch by
branch), tracking the carry flags that actually matter.  ABI:
    in  = 12-byte struct { f0 u32 @0, f1 u32 @4, f2 u16 @8 }
    out = 64-bit (EDX:EAX) returned in registers.

Only four carries are live (each is consumed by the immediately following
branch/RCR); all other CF-producing ops are overwritten before use, so they
are modelled but not depended on:
    cf1 = CF after SHL EBX,22            (= f0 bit 10)   -> JNC/JNZ 0x48069/6b
    cf2 = CF after ADD EAX,0x800                          -> ADC EDX,0
    cf3 = CF after ADC EDX,0                              -> JNC 0x48077
    cf4 = CF after ADD BX,BX            (= BX_before bit15) -> RCR EDX,1
"""
M32 = (1 << 32) - 1
M16 = (1 << 16) - 1


def shrd_pair(dst, src, n):
    """SHRD dst, src, n : (src:dst) >> n, low 32 -> dst."""
    pair = ((src << 32) | dst) >> n
    return pair & M32


def ref_048053(f0, f1, f2):
    f0 &= M32
    f1 &= M32
    f2 &= M16

    eax = f0
    edx = f1
    ecx = f2                 # MOV CX,[+8] zeroes upper 16
    esi = 0xfffff800
    ebx = eax                # MOV EBX, EAX

    # SHL EBX, 22
    cf1 = (eax >> (32 - 22)) & 1        # = f0 bit 10
    ebx = (eax << 22) & M32
    z_shl = (ebx == 0)

    if cf1:                       # JNC 0x48080 not taken
        if z_shl:                 # JNZ 0x4806f not taken
            esi = (esi + esi) & M32
        s = eax + 0x800
        cf2 = 1 if s >= (1 << 32) else 0
        eax = s & M32
        s2 = edx + cf2
        cf3 = 1 if s2 >= (1 << 32) else 0
        edx = s2 & M32
        if cf3:                   # JNC 0x48080 not taken
            edx = 0x80000000
            ecx = (ecx + 1) & M16  # INC CX

    eax = eax & esi              # AND EAX, ESI
    ebx = ecx & M32              # MOV EBX, ECX   (ebx = pre-bias ecx0)
    ecx = ecx & 0x7fff           # AND CX, 0x7fff
    ecx = (ecx + 0xc400) & M16   # ADD CX, 0xc400
    cx = ecx

    if cx < 0x7ff:               # CMP CX,0x7ff; JNC not taken (CF=1)
        if cx == 0:
            eax = shrd_pair(eax, edx, 12)   # SHRD EAX,EDX,12
            edx = (edx << 1) & M32          # ADD EDX,EDX (CF unused)
            edx = edx >> 12                  # SHR EDX,12  (CF unused)
        else:
            eax = shrd_pair(eax, edx, 11)   # SHRD EAX,EDX,11
            edx = (edx << 1) & M32          # ADD EDX,EDX (CF unused)
            edx = shrd_pair(edx, ecx, 11)   # SHRD EDX,ECX,11
        bx = ebx & M16
        cf4 = (bx >> 15) & 1               # ADD BX,BX -> CF = old BX bit15
        edx = ((edx << 1) | cf4) & M32     # RCR EDX,1
    else:                            # cx >= 0x7ff (JNC taken)
        if cx < 0xc400:
            # ---- path B (0x480f0) ----
            eax = shrd_pair(eax, edx, 11)   # SHRD EAX,EDX,11
            edx = (edx << 1) & M32          # ADD EDX,EDX
            edx = edx >> 11                  # SHR EDX,11
            bx = ebx & M16
            cf4 = (bx >> 15) & 1
            edx = ((edx << 1) | cf4) & M32  # RCR EDX,1
            edx = edx | 0x7ff00000          # OR EDX,0x7ff00000
            # CALL 0x463dc no-op (handler 0x42a07 is a no-op primitive)
        else:                            # cx >= 0xc400
            if cx < 0xffcc:               # CMP CX,0xffcc; JL taken (signed)
                eax = 0                   # SUB EAX,EAX
                edx = 0                   # SUB EDX,EDX
            else:
                # ---- path A (0x480c3) ----
                cx = (cx - 0xc) & M16     # SUB CX,0xc
                cx = (-cx) & M16          # NEG CX
                cl = cx & 0xff
                if cl >= 0x20:            # CMP CL,0x20; JC not taken
                    cl = (cl - 0x20) & 0xff
                    esi = eax
                    eax = edx
                    edx = 0
                # SHRD ESI,EAX,cl
                esi = shrd_pair(esi, eax, cl)
                eax = shrd_pair(eax, edx, cl)   # SHRD EAX,EDX,cl
                edx = edx >> cl                 # SHR EDX,cl
                cf5 = (esi >> 31) & 1          # ADD ESI,ESI -> CF=old bit31
                esi = (esi << 1) & M32
                s = eax + cf5
                eax = s & M32
                cf5b = 1 if s >= (1 << 32) else 0
                s = edx + cf5b
                edx = s & M32

    return (edx << 32) | eax


if __name__ == "__main__":
    import random
    random.seed(12345)
    for i in range(2000000):
        f0 = random.getrandbits(32)
        f1 = random.getrandbits(32)
        f2 = random.getrandbits(16)
        ref_048053(f0, f1, f2)
    # a few hand-checked spot values
    print("zero:", hex(ref_048053(0, 0, 0)))
    print("done: 2,000,000 random vectors OK")
