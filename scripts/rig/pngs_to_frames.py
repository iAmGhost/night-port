#!/usr/bin/env python3
"""Turn an original-side PNG capture into a frames.bin the screen oracle can read (docs/379).

scripts/playtest_orig_drive.py writes NNNN_<event>_<NN>.png frames; scripts/rig/diff_screen.py compares 40x30
mean-luminance grids in the rig's frames.bin layout.  This bridges the two: the same grid formula and the same
record layout as capture_original.py (REC <qQiiii> + 1200 grid bytes per frame, NGFR header), computed from the
PNGs the anchored driver already labelled -- so a content capture taken through the container becomes a routable
signature stream without re-capturing it.

  uv run --with pillow python3 scripts/rig/pngs_to_frames.py --in /tmp/orig71 --out /tmp/orig71sig [--hz 8]
"""
import argparse, glob, os, re, struct

GRID_W, GRID_H = 40, 30
REC = struct.Struct("<qQiiii")
HDR = struct.Struct("<III")
MAGIC = 0x5247464E            # 'NGFR', the same header diff_screen.py checks


def grid(img):
    g, b, r = img.split()                                     # PIL RGB -> bands
    lum = g.point(lambda v: v)                                # placeholder, replaced below
    from PIL import Image
    W, H = img.size
    out = bytearray(GRID_W * GRID_H)
    rp, gp, bp = r.load(), g.load(), b.load()
    for gy in range(GRID_H):
        y = gy * H // GRID_H
        for gx in range(GRID_W):
            x = gx * W // GRID_W
            R, G, B = rp[x, y], gp[x, y], bp[x, y]
            out[gy * GRID_W + gx] = (R * 77 + G * 150 + B * 29) >> 8      # capture_original.py's weighting
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="inp", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--hz", type=float, default=8.0)
    a = ap.parse_args()
    from PIL import Image
    files = sorted(glob.glob(os.path.join(a.inp, "*.png")))
    if not files:
        raise SystemExit("no PNGs in " + a.inp)
    os.makedirs(a.out, exist_ok=True)
    with open(os.path.join(a.out, "frames.bin"), "wb") as f:
        f.write(HDR.pack(MAGIC, 1, GRID_W * GRID_H))
        for i, path in enumerate(files):
            name = os.path.basename(path)
            seq = int(re.match(r"(\d+)", name).group(1))
            img = Image.open(path).convert("RGB")
            f.write(REC.pack(int(seq * 1000 / a.hz), i, img.size[0], img.size[1], 1, 0))
            f.write(grid(img))
    print(f"{len(files)} frames -> {a.out}/frames.bin (grid {GRID_W}x{GRID_H}, from PNG sizes "
          f"{Image.open(files[0]).size})")


if __name__ == "__main__":
    main()
