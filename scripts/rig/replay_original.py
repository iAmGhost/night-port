#!/usr/bin/env python3
"""Replay a fixed input sequence against the original and record its state sequence.

Input goes in through one /api/v1/input/sequence POST: the emulator schedules the key edges on its
own PIC timeline, so every run sees the *same* input stream, independent of polling jitter. Polling
only decides when we look, so the recorded object is the sequence of *distinct states* rather than a
wall-clock trace (docs/56: align on content, not on frame numbers).

State per sample = the whole DS group (.object2, 0x50000..0x650bf) read from physical memory at the
calibrated DS base, plus the framebuffer hash and video mode from /video/frame/info. A new state is
recorded whenever the DS digest or the frame hash changes.

  python3 scripts/rig/replay_original.py --drive DRIVE --out DIR [--hz 12] [--seconds 90]

Drive file:
  <t_ms> key <KBD_name> down|up        input edge on the replay timeline
  anchor static <stable_s> <max_wait_s>   wait until the framebuffer stops changing, then start
  anchor frame <sha1-prefix> <max_wait_s> wait for a specific frame, then start
The anchor is what makes a replay repeatable: the input timeline is released at a game-defined
moment, not at whatever wall-clock the host happened to reach (docs/56).
"""
import argparse
import hashlib
import json
import os
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
DS_LO, DS_HI = 0x50000, 0x66000
MASK = os.path.join(HERE, "..", "audit", "gen", "ds_mask.bin")


def masked_digest(ds, mask):
    """Digest over the cells the guest itself touches: stack padding and untouched holes must not
    make a state look different (scripts/audit/gen_headers.py builds the mask)."""
    return hashlib.sha1(bytes(b for b, m in zip(ds, mask) if m)).hexdigest()
CHUNK = 0x4000


def api(base, token, path, method="GET", body=None, timeout=30):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(base + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + token,
                                          "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


def load_ds_base():
    p = os.path.join(HERE, "gen", "ds_base.json")
    if not os.path.exists(p):
        sys.exit("run scripts/rig/calibrate_ds.py first")
    return json.load(open(p))["phys_ds_base"]


def read_ds(base, token, phys_base):
    out = b""
    for a in range(DS_LO, DS_HI, CHUNK):
        n = min(CHUNK, DS_HI - a)
        out += api(base, token, f"/api/v1/memory/{phys_base + a}/{n}", timeout=60)
    return out


def parse_drive(path):
    steps = []
    for line in open(path):
        line = line.split("#")[0].strip()
        if line:
            steps.append(line.split())
    return steps


def parse_events(steps):
    events = []
    for f in steps:
        if f[1] != "key":
            continue
    for f in steps:
        if f[1] == "key":
            press = f[3].startswith("d")
            events.append({"t": int(f[0]), "type": "key", "key": f[2], "pressed": press})
        elif f[0] != "anchor":
            sys.exit(f"unknown drive step: {' '.join(f)}")
    return events


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--drive", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--base", default="http://localhost:8386")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--hz", type=float, default=4.0)
    ap.add_argument("--seconds", type=float, default=90.0)
    ap.add_argument("--settle", type=float, default=3.0, help="seconds before the input timeline starts")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    token = open(a.token_file).read().strip()
    phys_base = load_ds_base()
    mask = open(MASK, "rb").read()
    if len(mask) != DS_HI - DS_LO:
        sys.exit("ds_mask.bin size mismatch")

    st = json.loads(api(a.base, token, "/api/v1/status"))
    if not st.get("running"):
        sys.exit("emulator not running")
    cyc = json.loads(api(a.base, token, "/api/v1/cpu/cycles"))
    if cyc.get("auto_adjust"):
        sys.exit("cycles are not pinned (auto_adjust=true) -> bringup with a pinned conf")

    steps = parse_drive(a.drive)
    events = parse_events(steps)

    # --- anchor: release the input timeline at a game-defined moment -------------
    anchor = None
    for f in steps:
        if f[1] != "anchor":
            continue
        kind, stable, limit = f[1], float(f[2]), float(f[3])
        t_wait, last_h, last_change = time.time(), None, time.time()
        while time.time() - t_wait < limit:
            h = hashlib.sha1(api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)).hexdigest()
            if kind == "static" and last_h == h and time.time() - last_change >= stable:
                anchor = {"kind": kind, "frame_sha": h, "wait_s": round(time.time() - t_wait, 3)}
                break
            if kind == "frame" and h.startswith(f[2]):
                anchor = {"kind": kind, "frame_sha": h, "wait_s": round(time.time() - t_wait, 3)}
                break
            if h != last_h:
                last_h, last_change = h, time.time()
            time.sleep(0.2)
        if anchor is None:
            sys.exit(f"anchor {kind} not reached within {limit}s")
    t0 = time.time()
    api(a.base, token, "/api/v1/input/sequence", "POST", {"events": events})

    states, prev = [], None
    last_ds = None
    while time.time() - t0 < a.seconds:
        ds = read_ds(a.base, token, phys_base)
        fr = api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)
        info = json.loads(api(a.base, token, "/api/v1/video/frame/info"))
        dsh, frh = masked_digest(ds, mask), hashlib.sha1(fr).hexdigest()
        now = time.time() - t0
        if prev != (dsh, frh):
            states.append({"t_wall": round(now, 3), "ds_sha": dsh, "frame_sha": frh,
                           "mode": info.get("video_mode"), "w": info.get("width"),
                           "h": info.get("height"), "changed_ds": last_ds != dsh})
            last_ds = dsh
            with open(os.path.join(a.out, f"state_{len(states) - 1:04d}.ds.bin"), "wb") as f:
                f.write(ds)
            with open(os.path.join(a.out, f"state_{len(states) - 1:04d}.frame.bin"), "wb") as f:
                f.write(fr)
            prev = (dsh, frh)
        time.sleep(1.0 / a.hz)

    meta = {"drive": os.path.abspath(a.drive), "events": events, "anchor": anchor, "hz": a.hz,
            "seconds": a.seconds, "states": len(states), "cycles": cyc,
            "timeline": states}
    json.dump(meta, open(os.path.join(a.out, "timeline.json"), "w"), indent=1)
    print(f"{len(states)} distinct states over {a.seconds}s -> {a.out}/timeline.json")


if __name__ == "__main__":
    main()
