#!/usr/bin/env python3
"""Byte-offset histogram of entity-record differences over a window.

The state oracle dumps a fixed-size block at every pointer cell (the guest keeps player/enemy records on
the heap), so a record that disagrees can be localised to the *field* that disagrees instead of "the
block differs". This walks the heap blobs per phase and reports, for each pointer cell, which record
offsets differ and how often, plus the two sides' values there.

  record_diff.py --orig RUN/orig --port RUN/port --drive DRIVE --from 60000 --to 160000
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import DS_LO, load_orig, load_port, phases_from_drive  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--drive", required=True)
    ap.add_argument("--from", dest="t0", type=int, default=0)
    ap.add_argument("--to", dest="t1", type=int, default=1 << 40)
    ap.add_argument("--top", type=int, default=12)
    a = ap.parse_args()

    phases = [p for p in phases_from_drive(a.drive) if p[2] > a.t0 and p[1] < a.t1]
    orig, ptr_cells = load_orig(a.orig)
    port, pcells = load_port(a.port)
    RB = 64

    def bucket(samples, idx):
        """idx picks the byte blob: the shipped side stores (t, ds, heap) while the port keeps the record
        blocks inline in its single DS blob, at each cell's rec_offset."""
        srt = sorted(samples, key=lambda s: s[0])
        b = {}
        for label, t0, t1 in phases:
            inside = [s[idx] for s in srt if t0 <= s[0] < t1]
            if not inside:
                prior = [s[idx] for s in srt if s[0] < t0]
                inside = [prior[-1]] if prior else []
            b[label] = inside
        return b

    hb_o, hb_p = bucket(orig, 2), bucket(port, 1)
    # only genuine pointer cells have a record block: a 256B array cell also carries a rec_offset from
    # the generator, and dumping past it just re-reads the array
    port_off = {c["name"]: c.get("rec_offset") for c in pcells if c["host_size"] in (4, 8)}
    rows = []
    for i, c in enumerate(ptr_cells):
        off = port_off.get(c["name"])
        hist, vals = collections.Counter(), {}
        n = 0
        for label, _t0, _t1 in phases:
            for bo, bp in zip(hb_o[label], hb_p[label] if label in hb_p else []):
                if off is None:
                    continue
                go, gp = bo[i * RB:(i + 1) * RB], bp[off:off + RB]
                if len(go) != RB or len(gp) != RB or not any(go):
                    continue
                n += 1
                for k in range(RB):
                    if go[k] != gp[k]:
                        hist[k] += 1
                        vals.setdefault(k, (go[k], gp[k]))
        if hist:
            rows.append((sum(hist.values()), c["name"], hex(c["ds"]), n, hist, vals))
    rows.sort(reverse=True)
    print(f"window {a.t0}..{a.t1} ms, {len(phases)} phases")
    for total, name, ds, n, hist, vals in rows[:a.top]:
        fields = ", ".join(
            f"+0x{k:02x}:{vals[k][0]:02x}/{vals[k][1]:02x}({c})"
            for k, c in sorted(hist.items(), key=lambda kv: -kv[1])[:8])
        print(f"{name:24s} ds={ds:9s} {total:5d} differing bytes in {n} blocks  {fields}")


if __name__ == "__main__":
    main()
