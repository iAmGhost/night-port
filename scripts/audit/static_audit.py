#!/usr/bin/env python3
"""Static equivalence audit: shipped NIGHT.EXE (Ghidra export) vs the port's own source.

Five checks, each a *presence* test rather than a behaviour test:

  C1 unmodelled      DS cells the guest reads/writes that the port never names
  C2 aliasing        DS intervals that overlap / one symbol sitting inside another,
                     on either side of the join (the docs/83 + docs/91 family)
  C3 image-init      image-initialised DS bytes vs the port's hand-written initializers
                     (the docs/55 + docs/99 family; file offset = DS - 0x7600)
  C4 repr-mismatch   DS cells the port declares as pointers/uintptr_t whose guest use
                     says "offset/small integer", and vice versa (docs/49/61/94/95 family)
  C5 dead-port-side  guest functions with no port counterpart; port functions that
                     nothing in the port calls (the docs/97 / docs/98 family)

Usage: python3 static_audit.py [--json OUT] [--md OUT]
"""
import argparse
import collections
import json
import os
import re

import ghidra_db
import port_src
from ghidra_db import _mentions

HERE = os.path.dirname(os.path.abspath(__file__))
HEAP_ALLOCATORS = {0x03de85, 0x03dfb5, 0x03de1a, 0x042ed0}   # docs/95 + heap family
POINTERISH = ("uintptr_t", "void *", "uint8_t *", "int *", "uint32_t *", "char *")
DS_CONST = __import__("re").compile(r"0x0*([5-6][0-9a-f]{4})\b")
STRIDE = __import__("re").compile(r"[A-Z]+\*(0x[0-9a-f]+|\d+)")
DISP = __import__("re").compile(r"\+\s*(0x[0-9a-f]+)")


def ds_immediates(db):
    """DS-range constants that appear as instruction operands: table bases and indexed accesses.

    Ghidra models this image as flat dword cells, so a table reached through a register base has NO
    symbol of its own -- the base only ever shows up as an immediate (e.g. the kill-quota table at
    0x52a1c, which is why a port driven by the decompiler alone zeroed it)."""
    out = collections.defaultdict(list)
    for m in db.mem_access:
        for op in m["ops"]:
            for mm in DS_CONST.finditer(op):
                out[int(mm.group(1), 16)].append((m["addr"], m["fn"], m["mnem"], op))
    return out


def port_written_names():
    """Port globals the port itself produces a value for (assignment, memcpy, &addr, fread).

    Keeps the image-init findings honest: a placeholder the port fills at load time is not a dropped
    initializer; a placeholder nothing ever writes is."""
    names = {g["name"] for g in port_src.globals_()}
    written = set()
    for _fname, text in port_src._files():
        for _lineno, _off, stmt in port_src._statements(text):
            one = " ".join(stmt.split())
            head = one.lstrip()
            while head.startswith("/*"):
                end = head.find("*/")
                if end < 0:
                    break
                head = head[end + 2:].lstrip()
            if port_src.DECL.match(head):
                continue          # a declaration + initializer is not a producer
            for n in names:
                if re.search(rf"\b{n}(\s*\[[^\]]*\])*\s*(=|\+=|-=|\|=|&=|\+\+|--)(?![=>])", one) \
                        or re.search(rf"&\s*{n}\b", one):
                    written.add(n)
    return written


def port_global_map():
    """DS address -> list of port declarations that claim it."""
    m = collections.defaultdict(list)
    for g in port_src.globals_():
        if g["ds"] is not None:
            m[g["ds"]].append(g)
    return m


def c1_unmodelled(db, pmap):
    out = []
    for addr, c in db.ds_dir_counts().items():
        if addr in pmap:
            continue
        out.append({"ds": addr, "reads": c["read"] + c["rw"], "writes": c["write"] + c["rw"],
                    "total": sum(c.values())})
    out.sort(key=lambda x: -x["total"])
    return out


def c2_aliasing(db, pmap):
    findings = []
    seen = set()
    # guest side: defined-data intervals that overlap each other
    dd = sorted(db.ds_data(), key=lambda d: int(d["addr"], 16))
    for a, b in zip(dd, dd[1:]):
        ea, sb = int(a["end"], 16), int(b["addr"], 16)
        if sb < ea:
            findings.append({"kind": "guest-overlap", "a": a, "b": b,
                             "overlap": ea - sb})
    # guest side: an item whose *sub-range* is itself a separate guest symbol (view pattern)
    by_addr = {int(d["addr"], 16): d for d in dd}
    for addr, d in by_addr.items():
        size = d["size"]
        if size <= 4:
            continue
        inner = [o for o in dd if int(o["addr"], 16) > addr and int(o["addr"], 16) < addr + size]
        if inner:
            findings.append({"kind": "guest-container-with-inner-symbols",
                             "container": d, "inner": inner[:6], "inner_n": len(inner)})
    # port side: two different port names claiming overlapping DS *spans*
    spans = []
    for addr, decls in pmap.items():
        for d in decls:
            gs = d.get("guest_size") or 4
            spans.append((addr, addr + gs, d))
    spans.sort(key=lambda t: (t[0], t[1]))
    for i, (a0, a1, da) in enumerate(spans):
        for b0, b1, db_ in spans[i + 1:]:
            if b0 >= a1:
                break
            if da["name"] == db_["name"]:
                continue
            findings.append({"kind": "port-span-overlap", "a": da["name"], "a_span": [hex(a0), hex(a1)],
                             "b": db_["name"], "b_span": [hex(b0), hex(b1)],
                             "overlap": min(a1, b1) - b0})
    for addr, decls in pmap.items():
        names = {d["name"] for d in decls}
        if len(names) > 1:
            findings.append({"kind": "port-two-names-one-cell", "ds": addr,
                             "names": sorted(names),
                             "types": [d["ctype"] for d in decls]})
    # join side: a port global that lands inside a LARGER guest item it does not name
    for addr, decls in pmap.items():
        for d in decls:
            gs = d.get("guest_size") or 4
            enclosing = [g for g in db.ds_data()
                         if int(g["addr"], 16) < addr < int(g["addr"], 16) + g["size"]]
            if enclosing:
                findings.append({"kind": "port-cell-inside-guest-item", "ds": addr,
                                 "port": d["name"], "port_size": gs,
                                 "guest_item": enclosing[0]})
    keep_out = []
    for f in findings:
        key = json.dumps(f, sort_keys=True, default=str)
        if key in seen:
            continue
        seen.add(key)
        keep_out.append(f)
    return keep_out


def c3_image_init(db, pmap):
    out = []
    for addr, decls in pmap.items():
        if not db.in_image(addr):
            continue
        img = None
        for d in decls:
            gs = d.get("guest_size") or 4
            img = db.image_at(addr, min(gs, 64))
            break
        if img is None:
            continue
        img_nz = any(img)
        # the port's initializer text for this cell (definitions, not externs)
        defs = [d for d in decls if d["file"] == "f.c"]
        init = (defs[0]["init"] if defs else "")
        host_only = bool(defs) and defs[0]["ctype"].startswith("struct")
        port_zero = init == "" or init in ("= 0", "= {0}", "= {0, 0}", "= NULL", "= {NULL}")
        if img_nz and port_zero:
            out.append({"ds": addr, "kind": "image-nonzero-port-zero",
                        "name": defs[0]["name"] if defs else decls[0]["name"],
                        "image_head": img[:16].hex(), "init": init})
        elif (not img_nz) and init and not port_zero:
            out.append({"ds": addr, "kind": "image-zero-port-nonzero",
                        "name": defs[0]["name"] if defs else decls[0]["name"],
                        "image_head": img[:16].hex(), "init": init[:120]})
    out.sort(key=lambda x: -len(x["image_head"]))
    return out


def c3b_full_span_initializers(db):
    """Byte-compare every port global that carries BOTH a DS address and a brace initializer against the
    shipped image at file offset DS-0x7600, over the PORT's own span (docs/121 §2).

    This check exists because the Ghidra symbol often has no recorded size for exactly these tables, so
    c3_image_init cannot reach them: the port's array width is then the only span being compared, and a
    table that is shorter or longer than the shipped data hides. A mismatch is a transcription error in
    the port's table, not a runtime difference.
    """
    # A DS address inside a shipped function body is CODE, not image data: the guest reads it with
    # data xrefs (a table embedded between instructions), and the port reproduces the *semantics*
    # rather than the bytes. Comparing those as initializers produced false positives -- the port's
    # g_scale_48a36 {0,1,2,3,4,4,5,5,6} is the decoded scale ladder, while the image bytes at that
    # address are instructions (feeding them in segfaults the fx fixtures). Filed separately.
    bodies = sorted((int(f["body_start"], 16), int(f["body_end"], 16), f["name"]) for f in db.functions)
    def code_owner(addr):
        lo, hi = 0, len(bodies) - 1
        while lo <= hi:
            mid = (lo + hi) // 2
            a, b, n = bodies[mid]
            if a <= addr <= b:
                return n
            # Data pools also sit in the few-byte gap just BEFORE a body (the guest reads them with a
            # data xref from that function): 0x48a36 is 0x12 below fn_048a48's entry.
            if 0 <= a - addr <= 0x40:
                return n + " (data pool in the gap before the body)"
            pass
            if addr < a:
                hi = mid - 1
            else:
                lo = mid + 1
        return None

    out, code_rows = [], []
    # One DS cell can be declared twice (a `uint32_t g_x = 1;` definition in f.c and an `extern uint32_t
    # g_x;` in f.h). Only the definition carries the initializer the loader-equivalent is, so a name is
    # judged by its initialised declaration when one exists -- otherwise every header extern reads as a
    # cell the port "left zero".
    by_name = {}
    for g in port_src.globals_():
        cur = by_name.get(g["name"])
        if cur is None or (not cur.get("init") and g.get("init")):
            by_name[g["name"]] = g
    # A definition the statement parser misses (a declaration inside a dense comment block) still exists
    # in the text, and a header `extern` must not be taken as the initialiser. So: if the parsed view has
    # no initialiser, look for a non-extern definition line in the port's .c files and read its init.
    src_text = {}
    for path in ("f/f.c", "f/fx_04794a.c", "sys/sys.c", "cd/cd_iso.c", "app/main.c", "app/rig_sink.c"):
        p = os.path.join(HERE, "..", "..", "port", "src", path)
        if os.path.exists(p):
            src_text[path] = open(p).read()
    def definition_init(name):
        for path, text in src_text.items():
            for mm in re.finditer(r"^[^\n]*?\b" + re.escape(name) + r"\b\s*(\[[^\]]*\])*\s*(=[^;{]*)?[;,]",
                                  text, re.M):
                line = mm.group(0)
                if "extern" in line.split(name)[0]:
                    continue
                return (mm.group(2) or "").strip()
        return None
    for g in by_name.values():
        init = (g.get("init") or "").strip()
        if not init:
            found = definition_init(g["name"])
            if found is not None:
                init = found
        if g["ds"] is None or g["ptr"]:
            continue                       # host pointer slot vs guest offset: c4's family
        if init.startswith("= {"):
            body = init[3:].strip()
            if body.endswith("}"):
                body = body[:-1]
            toks = [t.strip() for t in body.split(",") if t.strip()]
            if not toks or not all(re.fullmatch(r"0[xX][0-9a-fA-F]+|\d+", t) for t in toks):
                continue                   # nested braces / string initializers: not a scalar table
            width = g["type_size"]
            vals = [int(t, 0) for t in toks]
            port = b"".join(v.to_bytes(width, "little", signed=(v < 0)) if -2 ** (8 * width - 1) <= v < 2 ** (8 * width)
                            else b"?" * width for v in vals)
            kind = "array-init"
        elif init == "" or re.fullmatch(r"= -?(0[xX][0-9a-fA-F]+|\d+)", init):
            # docs/133: scalars belong here too. A DS-marked scalar initialised to 0 -- explicitly or by
            # BSS default -- claims the shipped cell starts zero. docs/132 §3's "port holds 0 where the
            # shipped side holds a value" family is exactly this, and it hid a real defect: DS
            # 0x5377c..0x537c5 is image-initialised with 18000 / 3600 / 1 / "EST" / "EDT".
            if (g.get("elems") or 1) != 1:
                continue
            width = g["type_size"]
            v = 0 if init == "" else int(init[1:], 0)
            port = v.to_bytes(width, "little", signed=(v < 0)) \
                if -2 ** (8 * width - 1) <= v < 2 ** (8 * width) else b"?" * width
            kind = "scalar-init"
        else:
            continue                       # string/struct initializers: not comparable byte-wise
        owner = code_owner(g["ds"])
        if owner:
            code_rows.append({"ds": hex(g["ds"]), "name": g["name"], "inside": owner,
                              "note": "address is inside a shipped function body (code, not image data); "
                                      "compared bytes would be instructions"})
            continue
        off = g["ds"] - ghidra_db.IMAGE_FILE_VS_DS
        if off < 0 or off + len(port) > len(db.image):
            out.append({"ds": hex(g["ds"]), "name": g["name"], "span": len(port), "kind": kind,
                        "bad": None, "note": "span runs outside the image-initialised DS region"})
            continue
        img = db.image_at(g["ds"], len(port))
        if img is None or img == b"\x00" * len(port):
            continue                       # BSS / zero image bytes: nothing to transcribe
        bad = [i for i in range(len(port)) if port[i] != img[i]]
        if bad:
            runs, i = [], 0
            while i < len(bad):
                j = i
                while j + 1 < len(bad) and bad[j + 1] == bad[j] + 1:
                    j += 1
                runs.append((bad[i], bad[j]))
                i = j + 1
            i0 = bad[0]
            out.append({"ds": hex(g["ds"]), "name": g["name"], "span": len(port), "kind": kind,
                        "bad": len(bad), "first": hex(g["ds"] + i0), "port": f"{port[i0]:#04x}",
                        "image": f"{img[i0]:#04x}",
                        "image_bytes": img[:24].hex(" "),
                        "image_text": "".join(chr(c) if 32 <= c < 127 else "." for c in img[:24]),
                        "runs": [f"{a:x}..{b:x}" for a, b in runs[:6]]})
    for r in code_rows:
        out.append(r)          # informational, but reported so the exclusion is auditable
    return out


def c4_representation(db, pmap):
    writes, reads = db.ds_write_sites(), db.ds_read_sites()
    guest_type = {int(d["addr"], 16): d["type"] for d in db.ds_data()}
    out = []
    for addr, decls in pmap.items():
        ptrdecls = [d for d in decls if d["ptr"] or d["ctype"] == "uintptr_t"]
        ws, rs = writes.get(addr, []), reads.get(addr, [])
        gt = guest_type.get(addr)
        img = db.image_u32(addr)
        small_imm = []
        for w in ws:
            for op in w[3][1:] if w[2] == "MOV" else []:
                op = op.strip()
                if op.startswith("0x"):
                    v = int(op, 16)
                    if 0 < v < 0x10000:
                        small_imm.append(f"{w[2]} {w[3][0]} {op}")
        idx_style = [r for r in rs if "[" in (r[3][0] if r[3] else "") and
                     any(t in r[3][0] for t in ("+", "*")) and _mentions(r[3][0], hex(addr))]
        score = len(small_imm) * 3 + (2 if img and 0 < img < 0x10000 else 0) + len(idx_style) * 2
        if ptrdecls and score:
            out.append({"ds": addr, "why": "port-declares-pointer-guest-looks-numeric",
                        "score": score, "port_name": ptrdecls[0]["name"],
                        "port_type": ptrdecls[0]["ctype"], "guest_type": gt,
                        "image_u32": img, "small_imm_stores": small_imm[:4],
                        "indexed_reads": [r[2] + " " + " ".join(r[3]) for r in idx_style][:3],
                        "n_reads": len(rs), "n_writes": len(ws)})
        # reverse direction: shipped symbol is a pointer, port models it as a plain integer
        if not ptrdecls and gt == "pointer":
            out.append({"ds": addr, "why": "guest-symbol-is-pointer-port-declares-integer",
                        "score": 5, "port_name": decls[0]["name"],
                        "port_type": decls[0]["ctype"], "guest_type": gt,
                        "image_u32": img, "n_reads": len(rs), "n_writes": len(ws)})
    out.sort(key=lambda x: -x["score"])
    return out


def c5_dead_sides(db):
    guest_fns = {f["name"] for f in db.functions}
    port_defs, port_calls = port_src.functions()
    called = set()
    for src, s in port_calls.items():
        called |= s
    guest_no_port = sorted(guest_fns - set(port_defs))
    port_dead = sorted(n for n in port_defs if n not in called and n not in ("main",))
    return {"guest_functions_without_port_counterpart": guest_no_port,
            "port_functions_never_called_in_port": port_dead}


def c6_image_data_without_symbol(db, pmap):
    """Runs of non-zero image-initialised DS cells that Ghidra never named.

    If the guest also points at the run with an immediate, the port had no decompiler-visible
    reason to copy those bytes -> the classic 'port zero-initialises shipped data' bug."""
    imms = ds_immediates(db)
    syms = sorted(db.ds_data(), key=lambda d: int(d["addr"], 16))
    covered = set()
    for d in syms:
        a, s = int(d["addr"], 16), d["size"]
        covered.update(range(a, a + s))
    runs, a = [], ghidra_db.DS_LO
    hi = ghidra_db.IMAGE_FILE_VS_DS + len(db.image)          # last image-initialised DS address
    hi = min(hi, ghidra_db.DS_HI)
    while a < hi:
        v = db.image_u32(a)
        if v and a not in covered:
            b = a
            while b < hi and db.image_u32(b) and b not in covered:
                b += 4
            runs.append((a, b))
            a = b
        else:
            a += 4
    out = []
    for a, b in runs:
        if b - a < 8:
            continue
        near = [x for base in imms for x in ([base] if a - 0x800 <= base < b + 0x800 else [])]
        port_here = [d["name"] for addr, decls in pmap.items() for d in decls if a <= addr < b]
        out.append({"ds_start": hex(a), "ds_end": hex(b), "cells": (b - a) // 4,
                    "image_head": (db.image_at(a, min(16, b - a)) or b"").hex(),
                    "immediates_pointing_nearby": len(near),
                    "distinct_base_constants": sorted({hex(x) for x in near})[:6],
                    "port_models_anything_here": port_here})
    out.sort(key=lambda r: -(r["cells"] * (1 + r["immediates_pointing_nearby"])))
    return out


def c7_table_bases(db):
    imms = ds_immediates(db)
    out = []
    for base, sites in sorted(imms.items()):
        fns = sorted({s[1] for s in sites if s[1]})
        out.append({"ds": hex(base), "n_sites": len(sites), "fns": fns[:6]})
    out.sort(key=lambda x: -x["n_sites"])
    return out


def c8_base_pair_aliasing(db, pmap):
    """Two DS base constants whose access spans overlap -> views of one shipped table.

    Ghidra models this image as flat dwords, so a table reached through a register base has no
    symbol; when the game (and therefore the port) carries two names for overlapping ranges of the
    same table, the write-then-read aliases become invisible. This finds the pairs from operand
   text alone: docs/91 (0x53e9c vs 0x53eb8 = +0x1c) and docs/98 (slot cells of a pool) look like
    this and must show up without being named."""
    acc = collections.defaultdict(lambda: {"n": 0, "strides": set(), "disps": set(),
                                           "fns": set(), "mnems": set()})
    for m in db.mem_access:
        for op in m["ops"]:
            base = None
            for mm in DS_CONST.finditer(op):
                base = int(mm.group(1), 16)
            if base is None:
                continue
            a = acc[base]
            a["n"] += 1
            a["fns"].add(m["fn"])
            a["mnems"].add(m["mnem"])
            for st in STRIDE.finditer(op):
                a["strides"].add(int(st.group(1), 16) if st.group(1).startswith("0x") else int(st.group(1)))
            for d in DISP.finditer(op):
                v = int(d.group(1), 16)
                if 0 < v < 0x1000:
                    a["disps"].add(v)
    bases = sorted(acc)
    out = []
    for i, b1 in enumerate(bases):
        for b2 in bases[i + 1:]:
            dist = b2 - b1
            if dist > 0x400:
                break
            a1, a2 = acc[b1], acc[b2]
            if a1["n"] < 2 or a2["n"] < 2:
                continue
            shared = a1["fns"] & a2["fns"]
            out.append({"base_a": hex(b1), "base_b": hex(b2), "distance": hex(dist),
                        "n_a": a1["n"], "n_b": a2["n"],
                        "strides_a": sorted(a1["strides"])[:4], "strides_b": sorted(a2["strides"])[:4],
                        "same_stride": bool(a1["strides"] & a2["strides"]),
                        "shared_functions": sorted(x for x in shared if x)[:4],
                        "port_name_a": sorted({d["name"] for d in pmap.get(b1, [])}),
                        "port_name_b": sorted({d["name"] for d in pmap.get(b2, [])}),
                        "score": (10 if a1["strides"] & a2["strides"] else 0) + len(shared) * 3
                                 + min(a1["n"], a2["n"])})
    out.sort(key=lambda x: -x["score"])
    # keep the pairs that can actually alias something the port models, or that are so close
    # together that they cannot be independent objects
    keep = [p for p in out if (p["port_name_a"] and p["port_name_b"]) or p["same_stride"]
            or int(p["distance"], 16) <= 0x40]
    return keep


def c9_access_width_vs_port_type(db, pmap):
    """Every DS cell the guest touches through a *sized, absolute* operand, where the port's declared
    element width is not one of the widths the shipped code actually moves there.

    Ghidra only defines a data symbol where it can, so 66 of the rig's 283 cells had no shipped span at
    all (docs/132 §3) and the state oracle had to fall back to the port's own width -- a comparison
    basis nobody checked. The instruction text is independent evidence: `MOV AL, byte ptr [0x63384]`
    says a byte lives at 0x63384. Findings are split by how actionable they are:

      scalar-width-mismatch      a single-value global whose width contradicts the shipped access
                                 (docs/133: 0x537a7 was a 4-byte global used as a 31-byte buffer)
      byte-array-with-dword-fields  a flattened array whose fields the guest moves as wider values
                                 (informational: the port reaches them through dword views)
      host-pointer-vs-guest-offset the docs/94 offset-vs-pointer family, already filed by c4

    A cell whose *defined* shipped span equals the port's element width is not a finding at all: a
    narrow read of a wide cell (a byte read out of a dword global) is normal.
    """
    acc = db.ds_direct_widths()
    dd = {int(d["addr"], 16): d for d in db.ds_data() if d.get("size")}
    out = []
    for addr, decls in sorted(pmap.items()):
        ws = acc.get(addr)
        if not ws:
            continue
        dspan = dd.get(addr, {}).get("size")
        for d in decls:
            if d.get("ptr"):
                continue          # offset-vs-pointer is c4's finding, not a width question
            slot = d.get("type_size") or 0
            if not slot:
                continue
            if slot in ws:
                continue
            if dspan and dspan == slot:
                continue          # matches the defined span; the access is just narrower
            if d["name"].endswith("_ent") or d["ctype"].endswith("*") \
                    or d["ctype"].startswith(("uintptr_t", "intptr_t")) and max(ws) == 4 and slot == 8:
                kind = "host-pointer-vs-guest-offset"
            elif (d.get("elems") or 1) > 1 or (d.get("elems") or 0) < 0 or d["ctype"].endswith("*"):
                kind = "byte-array-with-dword-fields"
            else:
                kind = "scalar-width-mismatch"
            out.append({"ds": hex(addr), "name": d["name"], "ctype": d["ctype"],
                        "kind": kind, "shipped_defined_span": dspan,
                        "elems": d.get("elems"), "port_elem_width": slot,
                        "guest_access_widths": sorted(ws),
                        "file": d.get("file"), "line": d.get("line")})
    out.sort(key=lambda x: (x["kind"], x["ds"], x["name"]))
    return out


def c9_width_coverage(db, pmap):
    """How much of the rig's comparison basis is actually shipped evidence (docs/132 §4(a))."""
    acc = db.ds_direct_widths()
    dd = {int(d["addr"], 16): d for d in db.ds_data() if d.get("size")}
    n_def = n_acc = n_none = 0
    for addr in sorted(pmap):
        if addr in dd:
            n_def += 1
        elif addr in acc:
            n_acc += 1
        else:
            n_none += 1
    return {"port_cells": len(pmap), "shipped_size_from_defined_data": n_def,
            "shipped_width_from_access_operand": n_acc, "no_shipped_width_at_all": n_none}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", default=os.path.join(HERE, "reports", "static_findings.json"))
    ap.add_argument("--md", default=os.path.join(HERE, "reports", "static_findings.md"))
    a = ap.parse_args()
    os.makedirs(os.path.dirname(a.json), exist_ok=True)

    db = ghidra_db.Db()
    pmap = port_global_map()
    res = {
        "c1_unmodelled": c1_unmodelled(db, pmap),
        "c2_aliasing": c2_aliasing(db, pmap),
        "c3_image_init": c3_image_init(db, pmap),
        "c3b_full_span_initializers": c3b_full_span_initializers(db),
        "c4_representation": c4_representation(db, pmap),
        "c5_dead_sides": c5_dead_sides(db),
        "c6_image_data_without_symbol": c6_image_data_without_symbol(db, pmap),
        "c7_ds_table_bases": c7_table_bases(db),
        "c8_base_pair_aliasing": c8_base_pair_aliasing(db, pmap),
        "c9_access_width_vs_port_type": c9_access_width_vs_port_type(db, pmap),
        "c9_width_coverage": c9_width_coverage(db, pmap),
    }
    with open(a.json, "w") as f:
        json.dump(res, f, indent=1, default=str)

    lines = ["# Static equivalence audit (Ghidra DB vs port source)", ""]
    lines.append(f"port globals with a DS address: {sum(len(v) for v in pmap.values())} "
                 f"across {len(pmap)} distinct DS cells; guest DS defined-data items: "
                 f"{len(db.ds_data())}; guest DS cells with accesses: {len(db.ds_dir_counts())}")
    for k in ("c1_unmodelled", "c2_aliasing", "c3_image_init", "c3b_full_span_initializers", "c4_representation",
              "c6_image_data_without_symbol", "c7_ds_table_bases", "c8_base_pair_aliasing",
              "c9_access_width_vs_port_type"):
        lines.append(f"\n## {k}: {len(res[k])} findings")
        for item in res[k][:25]:
            lines.append("  " + json.dumps(item, default=str)[:300])
    c5 = res["c5_dead_sides"]
    cov = res["c9_width_coverage"]
    lines.append(f"\n## c9 comparison-basis coverage: {cov['port_cells']} port DS cells -> "
                 f"{cov['shipped_size_from_defined_data']} shipped span, "
                 f"{cov['shipped_width_from_access_operand']} width only from the access operand, "
                 f"{cov['no_shipped_width_at_all']} with no shipped width at all")
    lines.append(f"\n## c5_dead_sides: guest-without-port={len(c5['guest_functions_without_port_counterpart'])}"
                 f" port-never-called={len(c5['port_functions_never_called_in_port'])}")
    for k in c5:
        for n in c5[k][:20]:
            lines.append(f"  {k[:12]} {n}")
    with open(a.md, "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines[:3] + [f"wrote {a.json} and {a.md}"]))
    print({k: (len(v) if isinstance(v, list) else {kk: (len(vv) if isinstance(vv, list) else vv)
                                                    for kk, vv in v.items()})
           for k, v in res.items()})


if __name__ == "__main__":
    main()
