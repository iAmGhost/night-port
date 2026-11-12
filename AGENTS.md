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
  What a boot means changes with: `NIGHT_RES_DIR` (resources), `NIGHT_CD_DIR` (per-track files vs the image),
  `NIGHT_NO_DISC=1` (drive treated as EMPTY -- run silent on the compiled-in TOC), `NIGHT_REQUIRE_DISC=1`
  (nothing substituted for an absent disc -- `cdinsert.pcx`; audits set it without `NIGHT_NO_DISC`),
  `NIGHT_CD_AUDIT` (record every disc read). docs/455 has the four combinations.
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
  the sample-rate converter without a browser. docs/457 is the write-up.

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