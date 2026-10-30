#!/usr/bin/env python3
"""logic_rate.py -- the game-logic step counter DAT_00063360 per second, both sides (docs/244 5).

DAT_00063360 is the counter lab_0003284e takes modulo 4 to pick a walk step (shipped 0x00032891/0x00032896), so it
advances once per player walk step on either side: it is a monotone in-game logic counter, which makes it the
cheapest shared measure of "how fast does this build run the game logic".  This is a rate claim, so it is made
from the plain replay dumps on both sides (docs/234 §3: no --rig-dump runs for pacing claims).

  python3 scripts/rig/logic_rate.py --orig RUN/orig --port RUN/port
"""
import argparse
import os
import statistics
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E401

DS_CELL = 0x63360


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig")
    ap.add_argument("--port")
    a = ap.parse_args()

    if a.orig:
        samples, _cells = load_orig(a.orig)
        vals, ts = [], []
        for s in samples:
            t, ds = s[0], s[1]
            if len(ds) <= DS_CELL + 3:
                continue
            vals.append(int.from_bytes(ds[DS_CELL:DS_CELL + 4], "little"))
            ts.append(t)
        report("shipped", vals, ts, len(ds) if vals else 0)

    if a.port:
        samples, cells = load_port(a.port)
        cell = next((c for c in cells if c["ds"] == DS_CELL), None)
        if cell is None:
            print("== port: DAT_00063360 is not in this dump's cell list")
            return
        vals, ts = [], []
        for s in samples:
            t, blob = s[0], s[1]
            vals.append(int.from_bytes(blob[cell["offset"]:cell["offset"] + 4], "little"))
            ts.append(t)
        report("port", vals, ts, cell.get("host_size", 0))


def report(side, vals, ts, bloblen):
    if len(vals) < 2:
        print(f"== {side}: no samples (ds blob size {bloblen})")
        return
    span_s = (ts[-1] - ts[0]) / 1000.0
    deltas = [b - a for a, b in zip(vals, vals[1:]) if 0 <= b - a <= 100000]
    total = vals[-1] - vals[0]
    c = Counter(deltas)
    print(f"== {side}: DAT_00063360 first={vals[0]} last={vals[-1]} total_delta={total} "
          f"over {span_s:.1f} s -> {total / span_s:.2f} steps/s  (blob {bloblen} B, {len(vals)} samples)")
    print(f"   per-sample delta counts: {c.most_common(6)}  median={statistics.median(deltas) if deltas else 0}")


if __name__ == "__main__":
    main()
