#!/usr/bin/env python3
"""Wrap the rig's raw CD-DA witness (port/cdda.raw, 44100 Hz stereo 16-bit signed little-endian,
the exact bytes the port handed the device for CD playback) in a WAV header, so diff_audio.py can
compare the port's *music* stream against the container's mixer capture. route.sh used to feed the
oracle port/pcm.wav only -- the board/SFX stream -- which is why every CD-DA track fingerprint came
back NOT FOUND in the original mix (docs/113)."""
import struct, sys
src, dst = sys.argv[1], sys.argv[2]
rate = int(sys.argv[3]) if len(sys.argv) > 3 else 44100
ch = int(sys.argv[4]) if len(sys.argv) > 4 else 2
data = open(src, 'rb').read()
n = len(data)
h = (b'RIFF' + struct.pack('<I', 36 + n) + b'WAVEfmt ' + struct.pack('<IHHIIHH', 16, 1, ch, rate,
       rate * ch * 2, ch * 2, 16) + b'data' + struct.pack('<I', min(n, 0x7fffffff)))
open(dst, 'wb').write(h + data)
print(f'{dst}: {n} bytes of {rate} Hz {ch}ch 16-bit = {n/(rate*ch*2):.1f}s')
