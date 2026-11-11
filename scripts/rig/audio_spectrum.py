#!/usr/bin/env python3
"""Spectral fingerprint matching: which disc track is audible, and when, inside a MIXED recording.

The RMS-envelope version (cd_track_timeline.py) works on the port's own CD stream (corr 1.000) but decays to
~0.45 on the container's mixer capture, because that recording is CD-DA plus digitrain SFX plus the
emulator's own mixer gain: a 4 Hz loudness envelope cannot separate material. This does the same question with
per-window narrow-band magnitudes (Goertzel), z-scored per band, which survive a constant gain and bursty SFX.

Why the shipped side needs this at all: the game programs no audio volume whatsoever (the guest contains
exactly one MSCDEX audio request, 0x1510 = play, at fn_010570; searches for 0x1511/set-volume and for SB
mixer-port writes return nothing), so the original's audible CD level is pure emulator mixer gain and cannot
be compared to the port's line-level witness in the time domain. The track *identity timeline*, however, is
comparable -- and that is the music item's actual question (title stop, slideshow track, stage-start change).
"""
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from disc_loc import find_img        # the disc is located by suffix, not by a rip's directory (docs/455)

import argparse, array, math, struct, sys

N = 512            # window (samples)
BINS = [40, 70, 120, 200, 330, 540, 880, 1450, 2400, 4000]   # log-spaced centre frequencies

def goertzel_windows(samples, rate):
    """Return list of windows; each window is BINS narrow-band magnitudes (float list)."""
    fs = [2 * math.cos(2 * math.pi * f / rate) for f in BINS]
    out = []
    n = len(samples)
    for s in range(0, n - N, N):
        feats = []
        w = samples[s:s + N]
        for c in fs:
            s1 = s2 = 0.0
            for v in w:
                s1 = c * s1 - s2 + v
                s2 = s1
            feats.append(math.sqrt(max(0.0, s1 * s1 + s2 * s2 - c * s1 * s2)))
        out.append(feats)
    return out

def load_wav(path):
    b = open(path, 'rb').read()
    i = b.find(b'fmt '); ch = struct.unpack('<H', b[i+10:i+12])[0]; rate = struct.unpack('<I', b[i+12:i+16])[0]
    j = b.find(b'data'); d = b[j+8:] if j >= 0 else b[44:]
    a = array.array('h'); n = len(d) // 2; a.frombytes(bytes(d[:n*2]))
    mono = array.array('f', [0.0] * (n // ch))
    for k in range(len(mono)):
        acc = 0
        for c in range(ch): acc += a[k*ch + c]
        mono[k] = acc / ch
    return mono, rate

def disc_pcm(img, lba, sectors):
    f = open(img, 'rb'); f.seek((lba - 150) * 2352 + 24)
    d = f.read(min(sectors, 300) * 2352)
    a = array.array('h'); a.frombytes(d)
    return array.array('f', [(a[2*k] + a[2*k+1]) / 2.0 for k in range(len(a) // 2)]), 44100

def zscore(mat):
    m = len(mat); k = len(mat[0])
    means = [sum(row[j] for row in mat) / m for j in range(k)]
    sds = [(sum((row[j] - means[j]) ** 2 for row in mat) / m) ** 0.5 or 1.0 for j in range(k)]
    return [[(row[j] - means[j]) / sds[j] for j in range(k)] for row in mat]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wav', required=True)
    ap.add_argument("--img", default=None,
                        help="the raw 2352-byte-frame disc image (default: the one this repo has, docs/455)")
    ap.add_argument('--tracks', required=True, help='index:lba:sectors (DECIMAL lba, as the port logs it)')
    ap.add_argument('--window-s', type=int, default=40)
    ap.add_argument('--top', type=int, default=2)
    a = ap.parse_args()
    if not a.img:                                  # the repo's own disc, found by suffix
        a.img = str(find_img())
        print(f"img: {a.img} (located -- pass --img to name another)", file=sys.stderr)
    mono, rate = load_wav(a.wav)
    tgt = zscore(goertzel_windows(mono, rate))
    print(f'{a.wav}: {len(tgt)} spectral windows ({len(tgt)*N/rate:.0f} s), {rate} Hz, {len(BINS)} bands')
    hop_s = N / rate
    for spec in a.tracks.split(','):
        idx, lba, sectors = spec.split(':')
        dpcm, drate = disc_pcm(a.img, int(lba), int(sectors))
        src = zscore(goertzel_windows(dpcm, drate))
        n = min(len(src), a.window_s * rate // N)
        best = []
        for off in range(0, max(1, len(tgt) - n)):
            sc = 0.0
            for j in range(len(BINS)):
                num = 0.0
                for k in range(n): num += src[k][j] * tgt[off + k][j]
                sc += num / n
            best.append((sc / len(BINS), off))
        best.sort(reverse=True)
        hits = []
        for sc, off in best:
            if any(abs(off - h[1]) < 10 for h in hits): continue
            hits.append((sc, off))
            if len(hits) >= a.top: break
        for sc, off in hits:
            print(f'  track {idx:>3} lba={lba}: audible from {off*hop_s:7.1f} s   spectral corr {sc:+.3f}')

if __name__ == '__main__':
    main()
