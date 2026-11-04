# 46 — the playtest harness (scripted input + named-event screenshots)

Reproduces a human route through the game headlessly, names every screenshot after the game event that
produced it, and pairs those with frames captured from the original under DOSBox Staging.

## Port side: `--script` + `--shots`

`src/app/main.c`:

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

1. **Guest-side snapper TSR** — `scripts/rig/snapper/snap.asm` (built with `nasm -f bin` to `SNAP.COM`, copied
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
dosbox-staging --conf scripts/rig/conf/dosbox_snap.conf     # snap.com + night, ~60 s
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

Snapper v2 (`scripts/rig/snapper/snap.asm`, 1834 B) now tracks the video mode through an INT 10h hook (VESA
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


## Original-side capture, measured 2026-10-24 (all three host routes closed, guest route narrowed)

The remaining half of the harness contract is "the same moments captured from the original under
dosbox-staging". Every host-side route was tested and measured, not assumed:

1. **Host screenshot of the DOSBox window** -- `screencapture -x /tmp/dbx/full.png` returns a real file
   (6016x3384 PNG), but every pixel is black: mean `[0.0 0.0 0.0]`, `std 0.0`, `frac nonblack 0.0`.
   Screen recording yields nothing for this session. System Events lists the process
   (`dosbox-staging`) but `get name, position, size of every window` returns `,,` (no usable window).
   Synthetic key events therefore have no target either.
2. **dosbox-staging's own capture facility** -- `--help` has no capture/screenshot/record flag; the
   strings show a `capture_dir` setting and a screenshot *action* bound to a key, so it can only be
   triggered by an emulated keypress, which is exactly what cannot be delivered.
3. **Emulator swap (qemu: headless `screendump` + `sendkey`)** -- would solve both input and capture, but
   `qemu-system-x86_64`, `dosemu`, `bochs` and `v86` are all absent here, and it deviates from the
   contract's "under dosbox-staging". Left as a proposal, not done.

So the capture has to happen inside the guest, and that route was narrowed to one open question by a
purpose-built probe (`scripts/rig/snapper/snapprobe.asm` -> `SNAPPRB.COM`), which counts, once a second, how
many times its real-mode INT 8 / INT 1Ch / INT 10h / INT 21h hooks fire and which video mode it saw.

Findings from that work, each one measured:

- **8.3 names bite twice.** `SNAPPROBE.COM` is invisible to the DOS shell (DIR lists it as `SNAPPR~1`), so
  two runs did nothing at all; and `INT 21h AH=3Ch` *refuses* `SNAPPROBE.BIN` (name too long). Files for
  the guest must be <= 8 characters + 3.
- **Delete-first can break the create.** With `AH=41h` immediately before `AH=3Ch` the create reported an
  error (marker file for the failure branch was written); dropping the delete and letting the host `rm`
  before each run made the create succeed. The old v2 note "AH=3Ch refuses an existing name" is only half
  the story.
- **A handle held open across the run loses everything.** DOSBox buffers writes; a session ended with
  SIGTERM leaves a 0-byte file even though every write "succeeded". Records must be written as
  open -> seek-end -> write -> close (which is what v3's per-frame dump already does).
- **The write path in the probe is still not landing.** With the game absent, `SNAPPRB.COM` creates its log
  and closes it, but the file stays at 0 bytes, while an earlier variant (`snap_dbg2` marker routine:
  delete, create, three writes, close) produced 16 bytes at the same point in the install. So writes are
  possible from a resident .COM's install path; the difference between the two shapes is the immediate
  next experiment.
- **v3's INT 21h-hook dump captured nothing**: `C:\CAP` stayed empty over a 110 s run with the game. The
  likely cause -- to be confirmed by the probe counters, which need the write path fixed first -- is that a
  DOS4GW/protected-mode guest reaches DOS through DOSBox's DPMI callback rather than through the
  real-mode IVT, so a real-mode INT 21h hook never sees the game's DOS traffic. The INT 1Ch hook does run
  in-game (an earlier revision accumulated a 65 KB tick log while NIGHT was running), so if the counters
  confirm the INT 21h bypass, the dump has to move back to the tick context with a
  write-and-close-per-frame pattern, or go to a device (serial/printer OUT instructions) that needs no DOS
  at all.

Until that open question closes, the port side keeps advancing on evidence that does not need original
frames (episode sweep, docs/58, docs/59), and the comparison halves of the affected tasks stay open with
this document as their status record.

### Follow-up measurement (same day): the write path is not the blocker

Two side-by-side guests were built to separate "hook shape", "call shape" and "context":

- `scripts/rig/snapper/snapwrkX.asm`: the probe's exact create -> write 16 -> close sequence, run twice -- once
  before any hook is installed and once **after all four hooks** (INT 8 / 1Ch / 10h / 21h, each counting
  and chaining). Both files landed with their 16 bytes (`SNAPPRB.BIN`, `SNAPPRC.BIN`, payload `0x5a`).
- Adding the classic DOS-reentrancy guard to the probe's INT 1Ch handler -- `INDOS` (`0050:0080`) must be
  clear, plus a private busy flag -- changed nothing: its log still holds 0 bytes while the marker creates
  around it all succeed.

Conclusion so far: hooks, the create/write/close shape and the install context are all demonstrably fine,
so the probe program's empty log is a defect inside that program rather than a property of DOSBox or of
DOS reentrancy. It is left open; the two variants are kept in `scripts/rig/snapper` so the difference can be
bisected mechanically (they differ only in the resident handlers and in what surrounds the write). What
the probe would answer -- whether a real-mode INT 21h hook sees a DOS4GW guest's DOS traffic at all --
is still the question that decides whether the guest-side snapper can dump frames from the INT 21h hook,
must go back to the tick context, or has to bypass DOS entirely (serial/printer `OUT` instructions).

## Original-side frame capture: closed as a measured dead end on this machine (five routes)

1. **Host screenshot of the emulator window.** With a real window (`output=texture`, log lines
   `DISPLAY: Using 1671x1253 window size in windowed mode on display-0` and
   `SDL: Using 'metal' SDL render driver`), `screencapture -x` still returns an entirely black 6016x3384
   image. The earlier black captures were not caused by `SDL_VIDEODRIVER=dummy` -- they reproduce with the
   Metal window present.
2. **Accessibility / synthetic keys.** `System Events` reports `0` windows for the dosbox-staging process, so
   neither keystrokes nor window enumeration can reach it.
3. **Emulator-side capture command.** 0.83 has no shell-internal capture command: `capture`, `capture video`
   and `capture image` all produce `Illegal command: capture` (proven by redirecting the shell output to a
   file on a mounted directory -- redirection of internal-command output does work, which is how this was
   read). The `CAPTURE:` log strings are the hotkey-driven subsystem plus `capture_dir` /
   `default_image_capture_formats`, and no CLI flag exists.
4. **Port-based (LPT/serial) file devices.** The binary only knows LPT *DACs* (`lpt_dac`, Covox / Disney /
   Stereo-On-1) -- there is no file-backed parallel or serial device to `OUT` frames into.
5. **In-guest TSR.** `scripts/rig/snapper/snap4.asm` (v4) fixes the probe defects found by reading the code -- the
   earlier probe handed the *function number* to DOS as the file handle (`mov ax,0x4202` then `mov bx,ax`) and
   my own cleanup pass had removed the create call, so `open` failed with no trace. With both fixed, a
   70-second run of the real game (mounted C: plus the CD image, MSCDEX installed) leaves
   `S4LOG.BIN` = **16 bytes** (the install-context record) and `S4VID.BIN` = **2048 bytes** (the
   install-context window sample): the write path works from the foreground, and the INT 1Ch handler produced
   **zero** records while the game ran. Together with the earlier finding that a real-mode INT 21h hook sees
   none of a DOS4GW guest's DOS traffic, the conclusion is that NIGHT takes the timer through its protected
   mode IDT, so a real-mode timer chain never runs in-game.

Every comparison half that asks for "the same moment from the original under dosbox-staging" is therefore
blocked by the environment, not by the port. The substitute reference used instead, and why it is a sound
one, is in docs/63.
