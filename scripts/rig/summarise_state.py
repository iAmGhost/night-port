#!/usr/bin/env python3
"""Summarise a state-oracle report: one line per cell, ranked by how many windows disagree.

diff_state.md carries one row per cell with a per-phase note, which is unreadable for a route with
dozens of windows. This collapses it to: verdict, windows compared, windows disagreeing, a sample of
the two sides' values, and whether the disagreement is representational (REPR/SIZE?) rather than
semantic. Cells that are never comparable (NO-DATA everywhere) are called out separately, because on a
mismatched timeline they can fake a clean bill of health.

  summarise_state.py reports/state_diff.md [--top 40] [--only-diff]
"""
import argparse
import collections
import re

ROW = re.compile(r"^\|\s*([A-Z?]+)\s*\|\s*(0x[0-9a-f]+)\s*\|\s*(\S+)\s*\|\s*(\S+)\s*\|\s*(\S+)\s*\|")
PHASE = re.compile(r"(P\d+@\d+)\s+([A-Z?]+(?:-[A-Z]+)?)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("report")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--only-diff", action="store_true")
    a = ap.parse_args()

    rows = []
    for line in open(a.report):
        m = ROW.match(line)
        if not m:
            continue
        verdict, addr, name, gsz, hsz = m.groups()
        phases = PHASE.findall(line)
        counts = collections.Counter(v for _, v in phases)
        # one representative value pair, taken from the first DIFF phase note
        sample = ""
        mm = re.search(r"(P\d+@\d+) [A-Z?-]+ orig=(\S+?) port=(\S+?)(?: \||$)", line)
        if mm:
            sample = f"{mm.group(1)}: orig={mm.group(2)} port={mm.group(3).split(',')[0]}"
        rows.append({"addr": addr, "name": name, "guest": gsz, "host": hsz, "verdict": verdict,
                     "phases": counts, "n": sum(counts.values()), "bad": counts.get("DIFF", 0)
                     + counts.get("REC-DIFF", 0) + counts.get("HEAP", 0),
                     "nodata": counts.get("NO-DATA", 0), "sample": sample})

    nodata = [r for r in rows if r["nodata"] == r["n"] and r["n"]]
    live = [r for r in rows if r["nodata"] != r["n"]]
    live.sort(key=lambda r: (-r["bad"], r["addr"]))
    if a.only_diff:
        live = [r for r in live if r["bad"]]
    print(f"cells: {len(rows)} comparable {len(live)} never-compared {len(nodata)}")
    print(f"{'addr':9s} {'name':22s} {'v':9s} {'bad/n':7s} {'guest/host':11s} sample")
    for r in live[:a.top]:
        print(f"{r['addr']:9s} {r['name'][:22]:22s} {r['verdict']:9s} "
              f"{r['bad']:3d}/{r['n']:<3d} {r['guest']}/{r['host']:6s} {r['sample'][:70]}")
    if nodata:
        print(f"\nnever compared (no matching window on both sides): "
              f"{', '.join(r['name'] for r in nodata[:20])}"
              f"{' ...' if len(nodata) > 20 else ''}")


if __name__ == "__main__":
    main()
