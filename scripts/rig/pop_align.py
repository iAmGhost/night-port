#!/usr/bin/env python3
"""pop_align.py -- find the first sample where the PORT holds more live entities than the shipped game.

docs/161 §5 quantified the end state (shipped entity count peaks at 6, in 3 samples; the port peaks at 9, with
294 samples above 5) and §7 queued the separating measurement: the two symptoms -- extra live entities, and
camera stop-bounds written by trigger records -- are both outputs of fn_03e20f, so the question is *when* the
populations part company and *what* the extra records are.

Alignment follows docs/158: anchor on a transition (by default the room-script drain DAT_00063e14 -> 67),
never on a level measurement, and rebase the port clock onto the shipped clock before comparing.  Both sides
carry ent00..ent11 record blocks (--ents on the shipped side, the port dumper's own slots), so the records
themselves are readable, not just the count.

Record field offsets are docs/125's constructor layout (packed, deliberately unaligned):
    +0x00 type byte   +0x01 x (dword)   +0x09 y (dword)   +0x17 state (dword)
    +0x29 hp (word)   +0x2b exp ramp (dword)   +0x30 dying flag (byte)
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
ENT_FIELDS = {"x": (0x01, 4), "y": (0x09, 4), "state": (0x17, 4), "hp": (0x29, 2), "flag30": (0x30, 1)}
# An unfilled slot is either the poison byte 0x80 (docs/133) or a run of zeroes; type 0 is a REAL type (the
# player), so "type == 0" must never be read as "empty" -- that mistake briefly hid the player from this census.
EMPTY_TYPES = (0x80, 0xff)


def unfilled(blk):
    return blk[0] in EMPTY_TYPES or blk[0] >= 0x40 or not any(blk[:0x40])


def s32(v):
    return None if v is None else (v - 0x100000000 if v >= 0x80000000 else v)


def resample(side, step):
    """One sample per bucket -- the last state recorded in it.  The shipped dump is change-triggered, so a
    quiet stretch collapses to its last state, which is the correct value to compare (docs/159 §6)."""
    out = {}
    for s in side:
        out[int(s[0] // step)] = s
    return [out[k] for k in sorted(out)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--anchor", default="0x63e14:67")
    ap.add_argument("--step", type=int, default=500)
    a = ap.parse_args()
    ds, val = (int(x, 0) for x in a.anchor.split(":"))

    O, pcl = load_orig(a.orig)
    P, pcells = load_port(a.port)
    names = [c["name"] if isinstance(c, dict) else c for c in pcl]
    cmap = {c["name"]: c for c in pcells}

    # Offsets differ by side: the shipped blob is a contiguous DS window (offset = ds - 0x50000), the port blob
    # is a packed cell arena whose offsets only meta.json knows (docs/159 §8).
    po = cmap.get("g_dat_%05x" % ds, {}).get("offset")
    if po is None:
        sys.exit(f"port dump has no cell for DS {ds:#x}")

    def first(side, off):
        for s in sorted(side, key=lambda x: x[0]):
            src = s[1][off:off + 4]
            if len(src) == 4 and int.from_bytes(src, "little") == val:
                return s[0]
        return None

    t_o, t_p = first(O, ds - DS_LO), first(P, po)
    if t_o is None or t_p is None:
        sys.exit(f"anchor {ds:#x} == {val} not found (orig={t_o}, port={t_p})")
    shift = t_o - t_p
    print(f"anchor {ds:#x} == {val}: shipped t={t_o} ms, port clock shifted by {shift} ms")

    blk_o = next((len(s[2]) // len(names) for s in O if len(s) > 2 and s[2]), 0)
    o_ent = [(i, n) for i, n in enumerate(names) if n.startswith("ent")]
    p_ent = [(n, c) for n, c in cmap.items() if n.startswith("ent")]

    def ds_o(blob, addr):
        o = addr - DS_LO
        return int.from_bytes(blob[o:o + 4], "little")

    def ds_p(blob, name):
        c = cmap.get(name)
        return int.from_bytes(blob[c["offset"]:c["offset"] + 4], "little")

    def blocks_o(heap):
        out = []
        for i, n in o_ent:
            blk = heap[i * blk_o:(i + 1) * blk_o]
            if len(blk) >= 0x40 and not unfilled(blk):
                out.append((n, blk))
        return out

    def blocks_p(blob):
        out = []
        for n, c in p_ent:
            ro = c.get("rec_offset")
            if ro is None or ro + 0x40 > len(blob) or unfilled(blob[ro:ro + 0x40]):
                continue
            out.append((n, blob[ro:ro + c.get("rec_bytes", 0x160)]))
        return out

    def show(tag, rows):
        for n, blk in rows:
            def f(k):
                off, w = ENT_FIELDS[k]
                v = int.from_bytes(blk[off:off + w], "little")
                return s32(v) if w == 4 else v
            print(f"   {tag} {n}: type={blk[0]:#04x} state={f('state'):#x} hp={f('hp')} "
                  f"flag30={f('flag30')} x={f('x')} y={f('y')}")

    ro = resample(O, a.step)
    rp = resample([(t + shift, *rest) for t, *rest in P], a.step)

    first = None
    diffs = buckets = 0
    tot_o = collections.Counter()
    tot_p = collections.Counter()
    n_o = n_p = 0
    i = j = 0
    while i < len(ro) and j < len(rp):
        to, bo = ro[i][0], ro[i][1]
        tp, bp = rp[j][0], rp[j][1]
        if abs(to - tp) > a.step:
            if to < tp:
                i += 1
            else:
                j += 1
            continue
        buckets += 1
        ho = ro[i][2] if len(ro[i]) > 2 else b""
        co, cp = ds_o(bo, 0x53C68), ds_p(bp, "g_dat_53c68")
        bo_rows, bp_rows = blocks_o(ho), blocks_p(bp)
        tot_o.update(b[0] for _, b in bo_rows)
        tot_p.update(b[0] for _, b in bp_rows)
        n_o += len(bo_rows)
        n_p += len(bp_rows)
        if cp > co:
            diffs += 1
            if first is None:
                first = (to, co, cp)
                print(f"\nFIRST population divergence at t={to} ms: shipped count={co} "
                      f"({len(bo_rows)} live records), port count={cp} ({len(bp_rows)} live records)")
                show("PORT", bp_rows)
                show("ORIG", bo_rows)
        i += 1
        j += 1

    print("\n== live-record type census over the aligned route (record-samples, not distinct records)")
    print(f"   ORIG {n_o} live-record samples: {sorted(tot_o.items(), key=lambda kv: -kv[1])}")
    print(f"   PORT {n_p} live-record samples: {sorted(tot_p.items(), key=lambda kv: -kv[1])}")
    if first is None:
        print(f"\nno aligned sample ({buckets} buckets) where the port holds more live entities than shipped")
    else:
        print(f"\nFIRST divergence: t={first[0]} ms shipped={first[1]} port={first[2]}")
        print(f"   aligned buckets where port count > shipped count: {diffs} of {buckets}")


if __name__ == "__main__":
    main()
