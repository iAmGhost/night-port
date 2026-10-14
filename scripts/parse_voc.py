#!/usr/bin/env python3
"""Verify every .voc in scratch/res against the spec in docs/04-sound.md.

Spec (re-derived from NIGHT.EXE playback path and the 100-file corpus):
  [26-byte header]
    0x00  19-byte ASCII "Creative Voice File" (NO null terminator)
    0x13  7 bytes, uniform across the corpus: 1a 1a 00 0a 01 29 11
          (decorative; the loader just seeks 26 bytes past the header)
  [tag stream]
    tag u8, len u24 LE, payload[len]
    0x00  stop (lone 0x00 byte, always the final byte of the file)
    0x01  voice:      [u8 handler][u8 rate_idx][8-bit unsigned mono PCM x (len-2), silence = 0x80]
    0x02  retrigger:  same layout; reuses previous handler, rate_param-1 (clamped 0x14)
    0x08  reserved:   4-byte constant payload 54 e9 00 01; skipped by the game
    other  skipped by the game dispatcher (none in the corpus)

Corpus facts: 106 voice blocks, 8 retrigger-less tag-8 blocks in 3 files,
handler bytes in {0x83,0x9c,0xa5,0xa6,0xe9}, rate_idx always 0.
"""
import glob, sys

SIG = b"Creative Voice File"
HDR_TAIL = bytes.fromhex("1a1a000a012911")
TAG8_PAYLOAD = bytes.fromhex("54e90001")
HANDLERS = {0x83, 0x9c, 0xa5, 0xa6, 0xe9}

def parse(data: bytes, path: str, errors: list[str]) -> dict:
    st = {"voice_blocks": 0, "pcm_bytes": 0, "handlers": set()}
    if data[:19] != SIG:
        errors.append(f"{path}: bad signature")
        return st
    if data[19:26] != HDR_TAIL:
        errors.append(f"{path}: header tail {data[19:26].hex()} != {HDR_TAIL.hex()}")
    if len(data) < 27:
        errors.append(f"{path}: too small")
        return st
    off = 26
    n = 0
    while off < len(data):
        tag = data[off]
        if tag == 0x00:
            if off != len(data) - 1:
                errors.append(f"{path}: stop tag at {off:#x}, not last byte")
            break
        if off + 4 > len(data):
            errors.append(f"{path}: truncated tag header at {off:#x}")
            break
        ln = data[off+1] | (data[off+2] << 8) | (data[off+3] << 16)
        if off + 4 + ln > len(data):
            errors.append(f"{path}: tag {tag:#x} at {off:#x} overruns file (len {ln:#x})")
            break
        if tag == 0x01:
            if ln < 2:
                errors.append(f"{path}: voice block too short ({ln})")
            h = data[off+4]
            st["voice_blocks"] += 1
            st["pcm_bytes"] += ln - 2
            st["handlers"].add(h)
            if h not in HANDLERS:
                errors.append(f"{path}: new handler byte {h:#x}")
            if data[off+5] != 0:
                errors.append(f"{path}: rate_idx != 0 ({data[off+5]})")
        elif tag == 0x02:
            if ln < 2:
                errors.append(f"{path}: retrigger block too short ({ln})")
            st["voice_blocks"] += 1
            st["pcm_bytes"] += ln - 2
        elif tag == 0x08:
            if ln != 4 or data[off+4:off+8] != TAG8_PAYLOAD:
                errors.append(f"{path}: tag 8 payload {data[off+4:off+8].hex()}")
        else:
            errors.append(f"{path}: unexpected tag {tag:#x} at {off:#x}")
        off += 4 + ln
        n += 1
    else:
        errors.append(f"{path}: no stop tag (EOF in tag stream)")
    return st

def main():
    files = sorted(glob.glob("scratch/res/*.voc"))
    errors = []
    total_pcm = 0
    total_blocks = 0
    for f in files:
        st = parse(open(f, "rb").read(), f, errors)
        total_pcm += st["pcm_bytes"]
        total_blocks += st["voice_blocks"]
    print(f"files: {len(files)}")
    print(f"voice blocks: {total_blocks}")
    print(f"total 8-bit PCM samples: {total_pcm}")
    if errors:
        print(f"FAILURES: {len(errors)}")
        for e in errors:
            print("  " + e)
        sys.exit(1)
    print("OK: all .voc parse per spec, 0 failures")

if __name__ == "__main__":
    main()
