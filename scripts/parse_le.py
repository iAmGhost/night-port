#!/usr/bin/env python3
"""Parse MZ + LE headers of NIGHT.EXE and dump compiler/extender markers.
Usage: parse_le.py [exe_path] [out_path]
Writes a report; prints only a short summary to stdout.

LE header field offsets are relative to the 'LE' magic (2 bytes), i.e. the
standard LE header begins at magic+2.
"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__))))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import struct, re, sys

exe = sys.argv[1] if len(sys.argv) > 1 else NIGHT_EXE
outp = sys.argv[2] if len(sys.argv) > 2 else 'scratch/le_report.txt'
d = open(exe, 'rb').read()
L = []
def w(s=''): L.append(s)

w(f"file: {exe}  size={len(d)} ({len(d):#x})")
assert d[:2] == b'MZ', 'not MZ'
e_lfanew = struct.unpack_from('<I', d, 0x3c)[0]
w(f"\nMZ: e_lfanew={e_lfanew:#x}  e_oemid={struct.unpack_from('<H',d,0x3e)[0]:#06x}  "
  f"e_lfarlc={struct.unpack_from('<H',d,0x32)[0]}")
stub_msg_off = struct.unpack_from('<H', d, 0x3a)[0]
stub_msg_len = struct.unpack_from('<H', d, 0x38)[0]
if stub_msg_off:
    w("DOS stub msg: " + repr(d[stub_msg_off:stub_msg_off+stub_msg_len]))
o = e_lfanew
w(f"\nLE header @ {o:#x}, magic={d[o:o+2]!r}  (fields below offset from 'LE' magic)")
def H(x): return struct.unpack_from('<H', d, o+x)[0]
def I(x): return struct.unpack_from('<I', d, o+x)[0]

# standard LE layout, offsets from 'LE' magic
rows = [
 (0x02,'version','h'),(0x04,'revision','h'),(0x06,'offset_to_flags','h'),(0x08,'flags','h'),
 (0x0A,'sect_align','h'),(0x0C,'stack_seg_idx','h'),(0x0E,'stack_size','i'),
 (0x12,'start_ip','i'),(0x16,'start_ip_seg_idx','h'),(0x18,'crc','i'),
 (0x1C,'autodata_seg_idx','h'),(0x1E,'sys_flag','h'),(0x20,'max_res','h'),(0x22,'min_res','h'),
 (0x24,'ord_base','h'),(0x26,'sel_base','h'),(0x28,'grp_ord','h'),(0x2A,'grp_sel','h'),
 (0x2C,'lang_ord','h'),(0x2E,'lang_sel','h'),(0x30,'code_page','h'),(0x32,'oem_func','h'),
 (0x34,'code_memsz','i'),(0x38,'data_memsz','i'),(0x3C,'fixed','h'),
 (0x3E,'mem_base','i'),(0x42,'data_base','i'),(0x46,'pm_version','h'),
 (0x48,'pref_entry_ip','h'),(0x4A,'pref_entry_seg_idx','h'),(0x4C,'page_mask','i'),(0x50,'page_shift','h'),
 (0x52,'cseltbl_off','i'),(0x56,'num_csels','i'),
 (0x5A,'reseltbl_off','i'),(0x5E,'num_resels','i'),
 (0x62,'modref_off','i'),(0x66,'switchtbl_off','i'),(0x6A,'usertbl_off','i'),
 (0x6E,'segusertbl_off','i'),(0x72,'segmaptbl_off','i'),(0x76,'sectab_off','i'),
 (0x7A,'num_sects','h'),(0x7C,'id1_off','h'),(0x7E,'num_id1','h'),
 (0x80,'id2_off','h'),(0x82,'num_id2','h'),(0x84,'nonmov_pages','h'),
 (0x86,'checksum','i'),(0x8A,'entry_cs_idx','h'),(0x8C,'entry_ip','h'),(0x8E,'flags2','h'),
 (0x90,'min_stack','h'),(0x92,'max_stack','h'),(0x94,'min_heap','h'),(0x96,'max_heap','h'),
 (0x98,'platform','h'),
]
w("\nLE header fields:")
for off, name, fmt in rows:
    v = H(off) if fmt == 'h' else I(off)
    w(f"  {off:#05x} {name:18s} = {v:#010x}  ({v})")

ns = H(0x7A); so = I(0x76)
w(f"\nsection table: abs {o+so:#x}, count={ns}")
if 0 < ns <= 64 and o+so+12*ns <= len(d):
    for i in range(ns):
        s = o+so+i*12
        name, sflags, soff, ssize, pageoff, mem = struct.unpack_from('<4sHIIHH', d, s)
        perm = ('R' if sflags & 2 else '-') + ('W' if sflags & 4 else '-')
        w(f"  [{i:2d}] {name.decode(errors='replace'):12s} {perm}  flags={sflags:#04x} fileoff={soff:#08x} "
          f"fsize={ssize:#08x} pageoff={pageoff:#06x} pages={mem}")
else:
    w("  !! section count/offset looks invalid; dumping raw 96 bytes at sectab_off:")
    raw = d[o+so:o+so+96]
    for i in range(0, 96, 16):
        w("    " + raw[i:i+16].hex() + "  " + ''.join(chr(c) if 32<=c<127 else '.' for c in raw[i:i+16]))
    w("  raw bytes at LE+0x76..0x9A: " + d[o+0x76:o+0x9A].hex())

for base, cnt, label in ((H(0x7C), H(0x7E), 'module'), (H(0x80), H(0x82), 'group')):
    if base and cnt:
        off = o + base; strs = []
        for _ in range(min(cnt, 8)):
            ln = d[off]
            if ln > 40: break
            strs.append(d[off+1:off+1+ln].decode(errors='replace')); off += 1 + ln
        w(f"\n{label} id strings: {strs}")

w("\ncompiler / extender markers in raw bytes:")
for pat in [b'Watcom', b'WATCOM', b'Rational', b'RATIONAL', b'Copyright', b'DOS/4G', b'4GW',
            b'Turbo', b'Borland', b'Microsoft', b'MSLIB']:
    idxs = [m.start() for m in re.finditer(re.escape(pat), d)]
    if idxs:
        w(f"  {pat.decode():12s} x{len(idxs)}  first={[hex(i) for i in idxs[:4]]}")
        for i in idxs[:3]:
            w("      " + repr(d[max(0, i-24):i+56]))

open(outp, 'w').write('\n'.join(L))
print("wrote", outp, f"({len(L)} lines)")
