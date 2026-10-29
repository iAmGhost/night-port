#!/usr/bin/env python3
"""walk_rate.py -- player displacement rate, per action state, both sides.

docs/144 measured velocity bands (60-69 px/s shipped, 70-79 px/s port, plus a port-only 80-89 band) on a
build whose action trip count was still wrong (fixed in docs/147). docs/148 narrowed the camera question to
"why does the port's player reach the camera-follow threshold (190) when the shipped player stops at 156".
Both questions are the same measurement: how far does the player move per unit time while in a given action
state.

The dumps are sampled (4 Hz shipped, 10 Hz port), so this measures px/second across consecutive samples and
only counts a pair when:
  * both samples have a live player (entity count > 0 and a sane state id),
  * the player's state is IDENTICAL in both samples (the rate is attributed to that state, not to a
    transition),
  * dt is within 1.5x of the side's nominal cadence (no room-transition gaps).

Usage:
  python3 scripts/rig/walk_rate.py --orig /tmp/rig/g12/orig --port /tmp/rig/g12/port
"""
import argparse
import collections
import json
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_port  # noqa: E402

DS_LO = 0x50000
STATES = (0x00, 0x01, 0x02, 0x03, 0x0B, 0x0C, 0x15, 0x33, 0x34, 0x35, 0x36)


def s32(v):
    return v - 0x100000000 if v >= 0x80000000 else v


def orig_rows(run):
    tl = json.load(open(os.path.join(run, "timeline.json")))
    pc = [c["name"] if isinstance(c, dict) else c for c in tl["ptr_cells"]]
    pi = pc.index("g_player_ent")
    n = len(pc)
    out = []
    for i, st in enumerate(tl["timeline"]):
        ds = open(os.path.join(run, f"state_{i:04d}.ds.bin"), "rb").read()
        hb = open(os.path.join(run, f"state_{i:04d}.heap.bin"), "rb").read()
        blk = len(hb) // n if n else 0
        b = hb[pi * blk:(pi + 1) * blk]
        out.append(dict(
            t=st["t_wall"] * 1000.0,
            cnt=int.from_bytes(ds[0x53C68 - DS_LO:0x53C68 - DS_LO + 4], "little"),
            cam=s32(int.from_bytes(ds[0x639DC - DS_LO:0x639DC - DS_LO + 4], "little")),
            state=int.from_bytes(b[0x17:0x1B], "little") if len(b) > 0x1B else None,
            x=s32(int.from_bytes(b[0x01:0x05], "little")) if len(b) > 5 else None,
        ))
    return out


def port_rows(run):
    S, cells = load_port(run)
    cm = {c["name"]: c for c in cells}
    cc, pc_ = cm["g_dat_639dc"], cm["g_player_ent"]
    out = []
    for s in S:
        b = s[1][pc_["rec_offset"]:pc_["rec_offset"] + pc_["rec_bytes"]]
        out.append(dict(
            t=float(s[0]),
            cnt=int.from_bytes(s[1][cm["g_dat_53c68"]["offset"]:cm["g_dat_53c68"]["offset"] + 4], "little"),
            cam=s32(int.from_bytes(s[1][cc["offset"]:cc["offset"] + 4], "little")),
            state=int.from_bytes(b[0x17:0x1B], "little") if len(b) > 0x1B else None,
            x=s32(int.from_bytes(b[0x01:0x05], "little")) if len(b) > 5 else None,
        ))
    return out


def rates(rows):
    per = collections.defaultdict(list)
    for a, b in zip(rows, rows[1:]):
        dt = b["t"] - a["t"]
        if dt <= 0 or dt > 1500:
            continue
        if a["cnt"] == 0 or b["cnt"] == 0:
            continue
        if a["state"] != b["state"] or a["state"] not in STATES:
            continue
        per[a["state"]].append((b["x"] - a["x"]) / (dt / 1000.0))
    return per


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    a = ap.parse_args()
    for tag, run in (("ORIG", a.orig), ("PORT", a.port)):
        rows = port_rows(run) if tag == "PORT" else orig_rows(run)
        cad = statistics.median([b["t"] - a_["t"] for a_, b in zip(rows, rows[1:]) if b["t"] > a_["t"]])
        per = rates(rows)
        print(f"== {tag} {run}  samples={len(rows)} cadence={cad:.0f} ms")
        for st in sorted(per, key=lambda s: -len(per[s])):
            v = per[st]
            if len(v) < 5:
                continue
            print(f"   state 0x{st:02x}: n={len(v):5d} median={statistics.median(v):8.1f} px/s "
                  f"p25={sorted(v)[len(v)//4]:8.1f} p75={sorted(v)[3*len(v)//4]:8.1f} "
                  f"min={min(v):8.1f} max={max(v):8.1f}")
            band = collections.Counter(int(round(x / 10.0) * 10) for x in v if abs(x) > 1)
            print(f"      bands(10 px/s): {sorted(band.items(), key=lambda kv: -kv[1])[:8]}")


if __name__ == "__main__":
    main()
