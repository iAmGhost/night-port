#!/usr/bin/env python3
"""Item 4 (docs/217): does the level-up / message presentation advance without a keypress?

Both sides are measured the same way: locate the DS-injection that forces the level-up transition
(DAT_0006336c <- 2, consumed at guest 0x0003ac2a -> fn_03d244), take the confirm press as the event
anchor, then census the PICTURE STABILITY of the key-free window that follows, using the 40x30
mean-luminance signature every capture script already stores per frame.

  distinct signatures      -- how many different pictures appear with no key event
  longest identical run    -- how long one picture is held
  changed-frame rate       -- pictures per second the capture says changed

A presentation that waits for a key holds one picture (1 distinct, long run); a presentation that
advances on its own keeps producing new pictures (many distinct, short runs).

Usage: keywait.py --port <route>/port --orig <route>/orig_screen [--orig-state <route>/orig]
                  [--lead 4000] [--window 15000]
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cells_reader import CellStream                      # noqa: E402
from diff_screen import read_frames                      # noqa: E402

LATCH = 0x6336C


def latch_ms_port(port_dir):
    cs = CellStream(port_dir)
    row = cs.by_name.get("g_dat_6336c")
    if row is None:
        return None, 0, 0
    c = row["ds"]
    nz = [(cs.t_ms(i), cs.val(i, c)) for i in range(cs.samples) if cs.val(i, c)]
    if not nz:
        return None, cs.samples, 0
    return nz[0][0], cs.samples, len(nz)


def latch_ms_orig_capture(log_path):
    """capture_original.py prints 'poke 6336c <- 2 : read back 2 at t=NNN ms' for every injection it runs."""
    log = log_path
    if not os.path.exists(log):
        return None
    for ln in open(log):
        if "read back" in ln and "poke" in ln:
            return int(ln.split("t=")[1].split(" ms")[0])
    return None


def census(path, t0, t1, label):
    F = [f for f in read_frames(path) if t0 <= f["t"] <= t1]
    if not F:
        print(f"{label}: no frames in [{t0}, {t1}]")
        return
    sigs = [bytes(f["sig"]) for f in F]
    distinct, longest, cur, run = set(), 0, sigs[0], 1
    prev = sigs[0]
    distinct.add(prev)
    for s in sigs[1:]:
        distinct.add(s)
        run = run + 1 if s == prev else 1
        longest = max(longest, run)
        prev = s
    changed = sum(1 for f in F if f["changed"])
    span = (F[-1]["t"] - F[0]["t"]) / 1000.0
    geoms = {}
    for f in F:
        geoms[(f["w"], f["h"])] = geoms.get((f["w"], f["h"]), 0) + 1
    print(f"{label}: frames={len(F)} distinct={len(distinct)} longest_run={longest} "
          f"changed={changed} ({changed / max(span, 0.001):.2f}/s) span={span:.1f}s geoms={geoms}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--orig", required=True)
    ap.add_argument("--orig-state", default="")
    ap.add_argument("--orig-log", default="", help="route's capture.log, for the poke line")
    ap.add_argument("--press-offset", type=int, default=4000,
                    help="ms from the latch injection to the confirm press (drive nominal: 94000 -> 98000)")
    ap.add_argument("--window", type=int, default=15000, help="key-free window length after the press, ms")
    a = ap.parse_args()

    pt, psamples, pnz = latch_ms_port(a.port)
    print(f"port latch (DS {LATCH:#x}): t={pt} ms  samples={psamples} nonzero={pnz}")
    if pt is None:
        sys.exit("port latch never observed non-zero")
    census(os.path.join(a.port, "frames.bin"), pt + a.press_offset + 1000,
           pt + a.press_offset + a.window, "PORT key-free window after press")
    census(os.path.join(a.port, "frames.bin"), pt + 1000, pt + a.press_offset - 500,
           "PORT key-free window before press")

    ot = None
    tl = os.path.join(a.orig, "timeline.json")
    if os.path.exists(tl):
        tlj = json.load(open(tl))
        for p in tlj.get("pokes", []) or []:
            if p.get("read_back"):
                ot = p["t_ms"]
                break
    if ot is None and a.orig_state and os.path.exists(os.path.join(a.orig_state, "meta.json")):
        mj = json.load(open(os.path.join(a.orig_state, "meta.json")))
        for p in mj.get("pokes", []) or []:
            if p.get("read_back"):
                ot = p["t_ms"]
                print(f"orig latch taken from the state replay meta: {p}")
                break
    if ot is None and a.orig_log:
        ot = latch_ms_orig_capture(a.orig_log)
    print(f"orig latch (DS {LATCH:#x}): t={ot} ms")
    if ot is None:
        sys.exit("shipped latch never observed; check the capture log for the poke line")
    census(os.path.join(a.orig, "frames.bin"), ot + a.press_offset + 1000,
           ot + a.press_offset + a.window, "SHIPPED key-free window after press")
    census(os.path.join(a.orig, "frames.bin"), ot + 1000, ot + a.press_offset - 500,
           "SHIPPED key-free window before press")


if __name__ == "__main__":
    main()
