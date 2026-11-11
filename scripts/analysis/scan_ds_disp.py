import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
d=open(NIGHT_EXE,'rb').read(); D=0x2600
lo=int(sys.argv[1],16); hi=int(sys.argv[2],16)
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
n=0
for ins in md.disasm(d[D:0x4b000+D], 0x2600):
    for op in ins.operands:
        if op.type==3 and lo<=op.mem.disp<=hi:
            print("%08x  %-8s %s"%(ins.address,ins.mnemonic,ins.op_str)); n+=1
print(n,"hits", file=sys.stderr)
