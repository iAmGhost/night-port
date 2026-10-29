#!/usr/bin/env python3
"""rec_stream.py -- census the record block behind a pointer cell on both sides.

docs/161 §4 established that fn_03e20f (the spawn dispatcher) is faithful: its guards and its stored value use
record[+0x09] for the horizontal test and record[+0x0d] for the vertical band, exactly as the shipped code at
guest 0x3e964-0x3e9a8 does.  §5 then showed the consequence is upstream -- the port keeps more live records
(985 vs 827 record-samples over the aligned route, 659 vs 508 of them type-1 enemies).

So the remaining question is what the dispatcher is *fed*.  A trigger record in the shipped game has this
shape (read by fn_03e20f, guest 0x3e216-0x3e25f):

    +0x01 type (dword)   +0x05 sub (dword)     +0x09 rx (dword)   +0x0d ry (dword)
    +0x11 gate (dword)   +0x15 cgate (dword)   +0x00 consumed byte (cleared on dispatch)

This script follows a pointer cell from either dump (the shipped blob is a DS window, the port blob is a cell
arena -- docs/159 §8) and prints per-field histograms plus the distinct record identities seen, so the two
sides' record streams can be compared without instrumenting the guest.

Usage:
  python3 scripts/rig/rec_stream.py --orig RUN/orig --port RUN/port --cell g_dat_53c74
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
FIELDS = {"type": (0x01, 4), "sub": (0x05, 4), "rx": (0x09, 4), "ry": (0x0d, 4),
          "gate": (0x11, 4), "cgate": (0x15, 4), "consumed": (0x00, 1)}


def s32(v):
    return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--cell", required=True, help="pointer cell name, e.g. g_dat_53c74")
    a = ap.parse_args()

    O, pcl = load_orig(a.orig)
    P, pcells = load_port(a.port)
    names = [c["name"] if isinstance(c, dict) else c for c in pcl]
    cmap = {c["name"]: c for c in pcells}
    if a.cell not in cmap or a.cell not in names:
        sys.exit(f"{a.cell} not dumped on both sides (port has it: {a.cell in cmap})")

    oi = names.index(a.cell)
    blk_o = next((len(s[2]) // len(names) for s in O if len(s) > 2 and s[2]), 0)

    def census(rows, tag):
        hists = {k: collections.Counter() for k in FIELDS}
        ids = collections.Counter()
        n = 0
        for blk in rows:
            if not blk or len(blk) < 0x20 or not any(blk[:0x20]):
                continue
            n += 1
            for k, (off, w) in FIELDS.items():
                v = int.from_bytes(blk[off:off + w], "little")
                hists[k][s32(v) if w == 4 else v] += 1
            ids[bytes(blk[:0x18]).hex()] += 1
        print(f"== {tag} {a.cell}: {n} non-empty record blocks")
        for k in ("type", "sub", "gate", "cgate", "rx", "ry", "consumed"):
            off, w = FIELDS[k]
            vals = sorted(hists[k].items(), key=lambda kv: -kv[1])[:8]
            print(f"   +0x{off:02x} {k:8s} {vals}")
        print(f"   distinct first-0x18 patterns: {len(ids)}  most common: {ids.most_common(3)}")

    orig_rows = [s[2][oi * blk_o:(oi + 1) * blk_o] for s in O if len(s) > 2 and s[2]]
    c = cmap[a.cell]
    ro = c.get("rec_offset")
    if ro is None:
        sys.exit(f"port dump has no record block for {a.cell}")
    port_rows = [s[1][ro:ro + c.get("rec_bytes", 0x160)] for s in P]

    census(orig_rows, "ORIG")
    print()
    census(port_rows, "PORT")


if __name__ == "__main__":
    main()
