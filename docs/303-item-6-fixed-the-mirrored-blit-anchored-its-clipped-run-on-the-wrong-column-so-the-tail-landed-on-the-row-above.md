# 303 — item 6 root cause: the mirrored blit anchored its clipped run on the wrong column, so the tail landed on the row above — at the right screen edge

Date: 2026-11-02.  Item: playtest `docs/manual-playtest/03-2026-10-29.md` item 6 ("attacking at the far
left edge draws the leg/arm sprite on the right side") and `04-2026-11-02.md` item 5 (a sprite fragment at
the right edge).  docs/301 confirmed the artifact is real and refuted three hypotheses; this doc closes the
mechanism.

## 1. What the write-level witness showed

docs/301 armed a row-crossing detector in `fn_03bb42` (`NIGHT_BLITEDGE=1`): it reports a write whose byte
span crosses a 320-byte row boundary, and only such a write.  This turn each of the six write sites got a
`site=` label.  On `OUT=/tmp/rig/le12 NIGHT_BLITEDGE=1 route.sh scripts/rig/drives/ep1_leftedge4.txt 220`:

```
blitedge n=41500 site=5 mode=1 dest=-14 col=39 srow=97 rows=78 span=53438..53440
          col_in_row=318..0 row=166..167 player_x=15 player_y=172 player_mirror=1
```

* **~43 000 crossings** counted during the run (486 logged lines: 400 plus every 500th ✓).
* **Every single one is `site=5`** — the `mode != 0` (mirrored) branch's `col_acc + rl > col` clip.
* Each crossing is one sprite row's tail leaving the row at its left end (`col_in_row=318..0`) and landing
  on the **previous row's columns 312-319** — a vertical slab at the right edge, 78 rows tall, while the
  player stands at the room's left stand limit (`player_x=15`) facing mirrored ✓✓ exactly the reported
  symptom ✓

## 2. The shipped arithmetic (`scripts/audit/disasm.py`, no Ghidra session)

`fn_03bb42`, mirrored branch.  Locals: `[ebp-0x10]` = row cursor `db`, `[ebp-4]` = width `col`,
`[ebp-0x14]` = `left`, `[ebp-0x24]` = `col_acc`, `[ebp-0x28]` = `rl`, `[ebp-0x2c]` = skip/ovf.

| site | shipped | count | destination | port |
|---|---|---|---|---|
| plain | `0x3be1d-0x3be2c` | `rl` | `db - col_acc` | ✓ `db - col_acc` |
| width overflow | `0x3be36-0x3be5c` | `rl - ovf` | **`db - col_acc`** (`0x3be55 mov eax,[ebp-0x10]`, `0x3be58 sub eax,[ebp-0x24]`) | ✗ **`db - col + 1`** |
| left overflow | `0x3be66-0x3be8d` | `rl - skip` | `db - left` | ✓ `db - (col_acc + skip)` |

The shipped *shortens the count and keeps the same cursor*.  The forward branch does the same thing on its
side (`0x3bd13: mov eax,[ebp-0x10]; add eax,[ebp-0x24]` → `db + col_acc` ✓) and the port matches **that**
one ✓ — only the mirrored clip is wrong ✓

## 3. Why it crosses a row

`db` is built before the width is reduced: `db = base + srow*0x140 + dest_off + col0 - 1`, so `db`'s column
is `dest_off + col0 - 1`.  Then the shipped reduces the width (`0x3bd97-0x3bda0: if (dest_off < 0) col +=
dest_off`), so the row's visible coverage is exactly columns `[db - col + 1 .. db]` = `[0 ..
dest_off + col0 - 1]` ✓ and because every run satisfies `col_acc + rl ≤ col` (plain), or is shortened to
`cnt = col - col_acc` (overflow), or starts at `left` (left-overflow), **the smallest column the shipped
ever writes in a row is `db - col + 1`, which is column 0** ✓ never negative ✓

`cpy_back` (`0x040475`) writes **downward from its first argument** (`do { *a=*b; b++; a--; } while`) ✓.  The
port gave it `db - col + 1` as the start of a backward run, so the run's *tail* went to columns `-1, -2,
…` = the previous row's `319, 318, …` ✓ The witness's own span arithmetic had already encoded the direction
(`off_lo = a - cnt + 1`) — I still wrote the destination as if the copy wrote upward ✓

## 4. The trap: the identical expression is *correct* in the two fill primitives

`fn_03bebb` and `fn_03c231` contain the same-looking `db - col + 1` (port `f.c:7757`, and `fn_03c231`'s
`dest - width + 1`) and must **not** be "fixed" for consistency:

* their helper is `0x41e20`, a **forward** filler ✓;
* their unclipped sites anchor the *low* end: `0x3c1a4-0x3c1aa` → `db - rl - col_acc + 1` ✓ (`inc eax` ✓),
  and `0x3c5e8-0x3c5f1` likewise ✓;
* their overflow sites are `0x3c1d1-0x3c1d8` → `db - col + 1` ✓ and `0x3c5bc-0x3c5c2` → `db - width + 1` ✓.

So the rule is: **classify a clip by the direction the helper writes.**  A forward writer clips by anchoring
the far end; a backward writer clips by anchoring the near end.  All six sites of `fn_03bebb`/`fn_03c231`
are faithful and untouched here ✓ (docs/254/263's "don't invent conjuncts" rule has a mirror image: don't
invent consistency either ✓).

## 5. Fix and fixture

`src/f/f.c`, `fn_03bb42`, overflow site: `cpy_back(video_map(db - col_acc), data, cnt)` (was
`db - col + 1`).

New fixture in `test/test_f1.c` for the configuration the suite never had — `dest_off < 0` **and** a run
that overflows the clipped width (`dest_off=-5`, `col0=44` → width 39, cursor at column 38, `start_row=1`,
one row with runs `{30, 20}` → `ovf=11`, `cnt=9`):

```
[PASS] copy mode1 clip: dest_off<0 + run overflow writes NOTHING in the row above (docs/303)
[PASS] copy mode1 clip: dst = db - col_acc, nine bytes land on cols 0..8 of the sprite's own row
```

Red/green proved by reverting only that line: `== FAIL (2) ==` with both checks failing on the old
expression, `== ALL PASS ==` on the fixed one ✓.  The suite had been green through the whole existence of
this artifact because its mirrored cases used `dest_off = 0` ✓ (docs/263 §4 again: a green suite that never
reaches the configuration proves nothing ✓).

## 6. Rig red/green on a matched basis

Same drive (`ep1_leftedge4`, 220 s), witness armed, only the binary changed:

| measurement | `le12` (unfixed) | `le14` (fixed) |
|---|---|---|
| logged crossings (of counted) | **486** (~43 000) | **0** ✓ |
| samples with X ∈ {15,16,17} and mirror = 1 | 556 | 559 |
| samples at X = 15 | 767 | 764 |
| changed frames | 5311 / 28444 | 5260 / 28504 |
| activity-profile cells | 271 SAME | 271 SAME ✓ |
| pacing (changes/s, orig / port, ratio) | 12.62 / 22.87 = 0.55 | 12.97 / 22.08 = 0.59 |

The phase is reached identically on both builds, the crossings vanish, and **the pacing residual is
unchanged** ✓

Correction to something said earlier today: the claim that the fix cut changed frames 5311 → 898 was wrong.
That number came from `/tmp/rig/le13`, which never entered a room — player X = -30 in 975/975 samples,
mirror 0 in all of them — a void run of exactly the `le1` kind (docs/288 §1).  The churn difference measured
that run's absence, not the fix ✓

## 7. What the screen census could not settle

New instrument `scripts/rig/edge_diff.py` pairs witness frames between two dumps by `t_ms` and counts
differing pixels in a region against a mid-screen control, discarding pairs whose whole-frame difference
exceeds a threshold.  On the pair docs/301 anchored (`le12` frame 5189 t=104250 vs `le14` frame 5156
t=104183): **region 316-319 × rows 160-175 → 32/210 differing pixels, worst delta 288, control 0/210** ✓

But across ten pairs the totals are region 114/600 (19.0 %) against control 65/600 (10.8 %), because two
runs of the same drive differ across the *whole* frame by ~7 % (enemy placement and cadence, docs/289
§4).  Cross-run pixel regions are therefore directional evidence, not a gate ✓ The gate is the write-level
witness (§1) plus the fixture (§5) ✓

## 8. The `> 0x13f` guard: real difference, provably inert

docs/301 §4 recorded `fn_03bb42`/`fn_03bebb`/`fn_03c231` bailing at `dest_off > 0x13f` where the shipped
compares `cmp [ebp-0xc], 0x140 / jg` (0x3bb6a, 0x3bee3, 0x3c259).  The port now matches the shipped.  It was
worth proving the difference was behaviourally inert before touching it: for `dest_off >= 0x140` the overflow
clip gives `left = dest_off + col - 0x140 >= col`, so a run either has `col_acc <= left` (skipped) or
`col_acc > left >= col` (width-overflowed); every count comes out ≤ 0 and nothing is written either way ✓
So the change is transcription fidelity, and the existing bound fixture (`dest_off = 0x200` → no write) still
passes ✓

## 9. Standing rules this item added

* A clip's correct anchor depends on the direction the copy helper writes.  Audit sites by helper direction,
  not by how similar the source lines look.
* A witness that logs only "something crossed a boundary" is not enough: label the site, or you cannot
  attribute the write to a line of code.
* Cross-run pixel comparisons inherit every run-to-run state difference; they can support a finding but
  cannot gate one.  Write-level witnesses and fixtures can.
* Before quoting a before/after difference, verify both runs reached the same phase (player X, room, mirror).
  `le13` measured a run that never entered the room.
