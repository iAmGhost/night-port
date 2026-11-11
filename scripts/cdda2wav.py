#!/usr/bin/env python3
"""Extract the 14 CD-DA audio tracks from TG_NIGHT.IMG (CUE: MODE1/2352) to WAV.

The IMG is a 2352-byte-sector binary image. Audio track N (CUE track N) starts
at its INDEX 01 LBA. 2352 -> 2324 usable PCM bytes per sector (16-bit stereo).
"""
import os as _os, sys as _sys     # aliases: this file's own imports come below
_sys.path.insert(0, _os.path.join(_os.path.dirname(_os.path.realpath(__file__))))
from disc_loc import find_cue as _find_cue, find_img as _find_img   # located by suffix (docs/455)

import re, struct, sys, os

CUE = str(_find_cue())
IMG = str(_find_img(CUE))
OUT = "scratch/cdda"

def msf2lba(m, s, f):
    return m * 60 * 75 + s * 75 + f

def main():
    cue = open(CUE).read()
    img = open(IMG, "rb").read()
    sectors = len(img) // 2352
    print(f"IMG: {len(img)} bytes = {sectors} x 2352B sectors")

    tracks = []
    cur = None
    for line in cue.splitlines():
        m = re.match(r"TRACK (\d+) (\w+)", line.strip())
        if m:
            cur = {"no": int(m.group(1)), "mode": m.group(2), "index": None}
            tracks.append(cur)
        m = re.match(r"INDEX 01 (\d+):(\d+):(\d+)", line.strip())
        if m and cur is not None:
            cur["index"] = msf2lba(int(m.group(1)), int(m.group(2)), int(m.group(3)))

    os.makedirs(OUT, exist_ok=True)
    for i, t in enumerate(tracks):
        if t["mode"] != "AUDIO":
            print(f"track {t['no']:2d}: MODE1 (data) — skip")
            continue
        start = t["index"]
        end = tracks[i+1]["index"] if i+1 < len(tracks) else sectors
        nsec = end - start
        pcm = bytearray()
        off = start * 2352
        for _ in range(nsec):
            pcm += img[off+16:off+2340]
            off += 2352
        pcm = bytes(pcm)
        dur = len(pcm) / (44100 * 4)
        wav = os.path.join(OUT, f"track{t['no']:02d}.wav")
        with open(wav, "wb") as f:
            f.write(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " + struct.pack("<I", 16))
            f.write(struct.pack("<HHIIHH", 1, 2, 44100, 44100*4, 4, 16))
            f.write(b"data" + struct.pack("<I", len(pcm)) + pcm)
        print(f"track {t['no']:2d}: LBA {start:6d}..{end:6d} ({nsec:5d} sec) "
              f"{dur:7.2f}s -> {wav} ({os.path.getsize(wav):9d} B)")

if __name__ == "__main__":
    main()
