# Goal

- Create modern SDL port of game.

# Analysis first

- Write analysis document when specific goal is achieved- so your work can be understood and reproduced.

# Paths

- ./NIGHT: Original game.
- ./NIGHT/TG_NIGHT.CUE, ./NIGHT/TG_NIGHT.IMG: Original CD Image
- ./scratch: regenerable caches only (extracted assets, decoded frames, captured runs); anything you can rebuild
  with ./scripts. Analysis work and evidence live in ./scripts and ./analysis instead.
- ./scripts: Store generated scripts here(for extracting graphic, audio, etc...); scripts/ref holds the python
  differential reference models the fixture suite compares against, scripts/gen the C/vector generators,
  scripts/analysis the per-question analysis tools, scripts/rig the differential rig.
- ./analysis: the evidence docs cite -- analysis/disasm (disassembly and Ghidra dumps), analysis/notes (porting
  specs and notes), analysis/probes (the small C repro programs), analysis/tables, analysis/refs (cited
  screenshots), analysis/legacy (run logs a doc still names).
- ./docs: Documents about analysis and porting
- ./src: the night_port python package the uv project installs (the scripts run under `uv run`)
- ./port: the SDL port. Sources live in port/src and port/test (test sources + port/test/vectors); every build
  output lands in port/build, which is git-ignored.

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
  -v $HOME/Git/night-port/NIGHT:/work/game:ro -p 8386:8386 \
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
mount C /work/game
imgmount D /work/game/TG_NIGHT.CUE -t iso
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
MOUNT C ~/Git/night-port/NIGHT
MOUNT D ~/Git/night-port/NIGHT/TG_NIGHT.CUE
C:
NIGHT
```

# How to analysis

- Use ghidra mcp for analysing NIGHT.EXE