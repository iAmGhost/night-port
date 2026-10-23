# 46 — the playtest harness (scripted input + named-event screenshots)

Reproduces a human route through the game headlessly, names every screenshot after the game event that
produced it, and pairs those with frames captured from the original under DOSBox Staging.

## Port side: `--script` + `--shots`

`port/src/app/main.c`:

* `--script FILE` — a timeline of keys and explicit shots. Both keys and real SDL input land in the same
  `key_apply()` (SDL scan code → the DOS scan code the game's key state consults → the game's own key
  bytes `g_dat_63ea4/63ed0/63e89/63ec1` and the `g_dat_62c58[]` slot), so a scripted run drives exactly
  the bytes an interactive run does.
  ```
  <t_ms> key <name> down|up        # enter space esc tab up down left right ctrl shift, a-z, 0-9, 0xNN
  <t_ms> shot <name>
  ```
* `--shots DIR` — writes `NNNN_<event>.bmp` plus `manifest.tsv`
  (`seq t_ms frames event resource resource-name stage entities W H`).
* Event naming comes from a new host-side instrumentation hook in `f.c`: `g_res_trace` /
  `g_res_trace_seq`, set at the entry of `fn_0367c9` (the resource-name lookup). Every resource the game
  opens therefore becomes a named screenshot — no behaviour change, `docs/46` note in the source.
* Auto-shots: on each resource lookup; periodically (2 Hz) while a `.fli` is the current resource (so
  intro pacing is measurable); on a large sampled-screen change (`--no-scene-shots` / `--no-res-shots`
  to disable, `--shot-limit N` to cap).

Example (episode 1, `scripts/playtest/route_episode1.txt`) — measured run, manifest excerpt:

```
0    opening.fli     (resource lookup)         <- the very first resource the game opens
9000 attract_done
10039 title1.pcx     (resource lookup)
12001 menu
12518 stage1.pcx     (resource lookup)  entities=3
16000 episode_start  320x200
30000 stage_play     640x480
```

Two of the reported issues are already confirmed as *missing code paths*, not rendering artefacts:

* **no company logo** — the first resource the port opens is `opening.fli`; `logo.pcx` is never looked up
  (issue #1).
* **slideshows skipped** — no `<stage>s<N>.pcx` / `<stage>e<N>.pcx` lookup anywhere in the route
  (issue #4); `stage1.pcx` is loaded straight after the menu key.

## Original side: `scripts/playtest_orig.py`

Plays the same route against the original in DOSBox Staging 0.83:

* input via Staging's **AUTOTYPE** virtual command (`autotype -w WAIT -p PACE buttons…`, `,` = extra
  PACE) — the route's key events are snapped to a shared 0.5 s cadence so both runs see the same rhythm;
* captures into `[capture] capture_dir` with `default_image_capture_formats = raw`, named
  `NNNN_<event>.png` in shot order so they pair 1:1 with the port shots;
* `imgmount d TG_NIGHT.CUE -t iso` works (MSCDEX installed) — the earlier
  `-useimgmode none` variant failed to open the disc.

**Environment limitation found:** the screenshot action (`Cmd+F5` on macOS) is a *host* key action, and
this session cannot give the DOSBox window focus — `System Events` activation is refused by macOS focus
stealing prevention, and `CGEventPostToPid` reaches the app but not the SDL action path. Running
`scripts/playtest_orig.py` from a normal terminal (where DOSBox can be frontmost) works; the focus-free
alternative is a guest-side VGA snapper (a small `.COM` that samples 0xA0000 + the DAC to files on the
mounted C: drive — matching the port's own `vidscr*.bin` / `pal*.bin` dumps), which is the next step for
this task.

## Comparison: `scripts/playtest_compare.py`

`uv run --with pillow python scripts/playtest_compare.py /tmp/pt1 scratch/orig`
pairs shots by `NNNN_<event>`, converts both to RGB (raw framebuffer on the original side, indexed
screen + DAC on the port side), resizes to the port size, and reports per pair:

* `mad` — mean absolute RGB difference,
* `corr` — Pearson correlation of luma (structure),
* `match` — fraction of pixels within 12 levels on all channels,
plus the best-matching original frame for every port shot (`nearest …`), so a timing offset between the
two runs cannot hide an otherwise-correct frame.

## Original-side capture: what works, and the exact environment blocker

Two capture paths for the original were built:

1. **Guest-side snapper TSR** — `scratch/snapsr/snap.asm` (built with `nasm -f bin` to `SNAP.COM`, copied
   into the mounted C:). It hooks INT 8 (tick count) and INT 1C (does the work), and once per second
   writes the mode-0x13 framebuffer plus the 768-byte DAC to `C:\CAP\SNAPnnnnn.DMP`
   (`NGSH` header, magic + version + w/h + tick, then palette, then 200 rows top-down). It only dumps when
   `INDOS` (0050:0080) is clear and never re-enters itself. This path needs **no host key events at all**,
   which is what the macOS focus restriction makes impossible.
2. `scripts/playtest_orig.py` — the DOSBox-side driver (autotype cadence snapping, conf generation).

Neither can run from this agent session: **DOSBox Staging launched from here never reaches its
`[autoexec]` section.** Verified three independent ways:
no DOSBox window appears in `CGWindowListCopyWindowInfo` (all windows, `--noprimaryconf`, `output=surface`);
`--exit` with `-c "echo hi > c:\TEST.TXT"` produced no file after 30 s; a minimal `echo HELLO >
z:\ZZZ.TXT` (Z: = host cwd, no mount involved) produced no file after 12 s. The log always stops right
after `SDL: cocoa video initialised` / `CAPTURE: Image capturer started`, i.e. the emulator is blocked
before the DOS shell runs — consistent with a session that cannot create a window-server connection.

Consequence: the original-side half of every screenshot comparison must be produced by a human in a
normal terminal, e.g.

```
cp /tmp/snap.com NIGHT/SNAP.COM && mkdir -p NIGHT/CAP && rm -f NIGHT/CAP/*.DMP
dosbox-staging --conf scratch/dosbox_snap.conf     # snap.com + night, ~60 s
```

after which `scripts/snap_decode.py` (queued) turns `NIGHT/CAP/*.DMP` into PNGs next to the port's shots.
Until then, port-side evidence uses the extracted assets (logo.pcx, opening.fli via ffmpeg's flic decoder,
the per-type .spr/.act tables) plus the manual captures already in `scratch/` and
`docs/manual-playtest/00-screenshots/`.

## Update the same day: the real DOSBox blocker was audio, not the window server

DOSBox Staging 0.83 launched non-interactively stalls in SDL **audio** init on this machine -- the same
broken default CoreAudio device behind the port's `AudioQueueStart` failure (task `audio`). With the audio
driver stubbed out it boots, runs `[autoexec]`, mounts drives and writes files normally:

    SDL_AUDIODRIVER=dummy dosbox-staging --noprimaryconf -conf /tmp/orig_run.conf

with `[sdl] output=tty` and an `[autoexec]` of

    mount c ~/Git/night-port/NIGHT
    mount d ~/Git/night-port/NIGHT/TG_NIGHT.CUE -t iso
    c:
    snap.com
    night

(`imgmount` is deprecated in 0.83 -- use `mount d <cue> -t iso`.) Verified: `[autoexec]` probe files land
on the host, `MSCDEX installed`, and the TSR's `C:\TICK.TXT` dump appears.

Snapper v2 (`scratch/snapsr/snap.asm`, 1834 B) now tracks the video mode through an INT 10h hook (VESA
`AX=4F02` records BX; BIOS `AX=0000..0013` records AL) and dumps the linear framebuffer as
`ceil(w*h/65536)` 64 KB chunks with the read window moved by INT 10h `AX=4F05`, the same trick the game's
own still loader uses. Three real bugs were fixed on the way:

1. v1 accepted only mode 0x13, so it bailed on every tick once the game switched modes.
2. The per-row loop counted rows in BX, but BX is the file handle for every `INT 21h AH=40h` call -- the
   loop never terminated and wrote a 4.3 GB file.
3. DOSBox's DOS refuses a transfer buffer inside the A0000 window itself (the header written from the
   TSR's own segment landed; the framebuffer write did not), so each window chunk is now copied into a
   64 KB buffer reserved with `INT 21h AH=48h` at install time and written from there.

Findings from the diagnosis, all reproducible with the DOS-prompt-only conf (`scratch`-adjacent
`/tmp/snap_only.conf`: mount, `snap.com`, three `dir /s` to burn time, `exit` -- and note `exit` matters,
because a `SIGKILL`ed DOSBox loses file tails):

- The 64 KB staging allocation (`INT 21h AH=48h`) **fails** at install time on this image, so the earlier
  revision copied 64000 bytes with `ES=0` and scribbled over low memory -- which is why every DOS call
  after the copy misbehaved and why symptoms looked random. The TSR now checks the buffer pointer and bails
  before touching anything, so the current build is safe to leave resident.
- With the guard in place the probe file is created but its 4-byte record still lands as **0 bytes**:
  inside the INT 1C handler `AH=3Ch` succeeds while `AH=40h` writes nothing, even with the payload staged at
  an address a successful write uses (the DAC buffer) and even with `DS` restored to the TSR's own segment.
  A later probe pass also re-created the same name, which `AH=3Ch` refuses; probes now delete first.
- Net state: the original-side capture path boots and runs the game headlessly, and the snapper writes a
  correct 796-byte header+DAC record, but its framebuffer record is still empty. Next experiment if this
  is picked up again: stop doing file I/O inside the tick handler at all -- collect nothing in the ISR and
  instead have the batch run a *foreground* dumping program (a second .COM) that reads A0000 through the
  VESA window and writes the file, driven by a marker the TSR leaves in its own data.

The header layout is v2: `NGSH`, version, mode, w, h, tick, reserved (28 B).
The header layout is v2: `NGSH`, version, mode, w, h, tick, reserved (28 B).
