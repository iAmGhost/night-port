#!/usr/bin/env python3
"""Coverage-guided input fuzzing for the port (docs/120).

Coverage signal: the port built by `make -C port censusapp` (-finstrument-functions) writes `census.bin`,
one byte per function in src/gen/fn_table.c, whose names ARE the shipped guest addresses -- so "never
entered" is a statement about the shipped code, not about the port's own structure.

Strategy: greedy coverage-guided corpus. A seed is kept (and becomes part of the corpus that later seeds
mutate) only if it strictly increases the union of entered functions. Every seed runs under a hard wall-clock
cap and a --frames budget, several container-free port processes in parallel. Any non-zero exit that is not
the timeout is saved as a minimal-ish repro (the drive plus the flags that reproduced it).

Usage:
  scripts/rig/fuzz.py --episodes 1,2,3 --seeds 8 --workers 3 --cap 100 --out /tmp/rig/fuzz1
"""
import argparse
import os
import random
import shutil
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BIN = os.path.join(REPO, "port", "test", "night-census")
BASE = os.path.join(REPO, "scripts", "rig", "drives", "ep1_kill3.txt")
ALT_BASES = {}   # --base name -> drive file (docs/120 §8: the grammar, not the seed count, is the limit)
OUT_DIR = "/tmp"

# Gameplay alphabet, kept to keys the game actually consumes (validated against gen/kbd_names.txt).
KEYS = ["left", "right", "up", "down", "lctrl", "space", "return", "esc", "lshift", "a", "b", "c", "d",
        "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
        "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "comma", "period", "slash"]

def rng_keep(name):
    """Deterministic ~1/4 keep rate for diversity, keyed on the seed name."""
    return (abs(hash(name)) % 4) == 0


def read_base(until_ms, base=None):
    lines, steps = [], []
    for raw in open(base or BASE):
        s = raw.rstrip("\n")
        if not s.strip() or s.lstrip().startswith("#"):
            continue
        p = s.split()
        if p[0].isdigit():
            if int(p[0]) <= until_ms:
                steps.append(s)
        else:
            lines.append(s)
    return lines + steps

def mutate(rng, base, window_lo, window_hi, n_events, inject):
    ev = []
    for _ in range(n_events):
        t = rng.randrange(window_lo, window_hi, 20)
        k = rng.choice(KEYS)
        dur = rng.choice([40, 70, 120, 250, 500])
        ev.append(f"{t} key {k} down")
        ev.append(f"{t + dur} key {k} up")
        if rng.random() < 0.12:
            ev.append(f"{t + dur + 20} key {rng.choice(KEYS)} tap")
    if inject:
        for _ in range(rng.randrange(1, 5)):
            t = rng.randrange(window_lo, window_hi, 20)
            what = rng.random()
            if what < 0.4:
                ev.append(f"{t} poke 0x{rng.choice(['62d58', '6339c', '53c68', '639bc']):s} {rng.randrange(0, 8)}")
            elif what < 0.7:
                ev.append(f"{t} play 0x17 {rng.choice(['0x15', '0x28', '0x29', '0x33', '0x65', '0x99'])}")
            else:
                ev.append(f"{t} ent {rng.randrange(0, 12)} 0x17 {rng.choice(['0x15', '0x28', '0x65', '0x99'])}")
    ev.sort(key=lambda s: int(s.split()[0]))
    return base + ev

def run(seed_dir, drive, stage, cap, frames):
    os.makedirs(seed_dir, exist_ok=True)
    dpath = os.path.join(seed_dir, "drive.txt")
    open(dpath, "w").write("\n".join(drive) + "\n")
    env = dict(os.environ, NIGHT_CENSUS="1", SDL_AUDIODRIVER="dummy")
    cmd = [BIN, "--res-dir", os.path.join(REPO, "NIGHT"), "--script", dpath,
           "--rig-dump", seed_dir, "--rig-hz", "1", "--frames", str(frames)]
    if stage:
        cmd += ["--warp-stage", str(stage)]
    t0 = time.time()
    try:
        r = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=cap)
        rc, out = r.returncode, (r.stdout or "") + (r.stderr or "")
    except subprocess.TimeoutExpired:
        rc, out = 124, ""
    log = os.path.join(seed_dir, "run.log")
    open(log, "w").write(out)
    return rc, out, time.time() - t0

def load_census(seed_dir, n):
    p = os.path.join(seed_dir, "census.bin")
    if not os.path.exists(p):
        p = os.path.join(seed_dir, "census.txt.bin")
    if not os.path.exists(p):
        return None
    b = bytearray(open(p, "rb").read())
    return b if len(b) == n else None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--episodes", default="1")
    ap.add_argument("--seeds", type=int, default=6)
    ap.add_argument("--workers", type=int, default=3)
    ap.add_argument("--cap", type=int, default=120)
    ap.add_argument("--frames", type=int, default=4000)
    ap.add_argument("--base-until", type=int, default=60000)
    ap.add_argument("--window", default="62000,140000")
    ap.add_argument("--events", type=int, default=60)
    ap.add_argument("--inject", action="store_true")
    ap.add_argument("--base", default="", help="seed-drive file (default: scripts/rig/drives/ep1_kill3.txt)")
    ap.add_argument("--rounds", type=int, default=1)
    ap.add_argument("--out", default="/tmp/rig/fuzz")
    a = ap.parse_args()

    stages = [int(x) for x in a.episodes.split(",")]
    lo, hi = (int(x) for x in a.window.split(","))
    os.makedirs(a.out, exist_ok=True)

    # function count straight from the generated table
    n = 0
    for line in open(os.path.join(REPO, "port", "src", "gen", "fn_table.c")):
        if line.startswith("const unsigned g_fn_count"):
            n = int(line.split("=")[1].strip().rstrip(";"))
    print(f"coverage universe: {n} shipped-named functions")

    covered = bytearray(n)          # union over the accepted corpus
    curve, crashes = [], []
    corpus = []                     # accepted (stage, drive) pairs

    for rnd in range(a.rounds):
        jobs = []
        for i in range(a.seeds):
            stage = stages[i % len(stages)]
            rng = random.Random(hash((rnd, i, stage)) & 0xFFFFFFFF)
            w_lo = lo + rng.randrange(0, max(1, (hi - lo) // 3))
            w_hi = min(hi, w_lo + rng.randrange(20000, max(21000, (hi - lo) * 2 // 3)))
            lo, hi_ = lo, hi
            src = corpus[rng.randrange(len(corpus))][1] if corpus and rnd else read_base(a.base_until, a.base)
            drive = src + mutate(rng, [], w_lo, w_hi, rng.randrange(a.events // 2, a.events * 2), a.inject)
            if a.base:
                drive = read_base(a.base_until, a.base) + mutate(rng, [], w_lo, w_hi,
                                                                 rng.randrange(a.events // 2, a.events * 2), a.inject)
            jobs.append((stage, drive, f"r{rnd}s{i}_st{stage}"))

        # run in parallel batches of --workers
        results = []
        queue = list(jobs)
        while queue:
            batch, queue = queue[:a.workers], queue[a.workers:]
            procs = []
            for stage, drive, name in batch:
                sd = os.path.join(a.out, name)
                os.makedirs(sd, exist_ok=True)
                open(os.path.join(sd, "drive.txt"), "w").write("\n".join(drive) + "\n")
                cmd = [BIN, "--res-dir", os.path.join(REPO, "NIGHT"), "--script", os.path.join(sd, "drive.txt"),
                       "--rig-dump", sd, "--rig-hz", "1", "--frames", str(a.frames)]
                if stage:
                    cmd += ["--warp-stage", str(stage)]
                lg = open(os.path.join(sd, "run.log"), "w")
                procs.append((name, stage, sd, subprocess.Popen(cmd, env=dict(os.environ, NIGHT_CENSUS="1",
                                                                             SDL_AUDIODRIVER="dummy"),
                                                                stdout=lg, stderr=subprocess.STDOUT)))
            for name, stage, sd, p in procs:
                try:
                    rc = p.wait(timeout=a.cap)
                except subprocess.TimeoutExpired:
                    p.kill(); p.wait(); rc = 124
                cen = load_census(sd, n)
                new = 0
                if cen:
                    for j in range(n):
                        if cen[j] and not covered[j]:
                            covered[j] = 1
                            new += 1
                if rc not in (0, 124):
                    # Crash attribution by COUNTERFACTUAL: re-run the same drive with every state write
                    # removed. If the crash disappears it was induced by the rig's own injection (an
                    # impossible game state, e.g. an entity count larger than the live chain), not by input.
                    lines = open(os.path.join(sd, "drive.txt")).read().splitlines()
                    inj = [l for l in lines if l.split()[1:2] in (["poke"], ["ent"], ["play"])]
                    if inj:
                        cf = os.path.join(sd, "drive_noinj.txt")
                        open(cf, "w").write("\n".join(
                            l for l in lines if l.split()[1:2] not in (["poke"], ["ent"], ["play"])) + "\n")
                        cfd = os.path.join(sd, "noinj")
                        os.makedirs(cfd, exist_ok=True)
                        cmd2 = [BIN, "--res-dir", os.path.join(REPO, "NIGHT"), "--script", cf,
                                "--rig-dump", cfd, "--rig-hz", "1", "--frames", str(a.frames)]
                        if stage:
                            cmd2 += ["--warp-stage", str(stage)]
                        try:
                            rc2 = subprocess.run(cmd2, env=dict(os.environ, NIGHT_CENSUS="1",
                                                                SDL_AUDIODRIVER="dummy"),
                                                 timeout=a.cap, stdout=subprocess.DEVNULL,
                                                 stderr=subprocess.DEVNULL).returncode
                        except subprocess.TimeoutExpired:
                            rc2 = 124
                    else:
                        rc2 = rc
                    kind = ("injection-induced (rc=" + str(rc) + " -> without injections rc=" + str(rc2) + ")"
                            if rc2 == 0 or (inj and rc2 != rc) else "input-only")
                    crashes.append((name, rc, sd, kind))
                    shutil.copy(os.path.join(sd, "drive.txt"), os.path.join(a.out, f"crash_{name}.txt"))
                    print(f"  CRASH {name} rc={rc} [{kind}]", flush=True)
                    if kind == "input-only":
                        print(f"    -> repro: {BIN} --res-dir NIGHT --script {sd}/drive.txt --warp-stage "
                              f"{stage} --rig-dump {sd}", flush=True)
                results.append((name, stage, rc, new))
                # Accept coverage-adding seeds, and keep a few non-adding ones for diversity: a plateau at
                # the same function set usually means the mutation window, not the drive, is exhausted.
                if new or rng_keep(name):
                    corpus.append((stage, open(os.path.join(sd, "drive.txt")).read().split("\n")))
                src = None
                print(f"  {name}: rc={rc} new_funcs={new} total={sum(covered)}", flush=True)
        curve.append(sum(covered))
        print(f"round {rnd}: coverage={sum(covered)}/{n} corpus={len(corpus)} crashes={len(crashes)}", flush=True)

    never = []
    import re as _re
    names = _re.findall(r'"([A-Za-z_][A-Za-z0-9_]*)"',
                        open(os.path.join(REPO, "port", "src", "gen", "fn_table.c")).read())
    names = [x for x in names if not x.startswith(("src/", "gen/"))]
    with open(os.path.join(a.out, "coverage.txt"), "w") as f:
        f.write(f"covered {sum(covered)}/{n}\n")
        for i, c in enumerate(covered):
            if not c and i < len(names):
                never.append(names[i])
        f.write("never entered (%d):\n" % len(never))
        f.write("\n".join(sorted(never)) + "\n")
    print(f"final coverage {sum(covered)}/{n}; never-entered {len(never)}; crashes {len(crashes)}")
    for c in crashes:
        print("  CRASH", c[0], "rc", c[1], "dir", c[2])

if __name__ == "__main__":
    sys.exit(main())
