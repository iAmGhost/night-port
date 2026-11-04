# 325 — Item F fixed: fn_03a5f7's per-pass VBLANK wait was modelled as a no-op, so 80 shear passes collapsed into 0.1 ms

Reported (docs/manual-playtest/06-2026-11-03.md, item F): "missing screen animation (odd/even rows of screen
crosses horizontally)", clarified as *"It's after dialogue is finished, for effect of actual stage 3 start."*
Playtester confirmation after this fix (2026-11-04): **"stage1-3 boss screen effect appears now."**
Continues docs/323 (localisation) and docs/324 (the show's key waits).

## 1. The routine, named by offsets

`fn_03a5f7` (0x0003a5f7, 78 instructions, ends `ret` at 0x3a70d).  Its **only** call site in the image is
**0x3a804**, inside `fn_03a70e` (the picture/CG show that the boss-room dialogue runs), executed once after the
last picture — i.e. exactly "after the dialogue is finished".

Outer loop `shift = 0, 4, 8, … 316` (`cmp [ebp-8], 0x140 / jl` at 0x3a6f8-0x3a6ff → 80 passes), gate
`n = 0x1ea / DAT_53104` then `cmp n, DAT_63e24 / jle 0x3a6c0` at 0x3a636-0x3a63c, and inside it a 200-row loop
(`row < 0xc8` at 0x3a651-0x3a658) split on row parity by `idiv ebx=2 / test edx,edx` at 0x3a65a-0x3a66c:

| row | offsets | meaning |
|---|---|---|
| **odd** | 0x3a66e-0x3a691 | `cnt = shift+1`; `src = base + (row+1)*0x140 - shift - 1`; `dst = 0xA0000 + row*0x140` |
| **even** | 0x3a698-0x3a6b9 | `cnt = shift+1`; `src = base + row*0x140`; `dst = 0xA0000 + (row+1)*0x140 - shift - 1` |

so each pass shears alternate rows horizontally by `shift`, and `shift` grows by 4 per pass — the reported
"odd/even rows cross horizontally".  **The port's row arithmetic is byte-faithful** (f.c's `fn_03a5f7`, verified
against 0x3a65a-0x3a6b9 term by term; `test_fn_03a5f7` T1 asserts the landed bytes).

What was missing is the pacing block **0x03a6c0-0x03a6d9**: recompute `n`, `cmp n, DAT_63e24`, `jg 0x3a6c0` —
a busy-wait that spins until the timer handler (0x3eebe, `g_dat_63e24++`) ages the counter — then
`SUB [0x63e24], n` at 0x3a6ee.  The port omitted it and said so in its own comment: "the port models it as a
no-op (it has no effect on the memory-copy sequence)".  The copy sequence is exactly what the wait *spaces
out*.

## 2. Red, on the stage-1 last-room overlay basis (`/tmp/rig/f325r`)

`--warp-stage 1 --warp-column 3` (node #4, col 3, the chain terminator) + `scripts/rig/drives/
ep1_st1_room3_walk.txt`, `NIGHT_SHOWDBG=1`, `--frames 14000 --rig-hz 1`:

```
reveal: passes=80 gate_spins=0 row_copies=16000 colAcc=320 elapsed_ms=0.1 n=7 DAT_53104=70 DAT_63e24=4294966736
```

80 passes x 200 rows = 16000 row copies, **zero paced waits**, whole reveal finished in **0.1 ms** inside one
host frame.  `DAT_63e24 = 4294966736 = 0xFFFFFFF9` (0 - 7, wrapped) is the fingerprint of a gate that never
waited.  Every pass but the last is therefore never displayed: the animation is gone ✓ exactly as reported.

## 3. Green, same drive and basis (`/tmp/rig/f325g2`)

```
reveal: passes=51 gate_spins=215695 row_copies=10200 colAcc=320 elapsed_ms=957.4 presents=80 n=7 DAT_53104=70 DAT_63e24=6
```

| | red `/tmp/rig/f325r` | green `/tmp/rig/f325g2` |
|---|---|---|
| outer passes walked | 80 | 80 (`colAcc=320`) |
| passes that drew | 80 | 51 |
| row copies emitted | 16000 | 10200 |
| paced VBLANK waits | **0** | **215695** |
| wall time of the reveal | **0.1 ms** | **957.4 ms** |
| arena presentations asked for | 0 (hook did not exist) | **80** (one per pass) |

The fix (`src/f/f.c`):

```c
sys_present_poll();                                                            /* this pass is on the CRT */
while (n > (int32_t)g_dat_63e24) { if (!sys_port_spin()) break; g_reveal_gate_spins++; }  /* 0x3a6c0-0x3a6d9 */
g_dat_63e24 = (uint32_t)((int32_t)g_dat_63e24 - n);                             /* 0x3a6ee */
```

`if (!sys_port_spin()) break` is this port's standing convention for a hardware busy-wait, so a build with a
clock paces the reveal and a test without one keeps the old collapse (that is what keeps T1's byte-exact
assertions valid).  Nothing was added to the gate: the shipped's `n > DAT_63e24` draw condition and its
`SUB` are unchanged.

**Residual to state, not hide:** 51 of 80 passes drew.  The shipped's own gate skips a pass when
`DAT_63e24 >= n`, and the port's tick pump ages `DAT_63e24` in catch-up batches rather than one tick at a
time, so 29 passes are skipped.  That is the pacing-granularity residual the rig already tracks elsewhere
(docs/244, docs/281), not a second defect in `fn_03a5f7`; the row arithmetic per drawn pass is unchanged.

## 4. Visibility half, and what the rig cannot see

The dump samples between harness frames, so a pass drawn *inside* a wait is invisible to it — a census of the
dump can prove pacing but never "the rows visibly crossed".  That is why the fix also installs a present hook
(`sys_set_present_hook` / `sys_present_poll`, `src/sys/sys.c|sys.h`, installed in `app/main.c`): on DOS the
CRT is scanning 0xA0000 while the game spins, so a pass is on screen the moment it is drawn, and the reveal
now asks for one presentation per pass (measured: `presents=80`).  The human half was closed by the
playtester: the effect appears now.

## 5. Stage-2 and stage-3 map bases: not reproducible there, and why (`/tmp/rig/x325s2`, `/tmp/rig/x325s3`)

`--warp-stage 2` and `--warp-stage 3` (no script, warp self-entry), `NIGHT_SHOWDBG=1`, `--frames 1200`:

| basis | `show: entry` | `show: pic` | `reveal:` calls | record type census | type == stage counter |
|---|---|---|---|---|---|
| stage 2 map | 0 | 0 | **0** | {0: 36, 1: 72} | **0** |
| stage 3 map | 0 | 0 | **0** | {0: 36, 1: 42} | **0** |

`fn_03a5f7` has one entry (0x3a804 inside `fn_03a70e`), and `fn_03a70e` is armed only from `fn_03af18` at
0x3aa4c-0x3aa8f, whose first load is `DAT_00053c80` — the guardian latch, set when a constructed record's type
index equals `DAT_00063370` (docs/311).  On both map-warp bases the constructed records carry type 0 or 1 and
**none carries the stage's own type**, so the latch is null and the arm cannot pass.  Item F's *reported
moment* (beat stage 2's boss, watch the transition into stage 3) is therefore **not reproducible on a
warp basis**: it needs the advance that follows the guardian dying.

**Specific run requested from the playtester:** hand playthrough — stage 1 last room (the type-1 boss with the
extra HP bar), defeat it, enter stage 2, defeat stage 2's boss, and watch the transition into stage 3 for the
row-shear effect; then the same effect at stage 3's own start.  That is the basis the rig cannot build yet, and
it is also exactly item B's runtime half (task-20).

## 6. docs/318's concession resolved

docs/318 conceded that "some routine other than `fn_03a5f7` draws it — and the rig has not identified it".  The
two families are now separated by offsets:

- **`fn_03a5f7` (0x3a5f7-0x3a70d)** — whole-screen, 80 passes, alternate-row horizontal shear, one call site
  0x3a804 (inside the dialogue show's tail).  This is the stage-start reveal.  It was missing because of the
  omitted pacing block, now paced and measured (§2-§3).
- **`fn_03ce0b` (0x3ce0b) → `blt_rows` (0x040491)** — per-*record* row blit with four arms (B0 plain,
  B1/B2/B3 offset) selected by the record's top row `+0x04`, its row count `+0x1a` and a per-row source
  address; docs/311 fixed four column/axis errors and two unsigned arm tests in it.  Its arm census
  (`NIGHT_BLITDBG=1`, docs/312) shows the offset arms running on the stage-1 chain basis (B0=576 B2=103
  B3=154) and not on the stage-2 map basis (B0=268, no offset arms) — sprite blits of an entity, not a
  screen-wide reveal.

`fn_03ce0b` was the right suspicion for "rows with a horizontal offset"; it was the wrong owner for the
stage-start effect.  Nothing else in the image writes the 0xA0000 arena with that alternating-parity shear.

## 7. Fixture

`test_fn_03a5f7` in `test/test_f1.c`:

```
[PASS]   T1 80 passes x 200 rows = 16000 row copies emitted
[PASS]   T1 with no tick source the per-pass wait takes the documented no-op exit (0 spins)
[PASS]   T3 with a tick source the reveal paces: 80 passes x n=7 = 560 pumped VBLANK waits
[PASS]   T3 the paced reveal lands the same final rows as T1
```

T3 installs a tick pump (`g_dat_63e24++`, the handler's own store at 0x3eebe) and asserts the exact wait count
— 560 = 80 passes x n(7) — and that the final landed rows still match T1's byte-exact expectations.
`make test` -> `== ALL PASS ==`, 0 FAIL.
