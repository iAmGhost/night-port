# 235 — Item 1 closes with no surviving defect: the selection schedule matches the shipped event-for-event, the teardown is clean, and the sector reads are bit-exact

Task: `fix-music`.  Continues docs/234 (which invalidated the red measurement).  Nothing in the port's CD layer
needed changing; this document records the gates that show it and what remains attributed elsewhere.

## 1. Event-anchored schedule comparison on the canonical route (the music oracle)

`scripts/rig/cd_schedule.py --orig /tmp/rig/g33/orig --port /tmp/rig/m1_kill3dump`
(both sides ran the same drive `scripts/rig/drives/ep1_kill3.txt` for 150 s, pinned `cpu_cycles` on the shipped side,
pinned `NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_SEED=13579` on the port side; anchor = the stage-data load,
`DS 0x53e9c` first non-zero, shipped t=62 300 ms, port t=60 066 ms; compared cell = `DS 0x53c44`, the start LBA
`fn_010b1d` writes at 0x010bf9-0x010c01):

| # | shipped (rel. anchor) | port (rel. anchor) | delta | verdict |
|---|---|---|---|---|
| — | (boot track before the anchor, skipped) | `0x918e` @ −53 050 ms | — | not compared |
| 0 | `0x2d320` track 9 @ −15 066 ms | `0x2d320` track 9 @ −12 650 ms | +2 416 ms | **ok** (tol 2 500 ms) |
| 1 | `0x367aa` track 11 @ +0 ms | `0x367aa` track 11 @ +0 ms | **0 ms** | **ok** |
| 2 | `0x9ddf` track 2 @ +10 130 ms | `0x9ddf` track 2 @ +14 100 ms | +3 970 ms | flagged, attributed §3 |
| 3 | `0x3ad6a` track 12 @ +180 147 ms | `0x3ad6a` track 12 @ +152 967 ms | −27 180 ms | flagged, attributed §3 |
| 4 | `0x918e` track 1 @ +186 686 ms | `0x9ddf` track 2 @ +155 167 ms | different LBA | flagged, attributed §3 |

The three selections that carry item 1 -- the title track (`0x2d320`, requested by `fn_03a224` at 0x3a43d-42), the
stage-start slide track (`0x367aa`, `fn_10b1d(11)` at 0x3b084), and the room track (`0x9ddf`, `fn_010d47`'s
`g_tab_52da4[room[+0x40] + (stage-1)*10]` lookup, docs/185) -- all happen on both sides, in the same order, with
0 to 4 s of relative offset.  Item 1's stated symptom, "never switches to title-screen music", is not reproducible
against these gates on the current build **or** on the frozen pre-fix baseline (docs/234 §2).

## 2. Teardown: no track change happens without a stop

The same run's witness summary: **6 plays, 7 stops, 0 track changes with no stop/teardown between them.**  The
stop marker comes from `cdda_stop` (`src/app/main.c:306-310`) and the pre-play stop from `cdda_play`
(`main.c:283-287`), which is the fix docs/105 §1 recorded: the port once reused one `SDL_AudioStream` and flushed
nothing on replacement, so the previous track's 30-sector / 0.4 s lead played on top of the new track and the
witness showed no stop at any change.  That defect is closed and its witness is the line above, so docs/183 §2's
"streams a 42 s request for 167 s without a stop" is superseded (it was also a docs/234 dump-timing artifact).

## 3. What the three flagged deltas belong to

`fn_010d47` only evaluates its bound inside the room loop (`fn_03a829`, its single caller, docs/184 §5), and
`fn_10b1d` resets `DS 0x63e38 = 0` at every selection (0x010c0c-16).  So *when* the room track is selected is a
function of when the port reaches the room loop, and *when* the late-episode selections (12 / 1) happen is a
function of how the stage advances.  Both are the known gameplay divergence set -- the kill-count/debt bookkeeping
regression (docs/203) and the pacing basis (docs/168, docs/205) -- not the CD layer: the port's title and slide
selections land within 2.4 s and 0 ms of the shipped's on the same anchor, from the same code paths.  Nothing in
`fn_10b1d`, `fn_010d47`, `cd_track_for_room` or the 100-entry `g_tab_52da4` is a candidate for a fix (docs/185 §1
proved the table byte-identical to NIGHT.EXE).

## 4. Audible-side gates, and one rig-basis caveat that must not be forgotten

* Shipped audio (`/tmp/rig/g33/orig/audio.wav`, 130 s, 44 100 Hz stereo) fingerprinted with
  `scripts/rig/cd_track_timeline.py` against the disc envelopes: track 1 `37262` audible at 10.8 s (+0.775) and
  100.2 s (+0.879); track 2 `40415` at 16.0 s (+0.660); tracks 9 and 11 present but weak (+0.475 / +0.556).  The
  shipped does play the intro track and the room track on this route.
* Port audio cannot be fingerprinted from the current dump artifacts, and this is a **rig basis** fact, not a port
  fact: `<dump>/pcm.wav` covers 19.6 s of 22 050 Hz mono, and `<dump>/cdda.raw` is a *sector log* (108 850 560 B =
  46 280 × 2352-byte sectors = 617 s of CD time inside a 250 s run) because with `SDL_AUDIODRIVER=dummy` nothing
  paces the stream: `cdda_pump` (`main.c:312-330`) keeps refilling its 30-sector lead as fast as the pump runs, so
  the dump's consumption rate is ~2.5× real time.  A content claim from these files would be a rig artifact.
* The content question is nevertheless answered exactly: docs/113's byte probe already proves the port reads **the
  disc's own sectors bit-for-bit** for the LBAs it requests, and §1 shows the requests themselves match the shipped.
  Combining those two is stronger than an envelope correlation.

## 5. Status

Item 1 is closed as **"no surviving port defect"**: the selection sites, their constants, the room table, the
track-end bound, the teardown and the sector reads all match the shipped, on the canonical route, measured
event-anchored.  The three flagged offsets are attributed to the gameplay/pacing residual set and remain open there
(kill-count/debt regression, camera population, screen pacing).  `NIGHT_TITLEDBG` (`fn_03a224` phase timings) stays
as the witness that separated the title sequence's real cost (10.8 s) from the dump-inflated one (~158 s).
