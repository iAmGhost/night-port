#!/usr/bin/env python3
"""Print a DS cell's value per drive phase on both sides (the schedule view).

diff_state.md only shows windows where a cell disagrees, which is the wrong shape for questions like
"which CD track was selected when": those need the whole timeline side by side. The shipped game keeps
its current CD track extent in DAT_00053c44 (start LBA) / DAT_00053c40 (end LBA) / DAT_00053c48 (length
in seconds), so those three cells *are* its music schedule, read straight out of memory.

  ds_trace.py --orig RUN/orig --port RUN/port --drive DRIVE --addrs 0x53c44,0x53c40,0x53c48
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import DS_LO, load_orig, load_port, phases_from_drive, val  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--drive", required=True)
    ap.add_argument("--addrs", required=True, help="comma-separated DS addresses")
    a = ap.parse_args()

    addrs = [int(x, 0) for x in a.addrs.split(",")]
    phases = phases_from_drive(a.drive)
    orig, _ = load_orig(a.orig)
    port, pcells = load_port(a.port)
    off = {c["ds"]: c["offset"] for c in pcells}

    def bucket(samples):
        srt = sorted(samples, key=lambda s: s[0])
        b = {}
        for label, t0, t1 in phases:
            inside = [s for s in srt if t0 <= s[0] < t1]
            if not inside:
                prior = [s for s in srt if s[0] < t0]
                inside = [prior[-1]] if prior else []
            b[label] = inside
        return b

    ob, pb = bucket(orig), bucket(port)
    print(f"{'phase':12s} " + " ".join(f"{hex(x):>18s}" for x in addrs))
    prev = None
    for label, t0, t1 in phases:
        row = []
        for ad in addrs:
            o = [val(s[1], ad - DS_LO, 4) for s in ob[label]]
            p = [val(s[1], off[ad], 4) for s in pb[label]] if ad in off else []
            o = [x for x in o if x is not None]
            p = [x for x in p if x is not None]
            last = max(set(o), key=o.count) if o else None
            # the port samples continuously, so a phase can contain a transition: report both ends
            pend = max(set(p), key=p.count) if p else None
            prange = (min(p), max(p)) if p and min(p) != max(p) else None
            row.append((last, pend, prange))
        cur = tuple(row)
        if cur == prev:
            continue
        prev = cur
        cells = []
        for (o, p, pr) in row:
            s = f"{o if o is None else hex(o)}"
            if pr:
                s += f"|{hex(pr[0])}->{hex(pr[1])}"
            s += f"/{p if p is None else hex(p)}"
            cells.append(s)
        print(f"{label:12s} " + " ".join(f"{c:>18s}" for c in cells))
    print("(cell format: orig_mode | port_transition / port_mode)")


if __name__ == "__main__":
    main()
