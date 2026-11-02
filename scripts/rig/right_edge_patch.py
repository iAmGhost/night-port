#!/usr/bin/env python3
"""Bottom-right corner patch census (docs/301).

Why this exists: the playtest report (docs/manual-playtest/04-2026-11-02.md item 5) says that walking the
player to the room's LEFT edge puts a piece of the player's own sprite at the RIGHT edge of the screen.
Every item-6 census this rig ran before that report measured ink relative to the PLAYER'S COLUMN, so the
screen's right edge was never looked at.  A first render of a port witness frame (t=104100 ms of
/tmp/rig/le8, player pinned at the left bound mid-attack) shows the artifact: a small isolated object at
the far bottom-right, on empty floor.

A plain "non-black ink" census is useless on this route: every edge column of every frame is 90/90
non-black because the room is bright everywhere (measured on /tmp/rig/do4: right-edge per-column mean = 90.0
in the left-bound phase, the mid phase and everywhere else -- an instrument that reads max everywhere is
broken, not a finding, docs/289 §4).  So this script measures LOCAL COLOR DIVERSITY: in a small corner
patch, how many pixels deviate from that patch's own modal colour, and how many distinct colours appear.
An isolated sprite fragment raises both; a stretch of flat floor does not.

Absolute colour across the two dumps stays inadmissible (docs/287); counts of local deviation are what is
quoted here.

  python3 scripts/rig/right_edge_patch.py /tmp/rig/le8
"""
import argparse
import json
import os
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_screen import read_frames


def patch_metrics(path, w, h, cols, rows, expect_bytes):
    d = open(path, "rb").read()
    off = 0
    if len(d) != expect_bytes:
        if len(d) == expect_bytes + 15:
            off = 15
        else:
            return None
    px = []
    for y in rows:
        base = off + y * w * 4
        for x in cols:
            p = base + x * 4
            px.append((d[p], d[p + 1], d[p + 2]))
    if not px:
        return None
    modal = Counter(px).most_common(1)[0][0]
    dev = sum(1 for c in px if c != modal)
    return dev, len(set(px)), len(px)


def census(dump, side, windows, n, ylo, yhi):
    rel = "orig_screen" if side == "orig" else "port"
    idx_path = os.path.join(dump, rel, "frames.bin")
    if not os.path.exists(idx_path):
        return {}
    frames = []
    for f in read_frames(idx_path):
        p = os.path.join(dump, rel, "frames", f"{f['frame']:06d}_{f['w']}x{f['h']}.bin")
        if os.path.exists(p):
            frames.append((f["t"], p, f["w"], f["h"]))
    out = {}
    for name, (t0, t1) in windows.items():
        sel = [f for f in frames if t0 <= f[0] <= t1]
        devs, cols_seen, nf, tot = [], set(), 0, 0
        for t, p, w, h in sel:
            cols = list(range(w - n, w))
            rows = range(int(h * ylo), int(h * yhi))
            m = patch_metrics(p, w, h, cols, rows, w * h * 4)
            if m is None:
                continue
            nf += 1
            devs.append(m[0])
            tot += m[1]
            cols_seen.add(w)
        if not nf:
            continue
        devs.sort()
        out[name] = dict(frames=nf,
                         dev_median=devs[len(devs) // 2],
                         dev_p90=devs[int(len(devs) * 0.9)],
                         dev_max=devs[-1],
                         frames_with_dev=sum(1 for v in devs if v > 40),
                         mean_distinct_cols=tot / nf)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--n", type=int, default=26, help="patch width in columns")
    ap.add_argument("--ylo", type=float, default=0.80)
    ap.add_argument("--yhi", type=float, default=0.98)
    ap.add_argument("--windows", default="86000-125000:left-hold-1,128000-167000:left-hold-2,60000-84000:control-no-hold")
    a = ap.parse_args()

    windows = {}
    for spec in a.windows.split(","):
        rng, name = spec.split(":")
        lo, hi = (int(v) for v in rng.split("-"))
        windows[name] = (lo, hi)

    for side in ("port", "orig"):
        res = census(a.dump, side, windows, a.n, a.ylo, a.yhi)
        print(f"--- {side}: bottom-right patch {a.n} cols x rows {a.ylo}-{a.yhi} ---")
        if not res:
            print("   no witness frames")
        for name, v in res.items():
            print(f"   {name:>18}: frames={v['frames']:4d} dev_from_modal median={v['dev_median']:5d} "
                  f"p90={v['dev_p90']:5d} max={v['dev_max']:5d} frames_dev>40={v['frames_with_dev']:4d} "
                  f"distinct_colours/patch={v['mean_distinct_cols']:.1f}")


if __name__ == "__main__":
    main()
