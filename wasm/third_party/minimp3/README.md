# minimp3 (vendored)

Minimalistic single-header MP3 (MPEG-1/MPEG-2 Layer III) decoder.

- Upstream: https://github.com/lieff/minimp3
- Pinned: `ea99364f61c14656440e8d77e9c233ccf3124633` (`master`, 2026-07-27)
- License: CC0-1.0 (public domain) -- see `LICENSE`
- Files taken: `minimp3.h` only. Do not edit; take a new upstream copy instead.

## Why this, and why only the low-level header

The browser port streams CD-DA out of compressed bytes instead of 2352-byte Red Book frames, and it
needs a decoder that (a) is C, so it compiles into the same wasm module as the game layer, (b) decodes
*one frame from bytes I hand it*, because the bytes come from HTTP Range windows and not from a seekable
FILE, and (c) does not need to know the whole file to start. `mp3dec_decode_frame()` is exactly that:
feed it a buffer, get 1152 samples and the number of bytes consumed.

`minimp3_ex.h` is deliberately not vendored. Its `mp3dec_ex` reader builds a seek table by scanning the
whole stream to support VBR seeks, which for this port means downloading every track to hear one scene
-- the thing docs/460 exists to prevent. Seeking here is analytic (constant-bitrate byte arithmetic, see
`wasm/src/wasm_mp3.c`), so the low-level decoder is the whole requirement.

Compile it in exactly one translation unit, with `MINIMP3_IMPLEMENTATION` defined before the include.
