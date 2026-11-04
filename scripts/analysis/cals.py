"""callee scan: scripts/analysis/cals.py <va-hex> <size-dec> -- call targets with ported-name or PENDING tag"""
import sys, re, struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600
va=int(sys.argv[1],16); size=int(sys.argv[2])
data=open('NIGHT/NIGHT.EXE','rb').read()
ST=open('docs/12-functions/STATUS.md').read()
names={}
for m in re.finditer(r'\| \[(.)\] \| (0x[0-9a-f]{6}) \| \d+ \| (\S+) \|', ST):
    names[int(m.group(2),16)]=(m.group(1), m.group(3))
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
out={}
for ins in md.disasm(data[va-D:va-D+size], va):
    if ins.mnemonic=='call' and ins.op_str.startswith('0x'):
        t=int(ins.op_str,16)
        st,nm=names.get(t,('?','UNKNOWN'))
        out[t]=(st,nm)
for t,(st,nm) in sorted(out.items()):
    print("%08x [%s] %s" % (t, st, nm))
print("size=%d" % size)
