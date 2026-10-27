#!/usr/bin/env python3
"""Audio oracle without an audio pipe: per-cell *activity* profiles, per window.

Measured constraint: dosbox-automation 0.85.1 exposes no audio capture endpoint (`openapi.json` lists
`capture/video/*` only), and reads of the Sound Blaster ports return 0xFF (`/io/port` probes of 0x210 /
0x214 / 0x330 / 0x334 / 0x33C), so there is no container WAV and no DSP register to sample. The audio
state that *is* observable is the DS state the game's own audio driver drives: which cells change, how
often, and in which window. That is enough for the question the playtest items ask -- presence,
absence, onset and which slot -- without pretending to compare waveforms.

Per cell per window it counts value transitions and reports the pair:

  DIFF        one side oscillates while the other is frozen (a stopped music loop, a missing SFX)
  SAME        comparable activity
  FROZEN-BOTH neither side touched it in the window

Usage:
  diff_activity.py --orig ORIG_RUN --port PORT_DUMP --drive DRIVE [--md OUT.md]
"""
import argparse
import collections
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "..", "audit", "gen")
DS_LO = 0x50000


def phases_from_drive(path):
    edges = []
    for line in open(path):
        f = line.split("#")[0].split()
        if len(f) >= 4 and f[1] == "key" and f[3].startswith("d"):
            edges.append(int(f[0]))
    edges = sorted(set([0] + edges))
    return [(f"P{i}@{t0}", t0, edges[i + 1] if i + 1 < len(edges) else 1 << 40)
            for i, t0 in enumerate(edges)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--drive", required=True)
    ap.add_argument("--md")
    ap.add_argument("--min-activity", type=float, default=0.3,
                    help="transitions needed on one side to call the other side frozen")
    ap.add_argument("--max-rows", type=int, default=40)
    a = ap.parse_args()

    cells = json.load(open(os.path.join(GEN, "rig_cells.json")))
    entropy = {int(k, 16) for k in json.load(open(os.path.join(GEN, "entropy_cells.json")))
               ["entropy_cells"]} if os.path.exists(os.path.join(GEN, "entropy_cells.json")) else set()
    tl = json.load(open(os.path.join(a.orig, "timeline.json")))
    t0 = tl["timeline"][0]["t_wall"] if tl["timeline"] else 0.0
    orig = []
    for i, st in enumerate(tl["timeline"]):
        p = os.path.join(a.orig, f"state_{i:04d}.ds.bin")
        if not os.path.exists(p):
            p = os.path.join(a.orig, f"state_{i + 1:04d}.ds.bin")
        if os.path.exists(p):
            orig.append((int((st["t_wall"] - t0) * 1000), open(p, "rb").read()))

    meta = json.load(open(os.path.join(a.port, "meta.json")))
    rec = struct.Struct("<qQQQII")
    rb = meta["record_bytes"]
    port = []
    with open(os.path.join(a.port, "cells.bin"), "rb") as f:
        f.read(16)
        while True:
            h = f.read(rec.size)
            if len(h) < rec.size:
                break
            t_ms, frame, audio, screen, tick, n = rec.unpack(h)
            blob = f.read(rb)
            if len(blob) < rb:
                break
            port.append((t_ms, blob, frame, audio))

    phases = phases_from_drive(a.drive)
    poff = {c["name"]: c["offset"] for c in meta["cells"]}

    def counts(samples, getter):
        """transitions per cell per phase"""
        out = collections.defaultdict(lambda: collections.defaultdict(list))
        for smp in samples:
            t = smp[0]
            for label, lo, hi in phases:
                if lo <= t < hi:
                    out[label].setdefault("samples", []).append(getter(smp))
                    break
        res = {}
        for label, d in out.items():
            seq = d["samples"]
            res[label] = seq
        return res

    o_by = counts(orig, lambda s: s[1])
    p_by = counts(port, lambda s: s[1])

    rows = []
    for c in cells:
        ds, gsz, hsz = c["ds"], c["guest_size"], c["host_size"]
        gs = gsz or hsz
        w = min(gs, hsz)
        off = poff.get(c["name"])
        if off is None or any((ds + k) in entropy for k in range(max(gs, hsz))):
            continue
        per, verdict = [], None
        for label, _lo, _hi in phases:
            ov = [b[ds - DS_LO:ds - DS_LO + w] for b in o_by.get(label, [])]
            pv = [b[off:off + w] for b in p_by.get(label, [])]
            # rates, not raw counts: the two sides are sampled at different densities (the emulator
            # poll and the port's frame cadence), so only transitions-per-second are comparable.
            co = sum(1 for x, y in zip(ov, ov[1:]) if x != y)
            cp = sum(1 for x, y in zip(pv, pv[1:]) if x != y)
            so = (ov[-1][0] - ov[0][0]) / 1000.0 if len(ov) > 1 else 0.0
            sp = (pv[-1][0] - pv[0][0]) / 1000.0 if len(pv) > 1 else 0.0
            ro, rp = (co / so) if so > 0.5 else 0.0, (cp / sp) if sp > 0.5 else 0.0
            per.append((label, round(ro, 2), round(rp, 2)))
            if verdict:
                continue
            if (ro >= a.min_activity and rp == 0.0) or (rp >= a.min_activity and ro == 0.0):
                verdict = "FROZEN"
            elif ro and rp and (ro / rp > 3 or rp / ro > 3):
                verdict = "RATE"
            else:
                verdict = "SAME"
        rows.append({"ds": hex(ds), "name": c["name"], "verdict": verdict or "NO-DATA",
                     "phases": per})

    tally = collections.Counter(r["verdict"] for r in rows)
    print(f"cells with activity profiles: {len(rows)}  phases={len(phases)}  tally={dict(tally)}")
    for r in rows:
        if r["verdict"] in ("FROZEN", "RATE") and a.max_rows:
            a.max_rows -= 1
            print(f'  {r["verdict"]:6s} {r["ds"]} {r["name"]:22s} '
                  + " ".join(f"{lb}:{co}/{cp}" for lb, co, cp in r["phases"]))
    print("(per phase: original transitions/s / port transitions/s)")
    print("port audio bytes handed to the device over the run:",
          port[-1][3] if port else 0, "bytes; sample windows:",
          f"{port[0][0]}..{port[-1][0]} ms" if port else "-")
    if a.md:
        os.makedirs(os.path.dirname(a.md) or ".", exist_ok=True)
        with open(a.md, "w") as f:
            f.write("# audio-activity oracle (no audio pipe: 0.85.1 has no capture/audio)\n\n")
            f.write(f"phases {len(phases)}; tally {dict(tally)}; per phase = orig/port transitions\n\n")
            f.write("| verdict | DS | port symbol | per-phase counts |\n|---|---|---|---|\n")
            for r in rows:
                if r["verdict"] == "SAME":
                    continue
                f.write(f'| {r["verdict"]} | {r["ds"]} | {r["name"]} | '
                        + " ".join(f"{lb}:{co}/{cp}" for lb, co, cp in r["phases"]) + " |\n")
        print(f"wrote {a.md}")


if __name__ == "__main__":
    main()
