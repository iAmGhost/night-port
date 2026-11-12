# 457 — The browser port runs on one thread, so everything crosses as memory

**Goal:** run the same game in a browser tab. Done: `make -C wasm && make -C wasm check` boots the game in
Chrome, reaches the title, accepts `START GAME`, loads a stage, streams three CD-DA tracks over HTTP Range
requests and plays them, and the check passes at
**981 frames / 44.6 fps with 10 key events landed and 3 CD tracks played** (headless). In a real windowed
Chrome it runs **1828 frames in 30 s = 61 fps** with `AudioContext running` and 1 236 992 CD-DA frames
pulled out of the ring. Nothing in `src/f`, `src/sys`, `src/cd` or `src/app/keymap.c` was forked: the game
layer compiles in place from the repo root, exactly as `pocket/` does it (docs/330 §5).

## The layout mirrors pocket/, because it is the same trick

```
wasm/
  Makefile                 game objects built from $(REPO)/src, host built here
  src/main_wasm.c          the host twin of src/app/main.c — SDL-free
  src/wasm_block.h         the one block of memory the host and the page share
  src/wasm_disc.c          CD-DA byte source: HTTP Range reads with a sliding window
  runtime/index.html       the page: canvas, HUD, log
  runtime/shell.js         the browser side: load, blit, audio, keys
  scripts/serve.py         Range-capable static server (http.server is not)
  scripts/headless.js      DevTools driver: inject keys, read the block, screenshot, assert
  scripts/check.js         make check: stage, serve, run, assert, stop
  build/ dist/ .obj/       all git-ignored
```

`pocket/src/night/Makefile` compiles `f.c`, `fx_04794a.c`, `sys.c`, `keymap.c` and the CD layer from
`$(REPO)/src` with VPATH, so one copy of the fixture-tested code serves both hosts. `wasm/Makefile` does
the same with `emcc`, and adds only `-fcommon` — `g_tab_52da4`, `g_tab_52be0` and `g_tab_531e0` are defined
in both `f.c` and `fx_04794a.c` as tentative definitions, which clang's default `-fno-common` turns into a
duplicate-symbol link error. The Pocket Makefile has been passing `-fcommon` for the same reason.

What the browser host replaces is the SDL layer and nothing else: the same boot recipe
(`board_boot`, `heap_arena_init`, `dispatch_a(0xff)`, `fn_0445c9_startup`, `fn_03af18`), the same
`sys_set_tick_pump` / `sys_set_input_pump` / `sys_set_present_hook` installs, the same
`sys_cd_set_audio_backend` seam, the same `sys_audio_fill` drain. `sys_port_spin()` lives in `f.c`
(`src/f/f.c:16400`) and calls the two pumps, so the game's `while (cond) { if (!sys_port_spin()) break; }`
busy-waits are real here too — they run **2.3 million pump calls per second**, the same order as the
desktop host's 2.9 M/s, which is the first reason the pump has to be cheap.

## The rule that shapes everything else

The build is `-sASYNCIFY`: when the host yields, the game's whole call stack is unwound into the asyncify
stack and the browser gets its turn. In that window **the wasm instance must not be entered again**. A
second entry — a `keydown` handler, an `AudioContext` callback, a `fetch` continuation that calls a wasm
function — would push its own frames into the same unwind bookkeeping and corrupt the sleeping stack.

So the only JS→wasm entry after boot is the one call that resumes a yield. Everything else crosses as
memory, described once in `src/wasm_block.h` and mirrored in `runtime/shell.js`, with the host printing
the same numbers at boot so a drift shows up in the console instead of as a wrong byte:

| direction | words | what |
|---|---|---|
| C → JS | 0–15 | frames presented, geometry, exit state, CD tracks seen, bytes queued, keys applied, flags |
| JS → C | 16–31 | AudioContext sample rate, mute, want-quit |
| both | 32–39 + 512 slots | the key queue: scancode + make/break as `uint32`, free-running head/tail |
| both | two PCM rings | 16384 stereo frames each; C produces, JS consumes, free-running counters, no locks |

`NK_ST_KEYS` (word 10) exists only so a test can assert a key landed *in the game* rather than merely at
the page: the harness dispatches 5 real key presses (10 events), and `keys: 10` with `keysDropped: 0` is
the difference between "the browser saw a key" and "the game saw a key".

## Audio: two rings, and the resampler belongs on the JS side

Ring 0 is the sound board's DMA ring, ring 1 CD-DA. Each ring carries frames at **one** rate — the rate of
whoever wrote it: the board's ring follows the DSP (`ring_rate_sync`, and the game really does program
22050, 10989, 11111 and 10000 Hz during a single opening), CD-DA is fixed at 44100 because that is what a
Red Book track is. The page resamples ring rate → `AudioContext` rate as it mixes, holds the fractional read
position *in the page*, and reads the ring **through a callback** rather than having frames handed to it.

That last clause is a bug fix. The first mixer had the producer dump every frame the host had written into a
16-frame tap history, which is fine until the producer runs ahead of the reader — it does, the game holds the
main thread for tens of milliseconds at a stretch — and then the frames about to be interpolated have already
been overwritten by frames not needed yet. Reading through a callback means the reader owns its window; the
test feeds the same input on two schedules, dribbled and all-at-once, and the output is bit-identical.

### The filter, because "the music is fine and some effects are off" said where to look

Effects come off the board at 10000–22050 Hz and the device runs at 44.1 kHz, so every effect is resampled at
a non-integer ratio like 4.013, while CD-DA (44100 → 44100) passes through and no filter can get it wrong. The
asymmetry was the complaint. Two-tap linear interpolation on such a ratio mirrors the spectrum about the input
Nyquist and rolls the top end off like sinc²; the desktop host does not have the problem, because the same
bytes go through `SDL_AudioStream`, which filters. So `runtime/resample.js` is a Blackman-windowed sinc,
16 taps, 64-phase polyphase table per ratio, cut off at whichever Nyquist is smaller (`make resample` prints
it; `?resample=linear` brings the old filter back for an A/B):

```
  freq   kind      sinc      linear   suppression gained
    1000 signal     -0.0 dB     -0.2 dB          -0.2 dB
    4500 signal    -12.5 dB    -17.0 dB          -4.6 dB     <- the dull top end
   6611 image      -38.6 dB    -23.7 dB          14.9 dB
  10111 image      -88.5 dB    -40.4 dB          48.1 dB
  17722 image     -101.4 dB    -40.8 dB          60.6 dB
unity gain: 44100 -> 44100 DC error 0.0000 %    (the CD-DA path)
```

16 taps is a real choice, not a free one: the image 1.1 kHz above the input Nyquist is only 25 dB down, because
a 16-tap window has a wide transition band. Deeper in the stopband it is 50–60 dB.

### A rate change is recorded, not acted on

The board reprograms its rate per DMA transfer, and the page keeps hundreds of milliseconds of audio, so a
change lands *inside* a queued run. `ring_rate_sync` used to flush the ring — discarding the tail of one sound
and the head of the next, **1 621 frames (37 ms) in a 34 s run, on 4 of its 6 rate changes**. A change is now
recorded as a mark, "absolute frame F onwards is at rate R", in a table the mixer walks, and the ratio switches
at exactly F — which is what `SDL_SetAudioStreamFormat` gives the desktop host. The table is a ring of entry
ordinals with a seeded base entry, and C drops entries the page has passed from the front; see the base-entry
bug below for why neither detail is cosmetic.

That placement was a bug fix, not a tidy-up. The first version resampled in C with a `phase` carried
between `ring_push` calls — but `phase` was absolute while each call's `span = nframes << 16` was not, so
whenever the carried phase had run past the new buffer's length the call produced nothing while the caller
had already consumed the board's bytes. The log said it plainly: **15 862 B queued against 161 836 B
lost**. In JS the ring *is* the carry buffer, the read position is a float the mixer owns, and the same
run reports `0B dropped / 0B deferred`.

The rest of the model is the desktop host's, kept on purpose:

- the board is drained to a queue (300 ms on the effects, 120 ms on the disc; `?sfxms=`, `?cddams=`) even when
  nothing can take the bytes, because the game waits on the board's DMA ring and bytes left in `sys.c`'s ring
  stall `fn_04185e` (docs/129 — `main.c`'s null sink is the same decision). The depth is not a latency taste:
  `au_start()` copies a whole transfer in at arm time, so a busy scene queues effects in bulk, and with 60 ms
  of queue the opening threw away 28 030 B of effects in 45 s. The desktop's bound is 96 KB of stream, about
  half a second, so the page matches it (docs/458);
- CD-DA gets **one** 30-sector cushion granted at PLAY, and the allowance **accumulates with wall time**
  (`g_cdda_budget += el`, `src/app/main.c:385`). A first version computed `allowed = Δt_since_last_pump +
  30` and capped the reader at ~33 sectors — 0.44 s of a 42 s track, the exact mistake docs/302 calls
  "an allowance re-granted per call is not a lead";
- when the wall clock runs away from the reader (a backgrounded tab) the reader **re-anchors instead of
  racing**: the game's own CD bookkeeping runs off ticks, which freeze with `requestAnimationFrame`, so
  sprinting to catch the clock would be the one case where the heard music is wrong — and it would stream
  at 10× real time doing it;
- `cdda_play` flushes the ring, because MSCDEX's PLAY replaces what was playing (docs/105 §1, docs/307).

### The mixer scaled one voice twice, so every effect was silent under the music

Reported as "effects cannot be played at the same time as the CD audio" -- and the report came with the reason
it had been missed: the listener had been playing scenes with no music.

Both rings converge on one output, and both arrive in int16 units, so the sum has to be divided by 32768. It
was, once per ring, into the shared destination:

```js
pulled.sfx  += mixRing(RING_SFX,  L, R, n);      // writes L/R, then scales L/R by 1/32768
pulled.cdda += mixRing(RING_CDDA, L, R, n);      // adds into L/R, then scales L/R by 1/32768 AGAIN
```

The converter accumulates (`outL[k] += a`), so the two voices really did add -- but the voice mixed *first*
was divided twice. Measured: an effect that reaches 0.244 of full scale on its own reached the output at
**7.45e-6**, i.e. **−103 dBFS**, in any output block that also held CD-DA -- 17 bits below the noise floor of
the converter it was played through, which is silence by another name. In a block where the disc ring had run
dry it was at full level. That is the whole of the symptom: perfect effects in a silent scene, gone the moment
the music starts -- and the silent sink path had the same arithmetic, so a headless run mixed it wrong too.

Why every counter stayed green: `pulled` counts frames, `inPeak` is measured at the **filter's input**, and
`underrun`/`drop`/`short` are all about *whether frames arrived*, never about what came out. Nothing in the
port had ever looked at the output. So the mixer moved into `resample.js` as `mix(dstL, dstR, n, a, b)` -- each
voice in its own buffer, the sum scaled once, hard-clamped (the card's summer has no headroom either, and
`clip` counts what it eats) -- and three output-side counters came with it:

```
mixing two voices (float full scale):
  effect alone: contribution 0.2441  with music: 0.2441  (music alone: 0.6103, with effect: 0.6103)
  shared block: both=true output peak 0.8515 clipped samples 0
```

The claim is that a voice's own level does not change because another voice joined it. Under the old
arithmetic that number was 0.2441 against 0.2441/32768, so `make resample` now fails if this ever comes back.
In the page, `make check` asserts `bothBlocks > 0` (the two voices shared an output block) and that each
contributed more than 0.02 of full scale in those blocks. A 45 s opening on a real device:

```
  bothBlocks 629 · peakIn [30720, 21512] int16 · ringPeak [0.390, 0.439] float · clip 0
```

`peakIn` is the run-level peak the filter was handed, and it exists because the older `inPeak` restarts at
every re-anchor -- 62 stops in that opening -- and had already wasted an hour by looking 2.4× low. Comparing
`ringPeak` against `peakIn` is what proves the converter adds no level of its own: an impulse through it gains
**1.000×** at every rate the board programs, which `make resample` now checks too.

## Disc: Range reads, and a refusal that is not an option

`src/cd/cd_iso.{c,h}` grew one seam — `cd_iso_set_remote(size_fn, pread_fn)` — which replaces
`fopen/fseek/fread` for track files while keeping every piece of geometry logic: the CUE parse, the
LBA → track owner map, lead-out, and refusing an absent track. All the geometry stays where the fixture
suite can see it.

`wasm/src/wasm_disc.c` answers those two callbacks with HTTP: `HEAD` for the size, `Range: bytes=` for the
bytes, a 64-frame (150 528 B ≈ 1.71 s) sliding window with two-slot prefetch, and a hard failure on any
response that is not `206` — a server that ignores `Range` would deliver a whole 62 MB track into the page
instead of a window, and that has to be loud. `scripts/serve.py` exists because `http.server` does ignore
`Range` (it answers 200 with the whole file), which is exactly the case above. A 22 s run that reached
gameplay measured **1397 sector reads served from 23 fetches / 3 462 144 B, fails=0**.

The window is clamped to the track's length, and a read that gets no answer at all is the end of *that
window*, not of CD-DA. Both halves are the same bug (docs/459): asking for the 64 frames past the last byte
of a track gets the connection closed without a response, the promise rejected, and — before the `try` — the
rejection unwound through the game's own stack, killing any run at the last sector of its longest song.

The game's own view of the disc stays a *filesystem*: `cd_iso` opens the CUE with `fopen`, so the CUE and
`NIGHT.RES` are written into the in-memory FS at `/data` before `callMain`, and only the track bytes are
URLs.

## What a visitor downloads

| | bytes |
|---|---|
| `night.wasm` | 462 411 |
| `night.js` (emscripten glue, `-sMODULARIZE`) | 65 930 |
| `shell.js` + `index.html` | 20 957 |
| `NIGHT.RES.gz` → 50 964 894 B written into MEMFS | 20 200 604 |
| 14 CD-DA tracks, Range-fetched as the game asks | 546 113 232 total, largest 65 143 344 |

So the app is half a megabyte, the resource container is 20 MB gzipped, and the music arrives a 150 KB
window at a time — the browser never needs the 520 MB, and the game never needs to know the difference
between a disc and a web server. `make lite` stages everything but the tracks and the game boots and plays
silent, which is the mode to check UI work in.

## The frame clock is not a clock

The host yields inside `tick_pump` and awaits `requestAnimationFrame`. Measured in a real window that
happened to be behind another window: **37 frames in 22 s**, with the audio callback running the whole
time — Chrome occlusion-throttles a window nobody is looking at, rAF to nearly nothing. So `nextFrame()`
resolves on whichever comes first, the next animation frame **or a 30 ms timer**: the yield's job is to
hand the browser its turn, not to wait for a paint, and a game whose music plays while its picture is
hidden should still be a game. Same window, same run, after that change: **1828 frames in 30 s**.

The harness gets the opposite treatment rather than the same one: `--disable-backgrounding-occluded-windows
--disable-renderer-backgrounding --disable-background-timer-throttling`, because a test window behind a
terminal is exactly the occluded case and a measurement of throttling is not a measurement of the port.

`make check` runs headless, where there is no audio device at all
(`The AudioContext encountered an error from the audio device or the WebAudio renderer`), so `?sink=1`
makes the page pull the rings on a 20 ms timer into a scratch buffer instead: the ring and budget maths
stays under test without a speaker. `NIGHT_HEADFUL=1` opens a real window with a real device and is what
proved the device path — `ac: running`, 1314 audio callbacks, 1 236 992 CD-DA frames out of the ring.

## Seven bugs that were only visible from inside the page

The mixer bug has its own section above; it is the eighth, and the only one of them that a listener noticed
first.

**A white canvas with a picture behind it.** The first frame drew nothing, while the game was drawing
normally: `video 599: 320x200 colours=121 dac lit=255 dacmax=255` — 121 distinct indices on screen and a
fully programmed DAC. The canvas was transparent, and the page's white background showed through it,
because `present()` did `new Uint32Array(M.buffer, ptr, ...)` and `M.buffer` is `undefined`: the module
object does not export `buffer`, and with `ALLOW_MEMORY_GROWTH` the buffer must be read through `HEAPU8`
every time anyway. `EXPORTED_RUNTIME_METHODS` needed `HEAPU8,HEAPU32,HEAP16`, and `heap()` became
`M.HEAPU8.buffer`. No exception reached the console at first, which is why this one cost so long: the
throw happened inside a synchronous EM_JS call from the game's own present hook.

**The rate the page reported was erased.** `block_init()` did `memset(g_block, 0, sizeof g_block)`, and
the page writes the AudioContext rate into the config window *before* `callMain`, so the first thing the
host did was zero it: `dev: 0` forever, rings at the default rate for no reason. `block_init` now zeroes
what the host owns and leaves words 16–31 alone.

**The keys that never arrived were the harness's, not the page's.** `seen.down` (events the page's own
listener counted, before wasm is involved) separated "the browser never got the key" from "the game never
applied it" — and led to the throttled-window finding above rather than to a keymap bug.

**What looks like a stall is often a clock.** The `cost:` line the host prints every 5 s
(`pumps`, `avg_gap`, `max_gap`, `frames`, `step us/tick`, `present us/frame`, `yield us/frame`) is what
settles the argument: at 640×480 the whole blit costs **54.8 ms across 981 frames = 0.056 ms/frame**, max
0.4 ms, so a low frame count is the frame clock being throttled, never the composition. `pumps` is also
the sanity check that the game is spinning at all: ~8–14 M per 5 s window, matching the desktop host.

**"I can't hear most sounds", from a mark table with no base.** The rate marks above were written as an
append-only array, and the mixer treated its *last* entry as the one to hold back — "the newest change may
not have landed yet". With one entry present it was therefore never applied, and with several the mixer was
always one rate change behind; the first sound of a session ran at ratio 1.0 instead of 0.25, which drained
the ring four times faster than the game filled it. Hence the base entry, seeded at boot and on every flush.
The lesson is in the counters, not the symptom: `underrun`, `short` and `drop` all stayed quiet, because a
reader that consumes too fast leaves a ring that looks exactly like a reader that consumes normally. `inPeak`
— the loudest frame the filter was actually handed — and the ratio of bytes in to frames out are in `make
check` now precisely because nothing that existed could see this.

**A history buffer is a window, not a mailbox.** Described above with the fix: the mixer reads the ring
through a callback so the reader, not the producer, decides which frames it holds.

The same session also produced three filter bugs that only the offline test (`make resample`) could see, and
none of them were audible in the way a bug is usually audible: the kernel scale was inverted (the cutoff landed
at a quarter of where it belonged, which killed 4.5 kHz rather than filtering images), the tap-ready guard was
off by one input frame, and — in the *test*, not the port — a caller that reused the same output region every
block made three identical readers look three different ways.

**The harness that reported its own failures as passing.** `NIGHT_CHECK` turned the numbers into assertions and,
on failure, set `process.exitCode = 1` -- and then reached the end of `main`, whose last statement was
`process.exit(0)`, which overrode it. So `make check` came back green from a `CHECK: FAIL`, and did so for
however long the mixer had been eating one of the two voices. `make check` now exits with the verdict. Worth a
second look at every other script in this repo that sets `exitCode` and then exits.

## Running it

```
make -C wasm                # emcc; needs ~/emsdk (Emscripten 7.0.0, pinned by the EM_JS convention)
make -C wasm stage          # + data/ (NIGHT.RES.gz, the CUE, symlinks to the CD tracks)
make -C wasm serve          # Range-capable server on 8091
make -C wasm check          # stage, serve, drive with keys, assert, stop
make -C wasm resample       # what the converter does to the board's samples (no browser, no game)
NIGHT_HEADFUL=1 CDP_PORT=9343 node wasm/scripts/headless.js http://127.0.0.1:8091/index.html 20000
```

Nothing plays until you press. The page preloads the module and inflates the 20 MB resource pack behind a gate,
and the single press on its button is both the user activation the browser demands before it will let a page
make a sound (an `AudioContext` created without one comes up `suspended`) and the start of `callMain` -- so the
first thing the player hears is the game's own first sound, not one they had to wait for a second press to
unlock. The press itself is instant, because everything slow has already happened. `?sink=1` and `?auto=1` skip
the gate (no output device to unlock, and a check that needs a finger would only be testing its own harness),
and the headful harness presses it for real: a scripted `element.click()` grants no activation, so it reads the
button's box and sends a mouse event to its centre. `NIGHT_SHOT_GATE=1` photographs the gate without pressing
it; keys are ignored by the page until the game runs, so a keyboard user's Space reaches the button.

A measurement run that is about audio should say so, or it will measure nothing: half the headful attempts the
AudioContext came up `suspended` and every counter read a healthy zero. `NIGHT_REQUIRE_AUDIO=1` makes the
harness fail instead of reporting silence as a result, and `NIGHT_CLICK="x:y,x:y@3000"` can press more than
once, because a press that lands while the page is still fetching the 20 MB resource pack is not a press that
starts a device.

The page prints the host's log to the browser console and to its own `log` panel; the HUD shows frames,
geometry, CD tracks, device rate and state, and bytes played out of the rings. Keys are the DOS make/break
codes through `keymap.c` (docs/91), the same map the fixture suite asserts against.

Four URL parameters exist for measurement runs, all printed by the boot banner: `?vgaws=` and `?chargeus=` are
the A0000 window-write charge in ns/byte and the least write debt worth a wait, which together pace the melting
title, the logo fade and the room's page flip (docs/459); `?scale=fit` forces the whole-number scale of the
bitmap's own ratio instead of the default 4:3 stretch; `?skipintro=1` and `?bgmoff=N` are the host's debug flags
for reaching the title fast and for simulating the pause menu's BGM MODE row. `NIGHT_KEYS` specs can carry an
absolute time (`Escape:150@118000`) and `NIGHT_LOG_LINES` widens the console excerpt, both so that a menu can be
driven and a 200 ms sampled calibration window actually be read (docs/459).

## Not claimed

It has now been listened to — that is how the resampler, the rate marks and the queue depth were found, and
how docs/458's stop semantics were reported — but there is still no microphone capture and no spectrum of what
actually leaves the machine: the tone claims rest on `make resample`, which is the filter's response, not the
room's. No persistence — `sys`'s save files are in-memory only and a reload starts
fresh. The tracks are staged as symlinks to `extracted/cd`, so serving this to another machine needs real
files. And the throttling story is only half-solved: with the tab actually hidden, Chrome drops its timers
to ~1 Hz and the game crawls, which is how every browser game behaves and is not fixed by a fallback timer.

Not attempted: getting the mixer off the main thread. `AudioWorklet` is the shape that would fix it — the
worklet needs a big enough internal FIFO to ride out a game hold — and until then a ScriptProcessor callback
serviced by a main thread the game owns for tens of milliseconds can render *less than the wall clock's worth*
of audio; one 26 s run delivered 14.9 s of it. Queue depth does not fix that, only a thread does.

And one thing the checks do not cover, found while measuring docs/459: a run has to last more than four minutes
before anything past the episode card is expected to happen, because the card holds while the show's track
plays and that track is 238 s long. Once it can last that long the room is reached — gameplay at 320x200
stretched into a 4:3 box, 12 CD tracks streamed, ~30 fps (analysis/refs/wasm-room-320x200-stretch.png) — but
getting there needs a scripted ~5-minute key route, so `make check`'s "reaches a stage" still means the card,
and the room loop, the pause menu and the room's page-flip charge stay unexercised by the harness. Two
measured disagreements sit in that unexercised part: the room flips pages at 4.3/s in the browser against the
shipped's 15.7 (docs/77), and no scripted room has yet armed the abort flag that `fn_039c03` enters on
(docs/93), so BGM MODE has been verified by its byte and its feed, not by its row.
