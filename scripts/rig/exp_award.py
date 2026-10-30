#!/usr/bin/env python3
"""Census the award field of records that reach the death site, shipped vs port (docs/198).

fn_015295's award is `player[+0x52] += victim[+0x32]`, gated on the victim's kind byte (+0x00) == 1 and
its settle byte (+0x30) == 1.  This walks ONLY the entity-list record blocks both dumpers store
(ent00..ent11 and any cell whose record snapshot is 0x160 bytes) and reports what the dying records
actually carry in the award field -- the only way to see whether EXP can differ per kind.

  python3 scripts/rig/exp_award.py --orig RUN/orig --port RUN/port [--from ms --to ms]
"""
import argparse
import os
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E401

RB = 0x160


def u(blk, off, w):
    return int.from_bytes(blk[off:off + w], "little")


def orig_records(sample, cells):
    _t, _ds, heap = sample
    off = 0
    for c in cells:
        n = c.get("rec_bytes") or 0
        blk = heap[off:off + n]
        off += n
        if n == RB:
            yield blk


def port_records(sample, cells):
    t, blob = sample[0], sample[1]
    for c in cells:
        n = c.get("host_size") or 0
        if n == RB:
            yield blob[c["offset"]:c["offset"] + n]


def census(rows, lo, hi, label):
    dead = Counter()
    kinds = Counter()
    per_kind = defaultdict(Counter)
    nrec = 0
    for t, recs in rows:
        if not (lo <= t <= hi):
            continue
        for rec in recs:
            if len(rec) < RB:
                continue
            kind = rec[0]
            state = u(rec, 0x17, 4)
            if kind >= 0x40 or state >= 0x1000:      # entity gate (docs/186 §1)
                continue
            exp = u(rec, 0x32, 4)
            hp = u(rec, 0x29, 2)
            tidx = u(rec, 0x0d, 4)
            nrec += 1
            kinds[kind] += 1
            per_kind[tidx][exp] += 1
            if rec[0x30] == 1:
                dead[(kind, tidx, exp, state, hp)] += 1
    print(f"== {label}: admissible record-samples={nrec}  kinds={dict(kinds.most_common(6))}")
    print(f"   dying records (+0x30==1): {sum(dead.values())}")
    for k, c in list(dead.most_common(8)):
        print(f"     kind={k[0]} type_idx={k[1]} award(+0x32)={k[2]} state={k[3]:#x} hp={k[4]} x{c}")
    print("   award-field (+0x32) values seen per TYPE INDEX (+0x0d), i.e. fn_01441e's table row:")
    for k in sorted(per_kind):
        vals = sorted(per_kind[k])
        print(f"     type_idx={k} ({k:#x}): {vals[:10]}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--from", dest="lo", type=int, default=0)
    ap.add_argument("--to", dest="hi", type=int, default=10 ** 9)
    a = ap.parse_args()
    osys, ocells = load_orig(a.orig)
    census([(s[0], list(orig_records(s, ocells))) for s in osys], a.lo, a.hi, "shipped")
    psys, pcells = load_port(a.port)
    census([(s[0], list(port_records(s, pcells))) for s in psys], a.lo, a.hi, "port")


if __name__ == "__main__":
    main()
