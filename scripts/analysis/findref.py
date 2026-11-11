"""find every instruction referencing a DS displacement: scripts/analysis/findref.py <disp-hex>"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600; disp=sys.argv[1].lower()
data=open(NIGHT_EXE,'rb').read()
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
