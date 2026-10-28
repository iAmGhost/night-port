#!/usr/bin/env python3
"""Event-anchored CD-DA track schedule comparison (the music oracle).

The playtest's music items are statements about *which track is selected when*, and both sides keep
that in memory: `fn_010b1d` writes DAT_00053c44 (start LBA), DAT_00053c40 (end LBA) and DAT_00053c48
(length in seconds), so the shipped side's schedule is readable from its DS snapshots, and the port
selects the same cells (it runs the same function) and additionally emits witness marks
(`cdda.jsonl`: play/stop with index/LBA/sectors).

Comparing the two in drive-nominal time is wrong: each side boots at its own speed and the drive's
nominal times are only estimates (docs/56). So the schedule is compared **relative to a shipped
event**, by default the stage-data load (DAT_00053e9c 0 -> nonzero, the same cell both drives anchor
on). Measured that way on route-e1 the two schedules agree to 0.1 s, which superseded an earlier
nominal-time reading that claimed the port selected the room track ~2 s early (docs/105 §7).

  cd_schedule.py --orig RUN/orig --port RUN/port [--anchor 0x53e9c] [--toc gen/toc.json]

The LBA -> track-index map comes from the port's own marks when no TOC file is given (its witness
records index and lba together).
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import DS_LO, load_orig, load_port  # noqa: E402


def changes(seq):
    out = []
    for t, v in seq:
        if not out or out[-1][1] != v:
            out.append((t, v))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--anchor", default="0x53e9c", help="DS cell that marks the stage-data load")
    ap.add_argument("--cell", default="0x53c44", help="DS cell holding the selected track's start LBA")
    ap.add_argument("--tol", type=int, default=2500, help="ms of agreement allowed per selection")
    a = ap.parse_args()
    anchor, cell = int(a.anchor, 0), int(a.cell, 0)

    orig, _ = load_orig(a.orig)
    port, pcells = load_port(a.port)

    def rd_orig(ds, addr):
        return int.from_bytes(ds[addr - DS_LO:addr - DS_LO + 4], "little")

    off = {c["ds"]: c["offset"] for c in pcells if c["ds"] in (anchor, cell)}
    if cell not in off:
        sys.exit(f"port table has no cell {hex(cell)}")

    # LBA -> track index, from the port's own witness marks
    lba2idx = {}
    marks = []
    pj = os.path.join(a.port, "cdda.jsonl")
    if os.path.exists(pj):
        for line in open(pj):
            j = json.loads(line)
            if j.get("event"):
                marks.append((j["t_ms"], j["event"], j.get("index"), j.get("lba")))
                if j["event"] == "play":
                    lba2idx[j["lba"]] = j["index"]

    def fmt(t, anchor_t, lba):
        idx = lba2idx.get(lba)
        return f"{t - anchor_t:+7d} ms  lba={hex(lba)}" + (f" track={idx}" if idx is not None else "")

    def schedule(samples, get):
        anch, seq, aseen = None, [], False
        for s in samples:
            t, v_anchor, v_cell = get(s)
            if not aseen and v_anchor:
                anch, aseen = t, True
            seq.append((t, v_cell))
        return anch, changes(seq)

    a_o, seq_o = schedule(orig, lambda s: (s[0], rd_orig(s[1], anchor), rd_orig(s[1], cell)))
    if anchor not in off:
        sys.exit(f"port table has no anchor cell {hex(anchor)}")
    a_p, seq_p = schedule(port, lambda s: (s[0], int.from_bytes(s[1][off[anchor]:off[anchor] + 4], "little")
                                           if anchor in off else 0,
                                           int.from_bytes(s[1][off[cell]:off[cell] + 4], "little")))

    print(f"anchor DS {hex(anchor)} first seen: orig t={a_o} port t={a_p}")
    print("\noriginal schedule (relative to its own anchor):")
    for t, v in seq_o:
        if v:
            print("  " + fmt(t, a_o or 0, v))
    print("\nport schedule (relative to its own anchor):")
    for t, v in seq_p:
        if v:
            print("  " + fmt(t, a_p or 0, v))

    print("\nselection-by-selection (skip the boot track that precedes the anchor):")
    po = [(t, v) for t, v in seq_o if v and t >= (a_o or 0) - 20000]
    pp = [(t, v) for t, v in seq_p if v and t >= (a_p or 0) - 20000]
    bad = 0
    for i in range(max(len(po), len(pp))):
        o = po[i] if i < len(po) else (None, None)
        p = pp[i] if i < len(pp) else (None, None)
        if o[1] is None or p[1] is None:
            print(f"  #{i}: orig={o[1] and hex(o[1])} port={p[1] and hex(p[1])}  MISSING on one side")
            bad += 1
            continue
        d = (p[0] - (a_p or 0)) - (o[0] - (a_o or 0))
        same = o[1] == p[1]
        ok = same and abs(d) <= a.tol
        if not ok:
            bad += 1
        print(f"  #{i}: {'same lba' if same else 'DIFFERENT lba'}  timing delta {d:+6d} ms  "
              f"{'ok' if ok else '<== divergence'}")

    # the port's own teardown behaviour: a play mark with no preceding stop since the last play
    plays = [m for m in marks if m[1] == "play"]
    stops = [m for m in marks if m[1] == "stop"]
    # order-based, not time-based: the port emits its replacement teardown in the same millisecond as
    # the new play mark, so a strict `<` comparison on t_ms reports a teardown that did happen
    overlap, saw_stop = 0, True
    for t, ev, idx, lba in marks:
        if ev == "play":
            if not saw_stop:
                overlap += 1
            saw_stop = False
        else:
            saw_stop = True
    print(f"\nport witness: {len(plays)} plays, {len(stops)} stops, "
          f"{overlap} track changes with NO stop/teardown between them")
    if bad:
        sys.exit(f"{bad} schedule divergence(s)")


if __name__ == "__main__":
    main()
