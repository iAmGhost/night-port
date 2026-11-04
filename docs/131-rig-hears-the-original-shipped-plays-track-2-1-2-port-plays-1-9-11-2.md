# 131 — The rig can finally hear the original: the shipped game plays track 2 → 1 → 2, the port plays 1 → 9 → 11 → 2

docs/130 fixed the capture rate and left the question open: which CD-DA is *audible* in the shipped mix, and
when. This turn produced a fingerprint that separates present from absent, and the answer reverses the picture.

## 1. A selective fingerprint: melodic peak-set IoU

`scripts/rig/music_melody.py` keeps only what survives a mixer: per 1 s window (0.2 s hop), the strongest few
spectral peaks binned to semitones, then mean intersection-over-union of that set sequence across a lag.

Separation measured on the same material (template = the port's own CD witness, which is byte-faithful to the
disc, docs/114):

| pairing | mean peak-set IoU |
|---|---|
| track template vs the port's own stream (positive control) | **0.966** |
| same template vs the shipped capture at the port's play moment | 0.330 |

Onset-train correlation (docs/130) could not separate these (hypotheses 0.00–0.47, controls 0.19–0.59); the
peak-set fingerprint puts the control at 0.97 and the negatives at 0.19–0.43, a 2.3× gap with a clean floor.

## 2. Shipped-side audible music, `ep1_kill3`, 266 s capture

For each disc track the game's schedule mentions, several template offsets, best match and its capture time:

| lba (game index) | template offset | best IoU | capture t | implied track start |
|---|---|---|---|---|
| 40415 (idx 2, stage) | +0 s | **0.987** | 141.4 s | 141.4 s |
| 40415 | +15 s | **0.914** | 27.4 s | 12.4 s |
| 40415 | +25 s | **0.949** | 37.4 s | 12.4 s |
| 40415 | +30 s | **0.938** | 42.4 s | 12.4 s |
| 40415 | +45 s | **0.940** | 185.4 s | 140.4 s |
| 40415 | +60 s | **0.912** | 200.4 s | 140.4 s |
| 40415 | +90 s | **0.849** | 230.4 s | 140.4 s |
| 37262 (idx 1, intro) | +0/+15/+30 s | 0.39 / 0.41 / 0.43 | – | absent |
| 37262 | +45 s | **0.962** | 144.4 s | 99.4 s |
| 37262 | +60 s | **0.988** | 158.4 s | 98.4 s |
| 185120 (idx 9) | +0/+25/+60 s | 0.25 / 0.35 / 0.22 | – | **absent** |
| 223146 (idx 11) | +0/+25/+60 s | 0.40 / 0.19 / 0.35 | – | **absent** |

The implied starts are internally consistent with **slope exactly 1** (offset +15 s moves the match +15 s), so
the emulator's CD path plays at the right tempo and the positions are trustworthy. Reading the table:

```
shipped (capture seconds):  12.4 s  track 2 begins  ... 99 s track 1 begins ... 140.4 s track 2 begins again
port    (own timeline):      3.5 s  track 1 (41.6 s) -> 47.4 s track 9 (cut after 27 s) -> 50.2 s track 11
                             (104 s) -> 70.9 s track 2 (runs to the end)
```

Three concrete divergences, all rig-detected, none of them visible to the schedule oracle:

1. **Track 11 is audible for 104 s in the port and never audible in the shipped mix** (0.19–0.40 across three
   template offsets). docs/106 and docs/114 called the schedule equivalent because the shipped DS cells *also*
   record a play request for that lba — the request exists, the audio does not appear. That is the class of bug
   a schedule comparison structurally cannot see.
2. **Track 2 (the stage track) is audible from 12.4 s in the shipped capture — before anything else** — while the
   port starts it only at 70.9 s and plays the intro track first. "No music for slideshow / no change when the
   game starts" in the user's report maps onto this ordering.
3. **The shipped mix replays track 2 from its start at ~140 s**, i.e. an explicit restart of the same track; the
   port never restarts a track (it plays each request once, sequentially).

## 3. What is now localized, and the next measurement

The port issues all CD plays through `cdda_play()` in `src/app/main.c`, fed by the DS selection cells
(`0x53c44` track index, `0x53c40` lba, `0x53c48` sectors, docs/106). Since the shipped side *requests* track 11
and produces no track-11 audio, the shipped game must be aborting/replacing that request before the drive
stream reaches the mixer — or the port is emitting audio for a request the shipped drive silently dropped.
The next measurement is therefore on the DS side, not the audio side: sample `0x53c44/0x53c40/0x53c48` at
high rate on both sides across the request window, plus the drive's own state, to see whether the shipped
request is cancelled within a few hundred ms. The rig now has both halves — audible presence and request
state — so this can be answered as a differential statement.

## 4. Phase-locking the two audio timelines (the capture does not start where the port's process starts)

The container session boots the game, calibrates, and only then starts sampling, so capture seconds are offset
from the port's process seconds. The DS cells give the anchor: on the shipped side `0x53c44 / 0x53c40 / 0x53c48`
current-lba/sectors changed exactly **once in 0–60 s, at capture t = 48.305 ms+48 s**, from lba `0x2d320`
(track 9, 0x171 = 369 s) to `0x367aa` (track 11, `0xee` = 238 s); the port issues the same pair of requests at
47.354 s and 50.190 s. Taking the track-11 request as the shared event gives an offset of about **+1.9 s**
(shipped capture = port time + 1.9 s), so the audible positions translate to:

| audible event (shipped) | capture s | port-equivalent s | what the port does there |
|---|---|---|---|
| track 2 begins | 12.4 | **10.5** | still inside the intro track (played 3.5–45.2 s) |
| track 1 begins | 99 | **97** | inside track 11 (50.2–154 s) |
| track 2 begins again | 140.4 | **138.5** | inside track 11, no restart at all |

Two claims that survive the anchoring, both rig-only findings: the shipped game has **stage-track audio at
port-equivalent ~10.5 s**, where the port is still playing the intro track; and the shipped game **restarts the
stage track mid-stage**, which the port never does. The track-11 absence stays open: the shipped side requests
it at 48.305 s (cell value `0x367aa`, 238 s) yet no track-11 audio exists anywhere in the 266 s capture, so the
request must be superseded before the drive's stream reaches the mixer — measuring that needs sampling above
the 4 Hz dump cadence, i.e. a dedicated high-rate pass over 48–72 s on both sides.

## 5. Replication on an independent run, and a cross-check from the state oracle

A second `ep1_kill3` session (`/tmp/rig/g6`, 200 s, fresh boot, corrected 44100 capture) reproduces the two
headline results with an independent fingerprint run:

```
lba 223146 (track 11): best peak-set IoU +0.380   -> absent, as in g5 (0.19-0.40)
lba 40415  (track 2) : best peak-set IoU +0.766   -> present, as in g5 (0.85-0.99)
```

The port's play list on that run also shows the restart mechanism working *in the port*, just at a different
moment than in the shipped mix:

```
play t=  3.53s idx=1  lba=37262  (42.0 s)     play t=234.27s idx=12 lba=241002 (238 s)
play t= 47.35s idx=9  lba=185120             play t=235.58s idx=2  lba=40415   <- stage track RESTARTED
play t= 50.16s idx=11 lba=223146
play t= 70.48s idx=2  lba=40415
```

So `fn_010d47` (the guest's periodic CD refresh at 0x010d47, which re-selects the room track once
`g_dat_63e38/490` exceeds `g_dat_53c48`, the current track's seconds) is wired and fires — the shipped game
restarts the stage track at port-equivalent ~138.5 s, the port at 235.6 s. The mechanism is not missing; the
*timing* and the *audibility of the intervening requests* are what diverge.

Cross-check from the state oracle on the same run: the CD selection cells are now flagged
(`DIFF 0x53c40`, `DIFF 0x53c44` — the current-track lba pair, docs/106), while the activity-profile oracle
still scores **269/269 SAME**, i.e. the two sides change the same cells in the same phases but hold different
track values for part of the route. The full state tally for g6 was `{'SIZE?': 32, 'PTR': 52, 'SAME': 53,
'REPR': 22, 'DIFF': 110, 'ENTROPY': 14, 'REC-DIFF': 36, ...}` over 344 cells, which is far worse than the
269/269 of the g2 route and needs its own triage (phase drift between the two 200 s sessions vs real value
divergence).

