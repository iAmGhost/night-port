#!/usr/bin/env python3
"""Right-edge ink census (docs/301).

The playtest report (docs/manual-playtest/04-2026-11-02.md item 5) says: when the player is walked to the
room's LEFT edge, a piece of the player's own sprite appears at the RIGHT edge of the screen.  Every
item-6 census this rig ran before that report measured ink relative to the PLAYER'S COLUMN (limb_side.py)
and never looked at the screen's right edge, so it could not have seen the reported artifact.

This script measures, per witness frame:
  * how much non-black ink sits in the last N columns of the picture ("the right edge"), restricted to the
    lower band where characters stand, and
  * how much sits in the first N columns,
gated by the player's own world X taken from the player_recs.jsonl sidecar (mapped to wall-clock ms through
markers.jsonl).  It is a single-side presence claim on one dump: no cross-dump hue or extent comparison
(docs/287: absolute colour across dumps is inadmissible; docs/299: numeric extents over differently-sized
frame sets are confounded).

  python3 scripts/rig/edge_ink.py /tmp/rig/le8            # port dump
  python3 scripts/rig/edge_ink.py /tmp/rig/le8 --side orig
"""
import argparse
import json
import os
import sys


def frames_index(dump, side):
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
    from diff_screen import read_frames
    rel = "orig_screen" if side == "orig" else "port"
    p = os.path.join(dump, rel, "frames.bin")
    if not os.path.exists(p):
        sys.exit(f"no frames index: {p}")
    out = []
    for f in read_frames(p):
        name = f"{f['frame']:06d}_{f['w']}x{f['h']}.bin"
        path = os.path.join(dump, rel, "frames", name)
        if os.path.exists(path):
            out.append((f["t"], path, f["w"], f["h"]))
    return out


def ink_cols(path, w, h, cols, expect_bytes):
    """Per-column non-black counts in the standing band."""
    d = open(path, "rb").read()
    off = 0
    if len(d) != expect_bytes:
        if len(d) == expect_bytes + 15:
            off = 15
        else:
            return None
    y0, y1 = int(h * 0.55), h
    cnt = {x: 0 for x in cols}
    for y in range(y0, y1):
        base = off + y * w * 4
        for x in cols:
            p2 = base + x * 4
            if d[p2] or d[p2 + 1] or d[p2 + 2]:
                cnt[x] += 1
    return cnt


def ink(path, w, h, cols, expect_bytes):
    d = open(path, "rb").read()
    off = 0
    if len(d) != expect_bytes:
        if len(d) == expect_bytes + 15:
            off = 15
        else:
            return None
    y0, y1 = int(h * 0.55), h
    n = 0
    for y in range(y0, y1):
        base = off + y * w * 4
        for x in cols:
            p = base + x * 4
            if d[p] or d[p + 1] or d[p + 2]:
                n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--side", default="port")
    ap.add_argument("--n", type=int, default=24, help="edge width in columns")
    ap.add_argument("--want", default="320x200", help="only census frames of this size (skip menu modes)")
    ap.add_argument("--left-x", type=int, default=-20, help="player X at/below this counts as 'at the left edge'")
    ap.add_argument("--mid-lo", type=int, default=10)
    ap.add_argument("--mid-hi", type=int, default=40)
    a = ap.parse_args()

    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
    from cells_reader import CellStream

    rel = "orig_screen" if a.side == "orig" else "port"
    idx = frames_index(a.dump, a.side)
    if not idx:
        sys.exit("no witness bins found")
    import collections
    sizes = collections.Counter((f[2], f[3]) for f in idx)
    print(f"{a.side}: {len(idx)} witness bins, sizes={dict(sizes)}")

    # player X -> ms, via the port dump's markers (tick is a sample index, not time)
    xwin = []
    try:
        cs = CellStream(os.path.join(a.dump, "port"))
        mk = sorted((json.loads(l) for l in open(os.path.join(a.dump, "port", "markers.jsonl"))), key=lambda m: m["tick"])
        tk2ms = {}
        for m in mk:
            tk2ms.setdefault(m["tick"], m["t_ms"])
        ticks = sorted(tk2ms)
        for line in open(os.path.join(a.dump, "port", "player_recs.jsonl")):
            r = json.loads(line)
            ms = tk2ms.get(r["tick"])
            if ms is None:
                import bisect
                j = bisect.bisect_left(ticks, r["tick"])
                if j == 0 or j >= len(ticks):
                    continue
                t0, t1 = ticks[j - 1], ticks[j]
                ms = tk2ms[t0] + (tk2ms[t1] - tk2ms[t0]) * (r["tick"] - t0) / max(1, t1 - t0)
            if -10000 < r["x"] < 10000 and -10000 < r["y"] < 10000:
                xwin.append((ms, r["x"]))
    except SystemExit:
        pass
    xwin.sort()

    def x_at(ms):
        if not xwin:
            return None
        lo, hi = 0, len(xwin) - 1
        while lo < hi:
            mid = (lo + hi) // 2
            if xwin[mid][0] < ms:
                lo = mid + 1
            else:
                hi = mid
        return xwin[lo][1]

    groups = {"left-edge": [], "mid": [], "other": []}
    ww, wh = (int(v) for v in a.want.split("x"))
    for t, path, w, h in idx:
        if (w, h) != (ww, wh):
            continue
        eb = w * h * 4
        right_cols = list(range(w - a.n, w))
        left_cols = list(range(0, a.n))
        x = x_at(t)
        if x is None:
            g = "other"
        elif x <= a.left_x:
            g = "left-edge"
        elif a.mid_lo <= x <= a.mid_hi:
            g = "mid"
        else:
            g = "other"
        r = ink_cols(path, w, h, right_cols, eb)
        l = ink_cols(path, w, h, left_cols, eb)
        if r is None:
            continue
        groups[g].append((t, x, sum(r.values()), sum(l.values()), r, l))

    band = max(int(200 * 0.45) * a.n, 1)  # band pixels per edge, 200-row reference
    for name, rows in groups.items():
        if not rows:
            print(f"{name:>10}: no frames")
            continue
        import statistics
        rc = {c: statistics.mean(g[4][c] for g in rows) for c in rows[0][4]}
        lc = {c: statistics.mean(g[5][c] for g in rows) for c in rows[0][5]}
        print(f"             right-edge per-column mean ink (rows={int(200*0.45)} max):")
        print("              " + "  ".join(f"{c}:{rc[c]:5.1f}" for c in sorted(rc, reverse=True)[:12]))
        print("             left-edge per-column mean ink:")
        print("              " + "  ".join(f"{c}:{lc[c]:5.1f}" for c in sorted(lc)[:12]))
        rs = [g[2] for g in rows]
        ls = [g[3] for g in rows]
        rs.sort()
        ls.sort()
        med = rs[len(rs) // 2]
        mx = rs[-1]
        print(f"{name:>10}: frames={len(rows)}  right-edge ink median={med:6d} max={mx:6d}  "
              f"frames_at_max={rs.count(mx):3d}  left-edge ink median={max(ls):6d}  band={band}")
        top = sorted(rows, key=lambda g: -g[2])[:3]
        for t, x, r, l, _, _ in top:
            print(f"             t={t:7d}ms X={x:6d} right_ink={r:6d} left_ink={l:6d}")


if __name__ == "__main__":
    main()
