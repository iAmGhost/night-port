# 413 — Read from the bytes: this game cannot stop CD audio, so the card's silence is the feed going idle

Ask: between the last introduction slide and the stage music starting, **how does the music stop?** I had a
fix in the tree that made the port silent there, but it was built on a claim I had never checked — that
`fn_000405a7` at `LAB_0003b1ad` was the game's "CD stop", firing ~3 s late because of the card hold. The user
asked me to find the mechanism out myself. The bytes answer it, and they also retire my claim.

`scripts/analysis/cdaudio_window.py` reproduces everything below (`uv run --with capstone -q python3 …`).
Flat address → file offset is `V - 0x2600`, the convention `scripts/analysis/findref.py` already uses.

## What the window actually contains

`room_start` tail — `0x03b130 .. 0x03b1ba`, i.e. from the last CD traffic of the show to the first of the room
loop. Annotations are mine; bytes and mnemonics are the game's:

```
0003b130  e8a95cfdff   call 0x10dde        <-- MSCDEX INT 2F AX=1510 media read   (last read of the show)
0003b135  ff3570330100 push 0x13370        ; the stage number
0003b13b  b8d1160000   mov  eax, 0x16d1    ; "stage%d.pcx"
0003b145  e8ae770000   call 0x428f8        ;   sprintf
0003b152  e837b1ffff   call 0x3628e        ; load stage1.pcx
0003b161  e8fa540000   call 0x40660        ; palette fade-in (50 steps)
0003b166  c745d800000000  xor  eax,eax / inc / cmp [ebp-0x28], 0xc00   ; THE HOLD: 0xc00 delay_units(1)
0003b183  e8d0770000   call 0x42958        ;   delay_units(1), with the DAT_13e89 ESC spin inside
0003b1a8  e848550000   call 0x406f5        ; palette fade-out (50 steps)
0003b1ad  e8f5530000   call 0x405a7        ; <-- INT 10h AX=0013: SET VIDEO MODE. Nothing else.
0003b1b2  e872f6ffff   call 0x3a829        ; the room loop's first body
0003b1ba  e81f5cfdff   call 0x10dde        <-- MSCDEX INT 2F AX=1510 media read   (first read of the loop)
```

Two things follow directly, and both contradict what the port believed:

**1. `fn_000405a7` is not a CD stop.** Its bytes are
`0x405bf: mov word [ebp-0x1c], 0x13` / `0x405cb: mov eax, 0x10` / `0x405d0: call 0x41e51` (the real-mode
request trampoline) — `INT 10h, AX=0013`, set video mode 13h, and that is the entire function. src/f calls
`sys_cd_audio_stop()` on the way there, which is the fiction docs/304 already flagged as superseded; it is
now disproven rather than superseded. It has 7 call sites (0x10caf, 0x365aa, 0x38af8, 0x3a57d, 0x3b1ad,
0x3b3ec, 0x3b52b), and every `cdda EVENT stop` the port logs from those sites is the **port's** bookkeeping,
not the game's instruction. The log line used to imply otherwise; it now says `by=port-bookkeeping` or
`by=host-card-rule`.

**2. The game has no playhead.** The whole MSCDEX surface is two functions, found by enumerating every place
the image can enter real mode with `eax=0x2f` (there are two) plus the two literal `int 0x2f` (a table of
`int 27h..31h` thunks at 0x4348f, and `AX=0x1600 INT 2F` — the Win386 Enhanced-Mode probe — at 0x48ff3):

| site | request | what it is |
|---|---|---|
| `0x104df` | `AX=0x1500` | MSCDEX installation check |
| `0x105ab` | **`AX=0x1510`** | **media read** — hard-coded in `fn_00010570`, the one dispatcher (11 callers, all inside `0x105xx..0x10dxx`) |

`fn_00010570` builds its parameter block as `[ebp-0x24]=0x1510`, a drive word from `[0x3c58]`, and a transfer
pointer from `[0x3c5c]+arg1` split into segment/offset — i.e. *read N sectors into this buffer*. No stop, no
pause, no resume, no seek. So:

- **Pause is not available to the game, and should not be added to the port.** With only bounded reads and no
  seek, "pause" and "stop" are the same observable, and a resume would be inventing behaviour the original
  never had. The question the user asked — "or just pause?" — has a firm answer from the binary.
- The silence over the card is therefore not a command. It is a **hole in the game's disc traffic**:
  `0x03b130` read, then a PCX load, two palette fades and 0xc00 `delay_units(1)` reads with nothing touching
  the CD, then `0x03b1ba` read. The shipped feed was measured as I/O-port transfers (docs/166: 324 transfers),
  i.e. a *fed* stream — so when the feeding stops, what is buffered plays out and the game goes quiet until
  the room loop starts reading again. **The game does not stop the music; it stops feeding it.** (That the
  drain is what makes it silent is the one link here I cannot see in the bytes — it is consistent with
  docs/166 and with what the machine does, not proven by them.)

That also reframes the divergence honestly. The port's divergence is not a missing wait — `delay_units` is modelled,
and the hold costs 3 s on the desktop and real time on hardware. It is that the hold takes 3 s on the desktop and costs real time on hardware. The
port's divergence is not the missing wait; it is that **src/f models `fn_00010dde` as a no-op** (`f.h` says
so), so nothing in the port ever stops feeding the audio either, and the stream just runs on.

## What ships, and why that shape

`CDDA_CARD_STOP=1` (night.ini / `NIGHT_CDDA_CARD_STOP`): when the host sees the stage-card resource lookup,
stop the CD-DA stream. The trigger is the resource the original loads at `0x3b152` — `stage%d.pcx` is built at
`0x03b13b` and loaded at `0x03b152`, and in src/f the name is assembled at exactly one site (f.c:28836), so
it cannot misfire on a lookalike. It is the *observable* the original produces at that window (silent over
the card, stage track from the room loop onward), implemented as one event, rather than an emulation of the
feed that produces it.

The alternative the mechanism suggests — a feed-idle watchdog ("if the game has issued no CD call for N ms,
let the buffer drain") — is the more faithful shape and the one that would generalise to every hold in the
game. It is not what ships: it would need the read sites modelled in src/f, and docs/404 is on record that
this subsystem's failures are ones my pre-chosen fuses miss. One variable per boot, in `night.ini`.

Measured on the desktop with `--script` anchored on `wait res opening.fli` (so it no longer presupposes the
phase it is measuring — the geometry-anchor trap in docs/410):

```
cdda EVENT play idx=11 lba=223146 sectors=17856 tick=21970      <- the show's track, the game's own request
cdda card stop: stage1.pcx on screen and the game has issued no CD call since 0x03b130
cdda EVENT stop tick=27372 by=host-card-rule                     <- 56.1 s; the card is up
cdda EVENT stop tick=28859 by=port-bookkeeping
cdda EVENT play idx=2 lba=40415 sectors=19639 tick=28859         <- 59.2 s; the room loop's track starts
```

`CDDA_CARD_STOP=0` gives zero `card stop` lines (the old behaviour) and a warp-stage run still sees the card,
so the rule is not episode-dependent. `make test` ALL PASS, `pocket/src/night` `make test` exit 0, so the
shared `src/f` and both hosts still agree.

## Parity: the SDL host had the same divergence, and now shares the same rule

The first cut of this lived in `pocket/src/night/main_of.c`, which left the desktop port doing the old thing
— a reasonable bet, since the Pocket fix was written against a Pocket observable. It was checked, not
guessed. `build/night --rig-dump` on the episode-1 route fed CD-DA at ~0.93x real time right across the card
window (3.15 MB between the two switches, no starvation), and its only stops were each one immediately
followed by a play: no silence, exactly as on the Pocket.

The rule therefore moved to `src/sys/sys.c`, into `sys_res_notify()` -- the single point where a resource
name is announced in order on *both* hosts (`src/f/f.c:3058`; the Pocket host registers no
`sys_set_res_hook` and samples `g_res_trace` instead, docs/380, but the announce itself is shared). The name
test, the gate and the stop are that one function; each host opts in, and each logs the stop in its own voice
with the attribution read off `sys_cd_card_stop_calls()`:

| host | lever | measured |
|---|---|---|
| SDL (`build/night`) | `NIGHT_CDDA_CARD_STOP` (default 1) | play idx=11 @16.76s -> **stop of a live lba=223146 @32.68s** -> play idx=2 @35.97s = **3.29 s quiet** |
| SDL, lever 0 | | stop and play both @35.96s = zero silence (the old behaviour) |
| Pocket shim (`app_pc`, -DOF_PC) | `night.ini CDDA_CARD_STOP` | `stop by=card-rule` @tick 27360 -> play idx=2 @tick 28837 = **3.01 s quiet** |

Two things worth keeping. The desktop's 3.29 s and the Pocket's 3.01 s bracket the hold the clock model
predicts (0xc00 `delay_units(1)` = 3.07 s), which is a mildly surprising agreement between two hosts that
drive audio by different mechanisms (SDL audio stream vs a mixer ring). And the `by=card-rule` /
`by=port-bookkeeping` split is what makes the 7 fictional stops legible instead of invisible.

**This changes the desktop's audio schedule**, so anything whose basis spans the card window needs
re-running: two-host audio diffs (docs/365) and CD-DA-vs-container comparisons (docs/114, docs/307). The
fixture suite is unaffected -- the gate defaults to 0 inside `sys.c`, so `test_f1` sees the old behaviour
until a host turns it on (`make test` ALL PASS).

## Confirmed on hardware -- and the hold costs 2.4x more wall time on the Pocket

Player confirmed the behaviour on a 0.1.93 boot. The card log (`Saves/night/common/nightlog.sav`, read forward
from the boot marker per docs/402) stamps it:

```
cdda EVENT play  idx=11 lba=223146 tick=21580      <- the introduction track
cdda EVENT stop  tick=24236 by=card-rule           <- the card is up; the rule fired
cdda card stop: stage1.pcx on screen, no CD call since 0x03b130 ...
cdda EVENT stop  tick=28130 by=port-bookkeeping
cdda EVENT play  idx=2  lba=40415  tick=28130      <- the stage's own track
```

The attribution works (`by=card-rule` appears exactly once, the 7 video-mode sites report
`by=port-bookkeeping`), and `pit=490 Hz` on the boot's own `memstat` line makes the tick unit wall time:
**the quiet stretch is 3894 / 490 = 7.95 s on the device.**

That is 2.4x the desktop's 3.29 s for the same `0xc00 delay_units(1)` hold, which is the same ratio docs/411
measured independently on a different phase (8.1 s on the device against 2.6 s on the desktop for the
opening). The two are consistent, and both point at the same thing: the hold is a loop of DOS clock reads, and
a clock read costs more wall time on the Pocket than here. So the *rule* is right and the *length* is not
host-independent. If the original's card is around 3 s, the Pocket now sits in silence roughly 8 s per
episode -- audible, and the lever is the cost of that loop, not this stop. Nobody has measured the
original's number for this window (docs/377's route has never been run for its audio), so the ear is still
the only evidence for what the right length is.

## Left open

- **Was the original silent at all, and for how long?** This is the assumption the whole change rests on, and
  it comes from the user's ear, not from a capture: the shipped capture's mix has little audible CD-DA
  (docs/304), and docs/377's original-side frame route has never been run for the audio of this window. If
  the original instead keeps playing, `CDDA_CARD_STOP=0` restores that with no rebuild of the rule.
- **Is the hold 3 s on the original's clock?** Measured since: **3.91 s** (docs/415). So the desktop's 3.29 s is
  close and the Pocket's 7.95 s is roughly double -- the device's `delay_units` cost, not this rule. Now measurable, not yet measured: docs/414 shows the container
  route holds a realtime factor of 1.000 (calibrated against the DOS clock itself), which is what makes this
  question answerable at all -- and shows the attempt that failed, and why. 0xc00 `delay_units(1)` = 3.07 s under the port's clock model.
  If the original's card is briefer, the lever is the cost of that loop, not this rule.
- **Does the show's track actually start at 56.1 s on the original's clock?** The port's own clock model says
  yes; nothing here measures the original. The hold is 0xc00 `delay_units(1)` = 3.07 s
  on the desktop clock model; the original's own timing for that loop is not measured, and the audible answer
  ("3 s of dead air before the stage music" vs "music ends as the card appears") is a difference the user can
  settle by ear faster than I can settle from bytes. This build gives the 3 s version. If the ear says the
  gap is shorter, the lever is `delay_units` cost in the hold, not this rule.
- **The 7 fictional stops.** `sys_cd_audio_stop()` fires at 7 sites where the original only sets video mode.
  Most are adjacent to a new track selection, so they are invisible; removing them is now a *well-evidenced*
  change rather than a guess, but it changes audio at all 7 sites and deserves its own measured build.
- `DISC_RAMP` / the 8 s opening-phase difference are unaffected by this change (docs/411).
