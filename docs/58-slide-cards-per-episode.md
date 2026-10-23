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
