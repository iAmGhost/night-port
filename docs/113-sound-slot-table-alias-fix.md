# 113: no punch/hit sound ever played -- DS 0x53d08 is the voice pool, one dword low

## The shipped path (guest 0x0003ab31-0x0003ab60, inside `fn_03a829`)

```
CMP  dword [0x63364], 0            ; the element-event latch (set in fn_01441e from byte[record+slot+0x63])
JZ   skip
CMP  dword [0x63368], 0            ; repeat/hold counter
JNZ  decrement
MOV  EAX, [0x63364]
SHL  EAX, 0x2
MOV  EAX, dword ptr [EAX + 0x53d08]
CALL 0x4185e                       ; play the sound object
MOV  dword [0x63364], 0
```

So the event index is **1-based into a dword table whose base is `DAT_00053d08`**. The master loader
`fn_03690b` writes that table at base `0x53d0c` (guest `0x00036ef9` `MOV [EDX + 0x53d0c], EAX`) for the 100
voice slots `"1.voc".."100.voc"` -- the same region, 4 bytes higher. Hence `entry(ev) == pool[ev-1]`, and the
measured shipped event ids (`DAT_00063364` fires with **1** and **19**) land on `pool[0]` and `pool[18]`,
exactly the slot numbering the loader uses.

## What the port did

The port modelled `0x53d08` as its own object, `uint32_t g_tab_53d08[101]`, which **nothing ever wrote** (one
declaration, one read). The call site therefore always passed null and no hit, punch or death sound could
ever play -- the playtest item "missing punch SFX". The static audit had already filed the modelling error
independently: `c2_aliasing` reports `port-span-overlap g_tab_53d08 [0x53d08,0x53e9c]` overlapping
`g_dat_53d0c` (100 entries), `g_dat_53e24` (10) and five `g_dat_53dd*` globals, and `c7_ds_table_bases`
shows base `0x53d08` has exactly **one** guest access site while `0x53d0c` has six across five functions.
This is the same one-address-two-arrays defect as docs/112 (`g_pool_53d0c`) and docs/109 (`g_dat_53ea8`).

## Fix

`g_tab_53d08` is deleted (with `g_pool_53d0c` already gone), and the event play now reads the real pool with
the shipped 1-based mapping, counting anything outside the 100 entries instead of reading past it:

```c
uint32_t ev = g_dat_63364;
if (ev >= 1u && ev <= 100u && g_dat_53d0c[ev - 1u]) { fn_04185e(g_dat_53d0c[ev - 1u]); g_sfx_plays++; }
else g_sfx_oob++;
g_dat_63364 = 0;
```

## Red before / green after (drive `ep1_kill3`, 14000 frames, `NIGHT_ACTTRACE=1`)

```
before: the play argument was an unfilled table -> always null; 0 sounds reachable
after:  element-event sounds: played=74  outside-pool=0
shipped reference (sampled at 6.5 Hz, so a floor): DAT_00063364 fires 4 times, ids {1, 19} -> pool[0], pool[18]
```

`make -C port test` ALL PASS, `make selftest` ALL PASS.

## Still to verify for this item

The rig has not yet A/B'd the *audio* of a combat window (container mixer capture vs the port's PCM sink, per
docs/103) -- that is the remaining presence proof for "missing punch SFX"; the state-side evidence (a sound
object actually reaches `fn_04185e` 74 times) is in place.

## Port-side audio output before/after (the rig's own PCM byte counter, same drive and seed)

The rig dump records the PCM sink's cumulative byte count per sample, so the SFX fix is visible without any
human listening -- compare two runs of `ep1_kill3` at 6 Hz (`/tmp/rig/rx1` is the pre-fix build,
`/tmp/rig/rx2` the fixed one):

```
rx1 (before): 477,970 audio bytes,  54/1308 samples emitting audio   per-60s: {0: 460804, 180: 17166}
rx2 (after): 1,135,130 audio bytes, 137/1302 samples emitting audio  per-60s: {0: 460804, 60: 36070, 120: 21642, 180: 319972, 240: 296642}
```

2.4x more audio in total, and -- the part that matters -- the extra output lands in the 120-300 s stretch,
which is exactly where this drive fights (the kill counter moves at 231 s, `NIGHT_HITTRACE` hits cluster
there). Before the fix the combat stretch was silent apart from the music track. The shipped-side mixer A/B
over the same window (`--audio-capture` + `diff_audio.py`) is the remaining presence proof for this item.

## Shipped mixer A/B over the same route (route.sh, `ep1_kill3`, 300 s) -- a RIG gap, not a game result

`OUT=/tmp/rig/rt-sfx scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 300` captured the container mixer
(`orig/audio.wav`, 46,906,916 B) and the port's PCM witness (`port/pcm.wav`, 890,824 B) and ran
`diff_audio.py` per window:

```
P180@247800  orig onsets=1 active=1.00 rms= 821.8   | port onsets=0 active=0.00 rms=0.0   [SILENT]
P181@248220  orig onsets=1 active=1.00 rms= 596.4   | port onsets=0 active=0.00 rms=0.0   [SILENT]
P182@248640  orig onsets=1 active=1.00 rms= 898.6   | port onsets=0 active=0.00 rms=0.0   [SILENT]
... 4 windows flagged, every combat-window row on the port side reads SILENT
CD-DA schedule: port 3.4s/47.4s/50.1s/70.9s vs original 10.0s/95.5s/43.0s/52.0s, corr 0.15-0.21 [NOT FOUND]
```

Read this as a rig defect first: the state-side witness on the very same build plays 74 sound objects and the
port's own PCM byte counter (docs/113 §before/after) rises by 660 KB concentrated in that stretch, yet the
WAV the witness writes for the route contains almost nothing (890 KB over 300 s is ~4 s of 22 kHz 16-bit
audio, and it is empty exactly where the counter says audio was produced). So the PCM witness and the counter
are not the same stream: the witness is not capturing the mixer path that `fn_04185e` feeds (nor the CD-DA
stream, which is also why every track fingerprint comes back NOT FOUND against the original mix).

Consequences for the goal: (1) the "missing punch SFX" item keeps its state-side green signature
(plays 0 -> 74, PCM counter 478 KB -> 1.14 MB) but has **no audio-oracle proof yet**; (2) the audio oracle's
port half is a rig gap to fix before `closeout` can claim the audio diff green -- the witness must tap the
device/mixer stream that carries SFX and the CD stream, not whatever sink produced `pcm.wav` here; (3) the
music cluster (already re-opened by the user as UNSOLVED) will need that same tap.

## Envelope alignment across offsets (scripts/rig/audio_align.py) -- a content difference, not a phase one

Because the `NOT FOUND` schedule fingerprints could have been an alignment artifact, the two envelopes were
compared at every offset up to +/-90 s (250 ms buckets, mean-removed dot product):

```
orig 1064 buckets @44100 Hz   port(cdda) 1271 buckets @44100 Hz
corr at zero shift : +0.046
best shift         : +31.0 s, corr +0.246
activity: orig 0.98   port 0.99        (both sides are emitting sound essentially the whole time)
```

No offset makes the envelopes agree (best +0.246), so this is not "the right music at a shifted clock" --
with both sides near-continuously active, the port is playing *different material at the same moments* (or a
different track order). That is the machine-readable form of the still-UNSOLVED music cluster, and it is the
right signature to attack next: `cd_schedule.py` (event-anchored LBA/track marks, docs/106) says the
*selection* logic is equivalent on the probes it covered, so the divergence has to be in when tracks get
replaced/stopped during play -- which is exactly where the docs/106 teardown fix landed and where the user's
re-report says it is still wrong.

## The schedule and the envelope disagree -- what that leaves for the music item

`cd_schedule.py` on the same capture (`--orig /tmp/rig/rt-sfx/orig --port /tmp/rig/rt-sfx/port`):

```
original: -15071 ms lba=0x2d320 t9 | +0 ms lba=0x367aa t11 | +9948 ms lba=0x9ddf t2
port    : -56656 ms lba=0x918e t1 | -12569 ms lba=0x2d320 t9 | +0 ms t11 | +10944 ms t2
selection-by-selection: same lba each time; deltas +2502 ms, +0 ms, +996 ms; 4 plays / 5 stops /
                        0 changes without a stop (the docs/106 teardown invariant holds)
```

So the *schedule* oracle still says equivalent (only the boot-track selection is off by 2.5 s), while the
*content* oracle says the envelopes never correlate (best +0.246 over +/-90 s) and the port's CD rms is
~5.9x the shipped mix rms (4700 vs 800). Those two statements can only both be true if the comparison is not
measuring the same thing: an RMS/onset envelope is a weak discriminator for continuous music, and the shipped
mixer capture is music+SFX at the game's own volume, while `cdda.raw` is the CD stream alone at line level.
Recorded as the open question for the music cluster: the next oracle step must compare *within a track*
(spectral/onset fingerprints of the same LBA range, or a byte-identity check of the sectors the shipped game
reads vs the port's `cdda.raw` for the same track) instead of whole-route envelopes.
