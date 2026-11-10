# 442 — a display policy with its own clock: the fade ramp stretched the stage cards and the flicker came back

The pacing fix worked and something else broke. Player on `0.2.16`: **"long logo hold fixed, stage card is
broken: fade in/out got slower… this fix makes cards skip before fade finishes. Also stage entering flicker
is back."**

## 1. Why that is the ramp, and why it is my design error rather than a tuning miss

`FADE_RAMP=1` (docs/436/439) plays a palette move on the panel over `FADE_RAMP_MS` of **wall time**. It has
its own clock, and the game does not stop for it. Two consequences follow directly from that:

* A fade *out* is a large palette move too. The counter has been reporting exactly such an arm all along —
  `fade ramp: n=42 (entries moved=254 …) from black=no` — which I read as "probably a hard cut at the title
  card" and filed as an acceptable limitation (docs/439 §1). It is far more likely the **stage card's fade
  out**, which the ramp then stretches to 700 ms.
* While a stretched ramp is running, the game moves on. The card it was fading is replaced, and the panel
  keeps receiving interpolated palettes from the *previous* move — the new card painted in old colours.
  That is "the card skips before the fade finishes" and it is also, in its look, the stage-entry flicker
  docs/423/424 chased and never pinned down.

So the ramp is not mis-tuned; the *class* of thing it is — a display policy with an independent clock —
cannot be right for content whose timing the game already owns. The `from black=no` arm was the evidence and
I mislabelled it. Overlapping ramps make it worse: arming a second ramp mid-flight rebaselines `from` to
whatever the panel is showing at that moment, which is not a colour the game ever asked for.

## 2. The A/B, on the card now

`FADE_RAMP=0`, everything else as measured (`PRESENT_CAP_MS=25 DELAY_US=500 ROW_SKIP=0 STALL_MS=400`), with
`FDUMP=2` (the present/palette trace, docs/424) still armed in case the flicker survives the revert. If the
stage cards and stage entry go clean, the ramp is dead and docs/436/439 become history rather than shipped
behaviour.

## 3. If the fade is still wanted, the only defensible place is the one I avoided

The original's stage-1 fade-in is real (docs/436: 26.5 → 43.8 → 78.2 → 95.4 mean luminance in ~0.6 s), so the
gap docs/426 named is still there. The honest place to close it is `fn_040660` / `fn_0406f5` themselves: they
write the DAC 50 times with no wait, and the original evidently spent a retrace per step (~0.83 s for the
logo card's fade pair, docs/417). Making *those functions* yield one retrace per step puts the fade's clock
back on the game's timeline instead of inventing one — and it is newly plausible because `0.2.16` changed the
cost structure that ruled it out: a poll is no longer a 30 ms surface scan (docs/441), so yielding inside a
fade does not necessarily overshoot the card holds the way `DELAY_US` once did.

What has to be handled honestly, in that order: the re-entry guard (a fade runs inside one pump visit, so
presenting from it must be fenced — docs/426's hazard), the shared-path timing (it lengthens holds on the SDL
host too, docs/441's caveat), and A/B on hardware against the original's measured fades rather than against
my own taste for how long a fade should take.

## 4. Tooling note

`devini.sh` now takes trailing `KEY=value` arguments, so a card-side A/B is one command and never a hand-edit.
Its first cut was wrong twice — it consumed `$1`, which line 18 reads as the card mount, and then left an
unbalanced quote in the replacement — both caught by `bash -n` before the card was touched, which is the
cheapest place a script bug can be found.
