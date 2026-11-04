#!/usr/bin/env python3
"""Reference for fn_04a9da (0x04a9da), register-ABI "double exp" helper.

Disasm (ESP after prologue = E0-24; caller FUN_0004a48f pushes {EDX=arg6, EAX=arg5, ECX=out_ptr}):
  local0 [ESP]   = [ESP+0x1c] = arg6   (E0+4)
  local4 [ESP+4] = [ESP+0x20] = arg5   (E0+8)
  EAX = [ESP+0x20] = arg5 ; ECX = [ESP+0x1c] = arg6
  if (arg5 & 0x7fffffff) || (arg6 != 0):
      W = [ESP+6] = { local4 high word (low2 of W), EDX_in low word (high2 of W) }
        = ((arg6 & 0xffff) << 16) | ((arg5 >> 16) & 0xffff)   # EDX_in == arg6
      EDX = ((W & 0x7ff0) >> 4) - 0x3fe      # (arith SAR 4, then SUB)
  else:
      EDX = 0
  EAX = [ESP+0x24] = out_ptr (E0+12)
  FLD double [ESP]   # x87-stack op the caller pops (FSTP); no integer effect -> omitted
  [EAX] = EDX        # *out_ptr = exp
  return EAX:EDX     # caller ignores (re-reads *out_ptr)
"""
import ctypes

M = 0xFFFFFFFF
def u32(x): return x & M

def ref(arg6, arg5, out=None):
    edx = 0
    if ((arg5 & 0x7fffffff) != 0) or (arg6 != 0):
        W = (((arg6 & 0xffff) << 16) | ((arg5 >> 16) & 0xffff)) & M
        edx = u32(ctypes.c_int32((W & 0x7ff0) >> 4).value - 0x3fe)  # SAR 4 keeps value<0x800; SUB wraps
        # simpler: (W & 0x7ff0) >> 4 is in [0,0x7f]; subtract 0x3fe with 32-bit wrap
        edx = u32((((W & 0x7ff0) >> 4) - 0x3fe))
    if out is not None:
        out[0] = edx
    return edx  # EDX (EAX=out_ptr ignored by caller)

def main():
    out = []
    out.append('#pragma once')
    out.append('#ifndef VEC_04A9DA_H')
    out.append('#define VEC_04A9DA_H')
    out.append('#include <stdint.h>')
    out.append('typedef struct { uint32_t arg6, arg5, exp; } v04a9da_t;')
    # edge cases: arg5 top-bit-only, arg6 nonzero, both zero, random exp fields
    cases = []
    cases += [(0,0),(0x80000000,0),(0x00010000,0),(0x007f0000,0),(0x7fffffff,0),
              (0x0000ffff,0),(0,0x00000001),(0x00000000,0x12345678),(0x80000000,0xdeadbeef),
              (0xffff0000,0x0000ff00)]
    import random
    random.seed(0x4a9da)
    for _ in range(600):
        a6 = random.getrandbits(32); a5 = random.getrandbits(32)
        cases.append((a6, a5))
    # some with arg5 high bits (exponent region) set
    for _ in range(200):
        a6 = random.getrandbits(32)
        a5 = (random.getrandbits(12) << 20) | random.getrandbits(20)
        cases.append((a6, a5))
    for (a6, a5) in cases:
        e = ref(a6, a5)
        out.append(f'  {{0x{a6:08x}u,0x{a5:08x}u,0x{e:08x}u}},')
    body = '\n'.join(out[5:])
    out2 = out[:5] + [f'static const v04a9da_t v04a9da_v[{len(cases)}] = {{', body, '};',
                      f'static const unsigned v04a9da_n = {len(cases)};', '#endif']
    hdr = '\n'.join(out2)
    with open('scratch/vectors_04a9da.h', 'w') as f:
        f.write(hdr + '\n')
    print(f'wrote scratch/vectors_04a9da.h with {len(cases)} vectors')

if __name__ == '__main__':
    main()
