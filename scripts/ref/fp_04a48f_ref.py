#!/usr/bin/env python3
# Reference for 0x04a48f (double -> fixed/exponent decimal-ASCII string).
# Mirrors the C port's control flow; primitives reimplemented to match the C ports
# (fx_double_naninf 11-bit check, fx_double_exp bias 0x3fe, fn_04a3a8 ~= d*10^k,
# fx_mantissa_scale, fn_04a814, fmt_u64_fixed). fn_04a367: *acc = scale(scaled dbl);
# return fn_04a814(arg2, acc).
import struct, math, sys

M64 = (1 << 64) - 1

def f64_bits(d): return struct.unpack('<Q', struct.pack('<d', d))[0]
def bits_f64(b): return struct.unpack('<d', struct.pack('<Q', b & M64))[0]

def fx_double_naninf(d, out):
    b = f64_bits(d); lo = b & 0xffffffff; hi = (b >> 32) & 0xffffffff
    if ((hi >> 20) & 0x7ff) != 0x7ff: return 0
    s = 'nan' if (lo | (hi & 0xfffff)) != 0 else 'inf'
    out.append(('-' + s) if (hi & 0x80000000) else s)
    return 1

def fx_double_exp(d):
    b = f64_bits(d); lo = b & 0xffffffff; hi = (b >> 32) & 0xffffffff
    if (hi & 0x7fffffff) != 0 or lo != 0:
        return ((hi >> 20) & 0x7ff) - 0x3fe
    return 0

def rint(x):
    if math.isinf(x) or math.isnan(x): return x
    return float(round(x))

def fn_04a3a8(d, k):
    if d == math.inf or d == -math.inf:
        d = math.copysign(sys.float_info.max, d)
    elif math.isnan(d):
        d = -sys.float_info.max
    if k >= 0: return d * (10.0 ** k)
    else: return d / (10.0 ** (-k))

def fx_mantissa_scale(d):
    b = f64_bits(d); lo = b & 0xffffffff; hi = (b >> 32) & 0xffffffff
    mant_hi = (hi & 0x000fffff) | 0x100000
    shift = ((hi >> 20) & 0xfff) - 0x433
    v = ((mant_hi << 32) | lo) & M64
    if shift == 0: r = v
    elif shift > 0: r = ((v << shift) & M64) if shift < 64 else 0
    else:
        sa = -shift
        if sa >= 64: r = 0
        else:
            q = v >> sa
            rnd = (v >> (sa - 1)) & 1
            sticky = (v & ((1 << (sa - 1)) - 1)) != 0
            if rnd and sticky: r = q + 1
            elif rnd and (q & 1): r = q + 1
            else: r = q
    r &= M64
    return (r & 0xffffffff), (r >> 32) & 0xffffffff

g_pow10 = [0,1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000,
           10000000000,100000000000,1000000000000,10000000000000,100000000000000,
           1000000000000000,10000000000000000,100000000000000000,1000000000000000000,
           10000000000000000000, M64]

def key_lt(hi, lo, ent):
    ehi = (ent >> 32) & 0xffffffff; elo = ent & 0xffffffff
    return (hi < ehi) or (hi == ehi and lo < elo)

def fn_04a814(start, lo, hi):
    e = start + 1; cnt = 0
    while not key_lt(hi, lo, g_pow10[e]):
        e += 1; cnt += 1
    while True:
        e -= 1
        if not key_lt(hi, lo, g_pow10[e]): break
        e -= 1; cnt -= 1
    return cnt

def fn_04a367(dlo, dhi, arg2, acc, eax_in):
    d = bits_f64(((dhi & 0xffffffff) << 32) | (dlo & 0xffffffff))
    if (eax_in & 0xffffffff) != (arg2 & 0xffffffff):
        d = fn_04a3a8(d, arg2 - (eax_in & 0xffffffff))
    acc[0], acc[1] = fx_mantissa_scale(d)
    return fn_04a814(arg2, acc[0], acc[1])

def fmt_u64_fixed(v, width):
    pos = width; cnt = width
    buf = ['0'] * (width + 1)
    buf[pos] = '\0'
    while True:
        g = v % 10000
        d0 = chr(ord('0') + (g % 10)); d1 = chr(ord('0') + ((g // 10) % 10))
        d2 = chr(ord('0') + ((g // 100) % 10)); d3 = chr(ord('0') + ((g // 1000) % 10))
        v //= 10000; pos -= 4
        buf[pos + 3] = d0; cnt -= 1
        if cnt == 0: break
        buf[pos + 2] = d1; cnt -= 1
        if cnt == 0: break
        buf[pos + 1] = d2; cnt -= 1
        if cnt == 0: break
        buf[pos] = d3; cnt -= 1
        if cnt != 0: continue
        else: break
    return ''.join(buf[:width])

def fn_04a48f(d, width, fmt):
    out = []
    p_cnt = 0; p_flag = 0
    if fx_double_naninf(d, out):
        return out[0], p_cnt, p_flag
    b = f64_bits(d); dlo = b & 0xffffffff; dhi = (b >> 32) & 0xffffffff
    ebx = width; acc = [0, 0]
    goto_render = False; goto_skip = False
    if (dhi & 0x7fffffff) == 0 and dlo == 0:
        goto_render = True
    else:
        if d < 0.0:
            d = -d; p_flag = 0xffffffff
            b = f64_bits(d); dlo = b & 0xffffffff; dhi = (b >> 32) & 0xffffffff
        p_cnt = fx_double_exp(d)
        ea = p_cnt if p_cnt >= 0 else -p_cnt
        n_est = (3 * ea + 5) // 10
        edx = n_est if p_cnt >= 0 else -n_est
        p_cnt = edx
        if fmt == 0x46: ebx += edx
        if ebx < 0:
            goto_skip = True
        else:
            if ebx > 0x10: ebx = 0x10
            flag8 = 0
            if d.is_integer() and ebx > edx:
                flag8 = 1; ebx = edx
            r1 = fn_04a367(dlo, dhi, ebx, acc, p_cnt)
            if r1 != 0:
                p_cnt = (p_cnt + r1) & 0xffffffff
                if fmt == 0x46 or flag8 != 0: ebx += r1
                if ebx < 1: ebx = 1
                if ebx > 0x10: ebx = 0x10
                r2 = fn_04a367(dlo, dhi, ebx, acc, p_cnt)
                if r2 > 0:
                    p_cnt = (p_cnt + 1) & 0xffffffff; ebx += 1
    if not goto_render and not goto_skip:
        if (acc[0] | acc[1]) != 0:
            goto_render = True
        else:
            p_flag = 0; p_cnt = 0
            if fmt == 0x46: ebx -= edx
    # render
    if ebx < 1: ebx = 1
    if ebx > 0x11: ebx = 0x11
    v = acc[0] | (acc[1] << 32)
    s = fmt_u64_fixed(v, ebx) if not goto_render or (acc[0] | acc[1]) != 0 else fmt_u64_fixed(v, ebx)
    return s, p_cnt, p_flag

if __name__ == '__main__':
    F = 0x46
    vecs = [
        (1.0, 10, F), (2.0, 10, F), (0.0, 3, F), (0.0, 5, F),
        (math.inf, 5, F), (-math.inf, 5, F), (float('nan'), 5, F), (-float('nan'), 5, F),
        (-1.0, 10, F), (-2.0, 10, F),
        (2.5, 5, 0x41), (1234.0, 12, F), (1234.5, 12, F),
        (0.5, 8, F), (100.0, 10, F), (1.0, 1, F), (1.0, 3, 0),
        (55.0, 4, F), (98765.0, 10, F),
    ]
    for d, w, f in vecs:
        s, pc, pf = fn_04a48f(d, w, f)
        print(f'{d!r:>20} w={w} f={f:#04x} -> out="{s}" pcnt={pc} pflag={pf:#x}')
