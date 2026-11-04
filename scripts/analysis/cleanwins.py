#!/usr/bin/env python3
"""Scan STATUS.md for unported functions whose every FUN_ callee is already [x].

Usage: python3 scratch/cleanwins.py [max_size]
Prints (size, addr, callee_count, pseudo_file) sorted by size, i.e. the bottom-up
"clean win" frontier. Run it before choosing the next port target.

Result on 2026-07-13 (448/528 done): ZERO clean wins remain at size <= 220 B --
the small tiers are exhausted, so the remaining work is the chain upward:
0x010efc, the 0x012616 / 0x012efa / 0x01348f / 0x01441e cluster, 0x037b03 (stage
load) and 0x0461cc (FPU engine).
"""
import re, glob, sys

MAX = int(sys.argv[1]) if len(sys.argv) > 1 else 220
status = open('docs/12-functions/STATUS.md').read()
ported = set(int(x, 16) for x in re.findall(r'^\| \[x\] \| 0x([0-9a-f]{6})', status, re.M))
rows = [(int(m.group(1), 16), int(m.group(2)))
        for m in re.finditer(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \|', status, re.M)]

hits = []
for addr, size in rows:
    if size > MAX or size < 8:
        continue
    fs = glob.glob('analysis/pseudo/*_%06x_*' % addr)
    if not fs:
        continue
    txt = open(fs[0]).read()
    callees = set(int(x, 16) for x in re.findall(r'FUN_00([0-9a-f]{5})', txt))
    missing = sorted(c for c in callees if c not in ported and c != addr)
    if not missing:
        hits.append((size, '0x%06x' % addr, len(callees), fs[0]))

for h in sorted(hits):
    print(h)
print('clean wins at size <= %d: %d' % (MAX, len(hits)))
