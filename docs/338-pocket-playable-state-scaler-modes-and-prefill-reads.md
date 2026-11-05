# 338 — the port is playable on the Pocket, and the last two failures named themselves

Core 0.1.17 through 0.1.19.  First run through to the main game on hardware:

- intro .fli video: shown, no flicker;
- main game (stage play): reachable by mashing buttons, sound and input working, no flicker;
- SFX: whole samples at the right speed (the "cut off early" report closed);
- the horrible buzz: gone (it was the looping voices of 0.1.16 — see docs/337 §5 and the zero-length refusal);
- still broken at that run: logo / title / episode slideshow showed **black**, intro and stage **music** absent.

Both of those named themselves in `nightlog.sav` (81 677 B).

## 1. Black 640x480: the core declared one scaler mode

```
caps max=800x600 stride=2048 phys=320x240 def=320x240 bytes=1048576 modes=13
mode 9: 640x360   mode 10: 640x400   mode 11: 640x480 stride=640 color=0   mode 12: 800x600
fb mode -> native 640x480 rc=0 in 41216us
stages last m=17 p=180 c=15054 f=214 x=236        (copy was 80 835us in 0.1.16)
memstat frames=104 ticks=987(+494/s)              (+13/s in 0.1.16)
```

The OS accepted the larger source framebuffer and the blit got 5x cheaper, and the tick pump stopped freezing
the logo (no re-anchor: `reanchor=1` for the whole 640x480 stretch, +491/s throughout).  Nothing reached the
LCD because `Cores/iamghost.night/video.json` declared a single scaler mode, 320x240 — so the core had no
scaler mode to map a 640x480 source onto the panel with.  The SDK's own core and the pocket template both list
eight modes including 640x480, as does openfpgaOS-Quake; 0.1.19 ships the same eight.

Two numbers to keep: a source-mode switch costs **41-53 ms** (so it runs at geometry changes, never per frame),
and a 640x480 frame in native mode costs **15 ms** against 80 ms of 2x2 majority arithmetic.

## 2. Missing intro/stage music: the prefill read died, and the log said where

```
cdda req idx=1  lba=37262  sectors=3153  (42.0 s) -> disc track 2  slot 18
cdda refused: prefill short (111 of 300 sectors) -- slot unreadable, voices stay off
cdda req idx=9  lba=185120 sectors=27697 (369.3 s) -> disc track 10 slot 26     (played)
cdda req idx=11 lba=223146 sectors=17856 (238.1 s) -> disc track 12 slot 28     (played)
cdda req idx=2  lba=40415  sectors=19639 (261.9 s) -> disc track 3  slot 19
cdda refused: prefill short (111 of 300 sectors)
```

The tracks that played are exactly the ones audible in the run (title, PCX slides); the two refusals are the
intro and the stage.  `of_disc_read_audio()` used `fopen("slot:N") / fseek / fread / fclose` **per 2352-byte
sector** — 300 opens to prefill a 4 s ring — and gave up mid-run naming nothing.  0.1.19 keeps one handle per
track, seeks only when the read is not the next sector in order, and logs `disc: slot N sector M read X of 2352
bytes` when a read is short.

## 3. Open questions for the next card run

- Do the logo, title and episode slideshow appear with 640x480 among the scaler modes?  If they are still
  black, `FB_MODE=0` in `Assets/night/common/night.ini` puts the 640x480 picture through the 2x2 software
  downscale at 320x240 (80 ms a frame, but it displays — and the tick pump now absorbs that without freezing).
- Does intro/stage music come through with the single-handle read?  If not, the new `disc:` line says which
  slot and sector.
- `of_gpu` is untouched in this port and the Doom port's `docs/rv32-gpu-performance.md` measures GPU command
  submission on the production CPU RTL — the next candidate for the 15 ms blit.
