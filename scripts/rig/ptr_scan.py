#!/usr/bin/env python3
"""Blind scan of the shipped side's DS for cells that hold live heap pointers.

The state oracle can only read records through cells the port already declared as pointers
(`gen/rig_cells.json`). That is circular: if the port mis-assigns a global, the rig cannot see the
shipped game's real entity list. This scan is the un-biased version -- it asks, for every 4-byte
aligned DS cell, how often its content looks like an address inside the guest's allocated range, and
whether that address changes over time. A cell that is pointer-like in most samples AND takes several
distinct values is a live heap object being created/destroyed: exactly the shape of an entity record.

  ptr_scan.py --orig RUN/orig [--lo 0x100000] [--hi 0x600000] [--top 30]

The value range defaults are derived from the pointer values observed in route runs (0x19d0e0,
0x2086e0, 0x2c2edc, 0x3818f8), i.e. the guest's flat/physical heap window.
"""
import argparse
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import DS_LO  # noqa: E402

DS_HI = 0x65000  # the dumped DS window, same as replay_original.py


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--lo", default="0x100000")
    ap.add_argument("--hi", default="0x600000")
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--every", type=int, default=1, help="sample stride")
    a = ap.parse_args()

    lo, hi = int(a.lo, 0), int(a.hi, 0)
    files = sorted(f for f in os.listdir(a.orig) if f.endswith(".ds.bin"))[:: a.every]
    n = len(files)
    hits = defaultdict(int)          # cell -> samples where pointer-like
    distinct = defaultdict(set)      # cell -> distinct pointer values
    for f in files:
        ds = open(os.path.join(a.orig, f), "rb").read()
        for i in range(0, len(ds) - 3, 4):
            v = int.from_bytes(ds[i : i + 4], "little")
            if lo <= v <= hi:
                cell = DS_LO + i
                hits[cell] += 1
                if len(distinct[cell]) < 64:
                    distinct[cell].add(v)

    live = [(c, h, len(d)) for c, h, d in ((c, hits[c], distinct[c]) for c in hits)
            if h >= n // 4]
    live.sort(key=lambda x: (-x[2], -x[1]))
    print(f"samples={n} window=[{hex(lo)},{hex(hi)}] pointer-like cells={len(live)}")
    print(f"{'cell':>10s} {'in':>5s} {'frac':>6s} {'distinct':>8s}")
    for c, h, d in live[: a.top]:
        print(f"{hex(c):>10s} {h:5d} {h/n:6.2f} {d:8d}")
    print("\nchurn ranking (distinct pointer values, then persistence):")
    for c, h, d in live[: a.top]:
        print(f"  {hex(c)} distinct={d} frac={h/n:.2f}")


if __name__ == "__main__":
    main()
