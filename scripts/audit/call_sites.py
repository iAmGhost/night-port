#!/usr/bin/env python3
"""Show what the shipped code puts in the register before each CALL to a target function.

The music cluster turned on this question: fn_010b1d (select + start a CD-DA track) takes its track
number in EAX, and the port hard-codes a few of those arguments. Which value does the original actually
compute at each call site? This walks every caller recorded in functions.json, disassembles the body from
NIGHT.EXE, and prints the instructions right before each CALL to the target.

  uv run --with capstone scripts/audit/cd_call_sites.py --target 0x10b1d [--before 12] [--all-calls]
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import disasm  # noqa: E402

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
except ImportError:
    sys.exit("capstone missing: run as `uv run --with capstone scripts/audit/cd_call_sites.py ...`")


def toint(x):
    if x is None:
        return None
    if isinstance(x, int):
        return x
    try:
        return int(str(x).strip(), 16)
    except ValueError:
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", required=True, help="guest address of the callee, e.g. 0x10b1d")
    ap.add_argument("--before", type=int, default=12)
    ap.add_argument("--all-calls", action="store_true", help="show every CALL in the body, not just the target")
    args = ap.parse_args()
    target = toint(args.target)

    fns = json.load(open(os.path.join(HERE, "functions.json")))
    callers = []
    for f in fns:
        for c in (f.get("callees") or []):
            if isinstance(c, dict):
                c = c.get("addr") or c.get("entry") or c.get("name")
            if toint(c) == target:
                callers.append(f)
    print("callers of %s: %d" % (hex(target), len(callers)))

    image = disasm.load()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    for f in sorted(callers, key=lambda x: toint(x["entry"])):
        s, e = toint(f["body_start"]), toint(f["body_end"])
        code = image[s - disasm.CODE_BASE:e - disasm.CODE_BASE]
        ins = list(md.disasm(code, s))
        hits = [i for i, x in enumerate(ins)
                if x.mnemonic.startswith("call") and
                (args.all_calls or hex(target) in x.op_str or str(target) in x.op_str)]
        print("\n=== %s  %s..%s  (%d insns, %d call sites) ===" % (f["name"], hex(s), hex(e), len(ins), len(hits)))
        for i in hits:
            print("  --> call at %s: %s %s" % (hex(ins[i].address), ins[i].mnemonic, ins[i].op_str))
            for x in ins[max(0, i - args.before):i]:
                print("      %s  %-8s %s" % (hex(x.address), x.mnemonic, x.op_str))


if __name__ == "__main__":
    main()
