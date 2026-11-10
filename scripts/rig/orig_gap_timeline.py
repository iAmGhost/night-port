#!/usr/bin/env python3
"""Is the ORIGINAL's stage entry also a multi-second blank? docs/434.

The port's `0.2.10` stall witness says the port's 3-second gaps are stage entry: ~2.6 MB of loads in the
window, no presents, while the 490 Hz game clock runs on. Before optimizing that, the fair question is whether
the original does the same thing -- it reads the same ~6 MB from the same era of optical drive, and docs/415
already found it holding 3.91 s on its stage card.

This measures the original's *screen-change timeline* in the same shape the port reports (`gap worst=`):
poll raw frames, hash the index pixels, and report the distribution of inter-change intervals plus the window
around the 640x480 -> 320x200 transition (slides -> gameplay), which is the port's stall.

Transport is `docker exec dbga curl` -- the API port is unreachable from this shell (docs/377). The container
route runs at factor 1.000 real time (docs/414), and sampling costs ~0.2 s per poll, so a change interval
below ~400 ms is at the floor of what this can resolve. Multi-second blanking -- the thing under question --
is well above it.

  TOKEN=$(docker exec dbga63 printenv DOSBOX_API_TOKEN)
  python3 scripts/rig/orig_gap_timeline.py --token "$TOKEN" --secs 200 --enter-when 480
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import time

# The paths below already carry /api/v1; prefixing it here too gave /api/v1/api/v1/... -> empty 404 replies,
# which read as "the game produced no frames" rather than as a bad URL. Docs/428's lesson, one layer down.
BASE = "http://localhost:8386"
CONTAINER = os.environ.get("DBGA_NAME", "dbga63")


def api(tok, path, method="GET", body=None, binary=False, name=None, mnt="/work"):
    """One API call, executed INSIDE the container. Raw replies land on the mounted volume."""
    cmd = ["docker", "exec", CONTAINER, "curl", "-s", "-m", "20",
           "-H", "Authorization: Bearer " + tok, BASE + path]
    if method != "GET":
        cmd += ["-X", method]
    if body is not None:
        cmd += ["-H", "Content-Type: application/json", "-d", json.dumps(body)]
    if binary and name:
        cmd += ["-o", os.path.join(mnt, name)]
    for attempt in range(6):
        r = subprocess.run(cmd, capture_output=True, text=True)
        out = r.stdout.strip()
        if '"error"' not in out or not binary:
            break
        time.sleep(0.4)
    if binary and name:
        host = os.environ.get("DBGA_MOUNT", "/tmp/dbgauto") + "/" + name
        try:
            with open(host, "rb") as f:
                return f.read()
        except OSError:
            return b""
    try:
        return json.loads(r.stdout) if r.stdout else {}
    except json.JSONDecodeError:
        return {"raw": r.stdout[:200]}


def press(tok, key, ms=150):
    """The exact event shape the rig's driver posts (playtest_orig_drive.key). Two things learned the hard
    way: the API rejects unknown key names with a 400 -- the emulator spells them KBD_ (KEY_DEFAULTS there),
    so "return" was refused for two whole runs while the game sat at the title card, and an input call that
    400s is indistinguishable from "the game ignored the press" unless you check the reply. Hence the return
    value and the print on failure."""
    kn = {"enter": "KBD_enter", "esc": "KBD_esc"}.get(key, key if key.startswith("KBD_") else "KBD_" + key)
    r = api(tok, "/api/v1/input/sequence", "POST",
            {"events": [{"type": "key", "key": kn, "pressed": True},
                        {"type": "key", "key": kn, "pressed": False, "delay_ms": ms}]})
    if "error" in r:
        print("!! press(%s) rejected: %s" % (key, str(r)[:140]), flush=True)
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--token", required=True)
    ap.add_argument("--secs", type=float, default=200.0)
    ap.add_argument("--enter-after", type=float, default=8.0,
                    help="press Enter once, after the game has sat in 320x200 this long (intro/attract)")
    ap.add_argument("--enter-480-every", type=float, default=6.0)
    ap.add_argument("--enter-any-after", type=float, default=12.0,
                    help="press Enter by this time even if no 320x200 window appeared (attract mode is 640x480)")
    ap.add_argument("--enter-max", type=int, default=14, help="press Enter at most this many times")
    ap.add_argument("--out", default="scratch/pocket/orig_gap")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)

    # The raw reply is a header (width/height/pitch/palette) then index pixels. Hashing the whole reply body is
    # enough: any visible change alters it, and the header is constant per video mode.
    tmp = "gap_raw_%d.bin" % os.getpid()
    rows = []
    t0 = time.time()
    last_hash, last_change = None, time.time()
    enters = 0
    low = 0.0
    pt = time.time()
    last_480_press = -1e9
    while time.time() - t0 < a.secs:
        t = time.time() - t0
        dt = time.time() - pt; pt = time.time()
        raw = api(a.token, "/api/v1/video/frame?format=raw", binary=True, name=tmp)
        info = api(a.token, "/api/v1/video/frame/info")
        h, w = info.get("height"), info.get("width")
        if not raw:
            continue
        hh = hashlib.sha256(raw).hexdigest()[:16]
        changed = hh != last_hash
        if changed:
            last_hash = hh
        rows.append({"t": round(t, 3), "w": w, "h": h, "changed": changed,
                     "gap_ms": round((t - last_change) * 1000)})
        if changed:
            last_change = t
        if h and h < 300:
            low += dt
        else:
            low = 0.0
        # The game may already be in the attract slide show (640x480) when we attach, in which
        # case a 320x200 window never arrives and the run measures nothing. Press anyway after --enter-any-after.
        # docs/435: the original's slide show HOLDS on the last slide / stage card until a key arrives
        # (the drive file says so, and the first run of this tool sat on a static 640x480 card for 45 s at a
        # time), so one press is not enough to cross into gameplay. Nudge while in 640x480, like the drive does.
        if (h and h >= 480) and (t - last_480_press) >= a.enter_480_every and enters < a.enter_max:
            last_480_press = t
            press(a.token, "enter")
            enters += 1
            print("[t=%6.1f] in 640x480 -> Enter #%d" % (t, enters), flush=True)
        if False and (low >= a.enter_after or t >= a.enter_any_after) and enters < a.enter_max:
            press(a.token, "enter")
            enters += 1
            print("[t=%6.1f] %s s in 320x200 (intro/attract) -> Enter #%d" % (t, int(low), enters), flush=True)
        sys.stdout.write("."); sys.stdout.flush()

    with open(os.path.join(a.out, "timeline.jsonl"), "w") as f:
        for r in rows:
            f.write(json.dumps(r) + "\n")

    changes = [r["t"] for r in rows if r["changed"]] or [rows[0]["t"]]
    gaps = [(changes[i + 1] - changes[i]) * 1000 for i in range(len(changes) - 1)]
    gaps.sort()
    def pct(p):
        return gaps[min(len(gaps) - 1, int(len(gaps) * p))] if gaps else 0
    print("\n## ORIGINAL screen-change timeline (%.0f s, %d samples, %d changes, ~%.1f samples/s)" %
          (time.time() - t0, len(rows), len(changes), len(rows) / (time.time() - t0)))
    print("   gap ms: median %.0f  p90 %.0f  p99 %.0f  MAX %.0f" %
          (pct(.5), pct(.9), pct(.99), gaps[-1] if gaps else 0))
    print("   changes taking > 1000 ms: %d" % sum(1 for g in gaps if g > 1000))
    print("   worst 6:", [round(g) for g in gaps[-6:]][::-1])

    # The transition the port stalls on: 640x480 (slides) -> 320x200 (gameplay).
    tr = None
    prev_h = None
    for r in rows:
        if prev_h and prev_h >= 480 and r["h"] and r["h"] < 300:
            tr = r["t"]
            break
        prev_h = r["h"]
    if tr:
        win = [g for (c, g) in zip(changes, gaps) if tr - 12 <= c <= tr + 4]
        print("   640x480 -> 320x200 transition at t=%.1f s; gaps in [-12s,+4s] window: %s" %
              (tr, [round(g) for g in sorted(win, reverse=True)[:6]] or "none"))
        print("   (the port, 0.2.10: median 38 ms, p90 65, p99 1 376, MAX 2 978 ms)")
    else:
        print("   no 640x480 -> 320x200 transition seen in the window")
    return 0


if __name__ == "__main__":
    sys.exit(main())
