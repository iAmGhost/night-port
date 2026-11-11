#!/usr/bin/env python3
"""Read the ORIGINAL game's CD-DA state (fn_10b1d/fn_10927 globals + TOC records) live.

Calibration is playtest_orig_drive.py's: locate the loaded image with code anchors from NIGHT.EXE,
derive the data base R from the `mov dword [abs],1` at 0x365eb, then DAT_x -> phys (x-0x50000)+R.
Prints DAT_00053c40/44/48/60/64 and the 16 TOC records fn_107b9 filled.
"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__)), ".."))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

import base64, json, os, sys, urllib.request, urllib.error

BASE = os.environ.get("DOSBOX_BASE", "http://localhost:8386")
TOKEN = open(os.environ.get("DBA_TOKEN_FILE", "/tmp/dbgauto/.token")).read().strip()
EXE = open(os.environ.get("NIGHT_EXE") or NIGHT_EXE, "rb").read()

def api(path, method="GET", body=None, accept="application/json"):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(BASE + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + TOKEN,
                                          "Content-Type": "application/json",
                                          "Accept": accept})
    with urllib.request.urlopen(req, timeout=25) as r:
        return r.read()

def mem_read(phys, n):
    b = json.loads(api(f"/api/v1/memory/{phys}/{n}"))
    return base64.b64decode(b["memory"]["data"])

def mem_search(val, lo=0, hi=16 << 20):
    return json.loads(api("/api/v1/memory/search", "POST",
                          {"value": val, "width": 4, "start": lo, "end": hi}))["matches"]

# ---- calibrate (same anchors as playtest_orig_drive.py) ----
# file offset = Ghidra address - 0x2600 for this MZ image (same constant as playtest_orig_drive.py)
exe_slice = lambda a, n: EXE[a - 0x2600 : a - 0x2600 + n]
deltas = {}
for anchor in (0x3bb42, 0x3a2d7, 0x12efa, 0x40660):
    want = exe_slice(anchor, 12)
    for cand in mem_search(int.from_bytes(want[:4], "little")):
        if mem_read(cand, 12) == want:
            deltas.setdefault(cand - anchor, []).append(anchor)
delta = max(deltas.items(), key=lambda kv: len(kv[1]))[0]
b = mem_read(0x365eb + delta, 10)
assert b[:2] == b"\xc7\x05", b.hex()
R = int.from_bytes(b[2:6], "little") - 0x13370
print("calibrate: code delta=%#x data base R=%#x" % (delta, R))

phys = lambda dat: (dat - 0x50000) + R
dw = lambda dat: int.from_bytes(mem_read(phys(dat), 4), "little")

cur_start, cur_end, secs, cnt, recs_ptr = dw(0x53c44), dw(0x53c40), dw(0x53c48), dw(0x53c60), dw(0x53c64)
print("DAT_53c60 track count      = %d" % cnt)
print("DAT_53c44 current startLBA = %d (%#x)" % (cur_start, cur_start))
print("DAT_53c40 current endLBA   = %d" % cur_end)
print("DAT_53c48 length seconds   = %d" % secs)
print("DAT_53c64 TOC record table = %d (%#x)%s" % (recs_ptr, recs_ptr, "  [DS-relative]" if recs_ptr < 0x100000 else "  [absolute]"))

rp = phys(recs_ptr) if recs_ptr < 0x100000 else recs_ptr
data = mem_read(rp, 5 * (cnt + 1))
for i in range(cnt + 1):
    rec = data[i * 5:i * 5 + 5]
    lba = int.from_bytes(rec[1:5], "little")
    print("  rec[%2d] status=%02x lba=%d" % (i, rec[0], lba))
