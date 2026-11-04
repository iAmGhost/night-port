"""aidd.py -- annotated AI-handler listing: fdis + call-target names + DS data values.

Usage: python3 scripts/analysis/aidd.py <va-hex> <end-hex> [--nodata]
"""
import sys, re, struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D = 0x2600
DATA = open('NIGHT/NIGHT.EXE', 'rb').read()
ST = open('docs/12-functions/STATUS.md').read()
NAMES = {}
for m in re.finditer(r'\| \[.\] \| (0x[0-9a-f]{6}) \| \d+ \| (fn_[0-9a-f]+) \|', ST):
    NAMES[int(m.group(1), 16)] = m.group(2)

def dsval(disp):
    """DS segment displacement -> image VA = disp + 0x50000 (capstone omits the base)."""
    va = disp + 0x50000
    off = va - 0x7600          # data segment: file offset = VA - 0x7600
    if off < 0 or off + 8 > len(DATA):
        return None
    d = struct.unpack_from('<d', DATA, off)[0]
    f = struct.unpack_from('<f', DATA, off)[0]
    u = struct.unpack_from('<I', DATA, off)[0]
    dq = (abs(d) > 1e-30 and abs(d) < 1e30 and abs(d - round(d)) < 1e-9)
    fq = (abs(f) > 1e-30 and abs(f) < 1e30 and abs(f - round(f)) < 1e-5)
    if dq: return 'qword=%.6g' % d
    if fq: return 'dword=%.6gf' % f
    if u: return 'dword=%d' % u
    return None

md = Cs(CS_ARCH_X86, CS_MODE_32); md.detail = True
start, end = int(sys.argv[1], 16), int(sys.argv[2], 16)
for ins in md.disasm(DATA[start - D:end - D], start):
    ann = []
    if ins.mnemonic == 'call':
        try:
            t = int(ins.op_str, 16)
            if t in NAMES: ann.append(NAMES[t])
        except ValueError: pass
    m = re.search(r'dword ptr \[0x([0-9a-f]+)\]|qword ptr \[0x([0-9a-f]+)\]', ins.op_str)
    if m:
        d = int(m.group(1) or m.group(2), 16)
        v = dsval(d) if '--nodata' not in sys.argv else None
        ann.append('DS 0x%x = %s' % (d, v))
    print('%08x  %-30s %s' % (ins.address, (ins.mnemonic + ' ' + ins.op_str).strip(),
                              '; '.join(ann)))
