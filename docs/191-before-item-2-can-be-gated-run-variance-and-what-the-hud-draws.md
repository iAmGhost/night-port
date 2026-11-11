# 191 — Two things that must be settled before item 2 can be gated: run-to-run variance, and what the HUD actually draws

## 1. The port's census is not reproducible ad hoc (blocks item 2's before/after evidence)

`fn_015295`'s gate census (docs/190 §1) was taken from ad-hoc invocations of the port binary with the canonical
environment (`SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_ACTTRACE=1`,
drive `ep1_kill3.txt`, `--frames 9000`, `--rig-hz 10`).  Four such runs of the **same binary, same drive, same
environment** produced:

| run | matcher visits | slot consumed | element-event sounds | `fn_015295` `+0x30!=1` rejections | reached award |
|---|---|---|---|---|---|
| `det1` | 3 917 | 4 274 | 34 | 1 435 | 0 |
| `det2` | 4 993 | 7 438 | 29 | **0** | 0 |
| `hz3` | 4 597 | 7 300 | 22 | 0 | 0 |
| `hz4` | 5 218 | 7 425 | 35 | 958 | 0 |
| `ch5` (earlier) | — | — | — | 1 108 | 0 |

`adds=0 added=0` held in every pinned run, but *why* it holds is not stable: the number of attempts that die at the
dying-flag gate ranges from 0 to 1 435.  A before/after diff for an EXP fix therefore cannot be quoted from this
harness.

What is and is not established by the dumps themselves: `cells.bin` from `det1` and `det2` differ in every sample,
but the first differing cell is `DS 0x533a0` with values `1 796 197 595` vs `1 802 144 987` — host heap pointers,
which are expected to move (docs/181 §4's "the port's copy holds a heap offset" family).  So pointer-valued cells are
not evidence of divergence, while the *counters* are, and the two facts have to be separated before the residual set
(docs/182's 25 rows) is quoted as if pointer cells were comparable.

Consequence, and the discipline it imposes: item 2's red-before/green-after evidence must come from `route.sh`
(both sides, one command, the pinned env it applies) and be repeated at least twice, not from single ad-hoc runs.
Until the variance is attributed (drive `wait ds` predicates reading live cells, or a genuine timing coupling in the
frame loop), award-path counts are a measurement I may report but not use as a gate.

## 2. What the HUD field actually renders on the unfixed build

Cropping the port's own frame stream (`g33`, frame `003047_320x200.bin`, region `x 55..150, y 10..40`, 4x nearest)
gives the HUD strip as the port draws it:

```
P. 00000
```

i.e. the 5-glyph field `fn_0137c1` renders *is* present and legible, and it holds zeros — consistent with §1
(`adds=0`, so `+0x52` never feeds `+0x2b`) and with the display-fold counters `commits=0 committed=0`.

The pieces the playtest's "garbled EXP text" can be attached to, all already in the port:

* `exp.spr` is loaded as a sprite block into `g_dat_63e04`, and `fn_0137c1` reads its font from
  `char_table = [g_dat_63e04 + 0x34]`, `rle_base = [g_dat_63e04 + 0x38]` (heap offsets) — `f.c:2824`, `f.c:13979-80`.
* `exptab.dat` is parsed into `g_dat_53c94[30]` ("6 rows of 5 numbers, row stride 0x14", `f.c:2664`, `f.c:2845`).
* The digit selection is `char_table[d - 0x2f]` for the character `d`, and the pad glyph is `char_table[1]` — the
  port's own comment records that this makes **the digit `0` and the blank pad read the same font element** ("original
  quirk, kept", `f.c:13972-73`, verified against guest `0x00013833`).  If the shipped table is indexed differently
  for the *label* part of the strip (the `EXP` letters the playtest sees), that is where a garble would come from, and
  it is checkable against `exp.spr`'s bytes without any runtime.

The shipped side has no `EXP` ASCII anywhere in `NIGHT.EXE` (a search for the three bytes `45 58 50` over the whole
image returns nothing), so the label is carried by `exp.spr`/`exptab.dat`, not by the executable — which means item
2a's localization is a **data-and-index** question (which glyph entry the port selects for the label and each digit),
and the screen side is only the confirmation.  `NIGHT_GLYPHDBG` already exists at both draw loops (`GLYPH_TRACE
("expnum", ...)`, `f.c:13986` and `f.c:13994`), so the witness half of the contract is ready to use.

## 3. Order of work this implies

1. Reproduce the census through `route.sh` twice and attribute the variance (drive predicates vs frame-loop timing).
   Without this, item 2b's fix evidence is not admissible.
2. Byte-compare the port's `exp.spr`/`exptab.dat` load against the shipped files (`exptab.dat` is an entry of
   `NIGHT.RES`, docs/02; `exp.spr` is a disc resource) and re-derive the label/digit index arithmetic from guest `0x000137c1-0x000138ba`.
3. Then, and only then, use the HUD crop above as the "before" picture for item 2a.
