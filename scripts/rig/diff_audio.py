#!/usr/bin/env python3
"""Audio oracle: the container's captured WAV vs the port's PCM witness, compared by event window.

Available and measured (it initially looked unavailable because `/openapi.json` omits them):
`POST /api/v1/capture/audio/{start,stop}` writes `capture/audioNNNN.wav` inside the container, and the
capture taps the mixer rather than the host device, so it produces real samples even with
`SDL_AUDIODRIVER=dummy` (measured: 44100 Hz stereo, RMS 786, 142 onsets over ~6 s at the title).

The comparison is deliberately not waveform identity: the two sides do not share a clock, the port's
board path runs at 22050 Hz mono, and the original's mixer runs at 44100 Hz stereo. What is comparable
is *activity*: the RMS envelope, the fraction of the window that is audible, the onset count, and the
zero-crossing rate (a cheap music/SFX/silence discriminator). Each phase of the drive is scored on
those, and verdicts name the window and the missing/extra onset count.

  diff_audio.py --orig WAV --port WAV --drive DRIVE [--md OUT.md]
"""
import argparse
import json
import os
import re
import struct
import sys
import wave

BUCKET_MS = 100


def read_mono(path):
    """A WAV from either side. Falls back to treating the file as raw 16-bit PCM: the port's witness
    header has been wrong before, and a header problem must not silently become an audio verdict."""
    try:
        return _read_wav(path)
    except Exception as e:
        raw = open(path, "rb").read()
        print(f"{path}: not a readable WAV ({e}); treating the body as raw 16-bit mono",
              file=sys.stderr)
        s = struct.unpack("<%dh" % (len(raw) // 2), raw)
        return s, 22050


def _read_wav(path):
    w = wave.open(path, "rb")
    if w.getnframes() == 0 and os.path.getsize(path) > 44:
        w.close()
        raise ValueError("header declares zero frames (sizes never finalised)")
    n, ch, sw, rate = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
    d = w.readframes(n)
    if sw != 2:
        sys.exit(f"{path}: {sw}-bit samples unsupported (want 16-bit PCM)")
    s = struct.unpack("<%dh" % (len(d) // 2), d)
    if ch == 2:
        s = tuple((s[i] + s[i + 1]) // 2 for i in range(0, len(s) - 1, 2))
    return s, rate


def envelope(s, rate):
    """(rms per 100 ms bucket, zero-crossing rate per bucket)"""
    step = max(1, rate // 10)
    out = []
    for i in range(0, len(s), step):
        seg = s[i:i + step]
        if not seg:
            break
        rms = (sum(x * x for x in seg) / len(seg)) ** 0.5
        zc = sum(1 for a, b in zip(seg, seg[1:]) if (a < 0) != (b < 0)) / len(seg)
        out.append((rms, zc))
    return out


def envelope_from_windows(windows, bucket_ms=BUCKET_MS, rate_fallback=0.0):
    """RMS/zcr buckets placed at real time: `windows` is [(t0_ms, t1_ms, samples, rate)] and the
    sample count in each window decides how much of that window's time is actually audible."""
    import math
    buckets = {}
    for t0, t1, seg, rate in windows:
        if not seg:
            continue
        if not rate:
            continue
        step = max(1, int(rate * bucket_ms / 1000.0))
        for j in range(0, len(seg), step):
            part = seg[j:j + step]
            if len(part) < step // 2:
                continue
            rms = (sum(x * x for x in part) / len(part)) ** 0.5
            zc = sum(1 for a, b in zip(part, part[1:]) if (a < 0) != (b < 0)) / len(part)
            buckets[int(t0 + j * 1000.0 / rate)] = (rms, zc)
    if not buckets:
        return []
    keys = sorted(buckets)
    lo, hi = keys[0], keys[-1]
    out = []
    for ms in range(lo, hi + bucket_ms, bucket_ms):
        out.append(buckets.get(ms, (0.0, 0.0)))
    return out


def features(buckets, thresh=300.0):
    onsets, active, loud = 0, 0, []
    prev = False
    for rms, zc in buckets:
        cur = rms > thresh
        if cur and not prev:
            onsets += 1
        if cur:
            active += 1
            loud.append(zc)
        prev = cur
    return {"buckets": len(buckets), "onsets": onsets,
            "active_frac": round(active / max(1, len(buckets)), 3),
            "mean_rms": round(sum(r for r, _ in buckets) / max(1, len(buckets)), 1),
            "mean_zcr": round(sum(loud) / len(loud), 3) if loud else 0.0}


def phases_from_drive(path):
    edges = []
    for line in open(path):
        f = line.split("#")[0].split()
        if len(f) >= 4 and f[1] == "key" and f[3].startswith("d"):
            edges.append(int(f[0]))
    edges = sorted(set([0] + edges))
    return [(f"P{i}@{t0}", t0, edges[i + 1] if i + 1 < len(edges) else 1 << 40)
            for i, t0 in enumerate(edges)]


def slice_buckets(buckets, lo, hi):
    """buckets are 100 ms apart and start at the capture's own t=0; the drive's phase times are
    relative to the same anchor, so the slice is a straight index range."""
    a, b = lo // (BUCKET_MS * 1), min(len(buckets), hi // (BUCKET_MS * 1))
    return buckets[a:b]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True, help="WAV captured inside the container")
    ap.add_argument("--port", required=True, help="WAV from night --rig-dump (pcm.wav)")
    ap.add_argument("--drive", required=True)
    ap.add_argument("--port-cells", help="the port's cells.bin: where the PCM was produced, per window")
    ap.add_argument("--port-log", help="the port's log, for the board rate its pump reports")
    ap.add_argument("--md")
    ap.add_argument("--missing-onsets", type=int, default=3,
                    help="onset deficit that counts as missing audio in a window")
    a = ap.parse_args()

    so, ro = read_mono(a.orig)
    sp, rp = read_mono(a.port)
    if a.port_cells and os.path.exists(a.port_cells):
        # The port's WAV header carries a *nominal* rate, but the board rate is whatever DSP command
        # 0x40 last programmed (measured 8000 Hz at the title) and PCM exists only while a transfer is
        # running. Reconstruct the envelope in real time instead: each record reports how many PCM
        # bytes were produced since the previous one, which fixes both the sample-to-time mapping and
        # the local rate. (Assuming the PCM spans the whole run, as a fixed rate would, slides the
        # envelope and invents silence in the middle of a route.)
        rec = struct.Struct("<qQQQII")
        pd = a.port_cells if os.path.isdir(a.port_cells) else os.path.dirname(a.port_cells)
        rb = json.load(open(os.path.join(pd, "meta.json")))["record_bytes"]
        if not a.port_log:
            for lg in (os.path.join(pd, "port.log"), os.path.join(pd, "..", "port.log")):
                if os.path.exists(lg):
                    a.port_log = lg
                    break
        rows = []
        with open(a.port_cells, "rb") as f:
            f.read(16)                              # NGRC header
            while True:
                b = f.read(rec.size)
                if len(b) < rec.size:
                    break
                rows.append(rec.unpack(b))          # t_ms, frame, audio_bytes, screen, tick, n
                f.read(rb)                          # the cell blob follows every record
        if len(rows) >= 2:
            # The board rate is whatever DSP command 0x40 last programmed (measured 8000 Hz in this
            # build), and the WAV header only carries a nominal 22050, so take the rate from the port's
            # own pump line and place the samples where the record timeline says they were produced.
            board = rp
            if a.port_log and os.path.exists(a.port_log):  # noqa: E501
                m = [int(x) for x in re.findall(r"audio=(\d+)Hz", open(a.port_log).read())]
                if m:
                    # the board rate is programmed per transfer, so the log shows several values; the
                    # one the pump reported most often is the rate the bulk of the PCM was produced at
                    board = max(set(m), key=m.count)
            samples, pos, out = sp, 0, []
            for (t0, _, a0, _, _, _), (t1, _, a1, _, _, _) in zip(rows, rows[1:]):
                dt = (t1 - t0) / 1000.0
                if dt <= 0:
                    continue
                want = max(0, (a1 - a0)) // 2       # 16-bit samples produced in this window
                seg = samples[pos:pos + want]
                pos += want
                out.append((t0, t1, seg, board))
            placed = sum(len(o[2]) for o in out)
            active = sum(1 for o in out if o[2])
            print(f"port PCM: {placed}/{len(sp)} samples placed into {active}/{len(out)} record windows"
                  f" at the board rate {board} Hz ({placed/board:.1f} s of sound in "
                  f"{(out[-1][0]-out[0][0])/1000.0:.1f} s of route)")
            ep, rp = envelope_from_windows(out, rate_fallback=board), rp
            if placed < len(sp):
                print(f"  note: {len(sp)-placed} samples in the WAV were not accounted for by the "
                      f"record byte counters; the envelope may under-report")
    eo, ep = envelope(so, ro), envelope(sp, rp)
    print(f"original: {len(so)/ro:.1f} s @{ro} Hz -> {len(eo)} buckets")
    print(f"port    : {len(sp)/rp:.1f} s @{rp} Hz -> {len(ep)} buckets")
    print("whole-route features")
    fo, fp = features(eo), features(ep)
    print(f"  original {fo}")
    print(f"  port     {fp}")

    phases = phases_from_drive(a.drive)
    rows = []
    for label, lo, hi in phases:
        co = features(slice_buckets(eo, lo, hi))
        cp = features(slice_buckets(ep, lo, hi))
        verdict = "SAME"
        if co["onsets"] >= a.missing_onsets and cp["onsets"] == 0:
            verdict = "MISSING"
        elif cp["onsets"] >= a.missing_onsets and co["onsets"] == 0:
            verdict = "EXTRA"
        elif co["active_frac"] >= 0.2 and cp["active_frac"] < 0.02:
            verdict = "SILENT"
        elif co["onsets"] and cp["onsets"] and (co["onsets"] / max(1, cp["onsets"]) > 3
                                                or cp["onsets"] / max(1, co["onsets"]) > 3):
            verdict = "RATE"
        elif abs(co["mean_zcr"] - cp["mean_zcr"]) > 0.15:
            verdict = "TIMBRE"
        rows.append((label, lo, hi, co, cp, verdict))
        print(f"  {label:12s} orig onsets={co['onsets']:4d} active={co['active_frac']:.2f} "
              f"rms={co['mean_rms']:7.1f} zcr={co['mean_zcr']:.3f} | "
              f"port onsets={cp['onsets']:4d} active={cp['active_frac']:.2f} "
              f"rms={cp['mean_rms']:7.1f} zcr={cp['mean_zcr']:.3f}  [{verdict}]")
    sched = []
    if a.port:
        cd = load_cdda(a.port)
        if cd and cd[0]:
            sched = cdda_schedule(cd[0], cd[1], cd[2], eo)
    if a.md:
        os.makedirs(os.path.dirname(a.md) or ".", exist_ok=True)
        with open(a.md, "w") as f:
            f.write("# audio oracle (container capture vs port PCM witness)\n\n")
            f.write(f"- original route: {fo}\n- port route: {fp}\n\n")
            f.write("| window | orig onsets | orig active | port onsets | port active | verdict |\n")
            f.write("|---|---|---|---|---|---|\n")
            for label, lo, hi, co, cp, v in rows:
                f.write(f"| {label} ({lo}-{hi} ms) | {co['onsets']} | {co['active_frac']} | "
                        f"{cp['onsets']} | {cp['active_frac']} | {v} |\n")
            if sched:
                f.write("\n```\n" + "\n".join(sched) + "\n```\n")
        print(f"wrote {a.md}")


# ---------------------------------------------------------------- CD-DA schedule
# The game's music is CD-DA on both sides, so "is the music right" is a schedule question, not a
# loudness question: which disc extent plays in which window. The port's witness records every play/stop
# with its index, LBA and length plus the bytes it fed the device, so each witnessed track can be matched
# against the original's captured mix by its energy profile. A track that plays in the port but never
# appears (or appears far away in time) in the original's mix is the music items in docs/manual-playtest
# stated as a measurement.
def load_cdda(port_dir):
    d = port_dir if os.path.isdir(port_dir) else os.path.dirname(port_dir)
    raw, idx = os.path.join(d, "cdda.raw"), os.path.join(d, "cdda.jsonl")
    if not (os.path.exists(raw) and os.path.exists(idx)):
        return None, []
    marks, cursors = [], []
    for line in open(idx):
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if "event" in j:
            marks.append(j)
        else:
            cursors.append(j)
    return raw, marks, cursors


def rms_at(raw, byte_off, n_bytes):
    """RMS over one slice of the s16le stereo CD-DA witness -- the same measure envelope() computes on
    the original's capture, so the two profiles are directly comparable."""
    with open(raw, "rb") as f:
        f.seek(byte_off)
        b = f.read(n_bytes)
    if len(b) < 4:
        return None
    n = len(b) // 2
    v = struct.unpack(f"<{n}h", b[: n * 2])
    return (sum(x * x for x in v) / n) ** 0.5 if v else None


def cdda_schedule(raw, marks, cursors, orig_env, bucket_ms=BUCKET_MS, step_ms=500, win_s=20.0):
    """For every witnessed track: where the port played it, and where in the original's capture the same
    music profile appears (lag = original_time - port_time)."""
    FB = step_ms // bucket_ms                      # fingerprint buckets
    prof = [e[0] for e in orig_env]                # rms per 100 ms
    out = [f"CD-DA schedule ({len(marks)} marks, fingerprint step {step_ms} ms over "
           f"{len(prof)} original buckets)"]
    for m in marks:
        if m["event"] != "play":
            continue
        t0 = m["t_ms"]
        start = m["bytes"]                          # witness byte offset where this track begins
        dur = min(win_s, m["sectors"] / 75.0)
        nb = int(dur * 44100 * 4)                   # s16 stereo bytes
        seg = [rms_at(raw, start + k * 44100 * 4 // 10, 44100 * 4 // 10)
               for k in range(int(dur * 10))]
        seg = [x if x is not None else 0.0 for x in seg]
        if not any(seg):
            out.append(f"  track index {m['index']} lba={m['lba']} t={t0/1000:.1f}s: "
                       f"no samples witnessed")
            continue
        fseg = _norm([sum(seg[i * FB:(i + 1) * FB]) / FB for i in range(len(seg) // FB)])
        fpro = _norm_prof(prof, FB)
        best, blag = -2.0, 0
        # both sides are zero-mean unit-norm, so the dot product IS the Pearson correlation; dividing by
        # the window length used to shrink every score to ~0.02 and made every track read NOT FOUND
        for lag in range(0, max(1, len(fpro) - len(fseg))):
            sc = sum(a * b for a, b in zip(fseg, fpro[lag:lag + len(fseg)]))
            if sc > best:
                best, blag = sc, lag
        pt0 = t0 / 1000.0
        ot0 = blag * step_ms / 1000.0
        verdict = ("MATCH" if best > 0.8 and abs(ot0 - pt0) < 2.0 else
                   f"SHIFTED {ot0 - pt0:+.1f}s" if best > 0.8 else "NOT FOUND in the original mix")
        out.append(f"  track index {m['index']} lba={m['lba']} sectors={m['sectors']}: "
                   f"port {pt0:.1f}s -> original {ot0:.1f}s  corr={best:.2f}  [{verdict}]")
    print("\n" + "\n".join(out))
    return out


def _norm(v):
    m = sum(v) / len(v) if v else 0.0
    w = [x - m for x in v]
    n = sum(x * x for x in w) ** 0.5
    return [x / n for x in w] if n else v


def _norm_prof(prof, fb):
    return _norm([sum(prof[i * fb:(i + 1) * fb]) / fb for i in range(len(prof) // fb)])


if __name__ == "__main__":
    main()
