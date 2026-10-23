# 59 — Per-episode sweep: all nine episodes enter, render, accept control

Reproduce with (`--warp-stage` is the host debug warp documented in docs/52 — it forces the stage number
every per-stage name is built from, because scripted play cannot clear nine episodes end to end):

```
cd port && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_BOARD=1 \
  ./test/night --script ../scripts/playtest/route_show_keyed.txt --warp-stage N \
  --shots /tmp/swN --no-audio --frames 9000
```

`route_show_keyed.txt` is the keyed route: it anchors on the picture-show resource names
(`na<stage>NNNN.pcx`, docs/58) and then paces keys, which is required because `fn_03a70e` waits for a key
press per picture with the keyboard pump off.

## Result (exit code 0 for all nine, every run reached `playing`)

| stage | card / intermission seen | live entities at `playing` | effects: transfers / bytes | CDDA: track selects / bytes consumed | outcome |
|---|---|---|---|---|---|
| 1 | `stage1.pcx`, cards 1s1..1s3 | 3 | 61 / 220 027 B | 4 / 17 026 128 B | plays |
| 2 | `stage2.pcx`, cards 2s1 2s2 | 4 | 61 / 220 027 B | 4 / 18 084 528 B | plays |
| 3 | `stage3.pcx`, cards 3s1..3s6 | 4 | 61 / 220 027 B | 4 / 18 082 176 B | plays |
| 4 | `stage4.pcx`, no cards shipped | 4 | 61 / 220 027 B | 3 / 17 830 512 B | plays |
| 5 | `stage5.pcx`, cards 5e1..5e4 | 4 | 61 / 220 027 B | 3 / 17 825 808 B | plays (was SIGSEGV, docs/52) |
| 6 | `stage6.pcx`, cards 6s1..6s3 | 4 | 61 / 220 027 B | 4 / 17 023 776 B | plays |
| 7 | `stage7.pcx`, card 7e1 | 4 | 61 / 220 027 B | 3 / 16 767 408 B | plays |
| 8 | `stage8.pcx`, card 8s1, **picture show `na80001.pcx`** | 2 | 61 / 220 027 B | 4 / 17 026 128 B | plays (was a deadlock, docs/52) |
| 9 | `stage9.pcx`, cards 9s1..9s3 | 1 | 61 / 220 027 B | 4 / 18 086 880 B | plays |

Screenshots: `docs/manual-playtest/04-shots/playing_stageN.bmp` (the `playing` frame of each run, 320x200
game view).

## What the columns mean, and what is the same across episodes

- **Background**: every episode presents its own `stageN.pcx` card before the game view; the entity
  counts differ (stage 1: 3, stages 2–7: 4, stage 8: 2, stage 9: 1), matching the room/spawn data the
  warp feeds the same loader — the per-type entity data is decoded independently in
  `scripts/decode_stg.py` (docs/52).
- **HUD**: the gauge is the same code path in every episode (`fn_01348f` + the panel sprites), positioned
  as measured in docs/50 (fill at columns 23..122, rows 31..32), so the docs/50 fix applies to all nine.
- **Effects**: identical counters for every stage (61 transfers, 220 027 B of 8-bit PCM at 10989 Hz,
  `pend=0`) because the *script* drives the same set of actions in each run — the stage does not change
  which effects fire in a scripted route. It is not evidence that a stage has no unique sounds, only
  that the harness's action set is the same.
- **Music (CDDA)**: 3 or 4 track selects per run and 16.7–18.1 MB of Red Book bytes consumed, chosen from
  the per-room table `g_tab_52da4` (docs/57). Stage 4/5/7 show 3 selects, the others 4 — consistent with
  the table's per-room track values.
- **Retrace waits** (the retrace-paced fade model of docs/47) vary per stage (316–622), tracking how many
  fade/composite frames each episode's startup shows.

## Two per-episode intermission mechanisms (don't conflate them)

1. `<stage>s<N>.pcx` / `<stage>e<N>.pcx` **cards** — counted per episode in docs/58 (stage 4 ships none).
2. `na<stage>NNNN.pcx` **picture shows** via `fn_03a70e` — confirmed live for stage 8; the wait for a key
   per picture is what deadlocked stage 8 before the cooperative-pump fix.

## Still open in this task

Original-side frames per episode for pixel comparison (blocked on the DOSBox capture path, docs/46), and
completing the feature sweep (pause / level-up / game-over screens) which needs the same capture path for
its comparison half.
