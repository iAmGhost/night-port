#!/usr/bin/env python3
"""Compare two port dumps of the SAME drive and say where the picture changed.

Purpose (docs/303): a fix to a blit clip must show up as a *localised* screen change, not as a global
residual.  Two dumps of one drive share the seed, the key script and the room art, so a difference that
concentrates in the suspect region and not in a control region is evidence about that region.

Witness frames are only written when the picture changes (rig_sink.c), and their file names are
change-counter ordinals, not timestamps (docs/287), so frames are matched by t_ms from frames.bin.

  python3 scripts/rig/edge_diff.py --a /tmp/rig/le12 --b /tmp/rig/le13 \
      --window 90000-120000 --region 316,320,100,180 --control 100,104,100,180 --png /tmp/rig/edge303

Pixels are 320x200 logical; the witness bins may be a multiple of that (640x480 here), which is handled
by scaling the requested logical box.
"""
import argparse
import os
import struct
import sys
import zlib

REC = struct.Struct("<qQiiii")
HDR = struct.Struct("<III")


def headers(path):
    with open(path, "rb") as f:
        h = HDR.unpack(f.read(12))
        if h[0] != 0x5247464E:
            sys.exit(f"{path}: not an NGFR frames.bin")
        n = h[2]
        out = []
        while True:
            rec = f.read(REC.size)
            if len(rec) != REC.size:
                break
            f.read(n)
            t_ms, frame, w, hh, changed, _pad = REC.unpack(rec)
            out.append((t_ms, frame, w, hh, changed))
        return out


def find(dirpath, frame, w, h):
    for name in os.listdir(os.path.join(dirpath, "port", "frames")):
        if name.startswith(f"{frame:06d}_"):
            return os.path.join(dirpath, "port", "frames", name)
    return None


def load(path, w, h):
    raw = open(path, "rb").read()
    if len(raw) < w * h * 4:
        sys.exit(f"{path}: payload {len(raw)} < {w*h*4}")
    return raw


def lum(raw, w, x, y):
    i = 4 * (y * w + x)
    return raw[i] + raw[i + 1] + raw[i + 2]


def box_diff(A, wa, ha, B, wb, hb, x0, x1, y0, y1):
    """Count logical pixels that differ, and the worst single-pixel delta, inside a logical box.
    Each side is scaled by its own geometry: one dump mixes 320x200 and 640x480 witnesses (docs/287). """
    sa, ta = wa // 320, ha // 200
    sb, tb = wb // 320, hb // 200
    n = d = worst = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            a = lum(A, wa, x * sa, y * ta)
            b = lum(B, wb, x * sb, y * tb)
            n += 1
            if a != b:
                d += 1
                worst = max(worst, abs(a - b))
    return d, n, worst


def crop_png(path, raw, w, h, x0, x1, y0, y1, scale=4):
    s, t = w // 320, h // 200
    cw, ch = (x1 - x0) * scale, (y1 - y0) * scale
    data = bytearray()
    for y in range(y0, y1):
        row = bytearray()
        for x in range(x0, x1):
            v = max(0, min(255, lum(raw, w, x * s, y * t) // 3))
            row += bytes((v, v, v)) * scale
        for _ in range(scale - 1):
            data += bytes(row)
        data += bytes(row)
    raw_stream = b"".join(b"\x00" + data[y * cw * 3:(y + 1) * cw * 3] for y in range(ch))

    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", cw, ch, 8, 0, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(raw_stream, 9))
    out += chunk(b"IEND", b"")
    open(path, "wb").write(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True)
    ap.add_argument("--b", required=True)
    ap.add_argument("--window", default="0-999999999", help="t_ms range for the comparison")
    ap.add_argument("--region", default="316,320,100,180")
    ap.add_argument("--control", default="100,104,100,180")
    ap.add_argument("--max-diff", type=float, default=40.0,
                    help="discard a pair whose WHOLE-frame difference exceeds this percent: a pair in a "
                         "different scene says nothing about a region (docs/303)")
    ap.add_argument("--max", type=int, default=12, help="max frames to pair")
    ap.add_argument("--png", help="write <png>_a.png / <png>_b.png crops of the first pair")
    args = ap.parse_args()
    t0, t1 = (int(v) for v in args.window.split("-"))
    rx0, rx1, ry0, ry1 = (int(v) for v in args.region.split(","))
    cx0, cx1, cy0, cy1 = (int(v) for v in args.control.split(","))

    def pairs(dirpath):
        out = []
        for (t, frame, w, h, changed) in headers(os.path.join(dirpath, "port", "frames.bin")):
            if not (t0 <= t <= t1):
                continue
            p = find(dirpath, frame, w, h)
            if p:
                out.append((t, p, w, h))
        return out

    A, B = pairs(args.a), pairs(args.b)
    print(f"{args.a}: {len(A)} witness frames in window   {args.b}: {len(B)}")
    if not A or not B:
        sys.exit("no witness frames in that window on one side")
    used = min(args.max, len(A), len(B))
    bt = [t for (t, _p, _w, _h) in B]
    tot = {"region_d": 0, "region_n": 0, "ctrl_d": 0, "ctrl_n": 0, "full_d": 0, "full_n": 0}
    worst_region = worst_ctrl = 0
    kept = dropped = 0
    for k in range(used):
        ia = k * len(A) // used
        ta, pa, wa, ha = A[ia]
        j = min(range(len(B)), key=lambda i: abs(bt[i] - ta))   # pair by nearest t_ms, not by index
        tb, pb, wb, hb = B[j]
        Ra, Rb = load(pa, wa, ha), load(pb, wb, hb)
        rd, rn, rw = box_diff(Ra, wa, ha, Rb, wb, hb, rx0, rx1, ry0, ry1)
        cd, cn, cw = box_diff(Ra, wa, ha, Rb, wb, hb, cx0, cx1, cy0, cy1)
        fd, fn, _ = box_diff(Ra, wa, ha, Rb, wb, hb, 0, 320, 0, 200)
        if 100.0 * fd / max(1, fn) > args.max_diff:
            dropped += 1
            print(f"  pair {k}: t={ta} vs t={tb}  DROPPED, whole-frame diff {100.0*fd/fn:.1f}% "
                  f"> {args.max_diff:.0f}% (different scene)")
            continue
        kept += 1
        tot["region_d"] += rd; tot["region_n"] += rn; tot["ctrl_d"] += cd; tot["ctrl_n"] += cn
        tot["full_d"] += fd; tot["full_n"] += fn
        worst_region = max(worst_region, rw); worst_ctrl = max(worst_ctrl, cw)
        print(f"  pair {k}: t={ta} (frame {A[ia][1]}) vs t={tb} (frame {B[j][1]})  "
              f"region {rd}/{rn} worst={rw}   control {cd}/{cn} worst={cw}   whole {fd}/{fn}")
        if k == 0 and args.png:
            crop_png(f"{args.png}_a.png", Ra, wa, ha, rx0 - 20, min(320, rx1 + 4), ry0, ry1, 4)
            crop_png(f"{args.png}_b.png", Rb, wb, hb, rx0 - 20, min(320, rx1 + 4), ry0, ry1, 4)
    print(f"pairs kept {kept} / dropped {dropped}")
    print(f"TOTAL region  {tot['region_d']}/{tot['region_n']} ({100*tot['region_d']/max(1,tot['region_n']):.1f}%) worst delta {worst_region}")
    print(f"TOTAL control {tot['ctrl_d']}/{tot['ctrl_n']} ({100*tot['ctrl_d']/max(1,tot['ctrl_n']):.1f}%) worst delta {worst_ctrl}")
    print(f"TOTAL whole   {tot['full_d']}/{tot['full_n']} ({100*tot['full_d']/max(1,tot['full_n']):.1f}%)")


if __name__ == "__main__":
    main()
