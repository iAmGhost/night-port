#!/usr/bin/env python3
"""NIGHT stage-data decoder: .wal (world/segments), .stg (event records), .chr (type table).

Formats reverse-engineered from NIGHT.EXE (see docs/10-gameplay.md §12):

  stage%d.wal  [u32 segCount] then per segment:
      u8  orient            0 = horizontal strip (H=200), 1 = vertical strip (W=320)
      u8  flag              (1 on some drop/transition segments)
      u32 worldX_left       f0 — segment rect in WORLD coordinates
      u32 worldY_top        f1
      u32 worldX_right      f2
      u32 worldY_bottom     f3
      u32 f4                walk-line Y offset inside segment (~110-140)
      u32 f5                floor/bottom Y offset inside segment (~161-195)
      bg:  orient==0: (f2-f0)*200 bytes (sprite-index map)
           orient==1: (f3-f1)*320 bytes
      u8[768]  palette
      u32 n; n * u16

  stage%d.stg  [u32 count] then count * 81-byte records:
      [u8 tag][80B payload]
      tag1 spawn : u8 type @+1, u8 side @+5, u16 X @+9, u16 Y @+0xd,
                   u8 zone @+0x11, u8 posmode @++0x15
      tag2 camera: u8 subtype @+5 (5/6 unlock scroll), u16 X @+9, u16 Y @+0xd
      tag3 trigger zone: u16 X0@+9 Y0@+0xd X1@+0x11 Y1@+0x15
      tag4 : u16 X@+9 Y@+0xd X2@+0x11 Y2@+0x15
      tag5 misc: 4 x u32 @+1,+5,+9,+0xd
      tag6 exit : u16 X@+9 Y@+0xd X2@+0x11 Y2@+0x15, u32 @+0x19 (last wins)

  night%d.chr  101 rows * 116 bytes at offset 0, then 234 bytes zero padding:
      u16 f1, u8 f2, u16 f3, u16 f4, u8 f5, u16 f6, u16 f7, u8[100] arr,
      u16 f8, u16 f9
  In memory the rows land at 0x53e9c + i*0x88 (136 B stride, see loader asm).

Usage:
  decode_stg.py <res_dir>          # cross-check all 9 episodes, print summary
  decode_stg.py <res_dir> -v       # verbose per-record
"""
import struct
import sys
import os

TAG_NAMES = {0: "pad", 1: "spawn", 2: "camera", 3: "zone", 4: "rect", 5: "misc", 6: "exit"}


def parse_wal(path):
    d = open(path, "rb").read()
    pos = 0
    count, = struct.unpack_from("<I", d, pos); pos += 4
    segs = []
    for s in range(count):
        orient, flag = d[pos], d[pos + 1]; pos += 2
        f = struct.unpack_from("<6I", d, pos); pos += 24
        bg = (f[2] - f[0]) * 200 if orient == 0 else (f[3] - f[1]) * 320
        pos += bg + 768
        np_, = struct.unpack_from("<I", d, pos); pos += 4
        u16s = list(struct.unpack_from(f"<{np_}H", d, pos)) if np_ else []
        pos += np_ * 2
        segs.append({
            "idx": s, "orient": orient, "flag": flag, "f": f,
            "x0": f[0], "y0": f[1], "x1": f[2], "y1": f[3],
            "walk_y": f[4], "floor_y": f[5],
            "bg_bytes": bg, "u16s": u16s,
        })
    assert pos == len(d), f"{path}: consumed {pos}/{len(d)} bytes"
    return segs


def parse_stg(path):
    d = open(path, "rb").read()
    count, = struct.unpack_from("<I", d, 0)
    pos = 4
    recs = []
    for _ in range(count):
        tag = d[pos]
        p = d[pos:pos + 81]
        if tag == 1:
            rec = {"tag": 1, "type": p[1], "side": p[5],
                   "x": struct.unpack_from("<H", p, 9)[0],
                   "y": struct.unpack_from("<H", p, 0xD)[0],
                   "zone": p[0x11], "posmode": p[0x15]}
        elif tag == 2:
            rec = {"tag": 2, "subtype": p[5],
                   "x": struct.unpack_from("<H", p, 9)[0],
                   "y": struct.unpack_from("<H", p, 0xD)[0]}
        elif tag == 3:
            rec = {"tag": 3,
                   "x0": struct.unpack_from("<H", p, 9)[0],
                   "y0": struct.unpack_from("<H", p, 0xD)[0],
                   "x1": struct.unpack_from("<H", p, 0x11)[0],
                   "y1": struct.unpack_from("<H", p, 0x15)[0]}
        elif tag in (4, 6):
            rec = {"tag": tag,
                   "x": struct.unpack_from("<H", p, 9)[0],
                   "y": struct.unpack_from("<H", p, 0xD)[0],
                   "x2": struct.unpack_from("<H", p, 0x11)[0],
                   "y2": struct.unpack_from("<H", p, 0x15)[0]}
            if tag == 6:
                rec["u32"] = struct.unpack_from("<I", p, 0x19)[0]
        elif tag == 5:
            rec = {"tag": 5, "u32s": struct.unpack_from("<4I", p, 1)}
        else:
            rec = {"tag": tag, "raw": p}
        recs.append(rec)
        pos += 81
    assert pos == len(d), f"{path}: consumed {pos}/{len(d)} bytes"
    return recs


def parse_chr(path):
    d = open(path, "rb").read()
    rows = []
    for i in range(101):
        o = i * 116
        f1, = struct.unpack_from("<H", d, o)
        f2 = d[o + 2]
        f3, = struct.unpack_from("<H", d, o + 3)
        f4, = struct.unpack_from("<H", d, o + 5)
        f5 = d[o + 7]
        f6, = struct.unpack_from("<H", d, o + 8)
        f7, = struct.unpack_from("<H", d, o + 10)
        arr = d[o + 12:o + 112]  # 100B, sequential (no alignment padding)
        f8, = struct.unpack_from("<H", d, o + 112)
        f9, = struct.unpack_from("<H", d, o + 114)
        rows.append({"id": i, "f1": f1, "f2": f2, "f3": f3, "f4": f4,
                     "f5": f5, "f6": f6, "f7": f7, "arr": arr, "f8": f8, "f9": f9})
    return rows


def world_bounds(segs):
    xs0 = min(s["x0"] for s in segs); xs1 = max(s["x1"] for s in segs)
    ys0 = min(s["y0"] for s in segs); ys1 = max(s["y1"] for s in segs)
    return xs0, xs1, ys0, ys1


def cross_check(res_dir, verbose=False):
    out = []
    ok_all = True
    for ep in range(1, 10):
        wal = parse_wal(os.path.join(res_dir, f"stage{ep}.wal"))
        stg = parse_stg(os.path.join(res_dir, f"stage{ep}.stg"))
        chr_ = parse_chr(os.path.join(res_dir, f"night{ep}.chr"))
        xs0, xs1, ys0, ys1 = world_bounds(wal)
        tags = {}
        types = set()
        errs = []
        for r in stg:
            tags[r["tag"]] = tags.get(r["tag"], 0) + 1
            if r["tag"] == 1:
                types.add(r["type"])
                if r["x"] >= 0xFFFE:
                    r["special"] = True  # camera-relative spawner (1/stage in ep6/7)
                elif not (xs0 <= r["x"] <= xs1):
                    errs.append(f"tag1 X {r['x']} outside world {xs0}..{xs1}")
                row = chr_[r["type"]] if r["type"] < 101 else None
                if row is None or all(v == 0 for v in
                                      (row["f1"], row["f2"], row["f3"], row["f4"],
                                       row["f5"], row["f6"], row["f7"])):
                    errs.append(f"tag1 type {r['type']} not defined in night{ep}.chr")
            if r["tag"] == 6:
                pass
        exits = [r for r in stg if r["tag"] == 6]
        ok = not errs
        ok_all &= ok
        out.append((ep, wal, stg, tags, types, (xs0, xs1, ys0, ys1), exits, errs))
        if verbose:
            print(f"=== ep{ep}: world X {xs0}..{xs1}, Y {ys0}..{ys1}, "
                  f"{len(stg)} recs, tags={tags}")
            for e in errs:
                print(f"  !! {e}")
    return out, ok_all


def main():
    args = [a for a in sys.argv[1:] if a != "-v"]
    verbose = "-v" in sys.argv
    res_dir = args[0] if args else "scratch/res"
    rows, ok_all = cross_check(res_dir, verbose)
    print(f"{'ep':>2} {'segs':>4} {'recs':>4}  tags  types used  world X/Y  exit(last tag6)")
    for ep, wal, stg, tags, types, (xs0, xs1, ys0, ys1), exits, errs in rows:
        t = " ".join(f"{k}:{v}" for k, v in sorted(tags.items()))
        ex = exits[-1] if exits else None
        exs = (f"X {ex['x']}..{ex['x2']} Y {ex['y']}..{ex['y2']}"
               if ex else "NONE (kill-clear)")
        mark = "" if not errs else "  <-- ERRORS"
        print(f"{ep:>2} {len(wal):>4} {len(stg):>4}  {t}")
        print(f"     types {sorted(types)}  world {xs0}..{xs1} x {ys0}..{ys1}  {exs}{mark}")
        for e in errs:
            print(f"     !! {e}")
    print("CROSS-CHECK:", "PASS (9/9)" if ok_all else "FAIL")
    sys.exit(0 if ok_all else 1)


if __name__ == "__main__":
    main()
