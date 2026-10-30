#!/usr/bin/env python3
"""field_writers.py -- every shipped instruction that WRITES a record field at a given displacement.

docs/187 could not name the producer of the queue value 54 because the pattern census
(`disasm.py --search 895036` etc.) only matches the plain `mov [reg+disp], reg` encodings.  The shipped code also
uses the indexed form seen at 0x3202c-0x3202f (`[reg + k*4 + 0x36]`), whose bytes are `89 94 88 36 00 00 00` and
which no fixed 3-byte pattern finds.  This walks every modelled function body with capstone and reports writes to a
displacement, whatever the addressing form.

  field_writers.py --disp 0x32 --disp 0x36 [--disp 0x31]
"""
import argparse
import os
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
EXE = os.path.join(HERE, "..", "..", "NIGHT", "NIGHT.EXE")
CODE_BASE = 0x2600
READS = {"CMP", "TEST", "PUSH", "XCHG"}      # not field writes in the sense we care about


def functions():
    import json
    d = json.load(open(os.path.join(HERE, "functions.json")))
    rows = d if isinstance(d, list) else d.get("functions", [])
    out = []
    for r in rows:
        try:
            s = int(str(r.get("body_start") or r.get("entry")), 16)
            e = int(str(r.get("body_end") or r.get("end")), 16)
        except (ValueError, TypeError):
            continue
        n = r.get("name") or r.get("guest_symbol") or "?"
        if e > s:
            out.append((n, s, e))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--disp", action="append", required=True)
    a = ap.parse_args()
    disps = {int(x, 0) for x in a.disp}
    image = open(EXE, "rb").read()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    n_body = 0
    hits = {}
    for name, start, end in functions():
        off = start - CODE_BASE
        stop = min(end - CODE_BASE, len(image))
        if off < 0 or stop <= off:
            continue
        n_body += 1
        for ins in md.disasm(image[off:stop], start):
            if ins.mnemonic.upper() in READS or not ins.operands:
                continue
            for idx, op in enumerate(ins.operands):
                if op.type != X86_OP_MEM:
                    continue
                if op.mem.disp not in disps:
                    continue
                # docs/239: the memory operand must be the DESTINATION.  In Intel syntax that means it is
                # operand 0; `mov eax, [ecx+0x30]` has the field as operand 1 and is a READ, which the old
                # guard let through (it only rejected non-store mnemonics at operand 0), inflating every
                # --disp census this tool has ever produced.
                if idx != 0:
                    continue
                if idx == 0 and ins.mnemonic.upper() not in ("MOV", "ADD", "SUB", "AND", "OR", "XOR", "SHL",
                                                              "SHR", "SAL", "SAR", "INC", "DEC", "NEG", "NOT"):
                    continue
                form = ins.reg_str if hasattr(ins, "reg_str") else ""
                base = ins.reg_name(op.mem.base) if op.mem.base else "none"
                index = ins.reg_name(op.mem.index) if op.mem.index else "none"
                hits.setdefault(op.mem.disp, []).append(
                    (ins.address, name, ins.mnemonic.upper(), base, index, op.mem.scale, ins.op_str))
    print("bodies walked: %d" % n_body)
    for d in sorted(hits):
        rows = hits[d]
        print("\nwrites to displacement +0x%02x: %d instruction(s)" % (d, len(rows)))
        idx_forms = [r for r in rows if r[4] != "none"]
        print("  of which indexed (base+index*scale): %d" % len(idx_forms))
        for (addr, fn, mn, base, index, scale, opstr) in rows:
            print("   0x%05x  %-14s %-4s [%s + %s*%d + 0x%x]  %s" % (addr, fn, mn, base, index, scale, d, opstr[:44]))


if __name__ == "__main__":
    main()
