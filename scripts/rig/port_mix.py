#!/usr/bin/env python3
"""Music-only mix for the port side, then time-shift search against the shipped mix.

The port's PCM witness (`pcm.wav`) carries only the Sound Blaster board DMA -- the CD-DA music goes to
`cdda.raw` (docs/114 proved those bytes are faithful to the disc), so comparing `pcm.wav` to the container's
mixer capture makes the port look silent (21.7 s of board audio over a 300 s route while the shipped mix is
active in 216/275 windows). This script builds the port's *music* timeline from `cdda.raw` + `cdda.jsonl` and
answers the question the music item actually needs: at what time offset, if any, does the port's music line up
with the shipped mix.

  python3 scripts/rig/port_mix.py --cdda-raw D/cdda.raw --cdda-jsonl D/cdda.jsonl \
          --orig orig/audio.wav [--bucket-ms 500] [--max-lag-s 25] [--md out.md]

CD-DA sector = 2352 B = 588 stereo 16-bit frames = 1/75 s, so a play mark at t0 with S sectors is a signal
that runs from t0 for S/75 s.
"""
import argparse
import array
import json
import math
import struct
import wave


def rms_env(samples, rate, bucket_ms):
    """RMS per bucket over interleaved stereo s16 (mean of channel energies)."""
    per = int(rate * bucket_ms / 1000)
    out = []
    n = len(samples) // 2
    for b in range(0, n, per):
        seg = samples[b * 2:(b + per) * 2]
        if not seg:
            out.append(0.0)
            continue
        acc = 0
        for v in seg[0::2]:
            acc += v * v
        out.append(math.sqrt(acc / max(1, len(seg[0::2]))))
    return out


def wav_env(path, bucket_ms):
    w = wave.open(path)
    rate = w.getframerate()
    n = w.getnframes()
    ch = w.getnchannels()
    sw = w.getsampwidth()
    raw = w.readframes(n)
    if sw == 1:
        vals = [v - 128 for v in raw]
    else:
        vals = list(struct.unpack("<%dh" % (len(raw) // 2), raw))
    if ch == 2:
        vals = vals[0::2]
    out = []
    per = int(rate * bucket_ms / 1000)
    for b in range(0, len(vals), per):
        seg = vals[b:b + per]
        out.append(math.sqrt(sum(v * v for v in seg) / max(1, len(seg))) if seg else 0.0)
    w.close()
    return out, rate, n / rate


def norm_corr(a, b, max_lag):
    """Best (lag_buckets, corr) aligning b onto a; lag>0 means b must be advanced by lag buckets."""
    def unit(x):
        m = math.sqrt(sum(v * v for v in x))
        return [v / m for v in x] if m else x
    best = (0, 0.0)
    for lag in range(-max_lag, max_lag + 1):
        num = 0.0
        den_a = den_b = 0.0
        for i, av in enumerate(a):
            j = i - lag
            if 0 <= j < len(b):
                num += av * b[j]
                den_a += av * av
                den_b += b[j] * b[j]
        if den_a and den_b:
            c = num / math.sqrt(den_a * den_b)
            if c > best[1]:
                best = (lag, c)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cdda-raw", required=True)
    ap.add_argument("--cdda-jsonl", required=True)
    ap.add_argument("--orig", required=True)
    ap.add_argument("--bucket-ms", type=int, default=500)
    ap.add_argument("--max-lag-s", type=int, default=25)
    ap.add_argument("--md")
    a = ap.parse_args()

    marks = [json.loads(l) for l in open(a.cdda_jsonl) if l.strip()]
    plays = [m for m in marks if m.get("event") == "play"]
    raw = open(a.cdda_raw, "rb").read()
    sectors_total = len(raw) // 2352
    print("cdda.raw: %d B = %d sectors = %.1f s of CD-DA; %d play marks" %
          (len(raw), sectors_total, sectors_total / 75.0, len(plays)))

    rate = 44100
    # one zero-filled timeline long enough for the whole route
    last_ms = max([m.get("t_ms", 0) for m in marks] + [0])
    dur_s = last_ms / 1000.0 + 5.0
    mix = array.array("h", bytes(int(dur_s * rate) * 2 * 2))
    placed = 0
    for p in plays:
        t0 = p["t_ms"] / 1000.0
        # reads are sequential in cdda.raw; a play consumes min(sectors, remaining) from the current cursor
        take = min(int(p.get("sectors", 0)), sectors_total - placed)
        for k in range(max(0, take)):
            src = (placed + k) * 2352
            dst = int((t0 + k / 75.0) * rate) * 2      # stereo frames -> sample index
            if dst + 588 * 2 > len(mix):
                break
            mix[dst:dst + 588 * 2] = array.array("h", raw[src:src + 2352])
        placed += take
        print("  play t=%.2fs index=%s lba=%d sectors=%d -> placed %d sectors (%.1f s)" %
              (t0, p.get("index"), p.get("lba", 0), p.get("sectors", 0), take, take / 75.0))
    env, orate, odur = wav_env(a.orig, a.bucket_ms)
    penv = rms_env(mix, rate, a.bucket_ms)
    print("shipped mix: %.1f s, %d buckets, active %d" % (odur, len(env), sum(1 for v in env if v > 40)))
    print("port  music: %.1f s, %d buckets, active %d" %
          (dur_s, len(penv), sum(1 for v in penv if v > 40)))
    lag, c = norm_corr(env, penv, int(a.max_lag_s * 1000 / a.bucket_ms))
    print("best alignment: lag %+d buckets (%+.1f s) corr=%.3f" %
          (lag, lag * a.bucket_ms / 1000.0, c))
    if a.md:
        with open(a.md, "w") as f:
            f.write("# Port music mix vs shipped mix (buckets %d ms)\n\n" % a.bucket_ms)
            f.write("- port music buckets active: %d of %d\n" %
                    (sum(1 for v in penv if v > 40), len(penv)))
            f.write("- shipped buckets active: %d of %d\n" % (sum(1 for v in env if v > 40), len(env)))
            f.write("- best alignment lag %+.1f s, corr %.3f\n" % (lag * a.bucket_ms / 1000.0, c))


if __name__ == "__main__":
    main()
