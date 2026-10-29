#!/usr/bin/env python3
"""frame_counter.py -- the entity frame counter (+0x11) census on the player record, shipped vs port.

docs/146 §8 found the port reading the action record's Y/X delta arrays at index 49 and above while the
record only declares 3 frames (`rec[+0x17]`, the same byte the shipped code compares the counter against to
advance the state). The question that decides whether that is a port defect or a wrong assumption about the
array length is empirical: does the SHIPPED player's counter ever pass its record's frame count?  The shipped
entity records are readable through the route dumps (docs/124), so both sides can be asked directly.

Offsets are taken from the shipped constructor's store set (docs/125): dword[+0x11] frame counter,
dword[+0x17] state, word[+0x29] hit points, and the record's own frame count is read separately from the
record block the entity resolves to, which is why this script reports the *counter* and its max, not a claim
about record lengths.

  python3 scripts/rig/frame_counter.py --orig /tmp/rig/g11/orig --port /tmp/rig/g11/port [--hp 900]
"""
import argparse
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E402


def dword(b, o):
    return int.from_bytes(b[o:o + 4], "little")


def word(b, o):
    return int.from_bytes(b[o:o + 2], "little")


def players(blocks):
    """blocks: iterable of (tag, block bytes). Yield (tag, block) for plausible player records."""
    for tag, b in blocks:
        if len(b) < 0x160 or b[0] >= 0x40:
            continue
        if word(b, 0x29) >= 900 and dword(b, 0x11) < 100000:
            yield tag, b


def census(blocks, label):
    n = 0
    ctr = collections.Counter()
    states = collections.Counter()
    worst = (0, None, None)
    for tag, b in players(blocks):
        v = dword(b, 0x11)
        st = dword(b, 0x17)
        n += 1
        ctr[v] += 1
        states[st] += 1
        if v > worst[0]:
            worst = (v, st, tag)
    print(f"{label}: player records={n}")
    if not n:
        return
    vals = sorted(ctr)
    print(f"  +0x11 min={vals[0]} max={vals[-1]} distinct={len(vals)} "
          f"median={vals[len(vals)//2]}")
    print(f"  +0x11 > 3 in {sum(c for v, c in ctr.items() if v > 3)}/{n} samples")
    print(f"  top counter values: " + ", ".join(f"{v}x{ctr[v]}" for v in vals[-6:]))
    print(f"  states seen: " + ", ".join(f"0x{k:x}x{c}" for k, c in states.most_common(8)))
    if worst[1] is not None:
        print(f"  max counter {worst[0]} on slot {worst[2]} state 0x{worst[1]:x}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    a = ap.parse_args()
    for label, path, loader in (("ORIG", a.orig, load_orig), ("PORT", a.port, load_port)):
        if not os.path.isdir(path):
            print(f"{label}: no dump at {path}")
            continue
        data = loader(path)
        samples = data[0]
        blocks = []
        for s in samples:
            # orig sample: (t, ds, heap_blocks dict of cell -> bytes); port: (t, heap, cells, dict)
            d = s[2] if len(s) >= 3 and isinstance(s[2], dict) else (s[3] if len(s) > 3 and isinstance(s[3], dict) else {})
            for cell, blk in (d or {}).items():
                name = cell if isinstance(cell, str) else str(cell)
                if name.startswith("ent") and isinstance(blk, (bytes, bytearray)):
                    blocks.append((name, bytes(blk)))
        census(blocks, label)


if __name__ == "__main__":
    main()
