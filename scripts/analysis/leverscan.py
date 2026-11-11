"""Skeleton + dependency classification for the big lever functions (0x012efa / 0x012616 / 0x01441e)."""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import re, collections, subprocess, json
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86_const import X86_OP_MEM, X86_OP_IMM
EXE=NIGHT_EXE; D=0x2600
status=open('docs/12-functions/STATUS.md').read()
ported=set(int(x,16) for x in re.findall(r'^\| \[x\] \| 0x([0-9a-f]{6})', status, re.M))
pending=set(int(x,16) for x in re.findall(r'^\| \[ \] \| 0x([0-9a-f]{6})', status, re.M))
for va,n,name in [(0x12efa,0x594,'0x012efa'),(0x12616,0x8e3,'0x012616'),(0x1441e,0xd80,'0x01441e')]:
    code=open(EXE,'rb').read()[va-D:va-D+n]
    md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
    calls=[]; globs=collections.OrderedDict(); loops=[]; prot=[]
    for i,ins in enumerate(md.disasm(code, va)):
        if i<14: prot.append('%06x %s %s'%(ins.address, ins.mnemonic, ins.op_str))
        for op in ins.operands:
            if op.type==X86_OP_MEM and op.mem.base==0 and op.mem.disp>0x1000:
                d=op.mem.disp
                if d>=0x10000: globs.setdefault('DAT_%06X'%(d+0x50000),[]).append(ins.address)
        if ins.mnemonic=='call' and ins.op_str.startswith('0x'):
            t=int(ins.op_str,16)
            kind='PORTED' if t in ported else ('PENDING' if t in pending else 'other')
            calls.append((ins.address,t,kind))
        if ins.mnemonic.startswith('j') and re.fullmatch(r'0x[0-9a-f]+', ins.op_str or ''):
            t=int(ins.op_str,16)
            if t<=ins.address: loops.append((ins.address,t))
    uniq={}
    for a,t,k in calls: uniq.setdefault(t,[0,k]); uniq[t][0]+=1
    pend=[('0x%06x'%t,c) for t,(c,k) in uniq.items() if k=='PENDING']
    print('### %s  (%d insns, %d calls, %d globals, %d loops)'%(name, len(list(calls))+0, len(calls), len(globs), len(loops)))
    print(' prologue:', ' | '.join(prot[:8]))
    print(' unported callees:', ', '.join('%s x%d'%p for p in sorted(pend)) or 'NONE')
    print(' ported callee targets:', ', '.join('0x%06x'%t for t,(c,k) in uniq.items() if k=='PORTED'))
    print(' globals:', ', '.join('%s(%d)'%(g,len(a)) for g,a in list(globs.items())[:40]))
    print()
