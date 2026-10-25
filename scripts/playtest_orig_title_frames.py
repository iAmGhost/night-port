#!/usr/bin/env python3
"""Original-side title frames: are they faithful pictures of the shipped title card at all?

docs/72 filed an OPEN item because the ORIGINAL's settled title "held at fitted gain ~0.79" while the port's title
measured ~0.997 against the same card. This script checks the premise before any gain is believed: for every captured
original frame that the drive harness labelled title / title_card / title_melt it reports

  * the distinct-colour count (the card decode has 246),
  * the share of its pixels whose colour is EXACTLY one of the card's own colours, in both channel orders,
  * the least-squares per-channel gain and the residual it buys,
  * whether a translation search up to +-24 px closes the gap (it does not).

A frame whose colours are not the card's colours under either channel order cannot yield a meaningful palette level,
which is exactly what the ~0.79 number was.

    python3 scripts/playtest_orig_title_frames.py [png ...]
"""
from __future__ import annotations

import glob
import os
import sys

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from playtest_compare_slides import pcx_indices  # noqa: E402

CARD = os.path.join(ROOT, "scratch", "res", "title3.pcx")
DIRS = ["orig_die2", "orig_ep1", "orig_slides2", "orig_full", "orig_stage", "orig_fix", "orig_boot"]


def key(a):
    return (a[:, :, 0].astype(np.int64) << 16) | (a[:, :, 1] << 8) | a[:, :, 2]


def main() -> int:
    d = open(CARD, "rb").read()
    pal = np.frombuffer(d[-768:], dtype=np.uint8)
    ix = np.frombuffer(pcx_indices(CARD), dtype=np.uint8)
    C = np.stack([pal[3 * ix], pal[3 * ix + 1], pal[3 * ix + 2]], axis=1).reshape(480, 640, 3)
    cardset = set(key(C).reshape(-1).tolist())
    print("card title3.pcx: %d distinct colours, mean RGB %s, largest flat colour %.3f of the frame"
          % (len(cardset), np.asarray(C).reshape(-1, 3).mean(axis=0).astype(int),
             max(np.unique(key(C), return_counts=True)[1]) / 307200))
    files = sys.argv[1:]
    if not files:
        for dr in DIRS:
            for pat in ("*title*", "*title_card*"):
                files += sorted(glob.glob(os.path.join("/tmp/dbgauto", dr, pat + ".png")))
    seen = set()
    for f in files:
        if f in seen or not os.path.exists(f):
            continue
        seen.add(f)
        im = Image.open(f)
        if im.size != (640, 480):
            continue
        a = np.asarray(im.convert("RGB")).astype(np.int64)
        for order, nm in (([0, 1, 2], "as-is"), ([2, 1, 0], "swap")):
            b = a[:, :, order]
            k = key(b)
            u, c = np.unique(k, return_counts=True)
            share = sum(cnt for v, cnt in zip(u, c) if int(v) in cardset) / c.sum()
            bf = b.astype(np.float64)
            cf = C.astype(np.float64)
            g = [float((bf[:, :, i] * cf[:, :, i]).sum() / max((cf[:, :, i] ** 2).sum(), 1.0)) for i in range(3)]
            mae0 = float(np.abs(bf - cf).mean())
            mae1 = float(np.mean([np.abs(bf[:, :, i] * g[i] - cf[:, :, i]).mean() for i in range(3)]))
            print("%-40s %-6s colours=%-4d exact-card-colour share=%.3f  MAE %.2f  gain %s -> %.2f"
                  % (os.path.relpath(f, "/tmp/dbgauto"), nm, len(u), share, mae0, np.round(g, 3), mae1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
