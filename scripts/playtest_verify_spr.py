#!/usr/bin/env python3
"""Independent check of the game's row-RLE sprite composites (docs/63 basis).

The port (NIGHT_MENUBG=1) prints, for every pause-menu sprite cell it composites, the cell's raw row-RLE
stream and the bytes the blit actually wrote on the back screen.  This script decodes that stream with its OWN
implementation of the format the shipped blitter uses (fn_03bb42 mode 0, src/f/f.c:7104, mirroring
0x0003bb42):

    per row:   N                       -- number of runs
    per run:   colAdvance, runLen, runLen literal bytes
    column position accumulates colAdvance + runLen

and compares the decoded pixels against what the port wrote, at every column the cell covers.  Columns the
cell does not cover are the backdrop underneath and are not compared.  A mismatch means the port's sprite
decode or placement is wrong; a clean run means the on-screen glyph is exactly what the shipped data says.

Usage: python3 scripts/playtest_verify_spr.py <run log>
"""
import re
import sys
from collections import defaultdict


def decode_rows(stream: bytes, rows: int):
    """Yield one dict {column: byte} per row, decoding the row-RLE stream independently."""
    p = 0
    for _ in range(rows):
        if p >= len(stream):
            yield {}
            continue
        n = stream[p]
        p += 1
        row = {}
        acc = 0
        for _ in range(n):
            if p + 2 > len(stream):
                break
            adv = stream[p]
            ln = stream[p + 1]
            p += 2
            data = stream[p:p + ln]
            p += ln
            for j, b in enumerate(data):
                row[acc + adv + j] = b
            acc += adv + ln
        yield row


def main() -> int:
    path = sys.argv[1]
    per = defaultdict(lambda: [0, 0, 0])          # tag -> [compared, matched, mismatched]
    cur = None
    for line in open(path, errors="replace"):
        m = re.search(r"menucell tag=(\w+) k=(\d+) dest=(0x[0-9a-f]+) srow=(-?\d+) col=(\d+) rows=(\d+) stream=([0-9a-f]+)", line)
        if m:
            cur = (m.group(1), int(m.group(2)), int(m.group(5)), int(m.group(6)), int(m.group(3), 16), bytes.fromhex(m.group(7)))
            continue
        m = re.search(r"menupix k=(\d+) r=(\d+) ([0-9a-f]*)", line)
        if not m or cur is None:
            continue
        tag, k, col, rows, dest, stream = cur
        r = int(m.group(2))
        got = bytes.fromhex(m.group(3))
        dec = list(decode_rows(stream, rows))
        if r >= len(dec):
            continue
        row = dec[r]
        st = per[tag]
        # The shipped blitter clamps to the screen: `right = 0x140 - dest_off` (f.c fn_03bb42, mirroring
        # 0x0003bb42). Columns past that are backdrop it never writes, so they are not comparable -- this is
        # why stage 5's player cell (dest 0x146, col 53, i.e. dest+col 379 > 320) "mismatched" before.
        right = 0x140 - dest
        for c, b in enumerate(got):
            if c in row and c < right:
                st[0] += 1
                if row[c] == b:
                    st[1] += 1
                else:
                    st[2] += 1
    tot = [sum(v[i] for v in per.values()) for i in range(3)]
    for tag, (cmp_, ok, bad) in sorted(per.items()):
        print(f"  {tag:<8} compared {cmp_:7d}  matched {ok:7d}  mismatched {bad:5d}")
    print(f"  TOTAL    compared {tot[0]:7d}  matched {tot[1]:7d}  mismatched {tot[2]:5d}"
          f"  -> {tot[1] / max(1, tot[0]):.4f}")
    return 0 if tot[2] == 0 and tot[0] > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
