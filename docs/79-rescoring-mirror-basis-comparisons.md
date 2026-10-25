# 79. Re-scoring every comparison that was measured against a channel-mirrored capture

docs/70 established that some dosbox-automation payloads come out R/B exchanged, and flagged that earlier numbers were
computed on that basis. This is the audit of what was affected, with old and new numbers side by side.

| comparison | quoted in | number on the mirrored basis | number on the corrected basis | status |
|---|---|---|---|---|
| settled **stage-1 gameplay** frame, port vs original | docs/69 | NCC 0.9928, **1.26 %** of pixels within +-3, MAE 14.6 (explained away as "different game states") | **100.0000 %** within +-3, **MAE 0.17**, 65.7 % of pixels exactly equal (docs/70) | corrected in docs/69; the "different game states" explanation was wrong -- the whole difference was the channel mirror |
| **pause menu** whole frame, port vs original | docs/68 | NCC **0.87**, "the underlying gameplay differs, so the panel region must be compared separately" | **86.18 %** of the 64,000 pixels within +-3, NCC **0.860**, MAE **9.04**, residual confined to the live backdrop behind the window; the window itself is byte-verified against `pause.spr` (docs/75) | re-scored in docs/75 and labelled in docs/68 |
| **`.fli` intro** index-matched pairs (n=277) | docs/71 | median 100 % within +-3, NCC 1.0000, 170 pairs >= 99 % | **unchanged, and now proven unaffected**: re-measured from the published figures `docs/manual-playtest/08-shots/fli_idx*_port_vs_original.png` -- comparing as published gives 100.00 % within +-3 (`fli_idx0129`, `fli_idx0133`), exchanging R/B collapses it to 0.67 % / 4.65 %, and the original half is chromatic (R == B on only 0.7-4.7 % of pixels), so the match is not an achromatic accident | verified; labelled in docs/71 |
| boot-window rows for logo / intro / slides / stage card | docs/46 | NCC 0.00-0.07 | those rows were already declared non-findings there (the 60 s original capture window ended before those moments existed on the original's screen); the real logo/intro/slide comparisons are docs/68, docs/71, docs/63 | labelled, no re-score needed |
| `scratch/origcmp/refs_orig/demo.png`, `fli.png` (640x200, mode-13h-era refs) | -- | no score in docs/55..docs/78 quotes `demo.png`; `fli.png` is a single early frame ref | -- | unused refs, noted so they are not mistaken for live evidence |

## The rule that came out of it

The channel order is **a property of the capture configuration, not of the video mode**. docs/70's conclusion
("mode-13h payloads are BGRX") was established by anchoring one stage-1 gameplay capture to shipped data, and it holds
for that capture; the `.fli` capture above is also mode 13h and is *not* mirrored. So for every original-side frame:

1. anchor to something whose true colours are known (a shipped card decode, or a shipped palette entry whose hue is
   unambiguous -- docs/70 used palette index 1 = (45,24,24) 6-bit = (182,97,97) on 378 pixels), and
2. print the orientation check next to the score (`scripts/playtest_orig_title_frames.py` prints both channel orders in
   one pass; the check above is the R==B share plus the swapped score).

A score without that line beside it is not usable evidence: docs/77 is a second example of the same failure mode, where
a gain-fitted NCC of 0.99 described a picture that was not the reference at all.
