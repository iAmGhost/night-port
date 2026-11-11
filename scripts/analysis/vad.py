#!/usr/bin/env python3
"""VA -> file offset helper for NIGHT.EXE (Ghidra flat VA = file offset + 0x2600).
Usage:
  vad.py dis <va> <len>          -> capstone text listing
  vad.py hex <va> <len>          -> hex dump
  vad.py words <va> <count>      -> 16-bit little-endian table
  vad.py dwords <va> <count>     -> 32-bit little-endian table
"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import sys
EXE = NIGHT_EXE
DELTA = 0x2600
data = open(EXE, 'rb').read()

def rd(va, n):
    off = va - DELTA
    return data[off:off + n]

def main():
    mode, va = sys.argv[1], int(sys.argv[2], 0)
    n = int(sys.argv[3], 0)
    b = rd(va, n)
    if mode == 'dis':
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        md.detail = True
        for i in md.disasm(b, va):
            print('%08x  %-24s %s %s' % (i.address, i.bytes.hex(), i.mnemonic, i.op_str))
    elif mode == 'hex':
        for k in range(0, len(b), 16):
            print('%08x  %s' % (va + k, b[k:k+16].hex()))
    elif mode == 'words':
        import struct
        for k in range(0, n * 2, 2):
            print('%08x  %04x' % (va + k, struct.unpack_from('<H', b, k)[0]))
    elif mode == 'dwords':
        import struct
        for k in range(0, n * 4, 4):
            print('%08x  %08x' % (va + k, struct.unpack_from('<I', b, k)[0]))

main()
