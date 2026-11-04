"""Keystone analysis: which unported functions are depended on by the most other unported ones."""
import re, glob, collections
status = open('docs/12-functions/STATUS.md').read()
ported = set(int(x,16) for x in re.findall(r'^\| \[x\] \| 0x([0-9a-f]{6})', status, re.M))
rows = [(int(m.group(1),16), int(m.group(2))) for m in
        re.finditer(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \|', status, re.M)]
pend = dict(rows)
deps = {}
for a,_ in rows:
    fs = glob.glob('analysis/pseudo/*_%06x_*' % a)
    if not fs: deps[a] = set(); continue
    t = open(fs[0]).read()
    c = set(int(x,16) for x in re.findall(r'FUN_00([0-9a-f]{5})', t))
    deps[a] = set(x for x in c if x in pend and x != a)
cnt = collections.Counter()
for a, ds in deps.items():
    for d in ds: cnt[d] += 1
print('pending:', len(pend))
for d, n in cnt.most_common(15):
    print('0x%06x' % d, 'needed by', n, 'pending fns, size', pend.get(d))
print()
small = sorted(((v,k) for k,v in pend.items()))[:12]
for v,k in small: print('small pending 0x%06x size %4d missing %d' % (k, v, len(deps[k])))
