#!/usr/bin/env python3
"""docs/91: turn the soak logs into the per-stage growth table.

    uv run python3 scripts/playtest_soak_report.py /tmp/night_soak            # both phases found in the names
    uv run python3 scripts/playtest_soak_report.py /tmp/night_soak --phase asan

A line is one stage: arena take-up at the first and last sample plus the peak, RSS high-water at the first and
last sample plus the peak, how many samples had a live room (g_dat_63354 != 0, only in runs built after the
witness was added), how many AddressSanitizer reports appeared, and how the run ended.
"""
import argparse
import glob
import os
import re
import sys

MEM = re.compile(r"memstat stage=(\d+)(?: room=(\d+))? frames=(\d+) arena=(\d+)/(\d+) rss_max=(\d+) KB")
ASAN = re.compile(r"ERROR: AddressSanitizer|runtime error: |SUMMARY: AddressSanitizer|\bSEGV\b")


def report(outdir: str, phase: str) -> int:
    rows = []
    for path in sorted(glob.glob(os.path.join(outdir, f"{phase}_stage*.log"))):
        m = re.search(r"stage(\d+)\.log$", path)
        if not m:
            continue
        stage = int(m.group(1))
        samples = []
        asan = 0
        asan_first = ""
        ended = ""
        for line in open(path, errors="replace"):
            mm = MEM.search(line)
            if mm:
                samples.append(tuple(int(x or 0) for x in mm.groups()))
                continue
            if ASAN.search(line):
                asan += 1
                if not asan_first:
                    asan_first = line.strip()[:160]
            if "game main unwound" in line:
                ended = "unwound"
            elif "the game main returned" in line:
                ended = "returned"
            elif line.startswith(("== ", "blit stats")):
                pass
        if not samples:
            rows.append([stage, "no memstat samples"])
            continue
        arena = [s[3] for s in samples]
        rss = [s[5] for s in samples]
        room = sum(1 for s in samples if s[1])
        last = samples[-1]
        rows.append([
            stage,
            f"arena {arena[0]/2**20:.2f} -> {arena[-1]/2**20:.2f} MiB (peak {max(arena)/2**20:.2f})",
            f"rss {rss[0]//1024} -> {rss[-1]//1024} MiB (peak {max(rss)//1024})",
            f"cap {samples[-1][4]/2**20:.0f} MiB",
            f"frames {last[2]}",
            f"room samples {room}/{len(samples)}",
            f"asan {asan}",
            ended or "killed by timeout",
        ])
        if asan_first:
            rows.append(["", f"first ASan line: {asan_first}"])
    if not rows:
        print(f"no {phase}_stage*.log in {outdir}", file=sys.stderr)
        return 1
    widths = [max(len(str(r[i])) for r in rows if i < len(r)) for i in range(max(len(r) for r in rows))]
    for r in rows:
        print("  ".join(str(x).ljust(w) for x, w in zip(r, widths)).rstrip())
    return 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("--phase", default="both")
    a = ap.parse_args()
    rc = 0
    for ph in (["plain", "asan"] if a.phase == "both" else [a.phase]):
        print(f"--- {ph} ---")
        rc |= report(a.outdir, ph)
    sys.exit(rc)
