#!/usr/bin/env python3
"""Reference sheet generator for the port loader self-test.

Reproduces scratch/sheets/*.png from raw .spr payloads, byte-for-byte.

Sheet rules (deduced from the original reference sheets, docs/03-formats.md §2):
  - grid of 150x150 cells, N_COLS columns; sprite i in cell (i % N_COLS, i // N_COLS)
  - cell fill = 128 (gray); sheet background = 128
  - cells that contain a sprite get a 1px border of value 60 around the full
    150x150 cell block (perimeter); empty cells have no border
  - each sprite is drawn at its cell origin + (1, 1)
  - palette = identity grayscale: pixel index v -> gray v (v=0 -> black)
  - segment `skip` runs leave the cell fill; segment pixel runs draw gray[v]
  - at most N_COLS*N_ROWS sprites are shown (first ones in file order)

Usage: python3 scripts/make_sheets.py [--spr NAME --cols C --rows R --out PATH] ...
Default: regenerate all five reference sheets into the given out dir.
"""
import argparse
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "..", "scratch", "res")

SHEETS = [
    ("pause", 6, 2),
    ("number1", 6, 2),
    ("ena-1", 6, 4),
    ("boss1", 6, 6),
    ("gun", 12, 12),
]
CELL = 150
FILL = 128
BORDER = 60
OFF = 10  # sprite offset inside cell


def load_sprites(path):
    d = open(path, "rb").read()
    n = struct.unpack_from("<I", d, 0)[0]
    off = 4
    out = []
    for _ in range(n):
        h = struct.unpack_from("<13I", d, off)
        sz = h[12]
        w, hh = h[0], h[1]
        pos = off + 52
        rows = []
        for _r in range(hh):
            nseg = d[pos]
            pos += 1
            row = bytearray([0] * w)  # row bg = black; segments draw over
            x = 0
            for _s in range(nseg):
                skip = d[pos]
                ln = d[pos + 1]
                pos += 2
                x += skip
                row[x:x + ln] = d[pos:pos + ln]
                x += ln
                pos += ln
            rows.append(bytes(row))
        out.append((w, hh, rows))
        off += 52 + sz
    assert off == len(d), f"{path}: consumed {off} of {len(d)}"
    return out


def render_sheet(sprites, n_cols, n_rows):
    W, H = n_cols * CELL, n_rows * CELL
    buf = bytearray([FILL] * (W * H))

    def put(x, y, v):
        if 0 <= x < W and 0 <= y < H:
            buf[y * W + x] = v

    # borders for cells that contain a sprite
    for i in range(min(len(sprites), n_cols * n_rows)):
        c, r = i % n_cols, i // n_cols
        x0, y0 = c * CELL, r * CELL
        for x in range(x0, x0 + CELL):
            put(x, y0, BORDER)
            put(x, y0 + CELL - 1, BORDER)
        for y in range(y0, y0 + CELL):
            put(x0, y, BORDER)
            put(x0 + CELL - 1, y, BORDER)
    # sprites
    for i in range(min(len(sprites), n_cols * n_rows)):
        c, r = i % n_cols, i // n_cols
        w, hh, rows = sprites[i]
        x0, y0 = c * CELL + OFF, r * CELL + OFF
        for y in range(hh):
            for x in range(w):
                put(x0 + x, y0 + y, rows[y][x])
    return bytes(buf)


def write_png_gray(path, w, h, data):
    raw = b"".join(b"\x00" + data[y * w:(y + 1) * w] for y in range(h))
    def chunk(typ, payload):
        c = typ + payload
        return struct.pack(">I", len(payload)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    open(path, "wb").write(png)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spr")
    ap.add_argument("--cols", type=int)
    ap.add_argument("--rows", type=int)
    ap.add_argument("--out")
    args = ap.parse_args()

    if args.spr:
        sprites = load_sprites(os.path.join(RES, args.spr + ".spr"))
        write_png_gray(args.out, args.cols * CELL, args.rows * CELL,
                       render_sheet(sprites, args.cols, args.rows))
        print("wrote", args.out)
        return

    for name, cols, rows in SHEETS:
        sprites = load_sprites(os.path.join(RES, name + ".spr"))
        out = os.path.join(HERE, "..", "scratch", "sheets", f"{name}_sheet.png")
        write_png_gray(out, cols * CELL, rows * CELL,
                       render_sheet(sprites, cols, rows))
        print("wrote", out)


if __name__ == "__main__":
    main()
