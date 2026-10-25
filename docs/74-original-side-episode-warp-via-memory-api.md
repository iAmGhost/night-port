# 74. Forcing the ORIGINAL game into any episode, from outside the guest

Closes the last gap in the per-episode sweep (docs/59): the original-side frames. Scripted play cannot clear nine
episodes end to end, and the port has long used a host-side debug warp (`--warp-stage N`) for exactly that reason.
This document does the equivalent on the original side without touching the game's source: the emulator's webserver
writes the shipped stage-index variable inside the guest.

## The mechanism, and why it is the shipped one

`DAT_00063370` is the master stage index: initialised to 1 by the master init at `0x365eb` and incremented by one at
`0x3b54c` when a stage completes. Every per-stage asset name is built from it (`stage%d.pcx`, `stage%d.wal`,
`stage%d.stg`, `night%d.chr`), the slide-count table `g_tab_52f2c` is indexed by it, and the room table offsets are
`(index-1)*10`. The port's warp simply assigns it before the loader names the assets, which is why the port needs no
other state to play stage N.

Doing the same in the guest needs the runtime address, and that has to be measured, not assumed:

1. **Code delta.** Search the guest's physical memory for a dword taken from NIGHT.EXE's code and keep candidates
   whose following 12 bytes match the file exactly. Four anchors agree: `delta = 0x1ba000`
   (`0x3bb42 -> 0x1f5b42`, `0x3a2d7 -> 0x1f42d7`, `0x12efa -> 0x1ccefa`, `0x40660 -> 0x1fa660`).
2. **Data base.** The loader relocates absolute data references. At `0x365eb` the file holds
   `c7 05 70 33 01 00 01 00 00 00` (`mov dword [0x13370], 1`) while the guest holds
   `c7 05 70 83 21 00 ...` (`mov dword [0x218370], 1`), so `DAT_x -> phys (x - 0x50000) + 0x205000` — which also
   explains the "pool offsets from DS 0x50000" convention used throughout the port.
3. **Sanity witnesses.** After that mapping `DAT_00063370 == 1` at boot and `DAT_00063e24` reads as a live counter
   (326288 in the probe session), so the data segment is really where the derivation says.

Both numbers are re-derived per session by `scripts/playtest_orig_drive.py` (`writemem` / `readmem` steps); they can
also be supplied (`DBA_CODE_DELTA`, `DBA_DATA_BASE`), in which case they are still verified byte-for-byte against the
loaded image before anything is written.

## The drive

`scripts/playtest_orig_episode_warp.py <episodes...>` restarts the emulator in the container for each episode, then
runs a detection-anchored drive: logo -> intro film (`wait_mode 200`) -> title cards (`wait_mode 480`) -> Enter ->
`writemem 0x63370 N` while the intro slide show is still running (before `stage%d.*` is looked up) ->
`key_until_mode 200 enter 30 1200` (the picture show waits a key per card and the card count differs per episode) ->
`shot stage_play` -> record 9 s of gameplay. Frames are decoded with the measured per-mode channel order (docs/70).

## Per-episode comparison against the port

Port shots are the `playing` frames of the docs/59 sweep (`docs/manual-playtest/04-shots/playing_stageN.bmp`);
the original's frames come from the runs above. Score = fraction of the 64,000 320x200 pixels within +-3 per channel,
best frame over the recorded gameplay window.

| episode | original frame | within +-3 | MAE |
|---|---|---|---|
| 1 | `0177_gameplay_11` (natural progression, docs/73) | **93.86 %** | 3.62 |
| 2 | `0082_gameplay_04` | 85.48 % | 12.96 |
| 3 | `0089_gameplay_11` | 82.27 % | 12.62 |
| 4 | `0089_gameplay_02` | 90.10 % | 5.09 |
| 5 | `0082_gameplay_04` | 88.16 % | 6.16 |
| 6 | `0080_gameplay_02` | 83.42 % | 11.37 |
| 7 | `0097_gameplay_19` | 89.43 % | 7.30 |
| 8 | `0080_gameplay_02` | 4.53 % | 41.19 — see below |
| 9 | `0080_gameplay_02` | **96.17 %** | 1.89 |

Eight of the nine episodes therefore match the original's own room view: the room backdrop, the palette and the HUD
land inside +-3, and the residual is where entities stand and which animation cell they are on, since the two runs
are not input-synced (the same effect that caps episode 1 at 93.86 %).

### Episode 8, filed

Under the forced index the original's stage-8 window is a *static* 320x200 picture (27 consecutive frames with
identical content, mean RGB (77.9, 70.0, 69.4), 177 distinct colours) while the port's stage-8 frame is a live room
(mean RGB (76.4, 57.7, 47.2)); NCC 0.197, 4.53 % within +-3. It is not `na80001.pcx` (mean (56,110,122)) and not the
`stage8.pcx` card (mean (79,100,134)). Raising the key pressure from 14 to 30 presses did not change it, so this is
not the per-picture key wait that the port needed the cooperative pump for (docs/52, docs/59). Left open: what the
original's forced stage-8 window actually shows (candidate: a picture-show card other than `na80001`, or a room that
needs per-episode state the forced index does not supply). The port's stage 8 itself plays, renders and accepts
control, with the entity-sprite and HUD composites byte-verified in docs/59's sweep.
**[closed, docs/82]** the window is the live episode-8 room with the episode's CG dialogue card (`na80001.pcx` content:
the speaker bust plus the subtitle box reading the same Korean line) composited over it -- identified by identical
per-colour occurrence counts in the bust region (1246 / 891 / 790 / 640 / 463 / 395 on both sides, colours a few LSBs
apart) and by the card's teal key `(27,123,143)` covering 57.27 % of the card appearing on **0.00 %** of the window's
pixels. It is static because the show runs once per room inside the room loop (`fn_03a70e`) and waits a key per picture
(docs/52). The 4.53 % / NCC 0.197 was a **phase** artifact: the port frame compared there had already left the show.
Aligned, the port's show frame is 100.00 % within +-3 (MAE 0.24, NCC 100.00 %) of the original's window, and the docs/74
pairing reproduces at 4.48 % / NCC 20.93 %. Guard: `scripts/playtest_ep8_show.py`.

## Container / API notes (each of these cost a run)

- The webserver rejects `Host: 127.0.0.1:8386` ("rejected request with Host header") — use `http://localhost:8386`.
- `/api/v1/dosbox/shutdown` and `/api/v1/control/shutdown` return HTTP 500 here, and the container image has no
  `pkill`, so instances are ended by scanning `/proc/*/comm` in the container and `kill -9`.
- **Never leave two emulator instances running**: with three alive, `/api/v1/memory/*` (GET and search) starts
  returning HTTP 500 while `/api/v1/status` keeps answering. The memory endpoints also intermittently 500 for full
  16 MB scans even with one instance, hence the verified-supplied-constants path.
- Launch recipe (fresh session): `docker exec -d dbga bash -c "cd /work/dosbox-automation-0.85.1-7567ec4-linux_x86_64
  && xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_one.log 2>&1"`.

## Reproduce

    export DOSBOX_API_TOKEN=...            # the container's bearer token
    uv run --with numpy --with pillow python3 scripts/playtest_orig_episode_warp.py 2 3 4 5 6 7 8 9

Frames and per-run drive files land in `/tmp/dbgauto/orig_warp/ep<N>/`; the scoring line is printed per episode.
