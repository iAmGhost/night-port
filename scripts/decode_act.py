#!/usr/bin/env python3
"""Python decode of a `.act` — render each action's record and confirm the
record->action mapping (docs/08-animation.md). Byte-exact against the C
`act_decode` (selftest ALL PASS). Uses the identity grayscale palette so the
POSE is what's checked (record structure + frame sequence).

Usage: python3 scripts/decode_act.py [gun|ena-1]
"""
import os, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "..", "scratch", "res")
sys.path.insert(0, HERE)
from make_sheets import load_sprites, write_png_gray, CELL  # reuse sprite parse + PNG


def load_act(path):
    d = open(path, "rb").read()
    c = struct.unpack_from("<I", d, 0)[0]
    off, recs = 4, []
    for _ in range(c):
        n = struct.unpack_from("<I", d, off)[0]
        fp = off + 109                       # 109-byte header (incl. [u32 n])
        fr = struct.unpack_from("<%dH" % n, d, fp) if n else ()
        recs.append(fr)
        off = fp + 10 * n                    # 2n frames + 2*(4n) params
    assert off == len(d), f"{path}: consumed {off}/{len(d)}"
    return recs


# record->action mappings (docs/08-animation.md). gun = verified; ena-1 = the
# same heuristic pick_anim_records() applies in the port (walk/atk/hit).
MAPS = {
    "gun":   [("idle", 0), ("hit", 10), ("punch", 52), ("jump", 57), ("walk", 41), ("kick", 66)],
    "ena-1": [("idle", 0), ("walk", 2), ("atk", 43), ("hit", 5)],
}


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "gun"
    sprites = load_sprites(os.path.join(RES, name + ".spr"))
    recs = load_act(os.path.join(RES, name + ".act"))
    items = MAPS.get(name, [(f"rec{i}", i) for i in range(min(len(recs), 12))])
    ncol = len(items)
    W, H = ncol * CELL, CELL
    buf = bytearray([128] * (W * H))

    def put(x, y, v):
        if 0 <= x < W and 0 <= y < H:
            buf[y * W + x] = v

    print(f"# {name}.act decode — {len(recs)} records (byte-exact to EOF)")
    print(f"  {'action':6s} {'rec':>3s} {'beats':>5s} {'midbeat':>7s} {'sprite':>6s}")
    for ci, (label, rec) in enumerate(items):
        fr = recs[rec]
        n = len(fr)
        b = n // 2 if n > 1 else 0
        f = fr[b]
        w, hh, rows = sprites[f]
        x0, y0 = ci * CELL + 10, 10
        for y in range(hh):
            for x in range(w):
                put(x0 + x, y0 + y, rows[y][x])
        print(f"  {label:6s} {rec:3d} {n:5d} {b:7d} {f:6d}")
    out = os.path.join(HERE, "..", "scratch", "shots", f"actdecode_{name}.png")
    write_png_gray(out, W, H, bytes(buf))
    print("wrote", out)


if __name__ == "__main__":
    main()
