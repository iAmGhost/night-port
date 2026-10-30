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

## 1a. The Y gate's tolerance is a constant, so the rejections are placement, not threshold

Parsing all 352 `GATE4` lines out of that run (`GATE4 y <ay> vs <by> dl=<dl>`):

```
dl = 7 in 352 of 352 lines          (abs: min 7, median 7, p90 7, max 7)
sample pairs: (172, 147) (172, 153) (172, 147) (139, 163) (139, 162) ...
```

`dl` never varies, so it is the shipped fixed allowance (the `7` the gate compares against), not a small shortfall:
the rejected pairs sit **15-24 pixels** apart vertically.  The gate is therefore behaving as written; what is wrong is
the vertical placement of the attacker relative to its target — the population/altitude question docs/161 §5 opened,
not a threshold to adjust here.

## 1b. Who is allowed to draw a number, and where (for item 2a's draw site)

`branch_xrefs.py --target 0x383a4 --target 0x38c23` gives the complete incoming edge set for the two integer-draw
routines, and every edge is modelled in the port:

```
0x383a4 <- fn_038445 at 0x3861e   -> f.c:25421  fn_0383a4(g_dat_633b0, hb+digits, 0x118, 0xae, g_dat_53100)
0x383a4 <- fn_038445 at 0x387d3   -> f.c:25467  (same geometry, second site)
0x38c23 <- fn_038f3d at 0x3983a   -> f.c:25765  fn_038c23(g_dat_633b0, num1, 0x68, 0xac, g_dat_53100)
```

The only number draw hanging off the EXP machinery is therefore `fn_038f3d`'s call at `0x3983a` — `fn_038f3d` is the
routine that folds `player[+0x52]` into the HUD field `+0x2b` (docs/189 §1) — drawn at x `0x68`, y `0xac`, 6 cols per
glyph.  Item 2a's "EXP text" must come from that site (or from `fn_038445`'s two, at y `0xae`), and its value is
`g_dat_53100`; the run that drew no `expnum` glyph at all also reported `commits=0` from `fn_038f3d`, so the next
measurement for `gate3-exp-text` is whether `0x3983a` is reached at all, not which glyph table entry is used.
