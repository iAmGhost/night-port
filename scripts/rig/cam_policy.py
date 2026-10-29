#!/usr/bin/env python3
"""cam_policy.py -- measure the camera policy inputs (fn_03d4e4) on both sides.

The shipped camera policy step (guest 0x0003d4e4) decides per frame whether the camera may scroll,
then hands the move to fn_03cb38 (right) / fn_03cbd9 (left), each of which clamps against the subject
record and calls fn_03d75c(delta) when the camera actually moved.  docs/142 measured the symptom
(shipped DAT_000639dc never leaves 27, the port reaches 298, 158 after docs/147) and docs/143 left the
question of WHICH input differs open.  This script reads every input of that decision straight out of
the two dumps and evaluates the shipped gate per sample, so the divergence is attributed to a named
field rather than to "the camera".

Inputs, all from the dumps (no guest instrumentation needed):
  DAT_000639dc / DAT_000639e0   camera x, y
  DAT_000639e4                  the byte gate inside both scroll helpers (0x3cb5d, 0x3cbf6)
  DAT_00053c68                  the entity-count escape used by both policy arms
  [DAT_00053c7c] + 0x17         player state, the ==3 fast-scroll arm (0x3cb6b, 0x3cbf9)
  [DAT_00053c7c] + 0x01 / +0x09 player x, y (0x3d512, 0x3d...)
  [DAT_00063354] record         the room/clamp block: +0x00 x_lo, +0x08 x_hi,
                                +0x2a/+0x2e stop bounds, +0x32/+0x36 second pair,
                                +0x3a/+0x3b disable bytes

Usage:
  python3 scripts/rig/cam_policy.py --orig /tmp/rig/g10/orig --port /tmp/rig/c1
"""
import argparse
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

DS_LO = 0x50000
FIELDS = [0x00, 0x08, 0x2a, 0x2e, 0x32, 0x36]
BYTES = [0x3a, 0x3b]   # the disable flags are byte-sized in the shipped tests (0x3d4ff, 0x3d576)


def u32(b, o):
    return int.from_bytes(b[o:o + 4], "little") if o + 4 <= len(b) else None


def s32(v):
    """The shipped comparisons are signed dword comparisons; a raw unsigned read makes -30 look like
    4294967266 and opens gates the guest never opens."""
    if v is None:
        return None
    return v - 0x100000000 if v >= 0x80000000 else v


def u8(b, o):
    return b[o] if o < len(b) else None


def orig_samples(run):
    tl = json.load(open(os.path.join(run, "timeline.json")))
    pc = [c["name"] if isinstance(c, dict) else c for c in (tl.get("ptr_cells") or [])]
    idx = {n: i for i, n in enumerate(pc)}
    nblocks = len(pc)
    out = []
    for i, st in enumerate(tl["timeline"]):
        ds = open(os.path.join(run, f"state_{i:04d}.ds.bin"), "rb").read()
        hb = open(os.path.join(run, f"state_{i:04d}.heap.bin"), "rb").read()
        blk = len(hb) // nblocks if nblocks else 0
        out.append((ds, hb, idx, blk))
    return out, idx, blk


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    a = ap.parse_args()

    # ---- shipped side -------------------------------------------------------
    osamp, oidx, oblk = orig_samples(a.orig)
    tl = json.load(open(os.path.join(a.orig, "timeline.json")))
    pc = [c["name"] if isinstance(c, dict) else c for c in tl["ptr_cells"]]

    def pick(name):
        return pc.index(name) if name in pc else None

    rows_o = []
    for ds, hb, idx, blk in osamp:
        def dso(ds_addr):
            o = ds_addr - DS_LO
            return u32(ds, o)
        pblk = hb[oidx["g_player_ent"] * blk:(oidx["g_player_ent"] + 1) * blk] \
            if pick("g_player_ent") is not None else b""
        rblk = hb[oidx["g_dat_63354"] * blk:(oidx["g_dat_63354"] + 1) * blk] \
            if pick("g_dat_63354") is not None else b""
        rows_o.append(dict(
            cam=dso(0x639DC), camy=dso(0x639E0), gate639e4=u8(ds, 0x639E4 - DS_LO),
            cnt=dso(0x53C68),
            pstate=u32(pblk, 0x17), px=u32(pblk, 0x01), py=u32(pblk, 0x09),
            room={f: u32(rblk, f) for f in FIELDS},
            room_b={f: u8(rblk, f) for f in BYTES},
        ))

    # ---- port side ----------------------------------------------------------
    S, cells = load_port(a.port)
    cmap = {c["name"]: c for c in cells}

    rows_p = []
    for s in S:
        blob = s[1]

        def dso(ds_addr):
            c = cmap.get("g_dat_%05x" % ds_addr)
            return u32(blob, c["offset"]) if c else None

        pc_ = cmap.get("g_player_ent")
        rc_ = cmap.get("g_dat_63354")
        pblk = blob[pc_["rec_offset"]:pc_["rec_offset"] + pc_["rec_bytes"]] if pc_ else b""
        rblk = blob[rc_["rec_offset"]:rc_["rec_offset"] + rc_["rec_bytes"]] if rc_ else b""
        rows_p.append(dict(
            cam=dso(0x639DC), camy=dso(0x639E0), gate639e4=u8(blob, cmap["g_dat_639e4"]["offset"]),
            cnt=dso(0x53C68),
            pstate=u32(pblk, 0x17), px=u32(pblk, 0x01), py=u32(pblk, 0x09),
            room={f: u32(rblk, f) for f in FIELDS},
            room_b={f: u8(rblk, f) for f in BYTES},
        ))

    # ---- census -------------------------------------------------------------
    def census(rows, title):
        print(f"== {title}: {len(rows)} samples")
        if not rows:
            return
        def cnt(k, key=None):
            c = collections.Counter()
            for r in rows:
                v = r[k] if key is None else r[k][key]
                c[v] += 1
            return c.most_common(6)
        print("   cam 0x639dc      :", cnt("cam"), "max", max((r["cam"] or 0) for r in rows))
        print("   camy 0x639e0     :", cnt("camy"))
        print("   gate 0x639e4     :", cnt("gate639e4"))
        print("   count 0x53c68    :", cnt("cnt"))
        print("   player state+0x17:", cnt("pstate"))
        print("   player x +0x01   :", "min", min(r["px"] or 0 for r in rows),
              "max", max(r["px"] or 0 for r in rows))
        for f in FIELDS:
            c = collections.Counter(r["room"][f] for r in rows)
            print(f"   room +0x{f:02x}      :", c.most_common(5))
        for f in BYTES:
            c = collections.Counter(r["room_b"][f] for r in rows)
            print(f"   room b+0x{f:02x}     :", c.most_common(5))
        print("   no-entity samples :", sum(1 for r in rows if r["cnt"] == 0), "/", len(rows))

    census(rows_o, "ORIG")
    print()
    census(rows_p, "PORT")

    # ---- gate evaluation, in shipped form ----------------------------------
    def gates(rows, title):
        agg = collections.Counter()
        prev = None
        moves = collections.Counter()
        for r in rows:
            cam, px = s32(r["cam"]), s32(r["px"])
            if cam is None or px is None:
                continue
            rm = r["room"]
            right_open = (r["room_b"].get(0x3b) == 0 and px - cam > 0xbe)
            left_open = (r["room_b"].get(0x3a) == 0 and px - cam < 0x82)
            # second veto layer: the +0x2e / +0x2a stop bounds
            right_veto = right_open and rm.get(0x2e) not in (None, 0) and cam + 0x140 > s32(rm[0x2e])
            left_veto = left_open and rm.get(0x2a) not in (None, 0) and cam < s32(rm[0x2a])
            agg["right_open"] += 1 if right_open else 0
            agg["left_open"] += 1 if left_open else 0
            agg["right_veto_by_2e"] += 1 if right_veto else 0
            agg["left_veto_by_2a"] += 1 if left_veto else 0
            agg["gate639e4_blocks"] += 1 if r["gate639e4"] not in (0, None) else 0
            agg["state3_samples"] += 1 if r["pstate"] == 3 else 0
            if prev is not None and cam != prev:
                moves[cam - prev] += 1
            prev = cam
        print(f"\n== {title} gate census")
        print("   ", dict(agg))
        print("    camera move deltas:", moves.most_common(8))

    gates(rows_o, "ORIG")
    gates(rows_p, "PORT")


if __name__ == "__main__":
    main()
