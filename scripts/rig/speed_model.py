#!/usr/bin/env python3
"""Per-tick speed model: separate ordinary movement from discontinuities (the "enemy teleports" item).

Raw position jumps are useless as a signature (docs/108): the shipped side shows MORE >12 px jumps than the
port (370 vs 83) because walking at 4 Hz already moves an entity a dozen pixels between samples. What actually
distinguishes a teleport is distance *per unit of elapsed time*. So: model the shipped side's own speed
distribution, take a per-elapsed-time ceiling from it, and report every transition on either side that exceeds
the other side's ceiling. Both sides are sampled through the same entity-list walk (+0x13f), and a sample pair
is only compared when the record's type byte is unchanged, so slot reuse (a new entity taking over a slot) is
not counted as a teleport.
"""
import argparse, bisect, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

RB = 0x160
X, Y, TYPE, STATE, NEXT = 0x01, 0x09, 0x00, 0x17, 0x13f

def rd(b, o, w=4):
    return int.from_bytes(b[o:o+w], 'little', signed=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--orig', required=True); ap.add_argument('--port', required=True)
    ap.add_argument('--tol', type=int, default=300)
    ap.add_argument('--from', dest='t_from', type=int, default=0)
    ap.add_argument('--to', dest='t_to', type=int, default=10**9)
    ap.add_argument('--pct', type=float, default=99.5, help='percentile of the reference side used as the ceiling')
    ap.add_argument('--ref', choices=['orig', 'port'], default='orig')
    a = ap.parse_args()
    O, oc = load_orig(a.orig); P, pc = load_port(a.port)
    ent_o = [(i, c) for i, c in enumerate(oc) if str(c['name']).startswith('ent')]
    ent_p = [(i, c) for i, c in enumerate(pc) if str(c['name']).startswith('ent')]
    # The dump records each cell's real in-blob offset; summing rec_bytes is wrong because non-record
    # cells also carry a rec_bytes value (rec_offset -1) and are not laid out in the record area.
    ob = {n: c.get('offset', sum(x.get('rec_bytes', 0) for x in oc[:i])) for n, (i, c) in enumerate(ent_o)}
    pb = {n: c.get('offset', sum(x.get('rec_bytes', 0) for x in pc[:i])) for n, (i, c) in enumerate(ent_p)}
    pt = [s[0] for s in P]

    def series(side):
        out = {}
        if side == 'orig':
            for t, ds, hb in O:
                if not (a.t_from <= t <= a.t_to): continue
                for n, (i, c) in enumerate(ent_o):
                    blk = hb[ob[n]:ob[n] + c.get('rec_bytes', RB)]
                    if len(blk) < RB or not any(blk): continue
                    out.setdefault(n, []).append((t, rd(blk, X), rd(blk, Y), blk[TYPE], rd(blk, STATE)))
        else:
            for row in P:
                t, blob = row[0], row[1]
                if not (a.t_from <= t <= a.t_to): continue
                for n, (i, c) in enumerate(ent_p):
                    blk = blob[pb[n]:pb[n] + c.get('rec_bytes', RB)]
                    if len(blk) < RB or not any(blk): continue
                    out.setdefault(n, []).append((t, rd(blk, X), rd(blk, Y), blk[TYPE], rd(blk, STATE)))
        return out

    def trans(se):
        tr = []
        for n, rows in se.items():
            for (t0, x0, y0, ty0, s0), (t1, x1, y1, ty1, s1) in zip(rows, rows[1:]):
                if ty0 != ty1: continue          # slot reused by a different entity: not movement
                dt = t1 - t0
                if dt <= 0: continue
                tr.append((n, t1, dt, abs(x1-x0), abs(y1-y0), ((x1-x0)**2 + (y1-y0)**2) ** 0.5, (x0, y0), (x1, y1), s0, s1))
        return tr

    so, sp = series('orig'), series('port')

    def link(se, radius=48):
        """Identity-aware linking: track each entity to the nearest entity of the SAME type in the previous
        sample instead of assuming list-slot stability. The slot-based version flagged mirrored pairs at
        identical timestamps (A: 238->98 while B: 98->238), which is the entity list re-linking, not movement;
        that is why 'enemy teleporting' looked like a port defect while the shipped side shows the same
        signature. A transition is only a jump when no predecessor of the same type is within `radius`."""
        frames = {}
        for n, rows in se.items():
            for r in rows: frames.setdefault(r[0], []).append((n,) + r[1:])
        out = []
        ts = sorted(frames)
        prev = {}
        for t in ts:
            cur = frames[t]
            for (n, x, y, ty, st) in cur:
                best = None
                for (pn, px, py, pty, pst) in prev.values():
                    if pty != ty: continue
                    d = ((x-px)**2 + (y-py)**2) ** 0.5
                    if best is None or d < best[0]: best = (d, pn, px, py, pst)
                if best is not None and best[0] <= radius:
                    out.append((n, t, t - prev[best[1]][0], abs(x-best[2]), abs(y-best[3]), best[0],
                                (best[2], best[3]), (x, y), best[4], st))
                else:
                    out.append((n, t, None, None, None, None, None, (x, y), None, st))
            prev = {n: (n, x, y, ty, st) for (n, x, y, ty, st) in cur}
        return out

    def trans(se):
        tr = []
        for n, rows in se.items():
            for (t0, x0, y0, ty0, s0), (t1, x1, y1, ty1, s1) in zip(rows, rows[1:]):
                if ty0 != ty1: continue          # slot reused by a different entity: not movement
                dt = t1 - t0
                if dt <= 0: continue
                tr.append((n, t1, dt, abs(x1-x0), abs(y1-y0), ((x1-x0)**2 + (y1-y0)**2) ** 0.5, (x0, y0), (x1, y1), s0, s1))
        return tr

    unlinked_o = [r for r in link(so) if r[2] is None]
    unlinked_p = [r for r in link(sp) if r[2] is None]
    to, tp = trans(so), trans(sp)
    print(f'identity-linked samples with NO predecessor within 48 px (spawn/teleport candidates): '
          f'orig={len(unlinked_o)} port={len(unlinked_p)}')
    def pct(vals, p):
        if not vals: return 0.0
        v = sorted(vals); k = min(len(v)-1, int(round(p/100.0*(len(v)-1))))
        return v[k]
    ref, other, refname = (to, tp, a.ref) if a.ref == 'orig' else (tp, to, a.ref)
    sp_ms = [d/dt for _, _, dt, _, _, d, _, _, _, _ in ref]
    ceil_ms = pct(sp_ms, a.pct)
    print(f'{refname}: transitions={len(ref)}  speed p50={pct(sp_ms,50)*1000:.0f} p90={pct(sp_ms,90)*1000:.0f} '
          f'p{a.pct}={ceil_ms*1000:.0f} max={max(sp_ms)*1000:.0f} px/s')
    # per-dt ceiling: max distance the reference side ever covers in that elapsed time
    bydt = {}
    for _, _, dt, _, _, d, _, _, _, _ in ref: bydt.setdefault(dt, []).append(d)
    lim = {k: pct(v, 100.0) for k, v in bydt.items()}
    dts = sorted(lim)
    def limit(dt):
        if not dts: return 0.0
        k = min(dts, key=lambda k: abs(k-dt))
        return lim[k] if abs(k-dt) <= 60 else max(lim.values())
    name = {'orig': 'port', 'port': 'orig'}[a.ref]
    # Primary criterion is speed (distance per elapsed time): the per-dt distance table is sparse at the
    # other side's sample intervals, so it is kept only as a secondary report.
    bad = [r for r in other if (r[5] / r[2]) > ceil_ms * 1.5 and r[5] > 12]
    print(f'{name}: transitions={len(other)}  exceedances beyond {refname} p{a.pct} speed x1.5 (>12 px): {len(bad)}')
    for r in sorted(bad, key=lambda r: -r[5])[:12]:
        print(f'   slot {r[0]:2d} t={r[1]:6d} dt={r[2]:4d}ms dist={r[5]:6.1f} (limit {limit(r[2]):5.1f}) '
              f'({r[6][0]},{r[6][1]})->({r[7][0]},{r[7][1]}) state {r[8]:#x}->{r[9]:#x}')

if __name__ == '__main__':
    main()
