#!/usr/bin/env python3
"""Per-variable, per-window state diff between the original and the port (state oracle).

Both sides are reduced to the *same* table: the cells the shipped Ghidra symbol database says exist
(scripts/audit/gen -> rig_cells.json). For the original the bytes come from physical DS memory
snapshots (scripts/rig/replay_original.py); for the port from its own dump (app/rig_sink.c). Nothing
is translated or normalised away: a cell whose two widths disagree is reported as a representation
finding, not silently truncated.

Windows are drive-relative: the drive file's key-down edges cut the timeline into phases, and each
phase is compared as a *value set* (plus first/last), because host pacing jitters the absolute frame
number while the ordered content of an input window does not move (docs/56).

  diff_state.py --orig A --port P --drive DRIVE [--md OUT.md]
  diff_state.py --baseline A B --drive DRIVE        # FP control: original vs original must be clean
"""
import argparse
import collections
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "..", "audit", "gen")
DS_LO = 0x50000


def load_cells():
    return json.load(open(os.path.join(GEN, "rig_cells.json")))


def load_entropy():
    p = os.path.join(GEN, "entropy_cells.json")
    if not os.path.exists(p):
        return set()
    return {int(k, 16) for k in json.load(open(p))["entropy_cells"]}


def load_orig(run):
    """-> (samples, ptr_cells). samples are (t_rel, ds_bytes, heap_bytes); t_rel is relative to the
    replay anchor. heap_bytes is the concatenation of one fixed-size block per pointer cell, in the
    order recorded in timeline.json["ptr_cells"]."""
    tl = json.load(open(os.path.join(run, "timeline.json")))
    reb = tl.get("rebases") or [
        {"t_ms": w["t_ms"], "nominal": w["t_ms"] - w["shift_ms"]} for w in tl.get("ds_waits", [])]
    t0 = tl["timeline"][0]["t_wall"] if tl["timeline"] else 0.0

    def nominal(t_ms):
        """Emulator clock -> drive nominal. The replay releases each segment when its `wait ds` fires,
        so the emulator's clock and the drive's nominal time differ by a per-segment offset; the port's
        dump is stamped in nominal time, and the phase windows are cut in nominal time too."""
        off = reb[0]["nominal"] - reb[0]["t_ms"] if reb else -int(t0 * 1000)
        for r in reb:
            if r["t_ms"] <= t_ms:
                off = r["nominal"] - r["t_ms"]
            else:
                break
        return t_ms + off
    out = []
    for i, st in enumerate(tl["timeline"]):
        p = os.path.join(run, f"state_{i:04d}.ds.bin")
        if not os.path.exists(p):
            p = os.path.join(run, f"state_{i + 1:04d}.ds.bin")
        if not os.path.exists(p):
            continue
        hp = os.path.join(run, f"state_{i:04d}.heap.bin")
        if not os.path.exists(hp):
            hp = os.path.join(run, f"state_{i + 1:04d}.heap.bin")
        hb = open(hp, "rb").read() if os.path.exists(hp) else b""
        out.append((nominal(int(st["t_wall"] * 1000)), open(p, "rb").read(), hb))
    return out, tl.get("ptr_cells", [])


def load_port(run):
    meta = json.load(open(os.path.join(run, "meta.json")))
    rb, cells = meta["record_bytes"], meta["cells"]
    rec = struct.Struct("<qQQQII")   # must match rig_sink.c's padding-free layout
    out = []
    with open(os.path.join(run, "cells.bin"), "rb") as f:
        f.read(16)                                    # NGRC header
        while True:
            h = f.read(rec.size)
            if len(h) < rec.size:
                break
            t_ms, frame, audio, screen, tick, n = rec.unpack(h)
            blob = f.read(rb)
            if len(blob) < rb:
                break
            out.append((t_ms, blob, cells, {"frame": frame, "audio": audio}))
    return out, meta["cells"]


def phases_from_drive(path):
    """Windows cut by key-down edges: [(label, t0, t1)].

    Everything before the drive's first anchor is dropped on both sides: each side boots at its own
    speed (the emulator reaches the title at 42 s, the port at 47 s), so those windows compare a
    half-loaded program against a differently half-loaded one and every cell reads DIFF. The anchor is
    the first instant where both sides are known to be at the same shipped event."""
    edges, anchors = [], []
    for line in open(path):
        line = line.split("#")[0].strip()
        f = line.split()
        if len(f) >= 4 and f[1] == "key" and f[3].startswith("d"):
            edges.append(int(f[0]))
        if len(f) >= 2 and f[1] in ("anchor", "wait"):
            anchors.append(int(f[0]))
    edges = sorted(set([0] + edges))
    if anchors:
        cut = min(anchors)
        edges = [e for e in edges if e >= cut] or [cut]
    out = []
    for i in range(len(edges)):
        t0 = edges[i]
        t1 = edges[i + 1] if i + 1 < len(edges) else 1 << 40
        out.append((f"P{i}@{t0}", t0, t1))
    return out


def val(blob, off, size):
    b = blob[off:off + size]
    return int.from_bytes(b, "little") if b else None


def looks_addr(v):
    """A value that reads like a linear address in the game's own data/heap range. Such cells cannot
    be compared by value across boots (the allocator's base moves), but their *shape* can."""
    return v is not None and 0x10000 <= v < 0x01000000


def heap_shape_equal(so, sp):
    """Allocation addresses move between boots, and how many intermediate pointer values a sample
    stream happens to catch also varies, so compare the shape that is stable: the number of distinct
    blocks and the span they occupy."""
    nz_o = {v for v in so if looks_addr(v)}
    nz_p = {v for v in sp if looks_addr(v)}
    if not nz_o or len(nz_o) != len(nz_p):
        return False
    if {v for v in so if not looks_addr(v)} != {v for v in sp if not looks_addr(v)}:
        return False
    return (max(nz_o) - min(nz_o)) == (max(nz_p) - min(nz_p))


def hexs(values, limit=3):
    """A phase's value set, rendered so a reviewer can see the shape without the tool deciding what
    the number means: <=4 bytes as an integer, longer cells as a byte prefix."""
    out = []
    for v in sorted(values, key=lambda x: (len(x), x))[:limit]:
        out.append("0x" + (v[:24].hex() + (".." if len(v) > 24 else "")) if len(v) > 4
                   else "0x" + int.from_bytes(v, "little").__format__("x"))
    return ",".join(out) + ("..." if len(values) > limit else "")


# Entity-record fields the shipped disassembly names, so a record diff LOCALIZES to a field (and hence
# to the function that writes it) instead of stopping at "the first differing byte" (blind-gate contract:
# every detection names the divergent window AND the suspect function / DS region).
ENTITY_FIELDS = [
    # Offsets and widths are NOT guesses: they are the store set of the shipped entity constructor
    # fn_010efc (guest 0x00010f36-0x000112e6), whose stores are packed and deliberately unaligned
    # (dword [EDX+0x1], [EDX+0x5], [EAX+0x17], ...; docs/125). Validated against shipped runtime
    # values: hp@+0x29 = 0x3e8 (1000) decaying to 0x3e5, exp_total@+0x2b stepping 0 -> 0xa -> 0x14.
    (0x17, "state", 4), (0x29, "hp", 2), (0x2b, "exp_total", 4), (0x32, "exp_award", 4),
    (0x4a, "link_ent", 4), (0x52, "score", 4), (0x56, "pos_x", 4), (0x6c, "chain_gate", 1),
    (0x73, "pos_y", 4), (0x123, "act_container", 4), (0x13f, "next_ent", 4),
]


def field_diff(a, b):
    out = []
    for off, nm, w in ENTITY_FIELDS:
        if off + w > min(len(a), len(b)):
            continue
        va = int.from_bytes(a[off:off + w], "little")
        vb = int.from_bytes(b[off:off + w], "little")
        if va != vb:
            out.append(f"{nm}@+0x{off:02x} {va:#x}/{vb:#x}")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", help="original run dir (replay_original.py output)")
    ap.add_argument("--port", help="port dump dir (night --rig-dump)")
    ap.add_argument("--baseline", nargs=2, metavar=("A", "B"), help="compare two original runs")
    ap.add_argument("--drive", required=True)
    ap.add_argument("--md")
    ap.add_argument("--max-rows", type=int, default=60)
    ap.add_argument("--update-entropy", action="store_true",
                    help="in --baseline mode, record measured-variable cells in entropy_cells.json")
    a = ap.parse_args()

    cells = load_cells()
    entropy = load_entropy()
    span = {}
    for c in cells:
        span[c["name"]] = (c["ds"], c["guest_size"], c["host_size"], c["ptr"])
    phases = phases_from_drive(a.drive)

    if a.baseline:
        (orig_side, orig_ptrs), (other_side, other_ptrs) = (load_orig(a.baseline[0]),
                                                            load_orig(a.baseline[1]))
        mode = "baseline"
    else:
        if not (a.port and a.orig):
            sys.exit("--orig and --port are required unless --baseline is used")
        (orig_side, orig_ptrs), (other_side, other_ptrs) = load_orig(a.orig), load_port(a.port)
        mode = "orig-vs-port"
    is_port = mode == "orig-vs-port"

    # index samples by phase
    def bucket(samples):
        """Blobs per phase. The shipped side only records a state when it *changes*, so a quiet window
        has no sample inside it at all; carrying the last earlier sample forward keeps the comparison
        continuous instead of turning every idle phase into NO-DATA (which also silently drops the
        cells that only matter once, like a kill counter)."""
        srt = sorted(samples, key=lambda s: s[0])
        b = collections.defaultdict(list)
        for label, t0, t1 in phases:
            inside = [s[1] for s in srt if t0 <= s[0] < t1]
            if not inside:
                prior = [s[1] for s in srt if s[0] < t0]
                inside = [prior[-1]] if prior else []
            b[label] = inside
        return b

    def span_t(samples):
        ts = [s[0] for s in samples]
        return f"n={len(ts)} t=[{min(ts)}..{max(ts)}]" if ts else "n=0"
    print(f"samples: orig {span_t(orig_side)} | {'port' if is_port else 'other'} {span_t(other_side)}")
    ob, pb = bucket(orig_side), bucket(other_side)
    poff = {}
    if is_port:
        poff = {c["name"]: c["offset"] for c in other_side[0][2]}
    rows = []
    for c in cells:
        ds, gsz, hsz, ptr = c["ds"], c["guest_size"], c["host_size"], c["ptr"]
        # With no shipped size for the symbol, compare the port's own width; where the two widths
        # differ, compare the overlap and keep the disagreement as a finding of its own.
        gs = gsz or hsz
        w = min(gs, hsz)
        if any((ds + k) in entropy for k in range(max(gs, hsz))):
            verdict, note = "ENTROPY", "varies across identical replays of the original"
        elif ptr:
            verdict, note = "PTR", f"host slot {hsz}B vs guest {gsz}B (offset-vs-pointer, docs/94 family)"
        elif gsz and gsz != hsz:
            verdict, note = "REPR", f"width {gsz}B guest vs {hsz}B port; compared the first {w}B"
        elif not gsz:
            verdict, note = "SIZE?", f"no shipped size for this symbol; compared {w}B"
        else:
            verdict, note = "SAME", ""
        per_phase = []
        if verdict in ("SAME", "REPR", "SIZE?"):
            off_p = poff.get(c["name"])
            for label, _t0, _t1 in phases:
                ov = [b[ds - DS_LO:ds - DS_LO + w] for b in ob.get(label, [])]
                pv = [b[off_p:off_p + w] for b in pb.get(label, [])] if off_p is not None else []
                if not ov or not pv:
                    per_phase.append((label, "NO-DATA", "", ""))
                    continue
                so, sp = {bytes(x) for x in ov if x}, {bytes(x) for x in pv if x}
                if so == sp:
                    per_phase.append((label, "SAME", hexs(so), hexs(sp)))
                    continue
                # localise: first byte position where the two sides' samples disagree
                vo = sorted(so, key=lambda x: (len(x), x))[0]
                vp = sorted(sp, key=lambda x: (len(x), x))[0]
                at = next((i for i in range(w) if vo[i] != vp[i]), 0)
                detail = f"@{hex(ds + at)} {vo[at]:#04x} vs {vp[at]:#04x}"
                verdict = "DIFF"
                per_phase.append((label, "DIFF", hexs(so), hexs(sp)))
                rows_note = note + f"; first differing byte {detail}"
                note = rows_note
        rows.append({"ds": hex(ds), "name": c["name"], "guest_size": gsz, "host_size": hsz,
                     "verdict": verdict, "note": note, "phases": per_phase})

    # ---- entity/record blocks behind the pointer cells -----------------------------------------
    # A fixed-size raw block at each pointer, compared at a fixed offset from the record base: the
    # playtest items live in record fields (state, stagger, EXP, HP, position), and this localises a
    # divergence to `base+0xNN` without assuming a layout the shipped binary has not told us.
    RB = 64
    if orig_ptrs:
        h_o = collections.defaultdict(list)
        for smp in orig_side:
            hb = smp[2] if len(smp) > 2 else b""
            if not hb:
                continue
            for label, t0, t1 in phases:
                if t0 <= smp[0] < t1:
                    h_o[label].append(hb)
                    break
        h_p = collections.defaultdict(list)
        if is_port:
            for smp in other_side:
                for label, t0, t1 in phases:
                    if t0 <= smp[0] < t1:
                        h_p[label].append(smp[1])
                        break
        oidx = {p["name"]: i for i, p in enumerate(orig_ptrs)}
        pidx = {p["name"]: i for i, p in enumerate(other_ptrs)
                if is_port and p.get("rec_offset", 0xFFFFFFFF) != 0xFFFFFFFF}
        if not is_port:
            pidx = oidx
        for name, i in oidx.items():
            j = pidx.get(name)
            if j is None:
                continue
            per, verdict = [], "REC-SAME"
            # Record-block geometry comes from each side's own dump: the shipped side concatenates one
            # fixed-size block per pointer cell in timeline["ptr_cells"] order, the port stores each
            # block at its declared rec_offset. The block size is per-run (replay_original PTR_BYTES,
            # rig_sink RIG_REC), so it is read from the entries instead of being hardcoded.
            rb_o = orig_ptrs[i].get("rec_bytes") or RB
            base_o = sum((p.get("rec_bytes") or RB) for p in orig_ptrs[:i])
            rb_p = (other_ptrs[j].get("rec_bytes") or RB) if is_port else rb_o
            base_p = (other_ptrs[j]["rec_offset"] if is_port
                      else sum((p.get("rec_bytes") or RB) for p in other_ptrs[:j]))
            for label, _t0, _t1 in phases:
                ov = [b[base_o:base_o + rb_o] for b in h_o.get(label, [])]
                pv = [b[base_p:base_p + rb_p] for b in h_p.get(label, [])]
                ov = [x for x in ov if len(x) == rb_o]
                pv = [x for x in pv if len(x) == rb_p]
                if not ov or not pv:
                    per.append((label, "NO-DATA", "", ""))
                    continue
                so, sp = {bytes(x) for x in ov}, {bytes(x) for x in pv}
                if so == sp:
                    per.append((label, "REC-SAME", "nonzero" if any(any(x) for x in so) else "all-null",
                                "nonzero" if any(any(x) for x in sp) else "all-null"))
                    continue
                nz_o = [x for x in so if any(x)]
                nz_p = [x for x in sp if any(x)]
                if not nz_o or not nz_p:
                    verdict = "REC-NULL"
                    per.append((label, "REC-NULL", f"{len(nz_o)} non-null", f"{len(nz_p)} non-null"))
                    continue
                if mode == "baseline":
                    verdict = "REC-VARIES"      # record contents embed allocation addresses: expected
                a_, b_ = nz_o[0], nz_p[0]
                at = next((k for k in range(RB) if a_[k] != b_[k]), 0)
                nbad = sum(1 for k in range(min(len(a_), len(b_))) if a_[k] != b_[k])
                fld = field_diff(a_, b_)
                verdict = "REC-DIFF"
                per.append((label, "REC-DIFF",
                            f"{nbad}B differ; rec+0x{at:02x}={a_[at]:#04x}",
                            f"rec+0x{at:02x}={b_[at]:#04x}"
                            + ("  <== " + "; ".join(fld) if fld else "")))
            base = next((c["ds"] for c in cells if c["name"] == name), 0)
            rows.append({"ds": hex(base), "name": name + "#rec", "guest_size": RB, "host_size": RB,
                         "verdict": verdict, "note": "raw block at the pointer cell's target",
                         "phases": per})

    tally = collections.Counter(r["verdict"] for r in rows)
    print(f"mode={mode}  cells={len(rows)}  phases={len(phases)}  tally={dict(tally)}")
    shown = 0
    for r in rows:
        if r["verdict"] in ("DIFF", "REPR", "PTR", "REC-DIFF", "REC-NULL") and shown < a.max_rows:
            shown += 1
            dph = [p for p in r["phases"] if p[1] != "SAME"]
            print(f'{r["verdict"]:6s} {r["ds"]} {r["name"]:24s} g={r["guest_size"]} h={r["host_size"]} '
                  f'{r["note"]}' + (f'  {"; ".join(f"{p[0]}:{p[1]} orig={p[2]} port={p[3]}" for p in dph[:2])}'
                                    if r["verdict"] == "DIFF" else ""))
    if a.md:
        os.makedirs(os.path.dirname(a.md), exist_ok=True)
        with open(a.md, "w") as f:
            f.write("# state-oracle diff table\n\n")
            f.write(f"mode `{mode}`; cells {len(rows)}; phases {len(phases)}; tally {dict(tally)}\n\n")
            f.write("| verdict | DS | port symbol | guest size | host size | note / first differing window |\n")
            f.write("|---|---|---|---|---|---|\n")
            for r in rows:
                if r["verdict"] == "SAME":
                    continue
                extra = r["note"] + "".join(
                    f' | {p[0]} {p[1]} orig={p[2]} port={p[3]}' for p in r["phases"] if p[1] != "SAME")
                f.write(f'| {r["verdict"]} | {r["ds"]} | {r["name"]} | {r["guest_size"]} | '
                        f'{r["host_size"]} | {extra} |\n')
        print(f"wrote {a.md}")
    if a.update_entropy and mode == "baseline":
        # measured variability, not assumed: cells that differ between two identical replays of the
        # original are nondeterminism sources, so they join the exclusion list with a reason.
        path = os.path.join(GEN, "entropy_cells.json")
        doc = json.load(open(path)) if os.path.exists(path) else {"entropy_cells": {}}
        for r in rows:
            if r["verdict"] in ("DIFF", "HEAP"):
                doc["entropy_cells"][r["ds"]] = {"name": r["name"],
                                                 "reason": "varies across identical replays "
                                                           "(allocation address / timing)"}
        json.dump(doc, open(path, "w"), indent=1)
        print(f"entropy_cells.json now has "
              f"{len(json.load(open(path))['entropy_cells'])} measured-variable cells")

    if mode == "baseline":
        bad = [r for r in rows if r["verdict"] == "DIFF"]
        print("FP CONTROL (original vs original):", "PASS" if not bad else f"FAIL ({len(bad)} DIFF)")
        sys.exit(0 if not bad else 1)


if __name__ == "__main__":
    main()
