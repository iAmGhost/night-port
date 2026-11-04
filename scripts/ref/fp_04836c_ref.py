#!/usr/bin/env python3
# Python differential ref for fn_04836c (0x04836c, L4, 53 insns).
# out = a OP b (via fn_047b4f -> fn_047b79) with zero/inf special cases and a
# sign-dependent post-scale. Composes the verified fp_048422_ref helpers
# (rb79 = fn_047b79, ref_048422, f04791a).
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fp_048422_ref as r422


def f047b4f(a_lo, a_hi, a_exp, b_lo, b_hi, b_exp):
    # ESI=(b->exp<<16)|a->exp; EAX=a->lo; EDX=a->hi; ECX=b->hi; EBX=b->lo
    return r422.rb79(a_lo & 0xffffffff, a_hi & 0xffffffff,
                     ((b_exp << 16) | a_exp) & 0xffffffff,
                     b_hi & 0xffffffff, b_lo & 0xffffffff)


def ref_04836c(a_lo, a_hi, a_exp, b_lo, b_hi, b_exp):
    a_exp &= 0xffff; b_exp &= 0xffff
    if (a_exp & 0x7fff) == 0:                       # a is zero
        if (b_exp & 0x8000) == 0:
            return (0, 0, 0)
        return (0x2168c235, 0xc90fdaa2, 0x4000)
    if (b_exp & 0x7fff) == 0:                       # b is zero
        return (0x2168c235, 0xc90fdaa2, (a_exp & 0x8000) | 0x3fff)
    ch = (a_exp >> 8) & 0xff
    if ch >= 0x80: ch -= 0x100
    o = f047b4f(a_lo, a_hi, a_exp, b_lo, b_hi, b_exp)
    lo, hi, exp = r422.ref_048422(o[0], o[1], o[2])
    dl = (exp >> 8) & 0xff
    if dl >= 0x80: dl -= 0x100
    if ch < 0:
        if dl > 0:
            lo, hi, exp = r422.f04791a(lo, hi, exp, 0x2168c235, 0xc90fdaa2, 0xc000)
    else:
        if dl < 0:
            lo, hi, exp = r422.f04791a(lo, hi, exp, 0x2168c235, 0xc90fdaa2, 0x4000)
    return (lo & 0xffffffff, hi & 0xffffffff, exp & 0xffff)


if __name__ == "__main__":
    cases = [
        (0x12345678, 0x9abcdef0, 0x0001, 0x11111111, 0x22222222, 0x0002),  # a>0 b>0
        (0x12345678, 0x9abcdef0, 0x8001, 0x11111111, 0x22222222, 0x0002),  # a<0 b>0
        (0x12345678, 0x9abcdef0, 0x0001, 0x11111111, 0x22222222, 0x8002),  # a>0 b<0
        (0x12345678, 0x9abcdef0, 0x8001, 0x11111111, 0x22222222, 0x8002),  # a<0 b<0
        (0x33333333, 0x44444444, 0x3ffe, 0x55555555, 0x66666666, 0x0003),  # a>0 b>0 hi-exp
        (0, 0, 0, 0x11111111, 0x22222222, 0x0002),                          # a=+0 b>0 -> +0
        (0, 0, 0x8000, 0x11111111, 0x22222222, 0x8002),                     # a=-0 b<0 -> neg-zero
        (0x12345678, 0x9abcdef0, 0x0001, 0, 0, 0),                          # a>0 b=0 -> +inf
        (0x12345678, 0x9abcdef0, 0x8001, 0, 0, 0),                          # a<0 b=0 -> -inf
        (0xffffffff, 0xffffffff, 0x0007, 0x80000000, 0, 0x0008),            # a>0 b>0 edge
    ]
    for c in cases:
        r = ref_04836c(*c)
        print("0x%08x,0x%08x,0x%04x, 0x%08x,0x%08x,0x%04x => 0x%08x,0x%08x,0x%04x" %
              (c[0], c[1], c[2], c[3], c[4], c[5], r[0], r[1], r[2]))
