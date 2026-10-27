#!/usr/bin/env python3
"""Measure the ORIGINAL's stage start slide-show pacing without pressing keys.

Boot -> wait for the title (track 9 window active) -> one Enter -> poll
(video height, DAT_53c44 playing-track LBA) every 1.5 s for 135 s. Calibrates the
data base live with the playtest_orig_drive.py anchors (fresh dosbox boots move it).
Writes one line per sample; run inside the container.
"""
import base64, json, os, time, urllib.request

TOKEN = open(os.environ.get("DBA_TOKEN_FILE", "/work/.token")).read().strip()
BASE = os.environ.get("DOSBOX_BASE", "http://127.0.0.1:8386")
EXE = open(os.environ.get("NIGHT_EXE", "/work/game/NIGHT.EXE"), "rb").read()

def api(p, m="GET", b=None):
    req = urllib.request.Request(BASE + p, data=json.dumps(b).encode() if b else None, method=m,
        headers={"Authorization": "Bearer " + TOKEN, "Content-Type": "application/json",
                 "Accept": "application/json"})
    return json.loads(urllib.request.urlopen(req, timeout=25).read())

def mem_read(phys, n):
    return base64.b64decode(api("/api/v1/memory/%d/%d" % (phys, n))["memory"]["data"])

def search(v):
    return api("/api/v1/memory/search", "POST", {"value": v, "width": 4, "start": 0, "end": 1 << 24}).get("matches", [])

deltas = {}
for a in (0x3bb42, 0x3a2d7, 0x12efa, 0x40660):
    want = EXE[a - 0x2600:a - 0x2600 + 12]
    for c in search(int.from_bytes(want[:4], "little")):
        if mem_read(c, 12) == want:
            deltas.setdefault(c - a, []).append(a)
delta = max(deltas.items(), key=lambda kv: len(kv[1]))[0]
b = mem_read(0x365eb + delta, 10)
R = int.from_bytes(b[2:6], "little") - 0x13370
print("calibrate delta=%#x R=%#x" % (delta, R), flush=True)

dw = lambda dat: int.from_bytes(mem_read((dat - 0x50000) + R, 4), "little")
def h(): return api("/api/v1/video/frame/info")["video_mode"]["height"]
def key(k):
    api("/api/v1/input/sequence", "POST", {"events": [
        {"t": 0, "type": "key", "key": k, "pressed": True},
        {"t": 80, "type": "key", "key": k, "pressed": False}]})

t0 = time.time()
while time.time() - t0 < 360:
    try:
        if dw(0x53c44) == 185120 and h() == 480:
            break
    except Exception:
        pass
    time.sleep(2)
print("t=%.0f at title (LBA=185120)" % (time.time() - t0), flush=True)
key("KBD_enter")
for _ in range(90):
    time.sleep(1.5)
    try:
        print("t=%5.1f h=%d LBA=%d" % (time.time() - t0, h(), dw(0x53c44)), flush=True)
    except Exception as e:
        print("err", e, flush=True)
