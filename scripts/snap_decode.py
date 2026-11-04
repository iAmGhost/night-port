#!/usr/bin/env python3
"""Turn the guest-side snapper's dumps (NIGHT/CAP/*.DMP, see scripts/rig/snapper/snap.asm) into PNGs.

The TSR writes, per snapshot (24-byte header): "NGSH" magic, u32 version, u32 width, u32 height,
u32 tick, u32 reserved, then a 768-byte DAC (256 x R,G,B read by INT 10h AX=1511h), then width*height
indexed bytes top-down. INT 10h AX=1511h reports 6-bit values, so the palette is scaled to 8 bits only
when every component is <= 63 -- the same conditional the port's DAC model uses (docs/44).

Usage:  python3 scripts/snap_decode.py NIGHT/CAP out_dir
"""
import os
import struct
import sys

from PIL import Image


def decode(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 24 or data[:4] != b"NGSH":
        raise SystemExit(f"{path}: not a SNAP dump (magic {data[:4]!r})")
    version, w, h, tick = struct.unpack_from("<IIII", data, 4)
    need = 24 + 768 + w * h
    if len(data) < need:
        raise SystemExit(f"{path}: truncated ({len(data)} B, need {need})")
    pal = data[24 : 24 + 768]
    pixels = data[24 + 768 : 24 + 768 + w * h]
    if max(pal) > 63:                       # already 8-bit (some emulated DACs report full range)
        table = bytes(pal)
    else:
        table = bytes(v << 2 | v >> 4 for v in pal)
    img = Image.frombytes("P", (w, h), bytes(pixels))   # P mode, so putpalette actually applies
    img.putpalette(table)
    return img.convert("RGB"), tick, version


def main(argv):
    if len(argv) < 2:
        raise SystemExit(__doc__.strip())
    src, dst = argv[1], (argv[2] if len(argv) > 2 else "scratch/orig")
    os.makedirs(dst, exist_ok=True)
    files = sorted(f for f in os.listdir(src) if f.endswith(".DMP"))
    if not files:
        raise SystemExit(f"no .DMP files in {src} -- run SNAP.COM inside DOSBox first (docs/46)")
    for i, name in enumerate(files):
        img, tick, version = decode(os.path.join(src, name))
        out = os.path.join(dst, f"{i:04d}.png")
        img.save(out)
        print(f"{name} -> {out}  {img.size[0]}x{img.size[1]} tick={tick} v{version}")
    print(f"{len(files)} frames decoded into {dst}; compare with scripts/playtest_compare.py")


if __name__ == "__main__":
    main(sys.argv)
