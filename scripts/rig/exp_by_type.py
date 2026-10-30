#!/usr/bin/env python3
"""exp_by_type.py -- the per-enemy-type EXP award, grouped by entity type, on both sides (docs/182 row 2b).

Item 2 of docs/manual-playtest/03-2026-10-29.md asks two things: the EXP *text* is garbled, and "check for each
enemy gains different EXP per enemy type".  The award mechanism is known from the shipped write site --
`*(player + 0x52) += victim[+0x32]` at guest 0x15453 (f.c:7938) -- so the comparable quantity is the award field
`+0x32` of each entity record, grouped by that record's type byte `+0x00`.

Records are read the sanctioned way: the port through scripts/rig/cells_reader.py (its `ent##` blocks), the shipped
side through `state_*.heap.bin` blocks in the order `timeline.json`'s `ptr_cells` lists them (docs/124, docs/167 §9).

  exp_by_type.py --run /tmp/rig/g33 [--award 0x32] [--type 0x00]
"""
import argparse, json, os, struct, sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cells_reader import CellStream


def read_blocks(path, n_blocks, rec_bytes):
    d = open(path, 'rb').read()
    out = []
    for k in range(n_blocks):
        o = k * rec_bytes
        if o + rec_bytes <= len(d):
            out.append(d[o:o + rec_bytes])
    return out



def looks_like_entity(blob):
    """docs/125 record layout: type byte at +0x00, state dword at +0x17, hp word at +0x29.

    Both dumps include blocks that are NOT entity records -- the shipped side dumps a record block at every
    pointer cell (64 of them per state, most of them tables), and a non-entity block's `+0x32` is a piece of
    unrelated data that looks like a huge award (0x65656566, 0x80808080...).  Without this gate the census is
    noise: the first run reported shipped "types" 0x03/0x05/0x0a/0x22 with awards like 0x2fdc002c.
    """
    if len(blob) < 0x2b:
        return False
    if blob[0] >= 0x40:
        return False
    state = struct.unpack_from('<I', blob, 0x17)[0]
    if state >= 0x100:
        return False
    hp = struct.unpack_from('<H', blob, 0x29)[0]
    return 0 < hp <= 0x2000


def hist(port_dir, orig_dir, award, typ, rec_bytes):
    # ---- port side: ent## cells from the rig dump ------------------------------------------
    cs = CellStream(port_dir)
    # all 12 ent## entries share the DS name of the pointer cell they were reached from (0x53c70); the
    # dict CellStream.cells keys by DS, so iterating its values collapses 12 slots into 1.  Iterate the
    # meta list itself, which keeps one entry per record slot.
    ents = [c for c in cs.meta['cells'] if c['name'].startswith('ent')]
    pmap = defaultdict(lambda: [0, set(), 0])          # type -> [samples seen, award values, nonzero samples]
    for i in range(cs.samples):
        for c in ents:
            off = i * cs.stride + 40 + c['offset']
            blob = cs.data[off:off + c['rec_bytes']]
            if len(blob) < max(award + 4, typ + 1) or not looks_like_entity(blob):
                continue
            t = blob[typ]
            v = struct.unpack_from('<I', blob, award)[0]
            e = pmap[t]
            e[0] += 1
            e[1].add(v)
            if v: e[2] += 1

    # ---- shipped side: heap blocks per sampled state ---------------------------------------
    tl = json.load(open(os.path.join(orig_dir, 'timeline.json')))
    pc = tl['ptr_cells']
    smap = defaultdict(lambda: [0, set(), 0])
    files = sorted(f for f in os.listdir(orig_dir) if f.startswith('state_') and f.endswith('.heap.bin'))
    n_states = 0
    for f in files:
        f = os.path.join(orig_dir, f)
        n_states += 1
        for blob in read_blocks(f, len(pc), rec_bytes):
            if len(blob) < max(award + 4, typ + 1) or not looks_like_entity(blob):
                continue
            t = blob[typ]
            v = struct.unpack_from('<I', blob, award)[0]
            e = smap[t]
            e[0] += 1
            e[1].add(v)
            if v: e[2] += 1
    return pmap, smap, len(ents), n_states, cs.samples


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--run', required=True)
    ap.add_argument('--award', default='0x32')
    ap.add_argument('--type', default='0x00')
    ap.add_argument('--rec', type=int, default=352)
    a = ap.parse_args()
    award, typ = int(a.award, 0), int(a.type, 0)
    pmap, smap, nent, nstates, nsamp = hist(os.path.join(a.run, 'port'), os.path.join(a.run, 'orig'),
                                            award, typ, a.rec)
    print("award field +%s, type byte +%s" % (hex(award), hex(typ)))
    print("port: %d entity slots over %d samples (10 Hz)   | shipped: %d sampled states (4 Hz)"
          % (nent, nsamp, nstates))
    types = sorted(set(pmap) | set(smap))
    print("%-6s %-28s %s" % ("type", "port award values (nonzero/seen)", "shipped award values (nonzero/seen)"))
    for t in types:
        p = pmap.get(t, [0, set(), 0]); s = smap.get(t, [0, set(), 0])
        fmt = lambda e: "%d/%d  {%s}" % (e[2], e[0], ", ".join(sorted(hex(v) for v in list(e[1])[:6])))
        print("%-6s %-34s %s" % ("0x%02x" % t, fmt(p), fmt(s)))


if __name__ == '__main__':
    main()
