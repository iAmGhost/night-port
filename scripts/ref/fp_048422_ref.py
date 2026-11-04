#!/usr/bin/env python3
# Differential reference for 0x048422 (FPU/fx128 composite, 354B).
# Composes the independently-verified callee refs (04794a, 047b79, 047d4e) and
# mirrors the ported 048f4c/048f9e/04791a logic. Mirrors the asm register flow
# (see STATUS.md spec). The C port composes the C callees; this composes the
# Python callee refs, so the two are independent implementations.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fp_04794a_ref as r94a_mod
import fp_047b79_ref as rb79_mod
import fp_047d4e_ref as rd4e_mod

def rol16(x):
    return ((x << 16) | (x >> 16)) & 0xffffffff

# C fn_04794a(eax,edx,ebx,ecx,esi)->(lo,hi); ref emulate(EAX,EBX,ECX,EDX,ESI)->(EAX,EDX)
def r94a(eax, edx, ebx, ecx, esi):
    lo, hi = r94a_mod.emulate(eax, ebx, ecx, edx, esi)
    return (lo & 0xffffffff, hi & 0xffffffff)

# C fn_047b79(eax,edx,esi,ecx,ebx)->(o_lo,o_hi,o_exp); ref emulate(EAX,EBX,ECX,EDX,ESI)->(EAX,EDX,ESI&0xffff)
def rb79(eax, edx, esi, ecx, ebx):
    o_lo, o_hi, o_si = rb79_mod.emulate(eax, ebx, ecx, edx, esi)
    return (o_lo & 0xffffffff, o_hi & 0xffffffff, o_si & 0xffff)

# C fx_mul64/fn_047d4e(a_lo,a_hi,b_lo,b_hi,esi)->(lo,hi,exp)
def rd4e(a_lo, a_hi, b_lo, b_hi, esi):
    r = rd4e_mod.ref(a_lo, a_hi, b_lo, b_hi, esi)
    return (r[0] & 0xffffffff, r[1] & 0xffffffff, r[2] & 0xffffffff)

def f048f4c(mant_lo, mant_hi, count, arr, exp_base):
    EAX, EDX = arr[0][0], arr[0][1]
    ESI = arr[0][2] & 0xffffffff
    E0 = exp_base & 0xffff
    for i in range(count):
        ESI = rol16(ESI)
        ESI = (ESI & 0xffff0000) | E0
        ESI = rol16(ESI)
        r = rd4e(EAX, EDX, mant_lo, mant_hi, ESI)
        EAX, EDX = r[0], r[1]
        ESI = rol16(r[2])                          # 048f82 ROL on rd4e's RETURNED exp (r[2])
        ESI = (ESI & 0xffff0000) | (arr[i+1][2] & 0xffff)
        ESI = rol16(ESI)
        r = r94a(EAX, EDX, arr[i+1][0], arr[i+1][1], ESI)
        EAX, EDX = r[0], r[1]
    return (EAX & 0xffffffff, EDX & 0xffffffff)

def f048f9e(lo, hi, exp, count, arr):
    esi = ((exp << 16) | exp) & 0xffffffff
    r = rd4e(lo, hi, lo, hi, esi)          # R = a^2
    s = f048f4c(r[0], r[1], count, arr, exp)
    t = rd4e(s[0], s[1], lo, hi, esi)      # t = s * a
    return (t[0] & 0xffffffff, t[1] & 0xffffffff, exp & 0xffff)

def f04791a(lo, hi, exp, b_lo, b_hi, exp_hi):
    esi = ((exp_hi << 16) | (exp & 0xffff)) & 0xffffffff
    r = r94a(lo, hi, b_lo, b_hi, esi)
    return (r[0] & 0xffffffff, r[1] & 0xffffffff, exp & 0xffff)

# Coeff table (0x48312, 90B = 9 fx128, 10-byte stride). Parsed from raw LE bytes.
_RAW = bytes.fromhex(
    "4e18e6c0e4c7d1b5fa3f"
    "290b0108089bc684fbbf"
    "1a26c56c2e02469dfb3f"
    "2a986b0a9db92bbafbbf"
    "71012b9527278ee3fb3f"
    "fc689cdd72244992fcbf"
    "c1adeb94ccccccccfc3f"
    "f28197aaaaaaaaaafdbf"
    "0000000000000080ff3f")
COEFF = []
for _i in range(9):
    _p = _RAW[_i*10:(_i+1)*10]
    COEFF.append((int.from_bytes(_p[0:4], 'little'),
                  int.from_bytes(_p[4:8], 'little'),
                  int.from_bytes(_p[8:10], 'little')))

def ref_048422(lo, hi, exp):
    lo &= 0xffffffff; hi &= 0xffffffff; exp &= 0xffff
    CL = (exp >> 8) & 0xff
    exp = exp & 0x7fff
    if exp == 0x3fff and hi == 0x80000000 and lo == 0:
        # AX=0x3ffe; AND CL,0x80; OR AH,CL; MOV [EDI+8],AX  -> AH=0x3f|sign, AL=0xfe
        return (0x2168c235, 0xc90fdaa2, (((0x3f | (CL & 0x80)) << 8) | 0xfe))
    CH = 0
    if exp >= 0x3fff:
        o = rb79(0, 0x80000000, (exp << 16) | 0x3fff, hi, lo)
        lo, hi, exp = o
        CH |= 2
    run = False
    if exp > 0x3ffd: run = True
    elif exp == 0x3ffd:
        if hi > 0x8930a2f4: run = True
        elif hi == 0x8930a2f4 and lo > 0xf66ab09b: run = True
    if run:
        esi0 = (0x3fff << 16) | exp
        t1 = r94a(lo, hi, 0xc265539e, 0xddb3d742, esi0)
        t2 = rd4e(lo, hi, 0xc265539e, 0xddb3d742, esi0)
        esi2 = (0xbfff << 16) | (t2[2] & 0xffff)
        t3 = r94a(t2[0], t2[1], 0, 0x80000000, esi2)
        o = rb79(t3[0], t3[1], (exp << 16) | 0xbfff, t1[1], t1[0])
        lo, hi, exp = o
        CH |= 1
    lo, hi, exp = f048f9e(lo, hi, exp, 8, COEFF)
    if CH & 1: lo, hi, exp = f04791a(lo, hi, exp, 0x6b9b2c23, 0x860a91c1, 0x3ffe)
    if CH & 2:
        lo, hi, exp = f04791a(lo, hi, exp, 0x2168c235, 0xc90fdaa2, 0xbfff)
        exp = (exp ^ 0x8000) & 0xffff   # 0x048572: XOR [EBX+9],0x80
    if CL & 0x80: exp = (exp ^ 0x8000) & 0xffff   # 0x04857b: XOR [EDI+9],0x80
    return (lo & 0xffffffff, hi & 0xffffffff, exp & 0xffff)

# Fixed test inputs covering every path: sentinel(+/-), exp>=0x3fff, threshold
# (exp>0x3ffd / exp==0x3ffd hi>th / exp==0x3ffd hi==th lo>th), no-blocks, sign.
TEST_INPUTS = [
    (0x00000000, 0x80000000, 0x3fff),  # sentinel (pos)
    (0x00000000, 0x80000000, 0xbfff),  # sentinel (neg sign)
    (0x12345678, 0x9abcdef0, 0x4000),  # exp>=0x3fff + threshold
    (0x11111111, 0x22222222, 0x4000),  # exp>=0x3fff + threshold
    (0x11111111, 0x8930a2f5, 0x3ffd),  # threshold: exp==0x3ffd, hi>th
    (0xf66ab09c, 0x8930a2f4, 0x3ffd),  # threshold: exp==0x3ffd, hi==th, lo>th
    (0xf66ab09b, 0x8930a2f4, 0x3ffd),  # NOT threshold: lo==th (boundary)
    (0x8930a2f4, 0x8930a2f4, 0x3ffd),  # NOT threshold: hi==th, lo<=th
    (0x12345678, 0x9abcdef0, 0x0001),  # no blocks (exp<0x3ffd)
    (0x12345678, 0x9abcdef0, 0x8001),  # sign set, no blocks
    (0xdeadbeef, 0xcafebabe, 0x0000),  # exp==0
    (0x00000000, 0x00000000, 0x0000),  # all-zero
    (0x33333333, 0x44444444, 0x3ffe),  # exp just below threshold
    (0xaaaaaaaa, 0xbbbbbbbb, 0xbfff),  # sign + exp>=0x3fff + threshold
]

if __name__ == '__main__':
    for (lo, hi, exp) in TEST_INPUTS:
        o = ref_048422(lo, hi, exp)
        print("0x%08x,0x%08x,0x%04x => 0x%08x,0x%08x,0x%04x" % (lo, hi, exp, o[0], o[1], o[2]))
