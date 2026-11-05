#!/usr/bin/env python3
"""Score the pause-menu *region* separately from the whole frame (docs/383).

The original's ESC windows are the game's own menu (LEVEL UP MODE / COMMAND LIST / BGM MODE / SOUND MODE / EXIT TO DOS)
drawn over live stage.  Whole-frame NCC conflates two different questions: does the port draw the same menu, and is it
standing on the same frame of stage animation.  The menu box is found from the pixels (its fill is magenta, which the
stage does not use elsewhere), so the region is measured, not hand-boxed.
"""
import argparse, glob, os, re, numpy as np
from PIL import Image

def load(p): return np.array(Image.open(p).convert("RGB")).astype(np.float64)

sampled = []

def menu_box(imgs, names):
    """Bounding box of the purple menu fill over every frame in the window, +-1 px margin."""
    x0 = y0 = 10**9; x1 = y1 = -1
    for f, a in zip(names, imgs):
        r, g, b = a[..., 0], a[..., 1], a[..., 2]
        # The stage has a pink floor band that satisfies "R and B above G" and blew the box out to the whole frame on
        # a first run; the menu fill is a much bluer purple, so the discriminator is the blue level itself.
        m = (b > 130) & (b - g > 60) & (r - g > 20)
        if m.sum() > 150:
            cols = a[m]
            if not sampled: sampled.append((f, int(m.sum()), tuple(int(x) for x in cols.mean(axis=0))))
        ys, xs = np.nonzero(m)
        if len(xs) < 200: continue
        x0, x1 = min(x0, xs.min()), max(x1, xs.max()); y0, y1 = min(y0, ys.min()), max(y1, ys.max())
    if x1 < 0: raise SystemExit("no menu fill found in these frames")
    for f, n, mean in sampled[:2]:
        print("  fill detected in %s: %d px, mean RGB %s" % (f, n, mean))
    return max(0, x0 - 1), max(0, y0 - 1), min(imgs[0].shape[1], x1 + 2), min(imgs[0].shape[0], y1 + 2)

def ncc(a, b):
    a, b = a - a.mean(), b - b.mean()
    d = np.sqrt((a * a).sum() * (b * b).sum())
    return float((a * b).sum() / d) if d > 0 else 1.0

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True); ap.add_argument("--orig", required=True)
    ap.add_argument("--window", required=True)
    a = ap.parse_args()
    rows = [l.split("\t") for l in open(os.path.join(a.orig, "events.tsv")).read().splitlines()[1:]]
    # events.tsv columns, measured off the file: seq t_ms event label sim mode bios file.  Both earlier failures
    # were this line: the window was matched against `label` (values like `1s3`), then the file was taken from
    # `bios`, which handed PIL the name "19".
    files = [r[7] for r in rows if len(r) > 7 and a.window in r[2]]
    o = [load(os.path.join(a.orig, f)) for f in files]
    box = menu_box(o, files)
    bw, bh = box[2] - box[0], box[3] - box[1]
    P = sorted(glob.glob(os.path.join(a.port, "*.bmp")))
    P = [p for p in P if re.search(r"_%dx%d\.bmp$" % (o[0].shape[1], o[0].shape[0]), p)]   # same geometry only
    full = []; crop = []
    for f, img in zip(files, o):
        reg = img[box[1]:box[3], box[0]:box[2]]
        best = max(((ncc(load(p)[box[1]:box[3], box[0]:box[2]], reg), p) for p in P), key=lambda t: t[0])
        fullbest = max((ncc(load(p), img) for p in P))
        full.append(fullbest); crop.append(best[0])
    full, crop = np.array(full), np.array(crop)
    print("window %s: %d frames, menu box x%d..%d y%d..%d (%dx%d), %d port witnesses at this geometry"
          % (a.window, len(files), box[0], box[2] - 1, box[1], box[3] - 1, bw, bh, len(P)))
    for name, v in (("whole frame", full), ("menu region", crop)):
        print("  %-12s NCC: mean %.4f  median %.4f  min %.4f   >=0.90: %d/%d"
              % (name, v.mean(), np.median(v), v.min(), int((v >= 0.90).sum()), len(v)))

if __name__ == "__main__":
    main()
