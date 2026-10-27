#!/usr/bin/env python3
"""Original-side screen capture: per-frame signatures on the emulator's own frame stream.

Emitted in the same record layout as the port's rig_sink frames.bin (scripts/rig/diff_screen.py reads
either), so the screen oracle compares like with like: a 40x30 mean-luminance grid per frame.

  python3 scripts/rig/capture_original.py --out DIR [--seconds 60] [--hz 60] [--drive FILE]

Without --drive it only watches; with --drive it releases the same input stream replay_original.py
uses, so a capture can be a full route.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))

GRID = 40 * 30
REC = struct.Struct("<qQiiii")     # t_ms, frame, w, h, changed, pad -- 32 B, rig_sink.c layout


def api(base, token, path, method="GET", body=None, timeout=30):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(base + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + token,
                                          "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read()


KEYNAME = {"enter": "KBD_enter", "return": "KBD_enter", "space": "KBD_space", "esc": "KBD_escape",
           "tab": "KBD_tab", "up": "KBD_up", "down": "KBD_down", "left": "KBD_left",
           "right": "KBD_right", "ctrl": "KBD_leftctrl", "shift": "KBD_leftshift",
           "alt": "KBD_leftalt", "backspace": "KBD_backspace"}


def drive_events(path):  # kept for drives without DS waits
    ev = []
    for line in open(path):
        f = line.split("#")[0].split()
        if len(f) >= 4 and f[1] == "key":
            ev.append({"t": int(f[0]), "type": "key", "key": KEYNAME.get(f[2].lower(), f[2]),
                       "pressed": f[3].startswith("d")})
    return ev


def signature(raw, info):
    """40x30 mean-luminance grid from whatever the emulator says the frame is."""
    w, h, pitch = info["width"], info["height"], info["pitch"]
    px = raw[15:]                                        # 15-byte header: w,h,pitch,bpp,...
    bpp = info.get("pixel_format", "")
    pal = None
    if info.get("is_paletted"):
        need = pitch * h
        if len(px) >= need + 768:
            pal = px[need:need + 768]
    sig = bytearray(GRID)
    for gy in range(30):
        y = gy * h // 30
        row = y * pitch
        for gx in range(40):
            x = gx * w // 40
            if info.get("is_paletted"):
                idx = px[row + x]
                if pal:
                    r, g, b = pal[idx * 3], pal[idx * 3 + 1], pal[idx * 3 + 2]
                else:
                    r = g = b = idx                  # documented degradation: no palette in the reply
            elif "BGRX" in bpp:
                o = row + x * 4
                b, g, r = px[o], px[o + 1], px[o + 2]
            else:                                    # RGBX / RGBA
                o = row + x * 4
                r, g, b = px[o], px[o + 1], px[o + 2]
            sig[gy * 40 + gx] = (r * 77 + g * 150 + b * 29) >> 8
    return bytes(sig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--base", default="http://localhost:8386")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--seconds", type=float, default=60.0)
    ap.add_argument("--hz", type=float, default=30.0)
    ap.add_argument("--drive")
    ap.add_argument("--witness-every", type=int, default=10)
    ap.add_argument("--anchor-stable", type=float, default=3.0)
    ap.add_argument("--ds-base", type=lambda x: int(x, 0), default=None,
                    help="physical address of DS offset 0 (from calibrate_ds.py); required when the "
                         "drive uses `wait ds`")
    ap.add_argument("--anchor-wait", type=float, default=90.0)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    token = open(a.token_file).read().strip()

    out = open(os.path.join(a.out, "frames.bin"), "wb")
    out.write(struct.pack("<III", 0x5247464E, 1, GRID))
    # Same anchoring rule as replay_original.py: the input timeline is released when the picture stops
    # changing, not when the container happened to start. Without it the capture session is a different
    # game situation from the replay session, which is what made the pacing metric swing.
    segments = []
    phys_base = a.ds_base
    if phys_base is None:
        try:
            sys.path.insert(0, HERE)
            from replay_original import load_ds_base
            phys_base = load_ds_base()
        except Exception:
            phys_base = None
    if a.drive:
        sys.path.insert(0, HERE)
        from replay_original import parse_drive, parse_events, ds_cond
        segments = parse_events(parse_drive(a.drive))
        if any(sg["wait"] for sg in segments):
            if phys_base is None:
                sys.exit("the drive uses `wait ds`, so this session needs a calibrated DS base "
                         "(run calibrate_ds.py first, or pass --ds-base)")
    if a.drive:
        stable, limit = a.anchor_stable, a.anchor_wait
        last_h, last_change, t_wait = None, time.time(), time.time()
        while time.time() - t_wait < limit:
            raw = api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)
            h = hashlib.sha1(raw).hexdigest()
            if last_h == h and time.time() - last_change >= stable:
                break
            if h != last_h:
                last_h, last_change = h, time.time()
            time.sleep(0.2)
        else:
            print("anchor not reached; capturing anyway", file=sys.stderr)
    t0 = time.time()
    seg_i, shift = 0, 0

    def post_segment(i):
        now = int((time.time() - t0) * 1000)
        evs = [{"t": max(0, ev["t"] + shift - now), "type": "key", "key": ev["key"],
                "pressed": ev["pressed"]} for ev in segments[i]["events"]]
        if evs:
            api(a.base, token, "/api/v1/input/sequence", "POST", {"events": evs})

    if a.drive:
        post_segment(0)
    prev, n, witness, frames_dir = None, 0, 0, os.path.join(a.out, "frames")
    sha_seen = {}
    while time.time() - t0 < a.seconds:
        raw = api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)
        info = json.loads(api(a.base, token, "/api/v1/video/frame/info"))
        sig = signature(raw, info)
        changed = sig != prev
        now_ms = int((time.time() - t0) * 1000)
        out.write(REC.pack(now_ms, n, info["width"], info["height"], 1 if changed else 0, 0))
        out.write(sig)
        out.flush()
        if changed and a.witness_every and n % a.witness_every == 0:
            os.makedirs(frames_dir, exist_ok=True)
            fp = os.path.join(frames_dir, f"{n:06d}_{info['width']}x{info['height']}.bin")
            with open(fp, "wb") as f:
                f.write(raw)
            sha_seen[fp] = hashlib.sha1(raw).hexdigest()[:12]
        prev, n = sig, n + 1
        time.sleep(1.0 / a.hz)
    out.close()
    print(f"{n} frames -> {a.out}/frames.bin ({len(sha_seen)} witnesses)")


if __name__ == "__main__":
    main()
