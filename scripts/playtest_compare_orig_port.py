#!/usr/bin/env python3
"""Pair the port's named-event shots with the ORIGINAL's captured frames and report similarity per pair.

This is the compare half of the playtest harness: the port writes screenshots named per event
(`--script route --shots DIR`), and playtest_orig_drive.py writes the original's frames named per event
(`--drive drive_orig_episode1.txt --out DIR`, events.tsv carries the label/mode/time). Pairing is by content,
not by clock, because the two sides do not share a timeline: the best original frame for each port shot is
found by normalized cross-correlation on a 160x120 luma thumbnail, and when the two frames have the same
pixel size the strict per-pixel numbers are reported too (fraction within a +-3 channel tolerance, which
covers DAC/gamma slop between the port's palette and the emulator's, plus mean absolute channel error).

  usage: playtest_compare_orig_port.py --port PORT_SHOT_DIR --orig ORIG_FRAME_DIR [--tol 3] [--events a,b,c]
"""
import argparse, glob, os, re, sys
import numpy as np
from PIL import Image

EVENT_RE = re.compile(r"^\d+_(?P<name>.+)\.(bmp|png)$")


def load(path):
    return np.array(Image.open(path).convert("RGB")).astype(np.float64)


def thumb(img, w=160, h=120):
    g = 0.299 * img[:, :, 0] + 0.587 * img[:, :, 1] + 0.114 * img[:, :, 2]
    H, W = g.shape
    iy = np.clip(((np.arange(h) + 0.5) * H / h - 0.5), 0, H - 1).astype(int)
    ix = np.clip(((np.arange(w) + 0.5) * W / w - 0.5), 0, W - 1).astype(int)
    return g[np.ix_(iy, ix)]


def ncc(a, b):
    a = a - a.mean()
    b = b - b.mean()
    return float((a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--orig", required=True)
    ap.add_argument("--tol", type=float, default=3.0)
    ap.add_argument("--events", default="", help="comma-separated event names to restrict the port side to")
    ap.add_argument("--window", default="", help="restrict the original side to frames whose event contains this,"
                                                " e.g. --window slides to compare a card against slide frames only")
    a = ap.parse_args()
    keep = set(x.strip() for x in a.events.split(",") if x.strip())

    orig = []
    for f in sorted(glob.glob(os.path.join(a.orig, "*.png"))):
        m = EVENT_RE.match(os.path.basename(f))
        if not m:
            continue
        if a.window and a.window not in m.group("name"):
            continue
        img = load(f)
        orig.append((os.path.basename(f), m.group("name"), img, thumb(img)))
    if not orig:
        sys.exit("no original frames in %s" % a.orig)
    print("original frames: %d   port shots: %s" % (len(orig), a.port))

    rows = []
    for f in sorted(glob.glob(os.path.join(a.port, "*.bmp")) + glob.glob(os.path.join(a.port, "*.png"))):
        name = EVENT_RE.match(os.path.basename(f)).group("name") if EVENT_RE.match(os.path.basename(f)) else "?"
        if keep and name not in keep:
            continue
        p = load(f)
        pt = thumb(p)
        scored = sorted(((ncc(pt, o[3]), o) for o in orig), key=lambda x: -x[0])
        sim, best = scored[0]
        # Report the strictest pixel agreement among frames that are structurally the same picture: the original
        # fades cards in at boot and out between screens, which leaves the structure intact while the absolute
        # colours differ, so "best NCC" alone would report a half-faded frame as the colour match.
        strict = []
        for s2, o in scored:
            if s2 < 0.95:
                break
            if o[2].shape != p.shape:
                continue
            d = np.abs(o[2] - p)
            strict.append((float((d.max(axis=2) <= a.tol).mean()), float(d.mean()), o[0]))
        strict.sort(reverse=True)
        w, mae, wof = strict[0] if strict else (float("nan"), float("nan"), "-")
        geo = "%dx%d" % (p.shape[1], p.shape[0])
        if best[2].shape != p.shape:
            geo += " vs %dx%d MISMATCH" % (best[2].shape[1], best[2].shape[0])
        nxt = [x[0] for x in scored[1:3]]
        while len(nxt) < 2:
            nxt.append(float("nan"))
        rows.append((name, best[0], sim, wof, w, mae, geo, nxt[0], nxt[1]))

    print("%-20s %-24s %7s %-24s %10s %7s %-14s %s" % (
        "port event", "best structural frame", "NCC", "strictest pixel frame", "within +-T", "MAE", "size", "next NCC"))
    for name, of, sim, wof, w, mae, geo, n1, n2 in sorted(
            rows, key=lambda r: (-(r[4] if r[4] == r[4] else -1), -r[2])):
        f2 = lambda v: "n/a" if v != v else "%.3f" % v
        print("%-20s %-24s %7.4f %-24s %10s %7s %-14s %s/%s" % (
            name, of, sim, wof, "n/a" if w != w else "%.4f" % w, "n/a" if mae != mae else "%.2f" % mae, geo, f2(n1), f2(n2)))

    # how much of the original's capture has a port counterpart at all
    port_thumbs = []
    for f in sorted(glob.glob(os.path.join(a.port, "*.bmp")) + glob.glob(os.path.join(a.port, "*.png"))):
        p = load(f)
        port_thumbs.append(thumb(p))
    covered = sum(1 for o in orig if max(ncc(t, o[3]) for t in port_thumbs) >= 0.90)
    print("\noriginal frames with a port counterpart at NCC>=0.90: %d/%d" % (covered, len(orig)))
    ev = {}
    for _, name, _img, _t in orig:
        base = re.sub(r"_\d+$", "", name)
        ev[base] = ev.get(base, 0) + 1
    print("original event windows: " + ", ".join("%s(%d)" % (k, v) for k, v in sorted(ev.items())))


if __name__ == "__main__":
    main()
