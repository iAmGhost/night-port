"""fp_04876d_ref.py -- differential ref for fn_04876d (0x04876d, 66B, L3).

a=EAX, b=EDX (both fx128 {lo,hi,exp16}). Three in-place steps:
  (1) a = ref_0487b0(a)                     [0x048773 CALL 0x0487b0]
  (2) b = a * b  (out=b)                     [0x04877c CALL 0x047d24, out=EBX=b]
         esi=(b.exp<<16)|a.exp; (lo,hi,esi)=rd4e(a_lo,a_hi,b_lo,b_hi,esi); b.exp=esi&0xffff
  (3) b = b * fx128(0x5c17f0bd,0xb8aa3b29)   [0x048782..a6 tail]
         esi=(b.exp<<16)|0x3fff; (lo,hi,esi)=rd4e(b_lo,b_hi,0x5c17f0bd,0xb8aa3b29,esi); b.exp=esi&0xffff
Returns (a, b) after all steps. Composes the verified fp_0487b0_ref + fp_047d4e_ref.
"""
import fp_0487b0_ref
import fp_047d4e_ref

M32 = 0xffffffff

def ref_04876d(a, b):
    ra = fp_0487b0_ref.ref_0487b0(a[0], a[1], a[2] & 0xffff)
    a = (ra["lo"] & M32, ra["hi"] & M32, ra["exp"] & 0xffff)
    # step 2: b = a*b, out=b (0x047d24 reg-ABI a=EAX,b=EDX,out=EBX; out==b)
    esi = ((b[2] & 0xffff) << 16) | (a[2] & 0xffff)
    rlo, rhi, res = fp_047d4e_ref.ref(a[0], a[1], b[0], b[1], esi)
    b = (rlo & M32, rhi & M32, res & 0xffff)
    # step 3: tail in-place fx_mul64 on b with the fixed constants
    esi2 = ((b[2] & 0xffff) << 16) | 0x3fff
    rlo, rhi, res = fp_047d4e_ref.ref(b[0], b[1], 0x5c17f0bd, 0xb8aa3b29, esi2)
    b = (rlo & M32, rhi & M32, res & 0xffff)
    return a, b

if __name__ == "__main__":
    import random, sys
    random.seed(int(sys.argv[1], 0) if len(sys.argv) > 1 else 1)
    # a few hand-picked + random, covering 0487b0 branches and underflow.
    cases = [
        ((0x11111111, 0x22222222, 0x3ffe), (0x33333333, 0x44444444, 0x3ffe)),
        ((0x0, 0x0, 0x0), (0x12345678, 0x9abcdef0, 0x4000)),           # a underflows
        ((0xb504f333, 0xf9de6484, 0x3ffe), (0xabcdef01, 0x23456789, 0x4001)),  # if-branch boundary
        ((0xb504f333, 0xf9de6484, 0x3ffe), (0x1, 0x0, 0x4000)),
        ((0x80000000, 0x0, 0x3fff), (0x80000000, 0x0, 0x3fff)),        # sentinel-ish
    ]
    for _ in range(10):
        cases.append((
            (random.getrandbits(32), random.getrandbits(32), random.randrange(0x3000, 0x4000)),
            (random.getrandbits(32), random.getrandbits(32), random.randrange(0x3000, 0x4000)),
        ))
    for a, b in cases:
        aa, bb = ref_04876d(a, b)
        print("IN  a=(%08x,%08x,%04x) b=(%08x,%08x,%04x)" % (a[0],a[1],a[2],b[0],b[1],b[2]))
        print("OUT a=(%08x,%08x,%04x) b=(%08x,%08x,%04x)" % (aa[0],aa[1],aa[2],bb[0],bb[1],bb[2]))
        print("---")
