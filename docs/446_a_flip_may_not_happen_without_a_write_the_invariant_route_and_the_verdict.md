# 446 — a flip may not happen without a write: the invariant, the route, and the verdict line

The flicker closed in docs/445 had no test, and it survived five builds (docs/423/424/426/443/445) because of
that. This document is the regression protection, and the honest part of it is *what cannot be tested*.

## 1. The bug is untestable on the desktop shim, by construction

The shim builds from the same `main_of.c` with `-DOF_PC`, but it never takes the native-aperture blit branch
that the device does, and its `frames.bin` is not reproducible (docs/432). So no assertion about pixel
equality between host and device would have caught this, and no replayed `frames.bin` diff would either. The
only place the bug is visible is hardware — which means the guard has to be something the **hardware run says
out loud**, plus something that makes sure a hardware run actually walks the path.

## 2. The invariant, in the device path (`0.2.20`)

```c
if (!skip || g_pal_pending) g_wrote++;          /* the write block ran */
...
if ((skip || smp) && !g_pal_pending) g_present_noflip++;
else { of_video_flush(); of_video_flip(); g_flipped++; }
...
if (g_flipped > g_wrote) NLOG("INVARIANT flip without write: f=%u skip=%u smp=%u pal_pending=%d ...");
```

**Why counters and not a buffer read-back.** The tempting instrument — hash the buffer being presented and
compare it with the game picture — is not available. `of_video_flip()` does not say which of the three buffers
becomes visible, and `surface_buffer_index()` can only name the buffer *we* wrote. So "did the presented buffer
hold the picture" is unobservable from inside the core, and the only thing left that is not a guess is the
structural rule the fix enforces: **never flip on a frame that did not write**. The counters make a refactor
that quietly breaks that rule talk, in a log, on hardware — where the bug lives.

Cost when nothing is wrong: two increments and an integer compare per present. No MMIO, no extra pass over the
surface (docs/411: a diagnostic can be the pathology). `PRESENT_SKIP=1` ships on, so this runs in a player's
build too.

## 3. The route, so a hardware run walks the trigger

`scripts/playtest/route_card_skip.txt`. The trigger — established in docs/445, never by me — is ESC-skipping
the episode-1 stage cards on the way into `stage1.pcx`. On the pad, ESC is `OF_BTN_SELECT` → make code `0x01`
(`main_of.c:2176`); the cards are `1s1/1s2/1s3.pcx` (names taken from device logs, not invented). Presses are
paced rather than sprayed, because the original aborts a card on any key **DOWN** (docs/415): a press landing
while a card is up is the skip, a press between cards is a stray key into the game.

The route runs on the shim and in the container, and guarantees only the **logic** half (card skipped mid-show,
geometry changing while the display path rotates). That is still worth having: it turns a two-minute hardware
confirmation into a confirmation rather than an exploration.

## 4. The verdict line, and the bug I caught in it on the first run

`logreport.py` now ends every run with the invariant: `OK -- flipnw=0, noflip reached N`, `VIOLATED`, or
`NOT MEASURED`. The first version of that line printed **`flipnw=0` for a `0.2.18` log that has no `flipnw=`
field at all** — an invariant reported satisfied using a number that does not exist in the file. That is the
sixth parser/report self-hit in this stretch of work (docs/433 counted five), and it is why the distinction
between *"measured as zero"* and *"this core does not report it"* is now explicit in the output.

## 5. What a future hardware run has to show

| line | meaning |
|---|---|
| `flipnw=0` | nothing was presented that nobody wrote |
| `noflip > 0` | the skip path is live — without it, `flipnw=0` is vacuous and proves nothing |
| flicker after ESC-skipping cards | the bug is elsewhere; `smean`/`pmean` (docs/443) are the columns that locate it |
