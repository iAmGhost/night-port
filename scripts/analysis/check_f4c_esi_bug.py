import sys
import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
from fp_047d4e_ref import ref
M32=0xFFFFFFFF; M16=0xFFFF
def rol16(x): x&=M32; return ((x>>16)|(x<<16))&M32
import random
random.seed(1234)
diff=0; same=0; samples=[]
for _ in range(20000):
    a_lo=random.getrandbits(32); a_hi=random.getrandbits(32)
    b_lo=random.getrandbits(32); b_hi=random.getrandbits(32)
    esi=random.getrandbits(32)
    rlo,rhi,resi = ref(a_lo,a_hi,b_lo,b_hi,esi)
    cport_post = rol16(esi)          # C port's post-rd4e ESI (carried rol16)
    real_post  = resi                # real rd4e-returned ESI
    if cport_post != real_post:
        diff+=1
        if len(samples)<5: samples.append((esi,cport_post,real_post))
    else:
        same+=1
print(f"post-rd4e ESI: C-port-model vs real rd4e-returned: diff={diff} same={same} / {diff+same}")
print("samples (esi_input, cport_post, real_post):")
for s in samples: print("  ", hex(s[0]), hex(s[1]), hex(s[2]))
