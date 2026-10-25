#!/usr/bin/env python3
"""docs/85: the level-up / status presentation timeline, original side vs port side.

Every captured frame is scored against the independent decode of the shipped levelup2.pcx card. Each
line carries the docs/77 premise checks next to the number, so a gain can never be quoted without them:
the per-channel level ratio (a real DAC fade keeps all three channels equal), the exact-card-colour share
for BOTH channel orders (the channel-order anchor), and the distinct-colour count of the frame vs the card.

  usage: playtest_levelup_timeline.py --orig DIR [--port DIR] [--card FILE] [--every N] [--grep PAT]
"""
import argparse, fnmatch, glob, os, sys
import numpy as np, PIL.Image

def load(path):
    im = PIL.Image.open(path).convert("RGB")
    return np.asarray(im, dtype=np.float64), im.size

def colour_stats(x, cardset):
    """exact-card-colour share and distinct-colour count, over unique rows (cheap: screens are <=256 colours)."""
    flat = x.astype(np.int64).reshape(-1, 3)
    uniq, cnt = np.unique(flat, axis=0, return_counts=True)
    hit = sum(c for u, c in zip(map(tuple, uniq), cnt) if u in cardset)
    return hit / flat.shape[0] * 100.0, len(uniq)

def score(f, card, cardset):
    """(orientation, within3@gain1, MAE@gain1, ratio triple, within3@fitted gain, fitted gain, exact share)."""
    best = None
    for name, x in (("as-captured", f), ("R/B-swapped", f[:, :, ::-1])):
        d = np.abs(x - card)
        w1 = (d.max(axis=2) <= 3).mean() * 100
        m = x.mean(axis=(0, 1)) / card.mean(axis=(0, 1))
        g = float(np.median(m))
        wg = (np.abs(x - g * card).max(axis=2) <= 3).mean() * 100
        share, ndist = colour_stats(x, cardset)
        t = (name, w1, float(d.mean()), m, wg, g, share, ndist)
        if best is None or w1 > best[1]:
            best = t
    return best

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port")
    ap.add_argument("--card", default=os.path.join(os.path.dirname(__file__), "..", "scratch", "res", "levelup2.pcx"))
    ap.add_argument("--every", type=int, default=1)
    ap.add_argument("--grep", default=None, help="only frames whose event name matches this glob")
    a = ap.parse_args()
    card, csize = load(a.card)
    cardset = {tuple(u) for u in np.unique(card.astype(np.int64).reshape(-1, 3), axis=0)}
    print(f"card {os.path.basename(a.card)} {csize[0]}x{csize[1]} distinct={len(cardset)}")
    for tag, d in (("ORIG", a.orig), ("PORT", a.port)):
        if not d:
            continue
        print(f"\n== {tag} {d} ==")
        best_row = None
        n = 0
        for p in sorted(glob.glob(os.path.join(d, "*.png")) + glob.glob(os.path.join(d, "*.bmp"))):
            b = os.path.basename(p)
            if a.grep and not fnmatch.fnmatch(b, "*" + a.grep + "*"):
                continue
            n += 1
            if (n - 1) % a.every:
                pass
            f, sz = load(p)
            if sz != csize:
                print(f"  {b:30s} {sz[0]}x{sz[1]}  (not {csize[0]}x{csize[1]}: scored separately)")
                continue
            s = score(f, card, cardset)
            print(f"  {b:30s} {s[0]:11s} within+-3@1.00={s[1]:6.2f}% MAE={s[2]:6.2f} "
                  f"ch-ratio={tuple(round(v,3) for v in s[3])} within3@{s[5]:.2f}={s[4]:6.2f}% "
                  f"exact-card-share={s[6]:6.2f}% distinct={s[7]}")
            if best_row is None or s[1] > best_row[1][1]:
                best_row = (b, s)
        print(f"  frames={n}")
        if best_row:
            s = best_row[1]
            print(f"  BEST: {best_row[0]} within+-3={s[1]:.2f}% MAE={s[2]:.2f} gain={s[5]:.2f} "
                  f"ch-ratio={tuple(round(v,3) for v in s[3])} exact-card-share={s[6]:.2f}%")
    return 0

if __name__ == "__main__":
    sys.exit(main())
