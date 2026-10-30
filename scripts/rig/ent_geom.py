#!/usr/bin/env python3
"""ent_geom.py -- per-side census of the record fields the hit gates compare (docs/238).

fn_013c4d's Y gate (shipped 0x00013d79-0x00013daf) compares `a[+9] + dl` / `a[+9] - dl` against `b[+9]`
with dl = byte[rec+0x56]; docs/195 §1a measured every one of 352 port rejections at dl = 7 and the
separation at 15-24 px, i.e. placement, not threshold.  This script asks the placement question directly on
one route and both dumps: what +0x09 (world Y), +0x05 (the height/offset term the shipped also sums at
0x00013d6a-76) and +0x01 (world X) do the admissible records actually carry, per kind, on the same route?

Basis rule (docs/234 §3): distribution claims only, from one route, both sides; never a pacing claim.

  python3 scripts/rig/ent_geom.py --orig RUN/orig --port RUN/port [--from ms --to ms]
"""
import argparse
import os
import statistics
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E401

RB = 0x160


def u(blk, off, w):
    return int.from_bytes(blk[off:off + w], "little")


def orig_records(sample, cells):
    _t, _ds, heap = sample
    off = 0
    for c in cells:
        n = c.get("rec_bytes") or 0
        blk = heap[off:off + n]
        off += n
        if n == RB:
            yield blk


def port_records(sample, cells):
    blob = sample[1]
    for c in cells:
        n = c.get("host_size") or 0
        if n == RB:
            yield blob[c["offset"]:c["offset"] + n]


def admissible(rec):
    if rec[0] >= 0x40 or u(rec, 0x17, 4) >= 0x1000:
        return False
    # docs/238: a record block with x == y == 0 and state 0 is an unused slot, not an entity;
    # keeping them swamps the census (2 741 of 3 022 port kind-0 samples were such slots).
    if u(rec, 0x01, 4) == 0 and u(rec, 0x09, 4) == 0 and u(rec, 0x17, 4) == 0:
        return False
    return True


def report(rows, lo, hi, label):
    per_kind = defaultdict(list)          # kind -> [ (y, y+hp_term, x, state) ]
    n = 0
    for t, recs in rows:
        if not (lo <= t <= hi):
            continue
        for rec in recs:
            if len(rec) < RB or not admissible(rec):
                continue
            n += 1
            y = u(rec, 0x09, 4)
            hterm = u(rec, 0x05, 4)
            per_kind[rec[0]].append((y, y + hterm, u(rec, 0x01, 4), u(rec, 0x17, 4)))
    print(f"== {label}: admissible record-samples={n}")
    for k in sorted(per_kind):
        v = per_kind[k]
        ys = [x[0] for x in v]
        sums = [x[1] for x in v]
        xs = [x[2] for x in v]
        distinct_y = sorted(set(ys))
        print(f"   kind {k}: samples={len(v)}  y: min={min(ys)} median={statistics.median(ys)} max={max(ys)} "
              f"distinct={len(distinct_y)}")
        print(f"      y+hp(+0x05): min={min(sums)} median={statistics.median(sums)} max={max(sums)}")
        print(f"      x: min={min(xs)} median={statistics.median(xs)} max={max(xs)}")
        # the ground line candidates, most frequent Y values
        top = Counter(ys).most_common(6)
        print(f"      most frequent y: {[(val, cnt) for val, cnt in top]}")
        st = Counter(x[3] for x in v).most_common(6)
        print(f"      most frequent state(+0x17): {[(f'{s:#x}', c) for s, c in st]}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)

    ap.add_argument("--port", required=True)
    ap.add_argument("--from", dest="lo", type=int, default=0)
    ap.add_argument("--to", dest="hi", type=int, default=10 ** 9)
    a = ap.parse_args()
    osys, ocells = load_orig(a.orig)
    report([(s[0], list(orig_records(s, ocells))) for s in osys], a.lo, a.hi, "shipped")
    psys, pcells = load_port(a.port)
    report([(s[0], list(port_records(s, pcells))) for s in psys], a.lo, a.hi, "port")


if __name__ == "__main__":
    main()
