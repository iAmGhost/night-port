# 228 — Item 3 fixed: the upgrade-bar loop takes the cursor row's purchase count for all six rows

Fix task: `fix-upgrade-menu`.  Red detection was committed first (docs/206 `30b4376`, docs/207 `d5bf3d6`,
gate report docs/226 `4ea273d`).  This document also keeps the two rig-shape corrections that made the first
attempt look like a break (docs/228 §3) -- they are rig defects, not consequences of the fix.

## 1. Root cause, from the shipped bytes

`fn_038f3d` (shipped 0x00038f3d, port `f.c:25720`) is the level-up / upgrade screen.  Its upgrade-bar loop
(0x00039848-0x000399a2) reads the **row's own** purchase count into both of its locals:

```
0x39848  mov  eax, [ebp-0x54]                      ; i = 0..5
0x3985b  shl  eax, 2
0x39865  mov  eax, [eax + ebp-0x11c]               ; sel[i]   -- indexed by i, not by the cursor
0x39865  mov  [ebp-0x50], eax                      ; n     = sel[i]
0x3986e  mov  [ebp-0x4c], eax                      ; shown = sel[i]
0x3988d  mov  eax, [ebp-0x54] ; 0x39890 cmp eax, [ebp-0x60]   ; the ONLY cursor-indexed test: i == page
0x398a0  cmp  [ebp-0x50], [edx + ebp-0xec]         ; shown < selMax[i]
0x398ac  inc  [ebp-0x50]                           ; n = shown + 1, and only for i == page  (the preview)
0x398b3  cmp  [ebp-0x50], 0 ; je 0x39943           ; a row with nothing to show is not blitted at all
```

and its second block, the small marker under the first two rows, also uses the row-indexed local:

```
0x39949  cmp  [ebp-0x4c], 0 ; je 0x399a2                       ; shown == sel[i]
0x39952  mov  ebx, [ebp-0x4c] ; shl ebx, 2                     ; shown*4
0x39966  add  eax, [ebx + 0x30]                                ; glyph = RLE base + table[shown + 12]
0x39978  push [array8 + shown*4 + 0x30]                        ; rows  = rows[shown + 12]
0x3997b  ...                                                   ; col   = cols[shown + 12]
0x3998c  imul eax, [ebp-0x54], 0x19 ; lea ebx, [eax + 0x38]    ; y = i*0x19 + 0x38, x = 0x8a
```

The port wrote `int32_t n = sel[page], shown = sel[page];` -- the cursor row's count applied to every row -- so a
single purchase lit the whole menu, and rows with `sel[i] = 0` never reached the shipped's `cmp 0 / je` bail.

The rest of the loop is faithful and was **not** changed: the bar index `gbase[i] + n - 1` matches
0x000398f0-0x000398fe (`(gbase[i] + n) * 4`, then `push [array + edx - 4]`), and the marker index `shown + 12`
matches the shipped's `[... + 0x30]` dword offset (12 dwords) at 0x00039966 / 0x00039978 / 0x00039989.

## 2. The change

`port/src/f/f.c`, one line in the row loop:

```c
- int32_t n = sel[page], shown = sel[page];
+ int32_t n = sel[i], shown = sel[i];          /* 0x0003985b-0x0003986e: [i*4 + ebp-0x11c] -> both locals */
```

with the shipped sites cited in the comment.  `page` remains correct where the shipped uses it: the "+1" test
(`f.c:25906`), the buy path (`f.c:25835-25853`, `g_dat_53c94[page*5 + sel[page]]`), the cost readout
(`fn_038b30(..., cost)`), the row-number image (`rowX[page] / rowY[page] / s[page]`) and the cursor movement.

## 3. Two rig-shape corrections found on the way (both rig-side, neither caused by the change)

**3a. Log ordering: SDL_Log is stdout, `fprintf(stderr, ...)` is stderr.**  With output redirected to a file,
stdout is block-buffered, so "which came first" in a captured log is not a fact.  Re-running with
`stdbuf -o0 -e0` reordered the same run and changed the apparent story; every timing claim below is from
unbuffered runs.

**3b. The harness quits 2 500 ms after the drive ends** (`main.c:1164`:
`if (g_pt_done_ms >= 0 && pt_now_ms() - g_pt_done_ms > 2500) g_quit = 1;`).  `ep1_lumenu.txt` ends at 98.6 s of
wall time while the level-up warp fires at frame 5 600, so the menu was cut after a handful of rows -- the reason
a fixed-build run witnessed 0 menu rows in one shape and 516 in another.  New drive
`scripts/rig/drives/ep1_lumenu2.txt` keeps the cursor keys pressed to 178.6 s so the menu survives thousands of
frames.

**3c. `fn_038f3d` dereferences the player record with no guard** -- `f.c:25745`
`g_exp_committed += ((struct sa32 *)((P + 0x52)))->v` with `P = g_player_ent`.  Warping the screen before the
player entity exists segfaults there (ASan: `SEGV on unknown address 0x52 ... #0 fn_038f3d f.c:25745 #1 tick_pump
main.c`), which is what a `NIGHT_SHOWSCREEN_AT=900` run hit.  The shipped reaches this screen only from live
play, so the guard is not the port's job; the *rig* rule is that `NIGHT_SHOWSCREEN_AT` must land after the player
exists -- proven present by the drive's own `play 0x27 1` / `play 0x28 2` pokes at 94.5 / 95 s.

## 4. Red before / green after, same drive, same rig

Drive `scripts/rig/drives/ep1_lumenu2.txt`; `NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
NIGHT_SHOWSCREEN=levelup NIGHT_SHOWSCREEN_AT=5600 NIGHT_LUMENU=1 NIGHT_MENUBG=1 NIGHT_GLYPHDBG=1`,
`stdbuf -o0 -e0`, 12 000 frames, `--rig-hz 5`, dumps `/tmp/rig/lu5_before`, `/tmp/rig/lu5_after`.
"before" = `/tmp/prefix/port/test/night` (worktree at `4ea273d`, the committed unfixed source).

**State half** -- every witnessed row visit is classified; a row is "lit without its own purchase" when
`sel[i] = 0` and `n != 0` while `i != page` (a row the player never bought that still gets a bar); the
`i == page` case is the shipped's own pending-purchase preview (0x000398ac) and is legitimate:

| build | row visits | menu frames | lit rows with no own purchase (`i != page`) | cursor preview (`i == page`) |
|---|---|---|---|---|
| before | 25 350 | 4 225 | **6 824** (rows 2, 3, 4, 5 -- 1 706 each) | 1 201 |
| after | 27 564 | 4 594 | **0** | 1 310 |

Example red rows (`before`): `i=3 sel[i]=0 n=1 max=1`, `i=4 sel[i]=0 n=1 max=1`, `i=5 sel[i]=0 n=1 max=1` --
1 706 visits each, i.e. those rows were blitted on every frame of the menu although nothing was purchased there.

**Screen half** -- the actual blit destinations recorded by the `lurow` glyph witness (dest = `barX[i]`,
srow = `barY[i]`; the port's own arena, so no cross-dump basis problem, docs/202/205):

| build | `lurow` blits | destinations |
|---|---|---|
| before | 42 | `dest=122 srow=151` x24, `dest=122 srow=170` x18 -- rows 4 and 5, both unpurchased |
| after | 42 | `dest=18 srow=69` x21, `dest=18 srow=94` x21 -- rows 0 and 1, exactly the two the drive purchased (`play 0x27 1` -> `sel[0] = 1`, `play 0x28 2` -> `sel[1] = 2`) |

Cell geometry is unchanged by the fix (`col=32 rows=9` in both), so this is not a side effect of a different
sprite being drawn: the same bar cell is now drawn on the purchased rows only.  docs/207's shipped contrast (bar
rows y = 111 / 131 / 151 / 170, sprite offsets 31 / 77, ink counts 928 / 530 / 852 / 963) is the shipped-side
reference for those row positions.

docs/206's other discriminator (rows whose `n` exceeded that row's own `selMax`, 338 of 1 176 rows in that run
shape) is a consequence of the same defect and depends on where the cursor happened to sit; the
"lit without its own purchase" census is cursor-independent and is the number quoted here.

## 5. Suite, sanitizer, and docs/65 rows

* `make -C port test` with the fix: **== ALL PASS ==** (3 067 checks), rc=0.
* `night-asan` on the same drive (`/tmp/rig/lu6_asan.log`, 8 000 frames, `--rig-hz 2`): **13 482** witnessed row
  visits over **2 247** menu frames, **0** lit rows without their own purchase (424 legitimate cursor previews),
  **zero** `ERROR: AddressSanitizer`, and exactly **one** sanitizer report -- the standing UBSan misaligned store
  at `f.c:7360` (`fn_04589e`, docs/224 §6), which appears on routes that never touch this screen.
* `docs/65` rows added: "the upgrade loop's row count comes from the cursor row" -> no, row-indexed, docs/228 §1;
  "`fn_038f3d` needs a null-player guard" -> no, rig warp artifact, docs/228 §3c.
