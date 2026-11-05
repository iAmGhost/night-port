#!/usr/bin/env python3
"""The anchored original-side driver, run through the dosbox-automation CONTAINER (docs/377 4, gap 1 closed).

scripts/playtest_orig_drive.py is the rig's anchored driver: it reads a detection-anchored drive file
(`wait res logo.pcx` parks the timeline until the game performs that resource lookup, then rebases what follows --
docs/55), injects the keys, and writes NNNN_<event>.png frames plus events.tsv.  It talks to http://localhost:8386,
and the container's published port is not reachable from this shell (docs/377 2 measures it: 000 on both localhost and
127.0.0.1, while the same call inside the container answers).  The container image has no python3, so the driver
cannot move inside.

Same recipe as scripts/rig/capture_container.py: import the driver, replace ONLY its api(), and let every call go out
as `docker exec <name> curl`.  The 409 retry the driver needs (back-to-back input/sequence posts collide) is kept here
rather than dropped, and a raw frame reply is written to the mounted /work directory and read back.

  uv run --with numpy --with pillow scripts/playtest_orig_drive_container.py \
      --out DIR --drive scripts/playtest/drive_orig_episode1.txt --hz 8
"""
import argparse
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import playtest_orig_drive as od                                   # the rig's driver: anchors, keys, events

MOUNT = os.environ.get("DBGA_MOUNT", "/tmp/dbgauto")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--drive")
    ap.add_argument("--name", default="dbga63")
    ap.add_argument("--token-file", default="/tmp/rig/.token")
    ap.add_argument("--hz", type=float, default=8.0)
    ap.add_argument("--thresh", type=float, default=0.90)
    ap.add_argument("--ref", action="append", default=None)
    a = ap.parse_args()
    token = open(a.token_file).read().strip()

    def api(base, tok, path, method="GET", body=None, raw_body=None, ctype="application/json", retries=8):
        data = None
        if body is not None:
            data = json.dumps(body).encode()
        elif raw_body is not None:
            data = raw_body.encode() if isinstance(raw_body, str) else raw_body
        binary = "format=raw" in path or "format=png" in path
        for attempt in range(retries + 1):
            # `-i` only when there is a body: docker exec does not forward stdin without it, and curl's `@-` then
            # posts an empty body -- which the emulator answers as "Malformed request body", indistinguishable from a
            # bad payload if you do not know the flag is missing (measured: 123 boot frames captured, then every key
            # injection failed with HTTP 400).
            cmd = ["docker", "exec"] + (["-i"] if data is not None else []) + [a.name, "curl", "-s", "-m", "20", "-X", method,
                   "-H", "Authorization: Bearer " + token,
                   "-w", "\n%{http_code}"]
            if data is not None:
                cmd += ["-H", "Content-Type: " + ctype, "--data-binary", "@-"]
            if binary:
                cmd += ["-o", "/work/_drv.bin"]
            cmd.append("http://127.0.0.1:8386" + path)
            r = subprocess.run(cmd, input=data, capture_output=True)
            if r.returncode:
                # curl 7/28 are "cannot connect" and "timed out": normal while dosbox-automation is still coming up
                # (the emulator binds its API a second or two after the process starts), and the driver already
                # retries 409s -- so a connection failure is retried on the same budget, not raised.
                if r.returncode in (7, 28) and attempt < retries:
                    time.sleep(1.0)
                    continue
                raise RuntimeError("docker exec curl rc=%d: %s" % (r.returncode, r.stderr.decode()[:200]))
            out = r.stdout
            code = int(out.rsplit(b"\n", 1)[-1].decode().strip() or 0)
            payload = open(os.path.join(MOUNT, "_drv.bin"), "rb").read() if binary else out[:-len(b"\n" + str(code).encode())]
            if code == 409 and attempt < retries:                 # an injected sequence is still in flight
                time.sleep(0.35)
                continue
            if code >= 400:
                raise RuntimeError("HTTP %d on %s %s: %s" % (code, method, path, payload[:300].decode(errors="replace")))
            return payload
        raise RuntimeError("retries exhausted on " + path)

    od.api = api                                                   # transport only
    argv = ["playtest_orig_drive.py", "--out", a.out, "--base", "http://127.0.0.1:8386",
            "--token", token, "--hz", str(a.hz), "--thresh", str(a.thresh)]
    if a.drive:
        argv += ["--drive", a.drive]
    if a.ref:
        for r in a.ref:
            argv += ["--refs", r]
    sys.argv = argv
    od.main()


if __name__ == "__main__":
    main()
