#!/usr/bin/env python3
"""selfsame.py -- is the port reproducible? Compare two dumps of the SAME drive.

This is the precondition for everything downstream: a minimal repro, a red-before/green-after claim,
and coverage-guided fuzzing all need one input to produce one game. It reports the first divergent
sample and the cells that differ, with the measured entropy cells (docs/101) excluded.

  python3 scripts/rig/selfsame.py /tmp/rig/pe1 /tmp/rig/pe2 [--tol 120] [--top 12]
"""
import argparse
import bisect
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_port  # noqa: E402


def entropy_cells():
    p = os.path.join(HERE, "..", "audit", "gen", "entropy_cells.json")
    if not os.path.exists(p):
        return set()
    return {int(k, 16) for k in json.load(open(p))["entropy_cells"]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runA")
    ap.add_argument("runB")
    ap.add_argument("--tol", type=int, default=120, help="max stamp gap when pairing samples (ms)")
    ap.add_argument("--top", type=int, default=12)
    ap.add_argument("--include-entropy", action="store_true")
    a = ap.parse_args()

    ent = set() if a.include_entropy else entropy_cells()
    A, ac = load_port(a.runA)
    B, bc = load_port(a.runB)
    bt = [s[0] for s in B]
    print(f"samples A={len(A)} B={len(B)}  cells={len(ac)}  entropy-excluded={len(ent)}")
    bad = 0
    per_cell = {}
    first = []
    for t, blob, cells, _ in A:
        j = bisect.bisect_left(bt, t)
        cands = [k for k in (j - 1, j) if 0 <= k < len(B)]
        if not cands:
            continue
        k = min(cands, key=lambda k: abs(bt[k] - t))
        if abs(bt[k] - t) > a.tol:
            continue
        tb = B[k][1]
        for c in cells:
            if c["ds"] in ent:
                continue
            o, n = c["offset"], c["host_size"]
            if blob[o:o + n] != tb[o:o + n]:
                bad += 1
                per_cell[c["name"]] = per_cell.get(c["name"], 0) + 1
                if len(first) < a.top:
                    first.append((t, hex(c["ds"]), c["name"],
                                  blob[o:o + min(4, n)].hex(), tb[o:o + min(4, n)].hex()))
    print(f"mismatched cell-samples: {bad} of {len(A) * len(ac)}")
    for t, ds, name, x, y in first:
        print(f"   t={t:>7} {ds:>8} {name:<24} A={x} B={y}")
    if per_cell:
        top = sorted(per_cell.items(), key=lambda kv: -kv[1])[:a.top]
        print("worst cells:", ", ".join(f"{n}:{c}" for n, c in top))
    if not bad:
        print("VERDICT reproducible: identical masked DS cell state at every paired sample")
    else:
        print("VERDICT NOT reproducible -- the port's own runs of one drive diverge")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
