#!/usr/bin/env python3
"""Compare a Pocket-shim rig dump against an SDL-host rig dump of the SAME scripted route (task-11).

Both hosts write the same src/app/rig_sink.c sidecars, so this is a port-vs-port diff through one shared src/f --
not a port-vs-emulator diff.  It has to be paced-independent, because the two hosts present at different wall-clock
rates: a `--frames 2500` run presented 2 501 frames on the Pocket shim and 5 233 on the SDL host in the same stage,
so frame *index* is not a shared coordinate.  What is shared is the state itself, so every comparison below is a
comparison of state sets or state keyed by the game's own tick.

  frames.bin    the 40x30 luma signature of each presented frame (rig_sink.c).  Compared as SETS: how many distinct
                pictures each host drew, and how many of them are the same picture on both.  Also the deduped
                sequence of view geometries, which is the shape of the run.
  cdda.jsonl    two kinds of record live in one file: the game's CD selects (they carry index/lba/sectors) and
                docs/307's periodic delivered-audio samples (they carry only `bytes`).  Selects are compared as an
                ordered, run-collapsed sequence; the audio half is compared as total bytes delivered.
  records.jsonl the entity list followed through its +0x13f links, stamped with the game tick.  Compared tick by
                tick, which is the only coordinate both hosts agree on.
  meta.json     the rig's table manifest (DS cell map): identical means the two hosts are watching the same memory.

Usage: diff_hosts.py <pocket_dump> <sdl_dump> [<pocket_dump2> <sdl_dump2> ...]
"""
import json
import os
import struct
import sys

HDR = 32                   # int64 t_ms, uint64 frame, int32 w, h, changed, pad


def frames(path):
    if not os.path.exists(path):
        return None
    d = open(path, 'rb').read()
    if len(d) < 12:
        return None
    _magic, _ver, siglen = struct.unpack_from('<III', d, 0)
    out, off = [], 12
    while off + HDR + siglen <= len(d):
        _t, frame, w, h, _changed, _pad = struct.unpack_from('<qQiiiq', d, off)
        out.append((frame, w, h, d[off + HDR: off + HDR + siglen]))
        off += HDR + siglen
    return out


def jsonl(path):
    if not os.path.exists(path):
        return []
    out = []
    for line in open(path, errors='replace'):
        line = line.strip()
        if line:
            try:
                out.append(json.loads(line))
            except ValueError:
                pass
    return out


def collapse(selects):
    """The game re-issues the same fn_010b1d selection on frame after frame (docs/105: every selection overlaps
    the old track), so a raw select stream is mostly repeats.  Collapse runs, keep the order."""
    seq = []
    for s in selects:
        if seq and seq[-1][:4] == s[:4]:
            seq[-1][4] += 1
        else:
            seq.append(list(s) + [1])
    return seq


def pair(pocket, sdl):
    print(f"\n=== {pocket}  vs  {sdl}")
    a, b = frames(os.path.join(pocket, 'frames.bin')), frames(os.path.join(sdl, 'frames.bin'))
    if not a or not b:
        print("  frames.bin   : MISSING on one side")
    else:
        sa, sb = {x[3] for x in a}, {x[3] for x in b}
        inter = len(sa & sb)
        print(f"  frames.bin   : {len(a)} vs {len(b)} presented frames; distinct pictures {len(sa)} vs {len(sb)}; "
              f"pictures drawn by BOTH hosts {inter} "
              f"({inter / max(len(sa), 1) * 100:.1f} % of the pocket's, {inter / max(len(sb), 1) * 100:.1f} % of the SDL host's)")
        def geoms(recs):
            out = []
            for _f, w, h, _s in recs:
                if not out or out[-1] != (w, h):
                    out.append((w, h))
            return out
        ga, gb = geoms(a), geoms(b)
        print(f"                 view geometry sequence: pocket {ga}")
        print(f"                                   sdl  {gb}")

    ca, cb = jsonl(os.path.join(pocket, 'cdda.jsonl')), jsonl(os.path.join(sdl, 'cdda.jsonl'))
    # The two hosts stamp the same selection differently: src/app/main.c writes a `stop` for the track it is
    # leaving and a `play` for the one it starts, while of_cdda.c replaces the voices and marks only the `play`.
    # The selection itself -- (index, lba, sectors) -- is the state fact, so the sequence compared here is the
    # ordered, run-collapsed sequence of plays.
    sel = lambda recs: [(r.get('event'), r.get('index'), r.get('lba'), r.get('sectors'))
                        for r in recs if 'index' in r]
    plays = lambda recs: [x[1:] for x in sel(recs) if x[0] == 'play']
    audio = lambda recs: [r.get('bytes', 0) for r in recs if 'index' not in r]
    qa, qb = collapse(plays(ca)), collapse(plays(cb))
    print(f"  cdda.jsonl   : select records {len(sel(ca))} vs {len(sel(cb))}; plays {len(plays(ca))} vs "
          f"{len(plays(cb))} -> {len(qa)} vs {len(qb)} distinct selections")
    for i in range(max(len(qa), len(qb))):
        x = qa[i] if i < len(qa) else None
        y = qb[i] if i < len(qb) else None
        mark = "same" if x == y else "DIFFER"
        print(f"      {i:>2}  pocket {x}   sdl {y}   {mark}")
    ba, bb = audio(ca), audio(cb)
    if ba or bb:
        print(f"                 delivered audio: pocket {max(ba or [0])} bytes, sdl {max(bb or [0])} bytes "
              f"({(max(ba or [0]) - max(bb or [0])) / 44100 / 4:.2f} s of stereo CD-DA apart)")

    ra, rb = jsonl(os.path.join(pocket, 'records.jsonl')), jsonl(os.path.join(sdl, 'records.jsonl'))
    def by_tick(recs):
        d = {}
        for r in recs:
            t = r.get('tick')
            if t is None:
                continue
            d[t] = (len(r.get('ents') or ()), tuple(sorted((e.get('type'), e.get('x'), e.get('y'))
                                                           for e in (r.get('ents') or []) if isinstance(e, dict))))
        return d
    ta, tb = by_tick(ra), by_tick(rb)
    common = set(ta) & set(tb)
    same = sum(1 for t in common if ta[t] == tb[t])
    same_cnt = sum(1 for t in common if ta[t][0] == tb[t][0])
    print(f"  records.jsonl: {len(ta)} vs {len(tb)} tick samples, {len(common)} ticks in common; "
          f"entity COUNT equal on {same_cnt}/{len(common)}, full (type,x,y) equal on {same}/{len(common)}")
    if common:
        print(f"                 tick range pocket {min(ta)}-{max(ta)}, sdl {min(tb)}-{max(tb)}")
    ma, mb = jsonl(os.path.join(pocket, 'markers.jsonl')), jsonl(os.path.join(sdl, 'markers.jsonl'))
    def res_seq(recs):
        out = []
        for r in recs:
            w = r.get('what', '')
            if w.startswith('res '):
                n = w[4:]
                if not out or out[-1][0] != n:
                    out.append([n, 1])
                else:
                    out[-1][1] += 1
        return out
    ra_, rb_ = res_seq(ma), res_seq(mb)
    na, nb = {x[0] for x in ra_}, {x[0] for x in rb_}
    print(f"  markers.jsonl: {len(ma)} vs {len(mb)} markers; RES loads {len(ra_)} vs {len(rb_)} runs "
          f"({len(na)} vs {len(nb)} distinct names); names common to both: {len(na & nb)}")
    only_a, only_b = sorted(na - nb), sorted(nb - na)
    if only_a:
        print(f"                 pocket-only res names: {only_a[:8]}{' ...' if len(only_a) > 8 else ''}")
    if only_b:
        print(f"                 sdl-only res names : {only_b[:8]}{' ...' if len(only_b) > 8 else ''}")
    if ra_ and rb_ and [x[0] for x in ra_] == [x[0] for x in rb_]:
        print("                 the ordered resource-load sequence is identical")

    for name in ('meta.json', 'cells.bin'):
        pa_, pb_ = os.path.join(pocket, name), os.path.join(sdl, name)
        if os.path.exists(pa_) and os.path.exists(pb_):
            same = open(pa_, 'rb').read() == open(pb_, 'rb').read()
            print(f"  {name:<13}: {'byte-identical' if same else 'differs'}")


def main():
    args = sys.argv[1:]
    if len(args) < 2 or len(args) % 2:
        print(__doc__)
        return 2
    for i in range(0, len(args), 2):
        pair(args[i], args[i + 1])
    return 0


if __name__ == '__main__':
    sys.exit(main())
