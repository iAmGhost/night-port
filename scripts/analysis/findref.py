"""find every instruction referencing a DS displacement: scripts/analysis/findref.py <disp-hex>"""
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600; disp=sys.argv[1].lower()
data=open('NIGHT/NIGHT.EXE','rb').read()
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
n=0
for va in range(0x7800,0x49000,1):
    try:
        ins=next(md.disasm(data[va-D:va-D+16], va))
    except Exception:
        continue
    if ins.op_str.count('0x%s'%disp)+ins.op_str.count('[%s]'%disp) and disp in ins.op_str:
        print("%08x  %s %s" % (ins.address, ins.mnemonic, ins.op_str)); n+=1
print("refs=%d"%n)
