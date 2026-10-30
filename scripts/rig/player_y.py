#!/usr/bin/env python3
"""player_y.py -- the Y time series of the player record and of kind-1 records, per side (docs/242).

docs/238 asked "where do the enemies stand?" and answered from a 33-sample window.  This asks the same
question with the whole dump and with the *order* kept: for every state sample in the window it records the
Y (+0x09) of the admissible kind-0 record with the smallest record offset (the player slot, DS 0x53c7c is
the first constructed record) and the multiset of kind-1 Y values, then prints them as run-length encoded
sequences so a "parked at one value" vs "drifting" difference is visible without plotting.

Basis rule (docs/234 §3, docs/191 §1): distribution and ordering claims only, never pacing.

  python3 scripts/rig/player_y.py --orig RUN/orig --port RUN/port [--from ms --to ms]
"""
import argparse
import os
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E401

RB = 0x160


def u(blk, off, w):
    return int.from_bytes(blk[off:off + w], "little")


def admissible(blk):
    """docs/186 §1 / docs/188 §3 gate: shape only, not an identity claim."""
    return blk[0] < 0x40 and u(blk, 0x17, 4) < 0x100 and 1 <= u(blk, 0x29, 2) <= 0x2000


def records(sample, cells, side):
    if side == "orig":
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


def rle(seq):
    out = []
    for v in seq:
        if out and out[-1][0] == v:
            out[-1][1] += 1
        else:
            out.append([v, 1])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig")
    ap.add_argument("--port")
    ap.add_argument("--from", dest="lo", type=int, default=0)
    ap.add_argument("--to", dest="hi", type=int, default=1 << 40)
    a = ap.parse_args()

    for side, path in (("orig", a.orig), ("port", a.port)):
        if not path:
            continue
        cells_path = os.path.join(path, "cells.json")
        samples, cells = (load_orig(path) if side == "orig" else load_port(path))
        n_pl = n_k1 = 0
        pl_series, k1_count = [], Counter()
        for smp in samples:
            t = smp[0] if side == "orig" else smp[0]
            if not (a.lo <= t <= a.hi):
                continue
            player_y, k1_ys = None, []
            for blk in records(smp, cells, side):
                if not admissible(blk):
                    continue
                y = u(blk, 0x09, 4)
                if not (0 <= y <= 400):
                    continue
                if blk[0] == 0 and player_y is None:
                    player_y = y
                elif blk[0] == 1:
                    k1_ys.append(y)
            if player_y is not None:
                pl_series.append(player_y)
                n_pl += 1
            k1_count.update(k1_ys)
            n_k1 += len(k1_ys)
        print(f"== {side}: samples-in-window={sum(1 for s in samples if a.lo <= s[0] <= a.hi)} "
              f"player-samples={n_pl} kind1-samples={n_k1}")
        runs = rle(pl_series)
        print(f"   player(+0x09) run-length sequence, first 40 runs: {runs[:40]}")
        c = Counter(pl_series)
        print(f"   player(+0x09) value counts: {c.most_common(8)}")
        print(f"   kind-1 (+0x09) value counts: {k1_count.most_common(10)}")
        if pl_series:
            band = sum(1 for y in pl_series if min(k1_count) - 7 <= y <= max(k1_count) + 7) if k1_count else 0
            print(f"   player samples within the kind-1 Y envelope +-7: {band}/{n_pl}")


if __name__ == "__main__":
    main()
