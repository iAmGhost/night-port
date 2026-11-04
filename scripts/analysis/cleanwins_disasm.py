"""True clean-win scan: dependency graph from CAPSTONE disassembly (not Ghidra pseudo).
A pending function is a clean win when none of its direct call targets is also pending."""
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
EXE='NIGHT/NIGHT.EXE'; D=0x2600
status=open('docs/12-functions/STATUS.md').read()
ported=set(int(x,16) for x in re.findall(r'^\| \[x\] \| 0x([0-9a-f]{6})', status, re.M))
rows=[(int(m.group(1),16), int(m.group(2))) for m in
      re.finditer(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \|', status, re.M)]
pend=dict(rows)
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
deps={}
for a,size in rows:
    code=open(EXE,'rb').read()[a-D:a-D+size]
    t=set()
    for ins in md.disasm(code, a):
        if ins.mnemonic=='call' and re.fullmatch(r'0x[0-9a-f]+', ins.op_str or ''):
            t.add(int(ins.op_str,16))
    deps[a]={x for x in t if x in pend and x!=a}
wins=[(pend[a], '0x%06x'%a, sorted('0x%06x'%c for c in deps[a] if c not in pend))
      for a in pend if not deps[a]]
for w in sorted(wins): print('CLEAN', w[1], 'size', w[0], 'external callees:', ', '.join(w[2][:8]))
print('clean wins (disasm-based):', len(wins))
cnt={}
for a in pend:
    for d in deps[a]: cnt[d]=cnt.get(d,0)+1
print('--- keystones (pending fn needed by most pending fns) ---')
for d,n in sorted(cnt.items(), key=lambda kv:-kv[1])[:10]:
    print('  0x%06x needed by %2d pending, size %d' % (d,n,pend[d]))
