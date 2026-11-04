"""Capstone listing for a NIGHT.EXE function: fdis.py <va-hex> [size]"""
import sys, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600
va=int(sys.argv[1],16); data=open('NIGHT/NIGHT.EXE','rb').read()
if len(sys.argv)>2: size=int(sys.argv[2],16) if len(sys.argv[2])>4 else int(sys.argv[2])
else:
    st=open('docs/12-functions/STATUS.md').read()
    m=re.search(r'\| (?:\[.\]) \| 0x%06x \| (\d+) \|'%va, st); size=int(m.group(1)) if m else 256
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
for ins in md.disasm(data[va-D:va-D+size], va):
    print("%08x  %-24s %s %s" % (ins.address, ins.bytes.hex(), ins.mnemonic, ins.op_str))
