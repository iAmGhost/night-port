#!/usr/bin/env python3
"""Recover WHICH disc track is audible at WHICH time in an audio capture, without any debugger.

The port announces every CD-DA play request (`cdda: track index N lba=... sectors=...`, LBA in DECIMAL), and
the byte probe in docs/113 proves the port then reads those exact sectors bit-for-bit. So the port side of the
music question is known. The shipped side has no such log and no breakpoints -- but the disc itself is the
reference: each track's PCM is in TG_NIGHT.IMG, so its amplitude envelope can be cross-correlated against a
recording to find where that track is audible. Running this on the container's mixer capture gives the
original's track timeline, and running it on the port's own cdda stream is the control that validates the
method (it must reproduce the announced play times).

Envelopes (RMS per bucket, gain-normalized) are used rather than sample correlation because the mixer capture
is music+SFX at the game's own volume while the disc is line level.
"""
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from disc_loc import find_img        # the disc is located by suffix, not by a rip's directory (docs/455)

import argparse, array, os, struct, sys

STEP = 250          # ms per envelope bucket

def wav_pcm(path):
    b = open(path, 'rb').read()
    i = b.find(b'fmt '); ch = 2; rate = 44100
    if i >= 0:
        ch = struct.unpack('<H', b[i+10:i+12])[0]; rate = struct.unpack('<I', b[i+12:i+16])[0]
    j = b.find(b'data')
    return (b[j+8:] if j >= 0 else b[44:]), ch, rate

def env_from_pcm(pcm, ch):
    n = len(pcm) // 2
    src = array.array('h'); src.frombytes(bytes(pcm[:n*2]))
    return src

def env_of(src, frames_per_bucket, ch, start=0, count=None):
    """RMS envelope; stride 2 over samples for speed."""
    n = len(src) // ch
    end = n if count is None else min(n, start + count * frames_per_bucket)
    out = array.array('f')
    for k in range(start * ch, end * ch, frames_per_bucket * ch):
        hi = min(k + frames_per_bucket * ch, len(src))
        acc = 0.0; cnt = 0
        for t in range(k, hi, 2):
            v = src[t]; acc += v * v; cnt += 1
        out.append((acc / cnt) ** 0.5 if cnt else 0.0)
    return out

def img_audio(img, lba, sectors):
    """Concatenated user data of `sectors` audio sectors, as 16-bit signed frames (stereo)."""
    f = open(img, 'rb'); f.seek((lba - 150) * 2352 + 24)
    return f.read(min(sectors, 600) * 2352)

def corr(a, b, off, n):
    lo = off; hi = off + n
    if lo < 0 or hi > len(b): return -2.0
    ma = 0.0; mb = 0.0
    for k in range(n): ma += a[k]; mb += b[lo+k]
    ma /= n; mb /= n
    sab = saa = sbb = 0.0
    for k in range(n):
        x = a[k] - ma; y = b[lo+k] - mb
        sab += x*y; saa += x*x; sbb += y*y
    if saa <= 0 or sbb <= 0: return -2.0
    return sab / ((saa*sbb) ** 0.5)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wav', required=True, help='recording to analyse (container mixer capture, or the port stream)')
    ap.add_argument("--img", default=None,
                        help="the raw 2352-byte-frame disc image (default: the one this repo has, docs/455)")
    ap.add_argument('--tracks', required=True, help='index:lba:sectors, comma separated (LBAs in DECIMAL, as the port logs them)')
    ap.add_argument('--window-s', type=int, default=90, help='seconds of each track to correlate')
    ap.add_argument('--top', type=int, default=3)
    a = ap.parse_args()
    if not a.img:                                  # the repo's own disc, found by suffix
        a.img = str(find_img())
        print(f"img: {a.img} (located -- pass --img to name another)", file=sys.stderr)

    pcm, ch, rate = wav_pcm(a.wav)
    fpb = rate * STEP // 1000
    tgt = env_of(env_from_pcm(pcm, ch), fpb, ch)
    print(f'recording {a.wav}: {len(tgt)} buckets @{STEP} ms ({len(tgt)*STEP/1000:.0f} s), {rate} Hz {ch} ch')
    for spec in a.tracks.split(','):
        idx, lba, sectors = spec.split(':'); lba = int(lba); sectors = int(sectors)
        raw = img_audio(a.img, lba, sectors)
        trk = env_of(env_from_pcm(raw, 2), fpb, 2)
        n = min(len(trk), a.window_s * 1000 // STEP)
        n = max(20, n)
        scored = []
        for off in range(0, max(1, len(tgt) - n)):
            c = corr(trk, tgt, off, n)
            if c > 0.4: scored.append((c, off))
        scored.sort(reverse=True)
        hits = []
        for c, off in scored:
            if any(abs(off - h[1]) < 8 for h in hits): continue
            hits.append((c, off))
            if len(hits) >= a.top: break
        if not hits:
            print(f'  track {idx:>3} lba={lba}: no envelope match above 0.4')
        for c, off in hits:
            print(f'  track {idx:>3} lba={lba}: audible at {off*STEP/1000:7.1f} s   corr {c:+.3f}')

if __name__ == '__main__':
    main()
