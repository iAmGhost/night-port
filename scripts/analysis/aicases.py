#!/usr/bin/env python3
"""aicases.py -- compare the AI-handler family pseudo listings case by case.

Splits each canonical listing at its `switch(...) {` and then at each `case 0x..:` label, and diffs
the bodies of matching cases. Prints only the cases that actually differ, so a 4 KB handler reduces
to the handful of state-machine blocks that changed.

Usage: python3 scripts/analysis/aicases.py <addrA> <addrB>
"""
import re, sys, difflib, glob
import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
from aicdiff import canon, strip_vars   # reuse the canonicalizer


def find(addr):
    g = glob.glob('analysis/pseudo/*_FUN_000%s_*.c' % addr)
    assert len(g) == 1, g
    return canon(g[0])


def split_cases(lines):
    """Return {case_label: [lines]} for the block after `switch(...) {`."""
    try:
        i = next(k for k, l in enumerate(lines) if l.startswith('switch('))
    except StopIteration:
        return {}
    body, cur, out = lines[i + 1:], None, {}
    depth = 0
    for l in body:
        if l.startswith('case') and depth == 0:
            cur = re.sub(r'[^0-9a-fA-F]', '', l.split()[1]) or '?'
            out[cur] = []
        if cur is not None:
            out[cur].append(l)
        depth += l.count('{') - l.count('}')
        if depth < 0:
            break
    return out


def main():
    A, B = find(sys.argv[1]), find(sys.argv[2])
    ca, cb = split_cases(A), split_cases(B)
    keys = sorted(set(ca) | set(cb), key=lambda k: int(k, 16) if k != '?' else -1)
    print('cases: A=%d B=%d' % (len(ca), len(cb)))
    for k in keys:
        la = [strip_vars(x) for x in ca.get(k, [])]
        lb = [strip_vars(x) for x in cb.get(k, [])]
        if la == lb:
            continue
        print('\n### case %s   (%d vs %d lines)' % (k, len(la), len(lb)))
        for d in difflib.unified_diff(ca.get(k, []), cb.get(k, []), lineterm='', n=1):
            if d.startswith('---') or d.startswith('+++') or d.startswith('@@'):
                continue
            print('  ' + d)


if __name__ == '__main__':
    main()
