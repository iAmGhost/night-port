# 463 — The browser port carried 546 MB of CD-DA; it now carries 50 MB of MP3, and one FIFO index was doing the work of two

Asked of the browser port: **put the music in a lossy format.** The format was left open, which is the right
way to leave it open, because the only real requirement is bandwidth: 546 MB of a 570 MB install set is
CD-DA. What follows is the decoder, the Red Book reader that sits on it, and the two probes that certify
them — one of which found, on its first run, that every track the port could play was scrambled.

## What the port was carrying

`make stage` puts 14 audio tracks under `dist/data/cd/trackNN.bin` — **546 MB** of Red Book sectors, 2,352
bytes at 75 sectors a second. `wasm/src/wasm_disc.c` reads them as it has to read anything in a browser:
sliding HTTP Range windows, 150 KB at a time, with read-ahead, backoff, and the rule that a network which
blinks is a hole in the music and nothing else (docs/460). While music plays that is **176,400 bytes a
second**, for as long as music plays; the game asks for a sector, and the port owes it 2,352 bytes on a
clock the game does not slow down for.

The same material at 128 kbps constant-bitrate MP3 is **50 MB** — 11.0x smaller, 16,000 bytes a second,
the same 14 tracks. Nothing else about the disc changed: `cd_iso.c` still bounds tracks by sectors, the
game's LBA arithmetic is untouched, the audio pump is untouched, and the same window service still answers
for the bytes.

## Why MP3, when the question was only about bytes

Because *where the music starts* is the whole problem. The game seeks by Red Book position: `cdda_play`
names an LBA, the pump asks for the sector at byte `lba * 2352`, and it must get the samples that live
there without downloading the track first. Opus and AAC would carry better at the same bitrate, and neither
will tell you what byte holds sample 4,611,000 without either a seek table (which means reading the whole
file, or shipping one) or a server that keeps decode state.

A **CBR** MP3 is a clock. Every frame is `bitrate/8` bytes and 1152 samples, so sample `S` lives at

```
byte = first_audio_byte + S * bytes_per_sec / sample_rate
```

— an estimate within a byte. Land on that byte, scan forward to the next real frame header, and from that
header on every sample is counted rather than estimated, so the seek is exact. That is the entire reason
the encoder is told to produce constant bitrate, and it is checked: `mp3_stream.c` notes the bitrates it
has seen, and a stream whose frames are not all one size is reported (`vbr=1`), refuses a *backward* seek,
and keeps playing forward — which is all the game ever asks of a track.

The decoder is [minimp3](https://github.com/lieff/minimp3), vendored at `ea99364f61c1` (CC0) as
`minimp3.h` alone, and only its low-level entry, `mp3dec_decode_frame` — "decode one frame from these
bytes." The high-level `minimp3_ex` wants a file or a buffer of the whole stream and builds a seek table
by scanning it, which here would mean downloading a track to hear one scene.

## Where the seams go

```
cd_iso.c ── nk_mp3_pread ── wasm_mp3.c      tracks.tsv index; sector -> sample range; one stream per track
                                    │
                                    └── mp3_stream.c   bitstream only: byte callback in, int16 stereo out
                                                          │
                                     nk_disc_read ── wasm_disc.c   one sliding-window Range service
```

- **`wasm/src/mp3_stream.{c,h}`** knows no network, no filesystem, and no emscripten. It is handed a
  callback — "give me n bytes at offset off", `0` = they are here, `-1` = they are not — and never assumes
  it can seek, never assumes the bytes are there, and never assumes a frame it wants is whole. The point is
  that the same file compiles into the wasm module *and* into a desktop probe, so the thing being certified
  is the thing that ships.
- **`wasm/src/wasm_mp3.c`** is the Red Book reader: it parses `tracks.tsv`, maps a sector read to a sample
  range, seeks one stream per track, and answers 2,352-byte reads. It is where the byte source is
  dispatched from: `nk_disc_pread` goes to the raw sectors or to this, per track.
- **`wasm/src/wasm_disc.c`** had its windows made **URL-addressed** (`nk_disc_read`, `nk_disc_get_text`,
  `nk_disc_size_url`, `nk_disc_size_set`) rather than track-addressed, so the MP3 source joins the existing
  service instead of duplicating it. The read-ahead, the backoff, the hole-becomes-silence rule and the
  one-rule-that-matters — the network must never stop the game — were the expensive parts of that file, and
  a second byte source gets all of them for free. `nk_disc_size_set` exists so an index that already states
  a file's length never causes a HEAD, and never reads one byte past the end of a file it is only windowing.

`tracks.tsv` is not optional, and the reason is the one thing the encoded file cannot tell you: **how long
the music is.** An MP3's byte length says nothing about its Red Book length, and `cd_iso` must be told a
length before it will accept an LBA.

```
track  mp3_bytes  redbook_sectors  bitrate_kbps  sample_rate  channels  decoded_samples  head_delay_samples
    2     674585             3153           128        44100         2          1854720                1105
```

## The 1,105 samples in front, and the same number missing at the back

An MP3 encoder runs a polyphase filterbank, and a filterbank has a state: LAME writes its warm-up — 1,105
samples, 25.1 ms — in front of the music and tells only a tag reader about it. `mp3dec_decode_frame` does
not drop it. Asked for sample 0 of a track, the port would have answered with 25 ms of the encoder
clearing its throat and the rest of the track one quarter of a second late — a clip at the head and every
music point in the game (docs/96) sliding.

It is measured rather than assumed, because the number depends on the encoder and the build.
`nk_mp3_probe.c` decodes a track's head, slides it across the raw sectors it was encoded from, and takes
the lag that lines them up: **1,105 samples on all fourteen tracks**, with a mean |difference| of
0.0003–0.011 of full scale at that lag (a lossy codec's own error against its source, which is what the
correct lag is supposed to look like). The number goes into `tracks.tsv` and into
`nk_mp3s_set_delay()` before the stream's first use, and from then on the module speaks the *source's*
timeline: `seek()` adds the delay, `pos()` subtracts it, and the padding is dropped on the way in.

That has a consequence at the other end. Take 1,105 samples off the front of the timeline and the same
number comes off the back, so *does the file still reach the end of its track?* The first answer to that
question in `encode_audio.py` was arithmetic — `decoded_samples - delay` against `sectors * 588` — and it
said **12 of 14 tracks ran out 0.06 to 1.9 sectors early**, loudly, on files that were fine. Both numbers
in that subtraction were wrong: `decoded_samples` was counted by a probe loop that *stopped at the track's
own length* (a cap that had made perfect sense for a straight decode), so `decoded_samples - delay` was
`raw_samples - delay`, and every track "ran short" by about the delay.

So the question is now asked of the stream. Walk forward from inside the track, one MP3 frame at a time,
and stop at the first frame the stream **could not decode** — not the first frame of zeros, because twelve
of these fourteen tracks fade to silence and end on samples of exactly zero, so zeros prove nothing. Measured,
every track answers for **6,912 to 17,280 samples past the end of its Red Book data** (0.16 to 0.39 s, 12
to 29 sectors), which is why `--pad` defaults to 0: it exists to pad the encoder's input if a future
encode ever runs short, and now there is a check that would tell us it did.

Getting that measurement right took two more of its own bugs: the walk reported `slack = np - p` against
the *previous* frame instead of the distance from the end of the track, so all fourteen files reported
exactly one frame of slack; and taking the hole counter before the seek rather than after made the seek's
own byte-crossings count as the file ending.

## The bug the probes existed to find

The first run of the first probe said the samples were wrong. Not their position — every seek reported
`pos` equal to what it was asked for, the byte clock, the frame count and the sample count all agreed with
one another — and yet the samples at that exact position differed from the sectors the MP3 was encoded from
by an order of magnitude more than 128 kbps costs, and `mad vs sequential`, the same samples taken once in
a straight run and once by seek, was just as loud. That combination — position exact, content wrong, the
two independent routes to the content disagreeing with the source in the same way — says the decoder is
being fed correctly and something in between is not keeping up.

The FIFO between them had one index doing two jobs. `fifo_head` is the read cursor; `fifo_write` wrote at
it, and advanced it:

```c
uint32_t at   = s->fifo_head;                                    /* wrong: reader and writer meet here */
uint32_t tail = (s->fifo_head + s->fifo_n) & s->fifo_mask;       /* right: writes trail reads by fifo_n */
```

so every decoded frame landed under samples the reader had not taken yet, and the next read started *after*
the frames that had just been written and handed back whatever the ring still held from a few frames
earlier. It is worth saying what this bug does **not** look like, because it is the reason it survived being
read: nothing in the module's own accounting moves. `frames`, `samples`, `map_err` and every seek's `pos`
still describe a stream of exactly the right shape — it is the *contents* of the ring that are shuffled.
The only thing in this repo that notices is a comparison against bytes the module has never seen, which is
the entire justification for `analysis/probes/nk_mp3_probe.c` existing. (Putting the bug back deliberately
to record a number for this paragraph did not produce one: the probe stopped making progress and had to be
killed. A ring whose cursors meet is not merely wrong, it can wedge.)

A latent one sat in the same file: widening a mono frame to stereo walked *forward* through its own buffer,
overwriting the source two slots ahead of the copy. No track on this disc is mono, so nothing here would
ever have shown it, and it is a single loop in the wrong direction — the kind that is worth fixing while
the code is open and the probe is in front of you.

The counter that caught the first bug is the interesting one. `mad vs sequential` takes the same samples
twice, once in a straight run and once by seek, so it isolates the seek path from the decoder — and a bad
FIFO shows up there immediately, even when every other number agrees with every other number.

## What is measured, and what is asserted

`make -C wasm audio` encodes and probes each track; `make -C wasm audio-check` drives the sector-level path.

| measured | how | across the 14 tracks |
|---|---|---|
| the byte clock | `frame_index * bytes_per_frame` against the byte the frame was actually found at | **1 byte** worst on every track, across the ~119,000 frames they hold between them |
| is it really CBR | the bitrates the first frames of every stream report | 128k on all 14, `vbr=0` |
| does a seek land | decoded 4,096 samples, slid ±2,048 against the raw sectors, and against the straight run | lag **0**, mad vs sequential **0.00000** — the one seek that reports otherwise is the last, which lands in a track's silent tail, where every lag matches and the number says nothing |
| the encoder's head padding | decoded head slid against the raw sectors | **1,105 samples** (25.1 ms) on all 14 |
| does the file cover the track | walk the stream past the track's last sample to the first undecodable frame | **+6,912 to +17,280** samples |
| the FIFO | samples the ring could not hold | **0** on all 14 |
| holes, resyncs | frames whose bytes never came | **0**, **0** |

The byte-clock row deserves a word, because the obvious way to measure it measures nothing: convert a
sample to a byte with the seek formula, convert back with its inverse, and print the difference — small by
construction, since you wrote both halves. The clock is only worth trusting against something independent,
so `map_err` compares the analytic estimate with the byte a frame was *actually found at*, counted by
incrementing `frame_index` once per counted frame while decoding. One byte of drift across a whole track is
what `bitrate/8` rounded per frame costs, and the forward-scan absorbs it.

`nk_mp3_disc.c` is the second probe and the one that faces the game: it links `wasm_mp3.c` itself and calls
`nk_mp3_pread` with the reads MSCDEX makes — one sector, a 64-sector run, the same sectors again backwards,
a read past the last sector — against a `fopen` stand-in for the three functions `wasm_disc.c` owns. Its
reference is **a straight run through the same track**, not the raw sectors, and the difference matters.
Against the raw sectors, track 07's last sector measured 0.118 of full scale — which looks like a geometry
error and is not: reached by a seek or by a straight run it reads back the *same* wrong samples, so the
difference is between the file and the sector it was encoded from, at the one place an encoder's final
frame has most to say. A geometry error would show up as the two routes disagreeing. So the floor — the
decode against the raw sectors — is reported separately, and a seek is judged against what the straight run
read at that same sample:

```
track 02: ok     3153 sectors, straight-run floor 0.01992 fs, seeks 0.00000 fs, run 0.00000 fs
track 08: ok    24839 sectors, straight-run floor 0.02319 fs, seeks 0.00000 fs, run 0.00000 fs
...
ok: 14/14 tracks answered every seek with the sector a straight run read
```

The floor — the codec's own cost, mean |difference| against the source, in units of full scale — is
0.003 to 0.023, and it is largest in loud passages and at a track's last frames, which is what a lossy
codec does. The seeks and the 64-sector run are exactly zero against the run in every track: the music the
game asks for at byte `lba * 2352` is the music that would have played had the track been played from the
start, which is the property the raw path has by construction and the one a decoder has to earn. A read
past a track's last sector is **refused** rather than answered with silence, because `cd_iso` bounds a
track by the length it was given and a full buffer of zeros instead of a "no" plays a song that never ends.

## What the page says now

```
audio: data/audio/tracks.tsv: 14 lossy tracks (track02 is 128 kbps 44100 Hz 2 ch), CD-DA will come from there
disc: 15 tracks, 269453 sectors, audio from /data/cd (0 of 14 audio tracks have no bytes)
disc: 14 of them will be answered out of data/audio instead of sectors
audio: track 2 from data/audio/track02.mp3 (0.6 MB, 3153 sectors of Red Book in it, 1105 samples of encoder padding thrown away)
pump: t=20016ms ... cdda=3222240B tracks=2 pos=696 ... ring=16/2672@44100 dev=44100 ... page avg=804 max=1318
      disc reads=20 fetched=14/806400B fails=0 waited=8(max 1ms) silence=0B
      mp3 frames=358/163840B seeks=0 holes=0 clock_err=1B decode=22ms
```

The last two fields of the pump line are new. `dev=44100` while the board runs at 11,111 Hz is the point: the
music's ring is marked with the rate of what is being decoded (`nk_mp3_rate()`), so a 48 kHz encode would not
silently play at 44.1. And the run above is *streaming*: `pos=696` sectors deep, 358 MP3 frames out of 806 KB
of Range windows, `holes=0` and `silence=0B`. Getting a run that looks like this out of the headless harness
was a story in itself — see "What the check was not measuring" below.

Switches: `?audio=0` (also `off`, `raw`) refuses the lossy path and streams sectors as this port did
before, `?audio=data/other` names a different directory, `--audio-dir` and `--no-audio` do the same from
argv. `sw.js` needed no code change: it only ever intercepts the shell allowlist, so `trackNN.mp3` and
`tracks.tsv` go to the network untouched, which is the only thing a Range request wants.

## What it costs the game, measured

Decode happens on the game's own thread, inside the Red Book read MSCDEX blocks on, so the question is not
"how much CPU over a minute" but "how long is the worst read that lands inside a frame". That number was
not being kept: `decode` timed the fill, and a seek — which can decode half a dozen frames and fetch a
window — was invisible. `nk_mp3_pread_stats` counts the whole call now, with buckets at 2 ms and 8 ms and a
cold/hot split, because a track's first reads are cold in three ways at once (decoder, bit reservoir,
window) and land while the game is usually mid-transition, whereas a slow read in the middle of a room is
what a player calls a stutter.

40 s of two tracks, headless, same build, same keys:

| | -O2, 0.9 s read-ahead | -O3 + 3 s read-ahead |
|---|---|---|
| decode wall time for ~700 MP3 frames | 93 ms | **59 ms** |
| per decoded frame | 134 µs | **85 µs** (≈ 190→307x realtime) |
| awaited disc reads | 15 | **5** |
| reads over 2 ms | 6 | **4** (3 of them cold, 1 steady) |
| reads over 8 ms, worst read | 0, 5 ms | 0, 5 ms |
| bytes fetched for the same music | 1.61 MB | **1.34 MB** |

Two changes, both free. The decoder is the one translation unit in the build that is a hot loop over data
rather than a state machine over frames, so it gets `-O3 -fno-math-errno -fno-trapping-math` on its own
(nothing in MP3's math depends on FP exception behaviour, and the audio-check floors are unchanged to the
last decimal, so the output is identical). And `NK_MP3_LEAD_MS` went from 0.9 s of audio to 3 s: 0.9 s had
been chosen to match the raw path's 64-sector read-ahead, which was the wrong unit to think in — the raw
path pays nothing to sit on bytes it already has, while this path blocks a Red Book read on a window that
has not arrived. 3 s of music is 48 KB of buffering at 128 kbps, one fortieth of what one raw track costs,
and it nearly halves the awaited reads while fetching *fewer* bytes overall.

What is left, and the two ways to take a real bite out of it:

- Steady state here is one read over 2 ms in forty seconds and nothing over 8 ms, which does not explain a
  stutter on a machine like this one — a slower device is where the ~19 µs-per-millisecond of decode starts
  to matter, since wasm runs this decoder roughly 8x slower than the same C runs natively.
- **A cheaper codec is the structural fix.** The decode is what would show up on a phone, and Opus decodes
  several times faster than minimp3 does — at the price of needing a seek map (Ogg carries one; the encoder
  could publish a sample→byte index next to `tracks.tsv`, one small file per track read once).
- **A Worker or AudioWorklet is the other one**, and it is the only change that makes decode cost zero on
  the game's thread regardless of codec. Both are bigger than the two lines above.

## What the check was not measuring, and why it said nothing drained the rings

For the whole of this work `make check` failed:

```
CHECK: FAIL
  nothing drained the PCM rings
```

and every symptom pointed at headless Chrome having no audio device: `"ac":"suspended"`,
`The AudioContext encountered an error from the audio device or the WebAudio renderer`, `page avg=0 max=0`,
`pulled.blocks=0`, and a `?sink=1` timer that appeared never to fire. It was not the audio device, and
`?audio=0` failed identically, which is the only reason it did not get blamed on the MP3 path.

The pump line had been telling us the real state all along and I had been reading it backwards:

```
ring=0/5292@44100 pos=9
```

`ring=%u/%u` prints **`ring_used(SFX)` / `ring_used(CDDA)`**, not used/capacity. The CDDA ring was *full* at
exactly its 120 ms queue target, and the host was correctly refusing to feed a full ring — the reader, not
the writer, was the problem. `cdda_stuck` never reported it either, because its first guard returns when the
ring is more than a quarter full.

The reader was returning at its own guard, `if (!M || muted) return;`, for 591 ticks out of 591 — because
**the page was muted**. Not by a click and not by a flag: by the harness parsing its own environment.

```js
const mutes = (process.env.NIGHT_MUTE || '').split(',').map(Number).filter(Number.isFinite);
```

`''.split(',')` is `['']`, and `Number('')` is **0**, which is finite. With `NIGHT_MUTE` unset — every run
that never asked to be muted, i.e. every `make check` — this yields `[0]`, and the harness presses the page's
mute button at t = 0 ms. `NIGHT_PROBE` had the same parse and ran its probe at 0 too.

```
mute: pressed at 0 ms -> "unmute"
MUTETOGGLE src=n/a detail=0 trusted=false  at shell.js:1196:10 ...
```

So the page muted itself, `startSink` declined to pull, the ring filled to the queue target, the host stopped
feeding at nine sectors, and the assertion — `pulled.blocks > 0` — failed. The parse came in with
`905eb07`, which is docs/462, the commit whose *subject* is about muting the page: the mute press it added
for testing made every run that did not ask for one a muted run. That is also why `"ac":"suspended"` looked
like the culprit — a run whose AudioContext happened to deliver one block before the click at t=0 scraped
past the assertion, which is what made it look intermittent.

The fix is four lines in `headless.js` (drop empty fields before `Number()`), and what it buys is not a
greener exit code but a check that finally measures the thing it claims to. With the page no longer muted by
accident, the same 20 s run streams this much music:

```
pump: ... cdda=512736B  pos=218 ... page avg=0    max=0    disc reads=4   fetched=3/172800B  silence=0B
      mp3 frames=114/65536B  seeks=0 holes=0 clock_err=1B decode=4ms
pump: ... cdda=3222240B pos=696 ... page avg=804  max=1318 disc reads=20  fetched=14/806400B silence=0B
      mp3 frames=358/163840B seeks=0 holes=0 clock_err=1B decode=22ms
```

`pos` climbs from 9 to 696 sectors, `page avg` stops being zero, and 358 MP3 frames — 160 KB of decoded
music — come out of 806 KB of Range windows with `holes=0`, `silence=0B`, `fails=0` and a byte clock that
stays one byte off. Next to the same run with `?audio=0`, which is the whole point of the change:

| 3.2 MB of Red Book delivered | MP3 | sectors |
|---|---|---|
| bytes fetched | **806,400** | 3,913,728 |
| disc reads | **20** | 1,397 |
| silence substituted, failed reads | 0, 0 | 0, 0 |

4.9x on the wire for the same music, in a run that now exercises the path instead of walking past it.

## Opus was tried, and it is not the decoder

A phone stutter was reported, and the obvious guess was that wasm running minimp3 is the thing a phone can't
afford. It isn't, and the alternative isn't cheaper: libopus 1.5.2 was built for wasm through its CMake
emscripten toolchain file and decoded 40 s of the same track in the same module with the same `-O3`.

| decode 40 s of track 05 | µs of main-thread time per second of audio | × realtime |
|---|---|---|
| minimp3, 128 kbps MP3 @ 44.1 kHz | **745** | 1344× |
| libopus 64 kbps @ 48 kHz | 1538 | 650× |
| libopus 96 kbps @ 48 kHz | 1617 | 618× |
| libopus 128 kbps @ 48 kHz | 1814 | 551× |
| native control, same code | minimp3 338 / libopus 1164 | 2960× / 859× |

Opus costs 2 to 2.5 times as much decode as MP3 does, natively as well as in wasm, so it is not a fix for a
stutter. The belief it was supposed to fix -- that Opus is the cheap-to-decode codec -- is true against
*quality*, not against MP3's decoder: fullband stereo SILK/CELT does a great deal more work per frame than
1152-sample Huffman/IMDCT.

Two things the experiment did establish, in case bandwidth becomes the reason to want it rather than CPU:
CBR Opus is as seekable as CBR MP3 -- `-vbr off -frame_duration 20` gives exactly 160/240/320 bytes per
packet at 64/96/128 kbps, 2001 packets for 40.0 s, so sample→byte is the same one line of arithmetic, with
`pre_skip=312` doing the job LAME's 1105 samples do here -- and the price for taking it is a resampler, not
a container. Opus emits 8/12/16/24/48 kHz and nothing else. The ring has to stay at 44.1 kHz, because 48 kHz
samples pushed as if they were 44.1 play a minor third sharp and finish a four-minute track 21 seconds early.

## What the page now says about its own audio

`make check` ends with:

```
page audio: dev=44100Hz block=1024 filter=sinc mix=51/1100us/block(1116) late=0/1116 blocks(max 0.0ms) underrun=[516,130]
```

`mix` is the mixer's own wall time per block (new): `lateMs` has been here all along and can tell you a block
ran late, but not whether the mixer ate the time or the main thread had been sold to the game first. Those
two answers need different fixes and the old counters could not tell them apart.

`dev` is the second thing that only a phone can say, and it matters because of a line in `ensureAudioOnce`
that reads like an assumption and is one: *"ask for 44.1 kHz ... so CD-DA needs no resampling at all."* That
is true of desktop Chrome, which honours the hint. Android and iOS answer 48 kHz, and then the **music** goes
through the same 16-tap windowed-sinc reader as the sound effects, on the main thread, every block. Measured
in node for scale: 28 µs per 1024-frame block at 44.1→44.1, 28 µs at 44.1→48, 14 µs with `?resample=linear`
-- i.e. well under a percent of a block's 21 ms on this machine, and whatever a phone's JavaScript makes of it.

Which leaves two levers that cost nothing to try on the device, both of them A/B-able by editing the URL:
`?resample=linear` halves the converter's work, and **`?spbuf=4096`** raises the audio callback's block size.
The second is the one aimed at a phone specifically. `ScriptProcessorNode` is serviced by the main thread, which
on a phone is also where the game's wasm lives, and 1024 frames at 48 kHz is a callback every 21 ms. Longer
blocks buy scheduling headroom for a latency no one can hear in a music track.

## What was ruled out, and the trap that nearly made the ruling-out a lie

**The only stutter-shaped thing measurable on this machine is not audio.** Frame cost grows after a minute or
so -- `present` 6 µs/frame while a 320x200 screen holds still, ~1050 µs/frame in the scene that switches to
640x480 -- and the curve is the same in all three configurations, MP3 on, MP3 off (`?audio=0`) and the audio
sink entirely off (`?sink=0`). Whatever that is, it begins where the display mode changes.

**The `-O3` experiment compared a build with itself.** Changing `CFLAGS` from `-O2` to `-O3` printed
identical numbers, because nothing in the Makefile says a flag is an input, so every object from the old
flags was still in place. A `$(OBJ_DIR)/.flags` stamp now carries them and the objects depend on it. With
that fixed, whole-game `-O3` buys 2-4% of `present` for 20 KB of download, so the game stays at `-O2` and
only the decoder carries `-O3` -- the game's own logic runs at `step=0..1 µs` per tick and was never the
thing a phone would struggle with.

## It played for a while, and then the music never came back

Reported from a phone: "it plays for a while, but music stops and start stutter. and stopped music never came
back." The log said `waited 200 ms` and, in the copy of it the page could not yet give up, something like "range
read got no answer".

The gap in the tests was not the decoder or the network. It was that the disc layer's one rule -- *a read that
cannot be answered is answered with silence, and the track is never abandoned* -- had an exception nobody was
watching.

**`nk_mp3_disc.c` grew a third scenario.** The byte source answers for 800 sectors of a track, goes dead for the
next 30, and answers forever after. The same walk is run twice: once with the dead window (the test) and once
clean (the reference), and the sectors are compared against each other. The assertions are the ones the phone
could see: no sector inside a track is ever *refused*; the outage lands (some sectors must go silent, or the
fault was injected into nothing); music returns within 300 sectors; and from the second whole sector back it is
the same music, sample for sample.

It was red in two places.

**A hole was refused, and the host reads a refusal as the end of the song.** `cdda_pump` in `main_wasm.c` takes
`rc != 0` from the byte source to mean *that is the end of the track*: it prints `the track is abandoned`, zeroes
its state, and waits for the game's next `fn_10b1d` to pick any music up again. `wasm_mp3.c` had the right
invariant written down -- "never refuse inside a track", with a comment saying *the pump reads non-zero as
end-of-track; refusing a hole would stop the music* -- and then ended with `return got ? 0 : -1;`. When a fill
produced nothing, which is exactly what a hole does, it refused. So 30 dead sectors were not 30 sectors of
silence; they were the end of the song, forever. That is "stopped music never came back".

It is silence now, with the frames counted as holes.

**What is left, and why the obvious next fix is not in this build.** The probe reports a seam: the first
sectors back after an outage are 0.025--0.164 of full scale off. That is the stream picking up from a byte
position it *estimated* across the hole -- `starve` advances the sample clock by an estimated byte advance, so
the clock becomes an estimate, and the cheap forward-decode path then trusts it. An anchor (trust the sample
clock, resync the byte position) makes it exact: with one in, 13 of 14 tracks came back at 0.00000 and the
fourteenth within 0.007. It is not in the build, because forcing an anchor per sector while the link is dead
turned a stall into a **wedged tab**: with the harness's `NIGHT_NET="offline=10000-16000"`, the raw path
(`?audio=0`) streamed ten pump lines through the same outage while the MP3 path stopped answering Chrome's
debugger protocol at all. Every forced re-anchor is a cold Range read, and while the network is down that is
seventy-five awaits a second inside the decode loop.

So the seam is measured and printed by `audio-check` as a NOTE rather than fixed by a change that trades a
hiccup for a freeze. The gate for the right one (`g_bytes_ok`: re-anchor when the bytes are answering, not while
they are not) is in `wasm_mp3.c`, waiting for its other half.

**What the browser now says.** `NIGHT_NET="offline=20000-26000"` against the fixed build: pump lines at 22, 24,
26, 28, 30 and 32 s, `mp3 frames` counting steadily across the hole, no `the track is abandoned`, page responsive
throughout. **Still open:** the same outage starting at 10 s -- while the first bytes are still arriving --
wedges the tab, which is a boot-path problem and not the one the phone reported.

## Preload: the mode where the network is not in the room

Everything above this section keeps the network inside the game loop, and the previous section measured what
that costs: a 22-second dropout in the MP3 path takes the tab down with it, while the raw-sector path rides
the same outage out. Streaming is the right trade on a metered link; it is not the right trade for a phone
sitting on Wi-Fi in a living room, which is where the dropouts were reported from. So the page now asks.

**Before the press to play**, the gate offers `music: stream as you play | pre-download (47 MB)`, the size
summed from `tracks.tsv` rather than typed in, the choice remembered in `localStorage`. Pressing play with
preload on brings the whole score down first -- the gate counts tracks and megabytes -- and then starts the
game.

The mechanism is one seam, deliberately the same one the network answers through. At the top of
`disc_pread_url` in `wasm_disc.c`:

```c
    /* Preloaded bytes answer first: a read the page can serve from memory is not a network question at all. */
    int got = nkjs_pre_read(url, (uint32_t)byte_off, dst, n);
    if (got == (int)n) { g_reads++; g_read_bytes += n; g_pre_reads++; g_pre_bytes += n; return 0; }
```

The bytes live in JS `Uint8Array`s in `globalThis.__nkPre.map`, keyed both by the URL the game builds and by
the bare filename -- `--audio-dir` changes the former and nothing else. `nk_disc_size_url` answers from the
same map, so a preloaded file never waits for a HEAD. A request that is not wholly inside a buffer falls
through to the normal path: no new EOF semantics, and in practice it never happens, because the index gives
the exact byte length and the encoder leaves slack past every track's last sample.

Keeping the 47 MB on the JS side of the wall is not tidiness. The module's heap already holds 51 MB of
NIGHT.RES, and a 16 KB `HEAPU8.set` per read costs microseconds.

**What the counters say.** The pump line gained `pre=N/KKB` next to `fetched`:

```
music: pre-downloaded 14/14 tracks (47 MB); no network will be needed for playback
disc reads=42 fetched=0/0B fails=0 silence=0B pre=42/672KB   mp3 frames=… holes=0 clock_err=1B
```

`fetched=0`, `fails=0`, `holes=0` and `clock_err=1B` is the point: preload does not perturb the byte clock, it
just removes the thing that could fail. And the run that wedges the streaming tab -- `NIGHT_NET=offline=18000-40000`,
22 seconds of nothing in the middle of a track -- passes in preload mode with eight pump lines straight
through the outage and the page answering the whole time.

`make check-preload` is that run with the assertions attached (`NIGHT_PRELOAD=1` in `headless.js`): the page
must say it downloaded the music, `pre` must be non-zero, and `fetched`, `fails` and `holes` must all be
zero. `?preload=1` / `?preload=0` force the mode from a URL for the harness, and `?audio=0` hides the row
entirely, since there is no MP3 to pre-download when the game is reading sectors.

**What it costs, and what it does not do.** 47 MB up front instead of 16 KB/s spread through the song, and a
two-second wait on localhost before the title appears. It does not cache across reloads: the registry is a
`Map` in the page, so a reload re-downloads everything. IndexedDB behind that map is the obvious next piece
and is deliberately not here, because a music cache you cannot see is a music cache that serves the wrong
bytes and gets blamed for the game.

## The one box: play the music, or don't

Pre-download was offered as a choice against streaming, and that framing was wrong: streaming is 16 KB a
second for the whole game, so "don't download" could never honestly mean "stream instead". The page now
offers one checkbox, ticked:

```
  19 MB of resources + 47 MB of music -- then no network while you play.
  [x] play the music (47 MB)
                                    [ press to play ]
  arrows move · SHIFT jump · Z punch · X kick · SPACE duck · ESC pause
```

The copy is deliberately flat: numbers and consequences, no explanation of why a browser wants a press and
no defending of the choice. The first draft of this gate ran to three paragraphs of cache policy and
autoplay rules, which is the kind of text a player reads once and then stops seeing. What stays on screen is
what changes the decision. The explanation of *why* a press is needed at all lives here instead, and the
reactive version (the HUD's `PRESS FOR SOUND`, shown only when the device is genuinely not running) stays
because it is telling you something is wrong, not teaching you browser policy.

Unchecking it means what it says: the game runs, the disc answers the geometry exactly as it would with
music -- the TOC, the track lengths, the game's own LBA arithmetic, every track taking the time it takes --
and every byte of the soundtrack is silence. Not one byte of music is asked for.

In C that is `nk_disc_silent_audio(1)` at the top of `nk_disc_pread` for the audio tracks, answered by the
same `fill_silence` that a dropped read uses. It is deliberately **not** `--no-audio`: refusing the MP3 path
falls back to streaming raw sectors, which is 176 KB/s for the same music, i.e. more data rather than less.
The host flag is `--no-music`, and the page's lever is `?music=0`.

**"KB streamed" had been lying, and the box exposed it.** The HUD counted `NK_ST_CDDA_BYTES`, bytes handed
to the ring, which is what the game *consumed*. That equalled what the network carried only while the music
was streamed -- silence for a dropped read, silence for a soundtrack switched off, and (once pre-download
existed) the whole score read out of memory all delivered bytes without streaming a single one. So the HUD
now reports both, from two words in the block: `123 KB music` for what the game consumed and `0 KB network`
for what the wire carried, plus `music off` when the box is unchecked.

The harness asserts all three claims: in pre-download mode `netKB` must be 0; with `?music=0` the status
block must say the music is off, `netKB` must be 0, CD-DA must still be *played* (so the silence is chosen
and not the feed dying, which used to look identical), and the CD-DA voice's own output peak in the blocks
it shares with the effects must be zero -- silence that is not silent would otherwise pass every one of those
assertions. The two-voices-audible assertion is skipped for `?music=0` for the obvious reason.

## Reproducing it

```sh
make -C wasm audio                        # encode all 14 + write tracks.tsv, probing each
KBPS=96 TRACKS="02 03" FORCE=1 make -C wasm audio
make -C wasm audio-check TRACKS="02 03 07 08 12 15"    # the sector-level geometry test
make -C wasm build && make -C wasm serve              # then open with ?diagms=2000
```

- `wasm/third_party/minimp3/` — `minimp3.h`, `LICENSE`, `README.md` (pinned commit, and why only the
  low-level header).
- `wasm/src/mp3_stream.{c,h}` — the bitstream decoder. `wasm/src/wasm_mp3.{c,h}` — the Red Book reader and
  the index. `wasm/src/wasm_disc.{c,h}` — URL-addressed windows.
- `wasm/scripts/encode_audio.py` — the encoder, the index writer, and the probe driver that refuses a file
  whose seeks do not answer to the clock it is about to publish.
- `analysis/probes/nk_mp3_probe.c` — bitstream questions, run on the shipped `mp3_stream.c`.
  `analysis/probes/nk_mp3_disc.c` — sector questions, run on the shipped `wasm_mp3.c`.

### the Opus comparison, if the codec question comes up again

Rebuilding that measurement takes a libopus for wasm, so it is written down rather than trusted to memory:

```sh
# 1. libopus 1.5.2 for wasm (the toolchain file is under emscripten/cmake, not emsdk/upstream/cmake)
curl -sfL -O https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz && tar xzf opus-1.5.2.tar.gz
cmake -S opus-1.5.2 -B build-wasm \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release -DOPUS_BUILD_PROGRAMS=OFF -DOPUS_BUILD_TESTING=OFF -DOPUS_EXTRA_C_FLAGS=-O3
cmake --build build-wasm -j8

# 2. CBR audio, repacked to a fixed stride, decoded against the MP3 in one wasm module
ffmpeg -f s16le -ar 44100 -ac 2 -t 40 -i extracted/cd/track05.bin \
       -c:a libopus -b:a 96k -vbr off -frame_duration 20 -application audio t96.opus
python3 scripts/analysis/pack_opus.py t96.opus t96.nkop 960 48000     # refuses a non-CBR encode
$EMCC -O3 -o bench.js analysis/probes/opus_decode_bench.c -I opus-1.5.2/include \
      -I wasm/src -I wasm/third_party/minimp3 -DMINIMP3_ONLY_MP3 -L build-wasm -lopus \
      wasm/src/mp3_stream.c -sENVIRONMENT=node -sNODERAWFS=1 -sINITIAL_HEAP=128MB -sALLOW_MEMORY_GROWTH=1
node bench.js "$PWD/t96.nkop" "$PWD/t128.mp3"

node scripts/analysis/jsmix.js          # what the page's sinc reader costs per block, per device rate
```
