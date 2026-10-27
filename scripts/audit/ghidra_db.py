#!/usr/bin/env python3
"""Load the Ghidra export of NIGHT.EXE plus the shipped image bytes.

NightExport.java (ghidra_scripts/) writes symbols.json / functions.json /
defined_data.json / mem_access.json next to this file. The shipped EXE gives the
image-initialised DS bytes: the LE image loads at linear DS-0x7600, so a DS address
maps to file offset addr-0x7600 (docs/99 measured that for DAT_00052a1c).
"""
import collections
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
DS_LO, DS_HI = 0x50000, 0x66000
IMAGE_FILE_VS_DS = 0x7600          # file offset = ds_addr - 0x7600
EXE = os.path.join(HERE, "..", "..", "NIGHT", "NIGHT.EXE")


def _load(name):
    with open(os.path.join(HERE, name)) as f:
        return json.load(f)


def _mentions(operand_text, addr_hex):
    """Does an operand text refer to the same DS address (Ghidra prints 0x00053c7c)?"""
    if not operand_text:
        return False
    want = addr_hex.lower().replace("0x", "")
    for tok in operand_text.replace("[", " ").replace("]", " ").split():
        if tok.startswith("0x") and tok[2:].lstrip("0") == want.lstrip("0"):
            return True
    return False


WRITE_MNEMS = {"MOV", "MOVZX", "MOVSX", "MOVABS", "SUB", "ADD", "AND", "OR", "XOR",
              "SHL", "SHR", "SAR", "NOT", "NEG", "MUL", "IMUL", "DIV", "IDIV"}
ALWAYS_READ = {"CMP", "TEST", "PUSH", "PUSHAD"}


class Db:
    def __init__(self):
        self.symbols = _load("symbols.json")
        self.functions = _load("functions.json")
        self.defined_data = _load("defined_data.json")
        self.mem_access = _load("mem_access.json")
        with open(EXE, "rb") as f:
            self.image = f.read()

    # ---- functions -------------------------------------------------------
    def fn_by_entry(self):
        return {int(f["entry"], 16): f for f in self.functions}

    def callees(self):
        return {int(f["entry"], 16): [int(c, 16) for c in f["callees"]] for f in self.functions}

    def reachable(self, roots):
        """BFS over direct call edges from the given entry addresses."""
        g = self.callees()
        seen, stack = set(), list(roots)
        while stack:
            a = stack.pop()
            if a in seen:
                continue
            seen.add(a)
            stack.extend(g.get(a, []))
        return seen

    # ---- DS defined data -------------------------------------------------
    def ds_data(self):
        return [d for d in self.defined_data if DS_LO <= int(d["addr"], 16) <= DS_HI]

    # ---- access directions ----------------------------------------------
    # Ghidra tags many DS references simply "DATA"; the direction then has to come from the
    # instruction itself (mnemonic + which operand holds the DS address).
    @staticmethod
    def dirs(m):
        """[(target_addr, direction)] for one instruction record; direction in read|write|rw."""
        mnem, ops = m["mnem"], m["ops"]
        out = []
        for i, r in enumerate(m["refs"]):
            a = int(r["to"], 16)
            t = r["rtype"]
            operand = next((o for o in ops if _mentions(o, r["to"])), "")
            frame = any("EBP" in o or "ESP" in o for o in ops)
            if "EBP" in operand or "ESP" in operand or (frame and not operand):
                # Ghidra's flat LE model renders stack frames as DS:[EBP+disp] and resolves them to
                # a DS address; those are locals, not DS globals, and are not global state.
                out.append((a, "stack"))
                continue
            if "READ_WRITE" in t:
                d = "rw"
            elif "READ" in t:
                d = "read"
            elif "WRITE" in t:
                d = "write"
            else:
                txt = r.get("to") and ops[0] if ops else ""
                first = ops[0] if ops else ""
                if mnem in ALWAYS_READ:
                    d = "read"
                elif mnem in ("INC", "DEC", "NEG", "NOT", "XCHG"):
                    d = "rw"
                elif mnem in WRITE_MNEMS and _mentions(first, r["to"]):
                    d = "write"
                else:
                    d = "read"
            out.append((a, d))
        return out

    def ds_dir_counts(self):
        out = {}
        for m in self.mem_access:
            for a, d in self.dirs(m):
                if d == "stack" or not (DS_LO <= a <= DS_HI):
                    continue
                slot = out.setdefault(a, {"read": 0, "write": 0, "rw": 0})
                slot[d] += 1
        return out

    def stack_only_cells(self):
        """DS-looking addresses whose every access is frame-relative (EBP/ESP) -> locals."""
        seen = {}
        for m in self.mem_access:
            for a, d in self.dirs(m):
                if DS_LO <= a <= DS_HI:
                    seen.setdefault(a, set()).add(d)
        return {a for a, ds in seen.items() if ds == {"stack"}}

    def ds_write_sites(self):
        out = {}
        for m in self.mem_access:
            for a, d in self.dirs(m):
                if d in ("write", "rw") and DS_LO <= a <= DS_HI:
                    out.setdefault(a, []).append((m["addr"], m["fn"], m["mnem"], m["ops"]))
        return out

    def ds_read_sites(self):
        out = {}
        for m in self.mem_access:
            for a, d in self.dirs(m):
                if d in ("read", "rw") and DS_LO <= a <= DS_HI:
                    out.setdefault(a, []).append((m["addr"], m["fn"], m["mnem"], m["ops"]))
        return out

    # ---- image bytes -----------------------------------------------------
    def in_image(self, ds_addr):
        off = ds_addr - IMAGE_FILE_VS_DS
        return 0 <= off < len(self.image)

    def image_at(self, ds_addr, size):
        off = ds_addr - IMAGE_FILE_VS_DS
        if off < 0 or off + size > len(self.image):
            return None
        return self.image[off:off + size]

    def image_u32(self, ds_addr):
        b = self.image_at(ds_addr, 4)
        if b is None:
            return None
        return int.from_bytes(b, "little")


if __name__ == "__main__":
    db = Db()
    dd = db.ds_data()
    xc = db.ds_xref_counts()
    print(f"functions={len(db.functions)} ds_defined_data={len(dd)} "
          f"ds_cells_with_xrefs={len(xc)} image={len(db.image):#x}")
