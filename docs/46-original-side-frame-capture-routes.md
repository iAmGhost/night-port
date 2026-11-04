
## Route 6 (this cycle): guest-side INT 10h snapshot TSR -- also dead, and it kills the emulator

The insight worth keeping, because it is correct and reusable: the capture does not have to come from the host at
all. DOSBox-Staging runs fine completely headless (`SDL_AUDIODRIVER=dummy`, `output=sdl2`, no window; verified with
a probe whose `ECHO probe-ok > probe.txt` landed on the host filesystem), and the guest can write files into the
mounted directory. So a guest program can be the capturer.

The trigger problem is what kills it. A `.COM` TSR (`scripts/rig/snapper/snapr5.asm`, nasm `-f bin`) that hooks the
**real-mode INT 10h vector** is the only hook the game cannot route around: its protected-mode VESA calls go through
DPMI's "call real-mode interrupt" service, which dispatches the real-mode vector, so every mode set (AH=4F/AL=02)
and window slide (AH=4F/AL=05) passes through it. Records were designed as `[NGH1][ah][al][ecx][edx][seq][len]` +
64 KiB of `A000:0`, appended to `SNAP.BIN`.

Measured (dosbox-staging 0.83, `machine=svga_s3`, `-conf` autoexec):

| variant | handler | result |
|---|---|---|
| install only (vector left alone) | -- | clean run, marker file written (so the TSR machinery, INT 27h sizing and file creation are all correct) |
| chain handler | `pusha/push es,ds` + `call far [cs:old_off]` + `iret` | autoexec stalls; run had to be killed at 20 s |
| tail-call handler | `jmp far [cs:old_off]` | stalls |
| dump-then-tail-call | dump, restore, `jmp far [cs:old_off]` | **the emulator process itself segfaulted** (host-side SIGSEGV) |

Two of my own bugs were found and fixed along the way (INT 21h/40h takes BX=handle and DS:DX=buffer, not DX=handle;
and a 4-push/3-pop imbalance in the dump routine), and the fixed code still fails -- the failure is in DOSBox's
handling of a patched real-mode INT 10h vector, not in the TSR. That closes the guest-hook family: the timer hook
is unreachable because NIGHT owns IRQ0/IRQ1 through its protected-mode IDT (docs/46 route 5), and the one vector
the game cannot avoid taking into real mode crashes the emulator when patched.

**Net for the objective:** with (1) `screencapture` returning black, (2) no window visible to System Events, (3) no
`capjpg`/`cappng`/capture CLI in 0.83, (4) no file-backed serial route, (5) the guest timer TSR producing zero
records, and (6) the guest INT 10h TSR stalling or crashing the emulator, original-side frames require a real
display session on this machine -- i.e. a DOSBox window launched from a GUI Terminal, which I cannot open myself.
Until then the shipped-asset decode basis (docs/63) is the reference everything in this port is measured against.

## Route 7 (this cycle): DOSBox-X, which *does* have capture commands -- but not headless

`dosbox-staging` has no image-capture command at all, so this cycle installed `dosbox-x` (Homebrew bottle,
2026.10.01), which ships the capture command family. It runs fully headless:

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox-x -nogui -conf <file>
```

and its `[autoexec]` really executes (verified: `MODE CO80` changes the emulated surface to 720x400 in the log, and
`ECHO hello > E:\mk.txt` lands on the host filesystem).

What does **not** happen is any capture file. Every syntax variant tried produced nothing, in the mounted directory
and anywhere else on the host (searched `~/Library/Application Support/DOSBox-X`, `~/Documents`, `/tmp` for
`.png/.jpg/.avi/.mpg/.raw/.info` newer than the run):

```
cap raw E:\a.raw          cap raw E:\b.raw movie     cap raw movie E:\c.raw
cap movie E:\d.avi        cap mpeg E:\e.mpg          cappng E:\f.png
capjpg E:\g.jpg           cap save E:\h.png
cap raw movie night.raw   cap raw add f1.raw         cappng shot.png   capjpg shot.jpg   (relative names)
```

The commands are simply inert in a `-nogui` session: DOSBox-X's capture path is driven from the SDL event loop and
its hotkeys (Ctrl+F9 capture cycle), and with `SDL_VIDEODRIVER=dummy` there is no event stream to drive it -- the
same class of failure as routes 1-3 (no host-visible window) and routes 4-6 (no guest-side path).

**Status of the objective's original-frame clause:** seven routes measured, all dead on this machine without a
graphics session. What would work, in one line: start `dosbox-x` (or `dosbox-staging`) from a GUI Terminal so a real
window exists, then its capture hotkey (or my guest-side dumper) yields the frames, and the port side is already
deterministic (fixed cycles, scripted input, named-event shots) so the pairs line up automatically.

## Route 8 WORKS (this cycle): dosbox-automation's REST API, run headless in a Linux container

The blocker was never "no capture inside DOSBox" but "no capture that survives having no window". **dosbox-automation**
ships an HTTP REST API whose frame endpoints are independent of the window and the hotkeys, so it answers even under
`xvfb`. It has no macOS build, so the linux-x86_64 release tarball runs in an amd64 container (OrbStack/Docker is
present on this machine). Full recipe in AGENTS.md; harness: `scripts/playtest_orig_capture_api.py`.

Measured just now, with the ORIGINAL game running (`/api/v1/status` -> `"program": "NIGHT"`, `"running": true`) and
its own mode transitions in the emulator log (mode 13h -> **VESA 640x480 mode 101h** -> 13h):

```
captured 875 frames into /tmp/dbgauto/orig_boot      # 100 s of the original's boot: logo, intro, title, attract
```

Two configuration gotchas cost time and are now recorded in AGENTS.md: the webserver binds `127.0.0.1` by default,
so a container's published port is unreachable from the host until `webserver_bind_address = 0.0.0.0` **and**
`webserver_allow_remote = on` are set; and `output=texture` still needs an X server (`SDL_VIDEODRIVER=dummy` aborts
in `GFX_InitSdl`), hence `xvfb-run`. Frames come back as raw BGRX32 with a documented header (width/height/pitch/
format) or as PNG, plus `/api/v1/video/frame/info` with the emulated video mode; lossless ZMBV video recording and
input injection (`/api/v1/input/sequence`, `/api/v1/input/type`) are also available, which makes the *same moments*
reproducible on both sides rather than merely nearby.

### First comparison pass: two harness fixes needed before the pairs are meaningful

Parsing the 875-frame store against the port's logo shot showed the harness, not the comparison, needs work:

- only 112 of 875 fetched frames were kept, because the scene-change threshold (`sig_delta >= 3`) is tuned too
  coarsely for fades (a fade-in changes every block slightly); and
- every kept raw frame decodes as 640x480 BGRX32 even while `frame/info` reports mode 13h (320x200), so either
  the raw endpoint returns the scaled surface or `frame/info`'s mode field latches the last `SET MODE`; the
  `pitch`/`width` in the frame header is authoritative and must drive the decode.

With those fixed, the pairing plan is content-based rather than clock-based: identify each original frame by
matching it to a shipped card decode (docs/63 basis), then compare the port frame captured for the same event.

## First real port-vs-original result: the title screen matches to the pixel

Pairing the port's named-event shots against the original's captured frames (60 s capture at 10 Hz, 561 frames,
`format=raw` BGRX32, downsampled-luma NCC plus a native-size per-pixel check with a +-3 tolerance for DAC/gamma):

| port frame | best original frame | NCC | pixels within +-3 |
|---|---|---|---|
| title frame (arena byte-identical to `title3.pcx`) | t=59.9 s | **1.0000** | **1.0000** (all 307,200 pixels) |
| logo / intro / slides / stage card | -- | 0.00-0.07 | 0.00-0.04 |

The title row is the first genuine original-vs-port proof: same content, same pixels. It also independently
validates the shipped-asset reference basis of docs/63 -- the port frame that was byte-identical to the decoded
`title3.pcx` is simultaneously identical to what the original draws.

The other rows are not port findings: the 60 s original window ends as the title appears (`cycles=max` boot plus
DOS autoexec), so the logo, `.fli` intro, per-episode slides and stage card were never on the original's screen
during the capture; the "best matches" are text/BIOS frames. Next step is an input-driven original capture
(`/api/v1/input/sequence` at the title, longer window) so those moments exist on both sides.
