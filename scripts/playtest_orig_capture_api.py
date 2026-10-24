#!/usr/bin/env python3
"""Capture the ORIGINAL game's frames from dosbox-automation's REST API (headless).

This is the capture path the playtest objective needs; the seven routes that fail on this machine (screencapture,
System Events, dosbox-staging capture commands, serial, and guest-side TSRs) are measured dead in docs/46.
dosbox-automation's /api/v1/video/frame works with no window and no hotkey, so original frames can be paired with
the port's named-event shots. See AGENTS.md ("Capturing the original's frames") for how to start the emulator.

Frames are stored verbatim in the API's raw form (documented header + BGRX32 pixels) plus a manifest carrying the
mode metadata, so the compare step can work in the emulator's own colour space rather than a lossy re-encode.

  usage: playtest_orig_capture_api.py OUTDIR [--token T] [--seconds N] [--hz 8] [--base URL]
"""
import argparse, json, os, struct, sys, time
import urllib.request

DEFAULT_BASE = "http://localhost:8386"


def api(base, token, path, out=None):
    req = urllib.request.Request(base + path, headers={"Authorization": "Bearer " + token})
    with urllib.request.urlopen(req, timeout=10) as r:
        data = r.read()
    if out:
        with open(out, "wb") as f:
            f.write(data)
    return data


def parse_raw(b):
    """API raw frame: width u32, height u32, pitch i32, pixel_format u8, palette_count u16, then pixels.

    Measured against the live emulator: `width` is the DOS mode width (320 for mode 13h), but each payload row is
    abs(pitch)/4 pixels -- 640 for mode 13h (the emulator's double-width surface, which frame/info flags as
    double_width) and 640 for VESA 101h too. Decoding by `width` silently rejects every mode-13h frame, so the
    pitch is authoritative. Payload length = abs(pitch) * height with a 15-byte header (verified:
    512015 = 2560*200 + 15 for a mode-13h frame).
    """
    w, h, pitch = struct.unpack_from("<IIi", b, 0)
    pf, pcount = struct.unpack_from("<BH", b, 12)
    hdr = 15
    return w, h, pitch, abs(pitch) // 4, pf, pcount, b[hdr:]


def signature(px):
    """Cheap scene signature: mean of 40x25 luma blocks, quantised. Changes when the picture changes."""
    n = len(px) // 4
    step = max(1, n // 1000)
    acc = [0, 0]
    out = []
    for i in range(0, n, step):
        b, g, r = px[i * 4], px[i * 4 + 1], px[i * 4 + 2]
        acc[0] += (r * 3 + g * 6 + b) >> 3
        acc[1] += 1
        if acc[1] == 25:
            out.append((acc[0] // 25) >> 2)
            acc = [0, 0]
    return tuple(out[:1000])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--token", default=os.environ.get("DOSBOX_API_TOKEN", ""))
    ap.add_argument("--base", default=os.environ.get("DOSBOX_BASE", DEFAULT_BASE))
    ap.add_argument("--seconds", type=float, default=60.0)
    ap.add_argument("--hz", type=float, default=8.0)
    ap.add_argument("--every", action="store_true", help="store every frame, not only scene changes")
    a = ap.parse_args()
    if not a.token:
        sys.exit("no token: pass --token or set DOSBOX_API_TOKEN")
    os.makedirs(a.outdir, exist_ok=True)
    man = open(os.path.join(a.outdir, "manifest.tsv"), "w")
    man.write("seq\tt_ms\tw\th\tpitch\tformat\tmode\tbios_mode\tcolour\tfile\tsig_delta\n")
    period = 1.0 / a.hz
    t0 = time.time()
    seq = 0
    prev = None
    while time.time() - t0 < a.seconds:
        it0 = time.time()                            # per-iteration clock: mixing absolute and relative time here
        tw = it0 - t0                                # made --hz a no-op (polled ~330 Hz instead of 10 Hz)
        try:
            info = json.loads(api(a.base, a.token, "/api/v1/video/frame/info"))
            raw = api(a.base, a.token, "/api/v1/video/frame?format=raw")
        except Exception as ex:                    # 503 while nothing is presented yet
            print("poll: %s" % ex)
            time.sleep(period)
            continue
        vm = info.get("video_mode", {})
        w, h, pitch, roww, pf, pcount, px = parse_raw(raw)
        sig = signature(px)
        delta = 0 if prev is None else sum(1 for x, y in zip(sig, prev) if abs(x - y) > 1)
        # A fade shifts every signature block slightly, so the coarse threshold of the first pass threw away the
        # frames that matter (112 of 875 kept); compare at delta >= 1, and pass --every for the full sequence.
        if a.every or prev is None or delta >= 1:
            name = "%04d_orig.raw" % seq
            with open(os.path.join(a.outdir, name), "wb") as f:
                f.write(raw)
            man.write("%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n" % (
                seq, int(tw * 1000), w, h, pitch, pf,
                "%dx%d" % (vm.get("width", 0), vm.get("height", 0)),
                vm.get("bios_mode_number", ""), vm.get("color_depth", ""), name, delta))
            man.flush()
            print("seq %04d t=%5.2fs hdr=%dx%d row=%dpx mode=%s %s sig_delta=%d" % (
                seq, tw, w, h, roww, vm.get("bios_mode_number"), vm.get("color_depth"), delta))
            prev = sig
            seq += 1
        time.sleep(max(0.0, period - (time.time() - it0)))
    man.close()
    print("captured %d frames into %s" % (seq, a.outdir))


if __name__ == "__main__":
    main()
