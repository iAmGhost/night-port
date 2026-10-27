#!/usr/bin/env python3
"""Read named fields out of the entity records on both sides, per drive phase.

The shipped game's entities are packed records on the heap, reached from DS pointer cells
(`DAT_00053c7c` = the player entity). The state oracle already dumps a fixed-size block at every
pointer cell on both sides, so a field can be compared directly instead of eyeballing hex: EXP lives
at `player[+0x2b]` (stage accumulator `+0x52`, per-kill score `e[+0xbb]`, lethal flag `+0x30`, action
state `+0x17`, position ints at `+1` and `+5` -- all from the disassembly comments in f.c).

  ent_fields.py --orig RUN/orig --port RUN/port --drive DRIVE \
      --cell g_player_ent --fields 0x17:b1,0x2b,0x52,0x30:b1,0xbb
Widths default to 4; suffix `:1`, `:2` or `:8` picks another. Values are printed per phase with the
side's maximum inside the phase, and any field whose phase-to-phase value went UP on one side but not
the other is flagged -- that is the shape of an award (EXP, score) that one game makes and the other
does not.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from diff_state import load_orig, load_port, phases_from_drive  # noqa: E402

RB = 64  # overridden per side from the dump's own rec_bytes


def parse_fields(spec):
    out = []
    for tok in spec.split(","):
        tok = tok.strip()
        if not tok:
            continue
        w = 4
        if ":" in tok:
            tok, ws = tok.rsplit(":", 1)
            w = int(ws)
        out.append((int(tok, 0), w))
    return out


def unpack(blob, off, w):
    if off + w > len(blob):
        return None
    if w == 1:
        return blob[off]
    if w == 2:
        return struct.unpack_from("<H", blob, off)[0]
    if w == 8:
        return struct.unpack_from("<Q", blob, off)[0]
    return struct.unpack_from("<I", blob, off)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orig", required=True)
    ap.add_argument("--port", required=True)
    ap.add_argument("--drive", required=True)
    ap.add_argument("--cell", required=True, help="pointer-cell name or DS address")
    ap.add_argument("--fields", required=True)
    ap.add_argument("--from", dest="t0", type=int, default=0)
    ap.add_argument("--to", dest="t1", type=int, default=1 << 40)
    a = ap.parse_args()

    fields = parse_fields(a.fields)
    want = int(a.cell, 16) if a.cell.lower().startswith("0x") else None

    orig, ocells = load_orig(a.orig)
    port, pcells = load_port(a.port)

    def find(cells):
        for i, c in enumerate(cells):
            if (want is not None and c["ds"] == want) or (want is None and c["name"] == a.cell):
                return i, c
        return None, None

    io, co = find(ocells)
    ip, cp = find(pcells)
    if co is None or cp is None:
        sys.exit(f"cell {a.cell} not dumped on "
                 f"{'both sides' if co is None and cp is None else ('original' if co is None else 'port')}")
    print(f"cell {co['name']} ds={hex(co['ds'])}  rec {co.get('rec_bytes')}B  "
          f"guest {co.get('guest_size')}B host {cp.get('host_size')}B ptr={cp.get('ptr')}")

    po = cp.get("rec_offset")
    if po is None:
        sys.exit("port cell carries no record block (not a pointer cell in the dump)")

    def samples(side):
        srt = sorted(side, key=lambda s: s[0])
        return srt

    os_, ps_ = samples(orig), samples(port)

    def per_phase(side, cellidx, recoff, heap_idx):
        srt = samples(side)
        rows = []
        for label, t0, t1 in phases:
            ins = [s for s in srt if t0 <= s[0] < t1]
            if not ins:
                prior = [s for s in srt if s[0] < t0]
                ins = [prior[-1]] if prior else []
            vals = []
            for s in ins:
                hb = s[heap_idx] if heap_idx is not None else s[1]
                rb = (ocells[cellidx].get("rec_bytes") if heap_idx is not None
                      else pcells[cellidx].get("rec_bytes")) or RB
                base = (sum(x.get("rec_bytes", RB) for x in ocells[:cellidx]) if heap_idx is not None
                        else recoff)
                blk = hb[base:base + rb]
                vals.append([unpack(blk, off, w) for off, w in fields])
            rows.append((label, vals))
        return rows

    phases = [p for p in phases_from_drive(a.drive) if p[2] > a.t0 and p[1] < a.t1]
    o_rows = per_phase(orig, io, None, 2)
    p_rows = per_phase(port, ip, po, None)

    hdr = "phase                   " + "".join(f"{('+'+hex(o)):>16s}" for o, _ in fields)
    print(hdr)
    prev = None
    for (label, ov), (_l, pv) in zip(o_rows, p_rows):
        omax = [max((v[i] or 0) for v in ov) if ov else None for i in range(len(fields))]
        pmax = [max((v[i] or 0) for v in pv) if pv else None for i in range(len(fields))]
        cells = "".join(f"{(f'{o:0x}/{p:0x}' if o is not None and p is not None else '-'):>16s}"
                        for o, p in zip(omax, pmax))
        mark = ""
        if prev is not None:
            for i in range(len(fields)):
                go = (omax[i] or 0) - (prev[0][i] or 0)
                gp = (pmax[i] or 0) - (prev[1][i] or 0)
                if (go > 0) != (gp > 0):
                    mark = "  <== one side awards, the other does not"
                    break
        print(f"{label:22s}{cells}{mark}")
        prev = (omax, pmax)


if __name__ == "__main__":
    main()
