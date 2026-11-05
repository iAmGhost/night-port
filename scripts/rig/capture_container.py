#!/usr/bin/env python3
"""Original-side capture through the dosbox-automation CONTAINER (docs/46's only working route on this machine).

scripts/rig/capture_original.py is the rig's own original-side capturer, but it talks to the emulator over
http://localhost:8386 -- and the container's published port is not reachable from this shell (measured again today:
`curl http://127.0.0.1:8386/api/v1/status` returns 000, while the same call inside the container answers).  The
container image also has no python3, so the capturer cannot be moved inside.

This wrapper keeps the capturer's logic byte-for-byte and replaces only its transport: every API call goes out as
`docker exec dbga curl`, and a binary frame reply lands in the mounted /work directory and is read back from
/tmp/dbgauto.  Importing capture_original and swapping its api() means the 40x30 grid, the geometry repair for mode
transitions (docs/181 §6, docs/292 §5.2) and the frames.bin record layout are the rig's, not a reimplementation.

  scripts/rig/capture_container.py --out DIR [--seconds 150] [--hz 6] [--drive FILE] [--name dbga63]
"""
import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import capture_original as co                                   # the rig's capturer: grid, layout, repairs

MOUNT = os.environ.get("DBGA_MOUNT", "/tmp/dbgauto")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", default="dbga63")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--seconds", type=float, default=150.0)
    ap.add_argument("--hz", type=float, default=6.0)
    ap.add_argument("--drive")
    ap.add_argument("--witness-every", type=int, default=10)
    a = ap.parse_args()
    token = open(a.token_file).read().strip()

    def api(base, tok, path, method="GET", body=None, timeout=30, raw=None):
        cmd = ["docker", "exec", a.name, "curl", "-s", "-m", str(int(timeout)), "-X", method,
               "-H", "Authorization: Bearer " + token]
        if body is not None:
            cmd += ["-H", "Content-Type: application/json", "-d", json.dumps(body)]
        if raw is not None:
            cmd += ["-H", "Content-Type: application/octet-stream",
                    "-d", raw.decode() if isinstance(raw, bytes) else str(raw)]
        binary = "format=" in path and "/info" not in path
        if binary:
            cmd += ["-o", "/work/_cap.bin"]
            cmd.append("http://127.0.0.1:8386" + path)
            r = subprocess.run(cmd, capture_output=True)
            if r.returncode:
                raise RuntimeError("frame fetch failed: " + r.stderr.decode()[:200])
            with open(os.path.join(MOUNT, "_cap.bin"), "rb") as f:
                return f.read()
        cmd.append("http://127.0.0.1:8386" + path)
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError("api call failed: " + r.stderr.decode()[:200])
        return r.stdout.encode()

    co.api = api                                              # transport only; logic untouched
    sys.argv = ["capture_original.py", "--out", a.out, "--base", "http://127.0.0.1:8386",
                "--token-file", a.token_file, "--seconds", str(a.seconds), "--hz", str(a.hz),
                "--witness-every", str(a.witness_every)]
    if a.drive:
        sys.argv += ["--drive", a.drive]
    co.main()


if __name__ == "__main__":
    main()
