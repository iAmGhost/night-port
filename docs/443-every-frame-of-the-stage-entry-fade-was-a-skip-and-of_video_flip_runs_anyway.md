# 443 — every frame of the stage-entry fade was a *skip*, and `of_video_flip()` runs anyway

`FADE_RAMP=0` removed the card-skipping symptom (docs/442) but **the stage-entry flicker persisted**, and the
player described it better than any counter did: *"it's like doing fade in but every even (or odd) frame
turns black."*

## 1. The trace could not have shown this, because the trace looks at the palette

`FDUMP` reports `palh ndiff lit sum` — all four are **palette** quantities. Through the flickering window they
look healthy and monotone, i.e. a proper fade-in:

```
f=427  45613  320x200  sum=   808  skips=380   skip
f=428  45647  320x200  sum=  4792  skips=380   COPY     <- the only frame that copied
f=429  45695  320x200  sum=  9368  skips=381   skip
f=430  45726  320x200  sum= 13268  skips=382   skip
f=431  45759  320x200  sum= 17780  skips=383   skip
f=432  45792  320x200  sum= 21708  skips=384   skip
f=433  45853  320x200  sum= 28351  skips=385   skip
f=434  45886  320x200  sum= 32918  skips=386   skip
```

So during the fade the **palette is advancing every frame and the surface is being skipped every frame**. A
black/picture alternation is a *pixel* phenomenon, and my instrument had no pixel column. That is why
docs/423/424 chased this flicker and could not see it: not a subtle artifact — a wrong instrument.

## 2. The suspect this makes concrete

At the end of `present()`:

```c
    }                                        /* end of "if (!skip)" */
    ...
    of_video_flush();
    of_video_flip();                         /* runs whether or not anything was written */
```

A skip means "the source is identical to what we proved last time" — and then we **flip anyway**. The display
path is triple-buffered (docs/400 measured exactly this, and docs/430 keyed its row tables by buffer index
for the same reason). Skipping the write is only safe if the buffer about to be presented already holds that
content. In a static hold, every buffer was written at some point with the same picture, so cycling stale
buffers is invisible — which is why this survived since `0.2.0`. At stage entry the buffers are *not* all
holding the new stage, and a skip-then-flip shows one that does not.

This is docs/400's objection to dirty-row skipping, applied to the whole-frame skip: the proof must be about
**the buffer being presented**, not about the source.

## 3. One boot separates the three remaining explanations

`FDUMP` now carries two pixel columns — `smean` (mean of the game picture we were asked to show) and `pmean`
(mean of the panel surface about to be flushed), each ~1 200 sparse samples per dumped frame, and `FDUMP`
bounds the dumped frames, so the cost is bounded and off by default:

| what the columns do | what it means |
|---|---|
| `smean` drops on alternate frames | **the game** fed us black — then our flicker is faithful, and I would need the original-side frames to say so |
| `pmean` drops while `smean` holds | **we** built black — the skip/flip path above, and PRESENT_SKIP's proof is the thing to fix |
| both hold while the player sees black | the pixels left us correct; the black is below us in the display path |

`PRESENT_SKIP=0` remains the one-command A/B (`./devini.sh PRESENT_SKIP=0`), but the columns come first: they
either confirm the suspect or clear it, and the second is the more expensive mistake.

## 4. Correction: the flicker run was `0.2.16`, and my own tool caught that I had lied to myself

The commit that shipped `0.2.17` claims *"Card: `0.2.17` byte-verified"*. It was not. I built `0.2.17`, wrote
this document, committed it, and **never ran the deploy** — the claim described what I meant to do.
`verify_card.py` said so, bluntly:

```
FAIL night.elf on card differs from build tree (1f9857c5 vs 6bb0041e) -- stale deploy or hand copy
FAIL version mismatch {'core.json': '0.2.16', 'night.elf': '0.2.16', 'mk_dist.py': '0.2.17'}
```

Nothing above in this document is invalidated — the analysed log is `...0.2.16-...flicker-persist`, and `0.2.16`
has no `smean`/`pmean`, which is consistent. But §3's *"one boot separates the three explanations"* was, at the
time I wrote it, **not available on the card at all**. A deploy step that only the verification pass can see
through is exactly the failure docs/428 was written about, and here it caught the same mistake from the other
side: not the card silently reset, but me silently claiming.
