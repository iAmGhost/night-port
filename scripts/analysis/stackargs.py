import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import re,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
D=0x2600; d=open(NIGHT_EXE,'rb').read()
st=open('docs/12-functions/STATUS.md').read()
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
for a in sys.argv[1:]:
    va=int(a,16); m=re.search(r'\| (?:\[x\]|\[ \]|\[-\]) \| 0x%s \| (\d+) \|'%a, st)
    n=int(m.group(1)) if m else 400
    ins=list(md.disasm(d[va-D:va-D+n],va))
    slots={}
    for i in ins[:12]: pass
    for i in ins:
        for mm in re.finditer(r'\[ebp \+ (0x[0-9a-f]+)\]', i.op_str or ''):
            slots.setdefault(mm.group(1),[0,0])
            k = 1 if i.mnemonic.startswith('mov') and i.op_str.strip().startswith('dword ptr [ebp') else 0
            slots[mm.group(1)][k]+=1
    print(a, "insns",len(ins), "stack slots:", {k:tuple(v) for k,v in sorted(slots.items())})
