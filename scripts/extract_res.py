#!/usr/bin/env python3
"""Extract assets from NIGHT.RES (T.G Entertainment's resource container).

Container format (verified against the loader in NIGHT.EXE, FUN_000366a4 @ 0x000366a4):

    offset 0x00:  DWORD  count            (number of entries, e.g. 444)
    offset 0x004: count x 0x14 bytes, directory, obfuscated with a reverse XOR chain:
                   for i = len-1 .. 1:  dir[i] ^= dir[i-1]     (previous byte still encoded)
                   dir[0] ^= 0x20
    offset 0x4 + count*0x14:             contiguous data blobs (stored uncompressed)

    Each directory entry (0x14 bytes, little-endian):
        [0x00..0x0B]  name[12]   ASCII, NUL-padded (no extension > 8.3)
        [0x0C..0x0F]  size   DWORD
        [0x10..0x13]  offset DWORD  (absolute file offset; entries are packed back-to-back:
                                     offset[i] + size[i] == offset[i+1], last ends at EOF)

Usage:
    python3 scripts/extract_res.py list [-e EXT]
    python3 scripts/extract_res.py extract [-o OUTDIR] [NAME ...]
    python3 scripts/extract_res.py info NAME
"""
import argparse
import os
import struct
import sys

RES = os.path.join(os.path.dirname(__file__), "..", "NIGHT", "NIGHT.RES")


def read_directory(data: bytes):
    count = struct.unpack_from("<I", data, 0)[0]
    raw = bytearray(data[4 : 4 + count * 0x14])
    for i in range(len(raw) - 1, 0, -1):
        raw[i] ^= raw[i - 1]
    raw[0] ^= 0x20
    ents = []
    for i in range(count):
        e = raw[i * 0x14 : (i + 1) * 0x14]
        name = e[:12].split(b"\0", 1)[0].decode("ascii")
        size, offset = struct.unpack_from("<II", e, 0x0C)
        ents.append((name, offset, size))
    return ents


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["list", "extract", "info"])
    ap.add_argument("-e", "--ext", help="list: filter by extension (without dot)")
    ap.add_argument("-o", "--out", default="scratch/res", help="output dir (default scratch/res)")
    ap.add_argument("names", nargs="*", help="entry names (extract/info)")
    ap.add_argument("--res", default=RES)
    args = ap.parse_args()

    data = open(args.res, "rb").read()
    ents = read_directory(data)

    if args.cmd == "list":
        for name, off, size in ents:
            if args.ext and not name.endswith("." + args.ext):
                continue
            print(f"{name:16s} {off:#10x} {size:#10x} {size}")
        return

    by_name = dict((n, (o, s)) for n, o, s in ents)
    if args.cmd == "info":
        for n in args.names:
            if n not in by_name:
                print(f"!! not found: {n}", file=sys.stderr)
                continue
            off, size = by_name[n]
            print(f"{n}: offset={off:#x} size={size:#x}")
            print("  " + " ".join(f"{b:02x}" for b in data[off : off + 32]))
        return

    # extract
    todo = args.names or [n for n, _, _ in ents]
    os.makedirs(args.out, exist_ok=True)
    n = 0
    for name in todo:
        if name not in by_name:
            print(f"!! not found: {name}", file=sys.stderr)
            continue
        off, size = by_name[name]
        os.makedirs(os.path.join(args.out, os.path.dirname(name)), exist_ok=True)
        with open(os.path.join(args.out, name), "wb") as f:
            f.write(data[off : off + size])
        n += 1
    print(f"extracted {n} file(s) -> {args.out}")


if __name__ == "__main__":
    main()
