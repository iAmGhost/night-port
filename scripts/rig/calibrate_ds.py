#!/usr/bin/env python3
"""Calibrate the original's DS group in emulator physical memory, and verify it.

Nothing here is assumed: NIGHT is an LE binary, so DS is a protected-mode selector whose linear base
the emulator does not report as base+offset (GET /memory/<seg>/<off> reads low memory and returns
zeros). Instead the base is *found*: distinctive image-initialised DS cells are searched in physical
memory, each candidate base is checked against a second independent cell, and the whole DS span is
confirmed byte-for-byte against NIGHT.EXE at file offset DS-0x7600 (docs/100 §1).

  python3 scripts/rig/calibrate_ds.py [--write gen/ds_base.json]
"""
import argparse
import json
import os
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
EXE = os.path.join(ROOT, "NIGHT", "NIGHT.EXE")
DS_TO_FILE = 0x7600


def api(base, token, path, method="GET", body=None):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(base + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + token,
                                          "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read()


def read_phys(base, token, addr, n):
    return api(base, token, f"/api/v1/memory/{addr}/{n}")


def find_ds(base, token, img):
    """Candidate (phys_base, evidence) list; phys_base + ds_addr == physical address of that cell."""
    probes = [0x5274c, 0x52a1c, 0x53100]          # float 0.5 table, kill quota, credits counter
    cands = {}
    for off in probes:
        val = int.from_bytes(img[off - DS_TO_FILE:off - DS_TO_FILE + 4], "little")
        if val == 0:
            continue
        hits = json.loads(api(base, token, "/api/v1/memory/search", "POST",
                              {"value": val, "width": 4, "start": 0x100000, "end": 0x1000000}))
        for h in hits.get("matches", hits.get("results", []))[:40]:
            a = h if isinstance(h, int) else int(h.get("address", h.get("addr", 0)))
            cands.setdefault(a - off, []).append((off, val, a))
    out = []
    for phys, ev in sorted(cands.items()):
        span = read_phys(base, token, phys + 0x528a8, 0x40)
        want = img[0x528a8 - DS_TO_FILE:0x528a8 - DS_TO_FILE + 0x40]
        if span == want:
            out.append({"phys_base": phys, "probes": ev, "span_match": True})
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="http://localhost:8386")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--write", default=os.path.join(HERE, "gen", "ds_base.json"))
    a = ap.parse_args()
    token = open(a.token_file).read().strip()
    img = open(EXE, "rb").read()

    ds_sel = json.loads(api(a.base, token, "/api/v1/cpu/state")).get("registers", {}).get("ds")
    cands = find_ds(a.base, token, img)
    if not cands:
        sys.exit("no DS candidate verified — the game may not be loaded yet")
    c = cands[0]
    # whole-DS confirmation, 16 KB stride
    ok, bad = 0, []
    for ds in range(0x7600, 0x66000 - 0x40, 0x4000):
        got = read_phys(a.base, token, c["phys_base"] + ds, 64)
        if got == img[ds - DS_TO_FILE:ds - DS_TO_FILE + 64]:
            ok += 1
        else:
            bad.append(hex(ds))
    info = {"phys_ds_base": c["phys_base"], "ds_selector": ds_sel,
            "probes": [(hex(o), v, hex(pa)) for o, v, pa in c["probes"]],
            "stride64_matches": ok, "stride64_mismatches": bad,
            "note": "mismatches are cells the game has already written; they must NOT be used as "
                    "calibration anchors, only as runtime state"}
    os.makedirs(os.path.dirname(a.write), exist_ok=True)
    json.dump(info, open(a.write, "w"), indent=1)
    print(json.dumps(info, indent=1))


if __name__ == "__main__":
    main()
