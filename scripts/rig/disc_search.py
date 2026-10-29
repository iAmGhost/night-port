#!/usr/bin/env python3
"""Search the whole disc image for a waveform that appears in a capture (reverse-direction NCC).

The forward direction (disc template -> capture) is what audio_ncc.py does, and it assumes the emulator's
CD-DA output starts at the same disc position the game asked for. If the emulator resolves the requested
LBA through a different convention (PREGAP/INDEX handling, track-relative vs absolute), the capture still
contains genuine disc audio but no forward template hits, which is indistinguishable from "no CD-DA at all".

This inverts the search: take an excerpt of the capture and find its best match anywhere in the disc image's
audio region. The hit position names the LBA convention (or shows there is no disc-derived audio at all).

  python3 scripts/rig/disc_search.py --wav CAPTURE.wav --at 20 --len 6 [--img NIGHT/TG_NIGHT.IMG]
"""
import argparse
import numpy as np


def wav_mono(path):
    b = open(path, "rb").read()
    i = b.find(b"fmt ")
    ch = int.from_bytes(b[i + 10:i + 12], "little")
    rate = int.from_bytes(b[i + 12:i + 16], "little")
    j = b.find(b"data")
    d = b[j + 8:] if j >= 0 else b[44:]
    x = np.frombuffer(d[:len(d) // 2 * 2], dtype="<i2").astype(np.float64)
    return x.reshape(-1, ch).mean(axis=1), rate


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wav", required=True)
    ap.add_argument("--at", type=float, default=20.0, help="excerpt start (s)")
    ap.add_argument("--len", dest="length", type=float, default=6.0)
    ap.add_argument("--img", default="NIGHT/TG_NIGHT.IMG")
    ap.add_argument("--from-sector", type=int, default=0)
    ap.add_argument("--to-sector", type=int, default=0, help="0 = end of image")
    ap.add_argument("--chunk", type=int, default=1 << 22)
    a = ap.parse_args()

    tgt, rate = wav_mono(a.wav)
    tpl = tgt[int(a.at * rate):int((a.at + a.length) * rate)]
    tpl -= tpl.mean()
    tn = np.sqrt((tpl * tpl).sum())
    print("template: %.1f s of the capture at t=%.1f s (capture rate %d)" % (a.length, a.at, rate))

    sectors_total = 633400656 // 2352
    lo, hi = a.from_sector, (a.to_sector or sectors_total)
    f = open(a.img, "rb")
    overlap = len(tpl)
    best = (-2.0, 0)
    nfft = 1 << (a.chunk + overlap).bit_length()
    ft = np.fft.rfft(tpl[::-1], n=nfft)
    pos = lo
    f.seek(pos * 2352)
    while pos < hi:
        raw = f.read(a.chunk * 2352)
        if not raw:
            break
        x = np.frombuffer(raw[:len(raw) // 2 * 2], dtype="<i2").astype(np.float64)
        x = x.reshape(-1, 2).mean(axis=1)
        x -= x.mean()
        nfft = 1 << (len(x) + overlap).bit_length()
        ft = np.fft.rfft(tpl[::-1], n=nfft)
        xc = np.fft.irfft(np.fft.rfft(x, n=nfft) * ft, n=nfft)[overlap - 1:overlap - 1 + (len(x) - overlap + 1)]
        cs = np.concatenate([[0.0], np.cumsum(x * x)])
        en = np.sqrt(np.maximum(cs[overlap:overlap + len(xc)] - cs[:len(xc)], 1e-9))
        ncc = xc / (en * tn)
        k = int(np.argmax(ncc))
        if ncc[k] > best[0]:
            best = (float(ncc[k]), pos * 44100 // 75 + k)     # CD-DA at 44100 Hz: 1 sector = 588 samples
        pos += len(raw) // 2352
        print("  scanned to sector %d (%.0f%%) best so far %+.3f" % (pos, 100.0 * pos / hi, best[0]), flush=True)
    sec = best[1] / 44100 * 75
    print("best NCC %+.3f at disc sector %.1f (%.2f min) i.e. LBA(absolute incl 150) ~%.0f" %
          (best[0], sec, sec / 75 / 60, sec + 150))


if __name__ == "__main__":
    main()
