#!/usr/bin/env python3
"""Find shipped tables that contain a given value set, and the instruction sites that index them.

A state-value question ("who writes 0x34 into record+0x17?") is answered by the *data* first: if a
value never appears in any table the shipped reads with a scaled index, then the store that produces
it is either an immediate or an arithmetic update, and the search space collapses.

  python3 scripts/audit/table_scan.py --values 0x33,0x34,0x35,0x36

Flat addressing (docs/12-functions): code literal X names DS address X + 0x50000; the file offset of
a DS address D is D - 0x7600 (validated against DAT_00052c3c[0] = 172 and DAT_00052ffc[0..9]).
"""
import argparse
import os
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "..", "..", "NIGHT", "NIGHT.EXE")
CODE_BASE = 0x2600
DS_DELTA = 0x50000
DATA_BASE = 0x7600
FUNCS = os.path.join(HERE, "..", "..", "docs", "12-functions", "ghidra_function_addrs.txt")


def load_funcs():
    addrs = []
    if os.path.exists(FUNCS):
        for ln in open(FUNCS):
            ln = ln.strip().replace("0x", "")
            if not ln:
                continue
            try:
                addrs.append(int(ln, 16))
            except ValueError:
                continue
    return sorted(set(addrs))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--values", default="0x33,0x34,0x35,0x36")
    ap.add_argument("--entries", type=int, default=256)
    args = ap.parse_args()
    want = {int(v, 0) for v in args.values.split(",")}

    blob = open(EXE, "rb").read()
    code = blob[CODE_BASE:]
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True

    tables = {}
    pos = 0
    n = len(code)
    while pos < n:
        got = False
        for ins in md.disasm(code[pos:], CODE_BASE + pos):
            got = True
            for op in ins.operands:
                if op.type == 3 and op.mem.scale == 4 and op.mem.disp:
                    ds = op.mem.disp + DS_DELTA
                    tables.setdefault(ds, []).append((ins.address, ins.mnemonic, ins.op_str))
            pos = ins.address + ins.size - CODE_BASE
        if not got:
            pos += 1

    hits = 0
    for ds, refs in sorted(tables.items()):
        if ds - DATA_BASE + 4 * args.entries > len(blob):
            continue
        vals = [int.from_bytes(blob[ds - DATA_BASE + 4 * k:ds - DATA_BASE + 4 * k + 4], "little")
                for k in range(args.entries)]
        match = [k for k, v in enumerate(vals) if v in want]
        if not match:
            continue
        hits += 1
        fns = sorted({r[0] for r in refs})
        print(f"DS {ds:#07x} (file {ds - DATA_BASE:#x}): {len(refs)} indexing sites, entries matching "
              f"{sorted(hex(v) for v in want)} at indices {[hex(k) for k in match[:12]]}")
        for a in fns[:6]:
            print(f"    site {a:#07x} " + "; ".join(f"{m} {o}" for ad, m, o in refs if ad == a)[:150])
    print(f"tables scanned: {len(tables)}, tables containing the values: {hits}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
