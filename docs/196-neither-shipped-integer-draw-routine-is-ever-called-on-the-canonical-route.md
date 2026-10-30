# 196 — `fn_0383a4` and `fn_038c23` are never called: the port draws no number on the canonical route

docs/195 §1b pinned the only three shipped call sites that draw an integer.  Counters were put inside both draw
routines (`g_numdraw_38c23`, `g_numdraw_383a4`, `f.c` next to their definitions; printed by the act-census block) and
the canonical route was run (`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
NIGHT_ACTTRACE=1`, drive `ep1_kill3.txt`, 9 000 frames = 150 s, binary rebuilt through `make -C port test`, suite ALL
PASS):

```
integer-draw calls (docs/195 §1b): fn_038c23=0  fn_0383a4=0
exp award path: adds=0 added=0 commits=0 committed=0
```

**Zero** invocations of either routine over the whole 150 s.  `fn_038c23` is reached only from `fn_038f3d` at
`0x3983a`, and `fn_038f3d`'s own EXP fold reports `commits=0`, so neither the fold nor the draw it contains runs.
`fn_0383a4` is reached only from `fn_038445` at `0x3861e`/`0x387d3` (the counter displays), which also never runs.

What that settles for item 2a: the divergence the playtest sees cannot be a wrong glyph or a wrong advance inside
these routines — the routines are not entered.  It is an **invocation** divergence, one level above everything
docs/192/194 examined, and it sits on the same unfinished kill path as item 2b (docs/195: 33 contacts resolve, 352
die at the Y gate with the shipped constant allowance; docs/190: 1 108 of 1 396 `fn_015295` attempts die at
`victim[+0x30] != 1`).

What it does not settle: the port does draw *something* number-shaped — the HUD field rendered by `fn_0137c1` shows
`P. 00000` (docs/191 §2), and that is a different routine with its own glyph quirk.  So the remaining question for
item 2a is which shipped draw the playtest's kill-time "EXP" text corresponds to among the sites that *do* run in
the port, and whether the port routes it through `fn_0137c1`'s field (index `-0xbc`, where digit `0` aliases the
blank glyph) instead of `fn_038c23` (index `-0xc0`, where `0` is glyph 0).  That is a draw-site attribution question,
and the next measurement is a screen-side one: crop the original's frames at the moment the shipped game shows the
kill EXP text (`x 0x68, y 0xac` region for `fn_038c23`, `x 0x41, y 0x14` for `fn_0137c1`) and see which region
changes, then check the same region on the port.

## 2. The other candidate for the kill-time text: the record-driven glyph blit

`fn_03df96` (guest `0x3df96`, port `f.c:14049`) is the remaining routine that can put text-shaped glyphs on screen: it
walks the `g_dat_62a90` records (14-byte stride at `g_dat_5d0b4`), and for each record blits glyph `rec[0]` with
`fn_03bb42` at `dest_off = rec[4] - font->x_tbl[g] - g_dat_639dc + rnd(rec[0x10])`,
`start_row = rec[8] - font->y_tbl[g] - g_dat_639e0`, `col = font->col_tbl[g]`, `rows = font->rowcount_tbl[g]`,
`mode = rec[0xc]`, font = global `g_dat_63dfc`.  Its complete incoming edge set is three calls, all modelled:

```
fn_03801b @0x3804e  -> f.c:15118
fn_038098 @0x38107  -> f.c:15335
fn_0381d7 @0x382e1  -> f.c:15413
```

i.e. it is part of the per-frame present/composite chain rather than the kill path, so a kill-time EXP string would
have to be *posted* as one of these records by the kill code.  That is the concrete thing to look for next: a writer
into the `g_dat_62a90` record block (or the `g_dat_5d0b4` stride area) at the moment of a kill — a write census over
`field_writers.py` for the records' displacements, then a check of whether the port performs the same store.  If the
port never posts the record, item 2a's garble is the *absence or partial fill* of that record, and the drawn glyphs
come from a stale record (which is exactly what a "garbled" string looks like).
