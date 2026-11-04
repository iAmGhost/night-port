# 96 — "Wrong music" = every CD-DA read was 150 sectors late; the track *requests* were already exact

Superseded: docs/106 and docs/114 -- the LBA-offset hypothesis did not survive the anchored schedule oracle
(same LBAs, same track lengths, deltas +1900/0/+109 ms); the real defect was the missing CD teardown at track
replacement.


Playtest item 2 (docs/manual-playtest/01-2026-10-26.md): "Wrong music for title /
stage-slideshow / main stage."

## 1. What the guest requests (Ghidra trace, docs/06 call-site census)

`fn_010b1d(k)` (orig 0x010b1d) is the only track player: it looks up its own TOC record table
`DAT_00053c64` (5-byte `{track, startLBA}` records filled by fn_107b9), sets the playing window
`DAT_00053c44..40` and `DAT_00053c48 = len/75` seconds, and asks MSCDEX to play. Request sites:

| moment | site | game track index (CD track) |
|---|---|---|
| opening movie | 0x38b02 in fn_038ae0 | 1 (CD 2) |
| **title screen** | 0x3a43d-42 in fn_03a224 | 9 (CD 10) |
| **stage start slide-show** | 0x3b084-89 in fn_03af18 (gated on `g_tab_52f2c[stage] != 0` + VESA re-set) | 11 (CD 12) |
| **room / main stage** | 0x3a863-78 in fn_03a829, table `g_tab_52da4[epData+0x40 + (stage-1)*10]` | ep1 → 2 (CD 3) |
| stage clear slides | 0x3b205-0a / 0x3b474-79 | 12 (CD 13) |
| ending | 0x3b2c6-cb | 10 (CD 11) |

All of these sites are already ported (f.c 15112, 14954, 26540, 22494, 26606+, 26462+), and the
scripted port run logs exactly that sequence (log /tmp/dbgauto/port_hold3.log):

```
cdda: track index 1 lba=37262  sectors=3153  (42.0 s)   opening
cdda: track index 9 lba=185120 sectors=27697 (369.3 s)  title
cdda: track index 11 lba=223146 sectors=17856 (238.1 s) stage-1 start slides
cdda: track index 2 lba=40415  sectors=19639 (261.9 s)  stage-1 room (episode-1 table track)
```

## 2. Measurement on the original (container, live memory — not inferred)

`scripts/playtest/read_orig_cd.py` calibrates the loaded image with the playtest_orig_drive.py
anchors (code delta=0x1ba000, data base R=0x205000 in this boot) and reads the game's own CD
globals over the REST memory API:

```
DAT_53c60 count=15; playing window at boot/opening = 37262..40415 (42 s)
TOC records at DAT_53c64 (absolute): 0, 37262, 40415, 60054, 77181, 96538, 114828, 136308,
  161147, 185120, 212817, 223146, 241002, 258870, 268621, lead-out 269453
```

Driven to the three moments (title detected at VESA 480, one Enter into the slide-show, Enters
until the mode-13h room — frames/mode through the same API):

| moment | measured DAT_53c44 / DAT_53c48 | game track | port requests |
|---|---|---|---|
| title | 185120 / 369 s | 9 (CD 10) | 9 ✓ |
| stage-1 slide-show | 223146 / 238 s | 11 (CD 12) | 11 ✓ |
| stage-1 room | 40415 / 261 s | 2 (CD 3) | 2 ✓ |

(Side evidence for the slide-show item: the original's cards only advanced when a key was injected
— the LBA stayed 223146 and mode 480 for 70+ idle seconds, then advanced on Enter.)

So the **track selection already matches the original at all three points**; the table values the
port feeds fn_10b1d are byte-identical to the measured guest table.

## 3. Root cause: the audio read addressed the IMG with MSCDEX absolute LBAs

`cd_iso_read_audio()` seeked to `disc_lba * 2352` — treating an MSCDEX absolute LBA (LBA 0 = 2 s
lead-in before the data track) as an index into TG_NIGHT.IMG, whose first byte is the TRACK 01
INDEX 01. Measured on the IMG itself: sector 37111 is the last MODE1 sector (sync
`00 FF FF FF FF FF FF 00`), 37112 is the first PCM frame of track 2 — i.e. disc LBA 37262 ↔ file
sector 37112, a constant **-150**, confirmed at the far end: measured lead-out 269453 - 150 =
269303 = the IMG's exact sector count (633400656 / 2352).

Consequence: every music track played from 2 s into itself (and track 15 ran off the end), which is
exactly "wrong music for title / slideshow / main stage" while the *requests* were right.

## 4. Fix and verification

`cd_iso_read_audio` now refuses LBAs inside the lead-in and reads `disc_lba - 150` (src/cd/cd_iso.c,
comment carries the measured anchors). test_f1 `test_cd_iso_real` gained two checks:

- `cd_iso_read_audio(37262)` returns IMG sector 37112 byte-for-byte (compared against the IMG
  opened directly) — the track-2 first frame;
- `cd_iso_read_audio(149)` → -1 (lead-in refused).

```sh
make -j8 build/test_f1 && ./build/test_f1     # ALL PASS (3063 checks)
```

Re-measure on the original (container up, token in /tmp/dbgauto/.token):

```sh
docker cp scripts/playtest/read_orig_cd.py dbga:/work/ && docker exec dbga \
  bash -c "cd /work && NIGHT_EXE=/work/game/NIGHT.EXE DOSBOX_BASE=http://127.0.0.1:8386 \
           DBA_TOKEN_FILE=/work/.token python3 read_orig_cd.py"
```

## 5. Not a guess / open residue

Nothing here needed a guess: request sites from the shipped code (docs/06 + the fn_03af18 disasm in
/tmp/dbgauto/af18_dis.txt), track windows and the TOC table measured live on the original, and the
file mapping measured on the shipped IMG. The only simplification kept: the per-frame MSCDEX play
*position* is host-pumped (docs/57), not queried by the guest — unchanged, and consistent with the
watchdog restarts (DAT_53c48 seconds vs tick clock).
