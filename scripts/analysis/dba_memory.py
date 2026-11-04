#!/usr/bin/env python3
"""Talk to the dosbox-automation webserver's memory API (the guest is NIGHT.EXE under DOS4GW).

The original's DAT_<hex> symbols used throughout the port (docs, f.c comments) are the game's data-segment
addresses as recorded by the disassembler. At runtime the image sits at some physical base inside the emulated
16 MB, so every address has to be calibrated once per session: find a code dword whose bytes are unique, then
delta = found - disassembler_address.

    memory.py calibrate            # print the delta, verify it against a second code pattern
    memory.py read ADDR LEN        # read at disassembler address (after calibration)
    memory.py write ADDR VALUE     # write a dword at a disassembler address
    memory.py search VALUE WIDTH
"""
from __future__ import annotations

import json
import os
import struct
import sys
import urllib.request

BASE = os.environ.get("DOSBOX_BASE", "http://localhost:8386")
TOKEN = os.environ.get("DOSBOX_API_TOKEN", "0f1e2d3c4b5a69788796a5b4c3d2e1f00f1e2d3c4b5a69788796a5b4c3d2e1f0")
EXE = os.environ.get("NIGHT_EXE", os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                                               "NIGHT", "NIGHT.EXE"))
FILE_BASE = 0x2600            # disassembler address -> NIGHT.EXE file offset: off = addr - FILE_BASE


def api(path, method="GET", body=None):
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(BASE + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + TOKEN,
                                          "Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read()


def read_phys(addr: int, length: int) -> bytes:
    return api(f"/api/v1/memory/{addr}/{length}")


def search(value: int, width: int = 4, start: int = 0, end: int = 1 << 24) -> list[int]:
    return json.loads(api("/api/v1/memory/search", "POST",
                          {"value": value, "width": width, "start": start, "end": end})).get("matches", [])


def write_phys(addr: int, payload: bytes) -> None:
    req = urllib.request.Request(BASE + f"/api/v1/memory/{addr}", data=payload, method="PUT",
                                 headers={"Authorization": "Bearer " + TOKEN,
                                          "Content-Type": "application/octet-stream"})
    with urllib.request.urlopen(req, timeout=30) as r:
        r.read()


def exe_bytes(addr: int, length: int) -> bytes:
    with open(EXE, "rb") as f:
        f.seek(addr - FILE_BASE)
        return f.read(length)


def calibrate(anchors=(0x3bb42, 0x3a2d7, 0x12efa, 0x40660)) -> int:
    """Locate the loaded image: search a code dword, keep candidates whose following bytes match the file."""
    deltas = {}
    for anchor in anchors:
        want = exe_bytes(anchor, 12)
        val = struct.unpack("<I", want[:4])[0]
        for cand in search(val, 4):
            got = read_phys(cand, 12)
            if got == want:
                deltas.setdefault(cand - anchor, []).append((anchor, cand))
    if not deltas:
        raise SystemExit("no candidate matched the file bytes for any anchor")
    best = max(deltas.items(), key=lambda kv: len(kv[1]))
    print(f"delta = {best[0]} (0x{best[0]:x}) confirmed by {len(best[1])} anchor(s): "
          + ", ".join(f"{a:#x}->{hex(c)}" for a, c in best[1]))
    if len(deltas) > 1:
        print("other deltas seen:", {k: len(v) for k, v in deltas.items()})
    return best[0]


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    cmd = sys.argv[1]
    if cmd == "calibrate":
        calibrate()
        return 0
    delta = int(os.environ.get("DBA_DELTA", "0"), 0)
    if cmd == "read":
        addr, length = int(sys.argv[2], 0), int(sys.argv[3])
        print(read_phys(addr + delta, length).hex())
    elif cmd == "read4":
        v = struct.unpack("<I", read_phys(int(sys.argv[2], 0) + delta, 4))[0]
        print(f"{v} (0x{v:x})")
    elif cmd == "write":
        addr, value = int(sys.argv[2], 0), int(sys.argv[3], 0)
        write_phys(addr + delta, struct.pack("<I", value))
        print(f"wrote {value} at {addr:#x} (+{delta:#x}) -> verify: "
              f"{struct.unpack('<I', read_phys(addr + delta, 4))[0]}")
    elif cmd == "search":
        print(search(int(sys.argv[2], 0), int(sys.argv[3]) if len(sys.argv) > 3 else 4))
    else:
        print(__doc__)
    return 0


if __name__ == "__main__":
    sys.exit(main())
