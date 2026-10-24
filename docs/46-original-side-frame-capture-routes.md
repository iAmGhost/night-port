
## Route 6 (this cycle): guest-side INT 10h snapshot TSR -- also dead, and it kills the emulator

The insight worth keeping, because it is correct and reusable: the capture does not have to come from the host at
all. DOSBox-Staging runs fine completely headless (`SDL_AUDIODRIVER=dummy`, `output=sdl2`, no window; verified with
a probe whose `ECHO probe-ok > probe.txt` landed on the host filesystem), and the guest can write files into the
mounted directory. So a guest program can be the capturer.

The trigger problem is what kills it. A `.COM` TSR (`scratch/snapsr/snapr5.asm`, nasm `-f bin`) that hooks the
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
