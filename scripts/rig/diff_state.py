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


def looks_like_entity(b):
    """Only name fields when the block plausibly IS an entity record: the constructor's type byte is a
    small id and the state dword stays inside the id alphabet. Without this gate the candidate names are
    applied to record-array blocks (0x51a0c, 0x53b3c ...) and read as nonsense."""
    if len(b) < 0x1b:
        return False
    return b[0] < 0x40 and int.from_bytes(b[0x17:0x1b], "little") < 0x200


def field_diff(a, b):
    if not (looks_like_entity(a) and looks_like_entity(b)):
        return []
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
    ap.add_argument("--anchor", default="",
                    help="event anchor '<ds>:<value>' (e.g. 0x63e14:67): the first sample on each side whose DS cell "
                         "holds that value. The port's dump timeline is rebased onto the original's, so phase windows "
                         "compare the SAME shipped event on both sides instead of two different wall clocks (docs/101, "
                         "docs/158 §5).")
    ap.add_argument("--update-entropy", action="store_true",
                    help="in --baseline mode, record measured-variable cells in entropy_cells.json")
    a = ap.parse_args()

    cells = load_cells()
    entropy = load_entropy()
    span = {}
    for c in cells:
        span[c["name"]] = (c["ds"], c["guest_size"], c["host_size"], c["ptr"])
    phases = phases_from_drive(a.drive)
    phases_dict = {lbl: (t0, t1) for lbl, t0, t1 in phases}

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

    # docs/158 §5: "first live entity" is a bad anchor (the shipped title screen already owns entities at 400 ms
    # while the port reaches its title at 60 s), and a level measurement is not a comparable event.  An anchor has
    # to be a TRANSITION both sides define identically, so --anchor takes <ds>:<value> and re-phases the port side
    # onto the original's clock before any window is cut.
    anchor_shift = None
    if a.anchor and mode == "orig-vs-port":
        try:
            dsx, val = a.anchor.split(":", 1)
            ad, av = int(dsx, 0), int(val, 0)
        except Exception:
            sys.exit(f"--anchor expects <ds>:<value>, got {a.anchor!r}")
        cell = next((c for c in cells if c["ds"] == ad), None)
        if cell is None:
            sys.exit(f"anchor DS {a.anchor} is not in the cell table")
        w = max(1, cell["guest_size"] or cell["host_size"])
        po = {c["name"]: c["offset"] for c in other_side[0][2] if c["ds"] == ad}
        pofs = po.get(cell["name"])

        def first_at(samples, portside):
            for smp in sorted(samples, key=lambda x: x[0]):
                o = pofs if portside else ad - DS_LO
                if o is None or o + w > len(smp[1]):
                    continue
                if int.from_bytes(smp[1][o:o + w], "little") == av:
                    return smp[0]
            return None
        t_o, t_p = first_at(orig_side, False), first_at(other_side, True)
        if t_o is None or t_p is None:
            sys.exit(f"anchor value {a.anchor} never appears (orig={t_o} port={t_p})")
        anchor_shift = t_o - t_p
        print(f"anchor {a.anchor}: orig t={t_o} ms, port t={t_p} ms -> port timeline shifted by {anchor_shift} ms")
        other_side = [(s[0] + anchor_shift,) + s[1:] for s in other_side]

    # index samples by phase
    def bucket(samples):
        """Blobs per phase. The shipped side only records a state when it *changes*, so a quiet window
        has no sample inside it at all; carrying the last earlier sample forward keeps the comparison
        continuous instead of turning every idle phase into NO-DATA (which also silently drops the
        cells that only matter once, like a kill counter)."""
        srt = sorted(samples, key=lambda s: s[0])
        b = collections.defaultdict(list)
        raw = collections.Counter()          # samples actually captured inside the window
        for label, t0, t1 in phases:
            inside = [s[1] for s in srt if t0 <= s[0] < t1]
            raw[label] = len(inside)
            if not inside:
                prior = [s[1] for s in srt if s[0] < t0]
                inside = [prior[-1]] if prior else []
            b[label] = inside
        return b, raw

    def span_t(samples):
        ts = [s[0] for s in samples]
        return f"n={len(ts)} t=[{min(ts)}..{max(ts)}]" if ts else "n=0"
    print(f"samples: orig {span_t(orig_side)} | {'port' if is_port else 'other'} {span_t(other_side)}")
    ob, o_raw = bucket(orig_side)
    pb, p_raw = bucket(other_side)
    # docs/159 §5: the shipped dump is change-triggered over the WHOLE DS group, so a window with zero shipped
    # samples means nothing in 0x50000..0x650bf changed during it.  Carrying the last sample forward is correct
    # for comparing values but it silently turns "the original was idle" into "the two sides agree".  A quiet
    # window is now labelled with its length and the other side's activity, so the claim stays checkable.
    quiet = {}
    if True:
        p_srt = sorted(other_side, key=lambda x: x[0])
        for label, t0, t1 in phases:
            if o_raw[label]:
                continue
            ps = [x[1] for x in p_srt if t0 <= x[0] < t1]
            moved = set()
            # the port's own cell table, not the shared rig table: entity record slots (ent00..) live only there
            pc = other_side[0][2] if is_port else cells
            for i in range(1, len(ps)):
                for c in pc:
                    w0 = max(1, c["host_size"])
                    if any((c["ds"] + k) in entropy for k in range(max(w0, c["guest_size"] or 1))):
                        continue          # entropy cells are excluded on BOTH sides (docs/101)
                    co = c.get("offset", c.get("rec_offset"))
                    if co is None or co + w0 > len(ps[i]):
                        continue
                    if ps[i][co:co + w0] != ps[i - 1][co:co + w0]:
                        moved.add(c["name"])
            quiet[label] = (int(t1 - t0), len(ps), sorted(moved))
    if quiet:
        tot = sum(v[0] for v in quiet.values())
        real = {k: v for k, v in quiet.items() if v[2]}
        ent_only = len(quiet) - len(real)
        print(f"shipped-quiet windows: {len(quiet)} of {len(phases)}  ({tot / 1000.0:.1f} s of the route); "
              f"{len(real)} of them the other side changed a NON-entropy cell in, {ent_only} entropy-motion only")
        # docs/159 §8: settle the reading-layer conflict inside the oracle -- one code path, one clock, one cell
        # table -- by printing BOTH sides' entity count and room command list for each quiet window, read with
        # exactly the offsets the verdict loop uses.
        probe = {}
        for c in cells:
            if c["ds"] in (0x53c68, 0x63e14, 0x639dc):
                probe[c["ds"]] = c
        pby = {c["name"]: c for c in other_side[0][2]}   # the port's own blob offsets (docs/159 §8)

        def rd(sample, c, portside):
            cc = pby.get(c["name"], c) if portside else c
            o = (cc.get("offset") if cc.get("offset") is not None else cc.get("rec_offset")) \
                if portside else cc["ds"] - DS_LO
            if o is None or o + 4 > len(sample):
                return None
            return int.from_bytes(sample[o:o + 4], "little")
        for k, v in sorted(real.items(), key=lambda kv: -len(kv[1][2]))[:8]:
            t0, t1 = phases_dict[k]
            last_o = ob.get(k, [])[-1] if ob.get(k) else None
            ins = [x[1] for x in sorted(other_side, key=lambda x: x[0]) if t0 <= x[0] < t1]
            def line(sample, portside):
                return " ".join(("none" if sample is None else
                                 f"{c['name']}={rd(sample, c, portside)}")
                                for c in (probe.get(0x53c68), probe.get(0x63e14), probe.get(0x639dc)) if c)
            print(f"   {k}: {v[0] / 1000.0:.1f} s quiet, {v[1]} other-side samples, {len(v[2])} cells moving: "
                  f"{', '.join(v[2][:8])}")
            print(f"        shipped carried sample: {line(last_o, False)}")
            if ins:
                print(f"        port first/last in window: {line(ins[0], True)} | {line(ins[-1], True)}")
    poff = {}
    if is_port:
        poff = {c["name"]: c["offset"] for c in other_side[0][2]}
    # docs/134 triage made machine-readable (scripts/audit/gen/oracle_families.json): cells that are
    # DOS-extender or host-address-space bookkeeping get a NAMED verdict instead of DIFF, with their raw
    # value sets still printed, so "not game state" is a claim the reader can check rather than a filter.
    fam = {}
    fpath = os.path.join(GEN, "oracle_families.json")
    if os.path.exists(fpath):
        doc = json.load(open(fpath))
        for fname, spec in doc.items():
            if fname.startswith("_"):
                continue
            for dsx, label in spec["cells"].items():
                fam[int(dsx, 16)] = {"family": fname, "label": label,
                                     "reason": spec["reason"], "docs": spec["docs"]}
    rows = []
    for c in cells:
        ds, gsz, hsz, ptr = c["ds"], c["guest_size"], c["host_size"], c["ptr"]
        gaw = c.get("guest_access_width")
        # With no shipped size for the symbol, compare the port's own width; where the two widths
        # differ, compare the overlap and keep the disagreement as a finding of its own.
        gs = gsz or hsz
        w = min(gs, hsz)
        # docs/132 §4(b): a plain DIFF must mean "same shipped width, same compared width, different
        # value". Where the comparison basis itself is weak -- no shipped width, or the two sides
        # disagree about the width -- a value disagreement is filed as DIFF-SIZE / DIFF-WIDTH so the
        # DIFF count cannot be read as a proven divergence.
        basis = "ok"
        if ds in fam:
            f = fam[ds]
            verdict, note, basis = f"NON-GAMEPLAY:{f['family']}", f"{f['label']} -- {f['reason']}", "family"
        elif any((ds + k) in entropy for k in range(max(gs, hsz))):
            verdict, note = "ENTROPY", "varies across identical replays of the original"
        elif ptr:
            verdict, note = "PTR", f"host slot {hsz}B vs guest {gsz}B (offset-vs-pointer, docs/94 family)"
        elif gsz and gsz != hsz:
            verdict, note, basis = "REPR", f"width {gsz}B guest vs {hsz}B port; compared the first {w}B", "width"
        elif not gsz:
            # No shipped *span* for this cell, but the operand text may still state the element width
            # (gen/ds_join.json guest_access_widths): carry it into the note so the reader can tell a
            # byte field flattened into a 256-byte port array from a genuine value divergence.
            ev = f"; shipped reads/writes move {gaw}B there" if gaw else ""
            verdict, note, basis = "SIZE?", f"no shipped span for this symbol; compared {w}B{ev}", "size"
        else:
            verdict, note = "SAME", ""
        bad_verdict = {"ok": "DIFF", "width": "DIFF-WIDTH", "size": "DIFF-SIZE",
                       "family": verdict}[basis]
        per_phase = []
        if verdict in ("SAME", "REPR", "SIZE?") or basis == "family":
            off_p = poff.get(c["name"])
            for label, _t0, _t1 in phases:
                if o_raw.get(label, 0) == 0:
                    per_phase.append((label, "SHIPPED-QUIET",
                                      f"no shipped sample in "
                                      f"{(phases_dict[label][1]-phases_dict[label][0]) / 1000.0:.1f} s",
                                      f"{p_raw.get(label,0)} other-side samples"))
                    continue
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
                ndiff = sum(1 for i in range(w) if vo[i] != vp[i])
                detail = f"@{hex(ds + at)} {vo[at]:#04x} vs {vp[at]:#04x} ({ndiff}/{w} bytes differ)"
                verdict = bad_verdict
                per_phase.append((label, bad_verdict, hexs(so), hexs(sp)))
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
    bad = sum(1 for r in rows if r["verdict"].startswith("DIFF"))
    hard = sum(1 for r in rows if r["verdict"] == "DIFF")
    fam_rows = [r for r in rows if r["verdict"].startswith("NON-GAMEPLAY:")]
    per_fam = collections.Counter(r["verdict"].split(":", 1)[1] for r in fam_rows)
    print(f"divergent cells: {bad}  of which same-width value divergences: {hard}")
    print(f"non-gameplay families (docs/134, scripts/audit/gen/oracle_families.json): {dict(per_fam)}")
    resid = sorted({(r["ds"], r["name"]) for r in rows if r["verdict"] == "DIFF"})
    print(f"residual same-width gameplay divergences: {len(resid)}")
    for d, n in resid:
        print(f"   {d} {n}")
    shown = 0
    for r in rows:
        if r["verdict"] in ("DIFF", "DIFF-WIDTH", "DIFF-SIZE", "REPR", "PTR", "REC-DIFF", "REC-NULL") \
                and shown < a.max_rows:
            shown += 1
            dph = [p for p in r["phases"] if p[1] != "SAME"]
            print(f'{r["verdict"]:11s} {r["ds"]} {r["name"]:24s} g={r["guest_size"]} h={r["host_size"]} '
                  f'{r["note"]}' + (f'  {"; ".join(f"{p[0]}:{p[1]} orig={p[2]} port={p[3]}" for p in dph[:2])}'
                                    if r["verdict"].startswith("DIFF") else ""))
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
            if r["verdict"] in ("DIFF", "DIFF-WIDTH", "DIFF-SIZE", "HEAP"):
                doc["entropy_cells"][r["ds"]] = {"name": r["name"],
                                                 "reason": "varies across identical replays "
                                                           "(allocation address / timing)"}
        json.dump(doc, open(path, "w"), indent=1)
        print(f"entropy_cells.json now has "
              f"{len(json.load(open(path))['entropy_cells'])} measured-variable cells")

    if mode == "baseline":
        bad = [r for r in rows if r["verdict"].startswith("DIFF")]
        print("FP CONTROL (original vs original):", "PASS" if not bad else f"FAIL ({len(bad)} DIFF)")
        sys.exit(0 if not bad else 1)


if __name__ == "__main__":
    main()
