#!/usr/bin/env python3
"""Per-event music presence: is the track the game asked for actually audible in the shipped capture?

docs/106 and docs/121 §6 proved the CD *schedule* (which lba, which track length, teardown) is equivalent on
both sides, and docs/114 proved the port's CD-DA bytes are faithful to the disc. What no oracle could confirm
was the audible consequence, because the container's capture was unusable for music (mixer rate 22050 written
into a 44100 header -> 54.7% duplicated samples; fixed in bringup.sh, docs/130).

This is a hypothesis test rather than a blind search: for each play event the port issued on the same drive,
take that event's own audio from the port's CD witness (`cdda.wav`, which is byte-faithful to the disc) as a
template, and look for it in the shipped capture inside a +/- window around the expected moment. A mismatched
(control) pairing is printed next to it so the threshold is measured, not guessed.

  python3 scripts/rig/music_presence.py --capture ORIG/audio.wav --port-stream PORT/cdda.wav \
      --events "idx:port_play_s,idx:port_play_s,..." [--win 25] [--len 12] [--hop-ms 100]
"""
import argparse
import sys

import numpy as np

sys.path.insert(0, __file__.rsplit("/", 1)[0])
from onset_align import corr_all, onset_series


def mono(path):
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
    ap.add_argument("--capture", required=True, help="shipped-side mixer capture wav")
    ap.add_argument("--port-stream", required=True, help="port cdda.wav (byte-faithful disc audio)")
    ap.add_argument("--events", required=True,
                    help="idx:stream_pos_s[:expected_capture_s][,...] -- stream_pos_s is where that play begins "
                         "inside cdda.wav (the witness is one sequential stream), expected_capture_s is where the "
                         "shipped capture should contain it")
    ap.add_argument("--win", type=float, default=25.0)
    ap.add_argument("--len", dest="tlen", type=float, default=12.0)
    ap.add_argument("--hop-ms", type=int, default=100)
    ap.add_argument("--tshift", type=float, default=0.0, help="shipped capture time = port time + tshift")
    a = ap.parse_args()

    cap, crate = mono(a.capture)
    T = onset_series(cap, crate, int(crate * a.hop_ms / 1000))
    print("capture %.1f s @ %d Hz | %d onset buckets | consecutive-equal %.3f" %
          (len(cap) / crate, crate, len(T), float((np.diff(cap.astype(np.int64)) == 0).mean())))
    st, srate = mono(a.port_stream)
    hop = int(crate * a.hop_ms / 1000)

    evs = []
    for spec in a.events.split(","):
        p = spec.split(":")
        idx, t = p[0], float(p[1])
        evs.append((idx, t, float(p[2]) if len(p) > 2 else t + a.tshift))

    rows = []
    for idx, t, exp_s in evs:
        tpl_audio = st[int(t * srate):int((t + a.tlen) * srate)]
        tpl = onset_series(tpl_audio, srate, hop)
        # windowed search around the expected moment
        exp = exp_s * 1000 // a.hop_ms
        lo = max(0, int(exp - a.win * 1000 / a.hop_ms))
        hi = min(len(T), int(exp + a.win * 1000 / a.hop_ms))
        seg = T[lo:hi]
        c, _ = corr_all(seg, tpl)
        kb = int(np.argmax(c)) if len(c) else 0
        best = float(c[kb]) if len(c) else 0.0
        base = float(np.median(c)) if len(c) else 0.0
        rows.append((idx, t, best, base, (lo + kb) * a.hop_ms / 1000.0))
        print("  idx %-3s port stream %7.2f s -> shipped near %7.1f s: corr %+.3f (median of window %+.3f) "
              "best at capture t=%.1f s" % (idx, t, exp_s, best, base, (lo + kb) * a.hop_ms / 1000.0))

    print("controls (template from one event searched at another event's moment):")
    for i in range(len(evs)):
        for j in range(len(evs)):
            if i == j:
                continue
            idx, t, _e = evs[i]
            tpl = onset_series(st[int(t * srate):int((t + a.tlen) * srate)], srate, hop)
            exp = evs[j][2] * 1000 // a.hop_ms
            lo = max(0, int(exp - a.win * 1000 / a.hop_ms))
            hi = min(len(T), int(exp + a.win * 1000 / a.hop_ms))
            c, _ = corr_all(T[lo:hi], tpl)
            if len(c):
                print("  %-3s -> %-3s window: best %+.3f median %+.3f" % (idx, evs[j][0], c.max(), np.median(c)))


if __name__ == "__main__":
    main()
