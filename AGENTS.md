# Goal

- Create modern SDL port of game.

# Analysis first

- Write analysis document when specific goal is achieved- so your work can be understood and reproduced.

# Paths

- ./TG_NIGHT.CUE, ./TG_NIGHT.IMG: Original CD Image (git-ignored; the scripts find the disc by suffix --
  the current directory, then a `NIGHT/` under it -- so a rip under any name works: docs/455)
- ./scratch: regenerable caches only (extracted assets, decoded frames, captured runs); anything you can rebuild
  with ./scripts. Analysis work and evidence live in ./scripts and ./analysis instead.
- ./extracted: the original game's files, and what `make extract` builds -- the install set (NIGHT.EXE,
  NIGHT.RES), TG_NIGHT.CUE and one raw-frame file per CD-DA track, so the port runs with no disc image at
  all (`./build/night` with no arguments; `scripts/extract_disc.py` and `night-extractor` are the same tool).
  570 MB, git-ignored, regenerable in ~3 min; docs/455 is the write-up and `make extract-verify` re-hashes it.
  `uv run night-extractor --data PATH...` takes any shape a person owns -- a pair, a rip directory, a zip,
  loose fan files, a bare image, NIGHT.RES alone -- and `scripts/analysis/check_input_shapes.py` is the shape
  test. `--wav` writes each cut track as 44.1 kHz WAV beside its .bin, which is how a fan repack's music gets
  judged by ear rather than by hash: its audio matches this disc byte for byte on 5 of 14 tracks and drifts
  inside the rest (the table, and the not-a-multiple-of-2352 arithmetic that rules out a dropped frame, are in
  docs/466 -- the extractor, byod.js and the Pocket's repack.py each print which kind they got). The Pocket's
  `scripts/pocket/repack.py` reads the same input layer and hands `gen_disc.py` a `scratch/pocket/disc.json`
  (the cue the card was cut with plus the disc's own geometry, never the rip's); `check_slots.py --dir DIR`
  checks either, and skips the byte comparison, for a fan tree.
  What a boot means changes with: `NIGHT_RES_DIR` (resources), `NIGHT_CD_DIR` (per-track files vs the image),
  `NIGHT_NO_DISC=1` (drive treated as EMPTY -- run silent on the compiled-in TOC), `NIGHT_REQUIRE_DISC=1`
  (nothing substituted for an absent disc -- `cdinsert.pcx`; audits set it without `NIGHT_NO_DISC`),
  `NIGHT_CD_AUDIT` (record every disc read). docs/455 has the four combinations.
  It is not required either: a directory holding only a CUE sheet and its image boots the game -- `--cue FILE`,
  else any `*.cue` there (TG_NIGHT.CUE first, then one whose image exists), else any in the current directory;
  NIGHT.RES is then read out of the disc's installer payload (`NIGHT/NIGHT.DAT`) at open time by
  `src/cd/cd_res.c` through `sys_file_open` (docs/465, asserted against `extracted/NIGHT.RES` in `make test`).
- ./scripts: Store generated scripts here(for extracting graphic, audio, etc...); scripts/ref holds the python
  differential reference models the fixture suite compares against, scripts/gen the C/vector generators,
  scripts/analysis the per-question analysis tools, scripts/rig the differential rig.
- ./analysis: the evidence docs cite -- analysis/disasm (disassembly and Ghidra dumps), analysis/notes (porting
  specs and notes), analysis/probes (the small C repro programs), analysis/tables, analysis/refs (cited
  screenshots), analysis/legacy (run logs a doc still names).
- ./docs: Documents about analysis and porting
- ./src: the port's C sources (src/f the function-to-function layer, src/app the SDL host, src/res, src/cd,
  src/sys, src/gen)
- ./test: the fixture suite's sources (test/test_f1.c, test/vectors the differential vector headers)
- ./build: every build output (test_f1, night, night-asan, night-asan-o2, night-census, selftest), git-ignored.
  `make test`, `make app`, `make asanapp`, `make selftest` run from the repo root.
- ./wasm: the browser port -- SDL-free host (wasm/src/main_wasm.c) over the same game layer compiled in
  place, a PCM-ring/key-block contract in wasm/src/wasm_block.h, CD-DA over HTTP Range reads. Needs ~/emsdk
  (`emcc`). `make -C wasm` builds, `make -C wasm serve` serves on 8091, `make -C wasm check` stages data,
  drives headless Chrome with injected keys and asserts frames/disc/audio/keys, `make -C wasm resample` tests
  the sample-rate converter without a browser. docs/457 is the write-up. Music can come from MP3 instead of
  Red Book sectors: `make -C wasm audio` encodes it (546 MB of CD-DA to 50 MB) and writes
  `dist/data/audio/tracks.tsv`, which `wasm/src/wasm_mp3.c` reads to answer the game's sector reads;
  `make -C wasm audio-check` drives that reader with MSCDEX-shaped reads against the sectors it replaces, and
  `?audio=0` turns the path off. docs/463 is the write-up, including the harness bug (`Number('')` is 0, so an
  unset `NIGHT_MUTE` parsed as "mute at t=0") that had every `make check` run muting the page it was testing.
  Music is **pre-downloaded**, not streamed: the gate holds nothing back but the engine, says what the press
  will cost (HEAD for the resource pack, `tracks.tsv` for the music), and downloads both after the press.
  The gate's one box, `play the music (47 MB)`, is ticked; unchecked the game runs with the
  soundtrack as silence and asks for no music byte at all (`--no-music`, `nk_disc_silent_audio` -- *not*
  `--no-audio`, which streams raw sectors and costs more). It is a choice about **hosted bytes**, so it is
  hidden while a game-data file is in play -- there the file decides, silence means a file with no image, and
  `?music=0` remains the lever for a run that wants silence (`make check` asserts the box is offered,
  `check-byod` that it is not). The HUD reports `KB music` (bytes the game
  consumed) and `KB network` (bytes the wire carried) separately, because they stopped being the same number
  the moment silence and pre-download existed. `make -C wasm check-cache` runs the cache twice in one Chrome
  profile and requires the warm pass to fetch zero bytes.
  The port also runs off **the visitor's own game data**, so a host can ship code and no game bytes
  (docs/464 -- archive.org will not hand bytes to a page; docs/465 for the disc's installer payload).
  `wasm/runtime/byod.js` reads a container, which is **a zip or the files picked together** (`multiple`):
  NIGHT.RES alone; a CUE and its IMG/BIN/ISO; a fan repack's MUSIC.RES/SOUND.RES. Names are the visitor's
  own -- the cue's own `FILE` line picks the image, `TG_NIGHT.*` and the repack's names are only ties -- and
  fan vs disc is decided by what the image *holds* -- `!(volume && res)`, not by a name and not by the
  volume alone: the Internet Archive repack's MUSIC.RES carries the disc's 174 header sectors and none of its
  files, and treating it as a disc slices every track 560 bytes late (docs/466). A picked image declares no
  CRC, so `measureCrc` reads it once, or the fan corrections stay off; a zip member's CRC is the declared one
  and must never be re-measured (hashing a deflated member hashes its compressed bytes).
  No CRC is a condition of loading anything -- neither a container's declared one nor the reference
  `0xf578a854`: a mismatch is a log line that says so and the port loads the file anyway (only a
  member shorter than its declared length fails, in both the browser and the extractor). A bare image of the disc's size falls back to
  `discCue()`, the JS twin of `--cue-toc`. The disc answers reads from `--cd-url nklocal:cd`; `--cd-url ''`
  means no audio byte source at all, and `--cue-toc` writes the compiled-in geometry out as the sheet
  `cd_iso` would have parsed (`cd_iso_toc_cue()`, asserted byte-for-byte against the shipped cue). With no
  `data/` staged on the server the gate offers only the file, and arms the press once one is chosen.
  `make -C wasm check-byod [Z=<file(s)>]` drives that run (DevTools file injection; quote several paths in
  `Z`), and `node wasm/scripts/byod-test.js <file> [more...]` checks the slicing and the payload reader --
  it also runs the same pair renamed and a bare image, against the rip at the repo root.
  The gate is Korean and carries the game's own title, 어쩐지 좋은 일이 생길 것 같은 저녁 (TG Entertainment
  1997; the IA slug `eojjeonji` is that title romanised), with `시작`, `데이터 파일 선택`, and an Internet
  Archive download link (`eojjeonji-definitive-for-dosbox`). The log panel and the HUD stay English,
  because `make check` and `check-byod` assert against their lines. docs/465 records where the split
  falls and why.
  URL levers: `?music=0/1`, `?cache=0/clear`, `?preload=0` (stream the MP3 instead of downloading it --
  harness and device reports only, no UI), `?audio=0`, `?resample=linear`, `?spbuf=4096`. The bottom bar
  toggles on a press on the game picture only (`canvas.getBoundingClientRect()`), never on the letterbox or
  the deck -- so a thumb that misses the virtual pad cannot hide the HUD; `make check` presses both points and
  asserts it, printing a `bar:` line.
  `make check` ends with a `page audio:` line (mixer's own µs per second of audio, missed deadlines, device
  sample rate, block size): phones answer 48 kHz while the music ring is 44.1, so the music is sinc-converted
  on the main thread there and only a phone can say what that costs. URL levers for a device report:
  `?audio=0`, `?preload=1/0`, `?resample=linear`, `?spbuf=4096`. `NIGHT_NET="offline=A-B"` in the harness drops
  the network mid-run; a 22-second dropout wedges the tab in streamed MP3 mode and is invisible in preload
  mode, which is the reason preload exists. Edit CFLAGS and the build rebuilds -- `$(OBJ_DIR)/.flags` carries
  them, because nothing else says a flag is an input and an -O3 A/B silently compares a build with itself.

# Tools

- use uv for python
- use `dosbox-staging` for running original game
- **for capturing the original's frames, use `dosbox-automation` through a Linux container: it is the only route
  that works headless on this machine** (see "Capturing the original's frames" below; docs/46 records the seven
  routes that do not work)
- **Debugging tools (e.g. `ldbg`): always add an arbitrary timeout** so a watch/step/continue cannot loop or block forever. Never run an interactive debugger session unbounded -- cap it (e.g. a timeout wrapper / max-steps / alarm) and recover from the timeout instead of hanging.

## Capturing the original's frames (solved: dosbox-automation + REST API in a container)

Getting frames out of the original for screenshot comparison failed on every route tried with `dosbox-staging` and
`dosbox-x` (docs/46 measures seven dead routes: `screencapture` returns black, no window is visible to System
Events from a non-GUI shell, staging 0.83 has no capture command at all, no file-backed serial route, and guest
TSRs are unreachable or crash the emulator). It works with **dosbox-automation**, which has a built-in HTTP REST
API whose frame/video endpoints do not depend on a window or a hotkey.

There is **no macOS build** (releases ship Linux tarballs/AppImages and Windows zips; `brew` only offers staging
and dosbox-x), and the Linux binary will not run under Rosetta directly, so run the linux-x86_64 tarball in an
amd64 container (OrbStack/Docker is available on this machine):

```sh
# 1. emulator (0.85.1) + a prepared image (one apt install, then docker commit so it is never repeated)
curl -sL -o /tmp/dbgauto/da.tar.xz \
  https://github.com/dosbox-automation/dosbox-automation/releases/download/v0.85.1/dosbox-automation-0.85.1-7567ec4-linux-x86_64.tar.xz
docker run -d --platform linux/amd64 --name dbga -v /tmp/dbgauto:/work -p 8386:8386 \
  -e DOSBOX_API_TOKEN=<64-hex-token> debian:bookworm-slim sleep 7200
docker exec dbga apt-get install -y --no-install-recommends \
  xvfb xauth curl libgl1 libasound2 libsdl2-2.0-0 libsdl2-net-2.0-0   # xauth is required by xvfb-run
docker commit dbga dbga:ready

# 2. run NIGHT: the game must be mounted UNDER the conf file's directory, because with the webserver on,
#    MOUNT only accepts paths inside the config's parent tree (mount_allowed_bases gates the rest)
docker run -d --platform linux/amd64 --name dbga -v /tmp/dbgauto:/work \
  -v $HOME/Git/night-port:/work/repo:ro -p 8386:8386 \
  -e DOSBOX_API_TOKEN=<same-token> dbga:ready sleep 7200
docker exec -d dbga bash -c 'cd /work/dosbox-automation-* && SDL_AUDIODRIVER=dummy \
  xvfb-run -a -s "-screen 0 1024x768x24" ./dosbox-automation -conf /work/night.conf > /work/run.log 2>&1'
```

with `night.conf` containing at least:

```ini
[webserver]
webserver_enabled = on
webserver_osd = off          ; keep the automation overlay out of captured frames
[sdl]
output = texture             ; still needs X: run it under xvfb-run, SDL_VIDEODRIVER=dummy aborts in GFX_InitSdl
[cpu]
cycles = max
[dosbox]
machine = svga_s3
memsize = 16
[autoexec]
mount C /work/repo/extracted
imgmount D /work/repo/TG_NIGHT.CUE -t iso
C:
```

then drive it from the host, no window involved:

```sh
curl -s -H "Authorization: Bearer $TOKEN" http://localhost:8386/api/v1/status
curl -s -H "Authorization: Bearer $TOKEN" http://localhost:8386/api/v1/video/frame/info
curl -s -H "Authorization: Bearer $TOKEN" "http://localhost:8386/api/v1/video/frame?format=png" -o frame.png
curl -s -H "Authorization: Bearer $TOKEN" "http://localhost:8386/api/v1/video/frame?format=raw" -o frame.raw
```

Measured on this machine: `/api/v1/status` answers `{"running":true,"is_shell":true,...}` and `video/frame` returns
a genuine PNG of the emulated screen (720x400 for text mode 3), with the log showing the VGA mode. Useful extras
documented upstream: `format=raw` returns a header (width/height/pitch/palette) plus index pixels, i.e. directly
comparable against the port's arena bytes; `format=rendered` shows what a window would show; lossless ZMBV video
recording for frame-by-frame pacing; input injection and drive-swap endpoints; `/api.html` serves a live Swagger
UI and `/openapi.json` the schemas.

Consequence: port-vs-original screenshot comparison is possible without a GUI session, so the comparison basis in
docs/63 (independently decoded shipped assets) can be checked against real original frames.

## Running original game

Run command below on dosbox for running original game

```bat
MOUNT C ~/Git/night-port/extracted
MOUNT D ~/Git/night-port/TG_NIGHT.CUE
C:
NIGHT
```

# How to analysis

- Use ghidra mcp for analysing NIGHT.EXE