"""linear-sweep xrefs to a target: scripts/analysis/xrefs.py <target-va-hex>"""
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600; T=int(sys.argv[1],16)
data=open('NIGHT/NIGHT.EXE','rb').read()
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
for ins in md.disasm(data[0x7800-D:0x49000-D], 0x7800):
    if ins.mnemonic in ('call','jmp') and ins.op_str.startswith('0x'):
        if int(ins.op_str,16)==T: print("%08x %s" % (ins.address, ins.mnemonic))
