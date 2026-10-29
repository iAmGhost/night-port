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
    ap.add_argument("--cells-from-md", default="", help="parse DIFF rows out of a diff_state.py --md report")
    ap.add_argument("--cells", default="", help="comma list; default = cells flagged by the state oracle")
    ap.add_argument("--width", type=int, default=4)
    a = ap.parse_args()

    orig = dt.load_orig(a.orig)[0]
    rp = dt.load_port(a.port)
    port, pcells = rp[0], rp[-1]
    off = {c["ds"]: c["offset"] for c in pcells}
    DS_LO = getattr(dt, "DS_LO", 0x50000)

    a.cells_from_md = a.cells_from_md or None
    if a.cells:
        cells = [int(x, 0) for x in a.cells.split(",")]
    elif getattr(a, "cells_from_md", None):
        # docs/157 §3: the probe has to sample THE SAME cells the oracle flagged, otherwise "N of M cells
        # align under a shift" says nothing about whether a flagged divergence is a phase artifact.
        import re as _re
        cells = sorted({int(m, 16) for m in
                        _re.findall(r"\|\s*(?:DIFF|DIFF-WIDTH|DIFF-SIZE)\s*\|\s*(0x[0-9a-f]{4,6})\b",
                                    open(a.cells_from_md).read())})
        if not cells:
            sys.exit(f"no DIFF rows found in {a.cells_from_md}")
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
        # docs/157 §3(a): equality is only defined on grid slots where BOTH sides have a sample.  Dividing
        # by the whole grid made unrelated cells share a floor of 0.838 -- that number is the fraction of the
        # timeline where the port had produced its first sample, not a comparison result.
        cmp0 = sum(1 for x, y in zip(o, p) if x is not None and y is not None)
        eq0 = sum(1 for x, y in zip(o, p) if x is not None and y is not None and x == y)
        lag0 = eq0 / max(1, cmp0)
        cands = []
        for si in range(nshift):
            sh = (si * a.step) - a.max_lag
            d = int(sh * a.grid_hz)
            eq = tot = 0
            for k in range(nt):
                j = k + d
                if 0 <= j < nt and o[k] is not None and p[j] is not None:
                    tot += 1
                    eq += (o[k] == p[j])
            if tot > nt * 0.5:
                cands.append((eq / tot, sh))
        # docs/157 §3(b): ties (constant cells reach the same equality at every shift) used to resolve to the
        # first candidate, i.e. -max_lag, which then looked like a measured offset.  Pick the equality the
        # data actually supports at the SMALLEST |lag|, and remember whether the winner sits on the boundary.
        best_eq, best_lag = lag0, 0
        for eq, sh in sorted(cands, key=lambda c: (-c[0], abs(c[1]))):
            if eq > best_eq + 1e-9:
                best_eq, best_lag = eq, sh
            elif abs(sh) < abs(best_lag) and eq >= best_eq:
                best_eq, best_lag = eq, sh
            break
        rows.append((ad, lag0, best_lag, best_eq, abs(best_lag) >= a.max_lag - 1e-9))

    rows.sort(key=lambda r: -r[3])
    print("cell            equal@lag0   best lag   equal@best   note")
    for ad, e0, sh, e, edge in rows[:40]:
        note = "boundary lag (artifact)" if edge else ("already aligned at lag 0" if sh == 0 else "shifted")
        print("0x%05x          %6.3f      %+7.1f s     %6.3f   %s" % (ad, e0, sh, e, note))
    good = [r for r in rows if r[3] > 0.9]
    good_shifted = [r for r in good if r[2] != 0 and not r[4]]
    already = [r for r in good if r[2] == 0]
    edge = [r for r in good if r[4]]
    print("\ncells >90%% equal with NO shift: %d of %d" % (len(already), len(rows)))
    if good_shifted:
        lags = sorted(r[2] for r in good_shifted)
        print("cells >90%% equal only after a NON-boundary shift: %d  (median %+.1f s, min %+.1f, max %+.1f)"
              % (len(good_shifted), lags[len(lags) // 2], lags[0], lags[-1]))
    print("cells whose only alignment sits on the search boundary (not a measurement): %d" % len(edge))
    print("cells never >90%% equal at any shift: %d" % sum(1 for r in rows if r[3] <= 0.9))


if __name__ == "__main__":
    main()
