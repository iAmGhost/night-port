#!/usr/bin/env python3
"""Event-anchored alignment for the audio oracle (docs/113).

diff_audio.py compares per-window features at nominal wall time, which is exactly the mistake docs/101
made for DS state: the two sides do not reach the same game moment at the same second, so a correct
schedule shows up as a low correlation ("NOT FOUND in the original mix"). This script searches the
time offset that maximises the correlation of the two envelope signals, and reports the residual
correlation at zero offset and at the best offset -- i.e. it separates "the wrong audio" from "the
right audio at a shifted time", which is the distinction the music cluster needs.
"""
import argparse, array, struct, sys

def envelope(path, step_ms=250):
    """RMS per step_ms bucket, from a PCM WAV (mono or stereo, 16-bit)."""
    b = open(path, 'rb').read()
    i = b.find(b'fmt ')
    ch, rate = 1, 44100
    if i >= 0:
        fmt = b[i+8:i+26]
        ch = struct.unpack('<H', fmt[2:4])[0]; rate = struct.unpack('<I', fmt[4:8])[0]
    j = b.find(b'data')
    off = j + 8 if j >= 0 else 44
    data = memoryview(b)[off:]
    n = (len(data) // 2) // ch
    step = max(1, rate * step_ms // 1000)
    out = array.array('f')
    src = array.array('h'); src.frombytes(bytes(data[:n*ch*2]))
    for k in range(0, n, step):
        acc = 0.0; cnt = 0
        for t in range(k*ch, min((k+step)*ch, n*ch), 4):   # sample every 4th value: fast, faithful enough
            v = src[t]; acc += v*v; cnt += 1
        out.append((acc/cnt) ** 0.5 if cnt else 0.0)
    return out, step_ms, rate

def norm(a):
    m = max(a) or 1.0
    return [x/m for x in a]

def corr(a, b, lag):
    """Pearson-free dot product of mean-removed envelopes, lag = b shifted by lag buckets."""
    n0, n1 = len(a), len(b)
    lo, hi = max(0, -lag), min(n0, n1 - lag)
    if hi - lo < 20: return 0.0, 0
    ma = sum(a[lo:hi])/(hi-lo); mb = sum(b[lo+lag:hi+lag])/(hi-lo)
    sab = saa = sbb = 0.0
    for k in range(lo, hi):
        x = a[k]-ma; y = b[k+lag]-mb
        sab += x*y; saa += x*x; sbb += y*y
    if saa <= 0 or sbb <= 0: return 0.0, hi-lo
    return sab/((saa*sbb) ** 0.5), hi-lo

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--orig', required=True, help='container mixer WAV')
    ap.add_argument('--port', required=True, help='port WAV (pcm.wav or wrapped cdda.wav)')
    ap.add_argument('--step-ms', type=int, default=250)
    ap.add_argument('--max-shift-s', type=float, default=60.0)
    a = ap.parse_args()
    eo, st, ro = envelope(a.orig, a.step_ms)
    ep, st2, rp = envelope(a.port, a.step_ms)
    print(f'orig {len(eo)} buckets @{ro} Hz, port {len(ep)} buckets @{rp} Hz, bucket {st} ms')
    xo, xp = norm(eo), norm(ep)
    mx = int(a.max_shift_s * 1000 / st)
    best = max(range(-mx, mx+1), key=lambda L: corr(xo, xp, L)[0])
    c0, n0 = corr(xo, xp, 0)
    cb, nb = corr(xo, xp, best)
    print(f'corr at zero shift      : {c0:+.3f}   ({n0} overlapping buckets)')
    print(f'best shift              : {best*st/1000:+.1f}s   corr {cb:+.3f}  ({nb} buckets)')
    print(f'activity: orig {(sum(1 for x in eo if x > 50))/len(eo):.2f}  port {(sum(1 for x in ep if x > 50))/len(ep):.2f}')
    if cb > 0.6:
        print('VERDICT: the same audio material, shifted -- a schedule/phase defect, not a content defect')
    elif cb < 0.25:
        print('VERDICT: no offset makes the envelopes agree -- different material or a stream that is not comparable')
    else:
        print('VERDICT: partial agreement -- compare per-window feature reports around the best shift')

if __name__ == '__main__':
    main()
