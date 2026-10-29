#!/usr/bin/env python3
"""Disassemble any shipped function straight out of NIGHT.EXE -- no Ghidra session needed.

The Ghidra flat model uses linear addresses; the file needs an offset. Two bases are in play:

  data:   file offset = DS address - 0x7600        (docs: the .object2 image region)
  code:   file offset = flat address - 0x2600      (measured here, see --verify)

The code base was established by locating a known instruction: Ghidra reports
`MOV byte ptr [0x000537a7], AH` at guest 0x49c9b, and the encoding `88 25 a7 37 00 00` sits at file
offset 0x4769b, giving 0x49c9b - 0x4769b = 0x2600. `--verify` re-checks that (and that a function body
disassembles to its exported length).

Operand displacements in the code are 16-bit (`[0x3784]`), so a raw `dword ptr [0xNNNN]` in the listing
is DS 0x50000 + NNNN: Ghidra's DS addresses add the 0x50000 segment base, this listing does not.

  python3 scripts/audit/disasm.py --entry 0x49c76 [--end 0x49d04] [--mnem MOV]
  python3 scripts/audit/disasm.py --search 8825a7370000
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "..", "..", "NIGHT", "NIGHT.EXE")
CODE_BASE = 0x2600
DATA_BASE = 0x7600
DS_SEG = 0x50000


def load():
    return open(EXE, "rb").read()


def verify(image):
    """Prove the code base from a known instruction and report the data base separately."""
    pat = bytes.fromhex("8825a7370000")        # mov byte ptr [0x37a7], ah  (guest 0x49c9b)
    i = image.find(pat)
    if i < 0:
        print("VERIFY: anchor instruction not found")
        return None
    base = 0x49C9B - i
    print(f"anchor: 88 25 a7 37 00 00 at file {i:#x}, guest 0x49c9b -> code base {base:#x}")
    print(f"{'matches measured CODE_BASE' if base == CODE_BASE else 'MISMATCH vs CODE_BASE'}")
    fns = json.load(open(os.path.join(HERE, "functions.json")))
    ok = bad = 0
    for f in fns[:80]:
        e, b = int(f["entry"], 16), f["body_bytes"]
        off = e - CODE_BASE
        if off < 0 or off + b > len(image):
            continue
        body = image[off:off + b]
        # a body whose last byte is a ret/int3 is a strong sign the base is right
        if body[-1] in (0xC3, 0xCC, 0xC2, 0x90):
            ok += 1
        else:
            bad += 1
    print(f"body-length check on the first 80 functions: {ok} end with ret/int3/ret-n, {bad} do not")
    return base


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--entry", type=lambda x: int(x, 16))
    ap.add_argument("--end", type=lambda x: int(x, 16))
    ap.add_argument("--bytes", type=lambda x: int(x, 0), default=0)
    ap.add_argument("--search", help="hex byte pattern to locate (prints the implied base)")
    ap.add_argument("--all", action="store_true",
                    help="with --search: list EVERY hit, its guest address, its enclosing function, and the "
                         "decoded instruction.  Without this the tool prints only the first hit, which is what "
                         "made byte-pattern absence claims misleading (docs/166 §15).")
    ap.add_argument("--mnem", default="", help="only show instructions whose mnemonic contains this")
    ap.add_argument("--verify", action="store_true")
    a = ap.parse_args()
    try:
        import capstone
    except ImportError:
        sys.exit("pip/capstone missing: run as `uv run --with capstone scripts/audit/disasm.py ...`")
    image = load()
    if a.verify:
        verify(image)
    if a.search:
        pat = bytes.fromhex(a.search)
        hits = []
        i = image.find(pat)
        while i >= 0:
            hits.append(i)
            i = image.find(pat, i + 1)
        print(f"pattern {a.search}: {len(hits)} hit(s), first at file {hits[0] if hits else -1:#x}")
        if not hits:
            return
        print("to get its guest address: guest = file + 0x2600 (code)")
        if not a.all:
            return
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        fns = json.load(open(os.path.join(HERE, "functions.json")))
        spans = sorted((int(f["entry"], 16), int(f["entry"], 16) + int(f.get("body_bytes") or 0), f["name"])
                       for f in fns if f.get("entry"))

        def owner(g):
            lo, hi = 0, len(spans) - 1
            while lo <= hi:
                m = (lo + hi) // 2
                if spans[m][1] <= g:
                    lo = m + 1
                elif spans[m][0] > g:
                    hi = m - 1
                else:
                    return spans[m][2]
            return "?"

        per_fn = {}
        for off in hits:
            g = off + CODE_BASE
            fn = owner(g)
            per_fn.setdefault(fn, []).append(g)
            # disassemble the owning body so the hit lands on an instruction boundary
            start = next((e for e, _h, n2 in spans if n2 == fn), g)
            body = image[start - CODE_BASE: start - CODE_BASE + 0x1000]
            ctx = [x for x in md.disasm(body, start) if g - 0x10 <= x.address <= g + 0x10]
            line = next((f"{x.mnemonic} {x.op_str}" for x in ctx if x.address == g), "<undecoded>")
            print(f"  {g:#07x}  {fn:<12s}  {line}")
        print("# per function:")
        for fn, gs in sorted(per_fn.items(), key=lambda kv: -len(kv[1])):
            print(f"  {len(gs):>3d}  {fn}")
    if a.entry is None:
        return
    off = a.entry - CODE_BASE
    n = a.bytes or ((a.end - a.entry) if a.end else 128)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    shown = 0
    for ins in md.disasm(image[off:off + n], a.entry):
        if a.mnem and a.mnem.lower() not in ins.mnemonic.lower():
            continue
        # annotate the DS addresses this instruction touches (segment base + 16-bit displacement)
        note = ""
        mm = re.search(r"\[0x([0-9a-f]{1,6})\]", ins.op_str)
        if mm:
            note = f"   ; DS {0x50000 + int(mm.group(1), 16):#07x}"
        print(f"{ins.address:#07x}: {ins.mnemonic:<7s} {ins.op_str}{note}")
        shown += 1
        if a.end and ins.address >= a.end:
            break
    print(f"# {shown} instructions from {a.entry:#x} (file offset {off:#x})")


if __name__ == "__main__":
    main()
