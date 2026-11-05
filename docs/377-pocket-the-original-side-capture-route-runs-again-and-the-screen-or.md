# 377 — the original-side capture route runs again, and the screen oracle can read the Pocket host's frames

Two things that were assumed available got re-verified on the machine today, and one gap got named precisely.

## 1. dosbox-automation in the container still runs the original

`dbga:ready` (the image committed in docs/46's solving) plus the mounted `NIGHT/` tree, `night.conf` with `NIGHT`
appended to `[autoexec]`:

```
19:48:40 DISPLAY: VGA 320x200 256-colour graphics mode 13h      ; the .fli intro / gameplay mode
19:48:40 DISPLAY: VESA 640x480 256-colour graphics mode 101h    ; the logo card and the slides
19:48:44 DISPLAY: VGA 320x200 256-colour graphics mode 13h
19:49:25 DISPLAY: VESA 640x480 256-colour graphics mode 101h
```

That mode sequence is the game's own phase signature -- the same 101h-for-cards / 13h-for-.fli-and-gameplay split
that `scripts/playtest/drive_orig_episode1.txt` is anchored on.  Reading video RAM through
`/api/v1/memory/0xb800/0/0x1000` shows `MSCDEX installed.` and `C:\>NIGHT`, so the CD path is up as well.

One API quirk worth knowing: `/api/v1/video/frame/info` kept reporting `bios_mode_number: 3 / 720x400` while the log
showed the game in 101h.  The capturer's geometry repair (docs/181 §6, docs/292 §5.2 -- derive geometry from the
payload actually delivered, not from the header's claim) is exactly what keeps such a session alive, and it did:
245 frames captured across a mode change with no crash.

## 2. The capturer's transport, swapped (scripts/rig/capture_container.py)

`scripts/rig/capture_original.py` talks to `http://localhost:8386`, and the container's published port is not
reachable from this shell -- measured again today, twice:

```
curl http://localhost:8386/api/v1/status   -> 000
curl http://127.0.0.1:8386/openapi.json    -> 000
docker exec dbga63 curl .../api/v1/status  -> {"running":true,...}
```

The image has no `python3`, so the capturer cannot simply be moved inside.  `scripts/rig/capture_container.py`
therefore imports `capture_original` and replaces **only its `api()` function**: every call becomes
`docker exec <name> curl`, and a binary frame reply is written to the mounted `/work/_cap.bin` and read back from
`/tmp/dbgauto`.  The 40×30 mean-luminance grid, the mode-transition geometry repair and the `frames.bin` record
layout stay the rig's own code -- a transport swap, not a reimplementation.

## 3. The oracle ran on the Pocket host's frames -- and what that run does and does not prove

```
$ ./pocket/src/night/app_pc --frames 4500 --rig-dump /tmp/port66          # cold boot, no warp
$ python3 scripts/rig/diff_screen.py --orig /tmp/orig66/frames.bin --port /tmp/port66/frames.bin
orig frames=245 distinct_pictures=1 rate=3.3/s
port frames=4501 distinct_pictures=770 rate=54.5/s
aligned pairs=245 route score=0.9495 (1 = identical)
divergent runs (score < 0.88): 0
pacing check: picture changes per second orig=0.01 port=9.32  <- pacing or slide-count divergence
channel-order check: inconclusive (sampled frames are near-greyscale, mean |R-B|=0.2)
```

What it proves: the Pocket host's `frames.bin` is readable by the rig's screen oracle, the two sides align
monotonically, and the plumbing for a picture-parity measurement exists end to end.

What it does **not** prove: any claim about picture content.  `orig distinct_pictures=1` says the original side saw
one picture for the whole 75 s -- the DOS shell and the autoexec text, because this capture had no drive script and
the game's title card waits for a key.  A route score of 0.9495 against a static frame is a plumbing result, not a
fidelity result, and it is written down that way.

## 4. The two halves that would make it a content measurement

1. **The original side needs its anchored driver.**  `drive_orig_episode1.txt` is detection-anchored (pixel height,
   not wall time) and speaks `playtest_orig_drive.py`'s vocabulary -- `record boot 5000` is rejected by
   `capture_original.py`'s key-list parser ("unknown drive step").  That driver needs the same transport swap this
   document describes, which is now a ~20-line change against a template that exists.
2. ~~The Pocket host has no scripted input.~~ **Closed the same day -- see §5.**

Until those exist, the picture-parity evidence for this port remains what docs/91 measured on the shared `src/f`
layer (348 of 517 original frames with a port counterpart at NCC ≥ 0.90, `.fli` pairs pixel-exact) plus docs/365's
Pocket-vs-SDL-host overlap (91.7–95.5 % of distinct pictures) and docs/373's card-exactness results -- which are
comparisons against the shipped assets rather than against live original frames.

## 5. Half of that gap closed the same day: `--script` on the Pocket shim

`app_pc` (and the same host code on the card) now takes `--script FILE` and feeds the rig's key lines into the pad
span table that `NIGHT_PAD_SCRIPT` already used (`main_of.c`).  The rules it states out loud:

- only `key` lines are honoured.  `wait res logo.pyc`, `shot ...` and `record boot` are counted and reported as
  skipped: the anchoring machinery that rebases a timeline on a resource lookup is the SDL host's reader (docs/55),
  and pretending otherwise would make a Pocket run look like an anchored one.
- keys go through the **direct** path (`night_key_apply` with a DOS make code), because route files name DOS keys:
  `x` in a route file is the X key (make `0x2d`), while the pad's X button is wired to Left Shift (`0x2a`, docs/333).
  The table's codes are the same set-1 codes the pad table already uses -- enter `0x1c`, esc `0x01`, space `0x39`,
  lctrl `0x1d`, lalt `0x38`, shift `0x2a` -- so a scripted run and a pad run land in the same key state.
- times convert on the pad's own grid, one poll per 8 ms; a press with no matching `up` is held to the end of the
  run and counted as such; an `up` with no `down` is a no-op.

Measured, 3 000 frames each, no warp, same seed:

| | unscripted | `--script /tmp/keys_test.txt` (7 spans, 1 anchor line skipped) |
|---|---|---|
| disc requests | `cdda=906/27697/2` | `cdda=2394/19639/4` |
| transfers / plays | 62 / 62 | 5 / 5 |
| ticks | 26 515 (+491/s) | 26 515 (+491/s) |
| arena | 3 903 168 B | 3 903 168 B |

The scripted run asked for a **different disc track** (19 639 sectors instead of 27 697) and stopped the 62-transfer
attract loop after five plays: the keys reach the game on the Pocket code path, and the game answers them.  Card
0.1.66 carries the same host change.

What is still open for a content-based original comparison is the other half: `drive_orig_episode1.txt` speaks
`playtest_orig_drive.py`'s anchored vocabulary, so that driver needs the transport swap §2 demonstrates before the
original side can be captured mid-route.
