#!/usr/bin/env python3
"""Reference for fn_048a48 (0x00048a48, L3, 502B, 171 insns). In-place:
fn_048a48(a, esi), a={lo,hi,exp(u16)}, esi in {0,1,2}. Trace-faithful to
scratch/dis_048a48_raw.txt; composes the verified callee refs. See
docs/12-functions/STATUS.md (c3258f5 + c3258f5-errata) for the algorithm.

CORRECTIONS vs the earlier (c3258f5) spec:
  * 0485eb expB = 0x3ffe (DI), NOT (a.hi)&0x7fff -- MOV EDX clobbers EDX, not EDI;
    0485eb masks edi &= 0x7fff so high16 (a_ptr) is dropped.
  * select uses a iff ((F0+1)&2)==0 (F0&3 in {0,3}); the CH re-read uses triple iff
    ((F0+3)&2)!=0 (the complement). They are NOT "F0 even/odd".
  * f4c runs iff ((F0+1)&2)!=0 or F1==1; f9e runs iff ((F0+1)&2)==0 or F1==1.
  * The callee ESI high16 (H) is discarded by the CH re-read -> no fn_048f9e ESI out needed.
"""
import fp_0485eb_ref
import fp_047b79_ref as rb79_mod
from fp_0487b0_ref import _f4c, _rd4e, _r94a

M32 = 0xffffffff
def m32(x): return x & M32
def s16(x):
    x &= 0xffff
    return x - 0x10000 if (x & 0x8000) else x

# coeff tables (scratch/048a48_tables_c.txt)
TBL_48982 = [
    (0x856a5359, 0xca963b81, 0x3fce),
    (0x9dc0f88f, 0xd73f9f39, 0xbfd6),
    (0x43684be5, 0xb092309d, 0x3fde),
    (0xaa271c7f, 0xd7322b3f, 0xbfe5),
    (0xb6399c7d, 0xb8ef1d2a, 0x3fec),
    (0x0d00d00c, 0xd00d00d0, 0xbff2),
    (0x88888888, 0x88888888, 0x3ff8),
    (0xaaaaaaaa, 0xaaaaaaaa, 0xbffc),
    (0x00000000, 0x80000000, 0x3fff),
]
TBL_489dc = [
    (0x9dc0f88f, 0xd73f9f39, 0x3fd2),
    (0x03e4e905, 0xc9cba546, 0xbfda),
    (0xc6c4bdaa, 0x8f76c77f, 0x3fe2),
    (0xc4fae397, 0x93f27dbb, 0xbfe9),
    (0x0d00d00d, 0xd00d00d0, 0x3fef),
    (0x0b60b60b, 0xb60b60b6, 0xbff5),
    (0xaaaaaaaa, 0xaaaaaaaa, 0x3ffa),
    (0x00000000, 0x80000000, 0xbffe),
    (0x00000000, 0x80000000, 0x3fff),
]
SCALE_48a36 = [0, 1, 2, 3, 4, 4, 5, 5, 6]

def _r91a(a_lo, a_hi, a_exp, b_lo, b_hi, exp_hi):
    # fn_04791a (fx4794a_store): esi=(exp_hi<<16)|a_exp; r=04794a(a,b,esi); out.exp=a_exp
    esi = m32(((exp_hi & 0xffff) << 16) | (a_exp & 0xffff))
    lo, hi = _r94a(a_lo, a_hi, b_lo, b_hi, esi)
    return m32(lo), m32(hi), m32(a_exp & 0xffff)

def _f9e(a_lo, a_hi, a_exp, count, arr):
    # fn_048f9e: esi=(a.exp<<16)|a.exp; R=a^2; S=f4c(R,count,arr,a.exp); T=S*a; out=(T.lo,T.hi), exp preserved
    esi = m32(((a_exp & 0xffff) << 16) | (a_exp & 0xffff))
    r = _rd4e(a_lo, a_hi, a_lo, a_hi, esi)
    s = _f4c(r[0], r[1], count, arr, a_exp)
    t = _rd4e(s[0], s[1], a_lo, a_hi, esi)
    return m32(t[0]), m32(t[1])

def ref_048a48(a_lo, a_hi, a_exp, esi):
    a_lo, a_hi, a_exp = m32(a_lo), m32(a_hi), m32(a_exp & 0xffff)
    esi = m32(esi & 0xffff)
    # STEP 1: 0485eb align-subtract-normalize (expB=0x3ffe=DI)
    r = fp_0485eb_ref.fn_0485eb(a_lo, a_hi, 0x2168c235, 0xc90fdaa2, a_exp, 0x3ffe)
    # r = (lo, hi, exp, ebx, bx, edi)
    a_lo, a_hi, a_exp = m32(r[0]), m32(r[1]), m32(r[2] & 0xffff)
    r_edi = m32(r[5])
    # STEP 2: sign block iff a.exp bit7
    signpath = (a_exp & 0x8000) != 0
    if signpath:
        a_lo, a_hi, a_exp = _r91a(a_lo, a_hi, a_exp, 0x2168c235, 0xc90fdaa2, 0x3ffe)
    base = m32((r_edi - 1) if signpath else r_edi)
    # STEP 3: F-flags + AL&1 block
    F = (base & 0xffff0000) | ((esi & 0xff) << 8) | (((base & 0xff) + ((esi & 0xff) & 2)) & 7)
    F0 = F & 0xff
    F1 = (F >> 8) & 0xff
    if (F0 & 1) != 0:
        a_lo, a_hi, a_exp = _r91a(a_lo, a_hi, a_exp, 0x2168c235, 0xc90fdaa2, 0xbffe)
        a_exp = (a_exp ^ 0x8000) & 0xffff
    # STEP 4: d2 + poly
    d2 = s16(a_exp) - 0x3ffe
    t_lo, t_hi, t_exp = 0, 0, 0
    if d2 < -0x20:
        t_lo, t_hi, t_exp = 0, 0x80000000, 0x3fff
    else:
        idx = min(8, -d2) if d2 < 0 else 0
        scale = SCALE_48a36[idx]
        if (((F0 + 1) & 2) != 0) or (F1 == 1):
            sq = _rd4e(a_lo, a_hi, a_lo, a_hi, m32(((a_exp & 0xffff) << 16) | (a_exp & 0xffff)))
            f4 = _f4c(sq[0], sq[1], 8 - scale, TBL_489dc[scale:], m32(sq[2]))
            t_lo, t_hi, t_exp = m32(f4[0]), m32(f4[1]), m32(f4[2] & 0xffff)
        if (((F0 + 1) & 2) == 0) or (F1 == 1):
            a_lo, a_hi = _f9e(a_lo, a_hi, a_exp, 8 - scale, TBL_48982[scale:])
    # STEP 5: select (a iff ((F0+1)&2)==0)
    if ((F0 + 1) & 2) == 0:
        s_lo, s_hi, s_exp = a_lo, a_hi, a_exp
        c_lo, c_hi, c_exp = t_lo, t_hi, t_exp
    else:
        s_lo, s_hi, s_exp = t_lo, t_hi, t_exp
        c_lo, c_hi, c_exp = a_lo, a_hi, a_exp
    s_exp = (s_exp ^ 0x8000) & 0xffff if (F0 & 4) else s_exp
    if F1 != 1:
        return (m32(s_lo), m32(s_hi), m32(s_exp))
    # CH block (F1 == 1, esi==1): ch re-read uses triple iff ((F0+3)&2)!=0 (== select_is_a)
    c_exp = (c_exp ^ 0x8000) & 0xffff if ((F0 + 2) & 4) else c_exp
    ESI = m32(((c_exp & 0xffff) << 16) | (s_exp & 0xffff))
    if c_lo == 0 and c_hi == 0 and (ESI & 0x7fff0000) == 0:
        return (0, 0x80000000, 0x7fff)
    o_lo, o_hi, o_esi = rb79_mod.emulate(m32(s_lo), m32(c_lo), m32(c_hi), m32(s_hi), m32(ESI))
    return (m32(o_lo), m32(o_hi), m32(o_esi & 0xffff))

if __name__ == "__main__":
    import random, sys
    random.seed(int(sys.argv[1], 0) if len(sys.argv) > 1 else 1)
    for i in range(20):
        lo = random.getrandbits(32); hi = random.getrandbits(32)
        exp = random.randrange(0x0000, 0x4000)
        esi = random.randrange(0, 3)
        r = ref_048a48(lo, hi, exp, esi)
        print("esi=%d in lo=%08x hi=%08x exp=%04x -> lo=%08x hi=%08x exp=%04x" %
              (esi, lo, hi, exp, r[0], r[1], r[2]))
