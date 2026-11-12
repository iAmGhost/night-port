# 461 — The d-pad kept walking after the finger left, so the page now takes the next press as the release

A report from the phone build: **"sometimes dpad got stuck. please make it unstuck when pressing again
(dpad should only allow 1 point touch so it will be not that hard)"**. The player had already diagnosed the
fix and the constraint it needs, and both turned out to be right.

## What "stuck" is, in this code

The deck does not push keys at the game when you press. It keeps three pieces of state — `padPtr` (what each
live pointer holds), `padPad` (the pointers that went down on the cross) and `padSent` (the union, i.e. what
the game was last told) — and pushes only the *edges* of that union (`padApply`). So the game's idea of the
world is exactly as current as the page's list of live pointers, and **a direction can outlive its finger
without any key ever being stuck in a queue**: the make is in the guest, and nothing will send the break.

The page trusts three events to end a press — `pointerup`, `pointercancel`, `lostpointercapture` — and a
document-level `pointercancel` as a net. That is a reasonable list and it is not a guarantee. iOS does not
have to deliver `pointerup` for a gesture it decided to own; the `pointercancel` net only fires when the
browser *cancels*, which is not the same as it forgetting; and `lostpointercapture`, which the comment in
`shell.js` used to call a safety net ("on a real device the very next event ... called this again and cleared
it"), only follows a release that happened. Nothing here says the browser dropped an event — only that the
page's only route back from "still held" is an event, and a second finger on the same element makes that route
longer.

That is also why the stuck state arrives in bursts and why a player who presses again is fine: the *next*
`pointerdown` is a fresh pointer id, and a fresh id has no history. The reported fix is the behaviour to
implement, and the constraint ("the cross is one finger") is what makes it cheap.

**The drop itself was not reproduced.** Chrome does not lose `pointerup`, and there is no device here. So the
fix does not chase the cause: it makes the state that the cause produces **impossible to inherit**, by
defining what a press means instead of hoping for the end of the last one.

## The three rules, and the one they deliberately do not include

1. **The cross answers to one finger, and a press starts clean.** A `pointerdown` on the cross ends every
   other pointer that went down on the cross, *before* it reads the new press (`padEndOthers`). The stuck
   direction is therefore released by the first thing the player does about it, which is pressing again.
   Nothing waits for a timer, and nothing is guessed: an old press is replaced, not merged.
2. **A pointer the browser says is gone is gone.** `document.pointerIdList` (Chrome 123, Safari 17) is the
   browser's own answer to "which fingers are on the glass", so any press first sweeps the pointers the page
   still holds that are not in that list (`padSweep`). This one heals a direction with *no* new press of the
   same kind — a thumb that lifts and then punches is enough — and where the property is missing it does
   nothing, leaving rule 1 to carry the cross.
3. **A verb pressed again was released somewhere the page never heard about.** On a deck button, the same
   scancode held by a different finger is that lost release, so the old finger is ended. This is the same
   heal applied to the buttons, because a punch that will not let go is worse than a stuck direction: the
   game keeps applying the modifier to every move.

Rule deliberately *not* included: releasing the whole deck on any press. `docs/457` describes the deck as
"one thumb and a finger", and walking while holding punch is one of its ordinary verbs — the union across
pointers exists precisely for that. A directional press that silently dropped the punch the other thumb was
holding would be the quieter of the two bugs, and the player would have no idea which press caused it. So the
heal is scoped to the cross (rule 1), to pointers the browser disowns (rule 2), and to the same verb (rule 3).

The controller's share of `padPtr` (`'gp0'`) is out of all three by type: its state is re-read every poll, so
it cannot drift, and a stick you are leaning on is nobody to release.

## The measurement

`NIGHT_TOUCH` in the harness drives the deck and asserts what it believes (see `wasm/scripts/headless.js`).
The interesting part is the `c` token, which replaces the page's `document.pointerIdList` with the ids the
driver is holding: without it, a synthetic finger is invisible to the browser and rule 2 does the healing, so
the test would pass for the wrong reason. With it, the page is told *"the browser says both fingers are still
down"*, which is exactly the state rule 2 cannot reach and the report describes:

```
touch: the driver now claims its fingers are on the glass
touch: finger 11 down at left (44,483)
touch: held [left], cross owner 11, pointers behind it [11] as expected
touch: finger 12 down at KeyZ
touch: buttons [KeyZ], dirs [left] as expected
touch: finger 13 down at right (163,483)
touch: held [right], cross owner 13, pointers behind it [12,13] as expected
touch: finger 14 down at KeyZ
touch: buttons [KeyZ], dirs [right] as expected
touch: every finger lifted
touch: buttons [nothing], dirs [nothing] as expected
touch: held [nothing], cross owner null, pointers behind it [] as expected
CHECK: PASS (730 frames, 48.7 fps, 18 keys, 3 CD tracks played)
```

Read line 6: finger 11 pressed left and **never lifted**, finger 13 then pressed right — and the deck holds
`[right]` alone. `pointers behind it [12,13]` is the same line proving the punch finger (12) was not touched:
the cross gave up 11, the buttons kept 12. Line 8 is rule 3: punch pressed again by finger 14 while finger 12
still holds it, and the deck ends 12 rather than holding the verb twice, with `dirs [right]` still intact.

The same script runs under the network emulation of docs/460 (`NIGHT_NET='latency=250 offline=8000-16000'`,
37.6 fps, PASS, no read failures) — a d-pad that heals is not a d-pad that costs frames.

`NightWasm.padState()` is new, and is the reason the line above can say *which* rule let a direction through:
`held` is what the game was last told, `dirs`/`keys` the same split by verb, `owner` the finger the cross
answers to, `ptrs` the pointers behind the state. A direction in `held` with no id in `ptrs` **is** the bug.

## What this does not claim

- **It does not claim iOS drops `pointerup`.** It claims the page has no route back from "still held" except an
  event, and removes the inheritance of that state. The cause is still the browser's.
- **Rule 2 covers iOS 17 and up only.** `document.pointerIdList` is not in iOS 16; there, only the press rules
  heal, which is the case the player asked for and no more.
- **A direction held by a press you never release is still held.** That is the design: the cross is a held
  state, and the first press of anything else on the cross ends it.
- **The harness does not touch the glass.** The driver dispatches the `pointerdown`/`pointerup`/
  `lostpointercapture` the deck listens to, from the page. Chrome's real touch pipeline
  (`Input.dispatchTouchEvent`) was tried first and hung the run while the disc was streaming — it drags in
  viewport emulation for nothing, because the thing under test is the deck's own bookkeeping.
