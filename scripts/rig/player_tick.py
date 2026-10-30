#!/usr/bin/env python3
"""player_tick.py -- how fast the player's own logic advances, both sides (docs/244 5).

The player record's animation frame counter (+0x11) is incremented by the shipped's per-frame machinery and reset
when an animation cycles, so counting its forward steps (and its resets) over a whole dump gives a logic-tick
rate that needs no emulator-side memory reads and no --rig-dump inflation: it is the same record block the state
oracle already compares.

  python3 scripts/rig/player_tick.py --orig RUN/orig --port RUN/port
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
    return blk[0] < 0x40 and u(blk, 0x17, 4) < 0x100 and 0 <= u(blk, 0x29, 2) <= 0x2000


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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig")
    ap.add_argument("--port")
    a = ap.parse_args()
    for side, path in (("shipped", a.orig), ("port", a.port)):
        if not path:
            continue
        samples, cells = (load_orig(path) if side == "orig" else load_port(path))
        prev = None
        ups = downs = same = wraps = 0
        states = Counter()
        first_t = last_t = None
        n = 0
        for smp in samples:
            t = smp[0]
            if first_t is None:
                first_t = t
            last_t = t
            pl = None
            for blk in records(smp, cells, side):
                if blk[0] == 0 and admissible(blk):
                    pl = blk
                    break
            if pl is None:
                continue
            n += 1
            fr = u(pl, 0x11, 4)
            states["0x%02x" % u(pl, 0x17, 4)] += 1
            if prev is not None:
                if fr == prev + 1:
                    ups += 1
                elif fr == 0:
                    wraps += 1
                elif fr < prev:
                    downs += 1
                else:
                    same += 1
            prev = fr
        span = (last_t - first_t) / 1000.0
        print(f"== {side}: player-samples={n} span={span:.1f}s forward_steps={ups} wraps={wraps} "
              f"other_back={downs} same={same} -> {(ups + wraps) / span:.2f} ticks/s")
        print(f"   player state(+0x17) counts: {states.most_common(6)}")


if __name__ == "__main__":
    main()
