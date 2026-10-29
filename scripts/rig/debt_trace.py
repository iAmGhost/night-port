#!/usr/bin/env python3
"""debt_trace.py -- the player's damage bookkeeping words, side by side, on one clock.

docs/162 §3/§8 narrowed the damage question to a single one: fn_01441e never writes a POSITIVE debt, and
fn_0137c1 only ever INCREMENTS it, so only two things can bring the player's debt (`+0x23`, a word) back to
zero -- the settle at guest 0x14984 (`hp += debt; debt = 0; byte [+0x30] = 2`) and the arming clear at guest
0x37756 (which zeroes the debt and puts 100 into the accumulator `+0x56` instead).  The shipped player shows a
zero debt in 70 % of its live samples and the port's in 17 %, so one of those two runs far more often on one
side than the other.

This script reads the four words straight out of the player record block on both sides -- the shipped dump has
a `g_player_ent` pointer cell with its own fixed-size block, the port dump has a rec_offset -- and prints, per
aligned bucket:

    +0x23 debt (signed word)   +0x56 accumulator (signed dword)
    +0x29 hp (signed word)     +0x30 flag

aligned on a TRANSITION (docs/158: DAT_00063e14 -> 67, the room-script drain), never on a level measurement.
Transitions where the debt reaches <= 0 are settle candidates; transitions where a positive debt appears out of
a zero (or out of the accumulator) are arming candidates.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
W = {"debt": (0x23, 2), "acc": (0x56, 4), "hp": (0x29, 2), "flag": (0x30, 1), "state": (0x17, 4)}


def signed(v, w):
    if v is None:
        return None
    return v - (1 << (8 * w)) if v >= (1 << (8 * w - 1)) else v


def field(blk, key):
    off, w = W[key]
    if off + w > len(blk):
        return None
    v = int.from_bytes(blk[off:off + w], "little")
    return signed(v, w) if w > 1 else v


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--anchor", default="0x63e14:67")
    ap.add_argument("--step", type=int, default=250)
    ap.add_argument("--transitions", type=int, default=40)
    a = ap.parse_args()
    ds, val = (int(x, 0) for x in a.anchor.split(":"))

    O, pcl = load_orig(a.orig)
    P, pcells = load_port(a.port)
    names = [c["name"] if isinstance(c, dict) else c for c in pcl]
    cmap = {c["name"]: c for c in pcells}
    po = cmap.get("g_dat_%05x" % ds, {}).get("offset")
    if po is None:
        sys.exit("port dump has no cell for the anchor")

    def first(side, off):
        for s in sorted(side, key=lambda x: x[0]):
            if int.from_bytes(s[1][off:off + 4], "little") == val:
                return s[0]
        return None

    t_o, t_p = first(O, ds - DS_LO), first(P, po)
    if t_o is None or t_p is None:
        sys.exit("anchor not found on one side")
    shift = t_o - t_p
    print(f"anchor {ds:#x} == {val}: shipped t={t_o} ms, port clock shifted by {shift} ms")

    oi = names.index("g_player_ent")
    blk = next((len(s[2]) // len(names) for s in O if len(s) > 2 and s[2]), 0)
    pr = cmap["g_player_ent"]
    ro, rb = pr.get("rec_offset"), pr.get("rec_bytes", 0x160)
    if ro is None:
        sys.exit("port dump has no player record block")

    def bucket(side, pick):
        out = {}
        for s in side:
            blk_ = pick(s)
            if blk_ is None:
                continue
            out[int(s[0] // a.step)] = (s[0], blk_)
        return out

    ob = bucket(sorted(O, key=lambda x: x[0]),
                lambda s: s[2][oi * blk:(oi + 1) * blk] if len(s) > 2 and s[2] else None)
    pb = bucket(sorted([(t + shift, *rest) for t, *rest in P], key=lambda x: x[0]),
                lambda s: s[1][ro:ro + rb])

    keys = sorted(set(ob) & set(pb))
    stats = {"ORIG": collections.Counter(), "PORT": collections.Counter()}
    events = {"ORIG": [], "PORT": []}
    prev = {"ORIG": None, "PORT": None}
    for k in keys:
        to, bo = ob[k]
        tp, bp = pb[k]
        if abs(to - tp) > a.step:
            continue
        for tag, blk_, t in (("ORIG", bo, to), ("PORT", bp, tp)):
            cur = {key: field(blk_, key) for key in W}
            if cur["flag"] is None:
                continue
            stats[tag]["samples"] += 1
            stats[tag]["debt>0"] += 1 if (cur["debt"] or 0) > 0 else 0
            stats[tag]["debt<=0"] += 1 if (cur["debt"] or 0) <= 0 else 0
            stats[tag]["acc>0"] += 1 if (cur["acc"] or 0) > 0 else 0
            stats[tag][f"flag{cur['flag']}"] += 1
            pv = prev[tag]
            if pv is not None:
                if pv["debt"] > 0 and cur["debt"] <= 0:
                    events[tag].append(("SETTLE", t, pv["debt"], cur["debt"], pv["hp"], cur["hp"]))
                if pv["debt"] <= 0 and cur["debt"] > 0:
                    events[tag].append(("ARMED", t, pv["debt"], cur["debt"], pv["acc"], cur["acc"]))
                if pv["hp"] != cur["hp"]:
                    events[tag].append(("HP", t, pv["hp"], cur["hp"], pv["debt"], cur["debt"]))
                if (pv["acc"] or 0) <= 0 and (cur["acc"] or 0) > 0:
                    # a re-arm of the accumulator (guest 0x377a1 writes 100 into +0x56).  Each re-arm restarts
                    # fn_0137c1's ramp and re-shields the player, so the COUNT of re-arms is the quantity docs/162
                    # §12 queued.  At 250 ms a full drain-and-rearm can hide inside one bucket, so this is a
                    # lower bound, and it is stated as one.
                    events[tag].append(("ACC_REARM", t, pv["acc"], cur["acc"], pv["debt"], cur["debt"]))
            if cur["hp"] is not None and pv is not None and cur["hp"] < pv["hp"]:
                d = pv["hp"] - cur["hp"]
                # hp is a signed word and the record block can be a freed/garbage block (docs/128/161 found the
                # player pointer reading a block with x ~ -1.4e9); an implausible drop is a BAD READ, not damage.
                if d > 2000:
                    stats[tag]["bad_reads"] += 1
                else:
                    stats[tag]["hp_lost"] += d
            prev[tag] = cur

    for tag in ("ORIG", "PORT"):
        print(f"\n== {tag} player record: {dict(stats[tag])}")
        kinds = collections.Counter(e[0] for e in events[tag])
        print(f"   transition events: {dict(kinds)}")
        for e in events[tag][:a.transitions]:
            kind, t, a0, a1, b0, b1 = e
            # event tuples carry (a0,a1) = debt or hp pair and (b0,b1) = the paired field; label each kind
            # explicitly so the two columns cannot be read swapped (docs/162 §9 caught exactly that here).
            if kind == "ACC_REARM":
                print(f"   {kind:10s} t={t:>7} acc {a0} -> {a1} (debt {b0} -> {b1})")
            elif kind == "HP":
                print(f"   {kind:7s} t={t:>7} debt {a0} -> {a1} (hp {b0} -> {b1})")
            elif kind == "SETTLE":
                print(f"   {kind:7s} t={t:>7} debt {a0} -> {a1} (hp {b0} -> {b1})")
            else:
                print(f"   {kind:7s} t={t:>7} debt {a0} -> {a1} (acc {b0} -> {b1})")


if __name__ == "__main__":
    main()
