# 197 — Player-record fields read on both sides: the accumulator diverges 114 vs 11 phases; the shipped block goes stale at P167

`scripts/rig/ent_fields.py` reads named fields out of the record snapshot attached to a pointer cell on both sides.
Run: canonical route (`/tmp/rig/g33`, drive `ep1_kill3.txt`, 413 shipped states, 1 500 port samples, 274 drive phases),
cell `g_player_ent` = DS `0x53c7c`, record snapshot 352 B on both sides.

## 0. Which "EXP text" item 2a means (reporter's clarification, recorded here first)

The playtest report means **the EXP text that appears when an enemy dies** — not the number field drawn on the
HUD.  Everything in §1 below (the `fn_0137c1` HUD field, docs/191 §2's crop of `P. 00000`) is therefore a
measurement of a *different* field, kept only as a control.  The kill-time number is drawn by `fn_038c23`
(only caller: `fn_038f3d` at `0x3983a`, x `0x68`, y `0xac`) or `fn_038445`'s two calls to `fn_0383a4`
(x `0x118`, y `0xae`), value `g_dat_53100` (docs/195 §1b, docs/196).  docs/182's row 2a now says this.

## 1. What the port's HUD field actually holds (item 2a, port side)

A witness in `fn_0137c1` (`NIGHT_EXPVIEW=1`, `f.c` after the `pad = 5 - len` line) logs the value it is about to draw:

```
draws logged: 400   display=0 pending=0 len=1 pad=4   (every single line, first to last)
max |display| = 0   max |pending| = 0
```

So the port draws the five-glyph field as `pad 4 blanks + one '0'` — a permanently-zero field — and the value fed into
the digit routine never moves.  Combined with docs/196 (neither `fn_0383a4` nor `fn_038c23` is ever called), the port
has **no live EXP number** on this route: the draw runs, the value is always 0.

## 2. The same fields read out of both games' player record

```
field    shipped nonzero phases      port nonzero phases
+0x2b    107  (first at P167)        0
+0x52    107  (first at P167)        0
+0x56    114  (first at P2)           11  (first at P1)
+0x30    115  (first at P159)         14  (first at P108)
+0x17    267  (first at P7)          268  (first at P6)
```

## 3. The caveat that decides how much of §2 is usable

Read phase by phase, the shipped column stops changing at **P167** and stays at `+0x2b = 0x1bfaaaa`, `+0x52 = 0x1bfbf`,
`+0x30 = 7`, `+0x17 = 5` for all 107 remaining phases — a frozen block, not a live record (docs/177's route-end
artifact family; the drive's last phase is a stage clear).  Therefore:

- the `+0x2b` / `+0x52` "shipped nonzero" counts are **not** an EXP award — in the live window (P0-P166) **both sides
  read 0 in both fields**, so this route shows no EXP display on either side and cannot gate item 2a or 2b;
- the usable divergences in the live window are `+0x56` (the accumulator: shipped live in most phases, port live in
  11) and the state field `+0x17`.

## 4. The state-field census confirms docs/188 §2 at record level

```
shipped +0x17: 5x108(frozen) 1x52  15x41  2x22  33x13  34x9  0x7  35x7
port    +0x17: 1x135  33x64  2x35  15x19  c x13  0x6   b x2
```

Shipped's player record passes through state ids `0x34` and `0x35` (and `0x36` appears in the docs/188 dumps); the
port's reaches `0x33` and never `0x34`/`0x35`.  That is the same divergence as DS `0x62d58`'s census
(shipped 0x33/0x34/0x35/0x36 vs port 0x33/0x0, docs/188 §2) now observed **inside the player record's own state
field**, i.e. the level-chain arms the port computes (docs/188 §4) never become the record's state.

## 5. Consequence for the gates

`gate3-exp-values` and `gate3-exp-text` need a route whose live window contains a real award: the accumulator `+0x56`
is the field that moves on the shipped side (114 phases) and barely moves on the port side (11), so the drive to look
for is one that keeps a fight live long enough for `+0x56` to fill and hand off to `+0x52`/`+0x2b` before the route
ends.  `ep1_kill3` ends (stage clear at P167+) before that hand-off happens on either side.
