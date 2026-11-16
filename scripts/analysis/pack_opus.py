#!/usr/bin/env python3
"""Repack a CBR Ogg/Opus encode into the fixed-stride container the port would read.

Why not just ship the Ogg: the whole reason Opus was worth trying is that CBR MP3 let the port compute
"which byte holds sample S" without a seek table, and Ogg throws that away for page headers it puts
between the samples. Repacking costs the encoder's padding, nothing else: every packet becomes exactly
`packet_bytes`, so sample -> byte stays one line of arithmetic. Fails loudly if the encode was not
constant bitrate, because then the claim it rests on is false.
"""
import struct
import sys

from oggpkts import packets

MAGIC = b"NKOP"
HEADER = "<4s8I"


def main(opus_path, out_path, frame_samples, sample_rate):
    pk = list(packets(opus_path))
    assert pk[0][:8] == b"OpusHead" and pk[1][:8] == b"OpusTags", "unexpected stream headers"
    version, channels, pre_skip = struct.unpack_from("<BBH", pk[0], 8)
    pk = pk[2:]                                  # the audio, not the container's own metadata
    sizes = {len(p) for p in pk[:-1]}          # the last packet may be short
    if len(sizes) != 1:
        raise SystemExit(f"not constant bitrate: {sorted(sizes)[:6]} ...")
    n = sizes.pop()
    with open(out_path, "wb") as f:
        f.write(struct.pack(HEADER, MAGIC, 1, n, frame_samples, sample_rate, channels,
                            len(pk), pre_skip, 0))
        for p in pk:
            f.write(p.ljust(n, b"\0"))
    dur = len(pk) * frame_samples / sample_rate
    print(f"{out_path}: {len(pk)} packets of {n}B, {frame_samples} spf, {sample_rate} Hz, "
          f"{channels} ch, pre_skip={pre_skip}, {dur:.1f}s audio, "
          f"{n * 50 * 8 / 1000:.0f} kbps payload")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]))
