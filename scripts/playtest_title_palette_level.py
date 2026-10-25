#!/usr/bin/env python3
"""docs/86: the ORIGINAL's settled title palette level, with the docs/77 premise checks attached.

The point of this script is that the premise is proven before any level is read:

  1. hold stability   -- distinct-colour count and byte agreement across the sampled hold;
  2. picture identity -- a per-index functional map: for every title3.pcx screen index, how many distinct
                         RGB values does the captured frame show? 1.00 (and dom-class 100%) means the
                         captured picture IS the shipped card decode, so a palette level is meaningful;
  3. channel order    -- the exact-card-colour share for BOTH orders at +-1 LSB (the card palette is read
                         straight from the PCX trailer, so +-0 exact matches are depressed by the game's own
                         6-bit DAC quantization -- see the per-index table);
  4. only then the per-index table: card RGB, port RGB, original RGB, and the orig/card ratio per channel.

  usage: playtest_title_palette_level.py --orig DIR_OR_PNG [--port DIR] [--card scratch/res/title3.pcx]
"""
import argparse, glob, os, sys, collections
import numpy as np, PIL.Image

def load_rgb(p):
    return np.asarray(PIL.Image.open(p).convert("RGB"), dtype=np.int64)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port")
    ap.add_argument("--card", default="scratch/res/title3.pcx")
    ap.add_argument("--tol", type=int, default=1, help="LSB tolerance for the colour-share premise check")
    ap.add_argument("--rows", type=int, default=20)
    ap.add_argument("--pick", default="", help="substring; the frame/series to analyse (e.g. title_long)")
    a = ap.parse_args()

    C = PIL.Image.open(a.card)
    cidx = np.asarray(C, dtype=np.int64).reshape(-1)
    cpal = np.array(C.getpalette()[:768], dtype=np.int64).reshape(-1, 3)
    crgb = cpal[cidx]
    H, W = np.asarray(C).shape

    def one(orig_path, port_path):
        O = load_rgb(orig_path)
        if O.shape[:2] != (H, W):
            print(f"  {orig_path}: {O.shape[1]}x{O.shape[0]} != card {W}x{H} -- skipped"); return
        Of = O.reshape(-1, 3)
        Os = Of[:, ::-1]                                     # R/B-exchanged order
        cardset = {tuple(c) for c in cpal}
        def share(x):
            cs = np.array(sorted(cardset))
            ux, inv = np.unique(x, axis=0, return_inverse=True)
            near = np.zeros(len(ux), dtype=bool)
            for j, u in enumerate(ux):
                near[j] = bool(np.any((np.abs(cs - u).max(axis=1)) <= a.tol))
            return near[inv].mean() * 100.0
        def dist(x): return len(np.unique(x, axis=0))
        def within(x, ref): return (np.abs(x - ref).max(axis=1) <= 3).mean() * 100.0

        # per-index map (the picture-identity proof)
        per = collections.defaultdict(collections.Counter)
        for i, o in zip(cidx.tolist(), map(tuple, Of.tolist())): per[i][o] += 1
        cpi = np.mean([len(v) for v in per.values()])
        dom = sum(max(c.values()) for c in per.values()) / Of.shape[0] * 100.0

        print(f"frame {os.path.basename(orig_path)}")
        print(f"  distinct colours          : {dist(Of)} (card palette {len(cardset)})")
        print(f"  picture identity          : indices seen {len(per)}/{len(np.unique(cidx))}, "
              f"colours/index {cpi:.2f}, dominant-class {dom:.2f}%")
        print(f"  exact-card share (+-{a.tol})   : as-captured {share(Of):6.2f}%   R/B-swapped {share(Os):6.2f}%")
        print(f"  within +-3 of card RGB    : as-captured {within(Of, crgb):6.2f}%   R/B-swapped {within(Os, crgb):6.2f}%")
        if port_path:
            P = load_rgb(port_path).reshape(-1, 3)
            if P.shape == Of.shape:
                print(f"  port-vs-original          : as-captured {within(Of, P):6.2f}%   R/B-swapped "
                      f"{within(Os, P):6.2f}%   (MAE {np.abs(Of-P).mean():.2f} / {np.abs(Os-P).mean():.2f})")
        # per-index table + ratio
        rows = []
        for i in sorted(per):
            o = per[i].most_common(1)[0][0]
            n = sum(per[i].values())
            rows.append((i, n, tuple(cpal[i]), o))
        print(f"  {'idx':>4} {'pixels':>7}  {'card RGB':>15}  {'orig RGB (as captured)':>22}  {'orig swapped':>15}  {'orig/card R,G,B':>21}")
        cr, og = [], []
        for i, n, c, o in rows:
            os_ = (o[2], o[1], o[0])
            cr.append(c); og.append(os_)
            if len(rows) <= a.rows or n > 5000:
                ratio = tuple(round(os_[k] / c[k], 3) if c[k] else 0.0 for k in range(3))
                print(f"  {i:4d} {n:7d}  {str(c):>15}  {str(o):>22}  {str(os_):>15}  {str(ratio):>21}")
        cr = np.array(cr); og = np.array(og)
        d = og - cr
        with np.errstate(divide="ignore", invalid="ignore"):
            rat = np.where(cr > 0, og / cr, np.nan)
            med = tuple(round(float(np.nanmedian(rat[:, k])), 4) for k in range(3))
        print(f"  palette level, per-index orig_swapped/card: median ratio {med}; "
              f"delta (orig_swapped - card) mean {tuple(round(float(d[:, k].mean()), 2) for k in range(3))}, "
              f"p5/p95 {int(np.percentile(d, 5))}/{int(np.percentile(d, 95))}")
        gain = float(np.median(np.sum(og * cr, axis=0) / np.sum(cr * cr, axis=0)))
        print(f"  single least-squares gain over the settled palette: {gain:.4f} "
              f"(premise satisfied: dominant-class {dom:.2f}%, share {max(share(Of), share(Os)):.2f}%)")

    origs = sorted(glob.glob(os.path.join(a.orig, "*.png"))) if os.path.isdir(a.orig) else [a.orig]
    if a.pick and os.path.isdir(a.orig):
        sel = [p for p in origs if a.pick in os.path.basename(p)]
        if sel: origs = sel
    ports = sorted(glob.glob(os.path.join(a.port, "*.bmp")) + glob.glob(os.path.join(a.port, "*.png"))) if a.port else []
    opick = origs[-1] if os.path.isdir(a.orig) else a.orig
    ppick = None
    if ports:
        card_rgb = cpal[cidx].reshape(H, W, 3)
        best = None
        for p in ports:
            im = PIL.Image.open(p).convert("RGB")
            if im.size != (W, H): continue
            d = np.abs(load_rgb(p) - card_rgb).max(axis=2)
            w = (d <= 3).mean()
            if best is None or w > best[0]: best = (w, p)
        ppick = best[1]; print(f"best port frame vs card: {os.path.basename(ppick)} within+-3={best[0]*100:.2f}%")
    one(opick, ppick)
    print(f"\nhold series ({len(origs)} samples)")
    card_rgb = cpal[cidx].reshape(H, W, 3)
    prev = None; prevkey = None; repeats = 0; total = 0
    for p in origs:
        O = load_rgb(p)
        if O.shape[:2] != (H, W): continue
        total += 1
        Of = O.reshape(-1, 3)
        wsw = (np.abs(Of[:, ::-1] - crgb).max(axis=1) <= 3).mean() * 100
        w3 = (np.abs(Of - crgb).max(axis=1) <= 3).mean() * 100
        chg = "" if prev is None else f" changed-vs-prev>3={((np.abs(Of-prev).max(axis=1))>3).mean()*100:5.2f}%"
        key = (round(wsw, 1), round(w3, 1), len(np.unique(Of, axis=0)))
        if prevkey is not None and key == prevkey:
            repeats += 1
        else:
            if repeats:
                print(f"      ... +{repeats} identical consecutive samples")
                repeats = 0
            print(f"  {os.path.basename(p):28s} colours={key[2]:3d} within3(swapped)={wsw:6.2f}% "
                  f"within3(as)={w3:6.2f}%{chg}")
        prev, prevkey = Of, key
    if repeats: print(f"      ... +{repeats} identical consecutive samples")
    print(f"  total {total} samples")
    return 0

if __name__ == "__main__":
    sys.exit(main())
