#!/usr/bin/env python3
"""Shipped-side audio transfer census from the I/O-port samples (docs/166).

Input is `capture_original.py --io 0x02,0x03,0x83,0x0a` output (`orig_screen/io.jsonl`).  The game's audio
driver programs the 8237 by hand (fn_040b83: clear the flip-flop on 0x0c, write the 24-bit address low/mid on
0x02, the page on 0x83, then the byte count as n-1 low/high on 0x03, then unmask on 0x0a), so the *changes* of
those registers are the shipped side's transfer programming events -- the only audio-side quantity the shipped
side exposes without a debugger.

Heuristic and its limits, stated because they matter: samples are taken at the capture rate, not per instruction.
A burst writes two bytes to 0x03, and both are usually seen as two changes, so consecutive changes are paired
into count words.  Fast bursts can be missed (a count word seen as one change) or merged, so the COUNT of
programming events is an upper bound on transfers and any single size is weaker evidence than the SHAPE of the
size distribution.

Usage:
  python3 scripts/rig/io_census.py --io DIR/orig_screen/io.jsonl [--port DIR/port/cells.bin]
"""
import argparse
import collections
import json


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--io", required=True)
    ap.add_argument("--port", help="optional port cells.bin, for the port's own feed census (docs/164 §6)")
    ap.add_argument("--md")
    a = ap.parse_args()

    rows = [json.loads(l) for l in open(a.io)]
    chg = collections.Counter()
    for r in rows:
        for k in r.get("changed", {}):
            chg[k] += 1
    print(f"samples: {len(rows)}  register changes: {dict(chg)}")

    seq = [(r["t_ms"], r["changed"]["0x03"][1]) for r in rows if "0x03" in r.get("changed", {})]
    counts = [(t1, v0 | (v1 << 8)) for (t0, v0), (t1, v1) in zip(seq, seq[1:])]
    sizes = [c + 1 for _, c in counts]                    # the count register is programmed as n-1
    hist = collections.Counter(sizes)
    print(f"count-word pairings: {len(counts)}  distinct sizes: {len(hist)}  "
          f"min {min(hist) if hist else 0}  max {max(hist) if hist else 0}")
    print("most common sizes:", hist.most_common(10))
    ladder = sorted(hist)[:24]
    print("small-size ladder:", ladder)
    diffs = collections.Counter(b - x for x, y in zip(ladder, ladder[1:]) for b in [y - x] if b > 0)
    print("gaps between adjacent small sizes:", sorted(diffs.items())[:10])
    per_min = collections.Counter(t // 60000 for t, _ in counts)
    print("programming events per minute:", dict(sorted(per_min.items())))
    tot = sum(sizes)
    if rows:
        span = (rows[-1]["t_ms"] - rows[0]["t_ms"]) / 1000.0
        print(f"bytes programmed over {span:.1f} s: {tot}  ({tot / max(1, span):.0f} B/s of DMA feed)")

    if a.port:
        import struct
        rec = struct.Struct("<qQQQII")
        pd = a.port if not a.port.endswith("cells.bin") else None
        path = a.port
        rb = json.load(open((__import__("os").path.dirname(path) if path.endswith("cells.bin") else path)
                            + "/meta.json"))["record_bytes"]
        n = 0
        with open(path, "rb") as f:
            f.read(16)
            while True:
                b = f.read(rec.size)
                if len(b) < rec.size:
                    break
                n += 1
                f.read(rb)
        print(f"port records: {n}  (its feed census is the audiodump witness: docs/164 §6 -- "
              f"73 feeds on this drive, 70 of them 3609 bytes)")

    if a.md:
        with open(a.md, "w") as f:
            f.write("# shipped-side DMA feed census from I/O samples (docs/166)\n\n")
            f.write(f"- samples {len(rows)}, register changes {dict(chg)}\n")
            f.write(f"- count-word pairings {len(counts)}, distinct sizes {len(hist)}, "
                    f"max {max(hist) if hist else 0}\n")
            f.write(f"- most common sizes: {hist.most_common(10)}\n")
            f.write(f"- total bytes programmed: {tot}\n")


if __name__ == "__main__":
    main()
