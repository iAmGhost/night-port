# 183 — Item 1 heard from the rig: the port holds one track, the shipped game moves to another

Item 1 of `docs/manual-playtest/03-2026-10-29.md`: *"Music is still wrong. Intro music plays, skips with keypress
then does not switch to title screen music. But track advances to next when the track ends."*  This is the blind-gate
measurement on the unfixed build (`574c636`), drive `scripts/rig/drives/ep1_title_music.txt`, both sides, no human
playtest involved.

## 1. Two rig facts that made the measurement possible

**(a) A route is as long as its last scripted event, not as long as `--frames`.**  `main.c:1164`:

```c
if (g_pt_done_ms >= 0 && pt_now_ms() - g_pt_done_ms > 2500) g_quit = 1;   /* script exhausted: wrap up */
```

`g_pt_done_ms` is set when the last scripted event fires (`main.c:746`).  The first version of this drive ended at
40.06 s, and the port's dump stopped at **42.5 s** — exactly last-event + 2.5 s -- while `--frames 15000` and a 7 200
frame budget were both nominally larger.  A 120 s request produced a 42.5 s route, silently.  The drive now carries
two read-only predicates (`70000 wait ds 0x53c44 >=0`, `100000 ...`) purely to extend the timeline without touching
game state.

**(b) The capture session now survives a VGA mode change** (`34e8697`, docs/181 §6): this route captured
3 720 frames / 266 DS witnesses on the original side where the previous 150 s run died with `IndexError`.

## 2. Selection sequence, same drive, both sides

Canonical route for this item: `OUT=/tmp/rig/g36`, `scripts/rig/route.sh scripts/rig/drives/ep1_title_music.txt 180`.

Shipped side, 4 Hz, 582 states, t = 25..279 795 ms (`0x53c44` = selected track start lba, `0x53c48` = sectors):

| state | ~time | selected | extent |
|---|---|---|---|
| 0 | 0 s | `0x2d320` | 369 s |
| 3 | ~0.8 s | `0x367aa` | 238 s |
| **122** | **~30.5 s** | **`0x9ddf`** | **261 s** |

… and nothing further for the rest of 280 s: three selections, the last one stable.

Port side, 10 Hz, 1 800 samples, t = 16..239 850 ms:

| time | selected | extent |
|---|---|---|
| 0.1 s | none (`0`) | — |
| 14.1 s | `0x918e` | 42 s |
| 180.1 s | `0x2d320` | 369 s |
| 226.3 s | `0x918e` | 42 s |

Play/stop witness (`cdda.jsonl`, 9 markers): `stop` 12.9 s, `play 1 (lba 0x918e, 42.0 s)` 12.9 s, `stop` 179.6 s,
`play 9 (369.3 s)` 179.6 s, `stop` 214.1 s, `stop` 226.1 s, `play 1 (42.0 s)` 226.1 s.  The teardown invariant still
holds (every replacement is preceded by a stop, docs/106), so this is not the docs/106 defect reappearing.

Three differences, all measurable:

1. **The shipped game has already selected a track at its first sample; the port's first selection is 12.9 s in.**
2. **The shipped game reaches `0x9ddf` (261 s) at ~30 s and stays there.  The port never selects `0x9ddf` at all** in
   a 240 s route.  `0x9ddf` is the selection that corresponds to the title screen.
3. **The port holds one selection far past its own extent**: track 1 is requested as 3 153 sectors = 42.0 s at
   12.9 s and is not stopped until 179.6 s — 166.7 s of a 42 s request, with CD-DA bytes delivered continuously the
   whole time (first sample 13.0 s, last 231.1 s, 66 650 976 B = 377.8 s of audio, **zero** stretches with no new
   bytes).  So "the track ends and the game asks for the next one" is what the *shipped* side does and the port does
   not: the port's stream simply runs on.

## 3. The audible half, fingerprinted out of the disc

`music_melody.py` (peak-set IoU; positive control 0.966, negative band 0.19-0.43 per docs/131) against the shipped
mixer capture `/tmp/rig/g36/orig/audio.wav` (28 339 936 B) with templates taken straight from
`NIGHT/TG_NIGHT.IMG`:

| template | LBA | best mean peak-set IoU | where | verdict |
|---|---|---|---|---|
| track 1 | 37262 (`0x918e`) | **+0.317** | capture 43.6 s | negative band |
| track 9 | 185120 (`0x2d320`) | **+0.314** | capture 52.2 s | negative band |
| track 11 | 223146 (`0x367aa`) | **+0.370** | capture 20.0 s | negative band |
| **title track** | **40415 (`0x9ddf`)** | **+0.837** | **capture 55.4 s** | **present** |

The track the shipped game selects at ~30 s is the one its mix audibly contains from ~55 s.  The track the port
selects for 167 s is not the one shipped plays at that point.  Note the selection→audibility lag on the shipped side
(~30 s selected, ~55 s audible): that gap is the shipped mixer/planner behaviour, and it is a separate question from
the port never making the selection at all.

## 4. Where the fix has to go

The port's selection code asks for a track extent and then hands sectors to the device without ever concluding the
request: the witness shows a 42 s extent producing 167 s of delivery.  The shipped side's behaviour is expressible
through the cells the rig already samples (`0x53c44` start lba, `0x53c40` end lba, `0x53c48` sectors): a selection is
bounded, and a title-screen state produces a *different* selection.  So the fix must answer two separate questions,
in this order:

1. what bounds a play request in the shipped driver (does it stop at `end lba`, and who requests the next one);
2. what state transition on the shipped side asks for `0x9ddf`, and whether the port models that transition.

Both are answerable from the disassembly, and `cd_schedule.py` + `music_melody.py` are the before/after diff.

## 5. One open item this run re-opened

`route.sh` still reports `report_audio.txt: No such file or directory` even though both audio oracle steps ran
("audio oracle: board/SFX stream", "audio oracle: CD-DA stream").  The board/SFX half has nothing to score on a
title route (no combat SFX are triggered), but the route should say that instead of `cat`-ing a missing file.  This
is a rig reporting defect, not a port defect; it is recorded here because a missing report is how an audio question
gets mistakenly closed.
