# 435 — the stage-entry stall is not a fidelity gap: the ORIGINAL holds a static card for ~20 s at that same moment

docs/434 asked a fairness question before spending an optimization turn: when the port spends ~3 s loading
stage 1 without presenting, is it doing something the original does not? Measured on the original through the
container route (docs/46's only working capture path), the answer is no.

## 1. How it was measured, and two tools that lied before they told the truth

`scripts/rig/orig_gap_timeline.py` polls `video/frame?format=raw`, hashes the reply, and reports the
distribution of inter-change intervals in the same shape the port's `gap worst=` reports. Two bugs made the
first two runs worthless, and both are the silent kind:

* **`/api/v1/api/v1/...`** — a prefix constant plus prefixed paths. curl returns an empty 404 body, which the
  sampler read as "the game produced no frames". 240 s of nothing looked like a dead game.
* **The API rejects unknown key names with a 400.** The emulator spells them `KBD_` (`KEY_DEFAULTS` in
  `playtest_orig_drive.py`); my `press("return")` was refused every time, and the game sat on the title card
  while my log said "pressed Enter #14". **An input call that 400s is indistinguishable from a game that
  ignores you**, unless the code checks the reply. It does now.

After both fixes the rig's own anchored drive (`drive_orig_episode1.txt` via
`playtest_orig_drive_container.py`, 273 captures over 115.6 s, `0168_stage_play.png` proves it reached the
stage) gave the real timeline:

```
static holds > 900 ms on the ORIGINAL:
  title_melt_02   640x480   33.9 -> 40.6    6.71 s
  slides_87       640x480   42.1 -> 62.7   20.62 s
  esc_win_02      320x200   97.8 -> 106.5   8.68 s
  down_win_01     320x200  106.8 -> 109.9   3.14 s
```

## 2. What that says

The port's worst gap is **2.98 s** (docs/433), and docs/434 established it sits exactly between
`res=1s1.pcx` (the stage-1 slide) and `res=stage1.pcx` (the stage card). At that same point in the original's
run, the original holds **one static 640x480 picture for 20.62 s** — the last slide / stage card, which the
drive file itself describes as holding until a key arrives (docs/415 measured 3.91 s for that card even with a
prompt press). Once past it, the original animates continuously: 48 captures in the 12 s after stage entry,
longest static picture 3.3 s — and that 3.3 s is the ESC menu, which is supposed to be still.

So: the port is not the one pausing at stage entry. It pauses for 3 s where the game's own presentation
pauses for seconds, on a card, by design. **Throughput work here would be optimizing the wrong thing.** The
thread is closed; the `STALL_MS` witness stays on, because it is how this got answered.

One honest limit on the claim: the drive's captures are dense inside `record` segments and sparse between
them — the window immediately before `stage_play` has one capture, so "the original had no 3 s blank right
there" is not something I measured. What I measured is that the region *contains* a 20 s static hold and that
gameplay animates cleanly after it. That is enough to stop the optimization, not enough to call the port's
load path perfect.

## 3. Where that leaves the user-visible list

| item | state |
|---|---|
| logo hold | solved — 38 s to ~6 s (PRESENT_SKIP, 0.2.0) |
| copy throughput (row skip) | measured and rejected twice (docs/432/433); `ROW_SKIP=0` |
| stage-entry 3 s stall | **not a fidelity gap** (this doc); it is 6 MB of loads at ~2.5 MB/s during a moment the original also stops |
| fade flicker | not reproduced since 0.2.3; FDUMP=2 stays armed |
| save/load depth | deferred by the user (docs/374) |
| stage-1 fade not rendered | known gap, user preference needed (docs/426) |

The remaining honest complaint is *feel*, not timing: the port's 3 s is a frozen picture while the original's
seconds are a held card. Both are still screens. Making the port redraw during a load would change nothing
about how long it takes, and would put pixels on the panel that the original does not show there — so it is
not obviously right either, and I would want the user's preference before doing it.
