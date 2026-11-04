#!/usr/bin/env python3
"""Standalone differential harness for fn_047b79: generated C vs Python emulator."""
import random, subprocess, os
import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
from fp_047b79_ref import emulate, DE

random.seed(1234)
N = 20000
vecs = []
de = 0
while len(vecs) < N:
    a0 = random.getrandbits(32); a1 = random.getrandbits(32); a2 = random.getrandbits(16)
    b0 = random.getrandbits(32); b1 = random.getrandbits(32); b2 = random.getrandbits(16)
    esi = ((b2 & 0xffff) << 16) | (a2 & 0xffff)
    try:
        r = emulate(a0, b0, b1, a1, esi)
    except DE:
        de += 1
        continue
    vecs.append((a0, a1, esi, b1, b0, r[0], r[1], r[2] & 0xffff))

gen = open('scratch/fn_047b79_gen.c').read()
body = ['#include <stdio.h>', gen, 'static const unsigned long V[] = {']
for v in vecs:
    body.append('    %u,%u,%u,%u,%u,%u,%u,%u,' % v)
body.append('};')
body.append('int main(){ int bad=0; for(unsigned i=0;i<%d;i++){ unsigned long *p=&V[i*8]; '
            'unsigned eax,edx; unsigned short si; fn_047b79(p[0],p[1],p[2],p[3],p[4],&eax,&edx,&si); '
            'if(eax!=p[5]||edx!=p[6]||si!=p[7]){ if(bad<8) printf("MISMATCH i=%%u in=(%%lu,%%lu,%%lu,%%lu,%%lu) '
            'got=(%%u,%%u,%%u) want=(%%lu,%%lu,%%lu)\\n",i,p[0],p[1],p[2],p[3],eax,edx,si,p[5],p[6],p[7]); bad++; } } '
            'printf("checked %%d de-skipped=%%d mismatches=%%d\\n", %d, %d, bad); return bad?1:0; }' % (len(vecs), len(vecs), de))
open('scratch/fp_047b79_diff.c', 'w').write('\n'.join(body))
r = subprocess.run(['cc', '-O1', '-o', 'scratch/fp_047b79_diff', 'scratch/fp_047b79_diff.c'],
                   capture_output=True, text=True)
if r.returncode != 0:
    print("COMPILE ERR:\n", r.stderr[:3000]); raise SystemExit(1)
print(r.stdout)
