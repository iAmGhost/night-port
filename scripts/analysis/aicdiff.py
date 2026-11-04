#!/usr/bin/env python3
"""aicdiff.py -- semantic diff between two Ghidra pseudo listings of the AI-handler family.

The handlers are near-clones whose ONLY differences are semantic; Ghidra's local-variable names
(local_7c, piStack_84, iVar2, LAB_0001a146, ...) shift with stack layout, so a raw diff is 90 %
noise. This tool canonicalizes every local/temp/label name to a per-function token keyed by first
appearance order, so a pure rename produces identical text and only real differences survive.

Usage: python3 scripts/analysis/aicdiff.py <pseudo-file-A> <pseudo-file-B>
"""
import re
import sys
import difflib

NAME = re.compile(r'\b(local_[0-9a-fA-F]+|piStack_[0-9a-fA-F]+|uVar\d+|iVar\d+|fVar\d+|'
                  r'ppiVar\d+|pcVar\d+|cVar\d+|bVar\d+|sVar\d+|dVar\d+|LAB_\w+|case_\w+|'
                  r'\+LAB_\w+|unaff_[A-Z0-9]+|extraout_[A-Z0-9_]+|in_[A-Z0-9_]+)\b')


def canon(path):
    lines = []
    seen = {}

    def sub(m):
        key = m.group(1)
        if key not in seen:
            seen[key] = 'V%d' % len(seen)
        return seen[key]

    for ln in open(path, encoding='utf-8', errors='replace'):
        s = ln.strip()
        # switch labels are not renamed across functions: keep them verbatim so the case bodies
        # line up instead of becoming per-function tokens.
        s = re.sub(r'\bcase_(0x[0-9a-fA-F]+|[0-9]+)\b', lambda m: 'CASE_' + m.group(1), s)
        if not s:
            continue
        if s.startswith('//'):
            continue
        if s.startswith('/*') or s.startswith('*'):
            continue
        # Ghidra spills the return-address stack slot before every call: pure noise
        if re.match(r'^V?\d* ?= \(int \*\)0x[0-9a-f]+;$', s) or re.match(
                r'^\w+ = \(int \*\)0x[0-9a-f]+;$', s):
            continue
        s = re.sub(r'FUN_000\w+', 'FN', s)
        s = NAME.sub(sub, s)
        lines.append(s)
    return lines


def resolve(arg):
    """Accept either a path or a bare hex address (e.g. 19e48) and find the pseudo listing."""
    import glob
    import os
    if os.path.exists(arg):
        return arg
    hits = sorted(glob.glob('analysis/pseudo/*FUN_000%s*.c' % arg.lstrip('0') or '0'))
    if len(hits) == 1:
        return hits[0]
    raise SystemExit('cannot resolve %r to a pseudo listing (%d hits)' % (arg, len(hits)))


CAST = re.compile(r'\((?:char|int|uint|byte|short|long|float|double|undefined\d+|'
                  r'ulonglong|uint\w*|int \*|char \*|undefined8|undefined4)\)')


def strip_vars(line):
    """Normalize away token numbers, Ghidra's pointer casts and its byte-access parentheses, so
    that a pure rename -- or the same code reached through a differently-typed entity variable --
    compares equal. This is what makes the hunk list trustworthy."""
    s = re.sub(r'V\d+', 'V', line)
    # Ghidra reaches the same entity field through different cast chains depending on how the
    # entity variable happened to be typed in this function. Normalize every "field of a pointer"
    # spelling to one form so those differences stop showing up as hunks.
    s = re.sub(r'\*\((?:int|uint|float|short|byte|undefined\d+|char \*|int \*)\s*\*\)'
               r'\(\s*(?:\(int\)\s*)?\(char \*\)?\s*(\w+)\s*\+\s*(0x[0-9a-fA-F]+)\s*\)',
               r'\1[\2]', s)
    s = re.sub(r'\(\s*\(char \*\)\s*(\w+)\s*\)\[\s*(0x[0-9a-fA-F]+)\s*\]', r'\1[\2]', s)
    s = re.sub(r'\(\s*\(int\)\s*(\w+)\s*\)\[', r'\1[', s)
    s = CAST.sub('', s)
    s = re.sub(r'\s+', ' ', s.replace('(V ', 'V ').replace(' *)', ')')).strip()
    return s


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    a, b = canon(resolve(sys.argv[1])), canon(resolve(sys.argv[2]))
    sm = difflib.SequenceMatcher(None, a, b)
    hunks = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            continue
        al, bl = a[i1:i2], b[j1:j2]
        # A hunk that is only a variable RENAME (the canonical numbering shifted because some
        # earlier local was added/removed) is noise: drop it.
        if len(al) == len(bl) and [strip_vars(x) for x in al] == [strip_vars(x) for x in bl]:
            continue
        decl = re.compile(r'^[A-Za-z_][A-Za-z0-9_ ]* \**V\d+( \[\d+\])?;$')
        if al and bl and all(decl.match(strip_vars(x)) for x in al + bl):
            continue                      # pure declaration-list reshuffle
        hunks.append((tag, i1, i2, j1, j2, al, bl))
    print('semantic ratio %.3f   lines %d / %d   %d real hunks'
          % (sm.ratio(), len(a), len(b), len(hunks)))
    for tag, i1, i2, j1, j2, al, bl in hunks:
        print('\n== %s  A[%d:%d]  B[%d:%d]' % (tag, i1, i2, j1, j2))
        for x in al[:40]:
            print('   - ' + x[:170])
        for y in bl[:40]:
            print('   + ' + y[:170])
    return 0


if __name__ == '__main__':
    sys.exit(main())
