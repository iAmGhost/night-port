# 228 — Item 3 fix attempt: the `sel[page]` index is wrong, but correcting it first exposes an earlier unwind (change reverted pending that)

Task: `fix-upgrade-menu`.  Red evidence (committed in docs/206/docs/226) and the change's effect are recorded
here; the one-line source change was **reverted** so the tree stays green until the new observation is explained.

## 1. The change and why it is the right one

`port/src/f/f.c` (row loop of `fn_038f3d`, the level-up/upgrade screen): `int32_t n = sel[page], shown = sel[page];`
was changed to `sel[i]`, matching the shipped, which reads the ROW's own purchase count into both locals
(0x0003985b-0x0003986e: `eax = i*4; eax = [eax + ebp-0x11c]; [ebp-0x50] = eax; [ebp-0x4c] = eax`) and uses the
cursor only for the "+1 while the cursor is on this row" test (0x0003988d-0x000398ac) and for the buy path.
The blit index `gbase[i] + n - 1` is faithful as-is: 0x000398f0-0x000398fe computes
`(gbase[i] + n) * 4` and then pushes `[array + edx - 4]` ✓ (so the `-1` belongs to the shipped, docs/206 §1).

## 2. Red-before census reproduced on the pre-fix binary (`/tmp/prefix` @ `4ea273d`)

`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60 NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=5600
NIGHT_LUMENU=1 NIGHT_GLYPHDBG=1`, drive `scripts/rig/drives/ep1_lumenu.txt`, 8000 frames, `--rig-hz 5`,
`/tmp/rig/lu_before.log`:

* warp at log line 412 ("showscreen warp: levelup at frame 5600"), `lumenu` witness rows at lines 422-1702
  (**1 194 rows over 196 menu frames**), 42 `lurow` glyph traces; unwinds start only at line 1703, i.e. after the
  menu finished.
* Rows 2-5 carry `sel[i] = 0` yet `n = 2` (the cursor row's count) with `max = 1..2`, so the port blits
  `gbase[i] + 2 - 1` on rows the shipped leaves dark — item 3's symptom, produced with no human input (docs/206 §3).

## 3. What the changed build did instead (the reason the change is not committed yet)

Same drive/env on the rebuilt binary (`/tmp/rig/lu_after.log`): the **unwind flood begins at log line 413, i.e.
before the warp at line 459**, and no `lumenu` row is ever witnessed (0 rows, 0 `lurow`).  The game main unwound
at frame ~5 575-5 625 in gameplay, before the level-up screen was entered, and every later pump unwound
instantly (3 891 unwinds vs 3 480 in the pre-fix run, both ending at `frames presented: 8000`).

So the row-index fix is not sufficient on its own: something upstream of the row loop depends on the value this
loop now reads, and giving it the shipped's row-indexed value changes control flow before the menu is reached.
Candidates to check next, in order:

1. `g_dat_53c94[page*5 + sel[page]]` / `selMax[page] <= sel[page]` in the buy path (`f.c:25835-25853`) and the
   per-frame re-read at `f.c:25887` — whether the shipped's *cursor* reads were also row-indexed in the port.
2. Whether `shown` feeds a state latch (the "EXP not enough" card, `help8.pcx`, `f.c:25837`) that the shipped
   drives from the cursor row instead.
3. Whether the port's snapshot of the player's selection bytes into `sel[6]` at `fn_038f3d` entry matches the
   shipped's field order (docs/206 §3 note: the injection must land while a stage is live).

## 4. ASan run limit

`night-asan` (ASan ~4x slower) reached only frame 1 534 in the 200 s cap, so it never hit the warp and cannot
attribute the unwind; it showed only the standing UBSan site `f.c:7360` (docs/224 §6).  Next attempt must either
raise the wall budget (or `--frames 2000` with `NIGHT_SHOWSCREEN_AT` scaled down) or run the ASan build with
`ASAN_OPTIONS=halt_on_error=0` plus the harness's own fault reporter to name the site.
