# 314. The stage-walk basis, the vertical-follow threshold, and the moment the player record stops being a record

Goal task-8 (round-4 item D: "stage 2's vertical camera must follow the player the way the horizontal camera
does, until the room's bottom is reached").  Everything here is on the **legitimate episode-1 basis**: no
`--warp-episode`, no `--warp-stage`, skip-intro ON, pinned env
(`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), `scripts/rig/drives/ep1_walk_st2.txt`,
5 Hz dumps.  "Stage 2" is episode 1's second room node (`DAT_00063354 = +0x18da60`), "stage 3" the third
(`+0x18ddd4`); `DAT_00063370` stays 1 throughout ✓

## 1. `fn_03cc6b` is faithful; its single caller had the test inverted (fixed)

`fn_03cc6b`'s scroll block was re-read from 0x3cc6b-0x3ce0a (full listing in docs/313 §4): both vertical arms
end in the same test — `eax = [[[ebp-4]+0x34c]+0x0c] - 0xc8`, `cmp eax,[0x139e0]`, `jle` skip, else `inc`
(0x3cd42, 0x3cdfc) — and `DAT_639e0` is never decremented anywhere in the shipped.  The port has both arms with
the right operands and signed tests (`f.c:9871`, `9886`) ✓ **not** a missing-store defect ✓

The defect was at the only call site, the tail of `fn_03d4e4`:

```
0x3d5e2  mov edx,[0x139e0]      ; camY
0x3d5e8  add edx,0xc2
0x3d5ee  cmp edx,[eax+9]        ; player record +0x09 = Y
0x3d5f1  jg  0x3d5fd            ; JG = SKIP the call
0x3d5f8  call 0x3cc6b
```

The port read the shipped's skip branch as its call branch (`if (camY + 0xc2 > player[+9]) fn_03cc6b(...)`).
Fixed at `port/src/f/f.c:14768` by negating the test, with both comments quoting 0x3d5f1 ✓ Consequence of the
correct rule: the follow runs only once the player's Y has reached `camY + 194` — an edge-triggered camera,
which is what "follows the player down" means in this engine.

**Two fixtures had encoded my inversion** (`test_f1.c:15775`, `15779`: "DAT_639e0+0xc2 > player[+9] -> fn_03cc6b
ran", "below the vertical threshold -> nothing moves").  They were written from the port's test, not from the
shipped's jumps — docs/263 §4's failure class a third time ✓ Rewritten to the shipped's polarity
(`player[+9] = 0x200` → the call runs and `DAT_639e0` becomes 1 ✓ `player[+9] = 50` → the shipped jumps over the
call ✓): `make -C port test` → **ALL PASS** ✓

Measured red before the fix (`/tmp/rig/w313g`): `DAT_639e0` = {0, 1} across 16000 frames, `fn_03cc6b` reached
twice in 16000 frames, and the one visit whose gate was true did perform the shipped's `inc`
(`[vgate] visit#2 e4=1 node=0x1 next=+0x18ddd4 next+c-0xc8=200 camY=0 inc_would_fire=1`) ✓ arithmetic fine,
threshold wrong ✓

## 2. What the stage-walk basis measures (`/tmp/rig/w314i`, `/tmp/rig/w314j`)

`[band]` witness (new in `fn_01586b`, prints only when the band changes — a per-call print turned the run
print-bound and the drive's wall-clock plan never reached gameplay, the docs/302 rate lesson applied to my own
instrument; `NIGHT_YCLAMP` now selects the light witness, `NIGHT_YCLAMP_FULL` the heavy one):

```
[band] #1 kind=1 x=319 y=150 d=1 mn=139 mx=195 camY=0   room=+0x18d6ec  (stage 1)
[band] #2 kind=0 x=3370 y=172 d=1 mn=115 mx=195 camY=0   room=+0x18da60  (stage 2, player)
[band] #3 kind=1 x=3675 y=341 d=1 mn=320 mx=395 camY=200 room=+0x18ddd4  (stage 3)
```

* Stage 1 genuinely cannot scroll vertically: its next node has `+0x0c = 200`, so the shipped's own gate
  `200 - 200 = 0 > camY` is false ✓ `[vgate]` in a Down-held stage-1 run: 49 visits, 0 gate-true ✓ and the
  player does descend there (Y = 172 → 187 → 195 ✓ `player DOWN fn_01586b calls 979` ✓) ✓
* Stage 3's `camY = 200` comes from **`fn_03cad3`'s `mov [0x139e0], eax`** (the room installer's `camY =
  [node+4]`, 0x3cb1f), not from the follow ✓ so the per-node camera row already works in the port ✓
* Stage 2's band permits 115..195 ✓ so the descent the playtester describes is reachable ✓ and with
  `camY = 0` the follow needs Y ≥ 194 ✓

## 3. Three rig bugs that were masquerading as game bugs

1. **Same-instant key pairs.** The level-up screen leaves only on `DAT_63e89/63eb2/63ebe` (0x3920a-31) and has a
   release drain (0x38edc-f21) that consumes the latch. A down+up posted in one pass hid it: the latch witness
   showed `0x63e89` rising and falling 18 times during a stuck window (`/tmp/rig/w314f`, ticks 63934-67237)
   while the exit test never sampled it. Press → hold 800 ms → release leaves the screen in **4 posts** instead
   of 100-and-never (`/tmp/rig/w314g`, `/tmp/rig/w314e`). Third sighting of docs/92 / docs/292.
2. **`menu` used the line time as its timeout** → "menu drive timed out after 0 posts, screen still open at
   t=39250ms", twice, 14450/16000 frames menu-owned (`/tmp/rig/w314d`). Its 4th token is now a *duration*
   armed when the line becomes current ✓
3. **A no-op `menu` rebased the timeline.** 17 such lines × −60000 ms slid the drive's tail into the past
   (`key 0x4d up (t=-737000ms)`, `key 0x50 down`/`up` 500 ms apart) and left the Down latch at 0 in 1334/1334
   samples (`/tmp/rig/w314g`): `walking-body calls 18194, player DOWN fn_01586b calls 0` ✓ A macro that found
   no screen open now does nothing, and a macro that ran pushes the tail *later* by the time it consumed, not
   earlier ✓

Rig counters added and exported through the sidecar (`player_recs.jsonl`): `md` (menu nesting), `adv`
(room-chain advances), `wk` (`LAB_0003284e` entries), `wa` (its arrow-admission passes), `wv` (its Down→
`fn_01586b(+step)` calls), `f30` (player record +0x30), `ag` (`fn_03272f`'s `+0x30 == 1` early returns).

## 4. The real blocker for item D, and probably for C and H too

`/tmp/rig/w314m`, first transition of the player record's own shape:

```
tick=524  f30=0 ag=0   wk=163 wa=163 wv=1  y=372 st=1  hp=1000  adv=2 md=0     last sane sample
tick=760  f30=1 ag=7   ...                    y=372 st=21 hp=984               (+0x30 set, HP took a hit)
tick=780  f30=43 ag=289 ...  y=724249387 st=724249387 hp=11051                (record is no longer a record)
...     to tick 1333, the end of the run, every field 724249387 = 0x2B377B0B
```

From tick 780 on, `g_player_ent` reads as **host pointer words in every field** ✓ the dump-side
`refused (outside arena) = 5336` ✓ and the two facts that follow are exactly the reported symptoms:

* `fn_03272f` (the player action resolver, called per frame from `fn_03a829` at 0x3ab02) hits
  `if (PL8(0x30) == 1) return;` (0x3274f-59, faithful ✓) and returns — `ag` climbs from 0 to 289 ✓ so no
  walking body, no vertical mover, no keyboard response ✓ `wv = 1` for the whole run ✓
* the follow call at 0x3d5f8 therefore never gets its condition (`playerY ≥ camY + 194`) ✓ `DAT_639e0`
  changes only at room installs ✓ "the camera does not follow the player down" ✓

So item D's remaining root cause is not in `fn_03cc6b` and not in the scroll stores: **the block that
`g_player_ent` names is overwritten while the pointer stays put.**  `/tmp/rig/w314n` (offset and span now in the
sidecar) settles which of the two it is:

```
tick=23   off=+0x3a75d8 span=0x160 tag=0   st=0        y=172      hp=1000   adv=0
tick=725  off=+0x3a75d8 span=0x160 tag=43  st=garbage  y=garbage  hp=11051  adv=2  ag=289
```

The offset and the span never change, so no re-point happens ✓ and the shipped's own store census agrees: the
only two writers of the player pointer in NIGHT.EXE are 0x3774c (stage loader) and 0x3b01b (the same construction
in `fn_03af18`'s tail), both modelled (`f.c:5803`, `f.c:28404`) ✓ **The record's tag byte goes 0 → 43** while its
body turns into host pointers: something reuses or overruns the player's 0x160-byte block during episode 1's
third room node.  Round-2 item C ("clearing stage 2 re-enters the same map with no enemies") and round-4 item H
(the menu opening on a glitched picture) sit in the same window, so this is the highest-value lead in the set.

## 5. Next

1. Find the writer that turns the player's block into pointer words: candidates are a record construction that
   reuses the block (`fn_010efc` ✓ the shipped's latch site 0x10f1d-0x10f2b ✓ docs/247/252's slot-identity
   family ✓), an overrun past the 0x160 span, or `fn_041e20`/`memset` with a size that exceeds the record.  The
   shipped's store census must be built per displacement (`field_writers.py ✓ docs/297`) rather than by grep,
   and docs/310's rule applies: "no writer exists" needs a census, not a pattern search.
2. Then re-measure item D on the same drive: green = during a held-Down window in `+0x18da60`, the player's Y
   climbing to the band bottom (195), the 0x3d5f8 call firing, `[vgate]` gate-true counts > 0 and `DAT_639e0`
   climbing toward the node's `+0x0c - 200 = 200` ✓ plus a `test_f1.c` fixture pinning the two `inc` arms (the
   caller's polarity is now pinned by the two rewritten `fn_03d4e4` fixtures ✓ a store-level fixture is still
   owed ✓).
