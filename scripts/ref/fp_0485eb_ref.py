#!/usr/bin/env python3
"""Reference for fn_0485eb (0x000485eb): 64-bit fixed-point align-subtract-normalize.

Faithfully mirrors the disassembly (flag-for-flag):
  PUSH ESI; PUSH EDI
  AND ESI,0x7fff; AND EDI,0x7fff; SUB ESI,EDI; JL 0x4864a
  SUB EDI,EDI
0x485ff: CMP ECX,EDX; JNZ 0x4860b
      CMP EBX,EAX; JZ 0x4860d
0x4860b: JNC 0x48612
0x4860d: SUB EAX,EBX; SBB EDX,ECX; STC
0x48612: ADC EDI,EDI; DEC ESI; JL 0x48627
      ADD EAX,EAX; ADC EDX,EDX; JC 0x4860d
      OR EDX,EDX; JS 0x485ff
      JMP 0x48612
0x48627: SUB ESI,ESI; OR EAX,EAX; JNZ 0x48633; OR EDX,EDX; JNZ 0x48633; JMP 0x48648
0x48633: OR EDX,EDX; JS 0x4863e
      ADC EAX,EAX; ADC EDX,EDX; DEC ESI; JMP 0x48633
0x4863e: POP ECX; PUSH ECX; AND ECX,0x7fff; ADD ESI,ECX; JMP 0x48648
0x4864a: ADD ESI,EDI; SUB EDI,EDI
0x4864e: POP ECX; POP EBX; AND EBX,0x8000; OR ESI,EBX
      OR EDX,EDX; JNZ 0x4865e; SUB ESI,ESI
0x4865e: XOR BX,CX; JNS 0x48665; NEG EDI; RET

Register-ABI:
  in : eax=A_lo, edx=A_hi, ebx=B_lo, ecx=B_hi, esi=expA, edi=expB
  out: eax=lo, edx=hi, esi=exp, ebx=(expA&0x8000), bx=((expA&0x8000)^(expB&0xffff)) low16,
       edi=sign/neg flag (NEG EDI if bit7(bx) set)
"""
import ctypes, random, sys

M = 0xFFFFFFFF
def u32(x): return x & M
def s32(x): return ctypes.c_int32(x & M).value

def fn_0485eb(a_lo, a_hi, b_lo, b_hi, expA, expB):
    eax, edx, ebx, ecx = u32(a_lo), u32(a_hi), u32(b_lo), u32(b_hi)
    esi, edi = u32(expA), u32(expB)
    esi0, edi0 = esi, edi
    cf = 0
    pc = 'start'
    for _ in range(3000000):
        if pc == 'start':
            esi = esi & 0x7fff; pc = 'A'
        elif pc == 'A':
            edi = edi & 0x7fff; pc = 'B'
        elif pc == 'B':
            cf = 1 if u32(esi) < u32(edi) else 0; esi = u32(esi - edi); pc = 'C'
        elif pc == 'C':
            pc = 'P4a' if s32(esi) < 0 else 'D'
        elif pc == 'D':
            edi = 0; pc = 'E'
        elif pc == 'E':  # 0x485ff
            cf = 1 if u32(ecx) < u32(edx) else 0
            pc = 'G' if ecx != edx else 'F'
        elif pc == 'F':  # 0x48603
            cf = 1 if u32(ebx) < u32(eax) else 0
            pc = 'H' if ebx == eax else 'G'
        elif pc == 'G':  # 0x4860b
            pc = 'J' if cf == 0 else 'H'
        elif pc == 'H':  # 0x4860d
            cf = 1 if u32(eax) < u32(ebx) else 0; eax = u32(eax - ebx); pc = 'I'
        elif pc == 'I':  # 0x4860f SBB EDX,ECX
            prev = edx; sub = u32(ecx + cf); cf = 1 if prev < sub else 0; edx = u32(edx - sub); pc = 'K'
        elif pc == 'K':  # 0x48611
            cf = 1; pc = 'J'
        elif pc == 'J':  # 0x48612 ADC EDI,EDI
            v = edi * 2 + cf; cf = 1 if v > M else 0; edi = u32(v); pc = 'L'
        elif pc == 'L':  # 0x48614 DEC ESI
            esi = u32(esi - 1); pc = 'N'
        elif pc == 'N':  # 0x48615 JL 0x48627
            pc = 'P27' if s32(esi) < -1 else 'M'
        elif pc == 'M':  # 0x4861b ADD EAX,EAX
            v = eax * 2; cf = 1 if v > M else 0; eax = u32(v); pc = 'O'
        elif pc == 'O':  # 0x4861d ADC EDX,EDX
            v = edx * 2 + cf; cf = 1 if v > M else 0; edx = u32(v); pc = 'P'
        elif pc == 'P':  # 0x4861f JC 0x4860d
            pc = 'H' if cf else 'Q'
        elif pc == 'Q':  # 0x48621 OR EDX,EDX; 0x48623 JS 0x485ff
            pc = 'E' if (edx & 0x80000000) else 'J'
        elif pc == 'P27':  # 0x48627
            esi = 0; pc = 'R'
        elif pc == 'R':  # 0x48629
            pc = 'T' if eax != 0 else 'S'
        elif pc == 'S':  # 0x4862d
            pc = 'T' if edx != 0 else 'U'
        elif pc == 'T':  # 0x48633
            pc = 'V' if (edx & 0x80000000) else 'W'
        elif pc == 'W':  # 0x48637 ADC EAX,EAX
            v = eax * 2 + cf; cf = 1 if v > M else 0; eax = u32(v); pc = 'X'
        elif pc == 'X':  # 0x48639 ADC EDX,EDX
            v = edx * 2 + cf; cf = 1 if v > M else 0; edx = u32(v); pc = 'Y'
        elif pc == 'Y':  # 0x4863b DEC ESI; 0x4863c JMP 0x48633
            esi = u32(esi - 1); pc = 'T'
        elif pc == 'U':  # 0x48631 JMP 0x48648
            pc = 'Z'
        elif pc == 'V':  # 0x4863e
            ecx = u32(edi0 & 0x7fff); esi = u32(esi + ecx); pc = 'Z'
        elif pc == 'P4a':  # 0x4864a ADD ESI,EDI
            esi = u32(esi + edi); pc = 'AA'
        elif pc == 'AA':  # 0x4864c SUB EDI,EDI
            edi = 0; pc = 'Z'
        elif pc == 'Z':  # 0x4864e
            ecx = u32(edi0)
            ebx = u32(esi0 & 0x8000)
            esi = u32(esi | ebx)
            if edx == 0:
                esi = 0
            bx = u32(ebx & 0xFFFF) ^ u32(ecx & 0xFFFF)
            if bx & 0x80:
                edi = u32(-(ctypes.c_int32(edi).value))
            return (eax, edx, esi, ebx, bx, edi)
        else:
            raise Exception('bad pc ' + pc)
    raise Exception('infinite loop at ' + pc)


def gen(n_random=600, seed=12345):
    rng = random.Random(seed)
    vecs = []
    # targeted edge cases
    edge = [
        (0, 0, 0, 0, 0, 0),
        (1, 0, 0, 0, 0, 0),
        (0, 1, 0, 0, 0, 0),
        (0xFFFF, 0, 0xFFFF, 0, 0, 0),        # A==B
        (0x10000, 0, 0xFFFF, 0, 0, 0),       # A slightly > B
        (0xFFFF, 0, 0x10000, 0, 0, 0),       # A < B
        (1, 0, 0, 0, 1, 1),                  # exponent diff
        (0, 0, 1, 0, 5, 1),
        (0x80000000, 0, 0, 0, 0, 0),         # A lo bit31
        (0, 1, 0, 1, 0, 0),
        (M, M, 0, 0, 0, 0),
        (M, M, 1, 0, 0, 0),
        (0, M, 0, 1, 3, 1),
        (0x12345678, 0x9ABCDEF0, 0x11111111, 0x11111111, 7, 2),
        (5, 3, 9, 10, 4, 9),                 # expA < expB (P4a path)
        (100, 50, 1, 1, 4000, 2000),         # big exp diff
        (1, 0, 2, 0, 0x8000, 0),             # expA sign bit
        (1, 0, 2, 0, 0, 0x8001),             # expB sign bit
        (0xDEADBEEF, 0x12345678, 0xCAFEBABE, 0x87654321, 100, 90),
    ]
    for e in edge:
        vecs.append(e)
    for _ in range(n_random):
        vecs.append((rng.getrandbits(32), rng.getrandbits(32),
                     rng.getrandbits(32), rng.getrandbits(32),
                     rng.getrandbits(16), rng.getrandbits(16)))
    # also sample exp ranges that hit the P27 (shift<-1) path
    for _ in range(200):
        vecs.append((rng.getrandbits(32), rng.getrandbits(32),
                     rng.getrandbits(32), rng.getrandbits(32),
                     rng.getrandbits(8), rng.getrandbits(8)))
    return vecs


def main():
    vecs = gen(n_random=250)
    out = []
    out.append('#ifndef VECTORS_0485EB_H\n#define VECTORS_0485EB_H')
    out.append('#include <stdint.h>')
    out.append('typedef struct { uint32_t a_lo,a_hi,b_lo,b_hi,expA,expB; uint32_t lo,hi,exp,ebx,edi; uint16_t bx; } v0485eb_t;')
    out.append(f'static const unsigned v0485eb_n = {len(vecs)};')
    out.append(f'static const v0485eb_t v0485eb_v[{len(vecs)}] = {{')
    for (alo,ahi,blo,bhi,ea,eb) in vecs:
        try:
            lo,hi,exp,ebx,bx,edi = fn_0485eb(alo,ahi,blo,bhi,ea,eb)
        except Exception as e:
            print('FAIL vec=(0x%x,0x%x,0x%x,0x%x,0x%x,0x%x) %s'%(alo,ahi,blo,bhi,ea,eb,e)); raise
        edi32 = ctypes.c_int32(edi).value
        out.append(f'  {{0x{alo:08x}u,0x{ahi:08x}u,0x{blo:08x}u,0x{bhi:08x}u,0x{ea:04x}u,0x{eb:04x}u,'
                   f'0x{lo:08x}u,0x{hi:08x}u,0x{exp:08x}u,0x{ebx:08x}u,0x{u32(edi32):08x}u,0x{bx:04x}u}},')
    out.append('};')
    out.append('#endif')
    with open('port/test/vectors/vectors_0485eb.h', 'w') as f:
        f.write('\n'.join(out) + '\n')
    print(f'wrote port/test/vectors/vectors_0485eb.h with {len(vecs)} vectors')


if __name__ == '__main__':
    main()
