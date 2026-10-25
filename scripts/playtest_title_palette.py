#!/usr/bin/env python3
"""Why does the ORIGINAL's settled title sit at fitted gain ~0.79 while the port settles at ~0.997 (docs/72's OPEN note)?

Compares, per channel, the least-squares scalar gain that maps one picture onto another, between three
representations of the settled title:

  O = the original's own captured frame (dosbox-automation raw payload, docs/70 channel order);
  P = the port's presented frame (host BMP shot, RGBA8888 read per playtest_compare_slides.bmp_rgb);
  C = the shipped card through its own trailer palette, in both the exact and the 6-bit-DAC-quantized form.

If O and P agree per pixel there is no palette difference to explain and the two quoted gains must have been
fitted against different references.

    python3 scripts/playtest_title_palette.py <orig_raw_dir> <port_shot_dir> [card.pcx]
"""
from __future__ import annotations

import glob
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from playtest_compare_slides import bmp_rgb, pcx_indices, pcx_rgb, pcx_size  # noqa: E402


def decode_raw(path):
    d = open(path, "rb").read()
    w, h, pitch = struct.unpack("<iii", d[0:12])
    payload = d[15:]
    rowpx = abs(pitch) // 4
    if rowpx * h * 4 != len(payload):
        return None
    px = np.frombuffer(payload, dtype=np.uint8).reshape(h, rowpx, 4)[:, :, :3]
    return w, h, px


def fit(a: np.ndarray, b: np.ndarray):
    """per-channel least-squares gain mapping a -> b, and the MAE before/after applying it."""
    a = a.astype(np.float64)
    b = b.astype(np.float64)
    g = [float((a[:, :, c] * b[:, :, c]).sum() / max((a[:, :, c] ** 2).sum(), 1.0)) for c in range(3)]
    before = float(np.abs(a - b).mean())
    after = float(np.mean([np.abs(a[:, :, c] * g[c] - b[:, :, c]).mean() for c in range(3)]))
    return g, before, after


def main() -> int:
    odir, pdir = sys.argv[1], sys.argv[2]
    card = sys.argv[3] if len(sys.argv) > 3 else os.path.join("scratch", "res", "title3.pcx")
    cw, ch = pcx_size(card)
    d = open(card, "rb").read()
    pal = np.frombuffer(d[-768:], dtype=np.uint8)
    idx = np.frombuffer(pcx_indices(card), dtype=np.uint8)
    ce = np.stack([pal[3 * idx], pal[3 * idx + 1], pal[3 * idx + 2]], axis=1).astype(np.uint8)
    # the game hands the table to the DAC shifted right by 2 (fn_036190 0x361f1-0x36203), so what a faithful
    # screen can show is the 6-bit value scaled back up: v & 0xFC
    cq = (ce.astype(np.uint16) & 0xFC).astype(np.uint8)
    C = ce.reshape(ch, cw, 3)
    Q = cq.reshape(ch, cw, 3)
    # pick the port frame that is actually the settled card (arena indices byte-identical), not merely the last shot
    pick, bestf = None, -1.0
    for raw in sorted(glob.glob(os.path.join(pdir, "*.raw"))):
        a = np.frombuffer(open(raw, "rb").read()[:cw * ch], dtype=np.uint8)
        if a.size != cw * ch:
            continue
        f = float((a == np.frombuffer(pcx_indices(card), dtype=np.uint8)).mean())
        if f > bestf:
            bestf, pick = f, raw[:-4] + ".bmp"
    print("port frame chosen (arena indices == card decode): %s  exact-index %.4f"
          % (os.path.basename(pick), bestf))
    shots = [pick] if os.path.exists(pick) else sorted(glob.glob(os.path.join(pdir, "*.bmp")))
    if not shots:
        print("no port shots in", pdir)
        return 1
    pw, ph, pl = bmp_rgb(shots[-1])
    P = np.frombuffer(b"".join(pl), dtype=np.uint8).reshape(ph, pw, 3)
    print("port shot %s  %dx%d" % (os.path.basename(shots[-1]), pw, ph))
    print("card %s  %dx%d" % (os.path.basename(card), cw, ch))

    if P.shape == C.shape:
        for nm, ref in (("card exact", C), ("card 6-bit quantized", Q)):
            g, b0, b1 = fit(P, ref)
            print("  P -> %-22s gain %s  MAE %.2f -> %.2f" % (nm, np.round(g, 3), b0, b1))

    best = None
    for f in sorted(glob.glob(os.path.join(odir, "*.raw"))):
        d = decode_raw(f)
        if not d or d[1] != 480:
            continue
        _, _, px = d
        if px.shape[1] != pw:                       # crop the emulator's overscan-free row if wider
            px = px[:, :pw]
        if px.shape[1] != pw:
            continue
        sc = float(np.abs(px.astype(np.int16) - P.astype(np.int16)).mean())
        if best is None or sc < best[1]:
            best = (os.path.basename(f), sc, px)
    if best is None:
        print("no usable original 480-row frames in", odir)
        return 1
    nm, sc, O = best
    print("original frame nearest the port's: %s  MAE %.2f" % (nm, sc))
    print("  O vs P: within +-3 %.4f%%  MAE %.2f" %
          ((np.abs(O.astype(np.int16) - P.astype(np.int16)) <= 3).all(axis=2).mean() * 100.0, sc))
    for refname, ref in (("card exact", C), ("card 6-bit quantized", Q)):
        g, b0, b1 = fit(O, ref)
        print("  O -> %-22s gain %s  MAE %.2f -> %.2f" % (refname, np.round(g, 3), b0, b1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
