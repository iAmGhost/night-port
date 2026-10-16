#!/usr/bin/env python
"""Progress report for docs/12-functions/STATUS.md.

Parses the per-level tables (columns: Status | Addr | Size | Name |
Subsystem | Notes) and prints:
  - totals (done/analyzed/skipped/pending)
  - per-level breakdown
  - the next N pending functions in work order
  - ordering violations (a pending/done function whose callee is not
    done/skipped, per analysis/callgraph.tsv + analysis/rank.tsv)

Usage: uv run scripts/status_report.py [next_count]
"""
import csv
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STATUS = ROOT / "docs/12-functions/STATUS.md"
CG = ROOT / "analysis/callgraph.tsv"
RANK = ROOT / "analysis/rank.tsv"

row_re = re.compile(
    r"^\|\s*\[( |~|x|-)\]\s*\|\s*0x([0-9a-f]{6})\s*\|\s*(\d+)\s*\|\s*([^|]*)\|"
)


def main() -> None:
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    states: dict[int, str] = {}
    for line in STATUS.read_text().splitlines():
        m = row_re.match(line)
        if m:
            states[int(m.group(2), 16)] = m.group(1)

    if not states:
        sys.exit("no rows parsed from STATUS.md")

    c = Counter(states.values())
    done, analyzed, skipped = c.get("x", 0), c.get("~", 0), c.get("-", 0)
    pending = c.get(" ", 0)
    print(f"TOTALS: {done}/{len(states)} done | "
          f"{analyzed} analyzed | {skipped} skipped | {pending} pending")

    rank = {int(r[1], 16): int(r[0]) for r in list(csv.reader(
        open(RANK), delimiter="\t"))[1:]}
    per = Counter()
    for a, s in states.items():
        per[(rank.get(a, -1), s)] += 1
    lvmax = max(k[0] for k in per)
    print("\nLevel  " + "".join(f"{'[x]':>6}{'[~]':>6}{'[-]':>6}{'[ ]':>6}"
                               for _ in [0]))
    for l in range(lvmax + 1):
        cells = "".join(f"{per.get((l, s), 0):>6}" for s in ("x", "~", "-", " "))
        print(f"L{l:<3d}  {cells}")

    calls: dict[int, set[int]] = {}
    for c1, _, t in list(csv.reader(open(CG), delimiter="\t"))[1:]:
        calls.setdefault(int(c1, 16), set()).add(int(t, 16))

    order = sorted(states, key=lambda a: (rank.get(a, 999), a))
    nxt = [a for a in order if states[a] == " "][:n]
    print(f"\nNext {len(nxt)} pending (work order):")
    for a in nxt:
        print(f"  L{rank.get(a, -1)}  0x{a:06x}")

    ok = {"x", "-"}
    viol = []
    for a in order:
        if states[a] in ok:
            for t in calls.get(a, ()):
                if t in states and states[t] not in ok:
                    viol.append((a, t, states[a]))
    if viol:
        print("\nOrdering violations (ported before callee):")
        for a, t, s in viol[:20]:
            print(f"  0x{a:06x}({s}) calls 0x{t:06x} ({states[t]})")
    else:
        print("\nOrdering: OK (no done/skipped function has a pending callee)")


if __name__ == "__main__":
    main()
