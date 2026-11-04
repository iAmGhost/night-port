#!/usr/bin/env python3
"""Dump tag-1 spawn records + .wal segments for an episode (stage-analysis).

Usage: dump_stage_recs.py <res_dir> <ep>
"""
import struct, sys


def parse_wal(path):
    d = open(path, "rb").read()
    pos = 0
    count, = struct.unpack_from("<I", d, pos); pos += 4
    segs = []
    for _ in range(count):
        orient, flag = d[pos], d[pos + 1]
        f0, f1, f2, f3, f4, f5 = struct.unpack_from("<6I", d, pos + 2)
        bglen = (f2 - f0) * 200 if orient == 0 else (f3 - f1) * 320
        pos += 2 + 24
        pos += bglen
        pos += 768
        n, = struct.unpack_from("<I", d, pos); pos += 4
        tail = list(struct.unpack_from(f"<{n}H", d, pos)); pos += n * 2
        segs.append(dict(orient=orient, flag=flag, left=f0, top=f1, right=f2, bottom=f3,
                         f4=f4, f5=f5, tail=tail))
    return segs


def parse_stg(path):
    d = open(path, "rb").read()
    count, = struct.unpack_from("<I", d, 0)
    recs = []
    pos = 4
    for _ in range(count):
        tag = d[pos]
        p = d[pos + 1: pos + 81]
        recs.append((tag, p))
        pos += 81
    return recs


def main():
    res, ep = sys.argv[1], int(sys.argv[2])
    segs = parse_wal(f"{res}/stage{ep}.wal")
    print(f"== ep{ep} .wal segments ==")
    for i, s in enumerate(segs):
        print(f"  seg{i}: orient={s['orient']} flag={s['flag']} rect=({s['left']},{s['top']})-({s['right']},{s['bottom']}) "
              f"f4={s['f4']} f5={s['f5']} tail={s['tail']}")
    recs = parse_stg(f"{res}/stage{ep}.stg")
    print(f"== ep{ep} .stg tag1 spawns (file order) ==")
    print(f"  {'#':>3} {'type':>4} {'side':>4} {'X':>5} {'Y':>5} {'zone':>4} {'posmode':>7}")
    n = 0
    for i, (tag, p) in enumerate(recs):
        if tag != 1:
            continue
        typ, side = p[0], p[4]
        X, = struct.unpack_from("<H", p, 8)          # rec +9
        Y, = struct.unpack_from("<H", p, 12)         # rec +0xd
        zone, posmode = p[16], p[20]                 # rec +0x11, +0x15
        n += 1
        mark = ""
        if X == 0xFFFF and Y == 0xFFFF:
            mark = "  <== SENTINEL 0xFFFF"
        print(f"  {i:>3} {typ:>4} {side:>4} {X:>5} {Y:>5} {zone:>4} {posmode:>7}{mark}")
    print(f"  total tag1: {n}")
    print(f"== ep{ep} tag2 camera recs ==")
    for i, (tag, p) in enumerate(recs):
        if tag != 2:
            continue
        sub = p[4]
        X, = struct.unpack_from("<H", p, 8)
        Y, = struct.unpack_from("<H", p, 12)
        print(f"  rec{i}: subtype={sub} X={X} Y={Y}")
    print(f"== ep{ep} tag3/tag4/tag6 ==")
    for i, (tag, p) in enumerate(recs):
        if tag in (3, 4, 6):
            X, = struct.unpack_from("<H", p, 8)
            Y, = struct.unpack_from("<H", p, 12)
            X2, = struct.unpack_from("<H", p, 16)
            Y2, = struct.unpack_from("<H", p, 20)
            extra = struct.unpack_from("<I", p, 24)[0] if tag == 6 else 0
            print(f"  rec{i} tag{tag}: ({X},{Y})-({X2},{Y2}){(' extra=' + str(extra)) if tag == 6 else ''}")


if __name__ == "__main__":
    main()
