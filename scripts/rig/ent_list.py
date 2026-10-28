#!/usr/bin/env python3
"""ent_list.py -- compare the ENTITY LIST slot by slot, shipped vs port (docs/108).

The DS cell table only reaches the list head (DAT_00053c70) and tail (DAT_00053c74), so the state
oracle could never see the entities that actually matter: the one that gets hit, the one that dies,
the one that awards EXP, the one that appears to teleport. Both dumpers now follow the +0x13f "next"
links and store up to N records per sample in list order; this script pairs the two sides by nominal
time and reports, per slot:

  * the state field (+0x17) census, with the hurt band the reaction table produces (0x0b..0x1e) called
    out separately -- that is the hurt/stagger / fallen / stuck-when-hit signature;
  * position continuity: |dx| per sample step for the same slot, with jumps beyond --jump quoted,
    which is the "enemy teleporting" item measured instead of eyeballed;
  * the EXP-award gate tuple of fn_015295 (byte0==1, +0x30==1, +0x25!=0xff, +0x05==0, +0x11>0x15),
    counted per side so a missing award is attributed to the gate that fails;
  * the hit-point words +0x23/+0x29 and the EXP value field +0x32.

  python3 scripts/rig/ent_list.py --orig /tmp/rig/ents-orig/orig --port /tmp/rig/ents-port \
      --from 150000 --to 300000
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff_state import load_orig, load_port  # noqa: E402

RB = 0x160
FIELDS = {
    "type": (0x00, 1), "x": (0x01, 4), "y": (0x09, 4), "cnt5": (0x05, 4), "slot": (0x11, 4),
    "state": (0x17, 4), "face": (0x1b, 4), "b16": (0x16, 1), "hp23": (0x23, 2), "hp29": (0x29, 2),
    "dead30": (0x30, 1), "b25": (0x25, 1), "exp32": (0x32, 4), "mv3e": (0x3e, 4), "knock42": (0x42, 4),
    "score52": (0x52, 4), "exp2b": (0x2b, 4),
}


def g(blk, name):
    o, w = FIELDS[name]
    return int.from_bytes(blk[o:o + w], "little", signed=(w == 4 and name in ("cnt5", "slot")))


def hurt(v):
    return 0x0b <= v <= 0x1e


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--from", dest="t_from", type=int, default=0)
    ap.add_argument("--to", dest="t_to", type=int, default=10 ** 9)
    ap.add_argument("--tol", type=int, default=300)
    ap.add_argument("--jump", type=int, default=12)
    a = ap.parse_args()

    O, oc = load_orig(a.orig)
    P, pc = load_port(a.port)
    def slots(cells):
        return [c for c in cells if str(c["name"]).startswith("ent")]
    so, sp = slots(oc), slots(pc)
    if not so or not sp:
        sys.exit("no ent## slots in one of the dumps -- re-dump with --ents / the rebuilt port binary")
    def base(cells, i):
        return sum(x.get("rec_bytes", 0) for x in cells[:i])
    ob = {i: base(oc, i) for i in range(len(oc))}
    pb = {i: base(pc, i) for i in range(len(pc))}
    ent_o = [(i, c) for i, c in enumerate(oc) if str(c["name"]).startswith("ent")]
    ent_p = [(i, c) for i, c in enumerate(pc) if str(c["name"]).startswith("ent")]
    print(f"slots orig={len(ent_o)} port={len(ent_p)}  window=[{a.t_from},{a.t_to}]")

    pt = [s[0] for s in P]
    import bisect
    live_o = live_p = 0
    st_o, st_p = {}, {}
    jump_o, jump_p = [], []
    gate_o = {"pass": 0, "fail": {}}
    gate_p = {"pass": 0, "fail": {}}
    prev_o, prev_p = {}, {}
    for t, ds, hb in O:
        if not (a.t_from <= t <= a.t_to):
            continue
        j = bisect.bisect_left(pt, t)
        cands = [k for k in (j - 1, j) if 0 <= k < len(P)]
        if not cands:
            continue
        k = min(cands, key=lambda k: abs(pt[k] - t))
        if abs(pt[k] - t) > a.tol:
            continue
        tblob = P[k][1]
        for n, (i, c) in enumerate(ent_o):
            blk = hb[ob[i]:ob[i] + c.get("rec_bytes", RB)]
            if len(blk) < RB or not any(blk):
                continue
            live_o += 1
            s = g(blk, "state")
            st_o[s] = st_o.get(s, 0) + 1
            x, y = g(blk, "x"), g(blk, "y")
            pk = prev_o.get(n)
            if pk and abs(x - pk[0]) + abs(y - pk[1]) > a.jump:
                jump_o.append((t, n, pk[0], x, pk[1], y, pk[2], s))
            prev_o[n] = (x, y, s)
            why = []
            if g(blk, "type") != 1: why.append("type")
            if g(blk, "dead30") != 1: why.append("+0x30")
            if g(blk, "b25") == 0xff: why.append("+0x25=ff")
            if g(blk, "cnt5") != 0: why.append("+0x05")
            if g(blk, "slot") <= 0x15: why.append("+0x11")
            if why:
                gate_o["fail"][why[0]] = gate_o["fail"].get(why[0], 0) + 1
            else:
                gate_o["pass"] += 1
        for n, (i, c) in enumerate(ent_p):
            blk = tblob[c["offset"]:c["offset"] + c.get("rec_bytes", RB)]
            if len(blk) < RB or not any(blk):
                continue
            live_p += 1
            s = g(blk, "state")
            st_p[s] = st_p.get(s, 0) + 1
            x, y = g(blk, "x"), g(blk, "y")
            pk = prev_p.get(n)
            if pk and abs(x - pk[0]) + abs(y - pk[1]) > a.jump:
                jump_p.append((t, n, pk[0], x, pk[1], y, pk[2], s))
            prev_p[n] = (x, y, s)
            why = []
            if g(blk, "type") != 1: why.append("type")
            if g(blk, "dead30") != 1: why.append("+0x30")
            if g(blk, "b25") == 0xff: why.append("+0x25=ff")
            if g(blk, "cnt5") != 0: why.append("+0x05")
            if g(blk, "slot") <= 0x15: why.append("+0x11")
            if why:
                gate_p["fail"][why[0]] = gate_p["fail"].get(why[0], 0) + 1
            else:
                gate_p["pass"] += 1

    ho = sum(v for k, v in st_o.items() if hurt(k))
    hp_ = sum(v for k, v in st_p.items() if hurt(k))
    print(f"entity samples: orig={live_o} port={live_p}")
    print(f"HURT-band (+0x17 in 0x0b..0x1e) samples: orig={ho} port={hp_}"
          + ("   <- missing hurt/stagger/fallen" if (ho and hp_ * 4 < ho) else ""))
    print("orig states:", sorted(st_o.items(), key=lambda kv: -kv[1])[:10])
    print("port states:", sorted(st_p.items(), key=lambda kv: -kv[1])[:10])
    print(f"position jumps >{a.jump}px: orig={len(jump_o)} port={len(jump_p)}"
          + ("   <- teleport-like" if len(jump_p) > 2 * max(1, len(jump_o)) else ""))
    for lbl, j in (("orig", jump_o[:4]), ("port", jump_p[:4])):
        for r in j:
            print(f"   {lbl} t={r[0]} slot={r[1]} x {r[2]}->{r[3]} y {r[4]}->{r[5]} state {r[6]:#x}->{r[7]:#x}")
    print(f"EXP-award gates (fn_015295): orig pass={gate_o['pass']} fail={gate_o['fail']}")
    print(f"                              port pass={gate_p['pass']} fail={gate_p['fail']}")


if __name__ == "__main__":
    main()
