#!/usr/bin/env python3
"""Compare two recorded replays of the original: is the state sequence reproducible?

Determinism is asserted on the *state sequence* (ordered distinct DS digests + frame hashes), not on
wall-clock timing: pinned cycles make the emulated timeline reproducible, but host scheduling is not.
Printed per run: the alignment residual (wall-clock offset between matching state indices), which is
what the event-anchored aligner has to absorb later.
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
JOIN = os.path.join(HERE, "..", "audit", "gen", "ds_join.json")
DS_LO = 0x50000
MASK = os.path.join(HERE, "..", "audit", "gen", "ds_mask.bin")


def differing_cells(runA, runB, idx, limit=12):
    """Which DS cells actually differ at state index `idx` (named through the shipped symbol DB)."""
    fa, fb = (os.path.join(runA, f"state_{idx:04d}.ds.bin"),
              os.path.join(runB, f"state_{idx:04d}.ds.bin"))
    if not os.path.exists(fa):                       # older captures numbered states from 1
        fa, fb = fa.replace(f"{idx:04d}", f"{idx + 1:04d}"), fb.replace(f"{idx:04d}", f"{idx + 1:04d}")
    if not (os.path.exists(fa) and os.path.exists(fb)):
        return []
    a, b = open(fa, "rb").read(), open(fb, "rb").read()
    mask = open(MASK, "rb").read()
    names = {}
    try:
        for r in json.load(open(JOIN)):
            nm = (r["port_names"] or [r["guest_symbol"] or "?"])[0]
            names[r["ds"]] = nm
    except OSError:
        pass
    out = []
    for i, (x, y, m) in enumerate(zip(a, b, mask)):
        if m and x != y:
            addr = DS_LO + i
            out.append((hex(addr), names.get(addr, "?"), x, y))
            if len(out) >= limit:
                break
    return out


def load(p):
    return json.load(open(os.path.join(p, "timeline.json")))


def seq(t):
    return [(s["ds_sha"], s["frame_sha"]) for s in t["timeline"]]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+", help="run directories containing timeline.json")
    ap.add_argument("--max-print", type=int, default=12)
    a = ap.parse_args()
    runs = [load(p) for p in a.runs]
    base = runs[0]
    print(f"reference: {a.runs[0]}  states={len(base['timeline'])}")
    ok = True
    for name, r in zip(a.runs[1:], runs[1:]):
        sa, sb = seq(base), seq(r)
        common = min(len(sa), len(sb))
        same = sum(1 for x, y in zip(sa, sb) if x == y)
        first_diff = next((i for i in range(common) if sa[i] != sb[i]), None)
        dres = [round(b["t_wall"] - x["t_wall"], 3)
                for x, b in zip(base["timeline"][:common], r["timeline"][:common])
                if x["ds_sha"] == b["ds_sha"]]
        print(f"\ncompare {name}: states={len(sb)} matching_prefix={same}/{common} "
              f"first_divergence_index={first_diff}")
        if dres:
            print(f"  alignment residual vs reference (s): min={min(dres):+.3f} "
                  f"max={max(dres):+.3f} spread={max(dres)-min(dres):.3f}")
        if first_diff is not None:
            ok = False
            for addr, nm, x, y in differing_cells(a.runs[0], name, first_diff):
                print(f"      cell {addr} {nm}: {x:#04x} vs {y:#04x}")
            for i in range(max(0, first_diff - 1), min(common, first_diff + 3)):
                print(f"    [{i}] ref t={base['timeline'][i]['t_wall']} {base['timeline'][i]['ds_sha'][:12]}"
                      f" | run t={r['timeline'][i]['t_wall']} {r['timeline'][i]['ds_sha'][:12]}")
        elif len(sa) != len(sb):
            ok = False
            print(f"  state COUNT differs: {len(sa)} vs {len(sb)}")
    print("\nDETERMINISM:", "PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
