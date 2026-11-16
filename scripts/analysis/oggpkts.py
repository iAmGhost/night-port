#!/usr/bin/env python3
"""Split an Ogg stream into packets and report their sizes.

CBR is the whole reason Opus would be as easy to seek as MP3, so the first question about a libopus
encode at -vbr off is whether the packets are really all the same size. 40 s of music at 20 ms frames
is 2000 packets; anything other than one distinct size means the byte clock would need a table.
"""
import sys
from collections import Counter


def packets(path):
    d = open(path, "rb").read()
    i = 0
    cur = b""
    while i + 27 <= len(d):
        if d[i:i + 4] != b"OggS":
            raise SystemExit(f"not an Ogg stream at {i}")
        seg = d[i + 26]
        lace = d[i + 27:i + 27 + seg]
        body = i + 27 + seg
        tot = sum(lace)
        j = body
        for v in lace:
            if v == 255:
                cur += d[j:j + 255]
                j += 255
            else:
                cur += d[j:j + v]
                j += v
                yield cur
                cur = b""
        i = body + tot
    if cur:
        yield cur


if __name__ == "__main__":
  for path in sys.argv[1:]:
    pk = list(packets(path))
    c = Counter(len(p) for p in pk)
    gran = 0
    print(f"{path}: packets={len(pk)} bytes={sum(len(p) for p in pk)} "
          f"distinct_sizes={c.most_common(4)}")
