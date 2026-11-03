# docs/312 — the three 2026-11-03 stage-2 reports: what the shipped bytes say, what the rig could and could not reach

Reports (round 2 of `docs/manual-playtest/05-2026-11-03.md`, build at `0e79e44`+`9abb102`):

- **A** — stage 2's vertical camera scrolls on its own while the player stands still.
- **B** — stage 2 can be passed without defeating its boss.
- **C** — clearing stage 2 does not advance to the next stage but re-enters the same stage-2 map, with no enemies.

Two port defects were found and fixed for these reports (`444c2aa`, `b5355d3`); the third mechanism turned out
to be the shipped's own, and one half of each report is handed to the playtester because the rig cannot reach
the phase.  Every number below names the run that produced it.

## 0a. Basis correction (2026-11-03, after the playtester's correction)

The playtester's reports are about **episode 1, stage 2 / stage 3**.  Everything in this document labelled
"stage-2 basis" was produced with `--warp-episode 2`, which is **not** episode 1's stage 2:

* `--warp-episode N` overwrites `DAT_00063370` (`f.c:5362`) — the counter that names every per-stage resource in
  the loader (`stage%d.wal` at `0x37b61-71`, `stage%d.stg` at `0x37c58-68`, `st%ds.spr`, `night%d.chr`) and that
  indexes the spawn tables.  Setting it to 2 loads a **different map** (episode 2: `stage2.stg` with 184 records,
  its type-2 record at x=7096 y=166, `stage2.wal`, `night2.chr`).  The shipped itself calls this counter
  "stage"; the port's own header records the history (`f.c:5341-5348`).
* "Episode 1, stage 2" is `--warp-episode 1 --warp-stage 2` (`f.c:5381-5400`): the warp walks the shipped's own
  next-room link (`node+0x34c`, set at `0x3caa2`) and installs that node ✓ which is what a player means.

Consequence, stated plainly: the rows in §2 and §3 naming `stage2.stg`, the `x = 7096` boss record, the 223
`SPAWNB` rejections, `DAT_639e0 = 0` in 1116 samples and the `blt_rows calls=268 ... all B0` arm census are
**episode-2 measurements** ✓ they describe a real map and a real gate, but they cannot gate an
episode-1-stage-2 report, and §2's "the rig cannot reach the boss" conclusion is not established for episode 1.

What is untouched, because it was measured while walking episode 1's own chain: `ep1_st2.txt` advances episode
1's nodes in order — `0x…62ec → 0x…6660 → 0x…69d4 → 0x…d48`, witnessed as `stage=1 node#0 counter=1/4` …
`node#3 counter=4/4` (`/tmp/rig/l312c`, `/tmp/rig/st2g`) — so item A's red/green pair (`/tmp/rig/st2c`+`st2d` →
`/tmp/rig/st2e`), the level-up row census (`/tmp/rig/l312e`, 23 778 row visits, 0 lit-without-purchase) and the
`fn_03ce0b` arm census (`/tmp/rig/bl312f`: `calls=936`, `B0=576 B1=0 B2=103 B3=154`, 0 out-of-band) are
episode-1 numbers, covering episode 1's stages 2, 3 and 4 ✓

Basis correction, second pass, from the playtester's two follow-ups:

1. `--warp-stage M` does **not** produce a usable stage-2/stage-3 visual basis.  It installs the M-th node
   (`f.c:5381-5400`) but the background art is not re-loaded for that stage — ep1 stage 2 still shows stage 1's
   subway background ✓ the playtester's observation ✓.  Warp-stage runs can carry DS/state claims only, never
   screen, sprite-placement or animation claims.  Witnessed: `/tmp/rig/e1s2` prints
   `[warp] episode=1 stage=2 node#1 hops=1 rooms=4 camX=3280 camY=0 band=1`.
2. On those same runs the camera routine never sees the warped node: every `[vgate]` visit (the
   `NIGHT_ROOMDBG` gate census added for item D, `f.c:9857-9876`) reports `node=0` with `next=+0x18da60` and
   `next+0x0c - 0xc8 = 0`, so `inc_would_fire = 0` in 40 of 40 visits — `fn_03cc6b` is reached through the head
   node and its gate is evaluated against the wrong record's extent ✓ a warp cannot demonstrate the vertical
   follow either way ✓
3. The poke-driven drives also stall: an arm-2 request makes `fn_03d244` open the level-up screen at `0x3d2ac`,
   and the run then spends its frame budget inside `fn_038f3d` ('mostly stuck on the upgrade menu').  Item D and
   the stage-3 items need an automation that follows the shipped's own advance path — request 2 → chain advance
   → menu keys + Esc → room install at `fn_03cad3` (`0x3d2bb`) — repeated until the target stage, with no warp
   flag.  The episode-1 chain runs already show that path working (`/tmp/rig/l312c`:
   `stage=1 node#0 counter=1/4` … `node#3 counter=4/4`; `/tmp/rig/st2g`: 9 menu entries, chain
   `62ec → 6660 → 69d4 → d48`).

## 0. Basis

Pinned env for every run in this document: `SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665
NIGHT_RIG_FPS=60`, `NIGHT_SKIP_INTRO=1` (docs/309: it shifts frame numbers, so frame-anchored numbers are
comparable only inside one mode), `--rig-hz 5` unless stated.  Drives:

| drive | run(s) | what it establishes |
| --- | --- | --- |
| `ep1_st2.txt` (poke `DAT_6336c=2`, Right held, Esc after requests) | `/tmp/rig/st2c`, `/tmp/rig/st2d` (red, unfixed), `/tmp/rig/st2e` (green), `/tmp/rig/st2f` (plain, 42000 frames), `/tmp/rig/st2asan`, `/tmp/rig/st2g` | the room chain advances (or does not), the chain terminator, the NULL-link crash and its fix |
| `ep1_st2w.txt` + `--warp-episode 2` | `/tmp/rig/st2w`, `/tmp/rig/st2sp`, `/tmp/rig/st2rb`, `/tmp/rig/st2b1`, `/tmp/rig/sg2`, `/tmp/rig/hd2` | **episode 2, not ep1 stage 2 (see §0a)**: an episode-2 sample basis (1116 of 1138 samples at `DAT_63370 == 2`), the `stage2.stg` tag-1 type census, the episode-2 spawn-gate verdicts |
| `ep1_st2_hd.txt` + `--warp-episode 1/2` | `/tmp/rig/sg1`, `/tmp/rig/sg2` | the .stg loader census on both episodes |
| `ep1_roomclear4.txt` (poke 3, 900 s of events) | `/tmp/rig/s2full2` (docs/310), `/tmp/rig/st2nat` (killed at ~frame 5700 of a 16000-frame route) | the stage tail reaching stage 2, the DS trajectory of the episode counter |
| `ep1_walkfar.txt`, `ep1_walkfar2.txt` (new, no pokes at all) | `/tmp/rig/wk1`, `/tmp/rig/wk2` | how far a natural walk gets: not far enough to reach the boss window |

## 1. Item A — the camera scrolls while the player stands still

### Localized defect (fixed, `444c2aa`)

`fn_03a829` (the room loop, shipped `0x0003aa2b-0x0003acdd`) re-reads `DAT_00063354` — the *current* room
record — before every call that takes a room argument:

```
0x0003ab18            -> CALL 0x3d4e4     the camera policy (its stop limits are the room's +0x2a/+0x2e/+0x32/+0x36)
0x0003aba4            -> CALL 0x3d606
0x0003ac4f / 0x3ac54  -> MOV EAX,[0x13354]; MOV EAX,[EAX+0x34c]      request-2 (exit-rect) arm
0x0003ac5a            -> MOV AL,[EAX+0x11] -> [0x13398]
0x0003ac71 / 0x3ac76  -> MOV EAX,[0x13354]; CALL 0x3d244
0x0003ac86 / 0x3ac91  -> request-4 (quota) arm, same two reads
0x0003aca8 / 0x3acad  -> MOV EAX,[0x13354]; CALL 0x3d244
0x0003acbb / 0x3acc7  -> MOV EAX,[0x13354]; the all-clear writes to +0x32 / +0x36
```

The port loaded `uint8_t *room = g_dat_63354` once at entry and used that local at all six sites.  `DAT_63354`
is swapped *inside* the loop — by `fn_03cc6b` (the vertical camera arm, `0x3ccb5`) and by `fn_03d244` itself
(`0x3d286-0x3d2a2`) — so the port kept handing the camera policy a record the loop had already left.  Item A's
symptom is exactly that: the policy stops the scroll at the *previous* room's limits, so the scroll runs while
nothing in the current room asks for it.

### Red / green, same drive (`ep1_st2.txt`)

Red (unfixed, `/tmp/rig/st2c` and `/tmp/rig/st2d`, reproduced twice): 5 `fn_03d244` calls, all with
`node=0x…62ec` and the same `next=0x…6660` — the room record never advances.  `DAT_63354` changes 3 times in
1914 samples.  Idle-camera pairs (player X, Y and state unchanged while `DAT_639e0` moves) = **14**; unmoving
pairs 1764.  `DAT_639e4 = 1` share **1191/1914**.

Green (`/tmp/rig/st2e`): nodes advance `62ec → 6660 → 69d4 → 6d48` (3 advances over episode 1's 4-node chain),
`DAT_63354` distinct 5, idle-camera pairs **1** (from 14), unmoving pairs 814, `DAT_639e4 = 1` share
**50/885**.

### The stage-2-internal re-check: not reachable by the rig, stated as such

The stage-2 sample basis the rig can build keeps the vertical camera at zero: `DAT_639e0` = 0 in all 1116
stage-2 samples of `/tmp/rig/st2w`, and 0 in all 1044 samples of `/tmp/rig/s2full2` (`cam_y` distinct = 1).  A
defect that only shows as unwanted *vertical* scroll inside stage 2 therefore cannot be redemonstrated there.
The stage-1 red/green pair above is the rig's closure of item A; **the stage-2 confirmation is the playtester's**
(scroll behaviour after walking stage 2 right, on the current build).

## 2. Item B — "stage 2 can be passed without defeating its boss"

### The shipped's request machinery, re-read this turn, against the port

```
0x000129d1-0x129dc  MOV AL,[EAX+0x30]; CMP ,1; JNE 0x12a7b   the dying flag gates the whole arm
0x000129e5-0x129eb  CMP eax, [0x3c7c]  -> 0x129ed MOV [0x1336c], 1   the record is DAT_53c7c  (player)
0x000129fc-0x12a02  CMP eax, [0x3c80]  -> 0x12a04 MOV [0x1336c], 3   the record is DAT_53c80  (the latch)
0x00012a10-0x12a1a  MOV [0x3c88] vs walked record; clear      DAT_53c88, the second gauge entity -- no request
0x00012a24-0x12a33  record kind == 1 -> INC [0x139bc]                  the kill counter
0x00012a39-0x12a5d  [0x13370]-1)*0x28 + [room+0x40])*4 + 0x2a1c; CMP kills; JL 0x12a67
                                                            -> MOV [0x1336c], 4  (quota met)
0x10f1d-0x10f2b     CMP type_idx, [0x13370]; MOV [0x3c80], record      the latch site
```

Port: `f.c:26834` (dying flag), `f.c:26844` (request 1 vs `g_player_ent` = DS `0x53c7c`, f.c:8371),
`f.c:26845` (request 3 vs `g_latched_ent` = DS `0x53c80`, f.c:8373), `f.c:26846` (gauge cell `0x53c88`),
`f.c:26850-26854` (kill counter `DAT_639bc`, quota `g_tab_52a1c[room+0x40 + (DAT_63370-1)*10]`),
`f.c:1336-1339` (latch).  All four request setters and the latch match instruction for instruction.

What each request does in `fn_03a829` (shipped `0x3abc0-0x3acb1`, port `f.c:23920-23937`):

```
0x0003abc0          request 1 -> break            (leaves the loop to 0x3aeb2; returns 0 or 2, never 1)
0x0003abca-0x3abdd  request 3 -> clear, return 1  (the ONLY return value fn_03af18 accepts as a stage clear)
0x0003ac2a-0x3ac7b  request 2 -> DAT_63398 from [node+0x34c]+0x11, clear, CALL 0x3d244, JMP 0x3acb2
0x0003ac7d-0x3acb1  request 4 -> same two reads, clear, CALL 0x3d244, falls to 0x3acb2
0x0003acb2-0x3ace1  the all-clear test, then CALL 0x10d47 and the loop continues
```

Arms 2 and 4 fall through into the loop tail: **they change rooms, they cannot end a stage** ✓ verified again
this turn from `0x3ac2a-0x3acc7`.  So "quota kills passed the stage without the boss" is not a mechanism the
shipped has, and the port does not model one either.

### The boss is in the data (loader census, `NIGHT_STGDBG=1`)

`fn_037b03`'s record loop, counted per tag-1 record type, and the one record whose type equals the episode
counter logged with its spawn fields:

```
/tmp/rig/sg1: stgdbg stage=1 records=88  tag1: 1:1 11:1 31:11 32:6 36:7 37:3 41:5 42:3 46:1
             stgboss stage=1 rec=0018ef98 sub=0 x=4061 y=369 zone=0 posmode=1
/tmp/rig/sg2: stgdbg stage=2 records=184 tag1: 2:1 12:2 13:1 51:27 52:17 56:13 57:6 61:6 62:9 63:4
             stgboss stage=2 rec=001ae224 sub=1 x=7096 y=166 zone=0 posmode=1
```

Each stage declares exactly one record whose type is the episode counter — the only record `fn_010efc`
(`0x10f1d-0x10f2b`) can latch as `DAT_53c80`, whose death is the only event that produces request 3 ✓ so the
shipped does gate the stage on the boss, and the type-2 boss record for stage 2 exists in `stage2.stg`.

### Why the rig never sees it (spawn-gate witness, `NIGHT_SPAWNTRACE=1`)

`e20f_spawn`'s old witness only fires for records that already passed the gates, so `fn_03e20f` now evaluates
and prints every gate before the arm runs (`SPAWNT` per type, capped; `SPAWNB` for the episode's own type,
uncapped).  Stage 2, `/tmp/rig/st2b1` (`--warp-episode 2`):

```
SPAWNB rec=001ae224 t=2 sub=1 gate=0 cgate=1 rx=7096 ry=166 room11=0 camx=0 camy=0
       cnt=4 cap=9 capok=1 rect=[0,0,0,0] inrect=0 camband=1 roomx=[0,1920] inx=0 inxw=0 col=0
223 dispatches, every one of them inx=0                      (shipped 0x3e4a4-0x3e50f: room[0x00] <= rx <= room[0x08])
3 spawns passed the gates in the whole run: types 51, 51, 52   (0x3e39f-0x3e3fd / 0x3e4a4-0x3e50f)
0 latch events; DAT_6336c census {2: 1040, 0: 76}; latch cell 0 in 1116/1116 samples
```

`rx = 7096` is far outside the room window `[0, 1920]`, so the boss record is dispatched 223 times and
rejected 223 times by the room-window test — not by the quota (`capok=1`) and not by the vertical band
(`camband=1`).  In stage 1 the same shape appears on every basis: the boss record sits at `rx=4061 ry=369`
while the walked room's window is `[0, 1600]` (`/tmp/rig/wk1`: 2187 dispatches, all `inx=0`; `/tmp/rig/st2nat`:
1601 dispatches, all `inx=0`).

The natural walks (no request pokes at all) show how far the rig gets.  `/tmp/rig/wk1` (`ep1_walkfar.txt`,
Right held 10 s → 600 s with re-arms): player X runs `-30 → 603` and then holds at 603 for 2033 of 2577
samples in state `0xC`; `cam_x` tops out at 298; `roomx=[0,1600] col=0` for the whole run; 0 latch, 0
`fn_03d244` calls, 0 `DAT_6336c` events.  `/tmp/rig/wk2` (`ep1_walkfar2.txt`, the same plus 346 attack presses):
identical — `camx` pinned at 298, 10 records pass the gates (types 31 and 36 at `rx` 134-602 ✓ i.e. near the
player), 96 death-walk events, 0 latch, 0 chain advance.  The player never reaches the room's exit rect, so
request 2 (the exit-rect arm, `0x00012eb2`/`0x00012ed3` → `0x3ac2a-0x3ac76`) is never set and the chain never
advances to the room whose window contains `rx=4061`.

### Verdict

No gate is missing in the port: the request setters, the latch, the quota index and the fall-through shape of
arms 2/4 are byte-faithful ✓ the boss record exists in the shipped's stage-2 data ✓ and the boss's spawn is
gated by the camera/room window exactly as the shipped computes it.  The rig cannot produce the state the
report describes (it cannot walk the player out of room 1), so **item B is recorded as a report the rig does
not reproduce, not as a localized port defect**.  The one thing the rig can say for it: in every basis it can
build, the stage advances only because the rig itself poked `DAT_6336c = 3`, never because a boss died — the
latch witness recorded 0 latch events across `st2sp`, `st2rb`, `st2b1`, `wk1` and `wk2`.

The open question for the playtester: in real play, does walking right end room 1 (exit rect → `fn_03d244` →
a new window), and does the boss then appear and latch?  That is the sequence the rig cannot script.

## 3. Item C — clearing stage 2 re-enters the same map with no enemies

### Two real port defects, both fixed

**(1) The stale room record** — `444c2aa`, the same fix as item A: red `/tmp/rig/st2c` + `/tmp/rig/st2d` (5
`fn_03d244` calls, all on `node=0x…62ec`, room never advances, `DAT_63354` changes 3 times in 1914 samples) →
green `/tmp/rig/st2e` (nodes `62ec → 6660 → 69d4 → 6d48`, `DAT_63354` distinct 5).

**(2) The null chain link** — `b5355d3`.  At the chain's last node `+0x34c` is 0 and both clear arms read
through it (`0x3ac54` / `0x3ac8b` then `0x3ac5a` / `0x3ac91` = `[0 + 0x11]`).  On DOS linear-0 is mapped; on
the host it faults.  Red: `/tmp/rig/st2f` (700 s, 42000 frames) SEGFAULTS, `rc=139`, at frame ~8951 just after
the third room advance; the ASan build on the same drive (`/tmp/rig/st2asan`) reports `SEGV ... address 0x11`.
Green: `/tmp/rig/st2g`, 16000 frames, `rc=0`, 0 fault reports, and the chain witness walks
`2ec → 6660 → 9d4 → d48 → (link 0 -> g_dat_63350) → 2ec`, with `DAT_63398` taking 1 from the linked node's
`+0x11` (`[room-chain] fn_03d244 node=0x…6660 next=0x…69d4 ... DAT_63398=1 DAT_639e0=1`).  The port models the
mapped zero page as `room_link_or_zero()` → `sys_linear0()` (the pattern already used for the null command
stream at `f.c:2334`), i.e. the shipped's behaviour, not a new guard.

### The re-entry itself is the shipped's chain terminator

```
0x0003d286  MOV EAX,[ebp-4]                ; the current room node
0x0003d289  CMP dword [EAX+0x34c], 0
0x0003d290  JE 0x3d2a2
0x0003d292-0x3d29b  DAT_00063354 = [node+0x34c]
0x0003d2a2-0x0003d2a7  DAT_00063354 = DAT_00063350      ; the chain head -- the loop-back
0x0003d2ac  MOV EAX,1; CALL 0x38f3d                     ; the level-up screen
0x0003d2bb  CALL 0x3cad3                                ; install the new node's room setup
```

Port: `f.c` `fn_03d244` with the same branch, and the `NIGHT_ROOMDBG=1` witness names both arms
(`-> next node` vs `| g_dat_63350 (terminator)`).  So after the last node of a stage's chain the *same* map is
installed again, and `fn_03a829`'s loop continues on it — "clearing re-enters the same map" is the shipped's
mechanism ✓ not a port defect.

### The stage tail, witnessed (which stage's files get asked for)

`/tmp/rig/s2full2` (`ep1_roomclear4.txt`, DS-window census, 1044 samples):

```
DAT_00063370 (episode): 1 -> 2 at frame 5670, 2 -> 1 at frame 22650
DAT_000639d8 (room count of the chain): 0 -> 4 at frame 2880, never changes again
DAT_0006336c (request): 0 -> 3 at frame 5550 (the drive's poke), -> 0 at frame 14340, never 1 or 2
DAT_00063398 (orientation): 0 in all samples        (arms 2/4 never ran on this drive)
DAT_63354 == DAT_63350 == 0x18d6ec in 948 of 1044 samples   (the room node IS the chain head)
DAT_639e0 (scroll row): 0 in all samples; DAT_639bc (kills): 0 in all samples
lookup stream: 2s1.pcx, 2s2.pcx, night2.chr x4, st2s.spr x2, stage2.pcx, stage2.stg, stage2.wal
               then 1s1.pcx x2, 1s2.pcx x2, 1s3.pcx x2, night1.chr x3, st1s.spr x4, stage1.pcx x2
```

The tail does load stage 2 (the same list appears on the warp basis, `/tmp/rig/st2w`: `stage2.stg`,
`stage2.wal`, `night2.chr`, `st2s.spr` x2, `2s1.pcx`, `2s2.pcx` with `[room-chain] ... stage=2` and
`stage=2 node#0 counter=1/4`, `node#1 counter=2/4` ✓).  What then returns the episode counter to 1 is the
shipped's own reset arms, modelled at `f.c:28331` (`0x03b578`, game over), `f.c:28372` (`0x03b3f1`) and
`f.c:28359` (`0x03b52b`, the increment) — with `fn_038445` (the CONTINUE / game-over loop, `0x038445`) reached
from `f.c:23947` (`0x0003aecf-0x0003aee1`).  The re-requested `stage1.*`/`night1.chr`/`st1s.spr` after the
stage-2 files are that path re-installing episode 1 ✓ not the port losing a stage increment.

The "no enemies" half follows from §2: with `DAT_63354` equal to the chain head and `cam_x` back at 0-298,
the spawn records of the stage's data fail `room[0x00] <= rx <= room[0x08]` (`0x3e39f-0x3e50f`) and nothing is
constructed — 10 spawn passes in `wk2` out of 2400+ dispatches ✓ 0 in `s2full2`.

### Verdict

Item C splits: two port defects, both localized to shipped offsets and both fixed with witnessed red/green
pairs (`444c2aa` ✓ `b5355d3` ✓); and the re-entry-with-no-enemies shape, which the shipped produces through
`0x3d289-0x3d2a7` + `0x3b578`/`0x3b3f1` and the spawn gates ✓ faithfully reproduced.

## 4. What the rig could not reach (limits, stated)

1. **Item A inside stage 2**: every stage-2 basis the rig has keeps `DAT_639e0` at 0 ✓ so an unwanted vertical
   scroll inside stage 2 cannot be shown or excluded there.
2. **Item B's boss fight**: the rig cannot walk the player out of room 1 (`wk1`: player stops at X=603 with
   `cam_x` at 298 for 2033/2577 samples ✓ `wk2` identical with attacks), so it never satisfies the boss's
   spawn window and never produces a latch.  0 latch events across five runs.
3. **No fresh shipped capture**: the dosbox-automation container's engine (OrbStack) is unresponsive, so all
   shipped-side comparisons in this document come from cached dumps (`/tmp/rig/rb15` for the canonical route)
   or from static bytes (capstone over NIGHT.EXE, `scripts/audit/disasm.py`).
4. **Item B/C runtime confirmation belongs to the playtester** for the parts above, on the current HEAD
   (`b5355d3` + the witnesses described here).

## 5. Sweep (same build as the fixes)

_(filled in below once `/tmp/rig/sweep312` completes)_
