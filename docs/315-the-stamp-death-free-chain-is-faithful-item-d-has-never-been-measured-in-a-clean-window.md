# 315. The stamp→death→free chain is faithful, item D has never been measured in a clean window, and a latch cell read at the wrong width lied

Goal task-8 (round-4 item D).  Basis: episode 1's own room chain, no warp flag, skip-intro ON, pinned env
(`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), `scripts/rig/drives/ep1_walk_st2.txt`, 5 Hz
dump `/tmp/rig/w315j` (1334 samples = 266.8 s).

## 1. docs/314 §4's "the player record stops being a record" is faithful behaviour, end to end

I followed every link against the shipped's bytes.  Nothing invented, nothing patched.

**Who stamps the player.** `field_writers.py --disp 0x30` gives 24 stores; the ones that can write 1 on a walked
record are fn_0113e7 (0x11651/0x11662/0x11673), fn_011827 (0x1191a), fn_013c4d (0x141ca), fn_01441e
(0x14913/0x1499f/0x14c1a/0x14cd5/0x14d06), fn_035c5c (0x35cc3); 2 is written at 0x1498d and 0x32c6e/0x32d4c.  The
witness names fn_01441e as the actor, and its two sites read faithfully:

```
0x14968  movsx eax, word [cur+0x23] ; test ; jg 0x149ac        ; debt still owed -> stop here
0x1497d  add  word [cur+0x29], dx                               ; settle the debt into HP
0x14984  mov  word [cur+0x23], 0
0x1498d  mov  byte [cur+0x30], 2                                ; settled, still alive
0x14994  movsx eax, word [cur+0x29] ; test ; jg 0x149ac
0x1499f  mov  byte [cur+0x30], 1 ; mov word [cur+0x29], 0       ; nothing left
...
0x14cba  movsx eax, word [cur+0x23] ; test eax,eax
0x14cc0  jne  0x14cc8 -> 0x14cd0 -> 0x14cdb                      ; debt != 0: no stamp
0x14cc2  cmp  [ebp-0x24], 0xb ; jge 0x14cca
0x14cca  cmp  [ebp-0x24], 0x14 ; jle 0x14cd2
0x14cd5  mov  byte [cur+0x30], 1                                 ; debt == 0 AND 11 <= atype <= 20
```

`atype` (the shipped's `[ebp-0x24]`) has exactly one store, 0x14829, fed by `edx = [ebp-0x10] + [ebp-0x2c]` /
`mov al, byte [edx + 0x38]` at 0x14821-0x14826 = `byte[rec_e + slot + 0x38]` ✓ the port's `f.c:25768` ✓
and the port's guard `debt == 0 && atype >= 0x0b && atype <= 0x14` (`f.c:25888`) is the same test ✓

The player is *not* excluded from this region: the shipped's player tests sit at 0x147f0, 0x1482c and 0x14c6c and
route the player into its own arms (`0x147f8-0x147fb` even stores the victim into DAT_53c88 only when the victim
is NOT the player ✓ modelled at `f.c:25764` ✓), but 0x14968-0x149a6 and 0x14cba-0x14cd9 have no player test ✓

**Who frees it.** `f.c:27020` (`fn_0112fd(e, 0); HEAP_FREE(e - hb)`) is 0x12a6c-0x12a71 ✓ and the shipped reaches
it for *any* record whose +0x30 == 1, player included: `0x12a67 xor edx,edx / 0x12a69 mov eax,[ebp-4] /
0x12a6c call 0x112fd / 0x12a71 call 0x41c28` is unconditional ✓ The request writes on the same path are
0x129ed (`= 1`, only when the walked record == `[0x3c7c]` ✓ i.e. the player ✓), 0x12a04 (`= 3`, == `[0x3c80]` ✓
the latched guardian ✓) and 0x12a5d (`= 4`, quota ✓) ✓

**Store census for the request cell** (`imm_scan --imm 0x1336c`): ten stores total — 0x129ed, 0x12a04, 0x12a5d,
0x12eba, 0x12ee7 (set 1/3/4/2/2) and 0x3a976, 0x3a9dc, 0x3abb7, 0x3ac14, 0x3ac67 (clears) ✓ the port writes the
cell at ten matching places (`f.c:27007/27008/27017/27146/27150` and `23979/23982/24074/24084/24088`) ✓ **no
per-frame clearer exists on either side**, so docs/312's "the poke dies in the prologue" applies only to pokes
issued between the prologue and the check, not to a request written by the death path ✓

**Witness-observed chain** (`/tmp/rig/w315h`, `/tmp/rig/w315i`, `/tmp/rig/w315j` — the arena witness prints each
link with the shipped's site name):

```
arena: fn_01441e 0x1498d debt settled, hp>0  ... PLAYER block off=0x3a75d8 tick 12001
arena: PLAYER +0x30 0 -> 2 at tick 12001 (state 0x11 hp 995)
arena: fn_01441e 0x14cd2                     ... PLAYER block off=0x3a75d8 tick 12093
arena: PLAYER +0x30 2 -> 1 at tick 12093 (state 0x15 hp 966)
deathtrace: tick=13255 off=0x3a75d8 type=0 state=0x1 hp=994 exp=5 live=1 PLAYER
arena: FREE on the PLAYER block off=0x3a75d8 tick 12381 held-by=- site=f.c:27020
[request] seen tick=12475 value=1 room=0x18ddd4 stage=1
```

so the player is stamped settled, then dying, then removed from the entity list and freed, and fn_03a829's
bottom-of-loop test does see request 1 ✓

**What each side does next.** The shipped's consumer is `fn_03a829`'s arm 1 (0x3abae-0x3abf5: clear the cell,
test DAT_53100, `call 0x38445`, then `[ebp-8] = 0` or `2` and `jmp 0x3acdd` = the epilogue) ✓ and
`fn_03af18`'s dispatch at 0x3b1b7:

| r | shipped | effect on the dangling player pointer |
|---|---|---|
| 1 | `call 0x3ace9` (0x3b1c9), stage tail, `g_dat_63370 += 1` | repaired at the next stage load (0x3774c) |
| 2 | `call 0x376c8` (0x3b559) then `jmp 0x3b1b2` (room loop again) | **repaired immediately** — fn_0376c8 IS the hero spawn (`fn_010efc` at 0x37733, `mov [0x3c7c], eax` at 0x3774c ✓ port `f.c:5801-5803` ✓) |
| 0 | `DAT_63370 = 1; call 0x37347; DAT_63398 = 0; jmp 0x3af69` (menu loop) | repaired only when a new game starts |
| -1 | `call 0x10dde; call 0x37306; "Thank You For Playing NIGHT"; ret` | run ends |

The port models all five (`f.c:24073-24098`, `f.c:28495-28509`) ✓ so the garbage tail in my drives is a *drive*
artifact: after a legitimate death the drive has no keys left to start a game, the port waits in fn_038445/menu
with the shipped's own stale `DAT_53c7c` ✓ and a playtester with hands presses something ✓

## 2. Item D still has no clean measurement: the Down window sat inside the level-up screen

`/tmp/rig/w315j`, Down-held sample ranges (latch read at byte width, see §3):

```
44.2-73.8s (149 samples) menu_depth=1 in every sample
                          walking-body calls 125 -> 125, admissions 125 -> 125, DOWN fn_01586b calls 1 -> 1
                          player Y set {173}
```

Every sample of the 30 s Down window had `fn_038f3d` open, so `fn_03272f` was never called and the resolver's
walking body never ran ✓ (docs/314 §3's "a DOWN window must not sit inside a menu" warning, triggered again by
the `wait state advance 1` rebase pulling the window ~16 s earlier than scheduled ✓).  The vertical camera cell
changed exactly once in the whole 266.8 s: `DAT_639e0: 0 -> 200 at 104.8 s` — that is `fn_03cad3`'s install
(0x3cb1f `mov [0x139e0], eax` ✓ = `camY = [node+4]` ✓), not one of the two follow arms (0x3cd42, 0x3cdfc) ✓

Drive fix landed: the Down window is now anchored on the phase, not the clock —

```
60200 menu 90000
60300 wait state menu-closed
60400 key down down
90400 key down up
90500 menu 90000
```

**Item D therefore remains open**: the threshold fix (`f.c:14768`, docs/313/314 §1) is in and correct, the two
`inc` arms exist and are arithmetically faithful, and the gate is edge-triggered (`fn_03cc6b` is called only when
`camY + 0xc2 <= player[+9]` ✓ 0x3d5f1's JG is the skip ✓), but no run has yet held Down in a menu-free stage-2
window to show the follow firing ✓

## 3. The arrow latch cells are bytes; reading them as dwords invents values

`DS 0x63ed0/0x63ed3/0x63ed5/0x63ed8` (Up / Left / Right / Down per `keymap.c:48`) read at width 4 in the dump
came back as `4352, 9216, 14080, 3328, 8192` for Down and `335544320` for Left ✓ i.e. neighbouring latch bytes
shifted into the value ✓ Read at width 1 the same samples are clean: Down = 1 in 149 samples, Right = 1 in 249,
Up and Left never ✓  Every latch claim in this doc uses width 1 ✓ Fourth sighting of docs/270's
declaration-width family (a dump read is only as good as the declared width) ✓

Related and quotable: `DAT_00063360` — the counter that indexes the walk-step table
(`step = [[0x52ffc] + ((DAT_63360 % 4) * 3 + rec[+0x25])] * 4`, shipped 0x32891-0x328a9 ✓ port `f.c:27858` ✓)
— sat at 2521 in 674 samples, 125 in 302, 0 in 126 ✓ so the *step magnitude* is row `%4 == 1` for half the run ✓
worth checking if the follow arms ever start firing ✓

## 4. A new fault on HEAD, bracketed but not yet localized

With the drive edited as in §2, a 16000-frame run faulted (`rc=139`, SIGSEGV) after `cdda: track index 9`.
Bracketed on the same drive, witnesses on, plain binary:

| `--frames` | rc | wall | last frame presented |
|---|---|---|---|
| 800 | 0 | 15 s | 781 |
| 1600 | 0 | 29 s | 1548 |
| 3200 | 0 | 58 s | 3182 |
| 6400 | 0 | 117 s | 6356 |
| 16000 | 139 | — | — |

`make -C port asanapp` + the same drive at 500 frames: `rc=0`, no AddressSanitizer/UBSan report ✓ so the fault is
between 6400 and 16000 frames ✓ (i.e. in the stage-2/stage-3 region this drive reaches ✓ the same region the
playtester's round-1 crash report named ✓).  Next bracket: 9600 / 12800 frames, then ASan at the bracketed count.

## 5. Next

1. Bracket the fault (9600/12800 ✓ then ASan at that frame count) and localize it from bytes.
2. Run the menu-anchored Down window and read `DAT_639e0` / `[vgate]` / `[band]`: green = the player descends to
   the band bottom (`room[+0x16] + camY`), `fn_03cc6b` is reached, at least one `[vgate]` visit has
   `inc_would_fire = 1`, and `DAT_639e0` climbs toward `[node+0x0c] - 0xc8` ✓ plus the owed store-level fixture
   for the two `inc` arms (the caller's polarity is already pinned by the two rewritten `fn_03d4e4` fixtures ✓).
3. Items E/F/G/H all need the same drive to reach ep1 stage 3 cleanly, so the fault in §4 is the shared blocker.
