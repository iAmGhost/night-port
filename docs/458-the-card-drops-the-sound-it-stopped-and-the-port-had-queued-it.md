# 458 — The card drops the sound it stopped; the port had queued it, so every effect waited behind the one the game had already cut off

Reported against the browser port, and true of the desktop one too: *"when sound plays simultaneously it feels
like stacking — it's like waiting for the current sound to stop and another played."* Timing was not the
complaint, and the music was not the complaint. Only effects, and only when a second one arrived.

## The card says "stop", and stop is about the future

The card path is single-voice. The game cuts the sound it is playing before arming the next: the DSP halt
(`0xd0`/`0xd1`) lands between the transfers, `au_transfers` and `au_stops` move together — 62 and 62 over the
opening, 20 and 20 over a stretch of the stage. `docs/04-sound.md` §6 said this in one line and the code
ignored it: *"the card path is single-voice, stops previous on new play"*.

What the port did with a stop, until now (`src/sys/sys.c:250`):

```c
case 0xd0: case 0xd1: au_stops++; dma_armed = -1; break;
```

Disarm the channel. Keep everything else. And `au_start()` does not replace the transfer, it **appends** it —
`au_put` walks `au_w` forward over whatever is already in the ring. So the game says *cut that*, the model
answers *I will play all of it, and then the next one*, and that is the stacking.

A DMA halt on real hardware abandons the transfer: the bytes the DMA has not fetched are gone, and a re-arm on
the same channel replaces it. The port had traded "abandon" for "defer", and a deferred second is heard as
every sound waiting its turn.

## The measurement

Reproduction is the opening, **not skipped** — the attract show is where the effects are, and it plays music
under them, which is the case that shows the defect. 45 s, one pointer press for the AudioContext, no keys:

```
audio=11111Hz pend=0 xfer=62 stops=62 sfx=332336B/28030B dropped/0B deferred
        (11 rate changes marked, 0 flushed: 0B) cdda=7528752B tracks=2
        at stop n=62 board avg=20265 max=45737 page avg=415 max=659
```

The last field is the one added for this: at the instant `au_stops` increments, how much of the **sound being
stopped** is still queued — `board` in bytes of the board's ring, `page` in frames the page had already taken
into its own. An average of **20 265 B still queued on the board, worst 45 737 B**. At the board's 10000–22050 Hz
that is roughly **a second of audio the game had already cancelled**, every time it cancelled one.

The counters that stayed green through all of it: `pend=0` (the board always drained), `0B deferred`, no
underrun, no drop. `pend` is zero because the host drains eagerly — which is what makes the queueing
*invisible*: the bytes leave the board ring promptly and then sit in the page's, in order, behind the sound
that was supposed to have ended.

## The fix

The stop has to discard, and it has to discard **wherever the bytes have got to**:

- `src/sys/sys.c` — the halt abandons the transfer: `au_r = au_w`.
- `src/app/main.c` — the desktop host flushes its SDL stream when `sys_audio_stops()` moves, or the bytes it
  pulled out of the board's ring keep playing from the stream and the halt is inaudible. CD-DA is bound as a
  separate stream, so the music is untouched by design.
- `wasm/src/main_wasm.c` — same for the page's ring (`ring_flush`, which bumps the reader's generation).

Measured over the same opening afterwards:

```
audio=11111Hz pend=0 xfer=62 stops=62 sfx=538236B/0B dropped/0B deferred
        at stop n=62 board avg=245 ...
```

245 B average, against 20 265 B. The 62 transfers are the same 62 transfers; the bytes left behind are not.

## A corollary that only showed up after

`au_start()` copies a **whole** transfer into the ring at arm time — the board's ring holds a sound in full the
moment it is armed — so a scene with many effects queues them in bulk, and the page's queue has to be deep
enough to receive that bulk or the host throws effects away. With the queue at 60 ms the opening dropped
**28 030 B of effects in 45 s**. The desktop host's own bound is 96 KB of SDL stream, about half a second, so
the browser port's default went to 300 ms (`?sfxms=` tunes it; `wasm_block.h` calls the words `NK_CFG_SFX_MS`
and `NK_CFG_CDDA_MS`). Same opening after: **0 B dropped**, and 538 236 B of effects reaching the mixer against
332 336 before — the extra 61 % is audio that had been discarded.

Sound order does not change, so this is not a latency trade. It is a drop count.

## What is not claimed, and what has to be re-run

- **The audio oracle has to be re-run.** `docs/129`, `docs/164`, `docs/170` compare the port's PCM against a
  capture of the original's mixer, and the halt now changes what the port emits — for the better, if §6's
  reading of the card is right. If the correlation goes **down**, then the halt is *not* an abandon in this
  game's hardware path, and the queueing was right and the stacking is the original's own behaviour. That test
  is the arbiter and it has not been run.
- **`sys_audio_pending()` is wrong across a wrap.** It is `au_w - au_r` where both are indices mod
  `AUDIO_RING`, so when `au_w` has lapped `au_r` the result is a huge unsigned: the same run reports
  `max=4294850898` (= 2³² − 2 482 142), which is no bytes at all. Pre-existing, unrelated to the halt, and now
  visible because the field is printed every second. A real lap counter or `(au_w - au_r) & (AUDIO_RING - 1)`
  fixes it; it has not been touched here so that this commit is only about the stop.
- **Mixing was not added.** `docs/04-sound.md` §6 permits it (*"the port may mix concurrently for better feel;
  keep 'latest wins' only if A/B shows audible difference"*). What A/B shows is that the original cuts, so this
  commit makes the port cut. Concurrent voices would be a host-side change, not a model change, and it would
  move the port away from the card on purpose.

## What to keep

- **A stop command is a statement about the future, and a queue of bytes has no way to honour it.** Every model
  that stores samples instead of time will answer "stop" by deferring, unless the discard is written where the
  bytes are — which is in the board's ring *and* in each host's queue *and* in the page's ring, three places.
- **An eager drain hides a queueing bug.** `pend=0` meant "the board is empty", not "nothing is backed up": the
  backlog had simply moved downstream of the only counter that looked at it. Counters must cover the whole
  path the bytes take, or the healthy ones are decoration.
- **The repro was the thing the player does not skip.** Every earlier audio measurement pressed through the
  attract show, so the one scene with dense overlapping effects was never in the sample.
