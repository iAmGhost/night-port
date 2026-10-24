
## Dense re-measurement closes the port-side half (this cycle)

The earlier pass sampled the startup with one or two scene shots, which left a **333-frame blind spot** across
the mode switch (frames 2671 -> 3004). The host now has `NIGHT_DENSE=1`, which keeps the periodic shot series
running during the 320x200 view at 100 ms spacing (the `dense_NN` family), so the switch is covered frame by
frame.

Over `route_levelup_warp.txt` with `NIGHT_VIDRAW=1 NIGHT_DENSE=1` (37 captured frames across frames
2641..3120, i.e. card -> stage card -> `episode_start` -> stage load -> first gameplay):

* **no frame diverges from its predecessor by more than 5 %** (worst vs-previous similarity 0.948);
* **no garbage signature**: the longest run of identical non-zero bytes stays at ~320 = exactly one 320-pixel
  row (a flat floor/wall row), never the long high-entropy regions corruption produces;
* the arena is unchanged (vs-previous 1.000) for whole stretches, matching a still card rather than churn.

So the playtest log's "corrupted screen during stage-1 startup (self-heals)" is **not reproducible in the
current build** -- it predates the `0x3DA` retrace-wait fix (docs/47) and the card/window fixes (docs/51), and
it does not reappear under dense sampling. What this measurement cannot say is whether the *original* shows
intermediate frames the port never renders; that half stays with docs/46.
