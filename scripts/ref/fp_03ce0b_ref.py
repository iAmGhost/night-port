#!/usr/bin/env python3
"""Independent reference for fn_03ce0b (0x03ce0b), 4-branch sprite-draw.
blt_rows(a=src, s=src pitch, w=width, h=rows, c=dst, t=dst pitch): copies `h` rows of
`w` bytes from src (pitch s) to dst (pitch t).
Video-arena: src is a flat 0xa0000 video addr -> g_video_ram[src-0xa0000].
Returns list of (src_off,sp,w,h,dst_off,dp) with src/dst as BYTE OFFSETS.
"""

def blits(g639e4, g639e0, ent):
    base = ent["base"]; g0 = g639e0
    e0 = ent[0x0]; e4 = ent[0x4]; ec = ent[0xc]; e1a = ent[0x1a]; e348 = ent[0x348]
    def S(off, pitch, h, dst_off):
        return (off, pitch, 0x140, h, dst_off, 0x140)
    out = []
    if g639e4 == 0:
        sa = base + e348 - e0
        out.append(S(sa, e1a, 0xc8, 0))
    elif g0 < e4:
        s1 = ent["s1"]; sp1 = s1[0x1a]
        h1a = 0xc8 - (e4 - g0)
        do1a = (0xc8 - (g0 - s1[0x4])) * 0x140
        out.append(S(e348, e1a, h1a, do1a))
        h1b = e4 - g0
        sa1b = s1[0x348] + base - s1[0x0] + sp1 * (0xc8 - h1b)
        out.append(S(sa1b, sp1, h1b, 0))
    elif g0 + 0xc8 > ec:
        s2 = ent["s2"]
        h2a = g0 - ec
        sa2a = e348 + (g0 - e0) * 0x140
        out.append(S(sa2a, e1a, h2a, 0))
        h2b = g0 - e0
        do2b = ((0xc8 - g0) + e0) * 0x140
        sa2b = s2[0x348] + base - s2[0x0]
        out.append(S(sa2b, s2[0x1a], h2b, do2b))
    else:
        sa = e348 + (g0 - e0) * e1a
        out.append(S(sa, e1a, 0xc8, 0))
    return out

def show(name, g639e4, g639e0, ent):
    print(f"\n== {name}  (g639e4={g639e4:#x} g639e0={g639e0}) ==")
    for (off,sp,w,h,doff,dp) in blits(g639e4, g639e0, ent):
        print(f"  src={off:#08x} spitch={sp} w={w:#x} h={h} dst_off={doff:#x} dpitch={dp:#x}")

# primary/sib: (base, e0, e4, ec, e1a, e348)
def d(t):
    return {0x0:t[1], 0x4:t[2], 0xc:t[3], 0x1a:t[4], 0x348:t[5]}
def mk(primary, s1=None, s2=None):
    return {"base":primary[0], 0x0:primary[1], 0x4:primary[2], 0xc:primary[3],
            0x1a:primary[4], 0x348:primary[5], "s1":d(s1) if s1 else None, "s2":d(s2) if s2 else None}

V1 = mk((0xa0000,0xa0000,0,1000,320,0xa0000))
V2 = mk((0xa0000,0xa0000,40,1000,320,0xa0000), s1=(0xa0000,0xa0000,0,0,320,0xa0400))
V3 = mk((0xa0000,40,100,200,320,0xa0000), s2=(0xa0000,0xa0000,0,10,340,0xa0200))
V4 = mk((0xa0000,0xa0000,0,1000,320,0xa0000))

show("B0", 0x0, 0, V1)
show("B1", 0x1, 10, V2)
show("B2", 0x1, 100, V3)
show("B3", 0x1, 100, V4)
