#!/usr/bin/env python3
"""Per-phase present census of a Pocket nightlog (docs/416, docs/417).

A nightlog mixes two very different problems in the same line stream: holds whose surface never changes (the
logo card, the stage card -- every byte copied is wasted, docs/416/417) and animated content whose copies are
all real (the .fli intro, gameplay -- here the copy is the frame budget and any hiccup shows as choppiness).
Sorting the log into phases separates them, because the readout already carries both halves:

    view=0 640x480 sig=ecd967f7 chg=1 pal=255   <- what is on screen, and whether it changed
    perf pres=17399us max=65306us pump=... skips=0
    stages last m=.. p=.. c=16900 f=.. x=..     <- the copy stage's own last-frame cost
    memstat frames=2097 ticks=18210(+492/s) ...

Usage:
    python3 scripts/analysis/present_census.py [nightlog.sav] [--hz 490]

Static windows (no sig change after the first frame) are the ones PRESENT_SKIP targets; saturated windows
(frames/s ~= 1/copy) are the ones it cannot help and the choppiness lives in.
"""
import argparse
import re
import sys

VW = re.compile(r'view=(\d) (\d+)x(\d+) sig=([0-9a-f]{8}) chg=(\d+) pal=(\d+)')
PERF = re.compile(r'perf pres=(\d+)us max=(\d+)us pump=(\d+) ticks=(\d+) skips=(\d+)'
                 r'(?:\(hash (\d+) smp (\d+)\))?')
STAGES = re.compile(r'stages last m=(\d+) p=(\d+) c=(\d+) f=(\d+) x=(\d+)')
MODE = re.compile(r'fb mode -> (native|game) (\d+)x(\d+) rc=(-?\d+)')
MEM = re.compile(r'memstat frames=(\d+) ticks=(\d+)\(\+(\d+)/s\)')


def load(path):
    raw = open(path, 'rb').read().decode('latin1')
    i = raw.rfind('boot marker')                      # newest boot only; the tail is a previous run (docs/402)
    return raw[i:] if i >= 0 else raw


def phases(text, hz):
    """Segment the run at every view/geometry change.

    memstat is the clock (ticks at hz, and the frame counter); the view= line names the phase and carries the
    signature; perf/stages attach to whichever phase is current. A phase's span ends where the next one starts.
    """
    ev = []                                       # (tick, frames, kind, payload)
    tick = frames = None
    for line in text.splitlines():
        m = MEM.search(line)
        if m:
            tick, frames = int(m.group(2)), int(m.group(1))
            ev.append((tick, frames, 'mem', None))
            continue
        m = VW.search(line)
        if m:
            ev.append((tick, frames, 'view',
                       (m.group(1), int(m.group(2)), int(m.group(3)), m.group(4), int(m.group(5)))))
            continue
        m = PERF.search(line)
        if m:
            ev.append((tick, frames, 'perf', (int(m.group(1)), int(m.group(2)), int(m.group(5)),
                                                  int(m.group(6)) if m.group(6) else -1)))
            continue
        m = STAGES.search(line)
        if m:
            ev.append((tick, frames, 'stages', int(m.group(3))))
        m = MODE.search(line)
        if m:
            ev.append((tick, frames, 'mode', '%s %sx%s rc=%s' % (m.group(1), m.group(2), m.group(3), m.group(4))))

    out, cur = [], None
    modes = []            # (tick, label): a mode line describes the surface the NEXT frames will use
    for tk, fr, kind, d in ev:
        if kind == 'view':
            view, w, h, sig, chg = d
            if cur is None or (view, w, h) != (cur['view'], cur['w'], cur['h']):
                if cur is not None:
                    cur['end_tick'], cur['end_frames'] = cur['last_tick'], cur['last_frames']
                cur = {'view': view, 'w': w, 'h': h, 'start_tick': tk, 'start_frames': fr,
                       'sigs': set(), 'pres': [], 'copy': [], 'mode': [], 'last_tick': tk, 'last_frames': fr,
                       'end_tick': tk, 'end_frames': fr}
                out.append(cur)
            if cur is not None:
                cur['sigs'].add(sig)
        elif kind == 'mode':
            # Recorded even before the first phase exists: the boot block's own mode set is the surface the
            # opening phase draws into, and dropping it attributed 38 s of 640x480 copies to the wrong surface.
            modes.append((tk if tk is not None else -1, d))
        elif cur is not None:
            if kind == 'mem':
                cur['last_tick'], cur['last_frames'] = tk, fr
            elif kind == 'perf':
                cur['pres'].append(d)
            else:
                cur['copy'].append(d)
        if cur is not None:
            cur['end_tick'], cur['end_frames'] = max(cur['end_tick'], tk or 0), cur['end_frames']
    if out:                                     # the last phase is closed by the log's end, not by a new phase
        out[-1]['end_tick'], out[-1]['end_frames'] = out[-1]['last_tick'], out[-1]['last_frames']
    modes.sort(key=lambda x: x[0])
    for p in out:
        st = p['start_tick'] if p['start_tick'] is not None else 0
        # A mode line printed inside a phase usually announces the surface the NEXT phase will use (the mode
        # change and the game's own view change are two different observations a second apart). Take the last
        # mode announced at or before the phase's start, else the earliest one in the log -- that is what the
        # boot block prints before any frame is counted.
        chosen = [m for m in modes if m[0] <= st]
        p['mode'] = [chosen[-1][1]] if chosen else ([modes[0][1]] if modes else [])
    for p in out:
        if p['start_frames'] is None: p['start_frames'] = 0
        if p['end_frames'] is None: p['end_frames'] = p['start_frames']
    return out


def surf_bytes(p):
    """What one present writes, in bytes: the display surface the OS applied, not the game picture.

    A 320x200 view on a native 640x480 surface writes 307 200 bytes (docs/416), and the same view on the OS
    320x240 surface writes 76 800 -- so a phase's copy time is meaningless without the mode it ran in.
    """
    if not p['mode']:
        return None
    last = p['mode'][-1]
    m = re.search(r'(\d+)x(\d+)', last)
    return int(m.group(1)) * int(m.group(2)) if m else None


def bytes_per_s(p, copy_us):
    n = surf_bytes(p)
    return n / (copy_us / 1e6) / 1e6 if n and copy_us else 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log', nargs='?', default='/Volumes/Pocket/Saves/night/common/nightlog.sav')
    ap.add_argument('--hz', type=float, default=490.0)
    a = ap.parse_args()
    text = load(a.log)
    ph = phases(text, a.hz)
    if not ph:
        print('no phases found', file=sys.stderr); return 1
    print('phases in the newest boot of %s (%d lines)' % (a.log, len(text.splitlines())))
    print('%-12s %-9s %6s %7s %8s %9s %9s %8s %s' %
          ('t0..t1', 'view', 'hold_s', 'sig_chg', 'pres/s', 'pres_us', 'copy_us', 'frames', 'shape'))
    tot_copy = 0.0
    for p in ph:
        t0 = (p['start_tick'] or 0) / a.hz
        t1 = (p['end_tick'] or 0) / a.hz
        hold = t1 - t0
        nchg = len(p['sigs'])
        pres = p['pres']
        pus = int(sum(x[0] for x in pres) / len(pres)) if pres else 0
        # The log's skips= is a CUMULATIVE counter, so a phase's skips are a difference, not a sum -- the
        # first cut summed it and printed 35111 "skips" for a phase of 2285 frames (docs/422).
        skc = [x[2] for x in pres]
        hsh = [x[3] for x in pres if len(x) > 3 and x[3] >= 0]
        skp = (max(skc) - min(skc)) if skc else 0
        skh = (max(hsh) - min(hsh)) if hsh else -1
        cus = int(sum(p['copy']) / len(p['copy'])) if p['copy'] else 0
        nf = max(0, (p['end_frames'] or 0) - (p['start_frames'] or 0))
        rate = nf / hold if hold > 0.05 else 0
        # How much of this phase could PRESENT_SKIP remove? A change costs three writes (the proof re-latches
        # into each display buffer, docs/416), so the skip fraction is one change-times-three over the frames.
        skip_pct = 100.0 * max(0.0, 1.0 - 3.0 * max(nchg - 1, 1) / max(nf, 1)) if nf else 0.0
        if nf == 0:
            shape = 'INCOMPLETE (no frame window in this log)'
        elif skip_pct >= 70:
            shape = 'MOSTLY STATIC %3.0f%% skippable' % skip_pct
        elif skip_pct >= 25:
            shape = 'MIXED        %3.0f%% skippable' % skip_pct
        elif pus and rate and (1.0 / max(rate, 1e-9)) < 1.35 * pus:
            shape = 'SATURATED, copy is the budget'
        else:
            shape = 'animated / cheap'
        tot_copy += nf * cus / 1e6
        pr = [x[0] for x in pres]
        lo = min(pr) if pr else 0
        hi = max(pr) if pr else 0
        print('%3.1f-%6.1f  %-9s %6.2f %7d %8.1f %8d %9d %8d  n=%2d %s%s' %
              (t0, t1, '%s %dx%d' % (p['view'], p['w'], p['h']), hold, nchg, rate, lo, cus, nf, len(pres), shape,
               ('  skips=%d%s' % (skp, (' (hash %d)' % skh) if skh >= 0 else '')) if skp else ''))
        sb = surf_bytes(p)
        if p['mode']:
            line = '%38s surface: %s   bytes/frame=%s' % ('', p['mode'][-1], sb)
            # MB/s only where the phase has enough perf samples for a mean to mean anything.
            if sb and cus >= 1000 and len(pres) >= 5:
                line += '   %.1f MB/s (pres %d..%d us)' % (bytes_per_s(p, cus), lo, hi)
            elif len(pres) < 5:
                line += '   <-- too few samples (%d) for a rate' % len(pres)
            print(line)
    wall = (ph[-1]['end_tick'] or 0) / a.hz
    print('\nwall %.1f s; time inside the surface copy ~%.1f s (%.0f%% of it)' %
          (wall, tot_copy, 100.0 * tot_copy / wall if wall else 0))


if __name__ == '__main__':
    main()
