#!/usr/bin/env python3
"""card_hold.py -- does a card/picture HOLD, or does the picture churn with no key event?

Item 4's oracle (docs/231).  The shipped's cards are gated: `fn_035f1b(name, dl)` presents one picture and
`fn_00035d5d` (0x00035d5d, three phases over the four press latches) consumes one press-and-release before the
next picture (docs/97's `fn_00035dfc` is the sibling gate used by the slide-show).  A build that no-ops the gate
keeps advancing pictures on its own, which is what a *picture-run* census sees: the longest run of identical
frames inside a key-free window, and how many new pictures appear per second.

Runs are paired with a drive file so the key timeline is known: a window is only key-free because the drive says
so, and the window boundaries are taken from the drive's own key lines.

  python3 scripts/rig/card_hold.py --dump /tmp/rig/kw_after --drive scripts/rig/drives/ep1_keywait.txt \
      --from 100.0 --to 125.0
"""
import argparse, os, re, struct, sys

MAGIC = 0x5247464E          # 'NGFR'


def frames(path):
    """Yield (t_ms, frame_no, w, h, changed, signature_hash) from a frames.bin dump."""
    import hashlib
    with open(path, "rb") as fh:
        head = fh.read(12)
        if len(head) < 12:
            raise SystemExit(f"{path}: too short to be a frames.bin")
        magic, ver, sigsz = struct.unpack("<III", head)
        if magic != MAGIC:
            raise SystemExit(f"{path}: bad magic {magic:#x} (expected NGFR)")
        esz = 32 + sigsz
        while True:
            e = fh.read(esz)
            if len(e) < esz:
                return
            t_ms, frame, w, h, changed, _pad = struct.unpack("<qQiiii", e[:32])
            yield t_ms, frame, w, h, changed, hashlib.md5(e[32:32 + sigsz]).hexdigest()


def key_windows(drive):
    """[(t_start_s, t_end_s, n_keys)] from the drive's `key ... down` lines, plus the whole span."""
    lines = []
    for ln in open(drive, errors="replace"):
        m = re.match(r"\s*(\d+)\s+key\s+(\S+)\s+(down|up)", ln)
        if m:
            lines.append((int(m.group(1)) / 1000.0, m.group(2), m.group(3)))
    downs = [t for t, _n, d in lines if d == "down"]
    out = []
    for i, t in enumerate(downs):
        nxt = downs[i + 1] if i + 1 < len(downs) else None
        out.append((t, nxt, i + 1))
    return out, (downs[0] if downs else None, downs[-1] if downs else None)


def report(dump, lo, hi, label):
    fr = [f for f in frames(os.path.join(dump, "frames.bin")) if lo * 1000 <= f[0] <= hi * 1000]
    if not fr:
        print(f"  {label}: no samples in {lo}-{hi} s")
        return None
    runs, best, same_pic = [], 0, 0
    cur = None
    for t_ms, frame, w, h, ch, sig in fr:
        if cur is not None and sig == cur:
            same_pic += 1
        else:
            runs.append((cur, same_pic))
            cur, same_pic = sig, 1
    runs.append((cur, same_pic))
    best = max(r[1] for r in runs)
    span = (fr[-1][0] - fr[0][0]) / 1000.0
    nrun = len(runs)
    print(f"  {label}: window {lo}-{hi} s  samples={len(fr)} span={span:.1f}s  "
          f"distinct_picture_runs={nrun}  longest_hold={best} samples "
          f"({best * (span / len(fr)):.2f} s)  new_pictures/s={nrun / span:.2f}  "
          f"changed-flag samples={sum(1 for f in fr if f[4])}")
    return dict(samples=len(fr), runs=nrun, longest=best, per_sec=nrun / span)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dump", required=True)
    ap.add_argument("--drive")
    ap.add_argument("--from", dest="lo", type=float)
    ap.add_argument("--to", dest="hi", type=float)
    ap.add_argument("--all-key-free", action="store_true",
                    help="report every gap between drive key-down events instead of one window")
    a = ap.parse_args()
    if a.all_key_free:
        if not a.drive:
            raise SystemExit("--all-key-free needs --drive")
        wins, span = key_windows(a.drive)
        print(f"drive {a.drive}: {len(wins)} presses, span {span[0]}-{span[1]} s")
        for i, (t0, t1, n) in enumerate(wins):
            hi = t1 if t1 is not None else (span[1] + 30.0)
            if hi - t0 < 2.0:
                continue
            report(a.dump, t0 + 1.0, hi, f"after press #{n} (key-free {t0 + 1.0:.1f}-{hi:.1f} s)")
    else:
        if a.lo is None or a.hi is None:
            raise SystemExit("give --from and --to, or --all-key-free with --drive")
        report(a.dump, a.lo, a.hi, "window")


if __name__ == "__main__":
    main()
