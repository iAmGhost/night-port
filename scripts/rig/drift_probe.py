#!/usr/bin/env python3
"""Is a DIFF verdict a value divergence or a timeline shift? Measure the lag that best aligns each cell.

docs/131 §5 recorded a route where the activity-profile oracle says 269/269 SAME (the same cells change in the
same phases) while the value oracle flags 110 DIFF. Those two observations are compatible with one dominant
cause: the two sessions are the same timeline displaced by a few seconds, e.g. the container session's boot and
the port process's boot are not the same length, so "phase P" windows compare different moments.

For every differing cell this resamples both value traces onto a common grid and finds the shift (in --step
increments, up to +/- --max-lag) that maximizes the fraction of equal samples. A cluster of cells agreeing on
one nonzero shift means the run is mis-phased and the DIFF verdicts are an alignment artifact (rig defect);
a spread of best-shifts around zero means the values genuinely diverge.

  python3 scripts/rig/drift_probe.py --orig RUN/orig --port RUN/port [--max-lag 40] [--step 0.5]
                                    [--grid-hz 2] [--cells 0x5333c,0x53c44]
"""
import argparse
import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import ds_trace as dt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--max-lag", type=float, default=40.0)
    ap.add_argument("--step", type=float, default=0.5)
    ap.add_argument("--grid-hz", type=float, default=2.0)
    ap.add_argument("--cells", default="", help="comma list; default = cells flagged by the state oracle")
    ap.add_argument("--width", type=int, default=4)
    a = ap.parse_args()

    orig = dt.load_orig(a.orig)[0]
    rp = dt.load_port(a.port)
    port, pcells = rp[0], rp[-1]
    off = {c["ds"]: c["offset"] for c in pcells}
    DS_LO = getattr(dt, "DS_LO", 0x50000)

    if a.cells:
        cells = [int(x, 0) for x in a.cells.split(",")]
    else:
        cells = sorted(off.keys())

    t_end = min(max(s[0] for s in orig), max(s[0] for s in port)) / 1000.0
    nt = int(t_end * a.grid_hz) + 1

    def trace(samples, addr, portside):
        """last-value-at-time on a common grid"""
        srt = sorted(samples, key=lambda s: s[0])
        out = []
        j = 0
        cur = None
        for k in range(nt):
            t = k / a.grid_hz
            while j < len(srt) and srt[j][0] / 1000.0 <= t:
                blob = srt[j][1]
                o = off.get(addr) if portside else addr - DS_LO
                if o is not None and o + a.width <= len(blob):
                    cur = int.from_bytes(blob[o:o + a.width], "little")
                j += 1
            out.append(cur)
        return out

    nshift = int(2 * a.max_lag / a.step) + 1
    rows = []
    for ad in cells:
        o, p = trace(orig, ad, False), trace(port, ad, True)
        if all(v is None for v in o) or all(v is None for v in p):
            continue
        base = sum(1 for x, y in zip(o, p) if x is not None and y is not None and x == y)
        best = (-1, 0.0)
        for si in range(nshift):
            sh = (si * a.step) - a.max_lag
            d = int(sh * a.grid_hz)
            eq = tot = 0
            for k in range(nt):
                j = k + d
                if 0 <= j < nt and o[k] is not None and p[j] is not None:
                    tot += 1
                    eq += (o[k] == p[j])
            if tot > nt * 0.5 and eq / tot > best[1]:
                best = (sh, eq / tot)
        if best[0] != 0 or True:
            rows.append((ad, base / max(1, nt), best[0], best[1]))

    rows.sort(key=lambda r: -r[3])
    print("cell            equal@lag0   best lag   equal@best")
    for ad, e0, sh, e in rows[:40]:
        print("0x%05x          %6.3f      %+7.1f s     %6.3f" % (ad, e0, sh, e))
    good = [r for r in rows if r[3] > 0.9]
    if good:
        lags = sorted(r[1] for r in good)
        print("\ncells that become >90%% equal under some shift: %d of %d" % (len(good), len(rows)))
        print("their shifts: median %+.1f s, min %+.1f s, max %+.1f s" %
              (lags[len(lags) // 2], lags[0], lags[-1]))
    print("cells never >90%% equal: %d" % sum(1 for r in rows if r[3] <= 0.9))


if __name__ == "__main__":
    main()
