#!/usr/bin/env python3
"""Correct-scale test vectors for fn_03ce0b. e0(=ent[0]) and g0(=g_639e0) are SMALL row counts;
base(=g_639dc) is a video base; e348/s*_e348 are sprite video addrs. dst is a SEPARATE buffer.
Verify all src in [0xa0000,0xb0000), h in (0,200], dst within a 0x14000 buffer."""
WIN_LO, WIN_HI = 0xa0000, 0xb0000
def ok(a): return WIN_LO <= a < WIN_HI
def blits(g4,g0,base,e0,e4,ec,e1a,e348,s1,s2):
    out=[]
    if g4==0:
        sa=base+e348-e0; out.append((sa,e1a,0x140,0xc8,0,0x140))
    elif g0<e4:
        sp1=s1[0x1a]; h1a=0xc8-(e4-g0); do1a=(0xc8-(g0-s1[0x4]))*0x140
        out.append((e348,e1a,0x140,h1a,do1a,0x140))
        h1b=e4-g0; sa1b=s1[0x348]+base-s1[0x0]+sp1*(0xc8-h1b)
        out.append((sa1b,sp1,0x140,h1b,0,0x140))
    elif g0+0xc8>ec:
        h2a=ec-g0; sa2a=e348+(g0-e0)*0x140
        out.append((sa2a,e1a,0x140,h2a,0,0x140))
        h2b=g0-e0; do2b=((0xc8-g0)+e0)*0x140; sa2b=s2[0x348]+base-s2[0x0]
        out.append((sa2b,s2[0x1a],0x140,h2b,do2b,0x140))
    else:
        sa=e348+(g0-e0)*e1a; out.append((sa,e1a,0x140,0xc8,0,0x140))
    return out
def valid(b, dstsize=0x14000):
    for (sa,sp,w,h,doff,dp) in b:
        if not ok(sa) or h<=0 or h>0xc8 or sp<=0: return False
        if doff<0 or doff+h*0x140>dstsize: return False
        if sa+(h-1)*sp+0x140>=WIN_HI: return False
    return True
def S(e0,e4,ec,e1a,e348): return {0x0:e0,0x4:e4,0xc:ec,0x1a:e1a,0x348:e348}
BASE=0x90000; E348=0xa0000
# B0
b0=blits(0,0,BASE,50,0,1000,0x140,E348,None,None)
# B1: g0<e4
b1=blits(1,10,BASE,50,40,1000,0x140,E348,S(0,0,0,0x140,0xa2000),None)
# B2: g0>=e4, g0+0xc8>ec
b2=blits(1,60,BASE,50,40,200,0x140,E348,None,S(0,0,0,0x140,0xa4000))
# B3: g0>=e4, g0+0xc8<=ec
b3=blits(1,60,BASE,50,40,500,0x140,E348,None,None)
for name,inp,bl in [("B0",(0,0,BASE,50,0,1000,0x140,E348),b0),
                    ("B1",(1,10,BASE,50,40,1000,0x140,E348),b1),
                    ("B2",(1,60,BASE,50,40,200,0x140,E348),b2),
                    ("B3",(1,60,BASE,50,40,500,0x140,E348),b3)]:
    print(f"\n== {name} (g4={inp[0]} g0={inp[1]} base={inp[2]:#x} e0={inp[3]} e4={inp[4]} ec={inp[5]} e1a={inp[6]:#x} e348={inp[7]:#x}) ==")
    print("  VALID" if valid(bl) else "  *** INVALID ***")
    for (sa,sp,w,h,doff,dp) in bl:
        print(f"  blit src={sa:#x} sp={sp} w={w:#x} h={h} dst_off={doff:#x} dp={dp:#x}")
