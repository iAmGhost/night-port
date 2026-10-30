# 184 — Item 1, mechanism level: how a track ends, and what the port never calls

docs/183 established the *symptom* from the rig.  This is the mechanism, read out of the shipped binary and the
port, and the two hypotheses that died on the way.

## 1. The shipped game bounds a CD play in seconds of game ticks

`fn_010d47` at guest `0x010d47`:

```
0x10d5f  mov  ebx, 0x1ea                 ; 490
0x10d64  mov  eax, [DS 0x63e38]          ; elapsed game-clock counter
0x10d69  mov  edx, [DS 0x63e38]
0x10d6f  sar  edx, 0x1f
0x10d72  idiv ebx                        ; eax = elapsed / 490
0x10d74  cmp  eax, [DS 0x53c48]          ; DS 0x53c48 is the SELECTED TRACK'S LENGTH IN SECONDS
0x10d7a  jle  0x10dd5                    ; not finished -> return
0x10d7c  mov  [DS 0x63e38], 0            ; elapsed reset
   ...   (0x10da6 reads the room table, 0x10dae shl 2)
0x10db9  call 0x10b1d                    ; select the next track
```

Two corrections to earlier assumptions, both worth writing down:

- `DS 0x53c48` is **seconds, not sectors**: track 9 is `0x171` = 369 and track 11 is `0xee` = 238, which are those
  tracks' lengths in seconds.  The port's `cdda.jsonl` witness prints `sectors` (3 153 for track 1) from a different
  cell, so a "track extent" in the witness and the cell the game compares against are different quantities.
- `490` is the divisor the game itself uses (`0x1ea`), i.e. the game's own notion of ticks-per-second.  This is the
  number docs/168 measured as the shipped clock rate, not the port's pinned `NIGHT_RIG_STEP_CALLS`.

The port models this faithfully (`f.c:2332`):

```c
int32_t q = (int32_t)g_dat_63e38 / 490;      /* IDIV EBX(0x1ea), signed truncated */
if (q <= g_dat_53c48) return;
g_dat_63e38 = 0;
...
fn_10b1d(cd_track_for_room(g_dat_63354));
```

## 2. Hypothesis killed: "the port hardcodes track numbers where shipped computes them"

`branch_xrefs.py` finds 14 CALL edges into `fn_010b1d`.  The port has 15 call sites, and the constants agree with the
shipped instructions at the sites checked:

```
shipped 0x3b077  CMP [4*[DS 0x63370] + 0x2f2c], 0 ; je skip        port: if (g_tab_52f2c[g_dat_63370] != 0)
shipped 0x3b084  MOV EAX, 0xb ; CALL 0x10b1d                        port: fn_10b1d(11)
shipped 0x3b1ea  CMP [4*[DS 0x63370] + 0x2f90], 0                   port: fn_03f15f() == 1 + table gate
shipped 0x3b205  MOV EAX, 0xc ; CALL 0x10b1d                        port: fn_10b1d(12)
shipped 0x3b2c6  MOV EAX, 0xa ; CALL 0x10b1d                        port: fn_10b1d(10)
```

The stage table behind those gates is extracted from `NIGHT.EXE` (`DS 0x52f2c..0x52fb8`, `f.c:27426`).  So the
per-scene track requests are modelled, constants and gates alike.

## 3. What the port actually misses: the title screen never runs the refresher

On the frozen build, title route, `g36` (port 10 Hz, 1 800 samples):

| port time | `DS 0x63e38` | `/490` | `DS 0x53c48` |
|---|---|---|---|
| 0 s | 0 | 0 | 0 |
| 10 s | 6 650 | 13 | 0 |
| 15 s | 1 397 | 2 | `0x2a` (42) |
| 40 s | 18 022 | 36 | `0x2a` |
| **— no samples between 40 s and 105 s —** ||||
| 105 s | 21 347 | 43 | `0x2a` |
| 174.9 s | 67 897 | 138 | `0x2a` |
| 179.9 s | **233 (reset)** | 0 | `0x171` (369) |
| 229.9 s | **2 539 (reset)** | 5 | `0x2a` (42) |

`DS 0x63e38` advances at 665/s under the pinned clock (3 325 per 5 s), so 42 s of the *game's* 490-tick seconds
(= 20 580 ticks) is only 31 s of port ticking.  Yet the selection did not change until 179.6 s, when
`fn_010d47` did fire (the witness shows `stop` then `play 9` at exactly that moment).

The reason is visible in the same dump: **the port produced no cell samples at all between 40 s and 105 s.**  The
title/menu screen runs inside a nested loop that does not reach the frame-boundary pump, and `fn_010d47` is reached
only through the entity/room layer (`f.c:23372`).  A screen that blocks in one game step therefore stops both the
rig's sampling *and* the game's own CD refresher -- the same root symptom seen from the two sides of the rig.

Meanwhile the shipped side selected three tracks inside its first 31 s (`0x2d320` → `0x367aa` → `0x9ddf`) and the
audible one is `0x9ddf` = **track 2, 261 s** (fingerprint +0.837, docs/183 §3).  On this route the port never
requests track 2 at all.

## 4. Two named tests, in order, before any change is made

1. **Who reaches `fn_010d47` while the title screen is up?**  Its only port call site is `f.c:23372`; name the
   enclosing function and determine whether the shipped `fn_010d47` has additional callers (`branch_xrefs.py
   --target 0x10d47`) that the port does not wire -- the same class of miss as docs/181's event table.
2. **Which shipped site asks for track 2 (`0x9ddf`)?**  Search the 14 `fn_010b1d` call sites for a `MOV EAX, 2`
   (or a table load yielding 2); the sites checked so far yield 9, 11, 12, 10 and the room-table mapping.  Until the
   producer of the title selection is named, changing the port's audio layer would be fixing the wrong layer.

Both are static questions with tools already in the tree.  The audio pump's own behaviour (streaming a 42 s request
for 167 s without a stop, docs/183 §2) is real and needs fixing too, but it is downstream: a track the game never
selects cannot be heard however correctly the pump plays it.

## 5. Test 1 answered: the call coverage is exact, so the miss is the loop, not the edge

```
branch_xrefs.py --target 0x10d47   ->  incoming branch edges to 0x10d47: 1
                                        CALL from fn_03a829 at 0x3acd3
```

The shipped refresher has exactly one caller, and the port models it: the call at `f.c:23372` is cited to guest
`0x0003ace6`, inside the same function that holds the `0x0003a878` track request — i.e. `fn_03a829`.  So docs/184 §4
test 1 comes back **negative for a missing edge**: both sides reach `fn_010d47` from `fn_03a829` and nowhere else.

That flips the diagnosis onto the frame loop.  `fn_03a829` is a per-frame routine in the entity/room layer.  The port
produced **no cell samples between 40 s and 105 s** on this route, which is the same window in which its title/menu
screen blocks inside a single game step (`main.c` already carries a comment about screens whose loop never reaches
the frame-boundary `present()`, and separate `card_shots_poll` / `warp_shots_poll` patches for exactly that).  While
that step is blocked, `fn_03a829` does not run, so `fn_010d47` cannot evaluate its `elapsed/490 > length` bound, and
no selection change is possible.  The shipped game's title screen keeps cycling its frame loop, selects three tracks
in its first 31 s, and its third selection (track 2, `0x9ddf`, 261 s) is the one its mix audibly contains.

The fix therefore belongs to the presentation layer, not the CD pump: while a title/menu screen is up, the port has
to keep running the game's frame chain (the `fn_03a829` → `fn_010d47` path included) the way the shipped loop does.
Two things make that the right target rather than a guess:

- it explains an observation from each side of the rig independently — the port's 65 s hole in its own cell dump, and
  the shipped side's three selections inside 31 s on the same drive;
- it is checkable by the existing oracles: `cd_schedule.py` on the selection cells and `music_melody.py` on the
  capture, before and after, with no new instrumentation.

Test 2 stays open and must be answered before the change: which shipped site asks for track 2.  Candidates are the
remaining `fn_010b1d` callers (`fn_038445`, `fn_038ae0`, `fn_039c03`, `fn_03a224`, `fn_03af18`'s three sites) and the
room-table path inside `fn_010b1d` itself.  Naming the producer of the title selection is what turns "the port must
pump frames during menus" from a plausible story into a stated cause.
