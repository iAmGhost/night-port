# 195 — Hit-gate census on the canonical route: 33 contacts resolve, 352 die at the Y gate

`NIGHT_HITTRACE=1` on the port, canonical environment (`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579
NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), drive `ep1_kill3.txt`, `--frames 9000` (the same 150 s route as `g33`):

```
GATE4   352     (0x00014c68 / 0x00014c88 region: the two-tier Y test, "GATE4 y A vs B dl=")
GATE5   178     (x straddle / corner test, 0x00014006-0x00014026)
GATEOK   33     (all gates passed -> the hit is resolved by fn_01441e)
GATE2    12
GATE6     3
```

So on this route 33 contacts reach `fn_01441e`'s resolution, while 352 candidate pairs are rejected by the **vertical
overlap** test and 178 by the horizontal one.  Compare with the award-side census on the same route (docs/190 §1):
`fn_015295` entered 1 396 times, 1 108 rejected at `victim[+0x30] != 1`, 0 reached the award — and the kill-time EXP
number never drew (docs/194 §3, zero `expnum` glyphs).

That is the shape of item 2's blocker, stated with counts: contacts do happen (33), but almost none of them run the
kill through the paths that set the settle flag and reach the award, and the single largest rejection is the Y-overlap
gate.  docs/161 (camera population) and docs/162 (damage debt) are upstream of that gate; this census is the "before"
number any fix to the kill path has to move, and it is quoted with the pinned environment because the same drive run
unpinned gives different counters (docs/191 §1).
