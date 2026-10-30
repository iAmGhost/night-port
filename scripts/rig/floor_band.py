#!/usr/bin/env python3
"""floor_band.py -- per-row ink profile of a saved witness frame, for the "floor renders black" claim.

Witness bins (port/src/app/rig_sink.c rig_screen_frame) are the exact pixels the host presents,
w*h*4 bytes, channel order as captured (ri/gi/bi chosen by the caller).  This script does not compare
two dumps pixel-by-pixel (docs/202, docs/205 forbid that across dumps); it asks one question per frame:
for each row of the presented canvas, how many pixels differ from that frame's modal colour, and how
many are pure black.  A rendered floor band shows a broad non-modal row profile; an unrendered one is
modal/black everywhere.

  floor_band.py <dump-or-dir-with-frames> [--pick N] [--frames a,b,c]
"""
import argparse, collections, os, struct, sys

def rows_of(path, w, h):
    px = open(path, 'rb').read()
    if len(px) != w * h * 4:
        raise SystemExit(f"{path}: {len(px)} bytes, expected {w*h*4}")
    prof = []
    for y in range(h):
        off = y * w * 4
        row = px[off:off + w * 4]
        cnt = collections.Counter()
        black = 0
        for i in range(0, len(row), 4):
            k = (row[i], row[i+1], row[i+2])
            cnt[k] += 1
            if k == (0, 0, 0): black += 1
        modal, n = cnt.most_common(1)[0]
        prof.append((y, w - n, black, modal, len(cnt)))
    return prof

def band(prof, y0, y1):
    ink = sum(p[1] for p in prof[y0:y1])
    blk = sum(p[2] for p in prof[y0:y1])
    rows_with_ink = sum(1 for p in prof[y0:y1] if p[1])
    return ink, blk, rows_with_ink

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--frames", default="", help="comma list of frame file names (default: --pick evenly spaced)")
    ap.add_argument("--pick", type=int, default=4)
    ap.add_argument("--mode", default="", help="only frames of this WxH, e.g. 320x200")
    args = ap.parse_args()
    d = args.dir if os.path.isdir(args.dir) else os.path.dirname(args.dir)
    fs = sorted(os.listdir(os.path.join(d, "frames")))
    if args.mode: fs = [f for f in fs if f.endswith(args.mode + ".bin")]
    if args.frames:
        sel = [f for f in args.frames.split(",") if f]
    else:
        step = max(1, len(fs) // args.pick)
        sel = fs[::step][:args.pick]
    for f in sel:
        wh = f.split('_')[1].split('.')[0]
        w, h = (int(x) for x in wh.split('x'))
        prof = rows_of(os.path.join(d, "frames", f), w, h)
        top, bot = band(prof, 0, h // 2), band(prof, h // 2, h)
        print(f"--- {f}  modal-row ink: top-half {top[0]} px / bottom-half {bot[0]} px "
              f"(black {top[1]}/{bot[1]}, rows-with-ink {top[2]}/{bot[2]})")
        step = max(1, h // 20)
        for i in range(0, h, step):
            y, ink, blk, modal, ncol = prof[i]
            bar = '#' * min(40, ink // 8)
            print(f"    y={y:3d} ink={ink:4d} black={blk:4d} colours={ncol:3d} modal={modal} {bar}")

if __name__ == "__main__":
    main()
