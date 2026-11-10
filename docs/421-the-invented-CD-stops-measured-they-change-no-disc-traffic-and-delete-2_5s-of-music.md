# 421 — the invented CD stops measured: they change no disc traffic, and they delete 2.5 s of music the original keeps playing

docs/413 proved from the bytes that `fn_000405a7` is `INT 10h AX=0013` — a video mode set — and that this game
has no CD-audio stop to issue (its only MSCDEX calls are `0x1500` and `0x1510`). It left the removal open
because it is an audible change with no measurement. docs/419 built the gate. Here is the measurement.

## What is gated

`src/f/f.c:810` — `void fn_000405a7(void) { sys_vga_set_mode(0x13); sys_cd_audio_stop(); }` is the **only**
caller of `sys_cd_audio_stop()` in the game layer (15 references to the function itself, one definition of the
stop). The gate therefore suppresses exactly the invented stop and nothing else:

```c
static int g_cd_bookkeeping_stop = 1;
void sys_cd_audio_stop(void) { g_cd_stops++; if (g_cd_stop && g_cd_bookkeeping_stop) g_cd_stop(); }
```

The call **counter still counts**, so the hosts' `by=port-bookkeeping` attribution keeps working either way, and
the card-silence rule (`res_is_stage_card`, a different code path in `sys_res_notify`) is untouched by this
gate. Exposed as `NIGHT_CDDA_STOP_FIX` (SDL) / `CDDA_STOP_FIX=` (night.ini). **Default 1 = unchanged.**

## The A/B (pocket path, same route/seed, warp to stage 1, 2000 frames)

```
                 stop events   play events   disc requests
fix=1 (ships)         5             4          236 reqs -> 2808 sectors
fix=0                 1             4          236 reqs -> 2808 sectors
--diff: only in A: 0   only in B: 0   shared: 2808 (every shared request byte-identical)
```

Two results, and the second is the one that reframes the question:

1. **Four of the five stops are the invention.** With it off, one stop remains — tick 3492, the card rule.
2. **Removing them changes nothing the disc can see.** Identical sector-level request sets, byte for byte. The
   feed is driven by the game's own requests and the ring clock, not by the mixer stop, so `--diff` alone would
   have said "no effect" and stopped there. The difference is entirely in the **mixer output**, which the audit
   cannot see — it had to be measured from the play/stop event ticks.

Silence inserted between a stop and the next play, from the event ticks (÷490 Hz):

| | silences |
|---|---|
| fix=1 (ships) | **2.50 s**, **0.26 s**, 3.02 s |
| fix=0 | 3.01 s |

The 3.0 s one is the card rule and is present in both (it matches the original's 3.91 s card window, docs/415).
The **2.50 s and 0.26 s are invented**. The worst of them is right at the start: the event log at fix=1 reads
`play idx=1` and `stop` at the **same tick 1223** — the port starts the track and kills it immediately, then
waits 2.50 s for the next `play`. At fix=0 that track runs until the game itself supersedes it 0.26 s later with
`play idx=9`. That is the original's shape: audio keeps playing until the game asks for something else.

So the faithful direction is `CDDA_STOP_FIX=0`, and it is still shipping at 1: it is an audible change, and the
next boot is already carrying `PRESENT_SKIP`. One variable per boot — docs/404's record of pre-chosen fuses
missing the actual failure is the reason I am not bundling these.

## A correction to docs/419, found doing this

docs/419 said "a non-empty diff is a real change, with no scheduling excuse available". That is true of the
**pocket path and false of the SDL host**: two identical `build/night` runs requested 2361 and 2366 sectors,
with differing sector sets, while two identical shim runs requested the identical 2808-sector set. The reason
is structural — the pocket feed is driven by the 490 Hz tick pump, the SDL host's by wall-clock demand from the
audio callback — so the amount read inside a fixed *frame* count genuinely varies there.

Practical rule for anyone using the gate: `--diff` is an exact instrument on the pocket path; on the SDL host
treat a small tail difference as variance and a *content* difference (same request, different bytes) as real —
which is exactly what it still checks hardest.

## The fixture debt this exposed, now closed

`test_f1.c:14224` asserts `sys_cd_audio_stop_calls() == 1` ("the gate stops the CD audio (fn_000405a7) on the way
out"). That assertion encodes a **port decision**, not a measurement — nothing in the original says it — and it
only counts the CALL, which the gate leaves counted either way, so it could not have noticed the gate at all.

`test_cd_stop_bookkeeping()` now installs a counting backend and asserts both directions: enabled → backend
stopped; disabled → the call is still counted but the backend is **not** stopped. Two more checks, 3091 total,
`make test` ALL PASS. The old assertion still stands, because it describes the default.
