# 462 — Muting the page killed the music for the rest of the session, because the disc pump's own safety net armed itself

Reported against the browser port: **"music does not resume after mute -> unmute"**.

## What a mute does, and the one state it has to survive

The page's `mute` button writes one word, `NK_CFG_MUTED`. The host keeps draining the Sound Blaster board into
a scratch sink (the game waits on the DMA ring, not on a device, so stopping there would freeze the game), but
**it stops feeding both PCM rings, and the page stops pulling them** — pulling bytes nobody can hear is
pointless work. So while muted: the SFX ring empties, the CD-DA ring tops up to its queue target (120 ms) and
the host stops reading ahead, and `g_cdda_pos` freezes while the wall clock keeps running.

That last clause is the whole report. The unfreeze has to cope with a reader that fell behind real time, and
`cdda_pump` had a safety net for exactly that — a backgrounded tab, which stops `requestAnimationFrame` and
therefore stops the game's tick clock too:

> Re-anchor rather than race when the wall clock ran away from the reader: do not let the reader sprint to
> catch the wall clock, because then the heard music is wrong *and* it streams at 10x real time doing it.

The intent was right. One of the two lines it wrote was not.

## The trap

```c
uint32_t allowed = (uint32_t)g_cdda_budget + CUSHION;              /* CUSHION = 30 sectors = 0.4 s */
if (allowed > g_cdda_pos + CUSHION * 2) {                           /* the reader is > CUSHION behind */
    g_cdda_clock_us = now - (g_cdda_pos + CUSHION) * 1000000ull / 75ull;   /* <-- the bug */
    g_cdda_budget   = g_cdda_pos;
    return;
}
```

The next pump derives its budget from elapsed time against that clock, so it credits
`elapsed + (pos + CUSHION)` sectors, *on top of* the `g_cdda_budget = g_cdda_pos` it had just written. Worked
through, the condition for re-anchoring again reduces to `pos + elapsed > 0`, which is true every pump
forever. The branch was a trap: once armed, `g_cdda_pos` never advanced again and CD-DA was dead for the rest
of the session — ring empty or full, counters otherwise healthy.

```
cdda: fed nothing for 2693 ms at pos=9/27697 of lba=185120: re-anchored, and again on the next pump --
      ring=1204/5292 budget=9 allowed=80 want=0
cdda: fed nothing for 7721 ms at pos=9/27697 of lba=185120: re-anchored, and again on the next pump --
      ring=16/5292 budget=9 allowed=80 want=0
```

`pos=9` is 9 sectors = the whole 120 ms queue, which is as far as the ring's target lets it get. `ring=16`
means the reader had already drained it and was *hungry*: this is the host refusing to feed, not the page
failing to read.

The re-anchor does not report on sight; it counts. **One** re-anchor after a backgrounded tab or a mute is the
safety net doing its job and the next pump feeds, so it says nothing; the second one in a row, with no byte
pushed in between, is the signature of a branch that cannot be left — which is what this was.

Arming it needs the reader to fall behind real time by more than one cushion — 0.4 s at `?cddams=120`,
`CUSHION=30`. A mute does that trivially, and so does anything else that stops the page's reader for a second:
a backgrounded tab, a long main-thread hold. The player found the reliable one.

**Why `make check` never saw it.** The harness runs with `?sink=1`, whose 20 ms interval drains the rings like
a device — and in every run the trap happened to be armed *just before the game started its next track*,
whose `cdda_play` resets `clock = now`, `budget = 0` and disarms it. In three of four sink runs the music ran
at 75 sectors/s and the check passed. It showed on the real `AudioContext` path, where the pull cadence is the
device's. That is the same lesson as docs/460, one file later: a silent sink under a timer is not a device.

## The fix, which is one line shorter than the bug

```c
g_cdda_clock_us = now;
g_cdda_budget   = g_cdda_pos;      /* nothing owed beyond what has actually been read */
```

Drop the surplus and start the clock *here*. `allowed - pos` is then `CUSHION + (reader lag)`, the feed
resumes on the next pump, and the branch only fires again on a genuine lag.

```
                                              pos after the mute is lifted
                                      BEFORE             AFTER
mute 9 s -> unmute 15 s (AudioContext path)   9, 9, 9     12 -> 378 -> 756
mute 20 s -> unmute 30 s, inside one track    9, 9        10 -> 388 -> 763
```

763 − 388 = 375 sectors in 5.0 s = **75 sectors/s**, i.e. it resumes in step with the wall clock rather than
racing to catch a phantom.

## Mute semantics, stated

Muting **pauses the disc feed**; unmuting resumes it from where it stopped, after the ≤120 ms that was already
in the ring. In the original a mute is analog — the disc keeps playing — so after a long mute this port's
track is behind the scene it belongs to. That is a choice: re-anchoring the stream forward to the scene would
drop music the player never heard and would need the game's own idea of where the track is, which `src/f`
does not model. Noted rather than fixed.

## Making this class of bug fail the build instead of the player's afternoon

- **`cdda_stuck()`** in the host: when the ring is quarter-empty and nothing has been pushed for 1.5 s, the
  pump prints *which* early return is running and with what numbers, once per reason (the re-anchor only on
  its second appearance in a row, so a legitimate recovery stays quiet). The line is the defect signature, and
  `NIGHT_CHECK` now fails the run on it:
  `the CD-DA feed stopped: cdda: fed nothing for 2693 ms at pos=9/27697 of lba=185120: re-anchored, ...`
- **The other half of the assertion**: a run that muted and unmuted (`NIGHT_MUTE="6000,13000"`) must have
  consumed CD-DA afterwards (`pulled.cdda`), because every other counter stayed healthy at the end.
- **The negative control was run.** The old arithmetic was restored, the same command was run, and it returned
  `CHECK: FAIL` with exactly the line above. A new assertion that passes without the bug is decoration. The
  fix was then put back and the same command returned `CHECK: PASS (931 frames, 46.5 fps)`, and the whole
  thing was run once more under `NIGHT_NET='latency=250 offline=9000-17000'` with the pad's touch assertions
  in the same run: `PASS (682 frames, 34.1 fps)`, no `fed nothing`.
- **`NightWasm.mix()`** dumps each ring's *reader*: `avail` and `released` (both parked ⇒ a reader that
  stopped; `released` moving with `avail` ⇒ a stream), the rate it anchored to, its fractional position, and
  the host's mark-table window. This exists because the first hypothesis was "the reader stopped", and the
  counters then looked exactly like that: `minFill` parked at the ring's target, `underrun` 0, `markMiss` 0 —
  an empty ring and a stopped reader are indistinguishable in every counter this file had.
- **`NIGHT_PROBE="2000,5000"`** dumps `flags`/`btn`/`pulled.cdda`/`ac` + the CDDA reader at those times;
  **`NIGHT_CLICKLOG=1`** logs the page's clicks with `isTrusted` and a stack, installed before navigation.

## What this does not claim

- **No measurements on an iPhone.** Everything here is headless Chrome, `AudioContext` path or `?sink=1`.
- **The first diagnosis was wrong, and it is kept here as evidence of how the counters lie.** Two runs with no
  mute press came back with the music dead at `pos=9` and a full ring; they were not new bugs, they were runs
  whose page was *muted* by a stray click (`DOM click: btn-mute at 0,0`, an activation click with no pointer
  behind it — the harness's doing, not the page's: `muted` has exactly one writer and it is the button's
  handler). The probes now print the button's label, which is why they exist.
- **The board's rate collapsing to 11111 Hz in those same windows** is the game reprogramming the DMA for a
  sound (`6 rate changes marked`); it is not part of this and was not chased.
- **`pulled.cdda > 10000` is a low bar.** It asks "did any CD-DA come through", not "was it continuous" — the
  continuous case is `drop`/`underrun`, and those stay asserted separately.
