import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
import fp_048422_ref as R
M32=0xffffffff
def f4c_fixed(mant_lo, mant_hi, count, arr, exp_base):
    EAX,EDX = arr[0][0],arr[0][1]; ESI = arr[0][2]&M32; E0 = exp_base&0xffff
    for i in range(count):
        ESI=R.rol16(ESI); ESI=(ESI&0xffff0000)|E0; ESI=R.rol16(ESI)
        r=R.rd4e(EAX,EDX,mant_lo,mant_hi,ESI); EAX,EDX=r[0],r[1]
        ESI=R.rol16(r[2]); ESI=(ESI&0xffff0000)|(arr[i+1][2]&0xffff); ESI=R.rol16(ESI)
        r=R.r94a(EAX,EDX,arr[i+1][0],arr[i+1][1],ESI); EAX,EDX=r[0],r[1]
    return EAX&0xffffffff,EDX&0xffffffff
def f9e_fixed(lo,hi,exp,count,arr):
    esi=((exp<<16)|exp)&M32; r=R.rd4e(lo,hi,lo,hi,esi); s=f4c_fixed(r[0],r[1],count,arr,exp)
    t=R.rd4e(s[0],s[1],lo,hi,esi); return t[0]&M32,t[1]&M32,exp&0xffff
R.f048f9e=f9e_fixed
V=[(0,0x80000000,0x3fff,(0x2168c235,0xc90fdaa2,0x3ffe)),
   (0,0x80000000,0xbfff,(0x2168c235,0xc90fdaa2,0xbffe)),
   (0x12345678,0x9abcdef0,0x4000,(0x2168c235,0xc90fdaa2,0x8000)),
   (0x11111111,0x22222222,0x4000,(0x2168c235,0xc90fdaa2,0x8000)),
   (0x11111111,0x8930a2f5,0x3ffd,(0x6b9b2c23,0x860a91c1,0x0000)),
   (0xf66ab09c,0x8930a2f4,0x3ffd,(0x6b9b2c23,0x860a91c1,0x0000)),
   (0xf66ab09b,0x8930a2f4,0x3ffd,(0x91cb110c,0x8d1eaa1a,0x3ffd)),
   (0x8930a2f4,0x8930a2f4,0x3ffd,(0x78ee5f50,0x8e35f3e0,0x3ffd)),
   (0x12345678,0x9abcdef0,0x0001,(0,0,0x0001)),
   (0x12345678,0x9abcdef0,0x8001,(0,0,0x8001)),
   (0xdeadbeef,0xcafebabe,0x0000,(0,0,0x0000)),
   (0,0,0x0000,(0,0,0x0000)),
   (0x33333333,0x44444444,0x3ffe,(0x6b9b2c23,0x860a91c1,0x0000)),
   (0xaaaaaaaa,0xbbbbbbbb,0xbfff,(0x2168c235,0xc90fdaa2,0x0000))]
same=0; diff=0; deg=0
for i,(lo,hi,exp,frz) in enumerate(V):
    a=R.ref_048422(lo,hi,exp); cl,ch,ce=a[0],a[1],a[2]
    match = (cl,ch,ce)==(frz[0],frz[1],frz[2])
    same+=match; diff+=(not match)
    if lo==0 and hi==0 and (cl==0 and ch==0): pass
    if (cl==0 and ch==0) and exp not in (0x0000,): deg+=1
    print(f"v{i:>2} in=({lo:#010x},{hi:#010x},{exp:#06x}) frozen=({frz[0]:#010x},{frz[1]:#010x},{frz[2]:#06x}) fixed=({cl:#010x},{ch:#010x},{ce:#06x}) {'MATCH' if match else 'DIFF'}")
print(f"\nMATCH={same} DIFF={diff}  degenerate(all-zero-out,nonzero-in-exp)={deg}")
