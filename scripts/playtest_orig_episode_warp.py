#!/usr/bin/env python3
"""Capture the ORIGINAL game at any of the nine episodes, for the per-episode playtest comparison (docs/74).

Scripted play cannot clear nine episodes end to end, so the port uses a host-side debug warp (`--warp-stage N`,
docs/52) which simply sets DAT_00063370 -- the shipped stage index, initialised to 1 at 0x365eb and incremented by
one at 0x3b54c on stage completion -- before the stage assets are named. This script does the equivalent on the
original side, from outside the guest: the dosbox-automation webserver writes that same dword while the intro slide
show is still running, i.e. before `stage%d.pcx` / `stage%d.wal` are looked up.

For each episode it restarts the emulator in the container, runs the detection-anchored drive through title ->
slides -> gameplay, records the gameplay frames, and scores them against the port's own per-episode screenshot.

    scripts/playtest_orig_episode_warp.py 2 3 4 ...        # episodes to capture (default 2..9)
"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__))))
from disc_loc import find_cue as _find_cue, find_exe as _find_exe, find_img as _find_img, find_res as _find_res
NIGHT_EXE = str(_find_exe())      # the shipped binary, wherever this tree keeps it now (docs/455)

from __future__ import annotations

import json
import os
import subprocess
import sys
import time
import urllib.request

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = os.environ.get("DOSBOX_BASE", "http://localhost:8386")
TOKEN = os.environ.get("DOSBOX_API_TOKEN", "")
CONTAINER = os.environ.get("DBA_CONTAINER", "dbga")
OUTROOT = "/tmp/dbgauto/orig_warp"
PORT_SHOTS = os.path.join(ROOT, "docs", "manual-playtest", "04-shots")

DRIVE = """record boot 5000
wait_mode 200 150
record fli 6000
wait_mode 480 180
record title_melt 6000
key enter 150
sleep 1200
writemem 0x63370 {stage}
record slides 9000
key_until_mode 200 enter 30 1200   # episode 8 runs a long na800NN picture show (docs/59)
shot stage_play
record gameplay 9000
stop
"""


def status() -> dict:
    req = urllib.request.Request(BASE + "/api/v1/status", headers={"Authorization": "Bearer " + TOKEN})
    with urllib.request.urlopen(req, timeout=10) as r:
        return json.loads(r.read())


def sh(*cmd: str) -> None:
    subprocess.run(cmd, check=False)


def post(path: str) -> None:
    req = urllib.request.Request(BASE + path, data=b"{}", method="POST",
                                 headers={"Authorization": "Bearer " + TOKEN,
                                          "Content-Type": "application/json"})
    try:
        urllib.request.urlopen(req, timeout=10).read()
    except Exception as e:
        print("  shutdown request:", e)


def restart_emulator() -> None:
    # No pkill in the container image; the webserver's own shutdown endpoint ends the instance.
    # The shutdown endpoints 500 and the image has no pkill, so end every dosbox process in the container
    # directly. Leaving more than one instance alive makes the webserver's memory endpoints fail.
    sh("docker", "exec", CONTAINER, "sh", "-c",
       "for p in /proc/[0-9]*; do c=$(cat $p/comm 2>/dev/null); case \"$c\" in *dosbox*) "
       "kill -9 \"${p#/proc/}\" 2>/dev/null;; esac; done")
    time.sleep(3)
    sh("docker", "exec", "-d", CONTAINER, "bash", "-c",
       "cd /work/dosbox-automation-0.85.1-7567ec4-linux-x86_64 && "
       "xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_warp.log 2>&1")
    for _ in range(40):
        try:
            status()
            return
        except Exception:
            time.sleep(1)
    raise SystemExit("the webserver did not come back up in 40 s")


def score_port(out_dir: str, stage: int) -> None:
    port_path = os.path.join(PORT_SHOTS, "playing_stage%d.bmp" % stage)
    if not os.path.exists(port_path):
        print(f"  (no port screenshot for stage {stage})")
        return
    P = np.asarray(Image.open(port_path).convert("RGB")).astype(np.int16)
    best = None
    for n in sorted(os.listdir(out_dir)):
        if "gameplay" not in n:
            continue
        O = np.asarray(Image.open(os.path.join(out_dir, n)).convert("RGB")).astype(np.int16)
        if O.shape != P.shape:
            continue
        w = float((np.abs(P - O) <= 3).all(axis=2).mean() * 100)
        mae = float(np.abs(P - O).mean())
        if best is None or w > best[0]:
            best = (w, mae, n)
    if best:
        print(f"  port playing_stage{stage}.bmp vs the original's warp-{stage} frames: "
              f"best {best[2]} -> {best[0]:.2f} % of pixels within +-3, MAE {best[1]:.2f}")
    else:
        print(f"  no comparable 320x200 gameplay frame was captured for stage {stage}")


def main() -> int:
    stages = [int(x) for x in sys.argv[1:]] or list(range(2, 10))
    os.makedirs(OUTROOT, exist_ok=True)
    for stage in stages:
        out = os.path.join(OUTROOT, f"ep{stage}")
        os.makedirs(out, exist_ok=True)
        df = os.path.join(out, "drive.txt")
        open(df, "w").write(DRIVE.format(stage=stage))
        print(f"== episode {stage}: restarting the emulator", flush=True)
        restart_emulator()
        env = dict(os.environ, DOSBOX_API_TOKEN=TOKEN, DOSBOX_BASE=BASE, NIGHT_EXE=NIGHT_EXE)
        subprocess.run(["uv", "run", "--quiet", "--with", "numpy", "--with", "pillow", "python3",
                        os.path.join(ROOT, "scripts", "playtest_orig_drive.py"), "--out", out,
                        "--drive", df, "--hz", "3"], env=env, check=False)
        score_port(out, stage)
    return 0


if __name__ == "__main__":
    sys.exit(main())
