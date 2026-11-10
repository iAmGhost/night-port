# 431 — "the logo wait is back and animation feels slower" was my own 0.2.7, and the log showed it in one line

The user's report after the `0.2.7` boot: slow wait on the logo is back, animation a bit slower. Their eye was
right, and the log agreed without me having to guess.

## 1. One line of the card log indicts the build

```
perf pres=13535us ... skips=2665(hash 2665 smp 0) | rows c=451638 s=1002 dry=1002 f=0 inv=1800 mm=0 chk=1002 | br n/g/d/o=48/1794/0/0
```

`ROW_SKIP=2` was supposed to be behaviour-neutral — write every row, only *measure*. It was not, for two
independent reasons, both mine:

- **`inv=1800` against `br n/g/d/o=.../1794/...`.** My invalidation rule was `if (!native && any_valid) invalidate`,
  written when only the native branch was instrumented. After I generalized the skip to the game branch, that
  rule wiped the tables on **every single game-branch frame** — so `rows c=451 638 / s=1002`: the machinery
  hashed every row of every frame and skipped essentially nothing. Cost: a full read of the surface per frame,
  on top of...
- **`skips=2665(hash 2665)`** — the `PRESENT_SKIP` frame hash was still running too. So on any frame the skip
  could not take, the port read the whole surface **twice**: once for the frame hash, once for the row hashes.

Net effect on the numbers: `pres` mean 23.1 ms (0.2.6) → 23.5 ms (0.2.7), frames/s median unchanged at 33. That
is ~2%, which the median does not show and a thumb on the D-pad does. docs/411's shape again — the build whose
job was to measure something was also the build that measured worst, and this time the instrumentation *was*
the pathology.

The useful part: **`mm=0` across 1002 self-checks.** Every time the row verdict said "this buffer already has
these bytes", reading the buffer back proved it true. The skip logic itself is sound; it was never allowed to
run.

## 2. The fixes, and what replaces the frame hash

- The frame hash is dropped when `ROW_SKIP` is on, not doubled alongside it. The per-row verdicts do the same
  job more precisely — "every row matches what this buffer holds" *is* "this frame is already on screen", per
  buffer, with no three-frame proof. A new `fs=` counter counts frames where every row was already in place, so
  the old `skips=... (hash X smp Y)` attribution keeps telling the truth about which mechanism skipped what.
- Invalidation now belongs only to branches that *do not* keep tables (the 2×2 downscale and the
  unexpected-mode fallback). The game branch maintains its own.

## 3. `gap worst=` — because average fps was never the complaint

Every metric so far has been a rate or a sum, and the complaint is about *unevenness*. `present_step` now
records the largest inter-present gap since the last report and prints it once a second:

```
gap worst=118ms since report (last frame 21ms ago)
```

A run can hold 33 fps on average and still feel terrible if 118 ms of that is one stall every second. From
here on, "slower-ish" is a number I can compare across boots instead of a description I have to interpret.
The median and the gap answer different questions and both stay in the log.

## 4. Card state, and what the next boot should show

`0.2.8`, `ROW_SKIP=1` — promoted past the dry run because `mm=0`. Expect, in a stage with music:

- `fs` climbing in the 640x480 phases (that is the whole point: the slides, cards and attract frames where only
  a band of rows moves), and `hash 0` because the frame hash is off while rows run;
- `rows s=` large, `c=` small, and `mm` staying at **0** — if it moves at all, `ROW_SKIP=0` is the revert and
  the `ROWSKIP MISMATCH: buf=.. row=..` line names the buffer and row;
- `gap worst=` as the headline for whether any of this helped the way it plays.

Still open from `0.2.6` and untouched by this: even a perfect row skip leaves the first write of a changed row
at ~23 cycles per word, and gameplay (320x200 picture, 2.9 ms copy) already skips almost nothing. If `fs` is
high, the copy stops being the story in the static phases and the next question becomes the smaller surface —
docs/401 condemned the *software downscale*, not the small destination.
