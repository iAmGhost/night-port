#!/usr/bin/env python3
"""Screen oracle: score the port's picture against the original's over a whole route.

Reads the two `frames.bin` streams (same record layout on both sides: scripts/rig/capture_original.py
and src/app/rig_sink.c) and aligns them monotonically rather than by frame number: the two sides
do not share a clock, so alignment is a path through the similarity matrix with a per-step lag, exactly
the event-anchored rule of docs/56. Every frame gets a score (mean absolute difference of the 40x30
luminance grid, normalised to 0..1 where 0 = identical), and contiguous low-scoring runs are reported
as divergences with timestamps on both sides.

Two premise checks are built in rather than left to a reviewer:
  * channel order (docs/70, docs/79): if witness frames exist, the score is recomputed with R and B
    swapped on one side; a better swapped score is reported as a channel-order defect, not as content.
  * pacing / phase pinning (docs/82): frames per second and the count of *distinct* pictures per side,
    so a route that runs at the wrong rate or shows fewer scene changes is called out explicitly.
"""
import argparse
import json
import os
import struct
import sys

REC = struct.Struct("<qQiiii")
HDR = struct.Struct("<III")


def read_frames(path):
    with open(path, "rb") as f:
        hdr = HDR.unpack(f.read(12))
        if hdr[0] != 0x5247464E:
            sys.exit(f"{path}: not an NGFR frames.bin")
        n = hdr[2]
        rec, sig, out = f.read(REC.size), f.read(n), []
        while len(rec) == REC.size and len(sig) == n:
            t_ms, frame, w, h, changed, _pad = REC.unpack(rec)
            out.append({"t": t_ms, "frame": frame, "w": w, "h": h, "changed": changed,
                        "sig": sig})
            rec, sig = f.read(REC.size), f.read(n)
    return out


def dist(a, b):
    return sum(abs(x - y) for x, y in zip(a, b)) / (255.0 * len(a))


def align(A, B, win=90):
    # The transition limit is a step in *port frames per original frame*, and it has to accommodate the slope between
    # the two streams: 14 001 port frames against 434 original frames needs ~32 per step, so a limit of 3 makes every
    # honest path illegal and the DP settles for a slope it can afford -- which is the other half of why a route score
    # fell back to its floor (docs/386 3, docs/387).
    # win is a frame window, not a time window: at 54.5 presentations/s the default 90 spans about 1.6 s of the port
    # side.  Two runs that reach the same phase tens of seconds apart therefore have no pair inside the window, and
    # the mean score collapses to a content-independent floor (see --floor, docs/384).
    """Monotone alignment by dynamic programming inside a lag window: A[i] maps to some B[j] with j
    near i*nb/na, and j never decreases. Frame numbers are not comparable across the two sides
    (docs/56), so this is the alignment."""
    na, nb = len(A), len(B)
    if not na or not nb:
        return [], 1.0
    na2 = min(na, 3000)
    step = max(3, nb // max(1, na) + 1)          # the slope the monotone path is allowed to follow
    INF = float("inf")
    dp = [[INF] * nb for _ in range(na2)]
    for i in range(na2):
        c = i * nb // na
        lo, hi = max(0, c - win), min(nb, c + win + 1)
        for j in range(lo, hi):
            s = dist(A[i]["sig"], B[j]["sig"])
            if i == 0:
                dp[i][j] = s
            else:
                prev = min(dp[i - 1][max(0, j - step):j + 1])
                if prev < INF:
                    dp[i][j] = prev + s
    j = min(range(nb), key=lambda x: dp[na2 - 1][x])
    pairs = []
    for i in range(na2 - 1, -1, -1):
        pairs.append((i, j))
        if i:
            j = min(range(max(0, j - step), min(nb, j + 1)), key=lambda x: dp[i - 1][x])
    pairs.reverse()
    pairs = [(i, jb, 1.0 - dist(A[i]["sig"], B[jb]["sig"])) for i, jb in pairs]
    return pairs, sum(p[2] for p in pairs) / len(pairs)


def attribution(run_dir, bad, limit=6):
    """Content attribution: for each divergent window, diff the two DS snapshots that bracket its
    start and name the cells the shipped game had just written. That is the 'what was happening' half
    of a divergence report, computed instead of eyeballed."""
    gen = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "audit", "gen")
    out = {}
    try:
        tl = json.load(open(os.path.join(run_dir, "timeline.json")))
        cells = json.load(open(os.path.join(gen, "rig_cells.json")))
    except OSError:
        return out
    names = {c["ds"]: c["name"] for c in cells}
    # the measured entropy cells (docs/101) tick constantly and would drown every attribution
    try:
        ent = json.load(open(os.path.join(gen, "entropy_cells.json")))["entropy_cells"]
        names = {ds: n for ds, n in names.items() if hex(ds) not in ent}
    except OSError:
        pass
    t0 = tl["timeline"][0]["t_wall"] if tl["timeline"] else 0.0
    tms = [int((st["t_wall"] - t0) * 1000) for st in tl["timeline"]]

    def snap(i):
        for cand in (i, i + 1):                     # state files have been numbered from 0 and from 1
            p = os.path.join(run_dir, f"state_{cand:04d}.ds.bin")
            if os.path.exists(p):
                return open(p, "rb").read()
        return None

    for key in bad:
        i = min(range(len(tms)), key=lambda k: abs(tms[k] - key[0])) if tms else 0
        a, b = snap(max(0, i - 1)), snap(i)
        if not a or not b:
            continue
        changes = []
        for ds in sorted(names):
            o = ds - 0x50000
            if a[o:o + 4] != b[o:o + 4]:
                changes.append(f"{hex(ds)} {names[ds]} {a[o:o+4][::-1].hex()} -> {b[o:o+4][::-1].hex()}")
                if len(changes) >= limit:
                    break
        out[key] = changes or ["no dumped DS cell changed at this moment"]
    return out


def channel_check(orig_bin, port_bin, pairs, limit=8):
    """Compare aligned witness frames under the declared channel order and with R and B swapped.
    Returns None when the witness sets do not overlap enough to say anything."""
    import glob
    od = os.path.join(os.path.dirname(orig_bin), "frames")
    pd = os.path.join(os.path.dirname(port_bin), "frames")
    ofiles = sorted(glob.glob(os.path.join(od, "*.bin")))
    pfiles = sorted(glob.glob(os.path.join(pd, "*.bin")))
    if not ofiles or not pfiles:
        return None

    def px_chroma(path):
        raw = open(path, "rb").read()
        px = raw[15:]
        step = 4 * 97
        tot = cnt = 0
        for o in range(0, len(px) - 3, step):
            tot += abs(px[o] - px[o + 2])          # B vs R
            cnt += 1
        return tot / max(1, cnt)

    def grid_from_orig(path):
        raw = open(path, "rb").read()
        wh = os.path.basename(path).split("_")[-1].replace(".bin", "")
        w, h = int(wh.split("x")[0]), int(wh.split("x")[1])
        px = raw[15:]
        g = bytearray()
        for gy in range(30):
            for gx in range(40):
                o = (gy * h // 30) * (w * 4) + (gx * w // 40) * 4
                b, gg, r = px[o], px[o + 1], px[o + 2]
                g.append((r * 77 + gg * 150 + b * 29) >> 8)
        return bytes(g)

    def grid_from_port(path, swap):
        wh = os.path.basename(path).split("_")[-1].replace(".bin", "")
        w, h = int(wh.split("x")[0]), int(wh.split("x")[1])
        px = open(path, "rb").read()
        ri, bi = (3, 1) if not swap else (1, 3)
        g = bytearray()
        for gy in range(30):
            for gx in range(40):
                o = ((gy * h // 30) * w + (gx * w // 40)) * 4
                r, gg, b = px[o + ri], px[o + 2], px[o + bi]
                g.append((r * 77 + gg * 150 + b * 29) >> 8)
        return bytes(g)

    og = [grid_from_orig(p) for p in ofiles]
    declared = swapped = 0.0
    chroma = 0.0
    n = 0
    for i, j, _ in pairs[::max(1, len(pairs) // limit)]:
        k, m = i % len(og), j % len(pfiles)
        ga = og[k]
        chroma += abs(px_chroma(ofiles[k]))
        for swap in (False, True):
            gb = grid_from_port(pfiles[m], swap)
            sc = 1.0 - dist(ga, gb)
            if swap:
                swapped = max(swapped, sc)
            else:
                declared = max(declared, sc)
        n += 1
    return (declared, swapped, chroma / max(1, n)) if n else None


def runs(pairs, A, thresh=0.88):
    """Contiguous aligned runs whose score drops below thresh, in original-side timestamps."""
    bad, cur = [], None
    for i, _j, sc in pairs:
        if sc < thresh:
            if cur is None:
                cur = [i, i, sc]
            else:
                cur[1] = i
                cur[2] = min(cur[2], sc)
        elif cur:
            bad.append(tuple(cur))
            cur = None
    if cur:
        bad.append(tuple(cur))
    return [(A[i0]["t"], A[i1]["t"], worst) for (i0, i1, worst) in bad]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True, help="original frames.bin")
    ap.add_argument("--port", required=True, help="port frames.bin")
    ap.add_argument("--thresh", type=float, default=0.88)
    ap.add_argument("--max-runs", type=int, default=15)
    ap.add_argument("--win", type=int, default=90, help="alignment lag window in port-side frames (90 = ~1.6 s at "
                                                        "54.5/s); widen it when the two runs reach the same phase at "
                                                        "different times, and read --floor to tell signal from floor")
    ap.add_argument("--floor", action="store_true", help="also score the port side with every grid zeroed: the value "
                                                         "this metric falls back to when no aligned pair matches")
    ap.add_argument("--orig-run", help="the replay_original.py run dir (DS snapshots): a divergent "
                                       "window is attributed to the DS cells the game wrote at that moment")
    ap.add_argument("--md")
    a = ap.parse_args()

    A, B = read_frames(a.orig), read_frames(a.port)
    pairs, mean = align(A, B, a.win)
    if a.floor:
        Z = [dict(r) for r in B]
        for r in Z:
            r["sig"] = bytes(len(r["sig"]))
        _, fmean = align(A, Z, a.win)
        # Reported first, because it is what the score means when it has nothing to match: a zeroed-grid port stream
        # scored 0.559023 against a real one scoring 0.559047 (docs/384), i.e. the number was the mean-luminance floor.
        print(f"floor control: the same alignment against grids zeroed scores {fmean:.4f} "
              f"(the route score is {(mean - fmean) / max(1e-9, 1.0 - fmean) * 100:.1f}% of the way from floor to identical)")
    changed_a = sum(1 for x in A if x["changed"])
    changed_b = sum(1 for x in B if x["changed"])
    fa = (A[-1]["t"] - A[0]["t"]) / 1000.0
    fb = (B[-1]["t"] - B[0]["t"]) / 1000.0
    print(f"orig frames={len(A)} distinct_pictures={changed_a} rate={len(A)/max(.001,fa):.1f}/s")
    print(f"port frames={len(B)} distinct_pictures={changed_b} rate={len(B)/max(.001,fb):.1f}/s")
    print(f"aligned pairs={len(pairs)} route score={mean:.4f} (1 = identical)")
    bad = runs(pairs, A, a.thresh)
    attrib = attribution(a.orig_run, bad) if a.orig_run else {}
    print(f"divergent runs (score < {a.thresh}): {len(bad)}")
    for t0, t1, worst in bad[:a.max_runs]:
        print(f"  orig t={t0}..{t1} ms ({(t1-t0)/1000.0:.1f}s) worst score={worst:.3f}")
        for line in attrib.get((t0, t1, worst), [])[:4]:
            print(f"      attributed: {line}")
    # pacing premise (docs/82): same route, same pinned cycles -> comparable rates and picture counts
    # The capture rate is our poll rate and the port's is its presentation rate, so those are not
    # comparable; pictures-that-actually-change per second is, and that is the pacing premise: the
    # game paces itself off the retrace/timer (docs/47, docs/82), so a route should change picture at
    # a comparable rate on both sides.
    ca, cb = changed_a / max(.001, fa), changed_b / max(.001, fb)
    pr = ca / max(.0001, cb)
    print(f"pacing check (docs/47, docs/82): picture changes per second orig={ca:.2f} port={cb:.2f} "
          f"ratio={pr:.2f}" + ("  <- pacing or slide-count divergence" if not (0.7 < pr < 1.4) else ""))
    print(f"  (raw capture/present rate orig={len(A)/max(.001,fa):.1f}/s port={len(B)/max(.001,fb):.1f}/s "
          f"-- not comparable, listed for the record)")
    # channel-order premise (docs/70, docs/79): re-score aligned pairs from witness frames under both
    # hypotheses; a channel-order bug must show up as "the other order scores better".
    ch = channel_check(a.orig, a.port, pairs)
    if ch:
        declared, swapped, chroma = ch
        if chroma < 8:
            print(f"channel-order check: inconclusive (sampled frames are near-greyscale, "
                  f"mean |R-B|={chroma:.1f}); declared={declared:.3f} swapped={swapped:.3f}")
        else:
            print(f"channel-order check: declared={declared:.3f} R/B-swapped={swapped:.3f} "
                  f"(chroma {chroma:.1f})"
                  + ("  <- channel order is wrong somewhere" if swapped > declared + 0.02 else "  ok"))
    if a.md:
        os.makedirs(os.path.dirname(a.md) or ".", exist_ok=True)
        with open(a.md, "w") as f:
            f.write("# screen-oracle scores\n\n")
            f.write(f"- orig: {len(A)} frames, {changed_a} distinct, {fa:.1f} s\n")
            f.write(f"- port: {len(B)} frames, {changed_b} distinct, {fb:.1f} s\n")
            f.write(f"- route score {mean:.4f}; aligned pairs {len(pairs)}\n")
            f.write(f"- picture changes per second: orig {ca:.2f}, port {cb:.2f} (ratio {pr:.2f})\n")
            if ch:
                f.write(f"- channel-order check: declared {ch[0]:.3f} vs R/B-swapped {ch[1]:.3f}\n")
            f.write("\n")
            f.write("| orig t0 | orig t1 | worst score |\n|---|---|---|\n")
            for t0, t1, worst in bad:
                f.write(f"| {t0} | {t1} | {worst:.3f} |\n")
        print(f"wrote {a.md}")


if __name__ == "__main__":
    main()
