#!/usr/bin/env python3
"""Is a fan repack's music the same music?  Compare two sets of cut tracks, second by second.

The extractor can slice a fan rip's `MUSIC.RES` into `trackNN.bin` files, and a hash says either `same` or
`different` -- which is not the question anybody is really asking.  The question is whether the music is the
same music: the same tune, the same length, the same place to start, and if it stops matching partway
through, *when* it stops matching and by how much.

So this walks both sides in one-second windows and reports, per track:

  * how many seconds are byte-identical, and the first second where they are not
  * the RMS and the sample peak of each side, which is how a re-encode or a volume change shows up
    (byte-identical implies both of these agree, and a track whose RMS matches but whose bytes do not is a
    shifted or re-encoded one rather than a different recording)

    uv run python scripts/analysis/fan_audio_vs_disc.py \\
        scratch/nkx/fan/cd extracted/cd
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

SECTOR = 2352
SEC_BYTES = 44100 * 4                    # 75 sectors a second, 2352 bytes each
try:
    import numpy as np
except ImportError:                      # the extract-only environment has no numpy
    np = None


def rms_peak(data: bytes) -> tuple[float, int]:
    if np is not None:
        a = np.frombuffer(data[:len(data) // 4 * 4], dtype="<i2")
        if a.size == 0:
            return 0.0, 0
        return float(math.sqrt(float(np.mean(np.square(a, dtype=np.float64))))), int(np.max(np.abs(a)))
    # No numpy: sample every 64th sample so a pass over a minute of audio stays cheap.
    vals = [int.from_bytes(data[i:i + 2], "little", signed=True) for i in range(0, len(data) - 1, 128)]
    if not vals:
        return 0.0, 0
    return math.sqrt(sum(v * v for v in vals) / len(vals)), max(abs(v) for v in vals)


def compare_one(a: Path, b: Path) -> dict:
    fa, fb = a.open("rb"), b.open("rb")
    n = min(a.stat().st_size, b.stat().st_size) // SEC_BYTES
    same = 0
    first = None
    rms_a = rms_b = peak_a = peak_b = 0.0
    ra = []
    rb = []
    for s in range(n):
        x = fa.read(SEC_BYTES)
        y = fb.read(SEC_BYTES)
        if x == y:
            same += 1
        elif first is None:
            first = s
        if s % 15 == 0 or s == n - 1:     # a sample of levels, not a pass over every sample
            r1, p1 = rms_peak(x)
            r2, p2 = rms_peak(y)
            ra.append((r1, p1))
            rb.append((r2, p2))
    fa.close(); fb.close()
    if ra:
        rms_a = sum(r for r, _ in ra) / len(ra)
        rms_b = sum(r for r, _ in rb) / len(rb)
        peak_a = max(p for _, p in ra)
        peak_b = max(p for _, p in rb)
    return dict(seconds=n, same=same, first=first, rms_a=rms_a, rms_b=rms_b,
                peak_a=peak_a, peak_b=peak_b)


def main(paths) -> int:
    if len(paths) < 2:
        sys.exit(__doc__)
    a_dir, b_dir = Path(paths[0]), Path(paths[1])
    tracks = sorted(p.name for p in a_dir.glob("track*.bin"))
    missing = [t for t in tracks if not (b_dir / t).is_file()]
    if missing:
        sys.exit(f"no counterpart in {b_dir} for: {' '.join(missing)}")
    print(f"A = {a_dir}\nB = {b_dir}\n")
    print(f"{'track':7s} {'sec':>6s} {'identical':>11s} {'first diff':>11s} "
          f"{'RMS A/B':>16s} {'peak A/B':>14s}  verdict")
    worst = 0
    for t in tracks:
        r = compare_one(a_dir / t, b_dir / t)
        pct = 100.0 * r["same"] / r["seconds"] if r["seconds"] else 0.0
        verdict = ("byte-identical" if r["same"] == r["seconds"] else
                   f"drifts after {r['first']} s" if r["first"] is not None else "no common length")
        if pct < worst or worst == 0:
            worst = pct
        level = ""
        if r["rms_b"] > 0:
            ratio = r["rms_a"] / max(1.0, r["rms_b"])
            level = f" ({100 * ratio:.0f}% level)" if abs(ratio - 1.0) > 0.02 else ""
        print(f"{t:7s} {r['seconds']:>6} {pct:>10.1f}% {(str(r['first']) if r['first'] is not None else '-'):>11s} "
              f"{r['rms_a']:>7.0f}/{r['rms_b']:<8.0f} {r['peak_a']:>6d}/{r['peak_b']:<7d}  {verdict}{level}")
    print(f"\nworst track: {worst:.1f}% of one second identical")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
