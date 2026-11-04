# 57 — CD-DA: fn_010b1d, the TOC track table, and the host audio path

Closes the "working audio (effects + CDDA)" half of the playtest goal. Two independent defects plus one
missing subsystem, all fixed and measured.

## What the original actually does

`fn_010b1d` (orig `0x010b1d`) was ported as a no-op stub with the note "CD track/TOC read (no-op);
EAX ignored". That note was wrong on both counts.

Disassembly (`0x010b35` … `0x010c2d`):

| address | effect |
|---|---|
| `0x010b35` | `MOV dword ptr [EBP-4], EAX` — **EAX is an argument** (Ghidra shows it as the canary local `local_20` because the canary slot and the argument share the frame) |
| `0x010b38` | gate: `DAT_00051a10 == 0` (sound effects off) → return |
| `0x010b45` | gate: `(int32)idx >= (int32)DAT_00053c60` (track count) → return, `JGE` signed |
| `0x010b54` | copy the 5-dword + 1-word MSCDEX read-request template from DS `0x51b20` |
| `0x010b65` | `lba = rec[idx].lba` — records at `DAT_00053c64` are 5 bytes: track byte, then the LBA dword at `+1` |
| `0x010b79` | `len = rec[count].lba - rec[idx].lba` (record `count` is the lead-out, written by `fn_010927` at `count*5+1`) |
| `0x010ba0` | if `idx != count-1`: `len = rec[idx+1].lba - rec[idx].lba` |
| `0x010bf9` | `DAT_00053c44 = lba`; `DAT_00053c40 = lba + len` |
| `0x010c0c` | `DAT_00063e38 = 0` — the tick counter the refresh watchdogs compare |
| `0x010c16` | `DAT_00053c48 = (int32)len / 75` — SAR + `IDIV 0x4b`: **the track length in seconds** |
| `0x010c2d` | `fn_010570(&block, 0)` — the MSCDEX media read, request word `0x1510` |

So `fn_010b1d` *selects and starts a Red Book track*. The three refresh watchdogs
(`fn_010d47` at `0x010d74`, the screen at `0x0003884b`, the one at `0x00039a35`) all compare
`DAT_00063e38 / 490` (elapsed seconds) against `DAT_00053c48` and re-issue the call — i.e. the game
restarts the track when it expires. That is also why the earlier port of those sites looked inert: they
only wrote globals whose only consumer was the "dead" call.

The track number is live, and comes from the per-room table `g_tab_52da4` (DS `0x52da4`, 100 entries,
values 2..9) indexed by `room[+0x40] + (stage-1)*10`, plus fixed values at the menu screens
(`mov eax, 1 / 9 / 0xb / 0xc` at `0x38b02`, `0x3a43d`, `0x391f6`, `0x38632` …). Because every table
read at those sites is now restored, the previously "dead data-table read" annotations in `f.c` were
removed.

## The disc

`NIGHT/TG_NIGHT.CUE` declares `TRACK 01 MODE1/2352` and **tracks 02..15 as AUDIO** — 15 tracks,
269 453 sectors. Data sectors in this image carry the ISO user data at offset 16 (`CD001` measured at
byte 17 of the PVD at LBA 16), which `cd_iso.c` already relied on.

Audio sectors, measured rather than assumed (probe over 400 sectors from `38612`, inside track 02):

| reading | rms | peak | zero-crossings | lag-1 autocorrelation (L / R) |
|---|---|---|---|---|
| little-endian S16 | 4206 | 24163 | 20.0 % | **0.982 / 0.972** |
| big-endian S16 | 18903 | 32768 | 50.0 % | −0.001 / +0.004 |

So this rip stores **little-endian 16-bit signed PCM**, unlike Red Book's on-disc byte order — same
family of non-standard layout as the +16 data offset. The sector bytes therefore go to the host
unchanged: 2352 bytes = 588 stereo frames = exactly 1/75 s, which is where the `/75` in
`DAT_00053c48` comes from.

## Port changes

1. `fn_10b1d(uint32_t track)` — faithful body as tabled above, plus two documented host guards:
   `DAT_00053c64 == 0` (the record array is heap-allocated by `fn_010927`; on DOS heap offset 0 is
   mapped garbage) and, at the two room-table sites, a range check on the index
   (`cd_track_for_room`), because the original read that index out of raw DS and could walk past the
   100 entries in states where no stage is loaded.
2. `sys_cd_audio_play(index, lba, sectors)` seam (+ `sys_cd_set_audio_backend`, counters
   `sys_cd_audio_play_calls/last_index/last_lba/last_sectors`), and `sys_cd_audio_stop()` now reaches
   the host as well.
3. `sys_cd_da_plan(pos, sectors, want, cap)` — the streaming policy as a pure function: whole sectors,
   never past the track end. A finished track is *not* looped by the host; the game restarts it through
   its own watchdog, which keeps the restart in the game clock.
4. `cd_iso_read_audio(lba, out2352)` — the whole raw sector, not the 2048-byte user slice.
5. Host (`src/app/main.c`): a second `SDL_AudioStream` (S16 stereo 44100 → device format) bound to
   the same device as the board stream, so SDL mixes effects and music. With no device at all the track
   is still consumed on the disc's own 75-sectors-per-second clock, so counters and the game's watchdog
   behave the same headlessly.
6. **`SDL_BindAudioStream` was missing entirely.** SDL3 plays a stream only once it is bound to an open
   device, so the board's bytes were being accepted (counters grew) but never reached hardware — that is
   why the available-byte figure climbed to ~98 KB and stopped at the latency bound.

## Measurements (`scripts/playtest/route_episode1_anchored.txt`, headless)

Track selection, no device (`--no-audio`), i.e. read-and-count:

```
disc: 15 tracks, 269453 sectors, lead-out MSF-derived
cdda: track index 1  lba=37262  sectors=3153  (42.0 s)
cdda: track index 9  lba=185120 sectors=27697 (369.3 s)
cdda: track index 11 lba=223146 sectors=17856 (238.1 s)
cdda: track index 2  lba=40415  sectors=19639 (261.9 s)
pump: ... cdda=10673376B tracks=4 pos=1155 snden=1
```

Note `sectors=3153` for the first audio track — exactly the `0xc51` difference the disc-present gate
`fn_010c3e` compares, an independent confirmation that the TOC records the port fills match the
original's expectations.

With a device (`SDL_AUDIODRIVER=dummy`) and the sound board modelled (`NIGHT_BOARD=1`):

```
audiodbg: dev=default submitted=261902 B drained=0 B stream=0 B transfers=61 stops=61 rate=10989 width=1 pending=0 pushed=220027
pump:     ... xfer=61 stops=63 cdda=44934960B tracks=4 pos=12320 snden=1 devinit=1 plays=63 real=61 null=2
```

Effects: 61 DSP/DMA transfers, 220 027 B of 8-bit mono PCM at 10989 Hz, **261 902 B submitted into a
bound stream whose backlog is 0 B** (before the bind fix: `submitted=11310 B`, `stream=98684 B`).
Music: 44 934 960 B of CD-DA read from the disc and fed to the second bound stream (12 320 sectors ≈
164 s of 44.1 kHz stereo).

## Tests

`test_cd_da_track_select` pins the arithmetic against a synthetic 4-track TOC (start/end LBA, the
last-track-uses-lead-out rule, `len/75`, the `DAT_00063e38` clear, both gates, and the seam call's
index/LBA/length) and `test_cd_da_plan` pins the streaming policy. Suite: **2838 checks, ALL PASS**
(2822 before).

## Still open for audio

Nothing in the sample/CD path is known-broken in the port now; what remains is *listening* — the
evidence is byte/frame counters into bound SDL streams, not an audible check on real hardware. A
real-device run (`NIGHT_BOARD=1`, no `SDL_AUDIODRIVER` override) on a machine with working CoreAudio is
the remaining confirmation.

## Effect samples verified byte-for-byte on the DMA wire (this cycle)

New `NIGHT_AUDIODUMP=<n>` diagnostic in the sys layer dumps the first 256 bytes of every armed DMA transfer with
the programmed sample count and rate (`audiodump n=... rate=... width=... ch=... bytes=<hex>`). Compared with an
independent parse of the shipped container (100 `.voc` assets, 106 voice blocks, per the docs/04 layout: 26-byte
header, then tag/len24/payload with tag 0x01 = handler, rate_idx, PCM):

```
DMA transfer 3607 samples @ 10989 Hz -> BYTE-EXACT in 1.voc: handler=0xa5 rate_idx=0 offset=0 (asset PCM 3607 B)
```

i.e. the samples the port hands the DMA are *exactly* the shipped asset's PCM -- the whole 3607-byte sample block,
from offset 0, at the programmed rate. That upgrades the effects claim from "transfers complete" (the counters in
docs/57) to "the hardware receives the shipped samples".

Scope note, measured: across three scripted runs (40 and 60 transfers sampled) every triggered effect is that one
asset -- `1.voc`, the starting weapon's sample, 3607 samples @ 10989 Hz, which is what the mASHed fire key plays.
The other 99 assets are loaded by the same path but need in-game events the scripted routes do not reliably
produce (enemy impacts, deaths, boss phases), so per-asset byte-exactness is proven for the one asset the routes
trigger and the corpus-wide claim rests on the loader path being shared; the rate is the same for every asset in
the corpus (`rate_idx` is 0 in all 106 blocks).
