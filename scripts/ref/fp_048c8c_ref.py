#!/usr/bin/env python3
"""Reference for fn_048c8c (0x00048c8c, L2, 467B, 163 insns). IN-PLACE single-arg:
fn_048c8c(a), a={lo,hi,exp(u16)} (fx128: lo@[0], hi@[4], exp@[8] word).
STATUS 2026-07-11: RUNS (pass-2 carry bug fixed) but UNVALIDATED / NOT YET USED FOR A C MIRROR.
  - Pass-2 normalize carry fixed (EAX->EDX, matching `ADD EAX,EAX; ADC EDX,EDX`); now
    completes without hanging on normalized fx128 inputs (hi top-bit set, exp~0x3ffe).
  - OPEN CONCERN (verify before C mirror): the math chain yields t1 == sq EXACTLY, i.e.
    fn_04794a(sq,(0x1bf21f8c,0xa6829a79),0x4003xxxx) returned its input unchanged for
    nonzero a (the 0x04796d path). Downstream t2 overflows to the {0,0x80000000,0x7fff}
    +inf sentinel, so the final result is CONSTANT per sign (pos->{0,0x80000000,0x0000},
    neg->{0,0,0xffff}). Confirm fn_04794a's 0x04796d-path semantics (multiply vs normalize
    vs a-op) + a ground-truth known value BEFORE trusting this ref. The C-vs-Python diff
    would pass even with a shared trace bug, so do NOT skip this check.

Trace-faithful to analysis/disasm/dis_048c8c_raw.txt; composes the verified callee refs
(fp_0487b0_ref._rd4e/_r94a, fp_047b79_ref.emulate). See docs/12-functions/STATUS.md.

7-step algorithm (register-level mirror of the x86):
 1) ESI=0 (neg flag); CX=orig exp; clear exp bit15 in memory (a_exp &= 0x7fff).
 2) zero-test on (cleared_exp, hi, lo): if all zero -> carry=0, a stays 0, skip 3-4.
 3) if CX bit15 (orig exp negative): ESI=-1; a=04791a(a,{0,0x80000000},0xbfff) in-place;
    a_exp ^= 0x8000 (restore sign). Else a_exp = cleared exp.
 4) normalize: BX=exp+4; ECX=0; (lo,hi)=a;
    while BX>=0x3fff: (EAX:EDX:ECX)<<=1, BX--   (ECX accumulates shift-out bits = carry flag)
    if (hi||lo)!=0: while hi>=0: (lo,hi)<<=1, BX--; then BX-=4  else BX=0;
    a={lo,hi,BX}.  ECX = carry flag (drives step 6).
 5) math chain (square + 4x 04794a/fx_mul64 + 047b79) -- see ref_048c8c body.
 6) poly-eval loop: for each set bit of carry flag (LSB first), a=047d4e(coeff[i],a,(a.exp<<16)|coeff.exp);
    coeff from 10-byte table @0x48c64.
 7) a.exp += (int16)neg_flag; a=04791a(a,{0,0x80000000},0xbfff) in-place (final scale).
"""
from fp_0487b0_ref import _rd4e, _r94a
import fp_047b79_ref as rb79_mod

M32 = 0xffffffff
def m32(x): return x & M32

def _r91a(a_lo, a_hi, a_exp, b_lo, b_hi, exp_hi):
    # fn_04791a (fx4794a_store): esi=(exp_hi<<16)|a_exp; r=04794a(a,b,esi); out.exp=a_exp (preserved)
    esi = m32(((exp_hi & 0xffff) << 16) | (a_exp & 0xffff))
    lo, hi = _r94a(a_lo, a_hi, b_lo, b_hi, esi)
    return m32(lo), m32(hi), m32(a_exp & 0xffff)

# coeff table @0x48c64 (10-byte fx128 stride: lo@0, hi@4, exp@8 word).
# REAL coefficients are entries 0-3 (0x48c64-0x48c8c); entry 3 = {0xf9de6484,0xb504f333,0x3fff}
# (same domain-threshold constant as 0x0487b0). From 0x48c8c onward is FUNCTION CODE -- but
# the poly loop iterates the full 32-bit carry ECX (SHR ECX,1) and reads coeff[i] for each
# SET bit, so for high-exp inputs it reads PAST entry 3 into these code bytes. The real
# code does exactly that, so we embed the raw 32-entry extent (faithful byte reads).
# The game's realistic inputs (from FUN_000461cc) keep pass-1 to <=4 iters (exp<=0x3ffe),
# so only entries 0-3 are read in practice; 4-31 matter only for degenerate high-exp inputs.
TBL_48c64 = [
    (0xcc487b15, 0x85aac367, 0x3fff),   # [0] real
    (0xea8bd6e7, 0x8b95c1e3, 0x3fff),   # [1] real
    (0x8db8a96f, 0x9837f051, 0x3fff),   # [2] real
    (0xf9de6484, 0xb504f333, 0x3fff),   # [3] real (domain threshold == 0x0487b0)
    (0x52565755, 0xf88b5351, 0xf62b),   # [4..31] function-code bytes (read faithfully)
    (0x084f8b66, 0x7f096780, 0x8366),
    (0x7500087f, 0x047f8309, 0x7500),
    (0x003f8303, 0x0b666674, 0x79c9),
    (0xff684e1a, 0x680000bf, 0x0000),
    (0x006a8000, 0xdf8bc78b, 0x4fe8),
    (0x80ffffec, 0x66800973, 0x5f8b),
    (0xc3836608, 0x8bc92b04, 0x0457),
    (0x8166078b, 0x723ffffb, 0x030a),
    (0x13d213c0, 0xeb4b66c9, 0x0bef),
    (0x0b0275d2, 0x2b0475c0, 0xebdb),
    (0x78d20b10, 0x13c00308, 0x66d2),
    (0x66f4eb4b, 0x8904eb83, 0x8907),
    (0x89660457, 0x02eb085f, 0xc92b),
    (0x078b5156, 0x6604578b, 0x778b),
    (0x10e6c108, 0x08778b66, 0xd88b),
    (0x1be8ca8b, 0x50fffff0, 0x5652),
    (0x829a79b9, 0x1f8cbba6, 0x1bf2),
    (0xffffe681, 0xce810000, 0x0000),
    (0xf9e84003, 0x8bffffeb, 0x87ec),
    (0x55870845, 0x00758704, 0xd6b9),
    (0xbbec96f0, 0x09d7bfdb, 0xe681),
    (0x0000ffff, 0x0000ce81, 0x3ffa),
    (0xffefd7e8, 0x051ab9ff, 0xe6d5),
    (0x36e187bb, 0xffe681e5, 0x00ff),
    (0x00ce8100, 0xe8400100, 0xebb8),
    (0x1f8bffff, 0xc1044f8b, 0x10c6),
    (0x08778b66, 0xe810c6c1, 0xefa8),
]

def ref_048c8c(a_lo, a_hi, a_exp):
    a_lo, a_hi, a_exp = m32(a_lo), m32(a_hi), m32(a_exp & 0xffff)
    # STEP 1: ESI=0; CX=orig exp; a_exp &= 0x7fff (clear exp bit15 in memory)
    ESI = 0
    CX = a_exp & 0xffff
    a_exp_clr = a_exp & 0x7fff
    all_zero = (a_exp_clr == 0 and a_hi == 0 and a_lo == 0)
    if not all_zero:
        # STEP 3: negate path iff orig exp negative (CX bit15)
        if CX & 0x8000:
            ESI = m32(-1)
            a_lo, a_hi, a_exp = _r91a(a_lo, a_hi, a_exp_clr, 0, 0x80000000, 0xbfff)
            a_exp = (a_exp ^ 0x8000) & 0xffff   # XOR byte[9],0x80 (restore sign)
        else:
            a_exp = a_exp_clr
        # STEP 4: normalize
        BX = (a_exp + 4) & 0xffff
        ECX = 0
        EAX, EDX = a_lo, a_hi
        while BX >= 0x3fff:                       # pass 1: (EAX:EDX:ECX) <<= 1
            c1 = (EAX >> 31) & 1
            c2 = (EDX >> 31) & 1
            c3 = (ECX >> 31) & 1
            EAX = (EAX << 1) & M32
            EDX = ((EDX << 1) | c1) & M32
            ECX = ((ECX << 1) | c2) & M32
            BX = (BX - 1) & 0xffff
        if EDX != 0 or EAX != 0:                  # pass 2: while hi>=0: (lo,hi)<<=1
            while not (EDX & 0x80000000):
                c1 = (EAX >> 31) & 1              # ADD EAX,EAX (CF=EAX bit31)
                EAX = (EAX << 1) & M32
                EDX = ((EDX << 1) | c1) & M32     # ADC EDX,EDX (carry EAX->EDX)
                BX = (BX - 1) & 0xffff
            BX = (BX - 4) & 0xffff                # hi<0: BX -= 4
        else:
            BX = 0
        a_lo, a_hi, a_exp = m32(EAX), m32(EDX), m32(BX & 0xffff)
        carry = m32(ECX)
    else:
        a_lo, a_hi, a_exp = 0, 0, 0
        carry = 0

    # STEP 5: math chain
    esi = m32(((a_exp & 0xffff) << 16) | (a_exp & 0xffff))       # (0x48d23/26)
    sq_lo, sq_hi, sq_exp = _rd4e(a_lo, a_hi, a_lo, a_hi, esi)     # square
    t1_esival = m32(0x40030000 | (sq_exp & 0xffff))
    t1_lo, t1_hi = _r94a(sq_lo, sq_hi, 0x1bf21f8c, 0xa6829a79, t1_esival)
    t2_lo, t2_hi, t2_exp = _rd4e(sq_lo, sq_hi, 0x9d7bfdb, 0xec96f0d6, m32(0x3ffa0000 | (sq_exp & 0xffff)))
    t3_esival = m32(0x40010000 | (t2_exp & 0xffff))
    t3_lo, t3_hi = _r94a(t2_lo, t2_hi, 0xe536e187, 0xe6d5051a, t3_esival)
    esi4 = m32(((a_exp & 0xffff) << 16) | (t2_exp & 0xffff))
    t4_lo, t4_hi, t4_exp = _rd4e(t3_lo, t3_hi, a_lo, a_hi, esi4)
    esi5 = m32(((sq_exp & 0xffff) << 16) | ((t4_exp & 0xffff) ^ 0x8000))
    u_lo, u_hi = _r94a(t4_lo, t4_hi, t1_lo, t1_hi, esi5)
    esi6 = m32(((sq_exp & 0xffff) << 16) | (t4_exp & 0xffff))
    v_lo, v_hi = _r94a(t4_lo, t4_hi, t1_lo, t1_hi, esi6)
    esi7 = m32((((t4_exp & 0xffff) ^ 0x8000) << 16) | (t4_exp & 0xffff))
    r_lo, r_hi, r_exp = rb79_mod.emulate(m32(v_lo), m32(u_lo), m32(u_hi), m32(v_hi), m32(esi7))
    a_lo, a_hi, a_exp = m32(r_lo), m32(r_hi), m32(r_exp & 0xffff)

    # STEP 6: poly-eval loop over coeff table @0x48c64, iterating carry flag (LSB first)
    c = carry
    i = 0
    while c != 0:
        bit = c & 1
        c >>= 1
        if bit:
            c_lo, c_hi, c_exp = TBL_48c64[i]
            a_lo, a_hi, a_exp = _rd4e(c_lo, c_hi, a_lo, a_hi, m32(((a_exp & 0xffff) << 16) | (c_exp & 0xffff)))
        i += 1

    # STEP 7: a.exp += (int16)neg_flag; a = 04791a(a, {0,0x80000000}, 0xbfff) in-place
    a_exp = (a_exp + (ESI & 0xffff)) & 0xffff
    a_lo, a_hi, a_exp = _r91a(a_lo, a_hi, a_exp, 0, 0x80000000, 0xbfff)
    return (m32(a_lo), m32(a_hi), m32(a_exp & 0xffff))

if __name__ == "__main__":
    import random, sys
    random.seed(int(sys.argv[1], 0) if len(sys.argv) > 1 else 1)
    for i in range(20):
        lo = random.getrandbits(32); hi = random.getrandbits(32)
        exp = random.randrange(0x0000, 0x4000)
        r = ref_048c8c(lo, hi, exp)
        print("in lo=%08x hi=%08x exp=%04x -> lo=%08x hi=%08x exp=%04x" %
              (lo, hi, exp, r[0], r[1], r[2]))
