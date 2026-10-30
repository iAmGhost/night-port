#!/usr/bin/env python3
"""branch_xrefs.py -- every branch edge in NIGHT.EXE, including the ones a CALL census cannot see.

docs/180 had to be self-corrected: `call_sites.py` walks CALL edges, and contiguity of two function bodies says
nothing about control flow.  This tool walks the whole per-function disassembly and records EVERY terminator edge
(CALL, JMP, and conditional jumps) with its target, which is the set a "who reaches this function?" question needs:
a tail jump or an inter-half jump is an entry too.

Usage:
  branch_xrefs.py --target 0x4589e [--target 0x4933a ...]
  branch_xrefs.py --json reports/branch_xrefs.json      (dump the full edge set)
"""
import argparse, json, os, re, sys
from collections import defaultdict

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
except ImportError:
    sys.exit("needs capstone:  uv run --with capstone scripts/audit/branch_xrefs.py ...")

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
EXE = os.path.join(ROOT, "NIGHT", "NIGHT.EXE")
CODE_BASE = 0x2600
FNS = os.path.join(HERE, "functions.json")


def functions():
    d = json.load(open(FNS))
    return d if isinstance(d, list) else d.get("functions", [])


def edges():
    image = open(EXE, "rb").read()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = False
    out = defaultdict(list)
    n_body = 0
    for f in functions():
        try:
            start = int(str(f.get("body_start") or f.get("entry")), 16)
            end = int(str(f.get("body_end") or f.get("end")), 16)
        except ValueError:
            continue
        off = start - CODE_BASE
        if off < 0 or off >= len(image):
            continue
        # body_start and body_end are BOTH flat addresses: the slice length is (end - start), not
        # (end - file offset).  Mixing the two made every body run far past its end and produced a
        # listing of garbage with no recognisable branches -- caught because the first run reported
        # "distinct branch targets: 0" across 530 bodies, which is impossible for this binary.
        stop = min(end - CODE_BASE, len(image))
        if stop <= off:
            continue
        n_body += 1
        for ins in md.disasm(image[off:stop], start):
            m = ins.mnemonic.upper()
            if not (m == "CALL" or m == "JMP" or m.startswith("J")):
                continue
            mm = re.search(r"0x([0-9a-fA-F]+)", ins.op_str)
            if not mm:
                continue
            tgt = int(mm.group(1), 16)
            if m in ("CALL", "JMP") or m.startswith("J"):
                out[tgt].append((ins.address, f.get("name"), m))
    return out, n_body


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", action="append", default=[])
    ap.add_argument("--json")
    a = ap.parse_args()
    eg, n = edges()
    print("bodies disassembled: %d, distinct branch targets: %d" % (n, len(eg)))
    for t in a.target:
        tgt = int(t, 16)
        lst = eg.get(tgt, [])
        print("\nincoming branch edges to %s: %d" % (hex(tgt), len(lst))
              + ("   [last instruction of a body decides whether a boundary is real -- see docs/180 §2]" if not lst else ""))
        for src, fn, m in lst[:12]:
            print("   %-8s from %-18s at %s" % (m, fn, hex(src)))
    if a.json:
        p = os.path.join(HERE, a.json) if not os.path.isabs(a.json) else a.json
        os.makedirs(os.path.dirname(p), exist_ok=True)
        json.dump({hex(k): v for k, v in eg.items()}, open(p, "w"), indent=1)
        print("wrote %s" % p)


if __name__ == "__main__":
    main()
