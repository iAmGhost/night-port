#!/usr/bin/env python3
"""hit_rate.py -- count landed hits per side from the dumps alone (docs/245 4).

A landed hit on an enemy decrements its pending-HP term `word[record + 0x23]` (shipped fn_01441e 0x1495e, reached
because an enemy's reserve pool `dword[record + 0x56]` is zero) or, when that term reaches zero, drives the
settle flag `byte[record + 0x30]` to 2 and then 1.  So counting debt decreases per record slot, per side, is a
hit counter that needs no instrumentation on the shipped side -- the record blocks are already dumped.

Record slots are matched by dump position, not by value (docs/186 1: iterate the cell list, and never treat a
value match as an identity claim).

  python3 scripts/rig/hit_rate.py --orig RUN/orig --port RUN/port
"""
import argparse
import os
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E401

RB = 0x160


def u(blk, off, w):
    return int.from_bytes(blk[off:off + w], "little")


def s16(blk, off):
    v = u(blk, off, 2)
    return v - 0x10000 if v >= 0x8000 else v


def plausible(blk):
    """Kind-1 enemy records carry +0x29 == 0, so docs/186's HP-bound gate must not be used here (docs/245 2)."""
    st = u(blk, 0x17, 4) & 0x7FFFFFFF          # the shipped masks the same field at 0x14749 (docs/245 5)
    return blk[0] == 1 and st < 0x100 and u(blk, 0x0d, 4) < 0x1000


def records(sample, cells, side):
    if side == "shipped":
        _t, _ds, heap = sample
        off = 0
        for c in cells:
            n = c.get("rec_bytes") or 0
            blk = heap[off:off + n]
            off += n
            if n == RB:
                yield blk
    else:
        blob = sample[1]
        for c in cells:
            n = c.get("host_size") or 0
            if n == RB:
                yield blob[c["offset"]:c["offset"] + n]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig")
    ap.add_argument("--port")
    a = ap.parse_args()
    for side, path in (("shipped", a.orig), ("port", a.port)):
        if not path:
            continue
        samples, cells = (load_orig(path) if side == "shipped" else load_port(path))
        prev = {}
        hits = defaultdict(int)
        big = [0, 0]
        settle2 = settle1 = 0
        first_t = last_t = None
        nrec = 0
        for smp in samples:
            t = smp[0]
            if first_t is None:
                first_t = t
            last_t = t
            for i, blk in enumerate(records(smp, cells, side)):
                if not plausible(blk):
                    prev.pop(i, None)
                    continue
                nrec += 1
                debt = s16(blk, 0x23)
                st = blk[0x30]
                p = prev.get(i)
                if p is not None:
                    if debt < p:                       # one landed hit (docs/245: dmg lands on +0x23)
                        hits[u(blk, 0x0d, 4)] += 1
                        big[0 if p - debt <= 16 else 1] += 1
                    if p != 2 and st == 2:
                        settle2 += 1
                    if st == 1:
                        settle1 += 1
                prev[i] = debt
        span = (last_t - first_t) / 1000.0
        tot = sum(hits.values())
        print(f"== {side}: samples={len(samples)} span={span:.1f}s enemy-record-samples={nrec} "
              f"debt-decrease events={tot} -> {tot / span:.2f} hits/s")
        print(f"   per type_idx: {sorted(hits.items(), key=lambda kv: -kv[1])[:8]}")
        print(f"   decrease size split: <=16 px {big[0]}  >16 (slot-reuse suspect) {big[1]}")
        print(f"   settle transitions: +0x30->2 seen {settle2} times, +0x30 == 1 seen {settle1} times")


if __name__ == "__main__":
    main()
