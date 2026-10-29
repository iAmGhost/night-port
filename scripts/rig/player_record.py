#!/usr/bin/env python3
"""player_record.py -- the dying-flag (+0x30) census on the PLAYER record, shipped vs port.

docs/136 established the shipped rule from NIGHT.EXE 0x00014cba-0x00014cd9: a victim whose debt word
(+0x23) is 0 and whose hit resolves to a reaction code 0x0b..0x14 is marked dying (+0x30 = 1) no matter
what its hit points say, and the port transcribes that faithfully. The differential question is therefore
not the rule but which hit resolutions reach it -- and the observable that settles it is whether the
shipped player record ever carries a dying flag, and at what hit points.

The player record is identified by shape, not by slot number: a hit-point word at +0x29 at or above --hp
(1000 is the shipped player maximum; docs/125 validated 0x3e8 -> 0x3e5 -> 0x3dd on shipped runtime
values). byte[+0x00] is reported, not required, because the constructor stores several kinds of record.

  python3 scripts/rig/player_record.py --orig /tmp/rig/g10/orig --port /tmp/rig/g10/port [--hp 900]
"""
import argparse
import collections
import os
import sys

RB = 0x160
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E402
from ent_list import FIELDS  # noqa: E402


def u(blk, name):
    o, w = FIELDS[name]
    return int.from_bytes(blk[o:o + w], "little")


def ent_layout(cells):
    """(name, offset, rec_bytes) for every ent## slot, using the dump's own offsets where it has
    them and ent_list.py's documented fallback where it does not."""
    base = []
    run = 0
    for c in cells:
        off = c.get("offset")
        base.append(run if off is None else off)
        run = base[-1] + 0  # the running sum is only used for the all-None (shipped) layout
    # shipped dumps carry no per-cell offset: the record area is the concatenation of rec_bytes
    run, out = 0, []
    for i, c in enumerate(cells):
        off = c.get("offset", run)
        rb = c.get("rec_bytes", RB)
        if off is None:
            off = run
        if str(c["name"]).startswith("ent"):
            out.append((c["name"], off, rb))
        run = off + rb
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--hp", type=int, default=900)
    a = ap.parse_args()

    O, oc = load_orig(a.orig)
    P, pc = load_port(a.port)

    for label, side, cells, hidx in (("orig", O, oc, 2), ("port", P, pc, 1)):
        ec = ent_layout(cells)
        if not ec:
            print(f"{label}: no ent## slots"); continue
        n = 0
        b0 = collections.Counter()
        deadc = collections.Counter()
        states = collections.Counter()
        hpc = collections.Counter()
        flagged, minhp, first = [], 1 << 30, None
        for s in side:
            t, hb = s[0], s[hidx]
            for name, off, rb in ec:
                blk = hb[off:off + (rb or RB)]
                if len(blk) < RB or not any(blk):
                    continue
                hp = u(blk, "hp29")
                if hp < a.hp:
                    continue
                n += 1
                minhp = min(minhp, hp)
                b0[u(blk, "type")] += 1
                hpc[hp] += 1
                d = u(blk, "dead30")
                deadc[d] += 1
                states[u(blk, "state")] += 1
                if d:
                    first = t if first is None else first
                    if len(flagged) < 8:
                        flagged.append((t, name, hex(u(blk, "state")), hp, u(blk, "hp23"), d,
                                        hex(u(blk, "exp32")), b0 and ""))
        print(f"{label}: records with hp(+0x29) >= {a.hp}: {n}   min hp={minhp}   "
              f"flagged={sum(v for k, v in deadc.items() if k)}   first flag t={first}")
        print(f"   byte[+0x00] census: {b0.most_common(6)}")
        print(f"   +0x30 census: {dict(sorted(deadc.items()))}")
        print(f"   hp census: {hpc.most_common(8)}")
        print(f"   state census: {states.most_common(12)}")
        for r in flagged:
            print(f"   FLAGGED while alive: t={r[0]} {r[1]} state={r[2]} hp29={r[3]} debt+0x23={r[4]} "
                  f"+0x30={r[5]} +0x32={r[6]}")


if __name__ == "__main__":
    main()
