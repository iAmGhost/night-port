#!/usr/bin/env python3
"""Make every typed flat-model access alignment-safe without changing semantics.

The port reproduces the shipped entity/record layout verbatim, and that layout is packed and deliberately
unaligned (dword stores at +0x01, +0x05, +0x17, ... -- docs/125 S1 reads them out of the shipped entity
constructor). Writing those as C typed dereferences (`*(uint32_t *)(e + 0x17)`) is undefined behaviour on a
host ABI that requires alignment, and UBSan says so: 771 distinct sites / 878 reports in one drive
(docs/123, docs/126).

Rewriting each site into a memcpy accessor would have to restructure assignment, `+=`, `++` and address-of
forms. Instead we swap only the *cast*: a packed struct whose single member has the original type is a
valid, alignment-safe lvalue view of the bytes on gcc/clang, so

    *(uint32_t *)(e + 0x17)      ->   ((struct ua32 *)(e + 0x17))->v

which preserves every expression form (store, compound assignment, `&(...)`) and keeps the exact byte width
the shipped instruction used -- width is what the flat model depends on.

Comment and string bodies are skipped: the transcription quotes the shipped code inside comments, and those
must not be rewritten (an earlier version invented a struct named after a `u8_t` cast that only existed in
a comment).

Usage:  python3 scripts/port/unaligned_casts.py [--write] file.c [file.c ...]
"""
import re
import sys
from collections import Counter

# cast type -> (struct name, member type); width = the original type's width
SCALARS = {
    "uint32_t": ("ua32", "uint32_t"),
    "int32_t": ("sa32", "int32_t"),
    "uint16_t": ("ua16", "uint16_t"),
    "int16_t": ("sa16", "int16_t"),
    "uint64_t": ("ua64", "uint64_t"),
    "int64_t": ("sa64", "int64_t"),
    "u32": ("ua32", "uint32_t"),
    "s32": ("sa32", "int32_t"),
    "u16": ("ua16", "uint16_t"),
    "s16": ("sa16", "int16_t"),
    "float": ("uafl", "float"),
    "double": ("uadbl", "double"),
}
# 8-bit scalar accesses are always aligned; pointer-to-8-bit is not (it is an 8-byte access).
SKIP_SCALARS = {"uint8_t", "int8_t", "u8", "s8", "char", "unsigned char", "signed char"}

HEAD_RE = re.compile(r"\*\(\s*((?:const\s+|unsigned\s+|signed\s+)*[A-Za-z_][A-Za-z_0-9_ ]*?)\s*(\*+)\s*\)")
CAST_RE = re.compile(r"\(\s*(?:const\s+|unsigned\s+|signed\s+)*[A-Za-z_][A-Za-z_0-9_ ]*\**\s*\)")
PRIM_RE = re.compile(r"[A-Za-z_][A-Za-z_0-9_]*")
SKIP_RE = re.compile(r"/\*.*?\*/|//[^\n]*|\"(?:\\.|[^\\\"])*\"|'(?:\\.|[^\\'])'", re.S)


def match_group(text, i, open_ch, close_ch):
    """i points at open_ch -- return index just past the matching close_ch."""
    depth = 0
    while i < len(text):
        c = text[i]
        if c == '"':
            i += 1
            while i < len(text) and text[i] != '"':
                if text[i] == "\\":
                    i += 1
                i += 1
        elif c == "'":
            i += 1
            while i < len(text) and text[i] != "'":
                if text[i] == "\\":
                    i += 1
                i += 1
        elif c == open_ch:
            depth += 1
        elif c == close_ch:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return -1


def read_addr(text, k):
    """Read the address operand of `*(T *)<here>`; return (index after it, operand text)."""
    parts = []
    while True:
        while k < len(text) and text[k].isspace():
            k += 1
        cm = CAST_RE.match(text, k)
        if not cm:
            break
        parts.append(cm.group(0))
        k = cm.end()
    if k < len(text) and text[k] == "(":
        end = match_group(text, k, "(", ")")
        if end < 0:
            return -1, ""
        parts.append(text[k:end])
        k = end
    else:
        pm = PRIM_RE.match(text, k)
        if not pm:
            return -1, ""
        parts.append(pm.group(0))
        k = pm.end()
    while k < len(text) and text[k] == "[":          # subscripts belong to the operand
        end = match_group(text, k, "[", "]")
        if end < 0:
            return -1, ""
        parts.append(text[k:end])
        k = end
    return k, "".join(parts)


def resolve(typ, stars):
    """(struct_name, member_decl) for a cast, or None to leave the site alone.

    `stars` counts the `*` in the cast: `*(T **)x` reads a `T *`, not a `T`."""
    t = " ".join(typ.split())
    for q in ("const ", "unsigned ", "signed "):
        if t.startswith(q.strip()) and len(t) > len(q.strip()) and not t[len(q.strip())].isalnum():
            t = t[len(q.strip()):].lstrip()
    t = t + " *" * (stars - 1)
    if stars == 1:
        if t in SKIP_SCALARS:
            return None
        return SCALARS.get(t)
    if t.endswith("*"):
        base = " ".join(t.rstrip("*").split())
        if base in SKIP_SCALARS or base == "void":
            # pointer-to-8-bit still needs a view (8-byte access); void* casts are flat-model arithmetic
            if base == "void":
                return None
            name = "ua_" + re.sub(r"[^a-z0-9]", "", base.lower()) + "p"
            return (name, t)
        name = "ua_" + re.sub(r"[^a-z0-9]", "", base.lower()) + "p"
        return (name, t)
    return None


def transform(text, spans=None):
    if spans is None:
        spans = [(m.start(), m.end()) for m in SKIP_RE.finditer(text)]

    def skipped(pos):
        lo, hi = 0, len(spans)
        while lo < hi:
            mid = (lo + hi) // 2
            a, b = spans[mid]
            if pos < a:
                hi = mid
            elif pos >= b:
                lo = mid + 1
            else:
                return True
        return False

    structs, out, i, hits = Counter(), [], 0, 0
    while True:
        m = HEAD_RE.search(text, i)
        if not m:
            out.append(text[i:])
            break
        if skipped(m.start()):
            out.append(text[i:m.end()])
            i = m.end()
            continue
        j, inner = read_addr(text, m.end())
        r = resolve(m.group(1), len(m.group(2))) if j >= 0 and inner else None
        if r is None:
            # 8-bit / void* casts need no view, but their address operand may still contain dword reads,
            # so advance only past the cast head and let the nested sites be transformed (docs/126)
            out.append(text[i:m.end()])
            i = m.end()
            continue
        name, member = r
        structs[name] = member
        # the address operand may itself contain flat-model reads -- transform those too, or a nested
        # `*(const uint32_t *)(...)` inside an address stays misaligned
        inner, sub, _n = transform(inner)
        for k, v in sub.items():
            structs[k] = v
        out.append(text[i:m.start()])
        out.append("((struct %s *)(%s))->v" % (name, inner))
        hits += 1
        i = j
    return "".join(out), structs, hits


BLOCK = """
/* ---- unaligned flat-model views (docs/126) ----------------------------------
 * The shipped record layout is packed and deliberately unaligned (dword stores at +0x01/+0x05/+0x17 ...
 * read out of the entity constructor fn_010efc, docs/125 S1), so every typed access into it is a misaligned
 * access in C terms.  A packed struct view is alignment-safe on gcc/clang and keeps the exact store width
 * the shipped instruction used; scripts/port/unaligned_casts.py produces the rewrite.
 * The packed attribute is what makes the view safe -- without it the ABI (and UBSan) still require the
 * member's natural alignment, which is exactly what the shipped layout violates. */
#if defined(__GNUC__)
#  define UA_ATTR __attribute__((packed, aligned(1)))
#else
#  define UA_ATTR
#endif
"""


def inject(text, structs):
    lines = "\n".join("struct UA_ATTR %s { %s v; };" % (n, m) for n, m in sorted(structs.items()))
    block = BLOCK + lines + "\n"
    m = re.search(r"^(#include[^\n]*\n)+", text, re.M)
    pos = m.end() if m else 0
    return text[:pos] + block + text[pos:]


def main():
    write = "--write" in sys.argv
    for path in [a for a in sys.argv[1:] if not a.startswith("--")]:
        src = open(path, encoding="utf-8").read()
        new, structs, hits = transform(src)
        print("%s: %d sites rewritten, %d view types %s" % (path, hits, len(structs), sorted(structs)))
        if write and hits:
            open(path, "w", encoding="utf-8").write(inject(new, structs))


if __name__ == "__main__":
    main()
