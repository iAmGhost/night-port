#!/usr/bin/env python3
"""Title-melt / logo-hold pacing regression harness (docs/72 numbers, re-checked in docs/76).

Runs the port from boot with no key input (scripts/playtest/route_attract_wait.txt) and NIGHT_DENSE=1 card shots,
then measures two wall/model-clock spans that docs/72 pinned against the ORIGINAL's captured frames:

  * the melting-title wipe: the run of card shots whose content is a PARTIAL match to the decoded title3.pcx
    (docs/72: first partial 47906 ms, first settled 55168 ms, span 7262 ms, 51 intermediate frames sampled,
    original 7190 ms -> ratio 1.010);
  * the company-logo hold, measured as the run of shots exactly equal to the decoded logo.pcx
    (docs/72: 879 ms plateau, 2289 ms visible).

Reference pixels are the shipped cards decoded by scripts/playtest_compare_slides.pcx_indices, i.e. an
independent decode, not the port's own loader.

    python3 scripts/playtest_title_melt_span.py
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
from playtest_compare_slides import pcx_indices  # noqa: E402

APP = os.path.join(ROOT, "port", "build", "night")
ROUTE = os.path.join(ROOT, "scripts", "playtest", "route_attract_wait.txt")


def card(name: str) -> np.ndarray:
    return pcx_indices(os.path.join(ROOT, "scratch", "res", name))


def main() -> int:
    out = tempfile.mkdtemp(prefix="meltspan_")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", NIGHT_DENSE="1",
               NIGHT_VIDRAW="1")
    cmd = [APP, "--script", os.path.relpath(ROUTE, os.path.join(ROOT, "port")), "--shots", out,
           "--no-audio", "--frames", "4500", "--shot-limit", "1200"]
    subprocess.run(cmd, env=env, cwd=os.path.join(ROOT, "port"), capture_output=True, timeout=900)
    mpath = os.path.join(out, "manifest.tsv")
    if not os.path.exists(mpath):
        print("no manifest in", out)
        return 1
    title, logo = card("title3.pcx"), card("logo.pcx")
    rows = []
    for line in open(mpath):
        f = line.rstrip("\n").split("\t")
        if len(f) < 9 or f[0] == "seq":
            continue
        seq, t, name = int(f[0]), int(f[1]), f[6]
        cand = [p for p in os.listdir(out) if p.startswith("%04d_" % seq) and p.endswith(".raw")]
        if not cand:
            continue
        raw = open(os.path.join(out, cand[0]), "rb").read()
        if len(raw) < 307200:
            continue                      # not a card-mode (640x480) sample
        a = np.frombuffer(raw[:307200], dtype=np.uint8).astype(np.int16)
        def frac(ref):
            r = np.frombuffer(ref, dtype=np.uint8).astype(np.int16)
            return float((np.abs(a - r) <= 2).mean()) if r.size == a.size else 0.0
        rows.append((seq, t, name, frac(title), frac(logo), a.size))
    part = [(s, t, f) for (s, t, nm, f, _, shp) in rows if 0.02 < f < 0.98]
    setl = [(s, t, f) for (s, t, nm, f, _, shp) in rows if f >= 0.98]
    logop = [(s, t) for (s, t, nm, _, f, shp) in rows if f >= 0.995]
    print("shots analysed:", len(rows))
    if part and setl:
        first_p, last_p = part[0], part[-1]
        span = setl[0][1] - first_p[1]
        print("title wipe: first partial seq=%d t=%dms  first settled seq=%d t=%dms  SPAN=%d ms  partial frames=%d"
              % (first_p[0], first_p[1], setl[0][0], setl[0][1], span, len(part)))
        print("            original span 7190 ms -> ratio %.3f   (docs/72 pinned 7262 ms / 51 partials / 1.010)"
              % (span / 7190.0))
    else:
        print("title wipe: NOT MEASURED (partials=%d settled=%d)" % (len(part), len(setl)))
    if logop:
        ts = [t for _, t in logop]
        # the plateau is the longest run of consecutive shots that are exactly the card
        best, cur = 1, 1
        for i in range(1, len(ts)):
            cur = cur + 1 if logop[i][0] == logop[i - 1][0] + 1 else 1
            best = max(best, cur)
        print("logo plateau (exact card matches): span=%d ms longest run=%d shots  (docs/72 pinned 879 ms)"
              % (ts[-1] - ts[0], best))
    print("shots kept in", out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
