#!/usr/bin/env python3
"""Melody-set fingerprint: presence of a disc track inside a mixed capture, immune to timbre and gain.

Sample-level NCC (audio_ncc.py) needs the capture to hold the disc's samples; the container's capture does not
(docs/130: DOSBox's CD path alters samples though the tempo is exact, corr 0.649 at scale 1.000). An onset
train (onset_align.py, music_presence.py) is tempo-robust but not selective enough -- with 12-28 s templates
its mismatched-pair controls score 0.00-0.59, the same band as the hypotheses.

This fingerprint keeps only what survives a mixer: the set of salient spectral peaks per short window, binned
to semitones. A melody is a sequence of such sets, so presence becomes "the template's peak-set sequence
overlaps the capture's sequence with high mean IoU at one lag", which is invariant to gain, EQ and synth
timbre, and fails for unrelated material because peak sets of different music rarely coincide in semitone bins.

Usage:
  python3 scripts/rig/music_melody.py --capture CAPTURE.wav --template-wav PORT/cdda.wav --tpl-at 68.9 \
      [--len 24] [--hop 0.2] [--win 1.0] [--peaks 6]
  python3 scripts/rig/music_melody.py --capture CAPTURE.wav --img TG_NIGHT.IMG --tpl-lba 223146 --tpl-at 20
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from disc_loc import find_img        # the disc is located by suffix, not by a rip's directory (docs/455)

import argparse
import numpy as np


def mono(path):
    b = open(path, "rb").read()
    i = b.find(b"fmt ")
    ch = int.from_bytes(b[i + 10:i + 12], "little")
    rate = int.from_bytes(b[i + 12:i + 16], "little")
    j = b.find(b"data")
    d = b[j + 8:] if j >= 0 else b[44:]
    x = np.frombuffer(d[:len(d) // 2 * 2], dtype="<i2").astype(np.float64)
    return x.reshape(-1, ch).mean(axis=1), rate


def peak_sets(x, rate, hop_s, win_s, npeaks, lo=80.0, hi=4200.0):
    """Semitone-binned salient spectral peaks per window -> a list of frozensets."""
    win, hop = int(rate * win_s), int(rate * hop_s)
    w = np.hanning(win)
    fr = np.fft.rfftfreq(win, 1.0 / rate)
    band = (fr >= lo) & (fr <= hi)
    fb, ib = fr[band], np.bincount  # noqa: F841
    out = []
    for s in range(0, max(0, len(x) - win), hop):
        seg = x[s:s + win]
        seg = seg - seg.mean()
        if np.sqrt((seg * seg).mean()) < 60:          # silence: no melody to speak of
            out.append(None)
            continue
        m = np.abs(np.fft.rfft(seg * w))[band]
        # local maxima of the magnitude spectrum, strongest npeaks
        idx = np.where((m[1:-1] > m[:-2]) & (m[1:-1] >= m[2:]))[0] + 1
        if len(idx) == 0:
            out.append(frozenset())
            continue
        idx = idx[np.argsort(m[idx])[::-1]][:npeaks]
        # relative strength matters: drop peaks far below the strongest
        idx = [k for k in idx if m[k] > 0.25 * m.max()]
        out.append(frozenset(int(round(12 * np.log2(max(fr[k], 40.0) / 440.0))) for k in idx))
    return out


def iou(a, b):
    if not a or not b:
        return 0.0
    return len(a & b) / float(len(a | b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--capture", required=True)
    ap.add_argument("--template-wav")
    ap.add_argument("--img", default=None,
                        help="the raw 2352-byte-frame disc image (default: the one this repo has, docs/455)")
    ap.add_argument("--tpl-lba", type=int)
    ap.add_argument("--tpl-at", type=float, default=20.0, help="seconds into the template source")
    ap.add_argument("--len", dest="tlen", type=float, default=24.0)
    ap.add_argument("--hop", type=float, default=0.2)
    ap.add_argument("--win", type=float, default=1.0)
    ap.add_argument("--peaks", type=int, default=6)
    a = ap.parse_args()
    if not a.img:                                  # the repo's own disc, found by suffix
        a.img = str(find_img())
        print(f"img: {a.img} (located -- pass --img to name another)", file=sys.stderr)

    cap, crate = mono(a.capture)
    A = peak_sets(cap, crate, a.hop, a.win, a.peaks)
    print("capture %.1f s @ %d Hz -> %d melody windows" % (len(cap) / crate, crate, len(A)))

    if a.template_wav:
        tpl_src, trate = mono(a.template_wav)
        label = "%s @%.1fs" % (a.template_wav, a.tpl_at)
    else:
        f = open(a.img, "rb")
        f.seek((a.tpl_lba - 150) * 2352)
        raw = f.read(int((a.tpl_at + a.tlen + 2) * 44100) * 4)
        d = np.frombuffer(raw[:len(raw) // 4 * 4], dtype="<i2").astype(np.float64)
        tpl_src, trate = d.reshape(-1, 2).mean(axis=1), 44100
        label = "disc lba %d +%d s" % (a.tpl_lba, int(a.tpl_at))
    seg = tpl_src[int(a.tpl_at * trate):int((a.tpl_at + a.tlen) * trate)]
    B = peak_sets(seg, trate, a.hop, a.win, a.peaks)
    print("template %s -> %d melody windows" % (label, len(B)))

    M = len(B)
    if M < 5 or len(A) <= M:
        return
    # mean IoU at every lag (coarse-to-fine is unnecessary at 0.2 s hops)
    scores = np.zeros(len(A) - M + 1)
    for lag in range(len(scores)):
        acc = 0.0
        for i in range(M):
            x, y = A[lag + i], B[i]
            if x is None or y is None or not x or not y:
                continue
            acc += iou(x, y)
        scores[lag] = acc / M
    k = int(np.argmax(scores))
    med, sd = float(np.median(scores)), float(np.std(scores))
    print("best mean peak-set IoU %+.3f at capture t=%.1f s | median %+.3f sd %.3f | best/sd %.1f" %
          (scores[k], k * a.hop, med, sd, (scores[k] - med) / sd if sd else 0.0))
    top = np.argsort(scores)[::-1][:6]
    print("top lags: " + ", ".join("%.1fs:%.2f" % (t * a.hop, scores[t]) for t in top))


if __name__ == "__main__":
    main()
