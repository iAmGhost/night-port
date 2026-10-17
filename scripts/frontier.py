#!/usr/bin/env python3
"""List level-N functions whose callees are ALL already ported (clean wins)."""
import sys, re, os
base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
lvl = int(sys.argv[1]) if len(sys.argv) > 1 else 2

# done addrs from STATUS.md
done = set()
with open(os.path.join(base, "docs/12-functions/STATUS.md")) as f:
    for line in f:
        m = re.match(r"\|\s*\[x\]\s*\|\s*0x([0-9a-f]{4,6})", line)
        if m:
            done.add(m.group(1).lower())

# rank: addr -> (level, size, name)
rank = {}
with open(os.path.join(base, "analysis/rank.tsv")) as f:
    next(f)
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= 4:
            rank[p[1].lower()] = (int(p[0]), int(p[2]), p[3])

# callgraph: caller -> set(callee addrs)
callees = {}
with open(os.path.join(base, "analysis/callgraph.tsv")) as f:
    next(f)
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) >= 3:
            callees.setdefault(p[0].lower(), set()).add(p[2].lower())

# clean = level matches, not done, all callees done
clean = []
for addr, (lv, size, name) in rank.items():
    if lv != lvl or addr in done:
        continue
    cs = callees.get(addr, set())
    missing = [c for c in cs if c not in done]
    if not missing:
        clean.append((size, addr, name, len(cs)))

clean.sort()
print(f"# L{lvl} clean candidates (all callees ported): {len(clean)}")
for size, addr, name, ncs in clean:
    print(f"{size:5d}  0x{addr}  {name}  ({ncs} callees)")
