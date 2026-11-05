# 363 — the nine-episode sweep on the Pocket code path, and the arena witness that made it comparable

Task-8's contract is docs/59's episode sweep replayed on the Pocket build, with the slide cards compared against the
shipped assets (docs/62 / docs/63).  Doing it needed two host capabilities the Pocket build did not have, and one
measurement choice that docs/62 had already argued for.

## 1. What had to be added first

`pocket/src/night/main_of.c` (all of it `#ifdef OF_PC`, so the device ELF never sees any of it):

- **The state half of the rig sink.**  It called `rig_screen_init()` only, so a Pocket-host dump held frames and
  nothing else -- which is why docs/333 could compare the two hosts' *mean RGB* and no more.  It now calls
  `rig_init()` / `rig_poll()` / `rig_close()`, so the same `src/app/rig_sink.c` sidecars the SDL host produces
  appear on the Pocket path: `cells.bin` (14 MB of the DS cell map per 2000 frames), `records.jsonl` (the entity
  list followed through its `+0x13f` links, per sample), `player_recs.jsonl`, `cdda.jsonl`, `meta.json`.
- **`--rig-hz` and `--rig-witness N`.**  Full-picture witnesses are 4 bytes a pixel; a nine-stage sweep does not
  need them when the arena raws are what the comparison reads, so `--rig-witness 0` keeps the dump to tens of MB
  while `frames.bin` still carries every presented frame's 80×60 luma signature.
- **`NIGHT_VIDRAW=1`, the arena witness.**  Next to each sampled frame the host writes `arena/<frame>_<W>x<H>.raw`:
  the `vw*vh` **index bytes** of `g_video_ram`, then `#base_delta` (the shipped back-screen base `DAT_633b0` minus
  the arena base), `#ents` (`DAT_53c68`, the game's own entity count) and the 768-byte DAC table as the host holds
  it.  That is docs/62's argument implemented on this host: *arena-vs-card*, because `fn_040660` ramps a card in
  over `0x19` steps, so an RGB sample mid-fade cannot match the card's trailer palette at all, while the arena is
  byte-identical from the first frame that holds the card.  Bounded: every 8th presented frame, at most 120 raws.

## 2. The route, and why it is reproducible

`scripts/pocket/sweep_episodes.sh` runs `app_pc --warp-stage N --frames 4500 --rig-dump <dir> --rig-witness 0` for
N = 1..9, fixed `NIGHT_SEED=1234`, dummy video/audio, `NIGHT_CD_DIR=scratch/pocket/cd`.

docs/333 §5 recorded that wall-clock pad scripts are unstable on this host (a Start press lands a few ticks off the
key-wait it was meant to satisfy, and the run parks in an opening↔title loop), and that the SDL host's answer --
`wait res` / `wait ds` anchors -- does not exist here.  What the Pocket host does have is the **warp entry pump**, and
it is anchored on game state, not on the clock: it posts a 250 ms confirm pulse only while `fn_03272f` (the player
action resolver) has never been called, and stops the moment the stage loop owns frames.  The pulse counts came out
18–31 per episode and every run printed `warp entry: stage loop reached`, which is why nine runs are reproducible
without an anchor system.

`scripts/pocket/sweep_report.py` reads the witnesses back.  Its card test is `identical(arena, card_indices)` --
byte-for-byte against the independent PCX decode (128-byte header, the 6-bit-count RLE, destination size from the
header box), the same decoder docs/62 wrote.

## 3. Result: 9/9 episodes entered and ran clean, 27 arena samples byte-identical to shipped cards

```
== 9/9 episodes entered and ran clean ==   (reproduce: FRAMES=4500 scripts/pocket/sweep_episodes.sh)
```

| ep | warp pulses | `fn_037b03` room chain | stage boss | views seen | SFX blocks / PCM frames | max live entities | cards byte-identical to the shipped PCX | episode CD track |
|---|---|---|---|---|---|---|---|---|
| 1 | 25 | 4 rooms | @(4061,369) | 640×480:10, 320×200:72 | 22 / 82 515 | 4 (types 0×1,1×2,2×1) | `1s1 1s2 1s3 stage1` @frames 208/280/336/392 | idx2 ✓ |
| 2 | 23 | 4 rooms | @(7096,166) | 9, 73 | 12 / 56 382 | 9 (0×1,1×5,2×3) | `2s1 2s2 stage2` @208/280/336 | idx3 ✓ |
| 3 | 31 | **7 rooms** | @(6403,186) | 13, 69 | 23 / 98 690 | 7 | `3s1 .. 3s6 stage3` @208..552 | idx9 ✓ |
| 4 | 18 | 2 rooms | @(1244,464) | 6, 76 | 7 / 34 527 | 7 | `stage4` @208 | idx4 ✓ |
| 5 | 18 | 1 room | @(97,141) | 6, 76 | 18 / 83 146 | 7 | `stage5` @208 | idx5 ✓ |
| 6 | 25 | 4 rooms | @(7207,478) | 10, 72 | 16 / 70 810 | 12 | `6s1 6s2 6s3 stage6` @208/280/336 | idx6 ✓ |
| 7 | 18 | 3 rooms | @(7218,467) | 6, 76 | 13 / 58 939 | 12 | `stage7` @208 | idx7 ✓ |
| 8 | 22 | 1 room | @(238,174) | 8, 75 | 10 / 34 987 | 5 | `8s1 stage8` @208/280 | idx8 ✓ |
| 9 | 25 | **8 rooms** | @(12264,1383) | 10, 72 | 2 / 10 375 | 1 | `9s1 9s2 9s3 stage9` @208/280/336 | idx2 ✓ |

Every identical row is **307 200 / 307 200 bytes**, the same criterion and the same number docs/62 measured on the SDL
host.  The slide *inventory* per episode also agrees with docs/58: episodes 4, 5 and 7 ship no intro cards and none
appear; episode 8 ships exactly `8s1`; episode 3 has the longest intro run (six cards, ending at frame 496 before
`stage3.pcx` at 552).  Episodes differ the way the shipped data says they should: 4/4/7/2/1/4/3/1/8 rooms in the
`fn_037b03` chain, bosses at different world coordinates, 1 to 12 live entities walked out of `records.jsonl`.

**The music selects are checked against the shipped table, not against a guess.**  The reporter reads
`g_tab_52da4[100]` out of `src/f/f.c` (DS `0x52da4`, ten entries per episode) and requires the run's selects to
contain one of that episode's values:

```
stage 1: table allows [2, 8], selects were [1, 9, 11, 2]      -> IN TABLE
stage 3: table allows [8, 9], selects were [1, 9, 11, 9, 12]  -> IN TABLE
stage 9: table allows [2, 8], selects were [1, 9, 11, 2]      -> IN TABLE   (episode 9's row is 0x02, not a new track)
```

`1` (opening), `9` (title), `11` (stage-1 start slides) and `12` (stage-clear slides) are docs/96's fixed selectors;
the episode row is what differs per stage, and all nine match.

## 4. What this sweep does *not* show, said plainly

- **The `na<stage>NNNN.pcx` picture shows (fn_03a70e) were not reached.**  Their best arena match is 0.2–0.8% of a
  320×200 sample, i.e. nothing.  docs/59 reached `na80001.pcx` on the keyed route; the warp-entry route enters the
  stage through the confirm pulses and does not reproduce the CG show.  That is a route gap, not a loader gap.
- **No ending was reached**, so the `e`-cards and the `2end0001..2end0010` family remain inventoried-but-unverified
  (docs/62 said the same).  Best scores against them are 1.7–6.6%, which is what an unrelated 640×480 picture of the
  same artwork family looks like, not a match.
- **Entity counts are not directly comparable with docs/59's** (3/4/4/4/4/4/4/2/1 there): docs/59 counts the entities
  visible at its `playing` screenshot on the keyed route, this table reports the maximum live count walked from
  `records.jsonl` over the whole 4500-frame run on the warp route.
- The stage-palette divergence docs/333 recorded is untouched: the port's stage frames read `255/97/97` on both hosts
  while the DOSBox original reads a grey-blue station scene.  It is a shared-`src/f` defect inherited by this host, and
  it is carried in task-11.
- Level-up and continue screens (in task-8's title, not in its contract) have not been swept on the Pocket path.

## 5. Performance numbers this sweep added, on the desktop shim

Present cost maxima per episode: 2 467 / 2 535 / 2 930 / 3 185 / 4 866 / 8 977 / 13 053 / 16 666 / 2 898 µs; tick rate
+491/s against `pit=490 Hz` in all nine; arena high-water 3.90–5.25 MB of the 16 MiB budget.  Nothing in the sweep
approached the 8-slide/scaler limits, and no run dropped a SFX block that it did not also report.

Core unchanged at 0.1.53 for the sweep (the rig additions are desktop-only); the fixture suite still passes; `src/f`
untouched.
