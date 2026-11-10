#!/usr/bin/env python3
"""Read a Pocket run log and answer the questions this project keeps asking, in one pass.

Why a script and not an inline parse: four of my misreadings this week were the PARSER, not the build
(docs/402 tail-of-a-longer-run, docs/422 summed-cumulative-counter, docs/427 hash-inside-the-copy-bucket,
docs/428 slice-eating-a-line-prefix). Every number below is line-anchored and every cumulative counter is
reported as a difference.

    ./scripts/pocket/logreport.py                       # the mounted card's log
    ./scripts/pocket/logreport.py scratch/pocket/logs/nightlog-0.2.8-....log

What it prints: the core version, the per-phase table (which geometry was on screen), the copy-stage cost per
phase (the number that decides ROW_SKIP), the gap-worst distribution (choppiness), and the skip/self-check
counters. Nothing here writes anything.
"""
import argparse
import pathlib
import re
import statistics
import sys

SPLIT = re.compile(r'split wall=(\d+)ms pump=(\d+)ms pres=(\d+)ms disc=(\d+)ms aud=(\d+)ms '
                   r'rest=(\d+)ms frames=(\d+)')
STAGES = re.compile(r'stages last m=(\d+) p=(\d+) c=(\d+) f=(\d+) x=(\d+)')
GAP = re.compile(r'gap worst=(\d+)ms since report')
PERF = re.compile(r'perf pres=(\d+)us .* rows c=(\d+) s=(\d+) fs=(\d+) dry=(\d+) f=(\d+) inv=(\d+) '
                  r'mm=(\d+) chk=(\d+) \| br n/g/d/o=(\d+)/(\d+)/(\d+)/(\d+)')
GEO = re.compile(r'(view=\d+ \d+x\d+)')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log', nargs='?', default='/Volumes/Pocket/Saves/night/common/nightlog.sav')
    a = ap.parse_args()
    p = pathlib.Path(a.log)
    if not p.exists():
        print('no log at %s' % p); return 1

    # Line-anchored, and the whole line -- docs/428: slicing from 'boot marker' ate the 'core X.Y.Z: ' prefix
    # that names the build, and I concluded the log did not record its version. It always has.
    lines = [l.strip() for l in p.read_bytes().decode('latin1').splitlines()]
    marks = [i for i, l in enumerate(lines) if 'boot marker' in l]
    if not marks:
        print('no boot marker in %s -- is the log from a run at all?' % p); return 1
    m = re.search(r'core (\S+): boot marker', lines[marks[-1]])
    ver = m.group(1) if m else '??'
    seg = lines[marks[-1]:]
    print('log: %s' % p)
    print('core %s | %d lines after newest boot marker | %d boot marker(s) in file' %
          (ver, len(seg), len(marks)))

    # Pair every split line with the geometry in effect at that moment, then bucket by geometry.
    geo, prev, secs, cum = None, None, [], [0]*7
    seg_stages, gaps = [], []   # (geometry, [copy ms]) per CONTIGUOUS phase -- two 640x480 phases
                            # share a label and must not be pooled
    for l in seg:
        g = GEO.search(l)
        if g:
            geo = g.group(1).replace('view=0 ', '640x480 ').replace('view=1 ', '320x200 ')
        s = SPLIT.search(l)
        if s:
            v = [int(x) for x in s.groups()]
            if prev:
                secs.append([v[k] - prev[k] for k in range(7)] + [geo])   # SPLIT has 7 groups
            prev = v
            cum = v
        st = STAGES.search(l)
        if st and geo:
            if seg_stages and seg_stages[-1][0] == geo:
                seg_stages[-1][1].append(int(st.group(3)) / 1000.0)              # copy stage, ms
            else:
                seg_stages.append((geo, [int(st.group(3)) / 1000.0]))
        gg = GAP.search(l)
        if gg:
            gaps.append(int(gg.group(1)))

    secs = [s for s in secs if s[7]]
    if not secs:
        print('no split lines -- this log predates the accounting line (0.2.5)')
        return 0
    print('run: %d one-second samples, %d frames total' % (len(secs), cum[6]))

    med = lambda xs: statistics.median(xs) if xs else float('nan')
    ph = []
    ph_segs = []   # phase segments that actually have stage lines, for the copy column
    for r in secs:
        if ph and ph[-1][0] == r[7]:
            ph[-1][1].append(r)
        else:
            ph.append((r[7], [r]))
    print('\n## per phase (geometry in effect; medians of per-second differences)')
    print('%-14s %5s %6s %8s %10s %7s %7s %7s' %
          ('geometry', 'secs', 'f/s', 'pres ms', 'copy ms', 'disc', 'aud', 'rest'))
    for g, rs in ph:
        if len(rs) < 3:
            continue
        c = [x for gg, rr in seg_stages if gg == g for x in rr]
        copy = ('%.1f' % med(c)) if c else '-'
        row = (g, len(rs), med([r[6] for r in rs]), med([r[2] for r in rs]), copy,
               med([r[3] for r in rs]), med([r[4] for r in rs]), med([r[5] for r in rs]))
        print('%-14s %5d %6.0f %8.0f %10s %7.0f %7.0f %7.0f' % row)
    if gaps:
        s = sorted(gaps)
        print('\n## choppiness (gap worst, ms/second, n=%d)' % len(gaps))
        print('   median %s  p90 %d  p99 %d  max %d' %
              (med(gaps), s[int(len(s) * .9)], s[min(len(s) - 1, int(len(s) * .99))], s[-1]))

    st = [l for l in seg if l.startswith('STALL')]
    if st:
        print('\n## stall witnesses (what the port was doing when no present happened)')
        for l in st[:8]:
            print('   ', l[:150])
        if len(st) > 8:
            print('    ... %d more' % (len(st) - 8))

    pf = [PERF.search(l) for l in seg if PERF.search(l)]
    if pf:
        v = [int(x) for x in pf[-1].groups()]
        # PERF groups: pres c s fs dry f inv mm chk br_n br_g br_d br_o -- 0.2.9's report first unpacked mm/chk
        # from the wrong slots and printed "mm=50 chk=0" for what was really inv=50, chk=4096. A reporting bug
        # that would have killed a correct build; the raw line is always in the log, and so is this comment.
        c, s_, fs, dry, fb, inv, mm, chk = v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]
        print('\n## row skip (cumulative, final perf line)')
        print('   rows copied %d  same %d  frames fully skipped %d  dry %d  fallback %d  invalidated %d' %
              (c, s_, fs, dry, fb, inv))
        print('   self-check: mm=%d mismatches over chk=%d verified skips  <-- mm must stay 0' % (mm, chk))
        print('   blit branches native/game/downscale/other = %d/%d/%d/%d' % (v[9], v[10], v[11], v[12]))
        if c + s_:
            print('   rows skipped: %.1f%%' % (100.0 * s_ / (c + s_)))

    # The verdict this report exists for. 30.1 ms is the measured 0.2.8 figure (64-bit fingerprint, 85% of
    # rows skipped, no saving); 12 ms is the bar set in docs/432 for keeping the feature at all.
    big = [(g, v) for g, v in seg_stages if '640x480' in g and len(v) >= 5]
    if big:
        g, v = max(big, key=lambda kv: len(kv[1]))
        c = med(v)
        print('\n## verdict: copy-stage median in %s = %.1f ms (0.2.8 was 30.1; bar to keep ROW_SKIP is ~12)' %
              (g, c))
        print('   %s' % ('ROW_SKIP earns its place.' if c < 12 else
                         'ROW_SKIP does not pay -- set ROW_SKIP=0 and the feature is off (docs/432).'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
