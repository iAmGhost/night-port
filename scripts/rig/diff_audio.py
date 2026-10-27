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
    ap.add_argument("--port-cells", help="the port's cells.bin, to re-derive the PCM sample rate")
    ap.add_argument("--md")
    ap.add_argument("--missing-onsets", type=int, default=3,
                    help="onset deficit that counts as missing audio in a window")
    a = ap.parse_args()

    so, ro = read_mono(a.orig)
    sp, rp = read_mono(a.port)
    if a.port_cells and os.path.exists(a.port_cells):
        # The port's WAV header carries a nominal board rate; the truth is how long the run lasted, so
        # derive the rate from the record timeline instead of trusting the header (docs/34: the board
        # rate is whatever DSP command 0x40 last programmed).
        rec = struct.Struct("<qQQQII")
        with open(a.port_cells, "rb") as f:
            f.read(16)
            hs = rec.unpack(f.read(rec.size))
            f.seek(-rec.size, os.SEEK_END)
            he = rec.unpack(f.read(rec.size))
        dur = (he[0] - hs[0]) / 1000.0
        if dur > 0.5:
            measured = len(sp) / dur
            if abs(measured - rp) / rp > 0.25:
                print(f"port WAV header says {rp} Hz; the run lasted {dur:.1f} s for {len(sp)} samples "
                      f"-> using {measured:.0f} Hz")
                rp = measured
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
        print(f"wrote {a.md}")


if __name__ == "__main__":
    main()
