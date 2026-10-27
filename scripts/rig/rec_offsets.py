#!/usr/bin/env python3
"""Field-activity map of a dumped record: which byte offsets change, how often, and whether they step.

The port's comments name entity fields (EXP at +0x2b, score at +0x52, award at +0xbb ...). Those names
are the thing being audited, so a field comparison must not depend on them. This script derives the
field layout from the shipped side's own bytes: for every byte offset inside the record it reports how
many consecutive-sample pairs changed it, and whether the changes were monotonic upward (a counter:
EXP, score, lives) or bidirectional (position, timers). Rare + monotonic-upward offsets are exactly the
shape of an award field, and the same offsets are then read on the port side -- so a missing award is
found without assuming the port got the offset right.

  rec_offsets.py --orig RUN/orig --port RUN/port --cell g_player_ent [--from 160000 --to 175000]
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402


def blocks(side, chosen, is_orig, ocells=None):
    """Yield (t_ms, record_bytes) for one pointer cell's record block."""
    out = []
    for s in side:
        for i, c in chosen:
            rb = c.get("rec_bytes") or 0
            if is_orig:
                base = sum(x.get("rec_bytes", 0) for x in ocells[:i])
                blk = s[2][base:base + rb]
            else:
                blk = s[1][c["rec_offset"]:c["rec_offset"] + rb]
            if len(blk) == rb and any(blk):
                out.append((s[0], blk))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--cell", required=True)
    ap.add_argument("--from", dest="t0", type=int, default=0)
    ap.add_argument("--to", dest="t1", type=int, default=1 << 40)
    ap.add_argument("--width", type=int, default=4, help="dword stride for the step report")
    ap.add_argument("--top", type=int, default=18)
    a = ap.parse_args()

    orig, ocells = load_orig(a.orig)
    port, pcells = load_port(a.port)

    def pick(cells):
        addr = int(a.cell, 16) if a.cell.lower().startswith("0x") else None
        return [(i, c) for i, c in enumerate(cells)
                if (addr is not None and c["ds"] == addr) or (addr is None and c["name"] == a.cell)]

    oc, pc = pick(ocells), pick(pcells)
    if not oc or not pc:
        sys.exit(f"cell {a.cell} not found (orig {len(oc)}, port {len(pc)})")

    def analyse(side, chosen, is_orig):
        bs = [b for (t, b) in blocks(side, chosen, is_orig, ocells) if a.t0 <= t <= a.t1]
        if len(bs) < 3:
            return None
        size = len(bs[0])
        w = a.width
        noff = size - w + 1
        changes, up, down = [0] * noff, [0] * noff, [0] * noff
        prev = [int.from_bytes(bs[0][o:o + w], "little") for o in range(noff)]
        for b in bs[1:]:
            cur = [int.from_bytes(b[o:o + w], "little") for o in range(noff)]
            for o in range(noff):
                if cur[o] != prev[o]:
                    changes[o] += 1
                    if cur[o] > prev[o]:
                        up[o] += 1
                    else:
                        down[o] += 1
            prev = cur
        return {"n": len(bs), "size": size, "changes": changes, "up": up, "down": down}

    ao, ap_ = analyse(orig, oc, True), analyse(port, pc, False)
    if ao is None or ap_ is None:
        sys.exit("not enough non-null record samples on one side "
                 f"(orig {0 if ao is None else ao['n']}, port {0 if ap_ is None else ap_['n']})")
    print(f"cell {a.cell}  samples orig={ao['n']} port={ap_['n']}  record {ao['size']}B  "
          f"dword stride {a.width}  window [{a.t0},{a.t1}]")

    # candidate award fields: rarely changes, never down, at least two steps
    cand = [o for o in range(len(ao["changes"]))
            if ao["up"][o] >= 2 and ao["down"][o] == 0 and ao["changes"][o] <= max(4, ao["n"] // 8)]
    cand.sort(key=lambda o: (-ao["up"][o], o))
    print("\nmonotonic-up (award-shape) offsets, orig side, with the port's activity at the same offset:")
    print(f"{'off':>6s} {'orig up/down':>14s} {'port up/down':>14s}   orig values seen")
    seen_o = {}
    for t, b in blocks(orig, oc, True, ocells):
        if not (a.t0 <= t <= a.t1):
            continue
        for o in cand[: a.top]:
            v = int.from_bytes(b[o:o + a.width], "little")
            seen_o.setdefault(o, set()).add(v)
    for o in cand[: a.top]:
        pv = (ap_["up"][o], ap_["down"][o])
        vals = sorted(seen_o.get(o, set()))[:5]
        print(f"+{hex(o):>5s} {str((ao['up'][o], ao['down'][o])):>14s} {str(pv):>14s}   "
              + " ".join(hex(v) for v in vals))
    if not cand:
        print("  (none)")

    # busiest offsets, for contrast: the moving fields
    busy = sorted(range(len(ao["changes"])), key=lambda o: -ao["changes"][o])[: a.top]
    print("\nbusiest offsets (movement/timers), orig vs port change counts:")
    for o in busy:
        print(f"+{hex(o):>5s} orig {ao['changes'][o]:4d} port {ap_['changes'][o]:4d}")


if __name__ == "__main__":
    main()
