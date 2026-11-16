"""CD-DA to WAV, so a human can listen to what the tool cut.

A raw CD-DA frame carries no header and needs none: all 2352 bytes are little-endian signed 16-bit
stereo PCM at 44.1 kHz -- 588 sample frames a second, which is why `sectors * 2352` is a track's byte
length and why `sectors / 75` is its seconds (docs/455).  A WAV of a CD-DA track is therefore a 44-byte
RIFF/WAVE header followed by the same bytes the slot file holds, which is also the point: if the two
disagree, it is the slicing, and a WAV says so in the one language a person can check -- their ears.

The header is written by hand rather than through `wave` because `wave` wants to seek back over the data
it has already written, and the data here is streamed out of a 633 MB image through a pipe-shaped reader.
"""
from __future__ import annotations

import hashlib
import struct
from pathlib import Path

from .disc import SECTOR

RIFF = b"RIFF"
WAVE = b"WAVE"
FMT = b"fmt "
DATA = b"data"


def wav_header(sectors: int) -> bytes:
    """The canonical PCM WAV header for `sectors` of CD-DA: 44.1 kHz, 2 channels, 16 bit."""
    pcm = sectors * SECTOR
    return (RIFF + struct.pack("<I", 36 + pcm) + WAVE
            + FMT + struct.pack("<IHHIIHH", 16, 1, 2, 44100, 44100 * 4, 4, 16)
            + DATA + struct.pack("<I", pcm))


def write_wav(dst: Path, src, off: int, sectors: int, force: bool = False, progress=None) -> tuple[int, str]:
    """Write one track's WAV -- header then `sectors` raw frames -- returning (bytes, sha256).

    `src` is any seekable handle over the image, and `off` a byte offset in it, so this is the same slice
    `part_audio` writes as a `.bin`, seen through a file a media player will open.
    """
    dst.parent.mkdir(parents=True, exist_ok=True)
    tmp = dst.with_suffix(dst.suffix + ".part")
    h = hashlib.sha256()
    done = 0
    total = sectors * SECTOR
    with tmp.open("wb") as g:
        g.write(wav_header(sectors))
        src.seek(off)
        while done < total:
            block = src.read(min(1 << 20, total - done))
            if not block:
                raise EOFError(f"short read at byte {off}: {total - done} bytes missing")
            g.write(block)
            h.update(block)
            done += len(block)
            if progress:
                progress(done / total)
    tmp.replace(dst)
    return total, h.hexdigest()


def seconds(sectors: int) -> float:
    return sectors / 75.0
