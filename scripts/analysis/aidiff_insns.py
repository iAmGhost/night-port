#!/usr/bin/env python3
"""aidiff_insns.py -- instruction-stream diff for the AI-handler family.

The handlers are near-clones compiled from one source pattern; Ghidra pseudo and even the raw
disassembly differ in local names / stack slots / branch targets, so compare the *mnemonic + masked
operand* stream: every immediate and displacement becomes '#', every register pair keeps its name.
SequenceMatcher over that stream then reports which regions are genuinely new.

Usage: python3 scripts/analysis/aidiff_insns.py <va-A> <va-B> [--full]
"""
import re, sys, difflib
rawA = rawB = []
STACKSHIFT = False
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

D = 0x2600
DATA = open('NIGHT/NIGHT.EXE', 'rb').read()
ST = open('docs/12-functions/STATUS.md').read()
md = Cs(CS_ARCH_X86, CS_MODE_32)

NUM = re.compile(r'0x[0-9a-fA-F]+|\b\d+\b')

def size_of(va):
    m = re.search(r'\| \[.\] \| 0x%06x \| (\d+) \|' % va, ST)
    return int(m.group(1)) if m else None

def stream(va):
    n = size_of(va)
    assert n, 'no size for %x in STATUS.md' % va
    out = []
    for ins in md.disasm(DATA[va - D:va - D + n], va):
        ops = NUM.sub('#', ins.op_str)
        out.append((ins.address, (ins.mnemonic + ' ' + ops).strip()))
    return out


BR = {'jmp', 'call', 'jmp.far', 'je', 'jne', 'jb', 'jbe', 'ja', 'jae', 'jl', 'jle', 'jg', 'jge',
      'jc', 'jnc', 'jz', 'jnz', 'jo', 'jno', 'js', 'jns', 'jp', 'jnp', 'jecxz', 'loop'}


def rawstream(va):
    """Text used for the substitution pass: immediates kept, branch/call targets masked."""
    n = size_of(va)
    out = []
    for i in md.disasm(DATA[va - D:va - D + n], va):
        t = ('%s %s' % (i.mnemonic, i.op_str)).strip()
        if i.mnemonic in BR:
            t = NUM.sub('#', t)
        if STACKSHIFT:                       # locals shift when the frame size differs: rename noise
            t = re.sub(r'\[ebp *[-+] *(0x[0-9a-fA-F]+|\d+)\]', '[ebp-#]', t)
        out.append(t)
    return out

def main():
    full = '--full' in sys.argv
    a, b = int(sys.argv[1], 16), int(sys.argv[2], 16)
    A, B = stream(a), stream(b)
    global rawA, rawB
    global STACKSHIFT
    STACKSHIFT = size_of(a) != size_of(b)          # frame sizes usually differ -> mask [ebp-*]
    rawA, rawB = rawstream(a), rawstream(b)
    sm = difflib.SequenceMatcher(None, [x[1] for x in A], [x[1] for x in B], autojunk=False)
    match = sum(blk.size for blk in sm.get_matching_blocks())
    print('A=0x%06x(%d insns)  B=0x%06x(%d insns)  shared=%d (%.1f%% of A)'
          % (a, len(A), b, len(B), match, 100.0 * match / len(A)))
    if '--consts' in sys.argv:
        # Matched pairs are only *shape*-equal.  Compare them again with branch/call targets masked
        # (those always differ between clones) and report every pair whose remaining text differs:
        # that is the list of real constant substitutions.
        pairs = [(i, j) for tag, i1, i2, j1, j2 in sm.get_opcodes() if tag == 'equal'
                 for i, j in zip(range(i1, i2), range(j1, j2)) if rawA[i] != rawB[j]]
        runs, cur = [], None
        for i, j in pairs:
            if cur and cur[0][0] == i - 1 and cur[0][1] == j - 1:
                cur.append((i, j))
            else:
                cur = [(i, j)]
                runs.append(cur)
        print('-- %d substitution runs / %d differing pairs' % (len(runs), len(pairs)))
        for r in runs:
            print('   run A 0x%06x..0x%06x (%d)' % (A[r[0][0]][0], A[r[-1][0]][0], len(r)))
            for i, j in r[:12]:
                print('     A %08x %-36s | B %08x %s' % (A[i][0], rawA[i], B[j][0], rawB[j]))
        sys.exit(0)

    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            continue
        print('== %s A[0x%06x-0x%06x] %d | B[0x%06x-0x%06x] %d'
              % (tag, A[i1][0], A[max(i1, i2 - 1)][0], i2 - i1, B[j1][0], B[max(j1, j2 - 1)][0], j2 - j1))
        if full or (i2 - i1) + (j2 - j1) <= 24:
            for addr, s in A[i1:i2]:
                print('   A %08x  %s' % (addr, s))
            for addr, s in B[j1:j2]:
                print('   B %08x  %s' % (addr, s))

main()
