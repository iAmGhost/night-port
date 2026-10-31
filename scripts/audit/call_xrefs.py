#!/usr/bin/env python3
"""Direct-call xref census for NIGHT.EXE (docs/271).

Why this exists: a `call` in this binary is E8 with a **relative** 32-bit displacement, so searching the
file for the target's absolute address finds nothing (that mistake produced a false "fn_0003feb8 has zero
call sites" verdict in docs/271, superseding the ad-hoc census).  Reachability verdicts -- the three-part
census docs/179-181 requires (call edges, body contiguity + boundary instruction, event table) -- must be
built from resolved E8 displacements, not byte patterns.

Usage:  python3 scripts/audit/call_xrefs.py 0x3feb8 0x3272f ...
        --inside 0x3272f+7014   restrict the sites to one function's body
"""
import bisect, sys
from collections import Counter, defaultdict

CODE_BASE = 0x2600
CODE_LO, CODE_HI = 0x10000, 0x4a000

def load(path="NIGHT/NIGHT.EXE"):
    return open(path, "rb").read()

def functions(path="docs/12-functions/ghidra_function_addrs.txt"):
    return sorted(int(l.strip().replace("0x", ""), 16) for l in open(path) if l.strip())

def build(blob, fns):
    callers = defaultdict(list)
    for o in range(CODE_LO - CODE_BASE, CODE_HI - CODE_BASE - 5):
        if blob[o] != 0xE8:
            continue
        rel = int.from_bytes(blob[o + 1:o + 5], "little", signed=True)
        site = o + CODE_BASE
        callers[site + 5 + rel].append(site)
    return callers

def owner(fns, addr):
    i = bisect.bisect_right(fns, addr) - 1
    return fns[i] if i >= 0 else 0

def main(argv):
    targets, inside = [], None
    i = 0
    while i < len(argv):
        if argv[i] == "--inside":
            lo, hi = argv[i + 1].split("+"); inside = (int(lo, 0), int(lo, 0) + int(hi, 0)); i += 2
        else:
            targets.append(int(argv[i], 0)); i += 1
    blob, fns = load(), functions()
    callers = build(blob, fns)
    if not targets:
        print("usage: call_xrefs.py 0xADDR [...] [--inside 0xLO+SIZE]"); return
    print(f"{sum(len(v) for v in callers.values())} resolved direct calls in {CODE_LO:#06x}..{CODE_HI:#06x}")
    for t in targets:
        sites = callers.get(t, [])
        if inside:
            sites = [s for s in sites if inside[0] <= s < inside[1]]
        own = Counter(f"fn_{owner(fns, s):#07x}" for s in sites)
        print(f"call {t:#07x}: {len(sites)} direct call site(s) from " +
              ", ".join(f"{k} x{v}" for k, v in sorted(own.items())))
        print("   sites: " + " ".join(f"{s:#07x}" for s in sites[:40]))

if __name__ == "__main__":
    main(sys.argv[1:])
