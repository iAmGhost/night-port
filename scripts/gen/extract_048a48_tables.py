import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import struct
RVA_BASE = 0x2600  # file offset = RVA - 0x2600
data = open(NIGHT_EXE,'rb').read()
def at(rva, n): return data[rva-RVA_BASE : rva-RVA_BASE+n]

def fx128(rva):
    b = at(rva, 10)
    lo, hi = struct.unpack_from('<II', b, 0)
    exp = b[8] | (b[9]<<8)
    return (lo, hi, exp)

print("=== 0x48982 : 9 x 10-byte fx128 (048f9e path, arr=0x48982+idx*10) ===")
t1 = []
for i in range(9):
    lo,hi,exp = fx128(0x48982 + i*10)
    t1.append((lo,hi,exp))
    print(f"  [0x48982+{i*10:#05x}] lo={lo:08x} hi={hi:08x} exp={exp:04x}")

print("=== 0x489dc : 9 x 10-byte fx128 (f4c path, arr=0x489dc+idx*10) ===")
t2 = []
for i in range(9):
    lo,hi,exp = fx128(0x489dc + i*10)
    t2.append((lo,hi,exp))
    print(f"  [0x489dc+{i*10:#05x}] lo={lo:08x} hi={hi:08x} exp={exp:04x}")

print("=== 0x48a36 : 9 x uint16 (scale table, idx=min(8,-d)) ===")
t3 = struct.unpack_from('<9H', at(0x48a36, 18), 0)
print("  ", [f"{v:04x}" for v in t3])

# layout sanity: 0x48982+90 == 0x489dc ; 0x489dc+90 == 0x48a36 ; 0x48a36+18 == 0x48a48
print("\n=== layout sanity ===")
print("  0x48982+90 = 0x%x  (expect 0x489dc)" % (0x48982+90))
print("  0x489dc+90 = 0x%x  (expect 0x48a36)" % (0x489dc+90))
print("  0x48a36+18 = 0x%x  (expect 0x48a48)" % (0x48a36+18))

# C emit
def emit(name, t):
    lines = [f"static const fx128 {name}[9] = {{"]
    for (lo,hi,exp) in t:
        lines.append(f"    {{0x{lo:08x}u, 0x{hi:08x}u, 0x{exp:04x}}},")
    lines.append("};")
    return "\n".join(lines)
print("\n=== C EMIT ===")
print(emit("g_coeff_48982", t1))
print()
print(emit("g_coeff_489dc", t2))
print("static const uint16_t g_scale_48a36[9] = { " +
      ", ".join(f"0x{v:04x}" for v in t3) + " };")
