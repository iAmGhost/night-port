# 106: music cluster -- anchored schedule oracle, the one real defect, and two items closed as equivalent

> **UNSOLVED (2026-10-28, user re-report on the current build).** The teardown fix in this page is real and
> stays, but the music is still wrong in play: this page's green evidence (selection LBAs, stage-relative
> deltas, 0 overlaps in the port's own stop/play witness) does NOT cover the audible symptom, so the oracle
> was insufficient, not the fix complete. Do not treat the playtest item as closed; treat "the schedule and
> witness look right but the player still hears the wrong music" as the open question, and note that the
> witness is one-sided (the shipped side's CD stop is not observable). Re-opened for a second pass with a
> two-sided audio oracle (per-window spectral/onset comparison of the container's captured WAV against the
> port's PCM, not just play/stop bookkeeping).

Date: 2026-10-28. Build before the fix: `build/night` at e3f872a. After: same tree plus the
`cdda_play` teardown. Routes: `/tmp/rig/route-e1` (ep1_combat, 230 s; shipped side shared, port side
re-dumped as `port2` = before, `port3` = after), probe route `/tmp/rig/cd-orig2` + `/tmp/rig/cd-port2`.

## New oracle: `scripts/rig/cd_schedule.py` (event-anchored CD schedule)

Both sides keep the selected track in DS: `fn_010b1d` writes `DAT_00053c44` (start LBA),
`DAT_00053c40` (end LBA), `DAT_00053c48` (length in seconds), and the port additionally witnesses every
request in `cdda.jsonl`. The script reads the shipped DS snapshots and the port's cells, maps LBA ->
track index from the port's own marks, and compares the two schedules **relative to each side's own
stage-data load** (`DAT_00053e9c` 0 -> nonzero), because drive-nominal time is only an estimate
(docs/56) -- that is exactly the mistake in docs/105 §1, which read "the port selects the room track
~2 s early" out of nominal phase windows.

route-e1, anchored (port2 = pre-fix):

```
anchor DS 0x53e9c first seen: orig t=61819  port t=60080
original:  -14585 ms lba=0x2d320 track=9   +0 ms lba=0x367aa track=11   +10593 ms lba=0x9ddf track=2
port:      -56331 ms lba=0x918e track=1    -12685 ms 0x2d320 track=9     +0 ms 0x367aa track=11  +10702 ms 0x9ddf
  #0 same lba  delta +1900 ms ok     #1 same lba delta +0 ms ok     #2 same lba delta +109 ms ok
```

So the selection *sequence*, the *LBAs* and the *track lengths* are equivalent: the port computes
`DAT_00053c48` = 42 / 369 / 238 / 261 s for tracks 1 / 9 / 11 / 2 and the shipped side holds 42 (at the
probe route) / 0x171=369 / 0xee=238 / 261 -- identical values, so `fn_010b1d`'s TOC walk
(`recs[idx].lba`, `recs[idx+1].lba - recs[idx].lba`, `len/75`) is right. **docs/96's LBA-offset
hypothesis is superseded**: nothing is off-by-a-track or off-by-a-track-start.

The periodic CD refresh is equivalent too. `fn_010d47` fires when `DAT_00063e38/490 > DAT_00053c48`;
in route-e1 the shipped tick climbs monotonically from 189 at t=72412 to 104267 at t=362752 (q reaches
213 < 261 s, so no refresh is due) and the port's tick runs at the same ~450 ticks/s (4500-4700 per
10 s bucket on both sides). On the probe route the reset count equals the selection count on both sides
(shipped 1, port 3 = its 3 selections): neither side re-arms, so the re-arm path is not a divergence.

The 80-150 s "gap" in both timelines is an alignment artifact, not a stall: on the shipped side it is
the replay's `wait ds` rebase, on the port side it is `rig_rebase` shifting the stamped clock when the
`150000 wait ds 0x63f50 >= 1` anchor fires. Do not read it as a frozen loop.

## The defect the schedule evidence leaves: no CD teardown at track replacement

The witness is unambiguous. Shipped-side windows (probe route, docs/105 §1): `DAT_00053c44` stays
`0x918e` (track 1, the intro) from t=3010 through t=144252 while the title is up; the port's marks were

```
t=3659 play index 1   lba=37262 sectors=3153      (intro, 42 s)
t=8120 play index 9   lba=185120                  <- no stop between them
t=32898 stop ; t=32901 play index 11
```

MSCDEX semantics: the play request behind `fn_010570` (request 0x1510, called from `fn_010b1d`)
*replaces* the playing track -- a drive cannot play two at once. The port reused one SDL stream and
never flushed on replacement, so the previous track's queued sectors (the planner keeps a 30-sector /
0.4 s lead) keep playing on top of the new track, and the only stops it ever emitted came from
`fn_000405a7`. Fix in `src/app/main.c` `cdda_play`: if a track is already playing, emit the stop
witness mark, `SDL_FlushAudioStream`, and clear the position before arming the new track.

Red before / green after (same shipped side, `cd_schedule.py` witness check):

```
port2 (before): 4 plays, 4 stops, 1 track change with NO stop/teardown between them
port3 (after):  4 plays, 5 stops, 0 track changes with NO stop/teardown between them
```

The schedule itself is unchanged by the fix (deltas +1915 / +0 / +204 ms), which is the right shape:
the selection logic was never wrong, only the teardown.

## Item-by-item verdict for the music cluster

| playtest sub-item | verdict | evidence |
|---|---|---|
| "intro music never stops even title appears" | **real, fixed** | no stop/teardown between `play 1` and `play 9` (port probe t=3659 -> 8120; route-e1 red=1 -> green=0). Overlap is bounded by the 0.4 s planner lead, so the audible symptom at the title is the old track bleeding across the transition rather than a track that plays forever. |
| "does not play music for slideshow" | **not reproduced; equivalent** | the slideshow track is selected at the same place as the shipped game: `DAT_00053c44` = 0x367aa (track 11) at stage+0 ms on both sides, and the port's CD-DA witness is non-silent through the whole slide window (RMS 4000-4460, silence fraction 0.00 across 5 s buckets) |
| "does not change music when game starts" | **not reproduced; equivalent** | room track 0x9ddf selected at stage+10.59 s (shipped) vs +10.70 s (port); title track 9 at -14.59 s vs -12.69 s. Residual 1.9 s on the title selection is inside the anchor tolerance (2.5 s) and is the boot/title pacing, not the CD path. |

Rig gap still open, recorded rather than papered over: the shipped side's CD **stop** is not observable
(no DS cell records it, and MSCDEX/MSCU port sampling is not wired into the rig), so the teardown
comparison is one-sided -- port witness vs shipped *selection* continuity. Wiring `/api/v1/io/port`
sampling of the IDE/MSCDEX command ports is the route if a two-sided stop oracle is ever needed.
