#!/usr/bin/env python3
"""Intra-function control-flow reachability over a shipped code range.

Why: "is this block reachable at all?" is the question docs/180 and docs/237 keep running into, and the honest
answer needs the branch structure of a single function -- a linear disassembly listing shows the blocks but not
which jumps land in them, and a whole-binary xref scan (branch_xrefs.py) only sees CALL edges.

Model: decode the range as instructions; edges = unconditional jmp, conditional jcc targets, and fall-through.
`ret` ends a path (a call site re-enters only at the function entry).  Call targets are NOT followed: this is
intra-procedural by design.

  python3 scripts/audit/cfg_reach.py --entry 0x1441e --end 0x15140 --target 0x15003 [--target 0x150f5]
  python3 scripts/audit/cfg_reach.py --entry 0x1441e --end 0x15140 --edges   (dump every edge)
"""
import argparse
import pathlib
import sys
from collections import defaultdict, deque

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from imm_scan import CODE_BASE, EXE  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402

image = open(EXE, "rb").read()
md = Cs(CS_ARCH_X86, CS_MODE_32)
md.detail = True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--entry", required=True)
    ap.add_argument("--end", required=True)
    ap.add_argument("--target", action="append", default=[])
    ap.add_argument("--edges", action="store_true")
    ap.add_argument("--preds", action="append", default=[], help="address to treat as a barrier (never entered)")
    a = ap.parse_args()

    entry, end = int(a.entry, 0), int(a.end, 0)
    barriers = {int(x, 0) for x in a.preds}
    start = entry

    ins = {}
    order = []
    addr = start
    while addr < end:
        for i in md.disasm(image[addr - CODE_BASE:end - CODE_BASE], addr):
            ins[i.address] = i
            order.append(i.address)
            addr = i.address + i.size
        if addr >= end:
            break
        if addr not in ins:            # capstone stopped: skip the undecodable byte and resume
            addr += 1
    if not order:
        sys.exit("nothing decoded")

    succ = defaultdict(list)
    preds = defaultdict(list)
    for addr2, i in ins.items():
        nxt = addr2 + i.size
        m = i.mnemonic
        if m == "ret" or m == "iret":
            continue
        if m.startswith("j"):
            if i.operands:
                t = i.operands[0].imm          # capstone reports jcc/jmp targets as absolute immediates
                if t in ins:
                    succ[addr2].append(t)
                    preds[t].append(addr2)
                else:
                    print(f"  note: {addr2:#x} {m} {i.op_str} jumps outside the range (indirect or external)")
            if m == "jmp":
                continue                                    # unconditional: no fall-through
        if nxt in ins:
            succ[addr2].append(nxt)
            preds[nxt].append(addr2)

    seen, dq = {entry}, deque([entry])
    while dq:
        cur = dq.popleft()
        for s in succ[cur]:
            if s in barriers or s in seen:
                continue
            seen.add(s)
            dq.append(s)

    print(f"{len(ins)} instructions decoded in {entry:#x}..{end:#x}; "
          f"{len(seen)} reachable from {entry:#x}")
    if a.edges:
        for a2 in order:
            for s in succ[a2]:
                print(f"  {a2:#x} -> {s:#x}  ({ins[a2].mnemonic} {ins[a2].op_str})")
    for t in a.target:
        tv = int(t, 0)
        r = "REACHABLE" if tv in seen else "NOT reachable"
        ps = ", ".join(f"{p:#x}" for p in sorted(preds.get(tv, [])))
        print(f"  target {tv:#x}: {r}   predecessors: {ps or '(none)'}")
        for p in sorted(preds.get(tv, [])):
            print(f"     via {p:#x}  {ins[p].mnemonic} {ins[p].op_str}  (reachable={p in seen})")


if __name__ == "__main__":
    main()
