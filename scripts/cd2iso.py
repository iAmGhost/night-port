#!/usr/bin/env python3
"""Convert a MODE1/2352 CD data track (raw .IMG, first track) to a 2048-byte ISO9660 image.

Usage: cd2iso.py <raw.img> <out.iso> [num_sectors]
If num_sectors is omitted, derived from the CUE's next-track index (pass via env or default to full data track).
User data in a MODE1 2352 sector is at offset 16, length 2048.
"""
import sys, os

def main():
    raw, out = sys.argv[1], sys.argv[2]
    # Track 1 ends where track 2 (AUDIO) begins: 08:14:62 -> frames
    if len(sys.argv) > 3:
        nsec = int(sys.argv[3])
    else:
        nsec = (8*60 + 14)*75 + 62  # 37112 frames (track2 INDEX01)
    SEC = 2352
    with open(raw, 'rb') as f:
        f.seek(0)
        need = nsec * SEC
        buf = f.read(need)
    mv = memoryview(buf)
    with open(out, 'wb') as o:
        for i in range(nsec):
            base = i * SEC
            o.write(mv[base+16 : base+16+2048])
    print(f"wrote {out}: {nsec} sectors -> {os.path.getsize(out)} bytes")

if __name__ == '__main__':
    main()
