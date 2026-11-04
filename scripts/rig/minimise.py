#!/usr/bin/env python3
"""Auto-repro minimiser: shrink a divergent drive to a minimal one and emit a witness bundle.

The rig produces whole-route reports; root-causing needs the opposite end — the smallest event list
that still diverges, plus everything a human (or another agent) needs to read the divergence without
re-running anything by hand.

  minimise.py --out DIR [--drive D] [--seconds S] [--inject NIGHT_RIG_CONTROL=no_timer]

Each candidate is a *fresh* route: the port runs in the host with the injected control, the shipped
game boots in a new container, and only the state oracle runs (cheapest oracle that carries a per-cell
verdict; the screen/audio oracles are reserved for the final bundle). Divergence is a count of
differing cells above a floor so one flaky cell cannot drive a shrink.

Then ddmin over the event lines (delta-debugging: drop chunks, keep any subset that still diverges),
then shrink the route length. Finally the bundle is built by re-running the *minimal* route through
route.sh with all four oracles, and witness/ is filled with both frames, the state diff, the audio
window and the port log, plus repro.sh which reproduces the same finding unattended.
"""
import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
GEN = os.path.join(ROOT, "scripts", "audit", "gen")


def run(cmd, timeout, env=None, cwd=ROOT):
    try:
        p = subprocess.run(cmd, timeout=timeout, capture_output=True, text=True, env=env, cwd=cwd)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired as e:
        return 124, (e.stdout or ""), "TIMEOUT"


def parse_tally(text):
    m = re.search(r"tally=\{(.*?)\}", text)
    if not m:
        return {}
    return {k.strip("'\" "): int(v) for k, v in
            (kv.split(":") for kv in m.group(1).split(","))}


TALLY = ["DIFF", "REC-DIFF", "PTR", "SIZE?", "REPR", "ENTROPY", "SAME", "REC-NULL"]
for _k in TALLY:
    globals()[_k.replace("-", "_")] = _k


def compare_dumps(a_run, b_run):
    """Differing DS cells between two port dumps of the same drive (clean build vs injected build).
    The measured entropy cells (docs/101) are excluded; pointer cells are counted separately because
    heap layout can differ between processes without any game-state meaning."""
    sys.path.insert(0, HERE)
    from diff_state import load_port
    A, cells = load_port(a_run)
    B, _ = load_port(b_run)
    try:
        ent = set(json.load(open(os.path.join(GEN, "entropy_cells.json")))["entropy_cells"])
    except OSError:
        ent = set()
    plain = [(c["offset"], c["host_size"], c["ds"]) for c in cells
             if not c.get("ptr") and hex(c["ds"]) not in ent]
    ptrs = [(c["offset"], c["host_size"], c["ds"]) for c in cells
            if c.get("ptr") and hex(c["ds"]) not in ent]
    n = min(len(A), len(B))
    if n == 0:
        return 0, {"records": 0, "cells_compared": len(plain)}
    differing, ptr_diff = set(), set()
    for i in range(n):
        ba, bb = A[i][1], B[i][1]
        for off, size, ds in plain:
            if ba[off:off + size] != bb[off:off + size]:
                differing.add(ds)
        for off, size, ds in ptrs:
            if ba[off:off + size] != bb[off:off + size]:
                ptr_diff.add(ds)
    return len(differing), {"records": n, "cells_compared": len(plain),
                            "ptr_cells_differing": len(ptr_diff)}


def probe(drive, dur, out, env_extra, baseline=None):
    """One fresh route. Returns (score, details).

    Default predicate: divergence from the shipped game (fresh container, state oracle).
    With `baseline`: injected build vs clean build on the same drive -- no container per candidate,
    and a fault that needs input cannot be reproduced by an empty drive, so the shrink has to keep the
    events that actually matter."""
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out, exist_ok=True)
    env = dict(os.environ)
    for kv in env_extra:
        env[kv.split("=", 1)[0]] = kv.split("=", 1)[1]
    if baseline is not None:
        clean = f"{out}/clean"
        os.makedirs(clean, exist_ok=True)
        rc, so, se = run(["./build/night", "--res-dir", "./NIGHT", "--script", drive,
                          "--rig-dump", clean, "--rig-hz", "10", "--frames", str(dur * 60)],
                         timeout=dur + 40, cwd=ROOT)
        if rc not in (0, 124) or not os.path.exists(f"{clean}/cells.bin"):
            return -1, {"stage": "port-clean", "rc": rc, "err": (se or so)[-300:]}
        rc, so, se = run(["./build/night", "--res-dir", "./NIGHT", "--script", drive,
                          "--rig-dump", f"{out}/port", "--rig-hz", "10",
                          "--frames", str(dur * 60)], timeout=dur + 40, env=env, cwd=ROOT)
        if rc not in (0, 124) or not os.path.exists(f"{out}/port/cells.bin"):
            return -1, {"stage": "port-injected", "rc": rc, "err": (se or so)[-300:]}
        return compare_dumps(clean, f"{out}/port")
    rc, so, se = run(["./build/night", "--res-dir", "./NIGHT", "--script", drive,
                      "--rig-dump", f"{out}/port", "--rig-hz", "10", "--frames", str(dur * 60)],
                     timeout=dur + 40, env=env)
    if rc not in (0, 124) or not os.path.exists(f"{out}/port/cells.bin"):
        return -1, {"stage": "port", "rc": rc, "err": (se or so)[-400:]}
    rc, so, se = run(["bash", f"{HERE}/bringup.sh"], timeout=180,
                     env={**env, "RUN_GAME": "1"})
    if rc != 0:
        return -1, {"stage": "bringup", "rc": rc, "err": (se or so)[-400:]}
    time.sleep(8)
    run(["python3", f"{HERE}/calibrate_ds.py"], timeout=120)
    rc, so, se = run(["python3", f"{HERE}/replay_original.py", "--drive", drive,
                      "--out", f"{out}/orig", "--seconds", str(dur), "--hz", "4"],
                     timeout=dur + 140)
    if rc != 0:
        return -1, {"stage": "replay", "rc": rc, "err": (se or so)[-400:]}
    rc, so, se = run(["python3", f"{HERE}/diff_state.py", "--orig", f"{out}/orig",
                      "--port", f"{out}/port", "--drive", drive, "--md", f"{out}/state.md"],
                     timeout=120)
    t = parse_tally(so)
    score = t.get("DIFF", 0) + t.get("REC-DIFF", 0)
    return score, {"tally": t, "port_rc": rc, "so_tail": so.strip().splitlines()[-1:]}


def split_drive(path):
    """(header lines that must stay, shrinkable event lines)"""
    head, ev = [], []
    for line in open(path):
        s = line.rstrip("\n")
        if not s.strip() or s.lstrip().startswith("#") or " anchor " in " " + s:
            head.append(s)
        else:
            ev.append(s)
    return head, ev


def write_drive(path, head, ev):
    with open(path, "w") as f:
        for l in head + sorted(ev, key=lambda s: int(s.split()[0])):
            f.write(l + "\n")


def ddmin(head, ev, dur, out, env_extra, min_diff, max_runs, log, baseline=None):
    """Classic delta-debugging on the event lines: keep any subset that still diverges."""
    runs = 0
    cur = list(ev)
    n = 2
    while n <= len(cur) and runs < max_runs:
        chunks = [cur[i::n] for i in range(n)]          # interleave: keeps temporal spread
        progressed = False
        for i, ch in enumerate(chunks):
            cand = [e for e in cur if e not in ch]
            if not cand:
                continue
            runs += 1
            write_drive(f"{out}/cand.txt", head, cand)
            score, det = probe(f"{out}/cand.txt", dur, f"{out}/probe", env_extra, baseline)
            log.append({"run": runs, "n": n, "dropped": len(ch), "kept": len(cand),
                        "dur": dur, "score": score, "tally": det.get("tally", {})})
            print(f"  run {runs:2d} n={n} drop={len(ch)} keep={len(cand)} dur={dur} "
                  f"score={score} {det.get('tally', {})}", flush=True)
            if score >= min_diff:
                cur, progressed = cand, True
                if len(cur) <= 1:
                    return cur, runs
                break
        if not progressed:
            n = min(n * 2, len(cur)) if n < len(cur) else n + 1
            if n > len(cur):
                break
    # single-line pass (the interleaved chunks above can leave a removable pair behind)
    for e in list(cur):
        if runs >= max_runs:
            break
        cand = [x for x in cur if x != e]
        if not cand:
            continue
        runs += 1
        write_drive(f"{out}/cand.txt", head, cand)
        score, det = probe(f"{out}/cand.txt", dur, f"{out}/probe", env_extra, baseline)
        log.append({"run": runs, "n": "single", "dropped": 1, "kept": len(cand), "dur": dur,
                    "score": score, "tally": det.get("tally", {})})
        print(f"  run {runs:2d} single drop={e!r} score={score} {det.get('tally', {})}", flush=True)
        if score >= min_diff:
            cur = cand
    return cur, runs


def shrink_duration(head, ev, dur, out, env_extra, min_diff, max_runs, log, baseline=None):
    best = dur
    cand = dur
    while cand > 4:
        cand //= 2
        write_drive(f"{out}/cand.txt", head, ev)
        score, det = probe(f"{out}/cand.txt", cand, f"{out}/probe", env_extra, baseline)
        log.append({"run": len(log) + 1, "n": "dur", "dropped": 0, "kept": len(ev), "dur": cand,
                    "score": score, "tally": det.get("tally", {})})
        print(f"  dur {cand}s score={score} {det.get('tally', {})}", flush=True)
        if score >= min_diff:
            best = cand
        else:
            break
        if len(log) >= max_runs:
            break
    return best


def grids(path):
    """(w, h, frames) from a frames.bin written by rig_sink / capture_original:
    header <III> = magic 'NGFR', version, grid length; then per frame a 32 B record
    <qQiiii> = t_ms, frame, w, h, changed, pad followed by grid bytes (40x30 luminance)."""
    hdr = struct.Struct("<III")
    rec = struct.Struct("<qQiiii")
    out = []
    with open(path, "rb") as f:
        magic, _ver, ngrid = hdr.unpack(f.read(12))
        if magic != 0x5247464E:
            return 0, 0, []
        while True:
            b = f.read(rec.size)
            if len(b) < rec.size:
                break
            t, frame, w, h, changed, _pad = rec.unpack(b)
            g = f.read(ngrid)
            if len(g) < ngrid:
                break
            out.append((t, frame, changed, g))
    return 40, 30, out


def witness_pgm(out_dir, port_frames, orig_frames, t_target, scale=6):
    """Both 40x30 luminance grids nearest t_target, side by side, upscaled: a PGM (no image deps)."""
    def nearest(frames):
        if not frames:
            return None
        return min(frames, key=lambda fr: abs(fr[0] - t_target))
    a, b = nearest(orig_frames), nearest(port_frames)
    if not a or not b:
        return None
    W, H = 40 * scale, 30
    rows = []
    for y in range(30):
        left = b"".join(bytes([g]) * scale for g in a[3][y * 40:(y + 1) * 40])
        right = b"".join(bytes([g]) * scale for g in b[3][y * 40:(y + 1) * 40])
        rows.append(left + b"\x00" + right)
    p = os.path.join(out_dir, "frames_side_by_side.pgm")
    with open(p, "wb") as f:
        f.write(f"P5\n{2*40*scale+1} 30\n255\n".encode())
        f.write(b"".join(rows))
    return {"file": p, "orig_t": a[0], "orig_frame": a[1], "port_t": b[0], "port_frame": b[1]}


def audio_slice(out_dir, wav, lo_ms, hi_ms):
    import wave
    if not os.path.exists(wav):
        return None
    w = wave.open(wav, "rb")
    p = w.getparams()
    rate, ch, sw = p.framerate, p.nchannels, p.sampwidth
    fr = w.getnframes()
    lo = max(0, min(fr, lo_ms * rate // 1000))
    hi = max(lo, min(fr, hi_ms * rate // 1000))
    w.setpos(lo)
    data = w.readframes(hi - lo)
    w.close()
    q = os.path.join(out_dir, "audio_window.wav")
    o = wave.open(q, "wb")
    o.setnchannels(ch); o.setsampwidth(sw); o.setframerate(rate)
    o.writeframes(data); o.close()
    return {"file": q, "ms": [lo_ms, hi_ms], "frames": hi - lo}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--drive", default=os.path.join(HERE, "drives", "title_basic.txt"))
    ap.add_argument("--seconds", type=int, default=20)
    ap.add_argument("--inject", default="", help="env assignment for the port, e.g. "
                                                 "NIGHT_RIG_CONTROL=no_timer (the synthetic divergence)")
    ap.add_argument("--min-diff", type=int, default=3, help="cell count that counts as divergent")
    ap.add_argument("--max-runs", type=int, default=18)
    ap.add_argument("--no-shrink-time", action="store_true")
    ap.add_argument("--baseline", action="store_true",
                    help="score = cells where the injected build differs from the clean build on the "
                         "same drive (no container per candidate; an input-dependent fault cannot be "
                         "reproduced by an empty drive, so the shrink must keep the key presses that "
                         "matter)")
    a = ap.parse_args()

    os.makedirs(a.out, exist_ok=True)
    env_extra = [a.inject] if a.inject else []
    head, ev = split_drive(a.drive)
    log = []
    print(f"baseline: {len(ev)} events, {a.seconds}s")
    score, det = probe(a.drive, a.seconds, f"{a.out}/baseline", env_extra, a.baseline)
    print(f"  baseline score={score} {det.get('tally', det)}")
    if score < a.min_diff:
        sys.exit("baseline does not diverge; nothing to minimise (use --inject for a synthetic "
                 "divergence, or lower --min-diff)")
    print("ddmin over event lines")
    ev_min, runs = ddmin(head, ev, a.seconds, a.out, env_extra, a.min_diff, a.max_runs, log,
                         a.baseline)
    dur_min = a.seconds
    if not a.no_shrink_time:
        print("shrink route length")
        dur_min = shrink_duration(head, ev_min, a.seconds, a.out, env_extra, a.min_diff,
                                  a.max_runs - runs, log, a.baseline)
    with open(f"{a.out}/minimal.txt", "w") as f:
        f.write(f"# minimal repro: {len(ev_min)} events, {dur_min}s "
                f"(from {len(ev)} events, {a.seconds}s; {runs} probes)\n")
        for l in head + sorted(ev_min, key=lambda s: int(s.split()[0])):
            f.write(l + "\n")

    # ---- witness bundle: full-fidelity route on the minimal drive -----------------------
    print("bundle: full route (all four oracles) on the minimal drive")
    penv = f"{a.inject}" if a.inject else ""
    rc, so, se = run(["bash", f"{HERE}/route.sh", f"{a.out}/minimal.txt", str(dur_min)],
                     timeout=dur_min + 600, env={**os.environ, "OUT": f"{a.out}/route",
                                                 "PORT_ENV": penv})
    print(f"  route rc={rc}")
    wit = f"{a.out}/witness"
    os.makedirs(wit, exist_ok=True)
    for src, dst in [(f"{a.out}/minimal.txt", "minimal.txt"),
                     (f"{a.out}/route/DIVERGENCE.md", "DIVERGENCE.md"),
                     (f"{a.out}/route/report_state.md", "report_state.md"),
                     (f"{a.out}/route/report_screen.md", "report_screen.md"),
                     (f"{a.out}/route/report_audio.md", "report_audio.md"),
                     (f"{a.out}/route/port.log", "port.log"),
                     (f"{a.out}/route/port/frames.bin", "port_frames.bin"),
                     (f"{a.out}/route/orig_screen/frames.bin", "orig_frames.bin"),
                     (f"{a.out}/route/orig/audio.wav", "orig_audio.wav")]:
        if os.path.exists(src):
            shutil.copy(src, os.path.join(wit, dst))
    finding = {}
    try:
        txt = open(f"{a.out}/route/report_screen.txt").read()
        m = re.search(r"orig t=(\d+)\.\.(\d+) ms .*?worst score=([\d.]+)", txt)
        if m:
            finding = {"t0": int(m.group(1)), "t1": int(m.group(2)), "score": float(m.group(3))}
    except OSError:
        pass
    if not finding:
        finding = {"t0": 0, "t1": dur_min * 1000, "score": None}
    manifest = {"drive_events": ev_min, "seconds": dur_min, "inject": a.inject or "none",
                "finding_window_ms": [finding["t0"], finding["t1"]], "probes": runs}
    if os.path.exists(f"{wit}/orig_frames.bin") and os.path.exists(f"{wit}/port_frames.bin"):
        _, _, OF = grids(f"{wit}/orig_frames.bin")
        _, _, PF = grids(f"{wit}/port_frames.bin")
        mid = (finding["t0"] + finding["t1"]) // 2
        manifest["frames"] = witness_pgm(wit, PF, OF, mid)
    if os.path.exists(f"{wit}/orig_audio.wav"):
        manifest["audio"] = audio_slice(wit, f"{wit}/orig_audio.wav",
                                        finding["t0"], finding["t1"] + 500)
    repro = f"""#!/usr/bin/env bash
# Unattended reproduction of the finding in this bundle.
#   finding: window {finding['t0']}..{finding['t1']} ms, screen score {finding['score']}
#   port control injected: {a.inject or 'none'}
set -uo pipefail
cd "{ROOT}"
OUT={a.out}/repro-route PORT_ENV="{penv}" timeout $(({dur_min} + 600)) \\
    bash scripts/rig/route.sh {a.out}/minimal.txt {dur_min}
grep -hE "^(tally|divergent runs|pacing check|whole-route|original:|port  )" \\
     {a.out}/repro-route/report_state.txt {a.out}/repro-route/report_screen.txt \\
     {a.out}/repro-route/report_audio.txt 2>/dev/null
"""
    with open(f"{wit}/repro.sh", "w") as f:
        f.write(repro)
    os.chmod(f"{wit}/repro.sh", 0o755)
    with open(f"{a.out}/runs.jsonl", "w") as f:
        for row in log:
            f.write(json.dumps(row) + "\n")
    manifest["runs"] = log
    with open(f"{wit}/manifest.json", "w") as f:
        json.dump(manifest, f, indent=1)
    print(f"wrote {a.out}/minimal.txt and {wit}/")
    print(f"  minimal: {len(ev_min)} events / {dur_min}s from {len(ev)} events / {a.seconds}s "
          f"({runs} probes)")
    print(f"  window {manifest['finding_window_ms']}  frames {bool(manifest.get('frames'))} "
          f"audio {bool(manifest.get('audio'))}")


if __name__ == "__main__":
    main()
