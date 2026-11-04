#!/usr/bin/env python3
"""Parse .wal segment files for all episodes.
Per-segment file layout (from FUN_0003c853 decompile):
  +0x00 u8 orient      (0=horizontal, 1=vertical strip)
  +0x01 u8 flag        (-> mem +0x11; entry-direction byte: 1 = enter from right)
  +0x02 u32 f0 left
  +0x06 u32 f1 top
  +0x0a u32 f2 right
  +0x0e u32 f3 bottom
  +0x12 u32 f4 walkline
  +0x16 u32 f5 floor
  +0x1a u32 (width = right-left, computed by loader)
  +0x1e nodes: 3 x 0x100 bytes
  +0x32e u32 techset   (count of u16 pairs that follow)
  +0x332 techset data: 2*techset bytes (u16 pairs)
"""
import struct, sys, pathlib

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "scratch/res")
for ep in range(1, 10):
    p = root / f"{ep}.wal"
    if not p.exists():
        print(f"ep{ep}: MISSING"); continue
    data = p.read_bytes()
    print(f"== ep{ep}: {p.name} {len(data)} bytes")
    off = 0
    idx = 0
    while off + 0x332 <= len(data):
        orient = data[off]; flag = data[off+1]
        f0, f1, f2, f3 = struct.unpack_from("<IIII", data, off+2)
        f4, f5 = struct.unpack_from("<II", data, off+0x12)
        tech = struct.unpack_from("<I", data, off+0x32e)[0]
        nodes = []
        for n in range(3):
            nb = data[off+0x1e+n*0x100 : off+0x1e+(n+1)*0x100]
            # node header guess: first 16 bytes (b-depth, x, w, tileoff, ...)
            nodes.append(nb[:16].hex())
        hdr = (f"  seg{idx}: o={orient} f={flag} rect=({f0},{f1})-({f2},{f3}) "
               f"w={f2-f0} h={f3-f1} f4={f4} f5={f5} techset={tech}")
        print(hdr)
        for n, h in enumerate(nodes):
            print(f"      node{n}: {h}")
        off += 0x332 + 2 * tech
        idx += 1
    if off != len(data):
        print(f"  !! {len(data)-off} trailing bytes (techset data overrun?)")
