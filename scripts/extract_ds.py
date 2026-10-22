#!/usr/bin/env python3
"""Extract initialized data-segment constants from NIGHT.EXE as a C initializer.

NIGHT.EXE is a flat DOS32A (Watcom) image whose segments are NOT contiguous in the file:

    code / rodata :  file offset = VA - 0x2600     (what scratch/fdis.py uses)
    data segment  :  file offset = VA - 0x7600     (verified: the dword table at DS 0x51c24
                                                    is byte-identical to Ghidra's image bytes)

Usage: uv run --with nothing python3 scripts/extract_ds.py <va-hex> <len> [name]
"""
import sys

EXE = "NIGHT/NIGHT.EXE"
DS_DELTA = 0x7600


def main() -> int:
    va = int(sys.argv[1], 16)
    n = int(sys.argv[2], 16) if len(sys.argv) > 2 and len(sys.argv[2]) > 4 else int(sys.argv[2])
    name = sys.argv[3] if len(sys.argv) > 3 else "blk_%x" % va
    data = open(EXE, "rb").read()
    b = data[va - DS_DELTA: va - DS_DELTA + n]
    if len(b) != n:
        print("short read: %d of %d" % (len(b), n), file=sys.stderr)
        return 1
    rows = []
    for i in range(0, n, 12):
        rows.append("    " + ", ".join("0x%02x" % x for x in b[i:i + 12]) + ",")
    print("uint8_t %s[0x%x] = {   /* DS 0x%05x .. 0x%05x, NIGHT.EXE file 0x%x */"
          % (name, n, va, va + n, va - DS_DELTA))
    print("\n".join(rows))
    print("};")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
