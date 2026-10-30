#!/usr/bin/env python3
"""Time-anchored screen comparison: the screen oracle without a frame-index DP.

docs/205 records why: diff_screen.py's align() walks the similarity matrix with a fixed +/-90 frame
lag window around the proportional position.  On the canonical route the two frame streams are 6064
shipped frames and 38 943 port frames, so the proportional position moves 6.4 port frames per shipped
frame and any clock drift bigger than ~14 % saturates the window.  When it saturates the DP keeps
matching late shipped frames against the port's very first frames (black 640x480 boot), and the pair
scores print as 0.000 for 40 seconds of route -- an alignment artifact that looks like a content
divergence.

Both dumps carry a millisecond stamp, and the shipped replay publishes a rebase table (emulator clock ->
drive nominal time) in its timeline.json, exactly the mapping diff_state.py uses to cut phase windows.
This script uses that: every shipped frame is mapped to nominal time, matched to the port frame nearest
in nominal time, and scored.  No frame numbers, no lag window, no DP.

  screen_time.py --orig-run g37 --port g37/port [--thresh 0.88] [--tol 400] [--md out.md]
"""
import argparse, bisect, json, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_screen import read_frames, dist  # record layout + score are shared, only pairing changes


def nominal_map(tl):
    reb = tl.get("rebases") or [
        {"t_ms": w["t_ms"], "nominal": w["t_ms"] - w["shift_ms"]} for w in tl.get("ds_waits", [])]
    t0 = tl["timeline"][0]["t_wall"] if tl.get("timeline") else 0.0
    if not reb:
        reb = [{"t_ms": 0, "nominal": int(t0 * 1000)}]

    def nom(t_ms):
        off = reb[0]["nominal"] - reb[0]["t_ms"]
        for r in reb:
            if r["t_ms"] <= t_ms:
                off = r["nominal"] - r["t_ms"]
            else:
                break
        return t_ms + off
    return nom, reb


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig-run", required=True, help="replay run dir (has timeline.json and orig/frames.bin)")
    ap.add_argument("--port", required=True, help="port dump dir (has frames.bin)")
    ap.add_argument("--thresh", type=float, default=0.88)
    ap.add_argument("--tol", type=int, default=400, help="max nominal-time gap accepted for a pair (ms)")
    ap.add_argument("--md")
    a = ap.parse_args()

    run = a.orig_run
    for cand in (os.path.join(run, "timeline.json"), os.path.join(run, "orig", "timeline.json")):
        if os.path.exists(cand):
            tl = json.load(open(cand)); base = os.path.dirname(cand); break
    else:
        sys.exit(f"{run}: no timeline.json")
    fb = os.path.join(base, "frames.bin") if os.path.exists(os.path.join(base, "frames.bin")) else None
    if fb is None:
        for cand in (os.path.join(run, "orig_screen", "frames.bin"), os.path.join(base, "orig_screen", "frames.bin")):
            if os.path.exists(cand):
                fb = cand; break
    if fb is None:
        sys.exit(f"{run}: no frames.bin")

    A = read_frames(fb); B = read_frames(os.path.join(a.port, "frames.bin"))
    nom, reb = nominal_map(tl)
    pt = [f["t"] for f in B]
    pairs = []; unmatched = 0
    for i, fa in enumerate(A):
        tn = nom(fa["t"])
        k = bisect.bisect_left(pt, tn)
        best = None
        for j in (k - 1, k):
            if 0 <= j < len(B):
                d = abs(B[j]["t"] - tn)
                if best is None or d < best[0]:
                    best = (d, j)
        if best is None or best[0] > a.tol:
            unmatched += 1; continue
        pairs.append((i, best[1], 1.0 - dist(fa["sig"], B[best[1]]["sig"]), best[0], tn))
    if not pairs:
        sys.exit("no pairs within tolerance")
    sc = sorted(p[2] for p in pairs)
    mean = sum(p[2] for p in pairs) / len(pairs)
    lines = []
    lines.append(f"shipped frames={len(A)}  port frames={len(B)}  pairs within {a.tol} ms={len(pairs)}  unmatched={unmatched}")
    lines.append(f"rebases (emulator ms -> nominal ms): {reb}")
    lines.append(f"route score mean={mean:.4f}  median={sc[len(sc)//2]:.4f}  p10={sc[len(sc)//10]:.4f}  min={sc[0]:.4f}  "
                 f"pairs<{a.thresh}={sum(1 for s in sc if s < a.thresh)}")
    from collections import Counter
    lines.append(f"shipped modes: {Counter((f['w'], f['h']) for f in A)}")
    lines.append(f"port modes:    {Counter((f['w'], f['h']) for f in B)}")
    runs = []; cur = None
    for p in pairs:
        if p[2] < a.thresh:
            if cur is None:
                cur = [p[4], p[4], 1, p[2], p]
            else:
                cur[1] = p[4]; cur[2] += 1
                if p[2] < cur[3]: cur[3] = p[2]; cur[4] = p
        elif cur:
            runs.append(cur); cur = None
    if cur: runs.append(cur)
    runs.sort(key=lambda r: -r[2])
    lines.append(f"\ndivergent runs (score < {a.thresh}) ordered by length: {len(runs)}")
    for t0, t1, n, worst, rep in runs[:18]:
        i, j, s, gap, tn = rep
        lines.append(f"  nominal t={t0}..{t1} ms ({(t1-t0)/1000:.1f}s, {n} pairs) worst score={worst:.3f} "
                     f"at nominal={tn} (shipped frame {A[i]['frame']} {A[i]['w']}x{A[i]['h']}, "
                     f"port frame {B[j]['frame']} {B[j]['w']}x{B[j]['h']}, gap {gap} ms)")
    same = [p for p in pairs if A[p[0]]["w"] == B[p[1]]["w"] and A[p[0]]["h"] == B[p[1]]["h"]]
    other = [p for p in pairs if p not in same] if len(pairs) < 20 else []
    if same:
        ss = sorted(p[2] for p in same)
        lines.append(f"same-mode pairs={len(same)}  mean={sum(p[2] for p in same)/len(same):.4f}  "
                     f"median={ss[len(ss)//2]:.4f}  min={ss[0]:.4f}  pairs<{a.thresh}={sum(1 for s2 in ss if s2 < a.thresh)}")
    cross = len(pairs) - len(same)
    if cross:
        cs = sorted(p[2] for p in pairs if not (A[p[0]]["w"] == B[p[1]]["w"] and A[p[0]]["h"] == B[p[1]]["h"]))
        lines.append(f"cross-mode pairs={cross}  mean={sum(cs)/len(cs):.4f}  median={cs[len(cs)//2]:.4f}  min={cs[0]:.4f}")
    print("\n".join(lines))
    if a.md:
        open(a.md, "w").write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
