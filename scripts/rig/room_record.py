#!/usr/bin/env python3
"""room_record.py -- byte census of the record behind DS 0x63354, on both sides.

DS 0x63354 is a pointer, not a struct: the shipped room play loop loads it as the camera policy's
argument (guest 0x0003ab18 `mov eax, [0x13354]` then `call 0x0003d4e4`), and the port mirrors it
(`uint8_t *room = g_dat_63354`, f.c:23128, passed at f.c:23193).  Both rigs dump the pointed-to block
(rec_bytes 352 on both sides), so every field of the camera policy's gate -- +0x2a/+0x2e stop bounds,
+0x32/+0x36 second pair, +0x3a/+0x3b disable bytes, +0x00/+0x08 extents, +0x40 track index -- is
readable from the dumps without guest instrumentation (docs/160).

This script answers the question cam_policy.py raised numerically: for each record offset, what value
SET does each side hold, and where are the two sets disjoint?  A disjoint set is a writer the port has
that the shipped code does not (or vice versa), which is the docs/94 heap-offset family.
"""
import argparse
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_port  # noqa: E402

DS_LO = 0x50000
KEY = "g_dat_63354"


def u32(b, o):
    return int.from_bytes(b[o:o + 4], "little") if o + 4 <= len(b) else None


def s32(v):
    return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--span", type=int, default=0x80)
    a = ap.parse_args()

    # ---- shipped: heap block per stored state, indexed by the ptr_cells list (docs/124) ----
    tl = json.load(open(os.path.join(a.orig, "timeline.json")))
    pc = [c["name"] if isinstance(c, dict) else c for c in tl.get("ptr_cells") or []]
    if KEY not in pc:
        sys.exit(f"{KEY} not among the shipped dump's ptr_cells: {pc[:6]}")
    k = pc.index(KEY)
    n = len(pc)
    o_rows = []
    for i in range(len(tl["timeline"])):
        hp = os.path.join(a.orig, f"state_{i:04d}.heap.bin")
        if not os.path.exists(hp):
            continue
        hb = open(hp, "rb").read()
        blk = len(hb) // n
        o_rows.append(hb[k * blk:k * blk + blk])

    # ---- port ----
    S, cells = load_port(a.port)
    cmap = {c["name"]: c for c in cells}
    c = cmap.get(KEY)
    if not c or c.get("rec_offset") is None:
        sys.exit(f"{KEY} has no record dump in {a.port}")
    p_rows = [s[1][c["rec_offset"]:c["rec_offset"] + c["rec_bytes"]] for s in S]

    def hist(rows, off):
        return collections.Counter(u32(r, off) for r in rows if len(r) >= off + 4)

    print(f"shipped samples {len(o_rows)}   port samples {len(p_rows)}   record offsets 0x00..{a.span - 0x10:02x}")
    disjoint = []
    for off in range(0, a.span, 4):
        ho, hp_ = hist(o_rows, off), hist(p_rows, off)
        so, sp = set(ho) - {None}, set(hp_) - {None}
        if not so or not sp:
            continue
        if so == sp:
            continue
        tag = "DISJOINT" if not (so & sp) else "PARTIAL"
        line = (f"  +0x{off:02x} {tag:8s} shipped {sorted(s32(v) for v in ho)[:5]} ({sum(ho.values())} samples)"
                f"  | port {sorted(s32(v) for v in hp_)[:5]} ({sum(hp_.values())} samples)")
        if tag == "DISJOINT" or (len(so) <= 2 and len(sp) > 2):
            disjoint.append(off)
        print(line)
    print(f"\noffsets with a disjoint or one-sided value set: {[f'+0x{o:02x}' for o in disjoint]}")


if __name__ == "__main__":
    main()
