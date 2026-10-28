#!/usr/bin/env python3
"""Does the container's mixer capture contain the disc's CD-DA at all, and at what lag? (numpy, run under uv)

The envelope (docs/113) and spectral (audio_spectrum.py) fingerprints both decay to ~0.3 on the container
capture while scoring 0.85-1.59 on the port's own CD stream. That is either (i) the fingerprint is too weak
against mixed audio, or (ii) the capture does not carry CD-DA. This settles it the only exact way: normalised
cross-correlation at the SAMPLE level between a disc-track segment and the capture. CD-DA through DOSBox's
mixer is a constant-gain copy of the disc samples, so a real match gives NCC ~1 regardless of gain; SFX on
top only lowers it slightly. If the capture has no CD-DA, NCC stays at noise level.
"""
import argparse, array, struct, sys
import numpy as np

def load_wav(path):
    b = open(path, 'rb').read()
    i = b.find(b'fmt '); ch = struct.unpack('<H', b[i+10:i+12])[0]; rate = struct.unpack('<I', b[i+12:i+16])[0]
    j = b.find(b'data'); d = b[j+8:] if j >= 0 else b[44:]
    a = np.frombuffer(d[:len(d)//2*2], dtype='<i2').astype(np.float64)
    return a.reshape(-1, ch).mean(axis=1), rate

def disc(path, lba, sectors):
    f = open(path, 'rb'); f.seek((lba - 150) * 2352 + 24)
    d = f.read(sectors * 2352)
    a = np.frombuffer(d, dtype='<i2').astype(np.float64)
    return a.reshape(-1, 2).mean(axis=1), 44100

def ncc_peak(tgt, tpl, lo, hi):
    """Best NCC of tpl inside tgt[lo:hi], in samples; local-energy normalised."""
    L, M = len(tgt), len(tpl)
    seg = tgt[max(0, lo):min(L, hi + M)]
    if len(seg) <= M: return -1.0, 0
    nfft = 1 << (len(seg) + M - 1).bit_length()
    fa = np.fft.rfft(seg, n=nfft)
    fb = np.fft.rfft(tpl[::-1], n=nfft)
    xc = np.fft.irfft(fa * fb)[:len(seg)][M-1:M-1 + (len(seg) - M + 1)]
    cs = np.concatenate([[0.0], np.cumsum(seg * seg)])
    e = np.sqrt(np.maximum(cs[M:M + len(xc)] - cs[:len(xc)], 1e-9))
    tn = np.sqrt((tpl * tpl).sum())
    ncc = xc / (e * tn)
    k = int(np.argmax(ncc))
    return float(ncc[k]), k

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wav', required=True)
    ap.add_argument('--img', default='NIGHT/TG_NIGHT.IMG')
    ap.add_argument('--tracks', required=True, help='index:lba:sectors:disc_offset_s[:expected_capture_s]')
    ap.add_argument('--seg-s', type=float, default=4.0)
    ap.add_argument('--search-s', type=float, default=45.0)
    a = ap.parse_args()
    tgt, rate = load_wav(a.wav)
    print(f'{a.wav}: {len(tgt)/rate:.0f} s @ {rate} Hz')
    for spec in a.tracks.split(','):
        p = spec.split(':'); idx, lba, sectors = p[0], int(p[1]), int(p[2])
        doff = int(p[3]) if len(p) > 3 else 30
        exp = int(p[4]) if len(p) > 4 else None
        # one CD sector = 2352 B = 588 stereo frames = 1/75 s, so seconds must be converted to sectors
        d, _ = disc(a.img, lba, (doff + int(a.seg_s)) * 75 + 1)
        tpl = d[doff*rate:(doff+int(a.seg_s))*rate]
        if exp is None:
            lo, hi = 0, max(0, len(tgt) - len(tpl) - 1)
        else:
            c = int(exp * rate)
            lo, hi = max(0, c - int(a.search_s*rate)), min(len(tgt)-len(tpl)-1, c + int(a.search_s*rate))
        v, k = ncc_peak(tgt, tpl, lo, hi)
        print(f'  track {idx:>3} lba={lba} disc+{doff}s ({a.seg_s:.0f}s seg): best NCC {v:+.3f} at capture t={(lo+k)/rate:7.2f} s')

if __name__ == '__main__':
    main()
