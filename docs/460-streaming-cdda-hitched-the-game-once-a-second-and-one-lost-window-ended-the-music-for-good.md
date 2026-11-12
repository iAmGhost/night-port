# 460 — Streaming CD-DA hitched the game once a second, and one lost window ended the music for good

Two reports against `wasm/`, from a phone:

1. **the music streaming makes the game stutter, and the stutter is constant — "like every second, intervaly"**;
2. **it is very unstable: it stops playing after a while and never recovered** — and the reporter's own follow-up
   said the stutter is gone when the music dies, which is the whole diagnosis handed over for free.

Both were symptoms of one design decision: `wasm/src/wasm_disc.c` awaited its HTTP reads *inside the game's own
busy-wait*. One of those decisions was a genuine trade-off — "the fetch is a yield, the browser gets its event
loop back". The other was not: an ASYNCIFY unwind stops the game's clock, not just the game's stack, and a byte
source that stops the clock once every 1.71 s of music is a byte source that stutters the game once every 1.71 s
of music. The second report was the same file's error handling: a failed read was a verdict on the whole run.

## Making it measurable first: the harness can put a network in front of the reads now

Every measurement below is from `wasm/scripts/headless.js` with the new `NIGHT_NET`, which drives Chrome's
`Network.emulateNetworkConditions`:

```
NIGHT_NET='latency=250'                     a 250 ms round trip on every Range read
NIGHT_NET='throughput=150000'               a 150 KB/s downlink
NIGHT_NET='offline=9000-20000'              the connection is DEAD between t=9 s and t=20 s
```

`offline=A-B` is what makes `fetch` reject the way a dropped Wi-Fi does, and it is the only way to ask this port
"what happens when the network blinks" without waiting for one. Chrome applies the emulation per target, so it
goes on before the navigation; `make check` goes through the service worker as always.

The other half of making it measurable were three counters on the pump line, because the old line could report
`fails=0` and a dead track in the same breath:

```
disc reads=1752 fetched=16/4816896B fails=31 waited=13(max 3ms) silence=2086224B
```

`waited` counts the reads the game had to *stop* for and `max` the worst such stop — that counter and the pump's
own `max_gap` are the stutter, side by side. `silence` is bytes answered with silence because the bytes did not
come, which is a hole in the music that used to be invisible.

## 1. The stutter: the read-ahead was a stop, not a look-ahead

Before: a 64-frame (150 528 B, 1.71 s) window, and `nk_disc_pread` awaited the fetch that refilled it — including
the *read-ahead* fetch, which is the one the game does not need yet and therefore should never wait for. With a
250 ms round trip:

```
OLD, NIGHT_NET=latency=250
  disc: the game waited 262 ms for a Range read at 0 of track 2
  cost: max_gap=263600us ... max_gap=292700us ... max_gap=294700us ... max_gap=298900us
  CHECK: 32.7 frames/s        (and the track died at pos=192, see §2)
```

Every number on that `max_gap` line is a different window: the pump's gap was 260–300 ms once per window, i.e.
once per 1.71 s of music, which is exactly "like every second, intervaly". `yield=32253us/frame` was the same
thing seen from the other side. With no emulation at all the round trip is ~1 ms on loopback and the hitch is
only ~20 ms — a phone on a real network is not.

Now the ask is **fire-and-forget**: `nkjs_start` hands the request to JS and returns a token; the bytes are
written into the wasm window by the completion, in one synchronous block, only if the token still matches what
C is waiting on (`superseded: these bytes are nobody's`). C adopts a window by polling it. The window went from
64 frames to 128 (301 056 B, 3.42 s of audio, two of them = 602 KB of static memory, which costs 874 bytes of
download — it is an inactive data segment), and the read-ahead starts at 64 sectors (0.85 s) of a window left
rather than 16. The ask rate is unchanged: 301 KB per 3.42 s is the same 88 KB/s as before.

```
NEW, NIGHT_NET=latency=250
  disc: the game waited 255 ms for a Range read at 0 of track 2 (cold)
  disc: the game waited 256 ms for a Range read at 0 of track 10 (cold)
  cost: max_gap=256400us ... max_gap=5200us ... max_gap=291400us ... max_gap=36500us ... max_gap=38300us
  disc reads=1751 fetched=16/4816896B fails=0 waited=2(max 255ms) silence=0B
  CHECK: 38.0 frames/s
```

Two waits in a 30 s run, both *cold*, and nothing above 38 ms anywhere else. The cold case is the first window of
a track: the ring is empty whether the game waits or not, so waiting buys the music's first sample instead of
losing a quarter of a second of it, and `NK_DISC_WAIT_COLD_US` bounds it at 1.5 s. Mid-stream the game waits at
most 200 ms, and only for a request it started itself.

The side-by-side, same machine, same build, `WAIT=30000`:

| | frames/s | worst pump gap | reads the game stopped for |
|---|---|---|---|
| OLD, no emulation | 45.9 | 87.9 ms | not counted, gaps ~20 ms per window |
| OLD, latency 250 | 36.8 | 309 ms | once per 1.71 s of music |
| NEW, no emulation | 46.0 | 87.5 ms | 3, worst 2 ms |
| NEW, latency 250 | 38.0 | 291 ms | 2, both cold, worst 256 ms |
| NEW, latency 80 + 150 KB/s | 43.2 | 119.9 ms | 2, both cold, worst 85 ms |

The last row is what a phone looks like: a link slow enough that a 300 KB window takes 2 s to arrive, still
streaming the whole song with nothing counted against it.

## 2. The death: three ways a read failure ended the music forever, and none of them needed the server to be down

**a) `cdda_pump` treated any refusal as the end of the track.** `if (cd_iso_read_audio(...) != 0) { g_cdda_sectors
= 0; break; }`. Nothing in the port re-PLAYs a track — only the game's next `fn_10b1d` does — so one failed read
was silence until the next card or room. With a 4 s outage, measured on the old code:

```
disc: a Range read at 451584 of track 2 got no answer (the window is treated as the end of the track)
cdda: read failed at pos=192/3153 of lba=37262 -- the track is abandoned
pump: t=15008ms ... pos=192 ...  t=20048ms ... pos=192 ...  t=25044ms ... pos=192
```

**b) a status that was not 206 latched `g_range_unsupported` for the whole page**, so every later read returned
−1 and CD-DA stayed silent for the rest of the session. The latch exists for a real reason — a server that
ignores `Range` would hand the page an 87 MB file instead of a 150 KB window — but the old code set it for a
404, a 500, a captive portal and a closed socket exactly as it did for a `200`-instead-of-`206`. It is now set
for `200` and for nothing else.

**c) a HEAD lost at boot mutes a track permanently.** `cd_iso` marks a track it cannot size absent
(`g_track_absent`), and only a re-open clears it. `nk_disc_size` now tries three times, 200 ms and 800 ms apart,
in the boot window where nobody is playing yet.

What a network that blinks costs now is silence for exactly as long as it was down, and then the music resumes
**on the same track, in step** — because the reader keeps advancing on the wall-clock budget (docs/302: an
allowance re-granted per pump call is not a lead) and answers a missing window with silence rather than a
refusal. 12 s of dead connection, measured:

```
NIGHT_NET='offline=8000-20000'
  net: OFFLINE at 8000ms        net: back at 20000ms
  disc reads=1368 ... fails=31  waited=13(max 1ms)  silence=2013312B      (t=20 s)
  disc reads=1752 ... fails=31  waited=13(max 3ms)  silence=2086224B      (t=25 s)   <- silence stops growing
  pos=607 (t=25 s) -> pos=981 (t=30 s)                                    = 74.7 sectors/s = real time
  CHECK: 45.9 frames/s, 2 CD tracks played, no "track is abandoned"
```

2 086 224 B of silence is 887 sectors = **11.8 s against an outage of 12.0 s**, and the `fetched` counter starts
climbing again the moment the connection does. The pump line now says so out loud when it happens:

```
disc: a Range read at 1211280 of track 2 got no answer -- silence for 1000 ms and then again (the track is NOT abandoned)
```

`-1` from the byte source is now only ever the truth about the track — past its last byte, no such file, or a
server that does not do Range — which is what makes `cdda_pump`'s "the track is abandoned" line (added here, so
the symptom has a message attached to it) mean what it says.

## 3. Two bugs in the first version of this fix, both caught by the counters

**It threw away slow fetches.** The first cut answered a wait that timed out by marking the window EOF, backing
off, and asking again — so a request that landed 50 ms late was discarded and re-asked, and a longer outage
never recovered at all (`silence` grew and `fetched` stayed flat for the whole rest of the run). A slow fetch and
a failed fetch are different things: `NKJS_GO` (still on its way) now means silence for this sector, no new ask,
and the next pump adopts the window when it fills.

**It asked the network for a new window per sector.** With the ring empty and the reader walking one sector per
pump, every miss that found nothing in flight started a fresh 300 KB window 2 352 B further on — 50+ per second,
ping-ponging both slots, and every answer landing too late to be for anything:

```
NKJS start slot=1 tok=139 off=1046640 len=301056
NKJS start slot=0 tok=140 off=1048992 len=301056
NKJS resp slot=1 tok=65  stale=true status=206
```

The fix is to ask "is a request already on its way that can answer this sector, or one of the 127 behind it?"
before asking anything. Measured in that state: 445 window asks in a 22 s run, 47 responses arriving to find
their request already replaced. Same run after the question was added: 38 asks, none stale. Those
`NKJS` lines were temporary instrumentation in the completion handler; they are gone, and this section is what
they were for.

## 4. What is still a stop, and what is still not claimed

The **cold wait** is the one remaining stop: up to 1.5 s, once per track, on a network that is slow rather than
dead — in these runs, 255 ms at a 250 ms round trip and 85 ms at 80 ms + 150 KB/s. It could be removed
altogether by warming the first window at PLAY time instead of at first read, which needs an LBA → (track,
offset) seam from `cd_iso` that does not exist yet; at a phone's round trip it costs a hitch at a card
transition, so it can wait for someone to mind.

**120 ms is still the CD-DA ring's depth** (`?cddams=`). The ring is what rides out a game-loop stall, and the
disc window — 3.4 s of raw bytes — is what rides out a network one, so the two cushions are no longer doing the
same job and the ring does not need to be deepened for the network's sake. Deepening it delays the heard music
by the depth, which is a different taste.

**No iPhone has seen any of this**, in the sense docs/457 means it: every number above is Chrome's network
emulation against a Range server on loopback. What the emulation can model is latency, bandwidth and a dead
socket; what it cannot model is a cellular handoff, a proxy that rewrites headers, or iOS suspending the tab —
and the last of those is where the reader's re-anchor path (the wall clock runs away from the reader, so it
re-anchors instead of racing) has never really been exercised. A report from a phone that this made *better* is
worth as much as a report that it did not.
