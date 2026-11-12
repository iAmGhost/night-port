# 459 — Four things reported from inside the browser tab: the melt that skipped, the music that would not stop, the hint, and the 320x200 frame

Four reports against `wasm/`, all of them from someone actually playing it in a tab:

1. the melting title intro "plays way too fast, like it is skipping";
2. turning **BGM MODE** off in the pause menu stops the sound effects but the CD music keeps playing;
3. the one-line key instructions along the bottom of the screen should go;
4. the room frames — a 320x200 bitmap — should be *stretched to the 640x480 ratio*, "something like 320x210 or 320x240", not floated at a whole-number scale of their own 16:10 shape.

Three are host bugs, one is a page decision, and the first one turned out to be two bugs with opposite signs.

## 1. The melt: this host had never charged a window write, and then charging it the desktop's way overshot

`fn_03a224` — the routine that melts the title card — is 240 passes, and each pass writes 320x480 = 0x280x0x1e0 =
307,200 bytes through the A0000 window. That is 73,728,000 window bytes, and `docs/72` measured the original's
whole wipe at **7.19 s**, i.e. 97.5 ns/byte, two 486 cycles per uncacheable write into a banked VGA window. The
port models exactly that with `g_vga_window_ns` and `vga_charge()` (docs/72, docs/76): bytes go into a debt
accumulator and the game waits the debt out in `sys_port_spin()`.

**`main_wasm.c` had never set `g_vga_window_ns`.** It stayed at the unit-test default of 0, every window write was
free, and the whole melt — plus the logo fade and the room's page flip — ran as fast as the CPU could copy.

The honest way to see "as fast as the CPU could copy" is to charge the writes at 1 ns/byte: 73.7 MB then costs
74 ms of model, which is nothing, so what remains is the passes' own compositing. With `?vgaws=1&chargeus=1000000`
and `?diagms=200`, the host's own charge counter says the 240 passes happen between frames 2583 and 2595 —

```
43244 frames 2583 bytes          0
43444 frames 2595 bytes   73728000
```

**twelve frames, 0.20 s, against the shipped's 7.19 s: 35x too fast.** That is the "like it is skipping" report,
and it is not an interpolation problem or a frame-rate problem; the animation was simply never timed.

### The second half: 63 ns/byte, the desktop's number, overshoots here by 2.5x

The first candidate was the desktop host's calibrated 63 ns/byte. Charged that way the wipe came out **48 ms per
pass**, where the model asks for 307200 x 63 ns = 19.35 ms — 11.6 s for the 240 passes, 1.6x the shipped and
*slower* than the hardware, in a browser tab.

The reason is what a wait costs in this host. `vga_charge()` is called once per scanline (480 times a pass), and
each call asked for its own `sys_port_spin()`. In the desktop host a spin is a `usleep` and a pump; in the browser
host one spin is a pass of the tick pump, which runs the audio pump, a present, and — at the frame boundary — the
Asyncify unwind and the wait for `requestAnimationFrame`. 480 waits per pass therefore paid for 480 pump passes.
The same coarse clock shows up the other way in the per-scanline run: the browser's `performance.now()` is
granular, so each wait overshoots its deadline by a clock quantum, and 480 quanta a pass is tens of milliseconds.

So the charge needed a coalescing threshold, and it belongs in `src/f` rather than in the browser host, because
what it models is that the game does not observe every individual write — only that time passed. New in `f.c`:

```c
uint32_t g_vga_charge_spin_us = 0;   /* the least debt worth a wait; 0 = wait per charge (desktop, fixtures) */
...
    s_debt_us += ((uint64_t)bytes * g_vga_window_ns) / 1000u;
    if ((uint32_t)s_debt_us < g_vga_charge_spin_us) return;
```

The bytes charged and the total time charged are identical; only the waits become fewer and longer. The default
stays 0, so the desktop host and the fixture suite keep the behaviour they were calibrated against (`make test`:
all pass). The browser host asks for 2000 µs — two milliseconds of debt, about a seventh of a pass.

### The calibration

Measured headless (`?sink=1&diagms=200`), the wipe span taken as the host's charge counter climbing from its
first non-zero sample to 73,728,000 bytes — the melt's own byte count, so it cannot catch anything else:

| ns/byte | wait threshold | wipe span | vs 7.19 s |
|---|---|---|---|
| 1 (free) | — | **0.20 s** | 0.03 |
| 63 | 0 (desktop's) | 11.6 s | 1.61 |
| 63 | 2000 µs | 5.0, 6.5 s | 0.70, 0.90 |
| 80 | 2000 µs | 6.2, 6.4, 6.6, 6.8 s | 0.86–0.95 |
| 90 | 2000 µs | 6.8 s | 0.95 |
| **98** | **2000 µs** | **7.5, 7.8 s** | **1.04, 1.08** |

The ship default is **98 ns/byte with a 2 ms wait threshold**, and the arithmetic closes: at 98 the model asks for
240 x 307,200 x 98 ns = 7.23 s, the passes themselves cost 0.20 s (measured above), so 7.43 s is what should be
seen, inside the 7.5–7.8 s band. That the number to use here is the *physical* one and not the desktop's tuned 63
is the point of the coalescing: `docs/72` chose 63 because every wait on the desktop overshoots its deadline, and
once the waits are coalesced the browser has no such overhead left, so it can spend the hardware's own figure.

Both are URL parameters — `?vgaws=` and `?chargeus=` — and the boot banner prints what the run is using.

## 2. BGM MODE: the game gates the request, and this host is the stream

`DAT_00051a10` has exactly two readers in the game. That is not a hunch: `analysis/pseudo/` decompiles all 529
functions in NIGHT.EXE and `grep` finds the byte in two of them —

- `fn_10b1d`, the CD-DA track request, returns at `0x010b38` when it is clear (`f.c:2459`);
- `fn_039c03`'s third row, `m == 2` — the row the game's own `pause.spr` labels BGM MODE — writes it
  (`0x00039e30`, ported at `f.c:27613`).

So the byte is the BGM enable, and it gates only the *next request*. Turning it off also reaches
`fn_10b1d(track)` at `0x00039e8c`, which the byte's own gate rejects, and turning it on reaches `fn_00010dde` at
`0x00039e69`, which `docs/413` established is an MSCDEX media read, modelled in `src/f` as a no-op.

On hardware that is the whole story, because the heard stream **is** the game's reads: stop requesting and the
buffered audio runs out and the machine goes quiet (docs/413's phrase: the game does not stop the music, it stops
feeding it). This host cannot work that way. It streams a whole track the moment `fn_10b1d` asks for one, because
the alternative — asking the server for 23 MB one 20 ms pump at a time — is not something HTTP will survive. So a
byte that is only consulted on the next request cannot silence a track already in flight, and with BGM MODE off
the music played on to its end, which is precisely what was reported. The SFX half of the report was never a bug:
`DAT_00053338` gates the effect mixer and that path does work.

The fix is in the host, because the host is the feed:

```c
static void cdda_bgm_gate(void) {
    if (g_dat_51a10) { g_bgm_off_logged = 0; return; }
    ...
    if (g_cdda_sectors) cdda_stop();      /* flush the PCM ring, stop Range-reading the track */
}
```

called from the 20 ms `cdda_pump()` pass. Turning BGM back on restarts nothing, on either side — the shipped's
"on" branch is a media read — and the music returns with the game's next `fn_10b1d`, i.e. the next card or room.

### Verified, but not through the menu

A headless run cannot reach the pause menu, because a headless run cannot get into a room (see *Not claimed*). So
`--bgmoff=N` / `?bgmoff=N` clears `DAT_00051a10` N ms into the run, which is the one thing the row's Up/Down press
does to the machine. With `?bgmoff=14000`:

```
cdda: track index 9 lba=185120 sectors=27697 (369.3 s)
bgm: cleared DAT_00051a10 at t=14019 ms (simulates the pause menu's BGM MODE row, 0x00039e30)
cdda: BGM MODE off -- dropping the 27344 sectors still queued

t=18001ms cdda=2624832B pos=0 bgm=0 fetched=19
t=20001ms cdda=2624832B pos=0 bgm=0 fetched=19
...     unchanged to the end of the run
```

The byte goes to 0, 27,344 queued sectors (6 minutes of audio) are dropped, and from that pump onward the host
reads nothing more from the server and streams nothing more into the ring: `cdda=` bytes, `pos` and `fetched` all
frozen for the last 26 seconds of the run. What is *not* witnessed is the row itself being walked and pressed in
a tab.

## 3. The hint line

`#hint` — `arrows move · space / enter confirm · F1 help · esc pause · z x c attack and jump`, pinned
`position: fixed; bottom: 44px` — is gone from `index.html`, along with the old `#tap-audio` badge's stylesheet
rule. What the player needs to know is on the title card the game draws itself, and the page's own HUD is now
just the counters plus four buttons.

## 4. The 320x200 frame, and the ratio nobody saw

`fit()` used to size the canvas by the largest *whole number* that fits the window, from the bitmap's own ratio.
For the 640x480 slides that is fine. The room path's bitmap is 320x200 — 16:10 — and 16:10 is a shape the
original never showed anybody: mode 13h's pixels are square at the DAC and double-square on the screen, which is
why scaling a room frame to 640x480 lines it up with the slides, the title melt and `docs/66`'s desktop view.
Stretching 320x200 to 320x240 on the screen — 4:3, exactly the "320x210 or 320x240" the report remembered — is
what the box did to the picture.

The page now has two modes, in a `#screen-holder` flex box that owns the leftover height:

- **stretch** (the default, persisted in `localStorage`): the canvas fills the 4:3 box, so a room frame becomes
  640x480.
- **fit**: the old behaviour, the bitmap's own ratio at a whole-number scale.

Both are `image-rendering: pixelated`. The stretch mode had `auto` for one revision, on the reasoning that a
non-integer scale repeats some source rows and columns before others and that nearest-neighbour would show it;
the answer from the player was that it looked blurred, and the answer from a screenshot of the same frame both
ways was that it *is* blur, not a texture problem -- the same 30 s title frame composited nearest-neighbour
carries **380 distinct colours**, with smoothing **29,125**. Twenty-nine thousand colours the game never
wrote, invented by the compositor between its palette entries. `showScale()` now sets `pixelated` whatever the
mode is, and `index.html` carries `crisp-edges`/`pixelated` as the base rule so a first paint cannot be smooth.

The HUD button says the mode you are *not* in, `?scale=fit` forces the other mode for one run, and `fit()` is
now also called when the log panel opens or closes, because the panel takes height the canvas had.

## What the harness gained

Walking a menu needs real seconds, and none of this was expressible before:

- `NIGHT_KEYS` specs can carry an absolute time — `Escape:150@118000` — and run on their own clock; plain specs
  still fire back to back from the midpoint of the run, which is what `make check` has always done. A `wait:NNN`
  spec just waits. `fn_039c03` re-arms its auto-repeat counters at 0x13 frames, so the index steps two or three
  rows on one long hold (`docs/60`'s trace) and back-to-back presses cannot land on a chosen row.
- `NIGHT_LOG_LINES` sets how many lines of the page console the harness prints (it was hard-wired to 60, and a
  calibration that samples every 200 ms needs 300).
- `?skipintro=1` reaches the host's `--skip-intro`, so a run does not have to sit through the logo hold and
  `opening.fli` to get anywhere near the room loop.

## 5. Found while measuring item 1: this port was dying at the last sector of every long CD track

The long runs — 300 s and more — all ended the same way, and always at the same place: the instant the CD-DA
position reached the length of the track it was playing.

```
t=250773ms frames=7709 pos=17195        <- track index 11, sectors=17856
t=255780ms frames=7850 pos=17575
[error] Failed to load resource: net::ERR_EMPTY_RESPONSE
[error] The AudioContext encountered an error from the audio device or the WebAudio renderer.
EXCEPTION: TypeError: Failed to fetch
    at __asyncjs__nk_js_fetch_range (night.js:1:57563)
    at night.wasm:wasm-function[34]:0x343ad
```

and then nothing: the page's own report says why nothing is wrong-looking —
`"raf": {"calls": 1199, "gaps": 20015.9}` — 20 seconds since the last `requestAnimationFrame`. The loop had
not slowed down; it was gone.

The mechanism is in `wasm_disc.c`'s read-ahead. It keeps a 64-sector (150,528 B) window and asks for the next
one whenever 16 sectors remain, so at the end of a track the next window starts inside the file and runs past
it. The browser's `Range` for bytes that do not exist gets the connection closed with no response — measured
against this repo's own `scripts/serve.py`:

```
curl -r 24143280-24293807 data/cd/track12.bin   -> 206, 150528 bytes
curl -r 24294808-24445335 data/cd/track12.bin   -> no response at all
```

`nk_js_fetch_range` had no `try`, so the rejected promise did not come back as a failed read: it unwound
*through the game's own stack*, out of the Asyncify state machine, and `main()` never returned to the pump.
Every run of NIGHT in a tab died with its background music, at the end of the song.

Two changes, both in `wasm_disc.c`:

- the track's byte length, which `nk_disc_size` already learns from the `HEAD`, is remembered, and every
  window — the one being read and the one being read ahead — is clamped to it, so the request for bytes that
  do not exist is never made;
- `nk_js_fetch_range` catches the network error and returns `-2`, which the caller treats as *the end of this
  window* rather than as the server refusing Range service. The distinction matters: the old code answered a
  single failed read by setting `g_range_unsupported` and going silent for the rest of the session.

Verified on the same route that had died twice: `pos` climbs to `17856` — the last sector of track 11 — the
host keeps pumping, `fetched` stops at 307 with no failure recorded, and frames keep coming for another 45
seconds (`t=260940 frames=7987 pos=17856` … `t=306107 frames=9287`).

## The room, at last

The reason the short runs looked stuck was item 1 plus arithmetic: the episode card holds while the show's
track plays, and the show's track is 238.1 s (`track index 11 ... sectors=17856`), so a run has to last more
than four minutes before anything else is expected to happen, and until today it could not last that long. With the
fix, a five-and-a-half-minute scripted run — `?skipintro=1`, `Enter@20s`, then presses paced through the
attract cycle, `docs/459`'s `NIGHT_KEYS` timings being the only way to express it — gets into the room:

```
display mode 320x200
video 17143: 320x200 colours=112 ... 17592: 320x200 colours=116      <- it is moving
cost: pumps=152 avg_gap=32928us frames=152 present=176us/frame
STATE: frames 17735, 320x200, 12 CD tracks, 216632 KB streamed, keys 38 delivered / 0 dropped, ac running,
       canvas [320, 200, 755, 566]
```

320x200 shown at 755x566 is a ratio of 1.334: the room frame filling a 4:3 box, which is item 4 seen on a real
room frame for the first time (the screenshot is `analysis/refs/wasm-room-320x200-stretch.png`; the player is
mid-guard with two attackers coming in from the right, HUD and EXP line drawn by the game).

One number from that room phase is a lead rather than a result: between 295 s and 325 s the host charged
5,440,000 window bytes, which at the room's 64,000-byte page flip is **4.3 flips/s at 30 presented fps** — the
shipped's rate is 15.7 flips/s (docs/77). The room path in the browser is running behind the original's cadence
and nobody has measured why.

## Not claimed

**The pause menu has still not been opened in a browser run, so BGM MODE is verified by its byte and its
feed, not by its row.** The room is reachable now, and `Escape` at 304 s, `Left/Left/Left`, `Up` all reach the
machine (38 key events, 0 dropped) — and move the *player*, not the entry index: `bgm` stays 1, the screenshot
at 330 s is the fight, not a menu. `fn_039c03` is entered from the room loop only under
`DAT_00063e89 != 0 && pl[0x30] != 1` (docs/93), and nothing about that abort flag armed in a scripted room.
So the row itself — the label, the walk, the press — is unexercised in the browser, on both sides of the
change; the desktop route that walks it (`scripts/playtest/route_pause_menu.txt`) has no browser equivalent.

Not claimed either: the room path's page-flip charge, whose rate disagrees with docs/77 as just measured; and
the melt's span at 98 ns/byte carries a run-to-run spread of ~0.3 s, which is the frame-granular yield, not the
model.
