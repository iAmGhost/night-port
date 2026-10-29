#!/usr/bin/env python3
"""SFX oracle with the music masked out (docs/164 §4 item 3, §7).

The container capture (`orig/audio.wav`) is DOSBox's MIXER output: the Sound Blaster board and CD-DA
are summed into it.  The port's board witness (`port/pcm.wav`) is board-only.  Comparing them raw scores
"does the original make noise" against "does the port's board make noise", which is why every fight window
used to read SILENT: the shipped side's activity was mostly the music.

The shipped side's CD-DA selection is visible in DS: 0x53c44 holds the current track index (docs/131 used
that cell to phase-lock the two audio timelines), and it is sampled at the dump cadence in the same session
as the capture.  So the CD-armed intervals are known, and SFX activity can be scored in the intervals where
NO track is selected -- which is the only part of the capture the port's board stream is comparable against.

Usage:
  scripts/rig/sfx_mask.py --run /tmp/rig/g16 [--drive scripts/rig/drives/ep1_kill3.txt]
"""
import argparse
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_audio import read_mono, envelope, features, phases_from_drive, slice_buckets, BUCKET_MS  # noqa: E402

def audio_music_windows(capture_wav, cdda_raw, hop_s=2.0, win_s=2.0, npeaks=6):
    """Which seconds of the shipped MIXER capture contain disc music, judged by content rather than by a DS
    cell (docs/165 §4: a stop leaves the last LBA in 0x53c44, so the cell cannot say when the drive is silent).
    The port's own CD-DA stream (cdda.raw, byte-faithful to the disc -- docs/114) supplies the templates;
    presence is the peak-set IoU of docs/131, whose measured band is 0.966 for a real match against 0.19-0.43
    for unrelated material."""
    import numpy as np
    from music_melody import peak_sets, iou
    cx, crate = read_mono(capture_wav)
    cx = np.asarray(cx, dtype=np.float64)
    raw = open(cdda_raw, "rb").read()
    frames = len(raw) // 4
    st = np.frombuffer(raw[:frames * 4], dtype="<i2").reshape(frames, 2)
    tx = st.mean(axis=1)
    cap = peak_sets(cx, crate, hop_s, win_s, npeaks)
    tpl = peak_sets(tx, 44100, hop_s, win_s, npeaks)
    tpl = [t for t in tpl if t]
    out = []
    for i, c in enumerate(cap):
        t0 = i * hop_s
        if not c:
            out.append((t0, t0 + win_s, 0.0))
            continue
        best = max((iou(c, t) for t in tpl), default=0.0)
        out.append((t0, t0 + win_s, best))
    return out


DS_LO = 0x50000                     # docs/124: the dump's DS window starts at the segment base, not 0x51000
TRACK_CELL = 0x53c44
TRACK_ALT = (0x53c40, 0x53c48)


def orig_series(run):
    """(t_capture_ms, t_nominal_ms, track_index) for every sampled state."""
    tl = json.load(open(os.path.join(run, "timeline.json")))
    reb = tl.get("rebases") or []

    def nominal(t_ms):
        off = reb[0]["nominal"] - reb[0]["t_ms"] if reb else 0
        for r in reb:
            if r["t_ms"] <= t_ms:
                off = r["nominal"] - r["t_ms"]
            else:
                break
        return t_ms + off

    out = []
    for i, st in enumerate(tl["timeline"]):
        for cand in (f"state_{i:04d}.ds.bin", f"state_{i + 1:04d}.ds.bin"):
            p = os.path.join(run, cand)
            if os.path.exists(p):
                break
        else:
            continue
        ds = open(p, "rb").read()
        off = TRACK_CELL - DS_LO
        if off + 4 > len(ds):
            continue
        tc = int(st["t_wall"] * 1000)
        out.append((tc, nominal(tc), struct.unpack_from("<I", ds, off)[0]))
    return out


def port_windows(port_dir):
    """Port board PCM placed in real time from the per-record byte counters (docs/129 §2)."""
    rec = struct.Struct("<qQQQII")
    rb = json.load(open(os.path.join(port_dir, "meta.json")))["record_bytes"]
    rows = []
    with open(os.path.join(port_dir, "cells.bin"), "rb") as f:
        f.read(16)
        while True:
            b = f.read(rec.size)
            if len(b) < rec.size:
                break
            rows.append(rec.unpack(b))
            f.read(rb)
    board = 0
    log = os.path.join(port_dir, "port.log")
    if os.path.exists(log):
        import re
        m = [int(x) for x in re.findall(r"audio=(\d+)Hz", open(log).read())]
        if m:
            board = max(set(m), key=m.count)
    sp, rp = read_mono(os.path.join(port_dir, "pcm.wav"))
    if not board:
        board = rp
    out, pos = [], 0
    for (t0, _, a0, _, _, _), (t1, _, a1, _, _, _) in zip(rows, rows[1:]):
        want = max(0, a1 - a0) // 2
        out.append((t0, t1, sp[pos:pos + want], board))
        pos += want
    return out, len(sp), pos, board


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", required=True, help="a route.sh output directory")
    ap.add_argument("--drive", help="drive script, for the phase windows")
    ap.add_argument("--loud", type=float, default=600.0,
                    help="bucket RMS above which the capture is treated as music-dominated (sanity check only)")
    ap.add_argument("--md")
    ap.add_argument("--max-buckets", type=int, default=4000)
    ap.add_argument("--mask", choices=("ds", "audio"), default="ds",
                    help="ds = track LBA cell (blind to stop gaps, docs/165 §4); "
                         "audio = content match against the port's byte-faithful disc stream")
    ap.add_argument("--iou", type=float, default=0.6, help="peak-set IoU counting as disc music (docs/131 band)")
    a = ap.parse_args()

    run = a.run.rstrip("/")
    odir, pdir = os.path.join(run, "orig"), os.path.join(run, "port")
    series = orig_series(odir)
    if not series:
        sys.exit("no state dumps under " + odir)

    # A single "shift" is WRONG here and was the first version's defect: the replay releases each drive
    # segment when its `wait ds` fires, so the session's capture clock and the drive's nominal clock are
    # related PIECEWISE (docs/158's anchor rebase is a jump, not a constant).  Map bucket -> nominal by
    # walking the state pairs and interpolating between neighbours, and refuse to guess outside the range
    # the capture actually covers.
    pairs_ct = sorted((int(st[0]), st[1]) for st in series)
    monotone = []
    best = None
    for c, n in pairs_ct:
        best = n if best is None else max(best, n)
        monotone.append((c, best))
    nom_of_cap = {}
    for b in range(0, a.max_buckets):
        t = b * BUCKET_MS
        for (c0, n0), (c1, n1) in zip(monotone, monotone[1:]):
            if c0 <= t <= c1:
                span = c1 - c0
                frac = 0.0 if span == 0 else (t - c0) / span
                nom_of_cap[t] = int(n0 + (n1 - n0) * frac)
                break
        else:
            if monotone and monotone[-1][0] <= t and monotone[-1][0] != monotone[0][0]:
                nom_of_cap[t] = monotone[-1][1]      # past the last sample: hold the last nominal time
    covered = (monotone[0][1], monotone[-1][1]) if monotone else (0, 0)
    print(f"capture clock -> drive nominal: piecewise over {len(monotone)} states, "
          f"nominal covered [{covered[0]}..{covered[1]}] ms; "
          f"endpoint offsets: first {monotone[0][1] - monotone[0][0]} ms, last {monotone[-1][1] - monotone[-1][0]} ms")

    tracks = {}
    for c, n, v in series:
        tracks[c] = v
    times = sorted(tracks)

    # The cell is not an index: the shipped values measured on /tmp/rig/g16 are 0x2d320, 0x367aa and 0x9ddf,
    # i.e. the track LBAs of docs/106.  A selection persists until it is replaced or stopped (docs/106's
    # teardown invariant), so "armed" means "the last state at or before this nominal time had a non-zero
    # LBA" -- NOT "a sample happened to fall within +-200 ms", which is what the first version did and what
    # invented 180 phantom CD-silent buckets at the rebase gaps.
    sel = [(n, tracks[c]) for c, n in monotone]
    sel.sort()

    def armed(nominal_ms):
        import bisect
        i = bisect.bisect_right(sel, (nominal_ms + 200, 1 << 30)) - 1
        return i >= 0 and bool(sel[i][1])

    so, ro = read_mono(os.path.join(odir, "audio.wav"))
    eo = envelope(so, ro)
    n_buckets = len(eo)

    def nom_of(b):
        return nom_of_cap.get(b * BUCKET_MS)

    def bucket_ms_of(b):
        """drive-nominal milliseconds for bucket b (None when the capture does not cover it)"""
        return nom_of(b)

    def feats(pairs):
        """features over (t_nominal_ms, rms, zcr, adjacent) samples.  `adjacent` says whether this bucket is
        the next one in CAPTURE order: the clock map is piecewise, so nominal time jumps at a rebase and
        adjacency must come from the capture, not from the nominal difference (docs/158)."""
        if not pairs:
            return {"buckets": 0, "onsets": 0, "active_frac": 0.0, "mean_rms": 0.0, "mean_zcr": 0.0}
        thr = 300.0
        onsets = act = 0
        rms_sum = zc_sum = 0.0
        prev_act = None
        prev_adj = None
        for t, rms, zc, adj in pairs:
            cur = rms > thr
            if cur and not (prev_act and prev_adj):
                onsets += 1
            act += 1 if cur else 0
            rms_sum += rms
            zc_sum += zc
            prev_act, prev_adj = cur, adj
        n = len(pairs)
        return {"buckets": n, "onsets": onsets, "active_frac": round(act / n, 3),
                "mean_rms": round(rms_sum / n, 1), "mean_zcr": round(zc_sum / n, 3)}

    # `mapped` = capture buckets the clock map covers; `cd_on` = whether a CD-DA track was selected there.
    mapped = [b for b in range(n_buckets) if bucket_ms_of(b) is not None]
    cd_on = {b: armed(bucket_ms_of(b)) for b in mapped}

    # The audio mask is the one that can see silence: a bucket is "music" only if its own content matches the
    # port's byte-faithful disc stream.  Bucket times here are CAPTURE-clock, which is what the content was
    # taken from.
    if a.mask == "audio":
        cw, cr = os.path.join(odir, "audio.wav"), os.path.join(pdir, "cdda.raw")
        if not os.path.exists(cr):
            print(f"audio mask unavailable: no {cr}; falling back to the DS-cell mask")
        else:
            music = audio_music_windows(cw, cr)
            n_ok = sum(1 for w in music if w[2] >= a.iou)
            print(f"audio mask: {len(music)} capture windows, best peak-set IoU anywhere "
                  f"{max((w[2] for w in music), default=0):.2f}; {n_ok} windows at or above {a.iou}")
            for w in sorted(music, key=lambda w: -w[2])[:6]:
                print(f"     capture {w[0]:6.1f}-{w[1]:<6.1f}s  IoU={w[2]:.2f}")
            cd_on = {b: any(w[0] - 1.0 <= b * BUCKET_MS / 1000.0 < w[1] + 1.0 and w[2] >= a.iou
                            for w in music) for b in mapped}

    def triple(b, prev_b):
        return (bucket_ms_of(b), eo[b][0], eo[b][1], prev_b is not None and b - prev_b == 1)

    cd_bs = [b for b in mapped if cd_on[b]]
    sfx_bs = [b for b in mapped if not cd_on[b]]
    feat_all = feats([triple(mapped[i], mapped[i - 1] if i else None) for i in range(len(mapped))])
    pairs_cd = [triple(b, None) for b in cd_bs]
    pairs_sfx = [triple(b, None) for b in sfx_bs]
    pairs_cd = [triple(cd_bs[i], cd_bs[i - 1] if i else None) for i in range(len(cd_bs))]
    pairs_sfx = [triple(sfx_bs[i], sfx_bs[i - 1] if i else None) for i in range(len(sfx_bs))]
    print(f"capture buckets mapped to nominal: {len(mapped)} of {n_buckets}")
    feat_cd, feat_sfx = feats(pairs_cd), feats(pairs_sfx)
    print(f"capture: {n_buckets} buckets @44100 -> CD-armed {len(pairs_cd)}, CD-silent {len(pairs_sfx)}")
    print(f"  whole capture    {feat_all}")
    print(f"  during CD-DA     {feat_cd}")
    print(f"  CD-silent (= the board's share) {feat_sfx}")
    loud = sum(1 for b in mapped if eo[b][0] > a.loud)
    both = sum(1 for b in mapped if cd_on[b] and eo[b][0] > a.loud)
    print(f"  sanity: buckets over RMS {a.loud:.0f}: {loud}, of which CD-armed: {both} "
          f"({(both / max(1, loud)) * 100:.0f}% of the loud capture is music)")
    # where is the board's share, in nominal (drive) time?  printed so a reader can sanity check that a
    # "CD-silent" interval really is one and not a track that kept playing after its index was cleared.
    runs = []
    for t, rms, _, _ in pairs_sfx:
        if runs and t - runs[-1][1] <= BUCKET_MS:
            runs[-1][1] = t
            runs[-1][2] += 1
            runs[-1][3] = max(runs[-1][3], rms)
        else:
            runs.append([t, t, 1, rms])
    print("  CD-silent runs (nominal ms, buckets, peak rms):")
    for r in runs[:12]:
        print(f"     {r[0]:>7}-{r[1]:<7} n={r[2]:<4} peak={r[3]:7.1f}")

    wins, total, placed, board = port_windows(pdir)
    # Port board PCM placed on a bucket map in nominal time, same bucket size as the capture.
    port_map = {}
    for (t0, t1, seg, rate) in wins:
        if not seg:
            continue
        step = max(1, int(rate * BUCKET_MS / 1000.0))
        pos = 0
        b = 0
        while pos < len(seg):
            part = seg[pos:pos + step]
            pos += step
            rms = (sum(x * x for x in part) / len(part)) ** 0.5
            zc = sum(1 for i in range(1, len(part)) if (part[i] < 0) != (part[i - 1] < 0)) / max(1, len(part))
            port_map[int(t0 + b * 1000.0 / rate)] = (rms, zc)
            b += 1
    print(f"port board PCM: {placed}/{total} samples in {sum(1 for w in wins if w[2])}/{len(wins)} record "
          f"windows at {board} Hz -> {len(port_map)} buckets")
    pkeys = sorted(port_map)
    print("port board activity (nominal ms):",
          f"{feats([(t, port_map[t][0], port_map[t][1], i and t - pkeys[i - 1] <= BUCKET_MS)
                    for i, t in enumerate(pkeys)])}")

    if a.drive:
        print("\nper-phase, board-share only (shipped scored where NO track is selected)")
        print(f"{'window':14s} {'orig SFX on/act/rms':30s} {'port on/act/rms':26s} verdict")
        rows = []
        for label, lo, hi in phases_from_drive(a.drive):
            co = feats([p for p in pairs_sfx if lo <= p[0] < hi])
            sel = [t for t in pkeys if lo <= t < hi]
            cp = feats([(t, port_map[t][0], port_map[t][1], i and t - sel[i - 1] <= BUCKET_MS)
                        for i, t in enumerate(sel)])
            # A phase with no mapped capture bucket says nothing about the shipped side: the capture is a
            # window, not the whole route.  Saying "EXTRA" there would credit the port with an excess the
            # shipped side was never observed making.
            covered = any(lo <= bucket_ms_of(b) < hi for b in mapped)
            v = "NO-DATA"
            if not covered:
                v = "NO-CAPTURE"
            elif co["buckets"] == 0 and cp["buckets"] == 0:
                v = "NO-CD-SILENCE"
            elif co["buckets"] == 0:
                v = "NO-CD-SILENCE"
            elif co["onsets"] >= 3 and cp["onsets"] == 0:
                v = "MISSING"
            elif cp["onsets"] >= 3 and co["onsets"] == 0:
                v = "EXTRA"
            elif co["active_frac"] >= 0.2 and cp["active_frac"] < 0.02:
                v = "SILENT"
            elif co["onsets"] or cp["onsets"]:
                v = "BOTH-ACTIVE"
            rows.append((label, lo, hi, co, cp, v))
            print(f"{label:14s} {co['onsets']:3d} {co['active_frac']:.2f} {co['mean_rms']:8.1f}      "
                  f"{cp['onsets']:3d} {cp['active_frac']:.2f} {cp['mean_rms']:8.1f}   {v}")
        if a.md:
            with open(a.md, "w") as f:
                f.write("# SFX oracle with CD-DA masked out (docs/164 §7, docs/165)\n\n")
                f.write("- clock mapping: piecewise capture -> nominal over the state series\n")
                f.write(f"- mask basis: {a.mask}\n")
                f.write(f"- capture buckets mapped: {len(mapped)}; armed {len(pairs_cd)}, "
                        f"unarmed (the board's share) {len(pairs_sfx)}\n")
                f.write(f"- port board PCM: {placed} samples in {sum(1 for w in wins if w[2])} record windows "
                        f"at {board} Hz\n\n")
                f.write("| window | orig SFX onsets | orig SFX active | orig rms | port onsets | port active | verdict |\n")
                f.write("|---|---|---|---|---|---|---|\n")
                for label, lo, hi, co, cp, v in rows:
                    f.write(f"| {label} ({lo}-{hi} ms) | {co['onsets']} | {co['active_frac']} | "
                            f"{co['mean_rms']:.1f} | {cp['onsets']} | {cp['active_frac']} | {v} |\n")


if __name__ == "__main__":
    main()
