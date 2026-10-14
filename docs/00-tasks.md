# Analyzation
- [x] Executable format & toolchain (MZ+LE 32-bit, Watcom C/C++, DOS/4G) → `docs/01-executable.md`
- [x] Container: `NIGHT.RES` format cracked, 444 assets extracted → `docs/02-container.md`, `scripts/extract_res.py`
- [x] Graphics & data formats decoded byte-exact: `.pcx` `.spr` `.act` `.stg` `.wal` `.chr` `.key` → `docs/03-formats.md`
- [x] Sound: Creative-VOC-style 8-bit mono samples, SB16 DMA ch1 playback, sample→event map → `docs/04-sound.md`, `scripts/parse_voc.py` (100/100 verified)
- [x] CD-DA music: MSCDEX INT 0x2E (play/TOC/identity check), 14 tracks extracted to WAV → `docs/06-cdda.md`, `scripts/cdda2wav.py`
- [x] Game Logic (startup/`main`, game loop, combat, EXP/근성치) + all `docs/03-formats.md` §10 open items resolved → `docs/05-logic.md`

# Porting
- [x] SDL3 + C skeleton (window, 640×480 ARGB framebuffer, input, selftest) → `port/`, `docs/06-port.md`
- [x] Audio backend: VOC SFX (1.voc on punch/kick) + CD-DA WAV music (tracks 1/9/10)
- [x] Asset pipeline: `NIGHT.RES` container decoded in C (444 entries), `.pcx`/`.spr`/`.act`/`.stg`/`.wal`/`.key`/`.voc`/`.fli` decoders, selftest byte-exact
- [x] Opening animation (`opening.fli`, custom 128B-header FLI, 1168 frames) + menu (scrolling `title1.pcx`) → screenshot evidence `scratch/shots/`
- [x] Stage 1 vertical slice: `stage1.pcx` background, `.stg`/`.act`/`.spr` entities (10 spawned), keyboard input, VOC SFX, CD track 10 → `port/src/stage1.c`, evidence `scratch/shots/stage1_f0100.png` + `stage1_demo.log`
