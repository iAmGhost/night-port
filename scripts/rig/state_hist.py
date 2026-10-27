#!/usr/bin/env python3
"""Census of the entity state field (+0x17) across every dumped record, both sides.

The playtest items "hurt/staggering motion is missing", "fallen state is almost missing" and "player
stuck when hit mid-kick" are all statements about which *states* a victim entity passes through, and
the disassembly names the ranges: f.c:24125 maps hit reactions onto "the 0x0b..0x1e hurt states" and
onto "0x66..0x5b", and f.c:26474 tests `10 < PL32(0x17) < 0x15` for the player's hurt branch. So the
signature does not need frame matching -- it is a count: how many samples of the fight is any dumped
entity sitting in a hurt state, and how many in a fallen state.

Records come from the pointer cells both sides already dump (entity list head/tail, player entity).
Widths are compared as raw bytes; the port's width-adapted link fields at +0x13f/+0x143 are beyond the
field offsets used here.

  state_hist.py --orig RUN/orig --port RUN/port [--cell g_dat_53c70 --cell 0x53c74]
                [--field 0x17] [--from 150000] [--to 230000]
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

CLASSES = [("neutral", 0x00, 0x0a), ("hurt/stagger", 0x0b, 0x1e), ("other", 0x1f, 0x5a),
           ("fallen", 0x5b, 0x66), ("other-high", 0x67, 1 << 32)]


def classify(v):
    for name, lo, hi in CLASSES:
        if lo <= v <= hi:
            return name
    return "?"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--cell", action="append", default=[], help="name or 0x.. (default: all ptr cells)")
    ap.add_argument("--field", default="0x17")
    ap.add_argument("--width", type=int, default=4)
    ap.add_argument("--from", dest="t0", type=int, default=0)
    ap.add_argument("--to", dest="t1", type=int, default=1 << 40)
    a = ap.parse_args()

    field, w = int(a.field, 0), a.width
    orig, ocells = load_orig(a.orig)
    port, pcells = load_port(a.port)

    def pick(cells):
        if not a.cell:
            return [(i, c) for i, c in enumerate(cells) if c.get("ptr") or "rec" in c or "rec_bytes" in c]
        out = []
        for want in a.cell:
            addr = int(want, 16) if want.lower().startswith("0x") else None
            for i, c in enumerate(cells):
                if (addr is not None and c["ds"] == addr) or (addr is None and c["name"] == want):
                    out.append((i, c))
        return out

    oc, pc = pick(ocells), pick(pcells)
    if not oc or not pc:
        sys.exit(f"cells not found: orig={len(oc)} port={len(pc)}; available port names: "
                 + ",".join(c["name"] for c in pcells if c.get("ptr"))[:300])

    def scan(side, chosen, is_orig):
        hist, ids, per_cell = {}, {}, {}
        for s in side:
            t = s[0]
            if not (a.t0 <= t <= a.t1):
                continue
            for i, c in chosen:
                rb = c.get("rec_bytes") or c.get("record_bytes") or 0
                if is_orig:
                    base = sum((x.get("rec_bytes", 0) for x in ocells[:i]), 0)
                    blk = s[2][base:base + rb]
                else:
                    blk = s[1][c["rec_offset"]:c["rec_offset"] + rb]
                if len(blk) < field + w or not any(blk):
                    continue
                v = int.from_bytes(blk[field:field + w], "little")
                hist[classify(v)] = hist.get(classify(v), 0) + 1
                ids[v] = ids.get(v, 0) + 1
                per_cell.setdefault(c["name"], Counter())[classify(v)] += 1
        return hist, ids

    from collections import Counter  # noqa: F401  (used through per_cell)
    oh, oids = scan(orig, oc, True)
    ph, pids = scan(port, pc, False)

    print(f"state field +{hex(field)} (w={w}) over t=[{a.t0},{a.t1}]  "
          f"cells orig={[c['name'] for _, c in oc]} port={[c['name'] for _, c in pc]}")
    print(f"{'class':14s} {'orig':>8s} {'port':>8s}")
    for name, _, _ in CLASSES:
        print(f"{name:14s} {oh.get(name,0):8d} {ph.get(name,0):8d}")
    print("\ndistinct state ids (count):")
    for label, ids in (("orig", oids), ("port", pids)):
        top = sorted(ids.items(), key=lambda kv: -kv[1])[:14]
        print(f"  {label:5s} " + " ".join(f"{hex(k)}x{v}" for k, v in top))
    for name, lo, hi in CLASSES:
        if name in ("hurt/stagger", "fallen"):
            o, p = oh.get(name, 0), ph.get(name, 0)
            if (o > 0) != (p > 0) or (o and p and abs(o - p) > 0.5 * (o + p)):
                print(f"\nVERDICT {name}: orig={o} port={p} "
                      f"-> {'side never enters the range' if min(o, p) == 0 else 'entry counts differ >2x'}")


if __name__ == "__main__":
    main()
