# 88 — The credit-less game-over branch (DAT_00053100 == 0) reached on the original side and scored

Goal task `orig-credits-zero-gameover`. docs/81 reached the CONTINUE screen (`fn_038445`, call site
0x0003aecf) but stopped one branch short: it always had a credit left. This pass drives the credit count to
zero through the memory API and scores the result.

## The branch, from the shipped code

The room play loop `fn_03a829` ends its `while (true)` on `DAT_0006336c == 1` (`break`), then runs -- in the
decompile of 0x0003abe7-0x0003aee1, and in the port at `f.c:22511-22512`:

```c
DAT_0006336c = 0;
if (DAT_00053100 == 0) { DAT_00053100 = 3; return 0; }        /* 0x0003aeb7-0x0003aec8  no credits left */
else { if (fn_038445() == 0) { DAT_00053100 = 3; return 0; }  /* 0x0003aecf-0x0003aee1  CONTINUE refused */
       else return 2; }
```

So the credit-less path never shows the CONTINUE card at all: it re-arms the credit count and returns 0 to the
dispatcher, which re-enters the boot / attract chain.

## Driving it in the guest (headless, scripted)

`scripts/playtest/drive_orig_nocredit.txt` = the docs/81 spine into a live episode-1 room, then the two shipped
state writes (docs/81's pacing rule: the write only counts while the owning loop is live):

```
readmem 0x53100 = 3        before                ->  writemem 0x53100 0 ; readmem 0x53100 = 0
readmem 0x63354 = 0x38202c (a room record is live)
readmem 0x6336c = 0        before                ->  writemem 0x6336c 1 ; readmem 0x6336c = 1
   ... 22 s of sampling at 6 Hz (301 frames) ...
readmem 0x53100 = 3        <- the branch's own DAT_00053100 = 3 reset: the witness that 0x0003aeb7 ran
readmem 0x63354 = 0x38202c (unchanged -- the room record pointer is not cleared)
readmem 0x6336c = 0        (the latch was consumed)
readmem 0x63370 = 1        (the episode counter did not move)
```

The presentation follows immediately: the mode goes 320x200 (room) → **640x480 plain fill** at t=61.43 s,
0.2 s after the write; then the boot/attract chain restarts -- title3 ramping in (harness card similarity
0.1248 at 62.3 s), 320x200 slide cards (1s1 / 1s2 / 1s3), the title1 / logo cycle at t=77 s.

## The port's same path (new host hooks, gated like docs/78)

```
credits warp: DAT_53100 = 0
quit warp: DAT_6336c = 1 at frame 3013 (credits=0)
display mode 640x480
```

`NIGHT_CREDITS=<n>` writes `g_dat_53100`; `NIGHT_QUIT_AT=<frame>` arms `g_dat_6336c = 1` only when a room
record is live (`g_dat_63354 != 0`, otherwise it logs a deferral instead of firing -- the same precondition
style as `fn_showscreen_gate`). After the branch the port walks the same chain: 640x480 black fill →
`opening.fli` at 320x200 (frame 3157) → **title3 held at 99.99 % within ±3** (frames 3187-3296) → fade →
the 1s1/1s2/1s3 slides → the stage1 card → and because the scripted route keeps feeding keys, a new episode
starts (mid show at 3679, episode_start at 4227).

## Scores

Every original 640x480 frame of the post-branch window was matched against every port post-branch 640x480
frame, in both channel orders (the docs/86 rule: this capture's 640x480 payloads are R/B-mirrored, the port's
frames are not):

| original frame | best port partner | order | within ±3 | MAE |
|---|---|---|---|---|
| 0176_nocredit_02 (black fill) | 0182_card_68 | as-captured | **100.00 %** | 0.00 |
| 0179_nocredit_05 | 0182_card_68 | as-captured | 100.00 % | 0.00 |
| 0181_nocredit_07 (fade ramp) | 0172_card_58 | R/B-swapped | 27.06 % | 1.66 |
| 0185_nocredit_11 (fade ramp) | 0175_card_61 | R/B-swapped | **100.00 %** | 0.19 |
| 0189_nocredit_15 | 0006_card_04 (boot phase) | R/B-swapped | 100.00 % | 0.30 |

Aggregate over the 25 original 640x480 samples vs the 65 port post-branch frames: **19 / 25 have a partner at
≥ 95 % within ±3; the median best pair is 100.00 %**; the six weaker samples are fade-ramp frames whose
fade step is one sample off between the two runs (the best partner for 0181_nocredit_07 still has MAE 1.66).
Pairing the first post-branch frames directly: `nocredit_02/04` vs `card_57` = 99.74 % within ±3, MAE 0.01
(both the plain 640x480 fill; the 0.26 % residual is a handful of non-black pixels).

## Verdict

**Faithful, and docs/81's gap is closed.** The credit-less branch is reached by the same shipped state writes
on both sides, the state effects match variable by variable (`DAT_00053100` 3 → 0 → **3**, latch consumed,
room record and episode counter untouched), the mode transition matches (320x200 → 640x480 within 0.2 s), and
the presentation frames agree at 100.00 % within ±3 wherever the two runs' fade phases line up.

One measured difference, with no port change following: inside its 22 s window the port reaches a *settled*
title3 (99.99 % within ±3 of the shipped card for 10 consecutive samples), while the original's window never
settles (its best title3 similarity is 0.1248 and it keeps cycling slides). That is the window length, not the
branch: docs/86 measured the original's title wipe as ~8.5 s after a ~2 s black hold, and it settles only when
the title is held long enough -- which docs/86's own capture then proved at 99.99 %.

## Reproduce

```sh
# original side
docker exec dbga bash -c 'for f in /proc/[0-9]*/comm; do case "$(cat $f 2>/dev/null)" in *dosbox*) kill -9 ${f#/proc/};; esac; done'
docker exec -d dbga bash -c "cd /work/dosbox-automation-0.85.1-7567ec4-linux-x86_64 && \
  xvfb-run -a -s '-screen 0 1024x768x24' ./bin/dosbox -conf /work/night.conf > /work/dbga_nc.log 2>&1"
export DOSBOX_API_TOKEN=...
uv run --with numpy --with pillow python3 scripts/playtest_orig_drive.py \
  --out /tmp/dbgauto/orig_nocredit --drive scripts/playtest/drive_orig_nocredit.txt --hz 6

# port side
cd port && NIGHT_CREDITS=0 NIGHT_QUIT_AT=3000 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./test/night --warp-stage 1 --script ../scripts/playtest/route_walk_fight.txt \
  --shots /tmp/port_nocredit --no-audio --frames 6000 --shot-limit 300
```
