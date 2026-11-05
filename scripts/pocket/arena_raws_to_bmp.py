#!/usr/bin/env python3
"""Turn the Pocket shim's arena witness raws into BMPs the label-pairing comparator can read (docs/380 5).

The rig's picture comparator (scripts/playtest_compare_orig_port.py, the basis of docs/91's 348-of-517 at NCC >= 0.90)
wants images; the Pocket shim's dump gives index bytes plus the palette the game's own DAC wrote (main_of.c writes
`vw*vh` index bytes, a `#base_delta=...#ents=N#pal` marker, then 768 palette bytes).  Those two halves are exactly a
paletted image, so this is a decode, not an approximation: index -> RGB through the game's palette, written as a 24-bit
BMP named after the frame it witnessed.
"""
import argparse, glob, os, struct

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="inp", required=True)   # `in` is a Python keyword, so the attribute is `inp`
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    files = sorted(glob.glob(os.path.join(a.inp, "arena", "*.raw")))
    if not files:
        raise SystemExit("no arena raws in " + a.inp + "/arena (run with NIGHT_VIDRAW=1)")
    os.makedirs(a.out, exist_ok=True)
    n = 0
    for path in files:
        base = os.path.basename(path)                      # NNNNNN_WxH.raw
        wh = base.rsplit(".", 1)[0].split("_")[-1]
        w, h = (int(x) for x in wh.split("x"))
        data = open(path, "rb").read()
        need = w * h
        mark = data.find(b"#base_delta=")
        if mark < need or len(data) < mark + 768:
            print("skip %s: layout unexpected (%d bytes, marker at %d)" % (base, len(data), mark)); continue
        idx = data[:need]
        pal = data[len(data) - 768:]                       # the game's own RGB555-ish DAC bytes
        # BMP pixel arrays are bottom-up when the height is positive, so the rows are written in reverse: emitting
        # them top-down hands PIL a vertically mirrored picture, which is why a first comparison scored every frame
        # near 0.1 against frames the same raws match byte-for-byte in sweep_report's index space.
        rows = []
        for y in range(h):
            row = bytearray()
            for x in range(w):
                p = idx[y * w + x] * 3
                row += bytes((pal[p + 2], pal[p + 1], pal[p]))     # BMP rows are B,G,R
            rows.append(bytes(row))
        px = b"".join(reversed(rows))
        pad = (-w * 3) & 3
        info = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 24, 0, len(px), 0, 0, 0, 0)
        off = 14 + len(info)          # the pixel array starts right after the 14-byte file header + 40-byte DIB;
                                      # adding 54 again points PIL 40 bytes past the data ("image file is truncated")
        hdr = b"BM" + struct.pack("<IHHI", 14 + len(info) + len(px), 0, 0, off)
        with open(os.path.join(a.out, base.rsplit(".", 1)[0] + ".bmp"), "wb") as f:
            f.write(hdr + info + (px + bytes(pad) * h if pad else px))
        n += 1
    print("%d BMPs -> %s" % (n, a.out))

if __name__ == "__main__":
    main()
