# 415 — the original's stage-card hold is 3.91 s of real time, so the Pocket's 7.95 s is about double

docs/413 left the question open and docs/414 said the container route could finally answer it (realtime factor
1.000) but could not reach the card. It reached it on the fourth attempt, and the number is not the one the
Pocket is using.

**Measured: `stage1.pcx` is on screen for 3.91 s before the mode leaves 640×480 and gameplay begins.**
The port does the same hold in 3.29 s on the desktop and **7.95 s on the Pocket** (docs/413, docs/414). So the
desktop is within ~0.6 s of the original and the Pocket runs roughly **2× too long** — the same direction as
docs/411's independent measurement on a different phase (8.1 s device vs 2.6 s desktop).

## The run

`scripts/rig/orig_card_hold2.py --advance-on-static 2.5 --max-presses 4`, fresh boot, 5 Hz raw-frame sampling,
`scratch/pocket/orig_hold5`. Emulation speed was calibrated first (docs/414): DOS clock advanced 20.00 s over
20.00 s host, factor 1.000, so these `t_ms` values are seconds.

```
t=   0.0 .. 23.2 s   320x200   attract
t=  23.2 s           640x480   title/copyright card
presses (Enter, on a 640x480 still >2.5 s): 26.8, 31.3, 35.3, 39.4
  change 28.57 s  -> witness_0025  title1/title3 = 0.933/0.928     (the copyright card)
  change 32.59 s  -> witness_0029  1s1 = 0.331                     (slide 1)
  change 36.61 s  -> witness_0033  stage1 = 0.440, 1s2 = 0.322     (transition)
  change 41.05 s  -> witness_0041  stage1 = 0.501, 1s2 = 0.231     (THE CARD)
t=  44.96 s           320x200   gameplay: station scene, EXP bar, platform  (witness_0045)
```

**The hold is 41.05 → 44.96 = 3.91 s**, and no key was sent after 39.36 s. The last press produced the card;
the card then held untouched and ran out on its own.

Identification is by picture, not geometry — 320x200 cannot separate gameplay from the attract menu (docs/410),
and here all four of title, slides and card are 640×480. Each witness PNG was correlated against the shipped
PCXes decoded by the game's own loader (`sweep_report.pcx_indices`) with luminance through the PCX file's own
trailer palette, 64×64 grid NCC. `title1/title3 = 0.93` for the copyright card is the control: it shows the
matcher scores ~1 when a frame really is that picture, and the card's 0.501 sits above every slide by a clear
margin. Eyeballing `witness_0041` agrees: "EPISODE-1 쏘가리..." over the villain's face, which is what
`stage1.pcx` is.

## The hold is skippable, and that measured a different number first

The third attempt (`orig_hold4`) pressed on a 4 s cadence with no cap, and reported a "hold" of **1.25 s** —
wrong, and the bytes say why. In the hold loop (`room_start`, 0x3b166..0x3b19e):

```
0003b17e  mov eax, 1 / call 0x42958      ; delay_units(1)
0003b188  cmp byte [0x13e89], 0
0003b18f  je  -> loop continue
0003b191  cmp byte [0x13e89], 0 / jne -> spin        ; wait for the key to come up
0003b19a  jmp 0x3b19e                                ; then LEAVE the hold early
```

so **any key down aborts the hold**. The cadence was cutting it short, and 1.25 s is "press-to-mode-change",
not the hold. Hence `--max-presses`: press through the press-gated slides, then hands off.

## One earlier guess of mine, corrected

The `--esc-once-on-static` option I added mid-way encodes a claim I had just made — that the card lives on the
ESC-unwind path and Enter-pressing through the show skips it. `orig_hold5` disproves that: Enter presses reached
the card normally. Its help text in the script now says so instead of repeating me. (`orig_hold6`, the ESC
attempt, is a null run for a different reason: with no key, the game's 640×480 title card just waits — 44.7 s
static, `witness` after 48 s unchanged — and my static-detection fired there instead of on a slide, so the ESC
was delivered to the title card and did nothing.)

## What this means, and what it doesn't

- The port's clock model for `delay_units` is not wildly wrong: 3.07 s predicted, 3.91 s observed including the
  two 50-step palette fades and the `INT 10h` mode set that sit inside the same window. The bracket is card
  drawn → mode leaves 640×480, which is generous, so the pure `delay_units` part is somewhat under 3.91 s.
- The Pocket's 7.95 s is the outlier. The lever is per-DOS-clock-read cost on the device (docs/411 already
  measured the same ~2-3× ratio on a different phase), **not** the `CDDA_CARD_STOP` rule — that rule is about
  *whether* it goes quiet, this is about how long it stays quiet.
- Single sample, one episode. The original's card hold may vary by stage (the `0xc00` count is a constant in the
  code, so it probably does not), and the attract loop's own 44-50 s title holds are a separate thing.
- Not measured: the original's audio during the hold. What is established here is the *picture's* duration.
  That the original is silent across it is still the player's ear (docs/413), though the two are now
  conveniently measurable with the same rig.
