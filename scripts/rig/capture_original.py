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


# docs/181 §6: this function crashed mid-route with "IndexError: index out of range" while reading a
# non-paletted frame, because it indexed as if every frame were 4 bytes per pixel.  The route changes VGA mode
# (the game runs mode 13h for graphics and mode 3 for the text banner), and a mode-3 reply is not 4 bytes per
# pixel; the first frames succeeded and the mode change is what killed the session.  A rig that dies at a mode
# transition silently drops the second half of a capture session, which is exactly how a "no audio report" and a
# half-length pacing number ended up in g33's report.
BPP_OF = {"BGRX": 4, "RGBX": 4, "RGBA": 4, "ARGB": 4, "ABGR": 4, "XRGB": 4, "XBGR": 4,
          "RGB565": 2, "BGR565": 2, "565": 2, "INDEX8": 1, "P8": 1, "INDEX": 1, "PALETTE8": 1}


def frame_geom(info, n_px):
    """Bytes per pixel, taken from the reply and cross-checked against the payload actually delivered."""
    w, h, pitch = info["width"], info["height"], info["pitch"]
    # `pitch` is BYTES per row in this reply (the paletted path has always indexed that way), so the payload a
    # frame needs is pitch*h -- NOT pitch*h*bpp.  Getting this wrong turns a valid 32-bit frame into a bogus
    # 8-bit one, which is how a first version of this validator mangled a mode-13h grid into zeros.
    fmt = (info.get("pixel_format") or "").upper().replace(" ", "")
    bpp = 1 if info.get("is_paletted") else BPP_OF.get(fmt)
    inferred = False
    if bpp and pitch < w * bpp:                       # the format claim contradicts the row stride
        bpp = None
    if bpp is None:
        fits = [b for b in (4, 2, 1) if pitch >= w * b]
        bpp = max(fits) if fits else 0
        inferred = bpp > 0
    if n_px < pitch * h or bpp == 0:
        return w, h, pitch, 0, fmt, inferred
    return w, h, pitch, bpp, fmt, inferred


def signature(raw, info):
    """40x30 mean-luminance grid from whatever the emulator says the frame is."""
    px = raw[15:]                                        # 15-byte header: w,h,pitch,bpp,...
    w, h, pitch, bpp, bpp_s, inferred = frame_geom(info, len(px))
    if bpp == 0:
        raise ValueError(
            "frame geometry is not self-consistent: %dx%d pitch=%d format=%r paletted=%r, %d payload bytes "
            "(no bytes-per-pixel of 4/2/1 fits the stride, or the payload is shorter than pitch*height)"
            % (w, h, pitch, info.get("pixel_format"), info.get("is_paletted"), len(px)))
    pal = None
    if bpp == 1:
        need = pitch * h
        if len(px) >= need + 768:
            pal = px[need:need + 768]
    sig = bytearray(GRID)
    for gy in range(30):
        y = gy * h // 30
        row = y * pitch
        for gx in range(40):
            x = gx * w // 40
            if bpp == 1:
                idx = px[row + x]
                if pal:
                    r, g, b = pal[idx * 3], pal[idx * 3 + 1], pal[idx * 3 + 2]
                else:
                    r = g = b = idx                  # documented degradation: no palette in the reply
            elif bpp == 2:                           # 565: 5-6-5, order as delivered
                # `row` is already y*pitch here -- multiplying again walks off the end of the payload,
                # the same class of error as reading the wrong blob in docs/167 §19.
                v = px[row + x * 2] | (px[row + x * 2 + 1] << 8)
                r, g, b = (v >> 11) << 3, ((v >> 5) & 0x3F) << 2, (v & 0x1F) << 3
            elif "BGRX" in bpp_s:
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
    ap.add_argument("--io", default="", help="comma-separated I/O ports to census every tick, "
                                             "e.g. 0x02,0x03,0x83,0x0a (docs/165 §9)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    token = open(a.token_file).read().strip()

    # docs/165 §9: I/O-port sampling.  The shipped side's audio protocol is visible on the DMA controller
    # (fn_040b83 writes the 24-bit transfer address to 0x02/0x03, the page to 0x83, and masks channel 1 on
    # 0x0a), so sampling THOSE ports censuses the shipped side's transfers without touching the game.
    # 0x22e (the SB read port) is deliberately NOT sampled: reading it pops the DSP acknowledge byte the
    # game itself is waiting on, which would make the oracle change the thing it measures.
    ports = [x.strip() for x in a.io.split(",") if x.strip()]
    io_rows = None
    if ports:
        io_rows = open(os.path.join(a.out, "io.jsonl"), "w")
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
    last_geom = None
    prev_io, io_ticks, io_changes = {}, 0, {}
    sha_seen = {}
    while time.time() - t0 < a.seconds:
        raw = api(a.base, token, "/api/v1/video/frame?format=raw", timeout=60)
        info = json.loads(api(a.base, token, "/api/v1/video/frame/info"))
        geom = frame_geom(info, len(raw) - 15)
        if geom != last_geom:
            print("mode change: %dx%d pitch=%d format=%r paletted=%r -> %d B/px%s, %d payload bytes"
                  % (geom[0], geom[1], geom[2], info.get("pixel_format"), info.get("is_paletted"),
                     geom[3], " (inferred from the stride, the reply's format claim does not fit it)"
                     if geom[5] else "", len(raw) - 15))
            last_geom = geom
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
        if io_rows is not None:
            vals = {}
            for pt in ports:
                try:
                    vals[pt] = json.loads(api(a.base, token, f"/api/v1/io/port?port={pt}", timeout=15))["value"]
                except Exception:
                    continue
            if vals:
                io_ticks += 1
                chg = {k: [prev_io.get(k), v] for k, v in vals.items() if prev_io.get(k) != v}
                for k in chg:
                    io_changes[k] = io_changes.get(k, 0) + 1
                io_rows.write(json.dumps({"t_ms": now_ms, "frame": n, "vals": vals, "changed": chg}) + "\n")
                prev_io = vals
        prev, n = sig, n + 1
        time.sleep(1.0 / a.hz)
    out.close()
    if io_rows is not None:
        io_rows.close()
        print(f"io census: {io_ticks} samples of {ports}, register changes: {io_changes} -> {a.out}/io.jsonl")
    print(f"{n} frames -> {a.out}/frames.bin ({len(sha_seen)} witnesses)")


if __name__ == "__main__":
    main()
