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


def segments(marks):
    """Rebuild the real read cursor into cdda.raw.

    `cdda.jsonl` marks are: play/stop events, plus `bytes` rows holding the **cumulative** number of CD-DA
    bytes read so far. A play resets the port's sector position, so a track's own bytes are the raw range
    between its play mark and the following stop/play -- not one global sequential stream (docs/129 §2
    caveat). Returns one dict per play: t0, index, lba, requested, cursor range, sectors actually read.
    """
    segs, cursor, open_seg = [], 0, None
    for m in marks:
        ev = m.get("event")
        if ev == "play":
            if open_seg is not None:
                open_seg["end"] = cursor
            open_seg = {"t0": m["t_ms"] / 1000.0, "index": m.get("index"), "lba": m.get("lba", 0),
                        "requested": int(m.get("sectors", 0)), "start": cursor, "end": None,
                        "t_end": None}
            segs.append(open_seg)
        elif ev == "stop":
            if open_seg is not None:
                open_seg["end"] = cursor
                open_seg["t_end"] = m["t_ms"] / 1000.0
                open_seg = None
        elif "bytes" in m and ev is None:
            cursor = int(m["bytes"])
    if open_seg is not None:
        open_seg["end"] = cursor
    for s in segs:
        s["read"] = max(0, (s["end"] or cursor) - s["start"]) // 2352
    return segs


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
    segs = segments(marks)
    raw = open(a.cdda_raw, "rb").read()
    print("cdda.raw: %d B = %.1f s of CD-DA; %d play marks" % (len(raw), len(raw) / 2352 / 75.0, len(segs)))

    rate = 44100
    last_ms = max([m.get("t_ms", 0) for m in marks] + [0])
    dur_s = last_ms / 1000.0 + 5.0
    mix = array.array("h", bytes(int(dur_s * rate) * 2 * 2))
    for s in segs:
        take = s["read"]
        for k in range(take):
            src = s["start"] + k * 2352
            dst = int((s["t0"] + k / 75.0) * rate) * 2      # stereo frames -> sample index
            if dst + 588 * 2 > len(mix):
                break
            mix[dst:dst + 588 * 2] = array.array("h", raw[src:src + 2352])
        print("  play t=%7.2fs idx=%-3s lba=%-7d req=%-6d read=%-6d (%.1f s of content, cursor %d)" %
              (s["t0"], s["index"], s["lba"], s["requested"], take, take / 75.0, s["start"]))

    env, orate, odur = wav_env(a.orig, a.bucket_ms)
    penv = rms_env(mix, rate, a.bucket_ms)
    print("shipped mix: %.1f s, %d buckets, active %d" % (odur, len(env), sum(1 for v in env if v > 40)))
    print("port  music: %.1f s, %d buckets, active %d" %
          (dur_s, len(penv), sum(1 for v in penv if v > 40)))
    max_lag = int(a.max_lag_s * 1000 / a.bucket_ms)
    lag, c = norm_corr(env, penv, max_lag)
    print("global alignment: lag %+d buckets (%+.1f s) corr=%.3f" %
          (lag, lag * a.bucket_ms / 1000.0, c))

    # Per-play local alignment: does each track the port played actually appear in the shipped mix, and at
    # what offset? A track with low corr at every lag is absent from the shipped mix (or the port's content
    # for it is wrong); a large lag means the same content starts at a different moment.
    rows = []
    for s in segs:
        if s["read"] < 75:
            continue
        seg = array.array("h", raw[s["start"]:s["start"] + s["read"] * 2352])
        senv = rms_env(seg, rate, a.bucket_ms)
        sl, sc = norm_corr(env, senv, max_lag)
        # corr of that track against the shipped mix at zero lag, and how many buckets are active on BOTH
        # sides once aligned (lag>0 means the track must be advanced, i.e. shipped bucket = i + sl)
        z = norm_corr(env, senv, 0)[1]
        cover = sum(1 for i in range(len(senv))
                    if 0 <= i + sl < len(env) and env[i + sl] > 40 and senv[i] > 40)
        rows.append((s, sl, sc, z, cover))
        print("  track idx=%-3s lba=%-7d: best lag %+6.1f s corr=%.3f | corr@0lag=%.3f | "
              "buckets active on both sides while aligned: %d/%d" %
              (s["index"], s["lba"], sl * a.bucket_ms / 1000.0, sc, z, cover, sum(1 for v in senv if v > 40)))
    if a.md:
        with open(a.md, "w") as f:
            f.write("# Port music mix vs shipped mix (buckets %d ms)\n\n" % a.bucket_ms)
            f.write("- port music buckets active: %d of %d\n" %
                    (sum(1 for v in penv if v > 40), len(penv)))
            f.write("- shipped buckets active: %d of %d\n" % (sum(1 for v in env if v > 40), len(env)))
            f.write("- global alignment lag %+.1f s, corr %.3f\n" % (lag * a.bucket_ms / 1000.0, c))
            f.write("\n| track idx | lba | port start | requested | read (s) | best lag | corr | corr@0 |\n")
            f.write("|---|---|---|---|---|---|---|---|\n")
            for s, sl, sc, z, _cov in rows:
                f.write("| %s | %d | %.2f s | %d | %.1f | %+.1f s | %.3f | %.3f |\n" %
                        (s["index"], s["lba"], s["t0"], s["requested"], s["read"] / 75.0,
                         sl * a.bucket_ms / 1000.0, sc, z))


if __name__ == "__main__":
    main()
