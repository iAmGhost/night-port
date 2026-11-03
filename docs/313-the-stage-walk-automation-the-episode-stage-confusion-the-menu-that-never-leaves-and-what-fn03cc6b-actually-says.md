# 313. The stage-walk automation: the episode/stage confusion, the level-up screen that never leaves, and what `fn_03cc6b` actually says

Goal task-8 (round-4 item D, "stage 2's vertical camera must follow the player").  Two playtester
corrections landed on the way there, and both changed what the rig is allowed to claim.

## 1. The two corrections, and what they invalidated

1. **"EP1-STG2, not EP2."**  `--warp-episode N` overwrites `DAT_00063370` (`port/src/f/f.c:5362`), which is the
   counter the loader builds every per-stage resource name from (`stage%d.wal` at 0x37b61-71, `stage%d.stg` at
   0x37c58-68, `st%ds.spr`, `night%d.chr`) and what the spawn tables are indexed by.  Setting it to 2 loads a
   *different map*.  Every docs/312 row labelled "stage-2 basis" (`/tmp/rig/st2w`, `st2sp`, `st2rb`, `st2b1`,
   `sg2`, `bl312e`) is an episode-2 measurement — including the `x = 7096` boss record and the 223 `SPAWNB`
   rejections.  Re-labelled in docs/312 §0a.
2. **"`--warp-stage` breaks proper background loading."**  `--warp-stage M` walks `node+0x34c` and installs the
   M-th node (`f.c:5381-5400`) without re-loading that stage's background — ep1 stage 2 still showed stage 1's
   subway art.  A warp-stage basis can carry state claims only, never screen/sprite/animation claims.
   Witness: `/tmp/rig/e1s2` → `[warp] episode=1 stage=2 node#1 hops=1 rooms=4 camX=3280 camY=0 band=1`, and all
   40 `[vgate]` visits on that run reported `node=0` — `fn_03cc6b` was reached through the *head* node, so its
   gate was evaluated against the wrong record's extent.

"Episode 1, stage 2" is expressible (`--warp-episode 1 --warp-stage 2`) but is a rigged basis; the honest basis is
the shipped's own advance path, which is what the new automation walks.

## 2. What the automation now is (`port/src/app/main.c`, `port/src/f/f.c`)

Host-side counters, explicitly *not* shipped DS claims:

* `g_menu_depth` / `f_rig_menu_depth()` — nesting of `fn_038f3d`.  The shipped body was renamed
  `fn_038f3d_body` and the public name is now a wrapper that increments before and decrements after ✓ the body
  itself is untouched.
* `g_room_advances` / `f_rig_chain_advances()` — incremented at the one instruction in `fn_03d244` that swaps
  `g_dat_63354` (`0x3d29b / 0x3d2a2`).  One call of `fn_03d244` is what a player calls "clearing a stage".
* `g_menu_frames`, printed in the exit census: `stage-walk census: menu-owned frames N of M (…), room-chain
  advances K, menu posts P`.  This is the measurement of "your rig is mostly stuck on the upgrade menu".
* New drive verbs: `t wait state menu-open | menu-closed | advance N` (event-anchored, rebasing like `wait res`)
  and `t menu <timeout>` — a witness-driven menu macro that parks the rest of the timeline and keeps posting
  Enter (the card wait takes Enter/CTRL/ALT/Space, docs/97) with Esc every fourth post (the screen's own exit
  test, 0x3920a-31) until `f_rig_menu_depth()` drops to 0.
* `[vgate]` witness inside `fn_03cc6b` (`NIGHT_ROOMDBG=1`) naming, per visit, the node, its `+0x34c` link, the
  link's `+0x04` and `+0x0c`, `DAT_639e0`, and whether the shipped's `inc` would fire.
* `[request]` witness at `fn_03a829`'s request checks: does the loop ever observe `DAT_6336c != 0`.

Drive: `scripts/rig/drives/ep1_walk_st2.txt` — anchor on `wait res title1.pcx`, Enter presses to start, Right
held, arm-2 requests (`poke 0x6336c 2`, the exit-rect arm fn_03a829 handles at 0x3ac2a-0x3ac7b and fn_00012dcc
writes at 0x12e8f-0x12ef0), `wait state advance 1`, the `menu` macro, then Down held, then Right.

## 3. The stall is a port defect, not a scripting mistake

Runs under the pinned env (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, skip-intro ON):

| run | menu macro | advances | menu-owned frames |
|---|---|---|---|
| `/tmp/rig/w313` (no state waits, 3 requests) | — | 0 | 0 of 12000 (0.0 %) |
| `/tmp/rig/w313b` (60 requests) | fixed keys | 1 | 10858 of 12000 (**90.5 %**) |
| `/tmp/rig/w313c` (4 cycles × 4 Enter + Esc) | fixed keys | 2 | 4454 of 16000 (27.8 %) |
| `/tmp/rig/w313d` (2 cycles) | fixed keys | 1 | 14857 of 16000 (**92.9 %**) |
| `/tmp/rig/w313e` | **witness-driven, 100 posts** | 1 | 14858 of 16000 (**92.9 %**) |

In `/tmp/rig/w313e` the macro posted 75 Enter and 25 Esc pairs over 40 s of screen time and `fn_038f3d(1)` — the
level-up screen `fn_03d244` opens at 0x3d2ac — **never returned**; both `menu` timeouts expired and the rest of
the drive never ran.  `w313c`, with a longer allowance, did get two exits (menu frames 27.8 %), so the screen
does leave sometimes; the amount of keyboard traffic it needs is not bounded the way a key-wait should be.
That single fact blocks every stage-2 measurement this goal needs, and it is the same screen round-4 item H
reports as opening on a glitched picture.

Facts gathered while localizing it:

* The screen leaves only when one of `DAT_00063e89` / `DAT_00063eb2` / `DAT_00063ebe` becomes non-zero
  (shipped 0x3920a-0x39225 → `jmp 0x39a56`), and the port's copy of that test at `f.c:26362` is faithful.
* `DAT_00063e89` is the Esc held-byte of the shipped's `DAT_00063e88[idx]` table (`keymap.c:43`), and
  `fn_0347e3`'s dispatch (shipped 0x34808-0x348c3) only routes `0x38, 0x48, 0x4b, 0x4d, 0x50` plus the modifier
  slots to `fn_034735` / `fn_034635` ✓ Esc and Enter are not dispatched by the pump in either side.
* `fn_038f3d` runs while the keyboard pump is gated off: `g_dat_52ff8 = 0` at 0x39c56 (`f.c:27137`), restored at
  0x3a16d (`f.c:27285`), and `fn_0347e3` returns immediately when it is 0 (`f.c:9126`, 0x347fb/cf).  **This is the
  next thing to measure** — whether the port's `sys_key_spin()` still arms the Esc latch while the pump is gated,
  and what the shipped's INT 09 path does that the port's does not.
* A `[request]` witness shows `fn_03a829`'s loop observed `DAT_6336c = 2` exactly **once** in a run that issued
  60 requests: the request checks sit at the loop bottom (0x3abc0-0x3accd) and the per-room prologue clears the
  cell twice (0x3a95a-0x3a984, 0x3a9e1-0x3aa04), so a drive poke is usually consumed by the prologue, not by the
  arm.  Requests must be repeated until one lands at the check ✓ which is why `w313b/c/e` advance at all.

## 4. Item D: what the shipped's scroll code actually is (and the port already has it)

Full shipped listing, `fn_03cc6b` (0x3cc6b-0x3ce0a):

```
0x3cc83  mov [ebp-4], eax                    ; a0
0x3cc88  mov al,[0x139e4] ; test ; jne 0x3cd4d          ; branch on the scroll-active flag
0x3cc98  mov eax,[a0+0x34c]                             ; the next room node
0x3cca4  cmp edx,[eax] ; jne 0x3cd48                    ; DAT_639dc == [next+0] ?
0x3ccb5  mov [0x13354], eax ; mov byte [0x139e4],1      ; swap to the next room
0x3ccc1  cmp dword [0x13370],1 ; je 0x3cd29             ; snapshot skipped in episode 1
         ... player +0x2b -> 63374, +0x28 -> 63384, +0x27 -> 63378, +0x26 -> 6337c, +0x25 -> 63380 ...
0x3cd29  eax = [[[ebp-4]+0x34c]+0xc] - 0xc8 ; cmp eax,[0x139e0] ; jle skip ; inc [0x139e0]
0x3cd4d  eax = [[[ebp-4]+0x34c]+4] - 1 ; cmp eax,[0x139e0] ; jne 0x3cde3
0x3cd6f  mov [0x13354], eax ; mov byte [0x139e4],0      ; the reverse swap
         ... same player snapshot ...
0x3cde3  eax = [[[ebp-4]+0x34c]+0xc] - 0xc8 ; cmp eax,[0x139e0] ; jle skip ; inc [0x139e0]
```

Both branches end with the *same* vertical test, and `DAT_639e0` is **only ever incremented** here (imm_scan over
the whole shipped: `mov` 0x3cb1f, `inc` 0x3cd42, `inc` 0x3cdfc — no `dec`, no `sub`).  The `mov` at 0x3cb1f is
inside **`fn_03cad3`** (`camY = [node+4]`, `camX = [node+8]` or `[node+8]-0x140`, flag = `[node+0x10]`), which the
port already models at `f.c:9166`.

Reading the port's `fn_03cc6b` against that listing: **it is byte-faithful** ✓ both `inc` arms present
(`f.c:9871`, `9886`), correct operands (`c+0xc` for the limit, `c+0x4` for the equality test), correct signed
tests, correct swap arms.  The only difference is the `c == NULL` early return (`f.c:9834`): the shipped reads
through the null link (`[0+0xc] = 12`, `12-200 = -188 > camY` false ✓ no inc ✓), the port returns ✓ same result ✓

So item D is **not** a missing-store defect in `fn_03cc6b`.  What the measurements show on the legitimate
episode-1 basis (`/tmp/rig/w313c`, no warp flags):

```
frame=0    camY=0   camX=0     e4=0 room=+0x0        (boot)
frame=3900 camY=1   camX=3280  e4=1 room=+0x18da60   player x=3389 y=172 st=2   <- ep1 stage 2
frame=5832 camY=200 camX=3600  e4=0 room=+0x18ddd4   player x=3950 y=372 st=1   <- ep1 stage 3
camY distinct: {0, 1, 200}      player Y distinct (real rows): 172, 372, 388, 395
```

`camY` does climb (0 → 1 → 200) and its ceiling is the shipped's own: the gate is `next+0x0c - 200 > camY`, and
for that pair `next+0x0c = 400`, so 200 *is* the limit the shipped computes ✓ The suspicious half is the
**player**: it stayed at Y = 172 through stage 2 while the view moved ✓ which is the opposite of "the camera
follows the player".  In stage 3 the player does move (372 → 388 → 395 ✓ during the 36 s Down hold ✓).

`fn_01586b` (shipped 0x1586b) is where the player's vertical band comes from:

```
0x15892  mn = [0x13354+0x12] + [0x139e0]                       ; room min + camY
0x158b0  if ([0x13354+0x10] == 0) mx = [0x13354+0x16] + [0x139e0]
0x158c7  else                     mx = [[0x13354+0x34c]+0x16] + [0x139e0]
```

so the band the player is clamped to moves with `camY` ✓ the coupling exists in both directions.  Item D is
therefore still open, with the question narrowed to: **in ep1 stage 2, holding Down must move the player's Y,
and the band it is clamped to must be the shipped's** — which needs a run whose Down window lands *inside* stage
2 (`w313c`'s landed in stage 3 ✓ `w313e`'s never ran because of §3 ✗).

## 5. Next

1. Localize §3: with `NIGHT_KEYDBG=1` (the latch-change witness inside `sys_key_spin`, `f.c:16075-16086`) on the
   `w313e` drive, read whether the Esc latch (`0x63e89`) ever changes while `g_dat_52ff8 == 0` inside
   `fn_038f3d`, and compare with the shipped's INT 09 make/break path that arms `DAT_00063e88[0x01]` without the
   pump.
2. Re-run the stage-walk with the Down window anchored inside stage 2 (after `menu` returns ✓ or with the menu
   skipped by `fn_038f3d`'s own `+0x30 == 1` test at 0x3b54c if §3 fixes the exit).
3. Census `fn_01586b`'s `mn`/`mx` per sample on that run (the shipped's own band arithmetic) and compare against
   the player's recorded Y from the `player_recs.jsonl` sidecar.
4. Items E/F/G/H all need the same route past the menu, so §3 gates them too.

## 6. Item D, root cause found: the caller's threshold was inverted (fixed)

`fn_03cc6b` has exactly one call site, at the tail of `fn_03d4e4` (`f.c:14769` ✓ shipped 0x3d5dd-f8):

```
0x3d5dd  mov eax,[0x53c7c]      ; the player record
0x3d5e2  mov edx,[0x139e0]      ; camY
0x3d5e8  add edx,0xc2
0x3d5ee  cmp edx,[eax+9]        ; player Y
0x3d5f1  jg  0x3d5fd            ; JG = SKIP the call
0x3d5f8  call 0x3cc6b
```

The shipped's `JG` is the **skip** branch: the vertical follow runs only once the player's Y has reached
`camY + 194` ✓ the port had `if (camY + 0xc2 > player[+9]) fn_03cc6b(...)` ✓ the shipped's skip condition
transcribed as its call condition ✓ so the port ran the follow exactly when the shipped refuses it, and stopped
running it precisely when the player had descended far enough to need it.  Fixed at `f.c:14768` (polarity
inverted, both comments quoting 0x3d5f1) ✓

Two fixtures asserted the inverted rule (`test_f1.c:15775`, `15779`: "DAT_639e0+0xc2 > player[+9] -> fn_03cc6b
ran" and "below the vertical threshold -> nothing moves") ✓ they were written from the port's test rather than
from the shipped's jump structure — docs/263 §4's failure class, again ✓ rewritten to the shipped's polarity
(`player[+9] = 0x200` -> the call runs and `DAT_639e0` becomes 1 ✓ `player[+9] = 50` -> the shipped jumps over
the call ✓ `make -C port test` → **ALL PASS** ✓).

Measured red, before the fix (`/tmp/rig/w313g`, stage-walk drive, no warp flags): `camY` set `{0, 1}` over 16000
frames, player record Y constant at 172 ✓ two `fn_03cc6b` visits, one of them with the gate true
(`[vgate] visit#2 e4=1 node=0x1 next=+0x18ddd4 next+c-0xc8=200 camY=0 inc_would_fire=1`) ✓ i.e. the one time it
was called it did the shipped's `inc` ✓ but it was called twice in 16000 frames ✓

The menu-stall half of §3 turned out to be an **injection artifact**, not a port defect: with the macro changed
from a same-instant down/up pair to press-hold-release (800 ms held, 400 ms gap — what a hand does), the level-up
screen left after **4 posts** instead of never (`/tmp/rig/w313g`, `/tmp/rig/w313h`: "level-up screen left after 4
posts") ✓ Third sighting of docs/92/docs/292's class: a key injected faster than the shipped's own reader samples
it looks like a game bug ✓

## 7. What is still open for item D

After the fix, on the same drive (`/tmp/rig/w313h`, `/tmp/rig/w313i`): the player's record Y stays **172** in
stages 2 and 3 even while Down is held for 30 s ✓ whereas on `scripts/rig/drives/ep1_downonly.txt` in **stage 1**
the player does descend (Y set `{172, 190, 195}` ✓ camX 298 ✓ camY 0 ✓ correct, since stage 1's next node has
`+0x0c = 200`, so the shipped's own gate caps `camY` at 0 there ✓).

So the follow code is now reached under the shipped's condition, and the remaining question is the **band**:
`fn_01586b` clamps the player to `[room+0x12] + camY, (room+0x10 ? [[room+0x34c]+0x16] : room+0x16) + camY`
(shipped 0x15892-0x158db ✓ quoted in §4), and in episode 1's stages 2/3 that band apparently does not allow
descending past 172.  Next measurement: a `fn_01586b` witness printing the `mn`/`mx` it computes and the value it
settles on, per call, on the stage-walk drive — then compare against the room node fields the shipped actually
loads for those stages.

## 8. Round-4 verdicts, items D to H

| item | where it was localized (shipped offsets) | red (unfixed build, run path) | fix | green (same basis, run path) |
|---|---|---|---|---|
| **D** stage 2's camera must follow the player down | the call site **0x3d5dd-0x3d5f8**: `JG 0x3d5fd` is the **skip** branch, so `fn_03cc6b` runs only when `camY + 0xc2 <= player[+9]`; the three scroll stores are **0x3cb1f** (`mov [0x139e0], eax`), **0x3cd42** and **0x3cdfc** (`inc [0x139e0]`) -- `imm_scan` finds no `dec`/`sub`, the shipped walks the row down only | docs/314 §1 (pre-fix `[band]`/`[vgate]` census: camY set `{0}` only, 0 gate-true visits) | `6c2d327` + `f6afb71` (`f.c:14768` polarity; two fixtures in `test_f1.c` had asserted the inverted rule and were rewritten) | `/tmp/rig/w316d` (`ep1_walk_st2.txt`, no warp, skip-intro, pinned): `[vgate]` 50 visits `inc_would_fire=1`, `DAT_639e0` 0>16>35>52>73>92, band 115/195>117/197 with camY, player DOWN `fn_01586b` calls 379>1221 |
| **E** one Enter must advance one dialogue line | `fn_00035dfc`'s press gate 0x3a785-0x3a7a4 and its three call sites 0x3b0ec / 0x3b26d / 0x3b4dc; `g_tab_52f2c[3] = 6` cards | **no red exists**: `/tmp/rig/e318g`, `/tmp/rig/e318h` and (on the final HEAD) `/tmp/rig/o320c` -- 6 gates, every one ended through the press arm (spins 1881/6588/18761/21811/25148/29447), cumulative `edges=0`, 0 gates satisfied without a press | none (nothing to fix) | same runs; docs/317 |
| **F** the stage-3 row-offset effect | the writer is **fn_03a5f7** (parity at 0x3a665-0x3a66c, shifted copies at 0x3a66e and 0x3a698) and its **only** call site is **0x3a804** inside `fn_03a70e`, the CG picture show | `/tmp/rig/f317a` and `/tmp/rig/f317b`: the warp basis gives `blt_rows arms B0=268 B1=0 B2=0 B3=0` (no offset arms), the legitimate chain basis gives `B0=1956 B2=125 B3=91` | none to the named routines: `fn_03a5f7`'s arithmetic is modelled at `f.c:14462` and pinned by `test_fn_03a5f7`; the reason the effect never appears is that its gate -- `DAT_00053c80` non-null with the record inside the camera band -- is 0 in 412/412 shipped samples and 1500/1500 port samples | docs/318, `03ff479` |
| **G** one health purchase must step 2 bars | **0x3956e-0x39586** and **0x395d0-0x395d7**: the shipped indexes `DAT_00052738` (HP levels {100,200,300,400,500}) and `DAT_00053c94` (the 6x5 cost table) with the cursor value *as an element*, while its other tables are reached through `shl 2` byte offsets | `/tmp/rig/g319h`: row 1, `sel 0->1` and HP `100 -> 500`; `sel 1->2` and HP `500 -> 0` | `14d6b6a` (`g_tab_52738[sel[1]]`, `g_dat_53c94[page*5 + sel[page]]`), fixture scenario C in `test_fn_038f3d` | `/tmp/rig/g319i`: HP steps `100 -> 200 -> 300` (+100 = two 0x32 bars), every committed purchase steps `sel` by 1; docs/319 |
| **H** picture, then menu, then a screen that stays glitched | **0x3b52b** `call 0x405a7` sits between the tail's end-slide loop (0x3b438-0x3b51e) and `mov eax,1 / call 0x38f3d` at 0x3b542-0x3b547; `fn_000405a7` is the VGA mode-13h set (record dword0 = 0x13 at 0x405bf, register set loaded at 0x4336d-0x4337b), modelled in the port as an audio stop | `/tmp/rig/h321d` (`ep1_st3clear_h.txt`, `--warp-episode 3`, arm-3 poke): the menu (`DAT_633a4` 0->1 at t=83000, `tail: stage=4`) sits inside `640x480 t68783..135333` (9359 dumped frames) and nothing returns to 320x200 | `2336118` (`sys_vga_set_mode(0x13)` seam + `fn_000405a7`) | `/tmp/rig/h321e`: `640x480 t87833..91083` (the end slides) then `320x200 t91083..135333` covering the menu (stage 3->4 at 91500, `DAT_633a4` 0->1 at 92000); docs/321 |

Two things the table deliberately does not smooth over: item E has no red, because none exists to be
found; and item F's red is a *gate* census, not a missing routine -- the effect's arithmetic is present
and faithful, and what is missing upstream is the latch `DAT_00053c80` never becoming non-null (the same
gate that keeps item B's boss from being constructed, docs/312).
