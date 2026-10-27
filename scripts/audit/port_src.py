#!/usr/bin/env python3
"""Parse the port's own globals and call graph (port/src/f) for the static audit.

Every port global keeps the shipped address in its name (g_dat_53c7c, g_tab_52c3c)
or in its declaration comment (DAT_00053338), which is what makes a mechanical
port-side <-> guest-side join possible.
"""
import os
import re

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "port", "src", "f")

TYPE_SIZE = {
    "uint8_t": 1, "int8_t": 1, "char": 1, "unsigned char": 1,
    "uint16_t": 2, "int16_t": 2, "short": 2,
    "uint32_t": 4, "int32_t": 4, "int": 4, "unsigned": 4, "float": 4,
    "uint64_t": 8, "int64_t": 8, "double": 8,
}

DECL = re.compile(
    r"^(?:(?:extern|static|const|volatile)\s+)*"
    r"([A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*)\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"((?:\[[^\]]*\])*)\s*"
    r"(=[^;]*)?;")
DIM = re.compile(r"\[([^\]]*)\]")
NAME_ADDR = re.compile(r"_(?:dat|tab|ds|scr)?_?([0-9a-f]{5})(?:\b|_)")
COMMENT_ADDR = re.compile(r"DAT_000([0-9a-f]{5})")
FNDEF = re.compile(r"^(?:static\s+|inline\s+)*[A-Za-z_][A-Za-z0-9_ \t\*]*?\b(fn_[0-9a-f]{4,6}|[a-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*\{", re.M)
FNCALL = re.compile(r"\b(fn_[0-9a-f]{4,6})\s*\(")


def _files():
    for name in ("f.h", "f.c"):
        p = os.path.join(SRC, name)
        if os.path.exists(p):
            yield name, open(p, encoding="utf-8", errors="replace").read()


def _statements(text):
    """Yield (start_line, statement_text) for top-level statements (brace depth 0)."""
    depth, i, line, seg_start, seg_line = 0, 0, 1, 0, 1
    while i < len(text):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif c == ";" and depth == 0:
            yield seg_line, text[seg_start:i + 1]
            seg_start, seg_line = i + 1, line + text.count("\n", seg_start, i + 1)
        elif c == "\n":
            line += 1
        i += 1


def globals_():
    out = []
    for fname, text in _files():
        for lineno, stmt in _statements(text):
            one = " ".join(stmt.split())
            head = one
            while head.startswith("/*"):
                end = head.find("*/")
                if end < 0:
                    break
                head = head[end + 2:].lstrip()
            m = DECL.match(head)
            if not m:
                continue
            ctype, name, dims, init = m.group(1), m.group(2), m.group(3) or "", m.group(4)
            base = ctype.replace("*", " ").strip()
            ptr = "*" in ctype
            if base not in TYPE_SIZE and not base.startswith(("struct ", "enum ")):
                continue
            elems = 1
            for d in DIM.findall(dims):
                d = d.strip()
                try:
                    elems *= int(d, 0)
                except ValueError:
                    elems = -1
            tsz = TYPE_SIZE.get(base, 4 if ptr else 0)
            addr = None
            mm = NAME_ADDR.search(name)
            if mm:
                addr = int(mm.group(1), 16)
            if addr is None:
                # only a comment AFTER the declarator belongs to this declaration
                tail = one[one.find(name) + len(name):]
                cm = COMMENT_ADDR.search(tail)
                if cm:
                    addr = int(cm.group(1), 16)
            out.append({
                "name": name, "ds": addr, "ctype": ctype, "ptr": ptr,
                "base_type": base, "elems": elems, "type_size": tsz,
                "guest_size": (tsz * elems if tsz and elems > 0 else None),
                "init": (init or "").strip(), "file": fname, "line": lineno,
            })
    return out


def functions():
    defined = {}
    calls = {}
    for fname, text in _files():
        for m in FNDEF.finditer(text):
            name = m.group(1)
            body = _brace_block(text, m.end() - 1)
            defined.setdefault(name, fname)
            calls[name] = set(FNCALL.findall(body))
        # calls that appear outside any parsed definition still matter for "never called"
        for m in FNCALL.finditer(text):
            calls.setdefault("__toplevel__" + fname, set()).add(m.group(1))
    return defined, calls


def _brace_block(text, open_idx):
    depth, i = 0, open_idx
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[open_idx:i + 1]
        i += 1
    return text[open_idx:]


if __name__ == "__main__":
    gs = globals_()
    withaddr = [g for g in gs if g["ds"] is not None]
    print(f"port globals parsed={len(gs)} with DS address={len(withaddr)} "
          f"pointer-typed={sum(1 for g in gs if g['ptr'])}")
    defined, calls = functions()
    print(f"port functions defined={len(defined)} with parsed bodies={len(calls)}")
