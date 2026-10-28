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
    # The port widens guest DS-relative heap offsets to uintptr_t where it holds a pointer (the docs/94
    # offset-vs-pointer family). Without these the entity globals -- the player entity at DAT_00053c84,
    # the latched entity at DAT_00053c80 -- were not in the symbol table at all, so neither the static
    # audit nor the state oracle could see the records they point at.
    "uintptr_t": 8, "intptr_t": 8, "size_t": 8,
}

DECL = re.compile(
    r"^(?:(?:extern|static|const|volatile)\s+)*"
    r"([A-Za-z_][A-Za-z0-9_]*(?:\s*\*)*)\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*"
    r"((?:\[[^\]]*\])*)\s*"
    r"(=[^;]*)?;")
DIM = re.compile(r"\[([^\]]*)\]")
NAME_ADDR = re.compile(r"_(?:dat|tab|ds|scr)?_?([0-9a-f]{5})(?:\b|_)")
# The port marks a DS-group global with the Ghidra name (`DAT_00053c84`) *or* with a bare address at the
# start of the trailing comment (`/* 0x53c80 ... */`). Both are accepted; a bare hex number anywhere else
# in the comment is not, because those are field offsets, not addresses.
COMMENT_ADDR = re.compile(r"DAT_000([0-9a-f]{5})")
COMMENT_ADDR_BARE = re.compile(r"/\*\s*0x([0-9a-f]{4,6})\b")
FNDEF = re.compile(r"^(?:static\s+|inline\s+)*[A-Za-z_][A-Za-z0-9_ \t\*]*?\b(fn_[0-9a-f]{4,6}|[a-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*\{", re.M)
FNCALL = re.compile(r"\b(fn_[0-9a-f]{4,6})\s*\(")


def _files():
    for name in ("f.h", "f.c"):
        p = os.path.join(SRC, name)
        if os.path.exists(p):
            yield name, open(p, encoding="utf-8", errors="replace").read()


def _statements(text):
    """Yield (start_line, start_offset, statement_text) for top-level statements (brace depth 0).

    Comment bodies are skipped rather than scanned: the port's doc comments contain semicolons and braces,
    and splitting on them produced statements that were comment fragments (`/* 0x53c7c (... POINTER;`),
    which is how several entity globals lost their address marker. The offsets are kept so a declaration's
    address marker can be read off its own physical line."""
    depth, i, line, seg_start, seg_line = 0, 0, 1, 0, 1
    while i < len(text):
        c = text[i]
        if text.startswith("/*", i):                                   # skip a comment body verbatim
            end = text.find("*/", i + 2)
            end = len(text) - 1 if end < 0 else end + 1
            line += text.count("\n", i, end)
            i = end
            continue
        if c == '"' or c == "'":                                       # skip a literal: a brace inside a
            q = c                                                       # string ('{' in a log format) used
            i += 1                                                      # to unbalance depth and swallow
            while i < len(text) and text[i] != q:                       # 90k+ characters into one
                if text[i] == "\\":                                   # "statement", which is how whole
                    i += 1                                              # ranges of declarations went unseen
                elif text[i] == "\n":
                    line += 1
                i += 1
            i += 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                # A function definition ends with `}`, not `;`; without closing the segment here the next
                # declaration is glued onto the whole body (90k+ characters) and DECL matches nothing --
                # that is how entire ranges of DS globals went unseen. But an *initializer* array also
                # closes at depth 0 (`uint32_t t[0x100] = { ... }`), so only treat the brace as a segment
                # end when what precedes it looks like a signature (a `)` before the opening `{`).
                seg = text[seg_start:i + 1]
                ob = seg.find("{")
                # the text before the first `{`, with comments removed, ends in `)` only for a definition
                # signature -- an array declaration ends in `=` (and its doc comment may contain `()`).
                pre = re.sub(r"/\*.*?\*/", " ", seg[:ob], flags=re.S).rstrip() if ob > 0 else ""
                if ob > 0 and pre.endswith(")"):
                    yield seg_line, seg_start, seg
                    seg_start, seg_line = i + 1, line + 1
        elif c == ";" and depth == 0:
            yield seg_line, seg_start, text[seg_start:i + 1]
            seg_start, seg_line = i + 1, line + text.count("\n", seg_start, i + 1)
        elif c == "\n":
            line += 1
        i += 1


def globals_():
    out = []
    for fname, text in _files():
        for lineno, off, stmt in _statements(text):
            one = " ".join(stmt.split())
            # a trailing comment from the *previous* declaration can end up glued to the front of this
            # statement's text, because statements are segmented at top-level `;`. Strip comments that
            # sit before the first `;` (declarators never contain one) -- this is what hid the entity
            # globals whose address marker is in their own trailing comment.
            semi = one.find(";")
            decl_text = re.sub(r"/\*.*?\*/", " ", one[:semi], flags=re.S).strip() + one[semi:]
            head = decl_text
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
                n = eval_dim(d.strip())
                elems = elems * n if n > 0 else -1
            tsz = TYPE_SIZE.get(base, 4 if ptr else 0)
            addr = None
            mm = NAME_ADDR.search(name)
            if mm:
                addr = int(mm.group(1), 16)
            if addr is None:
                # Two sources, in order: the text after the declarator inside the statement, and a comment
                # that starts on the SAME physical line as the statement's closing `;` (statements are
                # segmented at `;`, so `uint8_t *g_latched_ent = 0;  /* 0x53c80 ... */` puts its own marker
                # just past its own end). A comment on the following line is NOT used: that one documents
                # the next declaration. A bare hex number only counts if it is inside the DS group -- the
                # prose is full of code addresses (`0x3774c`) and field offsets (`+0x123`).
                i_self = stmt.find(name)
                tail = stmt[i_self:] if i_self >= 0 else ""
                end_off = off + len(stmt) - 1
                if re.match(r"[ \t]*/\*", text[end_off + 1:end_off + 40]):
                    cend = text.find("*/", end_off)
                    if cend > 0:
                        tail += " " + text[end_off:cend + 2]
                cm = COMMENT_ADDR.search(tail)
                if cm:
                    addr = int(cm.group(1), 16)
                else:
                    for cand in re.finditer(r"0x([0-9a-f]{4,6})\b", tail):
                        v = int(cand.group(1), 16)
                        if DS_LO <= v < DS_HI:
                            addr = v
                            break
            out.append({
                "name": name, "ds": addr, "ctype": ctype, "ptr": ptr,
                "base_type": base, "elems": elems, "type_size": tsz,
                "guest_size": (tsz * elems if tsz and elems > 0 else None),
                "init": (init or "").strip(), "file": fname, "line": lineno,
            })
    return out


DIM_EXPR = re.compile(r"^[0-9a-fA-FxX+*/\- \t()]+$")


DS_LO, DS_HI = 0x50000, 0x65000     # the DS group; markers outside it are prose addresses, not globals


def eval_dim(text):
    """Dimension of an array declarator. Port tables are sized with expressions
    (`uint32_t g_tab_52a1c[0x4ec/4]`), and a dimension that merely fails to parse must NOT collapse to
    1 element: the rig then compares one byte against the guest's four and reports a false difference
    (it did, for the kill-quota table). Returns -1 when the dimension is not a plain integer
    expression, so the caller can mark the cell's size unknown instead of guessing."""
    t = text.strip()
    if not t or not DIM_EXPR.match(t):
        return -1
    try:
        if re.fullmatch(r"0[0-9a-fA-Fx]+", t):
            return int(t, 0)
        v = eval(t, {"__builtins__": {}}, {})
    except Exception:
        return -1
    # `/` in Python is true division, so `[0x4ec/4]` comes back as 314.0
    if isinstance(v, float) and v.is_integer():
        v = int(v)
    return v if isinstance(v, int) and v > 0 else -1


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
