#!/usr/bin/env python3
"""Raw-pixel agreement per original event window (docs/388).

The rig's picture comparator pairs 160x120 luminance thumbnails, which answers "does this scene appear on both sides"
and nothing about resolution, crop, colour accuracy or which moment of the scene is being shown.  This tool pairs on the
same basis -- so the pairs are comparable -- but then reports what the pixels actually do: mean absolute error, the
fraction exactly equal, and the fraction within 3 LSB.  A claim like "the stage picture matches" belongs here; a count
of thumbnails belongs in playtest_compare_orig_port.py, and the two are quoted side by side on purpose.
"""
import argparse, glob, os, numpy as np
from PIL import Image

def load(p): return np.array(Image.open(p).convert("RGB")).astype(np.float64)

def thumb(img, w=160, h=120):                      # the comparator's own basis, copied so pairs agree
    g = 0.299 * img[:, :, 0] + 0.587 * img[:, :, 1] + 0.114 * img[:, :, 2]
    H, W = g.shape
    iy = np.clip(((np.arange(h) + 0.5) * H / h - 0.5), 0, H - 1).astype(int)
    ix = np.clip(((np.arange(w) + 0.5) * W / w - 0.5), 0, W - 1).astype(int)
    return g[np.ix_(iy, ix)]

def ncc(a, b):
    a, b = a - a.mean(), b - b.mean()
    d = np.sqrt((a * a).sum() * (b * b).sum())
    return float((a * b).sum() / d) if d > 1e-9 else float("nan")     # a flat side is a blind spot, not a match

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True); ap.add_argument("--orig", required=True)
    ap.add_argument("--windows", default="gameplay,slides,esc_win_,esc_win2_,title_melt,fli,boot")
    a = ap.parse_args()
    witnesses = []
    for p in sorted(glob.glob(os.path.join(a.port, "*.bmp")) + glob.glob(os.path.join(a.port, "*.png"))):
        img = load(p); witnesses.append((p, img, thumb(img)))
    rows = [l.split("\t") for l in open(os.path.join(a.orig, "events.tsv")).read().splitlines()[1:]]
    print("%-11s %5s %7s %8s %8s %8s %8s   %s" %
          ("window", "frames", "thumbNCC", "rawMAE", "exact%", "<=3LSB%", "geometry", "strongest raw pair (exact%)"))
    for name in a.windows.split(","):
        files = [r[7] for r in rows if len(r) > 7 and name in r[2]]
        if not files: print("%-11s (no frames)" % name); continue
        nccs, maes, exs, ex3s, geom, best_note, best_ex = [], [], [], [], 0, "", -1
        for f in files:
            o = load(os.path.join(a.orig, f)); t = thumb(o)
            p, w, wt = max(witnesses, key=lambda x: (ncc(t, x[2]) if ncc(t, x[2]) == ncc(t, x[2]) else -1.0))
            n = ncc(t, wt)
            if n == n: nccs.append(n)
            if w.shape != o.shape:
                geom += 1; maes.append(float("nan")); exs.append(float("nan")); ex3s.append(float("nan")); continue
            d = np.abs(o - w)
            mae = float(d.mean()); ex = float((d.max(axis=2) == 0).mean()); ex3 = float((d.max(axis=2) <= 3).mean())
            maes.append(mae); exs.append(ex); ex3s.append(ex3)
            if ex > best_ex: best_ex, best_note = ex, "%s <-> %s" % (f, os.path.basename(p))
        fin = lambda xs: [x for x in xs if x == x]
        N, M, E, E3 = fin(nccs), fin(maes), fin(exs), fin(ex3s)
        print("%-11s %5d %7s %8s %8s %8s %8s   %s" %
              (name, len(files),
               ("%.4f" % np.mean(N)) if N else "blind",
               ("%.2f" % np.mean(M)) if M else "n/a",
               ("%.1f" % (100*np.mean(E))) if E else "n/a",
               ("%.1f" % (100*np.mean(E3))) if E3 else "n/a",
               ("%d differ" % geom) if geom else "same",
               best_note if best_ex > 0 else "no pair shares a moment"))
    print("\nRead together: a high thumbNCC with a low raw MAE and high <=3LSB fraction is a picture match; a high")
    print("thumbNCC with raw MAE in the tens is the same scene at a different moment (docs/388 1).")

if __name__ == "__main__":
    main()
