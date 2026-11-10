# 448 — fade speed, measured on the panel's own palette path, with the key gate excluded

docs/447 §6 established *why* the old hold numbers were worthless for this question: `1sN` cards are gated on
`fn_00035dfc`, a press-and-release wait that exists in the original too, so a card's wall-clock length is
whenever the human pressed. What the game actually decides is two things per card: how fast it fades in, and
how fast it fades out. That is `0.2.22`'s `FADELOG`.

## 1. Measured where the fade actually is

`fn_040660(target, 0x19)` — the fade-in — is:

```c
for (i = 0; i < arg2; i++) {              /* arg2 = 0x19 = 25 */
    for (j = 0; j < 0x300; j++) local[j] = arg1[j] * i / arg2;
    fn_00035ec2();                        /* one retrace  */
    board_out(0, 0x100, local);           /* DAC upload   */
}
```

The palette table *is* the fade, so the timing is taken on the table the panel is handed, in `palette_push()`
— not inside `fn_040660` / `fn_0406f5`. Deliberately: those are shared `src/f` code that the SDL host and the
3 091 fixtures also run, and putting a witness on that path puts a witness in the measurement basis
(docs/411: a diagnostic can be the pathology). A ramp is consecutive luminance deltas over 300; **the flat
stretch after a ramp is the key gate, and it is not counted** — which is exactly the exclusion asked for.

One line per completed ramp, 24 per boot:

```
fade IN :    612 ms over 25 step(s) | lum      0 -> 195450 | res=1s2.pcx seq=180
```

`FADELOG` defaults to `0` in the release ini, is a required key in `mk_dist.py` (so it can't silently go
missing), and when off the cost is a function call and a branch.

## 2. What it will be compared against

The original's measured fades, not my impressions of them: logo-card fade pair 0.83 s (docs/417), stage-entry
fade-in ~0.6 s from the luminance ramp (docs/436). Those are 640x480 slides; the `1sN` cards get their own
original-side capture so the comparison is card-to-card rather than card-to-logo.
