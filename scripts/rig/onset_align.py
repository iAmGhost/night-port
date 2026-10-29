#!/usr/bin/env python3
"""Path-robust music presence test: onset-sequence correlation, not sample-level NCC.

Sample-level NCC (audio_ncc.py) needs the capture to carry the disc's *samples*. It reports ~0.1 for the
container capture against any disc track, while the same template scores ~1.0 against the port's own CD-DA
witness (which IS those bytes). That distinguishes two very different situations, and NCC cannot:

  (A) the capture contains the music but the emulator's CD path alters samples (gain stages, resampling,
      per-sector header handling) -> melody is there, timing is measurable;
  (B) the capture contains no CD-DA at all -> the shipped-side audio oracle is SFX-only.

An onset-energy sequence sampled at `--hop-ms` is invariant to gain, sample-format and mild resampling, so a
real recording match shows a SHARP peak, while a mixture of unrelated material stays near the noise floor.
The sharpness statistic (best / mean of the correlation distribution) is what makes the verdict defensible.

  python3 scripts/rig/onset_align.py --wav CAPTURE.wav --img NIGHT/TG_NIGHT.IMG \
      --tracks 1:37262,9:185120,11:223146,2:40415 [--hop-ms 100] [--seg-s 40] [--doff-s 20]
"""
import argparse
import numpy as np


def onset_series(samples, rate, hop):
    """Mean |sample| per hop window, then positive first difference: an onset/energy-derivative train."""
    n = (len(samples) // hop) * hop
    e = np.abs(samples[:n]).reshape(-1, hop).mean(axis=1)
    d = np.diff(e)
    d[d < 0] = 0.0
    d -= d.mean()
    return d


def corr_all(target, tpl):
    """Normalized cross-correlation of tpl against every position of target (FFT)."""
    L, M = len(target), len(tpl)
    if M < 8 or L <= M:
        return np.array([0.0]), 0
    nfft = 1 << (L + M - 1).bit_length()
    xc = np.fft.irfft(np.fft.rfft(target, n=nfft) * np.fft.rfft(tpl[::-1], n=nfft), n=nfft)[M - 1:M - 1 + (L - M + 1)]
    tn = np.sqrt((tpl * tpl).sum())
    cs = np.concatenate([[0.0], np.cumsum(target * target)])
    en = np.sqrt(np.maximum(cs[M:M + len(xc)] - cs[:len(xc)], 1e-9))
    return xc / (en * tn), M


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wav", required=True)
    ap.add_argument("--img", default="NIGHT/TG_NIGHT.IMG")
    ap.add_argument("--tracks", required=True, help="index:lba[,index:lba...]")
    ap.add_argument("--hop-ms", type=int, default=100)
    ap.add_argument("--seg-s", type=int, default=40)
    ap.add_argument("--doff-s", type=int, default=20)
    ap.add_argument("--sector-header", type=int, default=0)
    a = ap.parse_args()

    b = open(a.wav, "rb").read()
    i = b.find(b"fmt ")
    ch = int.from_bytes(b[i + 10:i + 12], "little")
    rate = int.from_bytes(b[i + 12:i + 16], "little")
    j = b.find(b"data")
    data = b[j + 8:] if j >= 0 else b[44:]
    x = np.frombuffer(data[:len(data) // 2 * 2], dtype="<i2").astype(np.float64)
    tgt = x.reshape(-1, ch).mean(axis=1)
    hop = int(rate * a.hop_ms / 1000)
    T = onset_series(tgt, rate, hop)
    print("%s: %.1f s @ %d Hz -> %d onset buckets (%d ms)" % (a.wav, len(tgt) / rate, rate, len(T), a.hop_ms))

    for spec in a.tracks.split(","):
        idx, lba = spec.split(":")
        lba = int(lba)
        f = open(a.img, "rb")
        # one CD-DA sector = 1/75 s, so seconds must be converted to sectors before counting bytes
        f.seek((lba - 150) * 2352 + a.sector_header)
        d = np.frombuffer(f.read(((a.doff_s + a.seg_s) * 75 + 1) * 2352), dtype="<i2").astype(np.float64)
        d = d.reshape(-1, 2).mean(axis=1)
        tpl = onset_series(d[a.doff_s * 44100:(a.doff_s + a.seg_s) * 44100], 44100, hop)
        c, M = corr_all(T, tpl)
        if len(c) < 3:
            continue
        k = int(np.argmax(c))
        mean, sd = float(c.mean()), float(c.std())
        print("  track %-3s lba=%-7d seg %ds: best corr %+.3f at capture t=%6.2f s | mean %+.3f sd %.3f "
              "| sharpness best/mean %+.2f  best/sd %.1f" %
              (idx, lba, a.seg_s, c[k], (k * hop) / rate, mean, sd,
               c[k] / mean if mean else 0.0, (c[k] - mean) / sd if sd else 0.0))


if __name__ == "__main__":
    main()
