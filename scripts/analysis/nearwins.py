"""Near-clean wins: unported functions with exactly N missing callees (default 1).
Usage: uv run python3 scratch/nearwins.py [max_missing]"""
import re, glob, sys
NMAX = int(sys.argv[1]) if len(sys.argv) > 1 else 1
status = open('docs/12-functions/STATUS.md').read()
ported = set(int(x, 16) for x in re.findall(r'^\| \[x\] \| 0x([0-9a-f]{6})', status, re.M))
rows = [(int(m.group(1), 16), int(m.group(2)), m.group(3))
        for m in re.finditer(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \| (\S+)', status, re.M)]
lvl = {}
for m in re.finditer(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \| \S+ \|', status, re.M):
    pass
hits = []
for addr, size, name in rows:
    fs = glob.glob('analysis/pseudo/*_%06x_*' % addr)
    if not fs: continue
    txt = open(fs[0]).read()
    callees = set(int(x, 16) for x in re.findall(r'FUN_00([0-9a-f]{5})', txt))
    missing = sorted(c for c in callees if c not in ported and c != addr)
    if 0 < len(missing) <= NMAX:
        hits.append((len(missing), size, '0x%06x' % addr, ['0x%06x' % m for m in missing], fs[0]))
for h in sorted(hits)[:40]:
    print(h[2], 'size', h[1], 'missing', h[3])
print('near wins (<=%d missing):' % NMAX, len(hits))
