#!/usr/bin/env python3
"""limb_side.py -- item 6's limb-side instrument (docs/299).

Answers "which side of the player is the attack limb drawn on?" without relying on cadence, colour or
capture geometry:

1. Each side's OWN record series gives its attack-state windows (states 0xb/0xc and the per-type attack
   states) and the player's world X at those windows -- so the two sides are matched by game phase, not
   by wall time (docs/289 §4 rule 1).
2. Each side's OWN frames inside those windows are compared against that side's own modal picture
   (docs/293's basis), and the changed pixels in the ground rows are split at the player's own screen
   column.  A limb drawn on the wrong side flips the left/right reach.

Usage:  limb_side.py <dump-pair-dir> [--px <player world X>] [--rows 120 200] [--cols-max 120]
The dir must contain orig/ (state_*.frame.bin, timeline.json) and port/ (frames/, frames.bin,
player_recs.jsonl, meta.json, cells.bin).
"""
import argparse, collections, glob, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import diff_state
from cells_reader import CellStream
from diff_screen import read_frames

ATTACK = (0xb, 0xc, 0x65, 0x66, 0x67, 0x68, 0x6f, 0x70, 0x71, 0x72, 0x73,
          0x83, 0x84, 0x85, 0xa3, 0xa4, 0xa6, 0xa7)
RB = 0x160


def s32(b, o):
    v = int.from_bytes(b[o:o + 4], "little")
    return v - (1 << 32) if v >= 1 << 31 else v


def windows(series, min_ms):
    """series: (t_ms, state) -> contiguous windows whose state is in ATTACK."""
    out, cur = [], None
    for t, st in series:
        if st in ATTACK:
            if cur is None:
                cur = [t, t]
            cur[1] = t
        else:
            if cur and cur[1] - cur[0] >= min_ms:
                out.append((cur[0], cur[1]))
            cur = None
    if cur and cur[1] - cur[0] >= min_ms:
        out.append((cur[0], cur[1]))
    return out


def shipped_side(route, min_ms):
    samples, cells = diff_state.load_orig(os.path.join(route, "orig"))
    idx = [k for k, c in enumerate(cells) if c.get("name") == "g_player_ent"]
    if not idx:
        raise SystemExit("shipped dump has no g_player_ent record block")
    i = idx[0]
    rb = cells[i].get("rec_bytes") or RB
    base = sum((c.get("rec_bytes") or RB) for c in cells[:i])
    series, xs, f1b = [], [], []
    for t, ds, heap in samples:
        blk = heap[base:base + rb]
        series.append((t, s32(blk, 0x17)))
        xs.append(s32(blk, 0x01))
        f1b.append(blk[0x1b])
    win = windows(series, min_ms)
    t_by = {k: s[0] for k, s in enumerate(samples)}
    files = []
    for p in glob.glob(os.path.join(route, "orig", "state_*.frame.bin")):
        if os.path.getsize(p) != 512015:          # 320x200 modes only (docs/292 §2)
            continue
        k = int(re.search(r"state_(\d+)", p).group(1))
        t = t_by.get(k)
        if t and any(a - 300 <= t <= b + 300 for a, b in win):
            files.append(p)
    ax = collections.Counter(x for x, (_, st) in zip(xs, series) if st in ATTACK)
    af = collections.Counter(f for f, (_, st) in zip(f1b, series) if st in ATTACK)
    return win, files, ax, af


def port_side(route, min_ms):
    cs = CellStream(os.path.join(route, "port"))
    n = len(cs.data) // cs.stride
    tby = {cs.tick(i): cs.header(i)["t_ms"] for i in range(n)}
    series, xs, f1b = [], [], []
    for line in open(os.path.join(route, "port", "player_recs.jsonl"), errors="replace"):
        r = json.loads(line)
        t = tby.get(r["tick"])
        if t is None or r["y"] <= 0 or not (-1000 < r["x"] < 4000):
            continue
        series.append((t, r["st"]))
        xs.append(r["x"])
        f1b.append(r["f1b"])
    win = windows(series, min_ms)
    by = {f["frame"]: f for f in read_frames(os.path.join(route, "port", "frames.bin"))}
    files = []
    for p in glob.glob(os.path.join(route, "port", "frames", "0*320x200.bin")):
        f = by.get(int(re.search(r"/(\d{6})_", p).group(1)))
        if f and any(a - 300 <= f["t"] <= b + 300 for a, b in win):
            files.append(p)
    ax = collections.Counter(x for x, (_, st) in zip(xs, series) if st in ATTACK)
    af = collections.Counter(f for f, (_, st) in zip(f1b, series) if st in ATTACK)
    return win, files, ax, af


def census(files, label, cols, px, rows, colmax, thresh=24):
    N = cols * 200
    hdr = 15 if cols == 640 else 0

    def lum(d, i):
        q = hdr + i * 4
        return (d[q] * 299 + d[q + 1] * 587 + d[q + 2] * 114) // 1000

    modal = [collections.Counter() for _ in range(N)]
    for b in files:
        d = open(b, "rb").read()
        for i in range(N):
            modal[i][lum(d, i) // 8] += 1
    base = [max(c.items(), key=lambda kv: kv[1])[0] * 8 if c else 0 for c in modal]
    del modal
    Lc = Rc = nf = 0
    Lre, Rre = [], []
    for b in files:
        d = open(b, "rb").read()
        xs = []
        for i in range(N):
            y = i // cols
            x = (i % cols) * 320 // cols
            if rows[0] <= y <= rows[1] and x <= colmax and abs(lum(d, i) - base[i]) > thresh:
                xs.append(x)
        if len(xs) < 10:
            continue
        nf += 1
        left = [x for x in xs if x < px]
        right = [x for x in xs if x >= px]
        if left:
            Lre.append(px - min(left))
        if right:
            Rre.append(max(right) - px + 1)
        Lc += len(left)
        Rc += len(right)
    ml = sum(Lre) / len(Lre) if Lre else 0.0
    mr = sum(Rre) / len(Rre) if Rre else 0.0
    print(f"{label}: frames={nf}  ink left of the player={Lc}  right={Rc}  "
          f"L/R={Lc / max(1, Rc):.3f}  mean reach left={ml:.1f}px  right={mr:.1f}px  "
          f"(split at the player's own column X={px}, rows {rows[0]}-{rows[1]}, cols<= {colmax})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("route")
    ap.add_argument("--px", type=int, default=15, help="player world X to split at (docs/299: +15 is this room's left stand limit)")
    ap.add_argument("--rows", type=int, nargs=2, default=[120, 200])
    ap.add_argument("--colmax", type=int, default=120)
    ap.add_argument("--min-ms", type=int, default=150)
    a = ap.parse_args()
    sw, sf, sax, saf = shipped_side(a.route, a.min_ms)
    pw, pf, pax, paf = port_side(a.route, a.min_ms)
    print(f"SHIPPED attack windows={len(sw)} {sw[:5]}")
    print(f"        attack-state X counts={dict(sorted(sax.items()))}  facing(+0x1b)={dict(sorted(saf.items()))}")
    print(f"PORT    attack windows={len(pw)} {pw[:5]}")
    print(f"        attack-state X counts={dict(sorted(pax.items()))}  facing(+0x1b)={dict(sorted(paf.items()))}")
    census(sf, "SHIPPED ", 640, a.px, a.rows, a.colmax)
    census(pf, "PORT    ", 320, a.px, a.rows, a.colmax)


import json  # noqa: E402  (used by port_side)

if __name__ == "__main__":
    main()
