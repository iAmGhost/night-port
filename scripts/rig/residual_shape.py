#!/usr/bin/env python3
"""residual_shape.py -- give every residual state-oracle row a SERIES SHAPE.

docs/172 and docs/177 each closed a residual row by noticing what the numbers looked like over a whole route: one
side always zero, a monotone stamp with a private epoch, a counter that stops when the drive runs out.  Doing that
row by row is slow, and "value divergence" says nothing about which of those shapes a row has.  This pass reads the
full series on both sides of a run and labels each residual with its shape, so the remaining rows can be routed to
the right kind of follow-up instead of counted.

Deliberately UNALIGNED: it compares shapes and value sets, not per-window equality, so it cannot produce a verdict
and must not be quoted as one.  Alignment stays the job of diff_state.py's transition anchor (docs/158).

Usage: residual_shape.py /tmp/rig/g27 [--report /tmp/rig/g27/report_state.txt]
"""
import argparse, glob, json, os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cells_reader import CellStream

DS_LO = 0x50000            # docs/124: the dump's DS physical base; 0x51000 reads one page off


def residuals(path):
    txt = open(path).read()
    m = re.search(r"residual same-width gameplay divergences: (\d+)", txt)
    n = int(m.group(1)) if m else 0
    out = []
    for line in txt.splitlines():
        r = re.match(r"\s+0x([0-9a-f]{4,5}) (\S+)$", line)
        if r:
            out.append((int(r.group(1), 16), r.group(2)))
        if len(out) >= n:
            break
    return out[:n]


def series_shipped(ds, width):
    vals = []
    for f in sorted(glob.glob(os.path.join(ORIG, "state_*.ds.bin"))):
        b = open(f, "rb").read()
        off = ds - DS_LO
        if off + width > len(b):
            continue
        vals.append(int.from_bytes(b[off:off + width], "little", signed=False))
    return vals


def shape(vs, vp):
    def mono(v):
        back = sum(1 for i in range(1, len(v)) if v[i] < v[i - 1])
        return back <= 1
    sz, sp = set(vs), set(vp)
    both = sz & sp
    ov_s = len(both) / max(1, len(sz))
    ov_p = len(both) / max(1, len(sp))
    if not any(vs) and any(vp):
        return "PORT-HOLDS-VALUE / SHIPPED-ALL-ZERO"
    if not any(vp) and any(vs):
        return "PORT-ALL-ZERO / SHIPPED-HOLDS-VALUE"
    if mono(vs) and mono(vp) and max(vs + vp) > 1000:
        return "MONOTONE-STAMP (private epochs; compare increments)"
    if ov_s >= 0.8 and ov_p >= 0.8:
        return "SAME VALUE SET, different order/dwell"
    if ov_s >= 0.4 or ov_p >= 0.4:
        return "PARTIAL overlap"
    return "DISJOINT value sets"


def main():
    global ORIG
    ap = argparse.ArgumentParser()
    ap.add_argument("run")
    ap.add_argument("--report")
    a = ap.parse_args()
    ORIG = os.path.join(a.run, "orig")
    rep = a.report or os.path.join(a.run, "report_state.txt")
    st = CellStream(os.path.join(a.run, "port"))
    rows = residuals(rep)
    print("residual rows read from %s: %d" % (rep, len(rows)))
    for ds, name in rows:
        c = st.by_name.get(name)
        if not c:
            print("  %-10s %-22s (not in this dump's cell table)" % (hex(ds), name))
            continue
        w = c.get("guest_size") or 4
        w = w if w in (1, 2, 4, 8) else 4
        vs = series_shipped(ds, w)
        vp = [st.val(i, ds, w) for i in range(st.samples)]
        lab = shape(vs, vp)
        print("  %-8s %-22s w=%-2s n=%4d/%4d  |S|=%3d |P|=%3d  %-38s shipped[%s..%s] port[%s..%s]"
              % (hex(ds), name, w, len(vs), len(vp), len(set(vs)), len(set(vp)), lab,
                 min(vs), max(vs), min(vp), max(vp)))


if __name__ == "__main__":
    main()
