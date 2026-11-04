#!/usr/bin/env python3
# Independent reference for fn_048e60 (0x048e60) 128-bit fixed-point normalize + Newton divide.
# Mirrors the P-code exactly; used to generate expected outputs for the C differential test.

def to_i16(x):
    x &= 0xffff
    return x - 0x10000 if x >= 0x8000 else x

def f(p0, p1, p2):
    p0 &= 0xffffffff; p1 &= 0xffffffff; p2 &= 0xffffffff
    u2 = p2 & 0xffff
    u5 = p0
    u3 = u4 = u7 = u1 = b8 = 0
    i6 = 0
    if u5 == 0:
        if p1 != 0:
            if u2 == 0x7fff:
                return (p0, p1, p2)          # early return (unchanged)
            # else fall into MAIN
        else:
            if (u2 & 0x7fff) == 0:
                return (p0, p1, p2)          # early return (unchanged)
            p2 = (p2 & 0xffff0000) | 0xffff  # fallthrough
            p1 = 0; p0 = 0
            p1 |= 0xc0000000
            return (p0, p1, p2)
    # MAIN
    if ((u2 & 0x7fff) == 0x7fff) or ((u2 >> 15) & 1):
        p1 |= 0xc0000000                      # ovp
        return (p0, p1, p2)
    if ((u2 & 0x7fff) == 0) or ((p1 >> 31) & 1):
        u3 = p1; u7 = 0
        u2 = ((u2 & 0x7fff) + 0xc001) & 0xffff
        u4 = u5
        if (u2 & 1) == 0:
            u7 = u3 & 1
            u3 = u3 >> 1
            u4 = ((((u7 & 1) << 32) | u5) >> 1) & 0xffffffff
            u7 = ((1 if (u5 & 1) else 0) << 0x1f) & 0xffffffff
        ne = ((to_i16(u2) >> 1) + 0x3fff) & 0xffff
        p2 = (p2 & 0xffff0000) | ne
        u5 = (((u3 & 0xffffffff) | 0x100000000) >> 1) & 0xffffffff
        if u3 == 0xffffffff:
            i6 = -1
            if u4 == 0xffffffff:
                p0 = u4; p1 = (i6 + b8) & 0xffffffff
                return (p0, p1, p2)
            u3 = ((((u4 << 32) | u7) // 0xffffffff)) & 0xffffffff
            u5 = 0xffffffff
        else:
            while True:
                u1 = ((((u3 << 32) | u4) // u5)) & 0xffffffff
                if (u5 - 1) <= u1:
                    break
                ssum = (u5 + u1) & 0xffffffff
                c = 1 if (u5 + u1) >= 0x100000000 else 0
                u5 = ((((c << 32) | ssum) >> 1)) & 0xffffffff
            r = (((u3 << 32) | u4) % u5)
            u3 = ((((r << 0x20) | u7) // u5)) & 0xffffffff
            u5 = (u5 + u1) & 0xffffffff
        i6 = (((u5 & 0xffffffff) | 0x100000000) >> 1)
        u4 = (((( (1 if (u5 & 1) else 0) << 32) | u3) >> 1)) & 0xffffffff
        u5 = 1 if (u3 & 1) else 0
        b8 = 1 if (u4 + u5) >= 0x100000000 else 0
        u4 = (u4 + u5) & 0xffffffff
        p0 = u4; p1 = (i6 + b8) & 0xffffffff
        return (p0, p1, p2)
    # check2 false: fallthrough
    p2 = (p2 & 0xffff0000) | 0xffff
    p1 = 0; p0 = 0
    p1 |= 0xc0000000
    return (p0, p1, p2)

cases = [
    (0x12345678, 0x80000001, 0x00000000),   # Newton, no div2 (exp=0)
    (0x00000001, 0x80000002, 0x00000001),   # Newton, div2 branch (exp odd)
    (0x00000000, 0x00000005, 0x00000000),   # u5=0, p1!=0, exp=0 -> Newton (u5 reassigned)
    (0x00000000, 0x00000000, 0x00000000),   # u5=0,p1=0,exp=0 -> early return
    (0x00000000, 0x00000000, 0x00000005),   # u5=0,p1=0,exp=5 -> fallthrough (zero+ovp)
    (0x00000000, 0x00000001, 0x00007fff),   # u5=0,p1!=0,exp=0x7fff -> early return
    (0x12345678, 0x12345678, 0x00008000),   # exp=0x8000, (exp&0x7fff)=0 -> Newton
    (0x12345678, 0x12345678, 0x00007fff),   # exp=0x7fff -> ovp (p1|=0xc0000000)
    (0x0000abcd, 0x80000001, 0x00003fff),   # p1 bit31 set, exp nonzero -> Newton
    (0x00000000, 0xffffffff, 0x00000000),   # u3==0xffffffff special (p1=-1), exp=0
]
for (a, b, c) in cases:
    r = f(a, b, c)
    print(f"in=({a:08x},{b:08x},{c:08x}) out=({r[0]:08x},{r[1]:08x},{r[2]:08x})")
