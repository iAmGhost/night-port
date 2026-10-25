# 81. The ORIGINAL's GAME OVER / CONTINUE screen, and the port against it

Closes the last original-side gap in the game-over work (the port side was fixed in docs/61 and the harness crash in
docs/78). Verdict up front: **the port's CONTINUE screen is 99.66 % identical to the original's** (MAE 0.36, NCC 99.26 %),
and the only differing pixels are the blinking marker next to YES/NO.

## Reaching the screen in the ORIGINAL, from outside the guest

No cheat: the transition is entered by writing the game's own state variable through the webserver memory API, using
docs/74's calibrated mapping (`delta 0x1ba000`, `DAT_x -> phys (x-0x50000)+0x205000`, both re-verified against the loaded
image before any write).

* `DAT_0006336c = 1` is the shipped "gone for good" state: `fn_03a829` tests it at `0x0003abc0`, breaks the room loop and
  falls into `0x0003aeb7`, where `DAT_00053100` (the CONTINUE credit count, init 3) selects between the credit-less path
  and `fn_038445`, the CONTINUE screen loop.
* **It only works while the room loop is live.** Measured, three failed runs first: written while the game sat on a
  640x480 card, the value was *not* consumed (`writemem 0x6336c <- 1 : read back 1`, still 1 twenty seconds later, and
  `DAT_00053100` stayed 3). Written inside a live room, the very next sample shows it consumed
  (`writemem 0x6336c <- 1 : read back 0`) and the screen appears within ~1.5 s.
* Abandoned route, filed as an observation: the entity-death byte (`e[0x30] == 1` at `0x000129c4`, with
  `DAT_00053c80` documented as the player entity) was not usable -- in a live room the guest reads
  `readptr 0x53c80 = 0` while its neighbours are pointers (`readptr 0x53c7c = 0x631798` [absolute]), so that DAT does not
  hold what the port's `e == g_dat_53c80` comparison implies at that moment. No port change follows from that alone; the
  state-variable path reaches the same screen.

### The pacing trap that cost the three failed runs

`key_until_mode 200` (docs/74) accepts the **first** sample at mode 13h. Measured: a press landing as the episode's
picture show ends makes the game flash mode 13h for a single sample and then fall back to a 640x480 card forever
(`stage 64356 ms` one 320x200 frame, then 640x480 for the whole run; `DAT_63354` non-null, `DAT_53c80` 0, credits 3).
The drive therefore gained **`key_until_mode_stable <height> <key> <tries> <gap_ms> <stable>`**, which requires the mode
to hold for `<stable>` consecutive samples before accepting it. With `stable=4` the stage plays: `mode 200 held 4 samples
after 9 press(es)` and 302 of the remaining 320 frames are 320x200 gameplay.

## Numbers

Original frames: `/tmp/dbgauto/orig_go5/` (drive `scripts/playtest/drive_orig_gameover2.txt`), decoded with the docs/70
per-mode channel order (`--swap13 1`). Port frame: `/tmp/port_go6/0174_warp_02.bmp`.

| pair | within +-3 | MAE | NCC |
|---|---|---|---|
| **port CONTINUE frame vs original CONTINUE frame** | **99.66 %** | **0.36** | 99.26 % |
| original CONTINUE frame vs shipped `gameove1.pcx` | 95.06 % | 8.55 | 75.47 % |
| original CONTINUE frame vs shipped `gameove2.pcx` / `gameove3.pcx` / `continue.pcx` | 94.95 / 94.31 / 93.48 % | 8.62 / 8.98 / 9.55 | 75.4 / 74.7 / 73.5 % |
| **port CONTINUE frame vs shipped `gameove1/2/3.pcx` / `continue.pcx`** | **95.06 / 94.95 / 94.31 / 93.48 %** | 8.60 / 8.66 / 9.03 / 9.59 | 75.4 / 75.3 / 74.6 / 73.5 % |

*Premise check (docs/79 rule), printed with the scores:* for the original side, exchanging R/B drops
`gameove1.pcx` from **95.06 % to 66.03 %** (MAE 8.55 -> 13.65), and the exact-card-colour share is 65.717 % -- so the
capture is in the right channel order and the match is not a palette accident.

The port and the original score **identically against the shipped card** (95.06 / 94.95 / 94.31 / 93.48 %): the ~5 % off
the card is the composited layer (`over.spr` of the kneeling player, the GAME OVER / CONTINUE ? YES / NO text from
`over.spr`, the CREDIT digit), and it is in exactly the same places on both sides.

Residual of the port-vs-original pair: **138 pixels (0.22 %), bounding box x 201..218, y 90..118** -- the snapper marker
that sits beside YES/NO and animates between frames. Nothing else differs.

Credit count during the screen (guest reads at +0/+5/+10/+15/+20 s): `DAT_00053100 = 3` throughout, consistent with
`fn_038445` decrementing at `0x000389f3` only when a CONTINUE pass completes; the drive pressed no keys during the screen.

## Artifacts

* `docs/manual-playtest/07-shots/gameover_continue_port_vs_original.png` (port left, original right)
* `docs/manual-playtest/07-shots/gameover_continue_original.png`, `..._port_2026-10-25.png`
* `docs/manual-playtest/07-shots/gameover_continue_residual.png` (the 138 differing pixels highlighted)

## Port-side capture recipe (the frame has to be taken in the right phase)

    cd port && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
      NIGHT_SHOWSCREEN=gameover NIGHT_SHOWSCREEN_AT=6000 \
      ./test/night --warp-stage 2 --script ../scripts/playtest/route_walk_fight.txt \
      --shots /tmp/port_go6 --no-audio --frames 7500 --shot-limit 400

The warp then fires inside a live room (`showscreen warp: gameover at frame 6022`) and `warp_shots_poll` samples the
screen every 0.5 s (`warp_00`..). `NIGHT_SHOWSCREEN_AT` **must land while the room is live**: with `AT=2000` the gate let
the warp through during the 640x480 stage-card phase (`2s1.pcx` in the manifest) and the capture is 640x480 and scores
9.9 % against the shipped card -- a harness artefact, not a port defect. The room phase in that run spans roughly
frames 4700..20200 (manifest rows `walk_*`, 320x200, entities 4).

## Original-side reproduce

    export DOSBOX_API_TOKEN=... DBA_CODE_DELTA=0x1ba000 DBA_DATA_BASE=0x205000
    # fresh emulator instance in the dbga container (docs/74 launch recipe), then:
    uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
        --out /tmp/dbgauto/orig_go5 --drive scripts/playtest/drive_orig_gameover2.txt --hz 3

Drive steps added for this (`scripts/playtest_orig_drive.py`): `key_until_mode_stable`, `poke_at` (write through a
pointer held in DAT space, with the absolute/DS-relative decision made from the measured value), `readptr`.

## Still open (unrelated to this screen)

`gameove*` cards themselves appear only as the CONTINUE backdrop in this window; the terminal branch (`credits == 0` ->
`0x0003aeb7` credit-less path) was not reached, because the guest's credit count never fell. Not needed for this
comparison; noted so it is not mistaken for a gap in the port's game-over rendering.
