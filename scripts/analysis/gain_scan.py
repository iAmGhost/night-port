#!/usr/bin/env python3
"""gain_scan.py -- how much room the port's audio actually has.

"is it quiet?" has two different answers and they need different fixes:
  * the port attenuating itself (a chain of mixer volumes nobody measured), and
  * the samples having no headroom left (nothing can be louder than full scale).
This measures the second question from the two data sets the repo already holds.

  scratch/pocket/cd/track*.bin    the shipped CD-DA, byte-exact (docs/114, docs/332)
  /tmp/rig/*/port/pcm.wav         the port's board-only stream: the samples NIGHT's own driver
                                  transferred to the Sound Blaster (docs/164, docs/166)

Usage: python3 scripts/analysis/gain_scan.py [more pcm.wav ...]
"""
import glob
import math
import pathlib
import struct
import sys
import wave

FS = 32767
KNEE = 20480          # 0.625 of full scale -- the knee of_cdda.c uses (docs/361)
SLOPE = 4             # the excess above the knee is compressed by this factor


def int16s(path):
    """Raw .bin (no header) or a wav, either way a flat stream of signed 16-bit samples."""
    p = pathlib.Path(path)
    b = p.read_bytes()
    if b[:4] == b'RIFF':
        try:
            w = wave.open(str(p))
            b = w.readframes(w.getnframes())
            w.close()
        except (EOFError, wave.Error):
            return ()                                # a truncated capture: skip it, do not guess
    n = len(b) // 2
    return struct.unpack('<%dh' % n, b[:2 * n])


def stats(s):
    if not s:
        return None
    peak = max(abs(x) for x in s)
    rms = math.sqrt(sum(x * x for x in s) / len(s))
    return dict(n=len(s), peak=peak, rms=rms,
                peak_db=20 * math.log10(max(peak, 1) / FS),
                rms_db=20 * math.log10(max(rms, 1) / FS),
                hot=sum(1 for x in s if abs(x) > 0.9 * FS))


def gain(x, gain_pct):
    """of_gain16(): full gain below the knee, compressed above it, never below the input, never past full scale."""
    a, v = abs(x), abs(x) * gain_pct // 100
    clamped = False
    if v > KNEE:
        c = KNEE + (v - KNEE) // SLOPE
        v = max(a, c)                        # a gain setting may not attenuate what the disc recorded
    if v > FS:
        v, clamped = FS, True
    return (v if x >= 0 else -v), clamped


def knee_ceiling(gain_pct):
    """What a full-scale input becomes under that curve, and whether the curve had to clamp."""
    return gain(FS, gain_pct)


def main():
    print("=== CD-DA (the shipped disc, byte-exact)")
    worst = 0
    tot = tot_hot = 0
    for f in sorted(glob.glob('scratch/pocket/cd/track*.bin')):
        st = stats(int16s(f))
        if not st:
            continue
        tot += st['n']
        tot_hot += st['hot']
        worst = max(worst, st['peak'])
        print(f"  {pathlib.Path(f).name:<12} peak {st['peak']:>6} ({st['peak_db']:>6.2f} dBFS)   "
              f"rms {st['rms']:>7.1f} ({st['rms_db']:>6.2f} dBFS)   samples >0.9 FS: {st['hot']}")
    print(f"  worst peak over the disc: {worst} = {worst / FS * 100:.1f}% of full scale")
    print(f"  gain available before ANY sample clips: {FS / max(worst, 1):.2f}x")
    for g in (125, 150, 175, 200, 250):
        clipped = 0
        for f in sorted(glob.glob('scratch/pocket/cd/track*.bin')):
            s = int16s(f)
            clipped += sum(1 for x in s if abs(x) * g > 100 * FS)
        ceil, clamps = knee_ceiling(g)
        print(f"  {g}%: naive gain clips {clipped:>9} of {tot} samples ({clipped / tot * 100:5.3f}%); "
              f"of_gain16 maps full scale to {ceil}{' (clamped)' if clamps else ' (never clamps, never attenuates)'}")

    print("\n=== the port's board-only SFX stream (samples NIGHT's own driver sent to the board)")
    pats = sys.argv[1:] or glob.glob('/tmp/rig/*/port/pcm.wav')
    peaks = []
    for f in sorted(pats):
        st = stats(int16s(f))
        if not st:
            continue
        peaks.append(st['peak'])
        print(f"  {pathlib.Path(f).parent.parent.name:<12} frames {st['n']:>8}  peak {st['peak']:>6} "
              f"({st['peak_db']:>6.2f} dBFS)  rms {st['rms']:>7.1f} ({st['rms_db']:>6.2f} dBFS)")
    if peaks:
        mx = max(peaks)
        print(f"  worst measured SFX peak: {mx} = {mx / FS * 100:.1f}% of full scale")
        for g in (150, 200, 240, 300):
            print(f"  {g}%: {int(mx * g / 100)} -> {'SAFE: no measured sample reaches full scale' if mx * g <= 100 * FS else 'clips'}")
    print(f"\n=== the curve the port applies (of_gain16, KNEE {KNEE}, SLOPE 1/{SLOPE})")
    print("   input:      1000     10000     20000     30720     32767")
    for g in (100, 125, 150, 200, 212):
        print(f"  {g:>4}%:   " + "  ".join(f"{gain(x, g)[0]:>6}" for x in (1000, 10000, 20000, 30720, 32767)))
    print("\n=== verdict the port acts on")
    above = n = 0
    for f in sorted(pats):
        s = int16s(f)
        above += sum(1 for x in s if abs(x) > KNEE); n += len(s)
    if n:
        print(f"  samples above the knee ({KNEE}) in the board captures: {above} of {n} ({above / n * 100:.3f}%)")
    print("  SFX: most of the material sits far below full scale, but the loudest measured sample is 93.8%,")
    print("       so the gain has to pass through the knee (200% tops out below full scale; a naive 2x clips).")
    print("  CDDA: the disc is already at 0 dBFS, so music gain is a compressor, not a volume knob.")
    print(f"        The knee ({KNEE}, slope 1/{SLOPE}) cannot exceed full scale below 212%, and the max() against the")
    print("        input means no sample is ever made quieter by a gain setting -- which is why clamp_gain tops out at 212.")
    print("        CDDA_GAIN=150 is the shipping default; 100% puts the disc back at exactly its own level.")


if __name__ == '__main__':
    main()
