# 59 — Per-episode sweep: all nine episodes enter, render, accept control

Reproduce with (`--warp-stage` is the host debug warp documented in docs/52 — it forces the stage number
every per-stage name is built from, because scripted play cannot clear nine episodes end to end):

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy NIGHT_BOARD=1 \
  ./build/night --script ../scripts/playtest/route_show_keyed.txt --warp-stage N \
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

## Sprite composites verified byte-for-byte in all nine episodes (this cycle)

`NIGHT_ENTSPR=1` (entity draw `fn_012efa`) dumps each composited entity frame's row-RLE stream plus the bytes the
blit wrote; `scripts/playtest_verify_spr.py` decodes the stream independently. Per episode, over
`route_show_keyed.txt --warp-stage N`:

| stage | compared bytes | mismatches |
|---|---|---|
| 1 | 48,301 | 0 |
| 2 | 48,301 | 0 |
| 3 | 50,789 | 0 |
| 4 | 48,301 | 0 |
| 5 | 46,972 | 0 |
| 6 | 48,301 | 0 |
| 7 | 48,301 | 0 |
| 8 | 49,644 | 0 |
| 9 | 48,301 | 0 |

**Total 439,190 compared bytes, 0 mismatches.** The stage-5 row was 412 mismatches on the first pass; those were
a *verifier* defect: the shipped blitter clamps at the right screen edge (`right = 0x140 - dest_off`), and the
player cell there sits at dest `0x146` with col 53 (`dest+col = 379 > 320`), so the clamped columns hold backdrop
the blit never writes. The mismatch count grew monotonically (1, 3, 8, 16, 26, 40) as the player walked further
off-screen -- the signature of clamping, not of a wrong decode. The verifier now skips columns past the clamp
(its own documented behaviour), and every episode is clean.

## Per-episode audio and pacing activity (this cycle)

Same nine runs (`NIGHT_BOARD=1`, `route_show_keyed.txt --warp-stage N`), read from the host pump counters:

| stage | retrace waits | effect transfers (xfer / stops) | CD-DA bytes streamed | track selects (`fn_010b1d`) |
|---|---|---|---|---|
| 1 | 470 | 61 / 63 | 17,023,776 | 4 |
| 2 | 418 | 61 / 63 | 18,082,176 | 4 |
| 3 | 623 | 61 / 63 | 18,082,176 | 4 |
| 4 | 316 | 61 / 63 | 17,828,160 | 3 |
| 5 | 317 | 61 / 63 | 17,832,864 | 3 |
| 6 | 469 | 61 / 63 | 17,023,776 | 4 |
| 7 | 317 | 61 / 63 | 16,774,464 | 3 |
| 8 | 367 | 61 / 63 | 17,026,128 | 4 |
| 9 | 469 | 61 / 63 | 18,084,528 | 4 |

Reading: every episode selects 3-4 Red Book tracks from the room table (`g_tab_52da4`, docs/57) and streams
16.8-18.1 MB of CD-DA through the host stream, and the DSP/DMA effect path completes transfers in all of them.
The effect-transfer count being identical (61/63) across episodes is a script artifact -- the routes press the
same key set, so the same in-game triggers fire; it is not a claim that every episode has the same sfx budget.
Retrace waits differ per episode (316-623), which is the pacing signal (docs/47): card counts and fade lengths
differ per episode, and each episode's sequence completes on the retrace/PIT clock rather than host speed.

## HUD gauge composites verified byte-for-byte in all nine episodes (this cycle)

`NIGHT_HUDDBG=1` (gauge panel in `fn_01348f`) dumps the composited gauge cell's row-RLE stream plus the bytes the
blit wrote; with `NIGHT_ENTSPR=1` in the same run, `scripts/playtest_verify_spr.py` decodes both independently:

| stage | entity-sprite bytes | HUD-gauge bytes | mismatches |
|---|---|---|---|
| 1 | 48,301 | 51,120 | 0 |
| 2 | 48,301 | 51,120 | 0 |
| 3 | 50,789 | 51,120 | 0 |
| 4 | 48,301 | 51,120 | 0 |
| 5 | 46,972 | 51,120 | 0 |
| 6 | 48,301 | 51,120 | 0 |
| 7 | 48,301 | 51,120 | 0 |
| 8 | 49,644 | 51,120 | 0 |
| 9 | 48,301 | 51,120 | 0 |

**899,270 compared bytes across the nine episodes, 0 mismatches.** Two harness bugs surfaced and were fixed on the
way, both mine:
1. the HUD dump shared the entity dump's 120-composite budget, so enabling both diagnostics starved entity
   coverage (stage 5 first reported *zero* entity composites); they now have separate budgets;
2. every dump read a fixed 512-byte window of the stream, an out-of-bounds read if an asset sits at the top of
   the heap arena. A stage-6 run segfaulted once with both flags on and did **not** reproduce in three isolated
   reruns (nor in the sweep after the bound was added), so it is not filed as a game defect -- but the dumps now
   go through `trace_stream_hex()`, which clamps to the arena, so the diagnostic can never become the crash it
   is hunting.
