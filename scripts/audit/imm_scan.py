#!/usr/bin/env python3
"""Scan the shipped code for instructions that name a constant (default 0xa0000, the video plane) or
call a given target, and label each hit with the function it sits in.

Why: a screen-oracle question ("which buffer is the presented one?") is answered by the sites that name
0xa0000 and by the call sites of the copy primitives (0x404c6 cpy_dw, 0x40491 blt_rows, ...).  A plain
grep over a capped disassembly listing misses them because the listing stops after its instruction cap.

  python3 scripts/audit/imm_scan.py --imm 0xa0000
  python3 scripts/audit/imm_scan.py --call 0x404c6
"""
import argparse
import bisect
import os
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "..", "..", "NIGHT", "NIGHT.EXE")
CODE_BASE = 0x2600                      # docs/12-functions/disasm.py: flat = file offset + 0x2600
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
    ap.add_argument("--imm", default="", help="constant to look for as an immediate operand (hex ok)")
    ap.add_argument("--call", default="", help="call target to look for (hex ok)")
    ap.add_argument("--start", type=lambda x: int(x, 0), default=0x10000)
    ap.add_argument("--end", type=lambda x: int(x, 0), default=0x53000)
    a = ap.parse_args()
    want_imm = int(a.imm, 0) if a.imm else None
    want_call = int(a.call, 0) if a.call else None
    if want_imm is None and want_call is None:
        sys.exit("give --imm and/or --call")

    image = open(EXE, "rb").read()
    code = image[a.start - CODE_BASE: a.end - CODE_BASE]
    funcs = load_funcs()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    hits = {}
    n = 0
    # capstone's generator stops at the first undecodable byte, which is why a straight dump of the whole
    # code range ends at 0x18dcc (10 560 instructions).  Resume past the gap instead of quitting.
    addr = a.start
    while addr < a.end:
        last = addr
        for ins in md.disasm(image[addr - CODE_BASE: a.end - CODE_BASE], addr):
            n += 1
            last = ins.address + ins.size
            imm_hit = want_imm is not None and any(o.imm == want_imm for o in ins.operands)
            call_hit = want_call is not None and ins.mnemonic == "call" and ins.operands \
                and ins.operands[0].imm == want_call
            if not (imm_hit or call_hit):
                continue
            i = bisect.bisect_right(funcs, ins.address) - 1
            owner = f"{funcs[i]:#x}" if i >= 0 else "?"
            hits.setdefault(owner, []).append((ins.address, ins.mnemonic, ins.op_str))
        if last <= addr:
            last = addr + 1
        addr = last
    print(f"{n} instructions scanned in {a.start:#x}..{a.end:#x}; "
          f"imm={want_imm and hex(want_imm)} call={want_call and hex(want_call)}")
    for owner in sorted(hits, key=lambda x: int(x, 16)):
        rows = hits[owner]
        print(f"\nfunction body containing {owner}: {len(rows)} hit(s)")
        for addr, mn, ops in rows[:10]:
            print(f"  {addr:#07x}  {mn:<8} {ops}")


if __name__ == "__main__":
    main()
