#!/usr/bin/env python3
"""Pair port playtest screenshots with the original's captures and score them.

Port shots (port/src/app/main.c harness) are `NNNN_<event>.bmp` + manifest.tsv; the original's are
`NNNN_<event>.png` produced by scripts/playtest_orig.py. Pairing is by the NNNN_<event> stem, so the
two runs line up by game event, not by wall clock.

Metrics per pair (after palette-accurate RGB conversion and nearest-neighbour resize to the port size):
  mad    mean absolute difference over RGB (0 = identical)
  corr   Pearson correlation of luma (1 = same structure)
  match  the fraction of pixels within 12 levels on every channel

A pair whose stem has no counterpart on the other side is reported as MISSING, and for every port shot
the best-scoring original frame overall is also reported (NEAREST) so a timing offset between the two
runs does not hide an otherwise correct frame.

Usage: uv run --with pillow python scripts/playtest_compare.py /tmp/pt1 scratch/orig [--report out.md]
"""
import argparse, os, sys
from PIL import Image


def load_rgb(path, size=None):
    im = Image.open(path).convert("RGB")
    if size and im.size != size:
        im = im.resize(size, Image.NEAREST)
    return im


def metrics(a, b):
    pa, pb = list(a.getdata()), list(b.getdata())
    n = len(pa)
    if n == 0 or len(pb) != n:
        return None
    sa = sum((p[0] + p[1] + p[2]) / 3.0 for p in pa)
    sb = sum((p[0] + p[1] + p[2]) / 3.0 for p in pb)
    ma, mb = sa / n, sb / n
    vab = vaa = vbb = 0.0
    mad = 0.0
    within = 0
    for x, y in zip(pa, pb):
        la, lb = (x[0] + x[1] + x[2]) / 3.0, (y[0] + y[1] + y[2]) / 3.0
        vab += (la - ma) * (lb - mb)
        vaa += (la - ma) ** 2
        vbb += (lb - mb) ** 2
        mad += abs(x[0] - y[0]) + abs(x[1] - y[1]) + abs(x[2] - y[2])
        if max(abs(x[0] - y[0]), abs(x[1] - y[1]), abs(x[2] - y[2])) <= 12:
            within += 1
    denom = (vaa * vbb) ** 0.5
    corr = (vab / denom) if denom > 1e-9 else 1.0
    return mad / (3.0 * n), corr, within / n


def stem(f):
    return os.path.splitext(f)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port_dir")
    ap.add_argument("orig_dir")
    ap.add_argument("--report", default=None)
    ap.add_argument("--pass-mad", type=float, default=18.0)
    a = ap.parse_args()

    def collect(d, exts):
        return {stem(f): os.path.join(d, f) for f in sorted(os.listdir(d))
                if os.path.splitext(f)[1] in exts}

    port, orig = collect(a.port_dir, {".bmp"}), collect(a.orig_dir, {".png"})
    if not port:
        print("no port shots (.bmp) in", a.port_dir, file=sys.stderr)
        return 2
    oimg = {}
    for k, p in orig.items():
        try:
            oimg[k] = load_rgb(p)
        except Exception as e:                            # noqa: BLE001
            print("skip", p, e, file=sys.stderr)
    rows, fails = [], 0
    for k, pp in sorted(port.items()):
        pim = None
        try:
            pim = load_rgb(pp)
        except Exception as e:                            # noqa: BLE001
            print("skip", pp, e, file=sys.stderr)
            continue
        best = None
        for ok, o in oimg.items():
            m = metrics(pim, load_rgb(o, pim.size))
            if m and (best is None or m[1] > best[1][1]):
                best = (ok, m)
        if k in oimg:
            m = metrics(pim, load_rgb(orig[k], pim.size))
            pair = f"mad={m[0]:6.2f} corr={m[1]:+.3f} match={m[2]*100:5.1f}%"
            okflag = m[0] <= a.pass_mad or m[2] >= 0.85
        else:
            pair, okflag = "MISSING", False
        if not okflag:
            fails += 1
        rows.append((k, pair, (f"nearest {best[0]} mad={best[1][0]:.2f} corr={best[1][1]:+.3f}"
                               if best else "no originals"), okflag))
    lines = ["| shot | paired original | nearest original | ok |", "|---|---|---|---|"]
    for k, pair, near, okflag in rows:
        lines.append(f"| {k} | {pair} | {near} | {'yes' if okflag else 'NO'} |")
    text = "\n".join(lines) + f"\n\n{len(rows)} port shots, {len(oimg)} original captures, " \
        f"{fails} not matching (threshold mad<={a.pass_mad} or match>=85%)\n"
    print(text)
    if a.report:
        open(a.report, "w").write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
