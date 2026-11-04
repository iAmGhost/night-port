#!/usr/bin/env python3
# Independent reference for the float-format subsystem LEAVES (NIGHT.EXE).
# 0x04a2a7 : inf/nan guard -> writes optional '-' + string ptr to *out, returns 1 if inf/nan
# 0x04a9da : frexp-like -> *exp = (exp_field - 0x3fe) (0 if x==0); normalized mantissa side effect
# 0x04a8b9 : write `count` right-aligned zero-padded decimal digits of a uint64, NUL at out[count]
#
# Doubles are passed as (lo, hi) little-endian dwords (bit 0-31, bit 32-63).
import struct, json, sys

def bits(x):
    lo, hi = struct.unpack('<II', struct.pack('<d', x))
    return lo, hi

def a2a7(lo, hi):
    # inf/nan guard. Returns (ret, sign, which, dash).
    word = (hi >> 16) & 0xffff
    if (word & 0x7ff0) != 0x7ff0:
        return dict(ret=0, sign=0, which=None, dash=False)
    sign = (hi >> 31) & 1
    mant = lo | ((hi & 0xfffff) << 32)
    which = 'inf' if mant == 0 else 'nan'
    return dict(ret=1, sign=sign, which=which, dash=(sign == 1))

def a9da(lo, hi):
    # exponent (field - 0x3fe); 0 if x == 0. Also the normalized mantissa double (for side-effect fidelity).
    if not (hi & 0x7fffffff) and lo == 0:
        return 0, 0.0
    field = (hi >> 20) & 0x7ff
    exp = field - 0x3fe
    full = (hi << 32) | lo
    norm = (full & ~(0x7ff << 52)) | (0x3fe << 52)
    mant = struct.unpack('<d', struct.pack('<Q', norm))[0]
    return exp, mant

def a8b9(val, count):
    # `count` right-aligned zero-padded decimal digits of val (low `count` digits), no NUL in return.
    out = []
    for _ in range(count):
        out.append(val % 10)
        val //= 10
    out.reverse()
    return ''.join(str(d) for d in out)

def main():
    vecs = {}
    # --- 0x04a2a7 inf/nan guard ---
    a2a7_cases = []
    for x in [float('inf'), float('-inf'), float('nan'), 1.0, -2.5, 0.0, 3.14, 1e300, -1e-300]:
        lo, hi = bits(x)
        a2a7_cases.append(dict(x=repr(x), lo=lo, hi=hi, expect=a2a7(lo, hi)))
    vecs['a2a7'] = a2a7_cases
    # --- 0x04a9da exponent ---
    a9da_cases = []
    for x in [0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 3.0, -3.0, 2.0, 100.0, -123456.78, 1e10, -1e-10, 3.14159, 2**100, 2**-100]:
        lo, hi = bits(x)
        exp, mant = a9da(lo, hi)
        a9da_cases.append(dict(x=repr(x), lo=lo, hi=hi, expect_exp=exp, expect_mant=repr(mant)))
    vecs['a9da'] = a9da_cases
    # --- 0x04a8b9 digit writer ---
    a8b9_cases = []
    for val in [0, 5, 42, 100, 123456, 999999999, 1234567890123]:
        for count in [1, 3, 4, 5, 6, 10, 16]:
            if count > 16: continue
            a8b9_cases.append(dict(val=val, count=count, expect=a8b9(val, count)))
    vecs['a8b9'] = a8b9_cases
    print(json.dumps(vecs))

if __name__ == '__main__':
    main()
