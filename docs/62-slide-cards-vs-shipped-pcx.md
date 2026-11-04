# 62 — Proving the intro slideshows display the shipped cards

Scope: the `slideshows` playtest task ("per-episode introduction slideshows skip straight to gameplay").
The skip itself was root-caused and fixed earlier (the `0x3DA` retrace wait stub, docs/47, commit `a3fd2e6`);
this document is the *proof* half: that each episode's slide sequence is actually shown, and that what is
shown is the shipped card.

## Why the obvious screenshot comparison was the wrong instrument

The first attempts compared a screenshot taken when the card was *announced* — and that frame is the
previous screen, because the announce happens inside the loader, before the blit. Measured on episode 1: the
`<stage>s<N>.pcx` lookups land at frames 2339 / 2378 / 2418, i.e. ~39 frames (0.7 s) apart, and the announce
shots show what was up before the load.

Two harness additions fix the sampling:

- `card_shots_poll()` (`src/app/main.c`): a 200 ms series of shots named `card_NN` while the view is
  640x480. It is called both at the frame boundary and from the spin point, because the slide show runs
  inside one host step (the driver `0x0003b0ba-0x0003b116` never returns to the frame pump).
- `NIGHT_VIDRAW=1`: next to every BMP, dump the **screen arena bytes** behind it (`g_video_ram`, plus a
  `#base_delta` line and the 768-byte DAC table the host used).

The comparison that matters is then arena-vs-card, which is independent of how the palette happened to be
ramping at the sample instant (`fn_040660(g_dat_636b8, 0x19)` fades the card in over 0x19 steps, so RGB
comparison against the card's trailer palette is meaningless mid-fade) and independent of BMP encoding
details. An independent decoder for the cards was written in `scripts/playtest_compare_slides.py`, mirroring
the game's own loader (`fn_036190` / `fn_03628e`): data after the 128-byte header, PCX RLE where a byte with
bits 6-7 set is a fill of `byte & 0x3f` copies, destination size from the header bounding box.

Tooling traps found while doing this, recorded because they produce confident nonsense if missed:

- The card geometry is **not** constant: `levelup2.pcx` is 320x200 (0xfa00, `fn_036190`) while the slide
  cards are 640x480 (0x4b000, `fn_03628e`). Reading the header as fixed-size makes a decoder overrun.
- PCX header fields are at their standard offsets (`planes` at 64, `bits/pixel/plane` at 65, scanline at 68) —
  an offset slip silently yields a "sheared" image.
- Host BMPs are 32-bit, **bottom-up**, and stored **B,G,R**; the row slice must be `w * bpp / 8`, not
  `w * bpp`. Each of those three mistakes was made here and each shows up as a plausible-looking but wrong
  diff, so the arena comparison is also the defensive choice.

## Measured result

`./build/night --script scripts/playtest/route_show_keyed.txt --warp-stage N --shots DIR --no-audio --frames 9000`
with `NIGHT_VIDRAW=1`, then `python3 scripts/playtest_compare_slides.py DIR N`:

| episode | intro slides shown | each card vs independent decode | ending cards shipped | ending cards reached |
|---|---|---|---|---|
| 1 | `1s1 1s2 1s3` | 307200/307200 bytes identical, each | `1e1` | no |
| 2 | `2s1 2s2` | 307200/307200, each | `2e1` + `2end0001..2end0010` | no |
| 3 | `3s1 .. 3s6` | 307200/307200, each | `3e1 3e2` | no |
| 4 | none shipped | n/a | none | n/a |
| 5 | none shipped | n/a | `5e1 .. 5e4` | no |
| 6 | `6s1 6s2 6s3` | 307200/307200, each | `6e1 .. 6e3` | no |
| 7 | none shipped | n/a | `7e1` | no |
| 8 | `8s1` | 307200/307200 | `8e1` | no |
| 9 | `9s1 9s2 9s3` | 307200/307200, each | `9e1 .. 9e3` | no |

So every episode that ships intro slides shows all of them, in order, before gameplay (`playing` is reached
after the sequence in every run), and the bytes on screen are exactly the shipped card. The per-episode
counts agree with the inventory in docs/58 (episode 4 ships none, so "every episode has a slideshow" is
false, and the port does not invent one there).

A rendered frame was also read visually as a cross-check: the stage-1 slide at frame 2364 shows the card
(the Korean narration block over the train-carriage artwork) with the fade still ramping.

## New asset family recorded

`2end0001.pcx` .. `2end0010.pcx` (10 files, 640x480) exist only for episode 2 and are a second, longer
ending sequence distinct from the single `<stage>e<N>.pcx` cards: 16 `e`-cards across the episodes
(`1e1, 2e1, 3e1-2, 5e1-4, 6e1-3, 7e1, 8e1, 9e1-3`) versus these 10. No route reaches an ending yet, so the
`e` and `end` families are inventoried but unverified; reaching them is stage-completion work, not loader
work.

## Regression coverage

- `test_vga_retrace_wait` (docs/47) pins the timing model whose absence caused the slides to be skipped.
- `test_slide_name_sequence` (added with this document) pins the slide naming path itself:
  `fn_0428f8` with `"%ds%d.pcx"` and the pushed pair `(stage, index)` must yield `1s1.pcx` / `3s6.pcx`, and
  the post-slide card must be `stage9.pcx`. A mis-ordered vararg cursor renames every card to a nonexistent
  file, which in a playtest looks exactly like "the slideshow was skipped".
- `test_...` at test_f1.c:13732 pins that `fn_03628e` lands the whole 0x4b000 RLE screen in order with the
  window sliding by 0x10000 per chunk (docs/51) — the loader the slide cards use.

Suite after this change: **ALL PASS** (2853 checks).
