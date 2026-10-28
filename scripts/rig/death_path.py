#!/usr/bin/env python3
"""What happens to an entity record after its hit points reach zero (the "enemies blink and disappear" report).

The shipped game routes death through the falling family: the reaction table gives 0x66..0x5b for types 22..32
(docs/117), and fn_012616 turns state 0x70 into 0x66 with +0x50 = 200 and the partner record's +0x17 = 0x1f
(port f.c, guest 0x00012908-0x00012956). So a dead entity must be *seen* in 0x6f..0x73 and/or 0x5b..0x66 for a
stretch of samples before its record goes empty. If a side removes the record at the moment hp hits zero, the
corpse animation never plays -- which is what "just blinking and disappearing" describes.

Both sides are read through the same entity-list walk (ent## cells from --ents / the port rig dump).
"""
import argparse, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port  # noqa: E402

RB = 0x160
FALL = range(0x5b, 0x67)
DEATH_CHAIN = {0x6f, 0x70, 0x71, 0x72, 0x73}

def rd(b, o, w=4):
    return int.from_bytes(b[o:o+w], 'little', signed=True)

def series(rows, cells, offs):
    out = {}
    for t, blob in rows:
        for n, off in offs.items():
            blk = blob[off:off + cells[n]]
            if len(blk) < RB or not any(blk): continue
            out.setdefault(n, []).append((t, rd(blk, 0x17), rd(blk, 0x29, 2), blk[0x30]))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--orig', required=True); ap.add_argument('--port', required=True)
    ap.add_argument('--window-ms', type=int, default=4000)
    a = ap.parse_args()
    O, oc = load_orig(a.orig); P, pc = load_port(a.port)
    def prep(cells):
        ent = [(i, c) for i, c in enumerate(cells) if str(c['name']).startswith('ent')]
        offs = {n: c.get('offset', sum(x.get('rec_bytes', 0) for x in cells[:i])) for n, (i, c) in enumerate(ent)}
        sizes = {n: c.get('rec_bytes', RB) for n, (i, c) in enumerate(ent)}
        return offs, sizes
    oo, osz = prep(oc); po, psz = prep(pc)
    orows = [(t, hb) for t, ds, hb in O]
    prows = [(r[0], r[1]) for r in P]
    so = series(orows, osz, oo); sp = series(prows, psz, po)
    for name, se in (('shipped', so), ('port', sp)):
        deaths = 0; with_fallen = 0; with_chain = 0; printed = 0
        hist = {}
        for n, rows in se.items():
            for k in range(1, len(rows)):
                t0, st0, hp0, d0 = rows[k-1]; t1, st1, hp1, d1 = rows[k]
                if d1 and not d0:                       # the sample where the dead flag goes up
                    deaths += 1
                    seen = []
                    for (t2, st2, hp2, d2) in rows[k:min(len(rows), k+24)]:
                        if t2 - t1 > a.window_ms: break
                        seen.append(st2); hist[st2] = hist.get(st2, 0) + 1
                    if any(s in FALL for s in seen): with_fallen += 1
                    if any(s in DEATH_CHAIN for s in seen): with_chain += 1
                    if printed < 6:
                        printed += 1
                        print(f'   {name} slot {n} t={t1} dead-flag up at state {st1:#x}; next states: '
                              + ' '.join(f'{s:#x}' for s in seen[:16]))
                    break
        print(f'{name}: dead-flag transitions={deaths}  reached a fallen state (0x5b..0x66)={with_fallen}  '
              f'reached the death chain (0x6f..0x73)={with_chain}')
        print('   states seen after the dead flag: ',
              ' '.join(f'{s:#x}x{c}' for s, c in sorted(hist.items(), key=lambda kv: -kv[1])[:12]))
    removal_analysis('shipped', so)
    removal_analysis('port', sp)

def removal_analysis(name, se, gap_ms=700, pre_ms=2000):
    """A record going empty for at least two consecutive dumps = the entity was unlinked (killed and removed).
    What matters is the state sequence in the window BEFORE removal: the shipped game must show the falling
    family there (0x5b..0x66, reached via the 0x6f..0x73 death chain), a port that "blinks and disappears"
    shows ordinary combat states right up to the cut."""
    removals = 0; fell = 0; chain = 0; hist = {}; shown = 0
    for n, rows in se.items():
        for k in range(len(rows) - 1):
            if rows[k+1][0] - rows[k][0] < gap_ms: continue
            removals += 1
            t_last = rows[k][0]
            pre = [st for (t, st, hp, d) in rows if t_last - pre_ms <= t <= t_last]
            for st in pre: hist[st] = hist.get(st, 0) + 1
            if any(st in FALL for st in pre): fell += 1
            if any(st in DEATH_CHAIN for st in pre): chain += 1
            if shown < 6:
                shown += 1
                print(f'   {name} slot {n} last seen t={t_last} state {rows[k][1]:#x}; states in the '
                      f'{pre_ms} ms before removal: ' + ' '.join(f'{st:#x}' for st in pre[-16:]))
            break
    print(f'{name}: removals={removals}  fallen state seen just before removal={fell}  '
          f'death-chain state seen={chain}')
    print('   pre-removal state histogram:', ' '.join(f'{st:#x}x{c}' for st, c in sorted(hist.items(), key=lambda kv: -kv[1])[:12]))

def main2():
    pass

if __name__ == '__main__':
    main()
