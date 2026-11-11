"""linear-sweep xrefs to a target: scripts/analysis/xrefs.py <target-va-hex>"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600; T=int(sys.argv[1],16)
data=open(NIGHT_EXE,'rb').read()
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
for ins in md.disasm(data[0x7800-D:0x49000-D], 0x7800):
    if ins.mnemonic in ('call','jmp') and ins.op_str.startswith('0x'):
        if int(ins.op_str,16)==T: print("%08x %s" % (ins.address, ins.mnemonic))
