#!/usr/bin/env python3
"""Read a Pocket log apart by display branch, and say which branch the wall went into.

WHY THIS EXISTS.  `frames=` counts flips, so a log read as one number cannot tell a 640x480 slide from the
320x200 game view -- and the two paths have costs an order of magnitude apart (25 ms and 4 ms a present,
measured).  `br n/g/d/o` counts presents per branch, so the branch deltas per window separate them, and a
"it got better / it got worse" report becomes a comparison of medians instead of a story.  This is the
reader for that, written while adjudicating docs/456 9; keeping it as a scratch heredoc is how a claim
like "the fade is 13 fps" got into a doc without a reproducible command behind it.

    scripts/pocket/branchfps.py scratch/pocket/logs/*0.3.5*.log

Two traps it prints rather than hides:

  - `rest` is CLAMPED.  It is `wall - (pres+pump+disc+aud)`, floored at 0, in a CUMULATIVE counter, and the
    buckets are not disjoint (a CD-DA SD read sits inside the pump's span).  So a window with music can push
    the sum past the wall, after which rest freezes and its window delta is meaningless.  0.3.5 logs carry
    `acc=`; a window with acc - dt >= 0 is reported as CLAMPED.
  - a `hold` (the stage-entry card) legitimately shows sub-1 fps.  Windows whose `hold` is non-zero are
    labelled HOLD so they stop being read as a stutter.
"""
import re
import sys

SLOW_MS = 25.0          # fps under which a window is worth printing in full
PERF_BRANCH = ("n", "g", "d", "o")


def first(pat, line, default=0):
    m = re.search(pat, line)
    return int(m.group(1)) if m else default


def windows(path):
    """One record per `split wall` window: its deltas and the branch counters around it."""
    out = []
    prev = None
    perf = None
    hist = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.search(r"hist <20/(\d+) 20-40/(\d+) 40-80/(\d+) 80-160/(\d+) 160\+/(\d+)", line)
        if m:                                   # printed just before its split wall, so it belongs to the
            hist = [int(x) for x in m.groups()]  # window that follows it
        if line.startswith("perf "):
            perf = tuple(first(p, line, -1) for p in (
                r"hash (\d+) smp", r"br n/g/d/o=(\d+)/", r"br n/g/d/o=\d+/(\d+)/",
                r"br n/g/d/o=\d+/\d+/(\d+)/", r"br n/g/d/o=\d+/\d+/\d+/(\d+)", r"testless=(\d+)"))
        m = re.search(r"split wall=(\d+)ms pump=(\d+)ms pres=(\d+)ms disc=(\d+)ms"
                      r" aud=(\d+)ms rest=(\d+)ms(?: acc=(\d+)ms)? frames=(\d+)"
                      r".*?\bhold=(\d+)ms", line)
        hold = 0
        if m:
            hold = int(m.group(9))
        else:   # a log from before the hold witness: read the same line without it
            m = re.search(r"split wall=(\d+)ms pump=(\d+)ms pres=(\d+)ms disc=(\d+)ms"
                          r" aud=(\d+)ms rest=(\d+)ms(?: acc=(\d+)ms)? frames=(\d+)", line)
        if not m:
            continue
        v = [int(x) if x is not None else -1 for x in m.groups()[:8]]
        cur = dict(zip(("wall", "pump", "pres", "disc", "aud", "rest", "acc", "frames"), v),
                    perf=perf, hold=hold, hist=hist)
        hist = None
        if prev:
            dt = cur["wall"] - prev["wall"]
            if dt > 0:
                cur["dt"] = dt
                for k in ("pres", "pump", "disc", "aud", "rest", "acc", "frames"):
                    cur["d_" + k] = (cur[k] - prev[k]) if cur[k] >= 0 and prev[k] >= 0 else None
                cur["br"] = [cur["perf"][i + 1] - prev["perf"][i + 1]
                             for i in range(len(PERF_BRANCH))] if cur["perf"] and prev["perf"] else [0] * 4
                cur["testless"] = (cur["perf"][5] - prev["perf"][5]) if cur["perf"] and prev["perf"] \
                    and cur["perf"][5] >= 0 and prev["perf"][5] >= 0 else 0
                cur["fps"] = cur["d_frames"] * 1000.0 / dt
                out.append(cur)
        prev = cur
    return out


def med(xs):
    xs = sorted(xs)
    return xs[len(xs) // 2] if xs else 0.0


def report(path):
    w = windows(path)
    print(f"\n=== {path.split('/')[-1]}   ({len(w)} windows)")
    if not w:
        print("    no split-wall windows -- a pre-acc= log still reads, but see the CLAMPED caveat")
        return
    for i, name in enumerate(PERF_BRANCH):
        sel = [d for d in w if d["br"][i] >= 25 and d["d_frames"] and d["d_frames"] > 0]
        if not sel:
            continue
        fps = [d["fps"] for d in sel]
        per = [d["d_pres"] / d["d_frames"] for d in sel]
        au = [100.0 * ((d["d_aud"] or 0) + (d["d_disc"] or 0)) / d["dt"] for d in sel]
        print(f"    br {name}: {len(sel):2} win  fps med {med(fps):5.1f} "
              f"[{min(fps):5.1f}-{max(fps):5.1f}]  pres/frame med {med(per):5.1f} ms  "
              f"aud+disc med {med(au):5.1f}%")
    tl = sum(d["testless"] for d in w)
    if tl:
        print(f"    PRESENT_TESTLESS fired {tl} frames this run")
    gap_shape(w)
    slow = [d for d in w if d["fps"] < SLOW_MS]
    print(f"    windows below {SLOW_MS:.0f} fps: {len(slow)}")
    for d in slow:
        tags = []
        if d["hold"]:
            tags.append("HOLD")
        if d["d_acc"] is not None and d["d_acc"] >= d["dt"]:
            tags.append("CLAMPED(rest meaningless)")
        au = 100.0 * ((d["d_aud"] or 0) + (d["d_disc"] or 0)) / d["dt"]
        pr = 100.0 * (d["d_pres"] or 0) / d["dt"]
        rs = 100.0 * (d["d_rest"] or 0) / d["dt"]
        br = "/".join(str(x) for x in d["br"])
        hh = "  hist " + " ".join(f"{lab}/{v}" for lab, v in
                                  zip(("<20", "20-40", "40-80", "80-160", "160+"), d["hist"])) if d["hist"] else ""
        print(f"       +{d['wall']:>6}ms {d['dt']/1000:4.1f}s {d['fps']:5.1f} fps  br n/g/d/o={br:15}"
              f"  pres {pr:5.1f}%  aud+disc {au:6.1f}%  rest {rs:5.1f}%   {' '.join(tags)}{hh}")


def gap_shape(w):
    """The gap histogram's own verdict (docs/456 10): a stutter should be BIMODAL, a slow machine unimodal.

    `gap worst` cannot separate 15 ms and 50 ms frames arriving alternately from a steady 30 ms, so the
    histogram was built to say which one a second is.  Two cheap readings, both printed rather than
    judged: the run's aggregate shape, and the longest run of windows sitting on the 12-15 fps band --
    because the boot the player called stuttering held nine of those in a row (13.3/13.7 alternating)
    while the smooth one held none, and a periodic bad rate is a different complaint than a slow one.
    """
    hs = [d["hist"] for d in w if d["hist"]]
    if hs:
        agg = [sum(h[i] for h in hs) for i in range(5)]
        tot = sum(agg) or 1
        print("    gap hist, whole run:  " + "  ".join(
            f"{lab} {v:5} ({100.0 * v / tot:4.1f}%)" for lab, v in
            zip(("<20ms", "20-40", "40-80", "80-160", "160+"), agg)))
        bimodal = [d for d in w if d["hist"] and sum(d["hist"][2:]) >= 3 and d["hist"][0] >= 5]
        quiet = [d for d in bimodal if not d["hold"]]
        print(f"    bimodal windows (>=5 frames under 40 ms AND >=3 over 40 ms): {len(bimodal)} of {len(hs)},"
              f" {len(quiet)} of them WITHOUT a hold")
        if bimodal and not quiet:
            print("      every bimodal window here is a HOLD: the >40 ms tail is the stage-entry card doing"
                  " its job, not a hitch. A threshold that fires on the smoothest boot on the record is the"
                  " wrong threshold (docs/456 11).")
    best = run = 0
    for d in w:
        run = run + 1 if 12.5 <= d["fps"] < 14.5 else 0
        best = max(best, run)
    print(f"    longest run of 12-15 fps windows: {best}   (a periodic judder; the stutter boot measured 9)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        report(p)
