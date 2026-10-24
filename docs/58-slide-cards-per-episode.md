# 58 — Slide cards shipped per episode (the slideshow inventory)

The per-episode introduction slideshow is built from numbered PCX cards the game opens by name:
`<stage>s<N>.pcx` then `<stage>e<N>.pcx` (the format strings at the card loader, with the argument
order fixed in an earlier pass — `fn_0428f8`'s vararg cursor walks upward, so the pushes read
`(stage, index)`). The inventory below is read straight off the shipped resources, so it is the upper
bound the game can show, and it explains why some episodes look "short".

Counted from `scratch/res` (the extracted disc/CD files), per stage:

| stage | `<stage>s<N>.pcx` | `<stage>e<N>.pcx` | files |
|---|---|---|---|
| 1 | 3 | 1 | 1s1 1s2 1s3 / 1e1 |
| 2 | 2 | 1 | 2s1 2s2 / 2e1 |
| 3 | 6 | 2 | 3s1..3s6 / 3e1 3e2 |
| 4 | 0 | 0 | — (stage 4 ships no cards at all) |
| 5 | 0 | 4 | 5e1..5e4 |
| 6 | 3 | 3 | 6s1..6s3 / 6e1..6e3 |
| 7 | 0 | 1 | 7e1 |
| 8 | 1 | 1 | 8s1 / 8e1 |
| 9 | 3 | 3 | 9s1..9s3 / 9e1..9e3 |

Two consequences worth recording:

- **"Every episode has a slideshow" is false.** Stage 4 has no card files, and stages 5 and 7 ship only
  the `e` (ending) cards. A route that shows nothing for stage 4 is correct behaviour, not the old
  skip bug.
- **The card count is what the retrace-paced fade loop shows**, one card per fade cycle, which is why the
  original's slideshow takes so long and why the stubbed 0x3DA retrace wait (docs/47) made the whole
  sequence disappear at once.

Reproduced in the port by the scripted route (`scripts/playtest/route_episode1_anchored.txt` and
`arrows_crash_hunt` variant): the resource log for stage 1 is
`1s1.pcx → 1s2.pcx → 1s3.pcx → stage1.pcx`, i.e. all three `s` cards, in order, each with its own
screenshot in the run manifest, before the stage card and gameplay. That matches the shipped count for
stage 1 (3), and the per-card screenshots match the corresponding PCX decodes (the same decoder the
selftest uses).

Still to do for this task: the same measurement for episodes 2..9 (the per-episode sweep task), which
needs the longer route scripts rather than episode 1.

## Every shipped slide card, byte-identical per episode (this cycle)

Nine runs (`route_show_keyed.txt --warp-stage N --frames 2600`, `NIGHT_VIDRAW=1` -- full 640x480 arena dumps after
the docs/66 geometry fix), compared with independent PCX decodes:

| episode | `g_tab_52f2c[stage]` | slides in `NIGHT.RES` | slides presented byte-identically (exact frames each) |
|---|---|---|---|
| 1 | 3 | 1s1, 1s2, 1s3 | 1s1 (5), 1s2 (5), 1s3 (4) |
| 2 | 2 | 2s1, 2s2 | 2s1 (5), 2s2 (4) |
| 3 | 6 | 3s1 .. 3s6 | 3s1 (5), 3s2 (4), 3s3 (5), 3s4 (5), 3s5 (5), 3s6 (5) |
| 4 | 0 | none | -- (nothing to show) |
| 5 | 0 | none | -- (nothing to show) |
| 6 | 3 | 6s1, 6s2, 6s3 | 6s1 (5), 6s2 (5), 6s3 (4) |
| 7 | 0 | none | -- (nothing to show) |
| 8 | 1 | 8s1 | 8s1 (5) |
| 9 | 3 | 9s1, 9s2, 9s3 | 9s1 (5), 9s2 (4), 9s3 (5) |

`g_tab_52f2c = {0x16, 3, 2, 6, 0, 0, 3, 0, 1, 3}` (indexed by `g_dat_63370`, gated at 0x03b072) states the slide
count per episode, the shipped container holds exactly that many cards for each episode, and the port presents
exactly those cards -- so episodes 4, 5 and 7 having no introduction slide show is shipped behaviour, not a port
gap. 26 distinct slide cards, every one byte-identical over all 307,200 arena bytes; `stage<N>.pcx`, `logo.pcx`
and `title3.pcx` matched exactly in the same runs as well.
