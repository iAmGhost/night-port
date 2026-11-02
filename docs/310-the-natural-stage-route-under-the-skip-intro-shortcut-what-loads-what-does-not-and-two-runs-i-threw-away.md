# docs/310 — the natural stage-1 → stage-2 route under the skip-intro shortcut: what loads, what does not, and two runs I had to throw away

Companion to docs/309 (the skip-intro shortcut, commit `0e79e44`). Prompt: the 2026-11-03 report
"stage 2 after clearing stage 1 shows a solid black background and crashes after a while, and stage 2 loads
the same WAL as stage 1", plus "try reproducing it now that the intro can be skipped".

## 1. The WAL observation is warp semantics, not a load bug

`DAT_00063370` is the **episode** counter (docs/308 §5), and the stage resources are named straight from it —
`f.c:5336-5343`:

```c
cur = (uint8_t *)&g_dat_63370;
fn_0428f8(name, (const uint8_t *)"st%ds.spr", &cur);             /* 0x37c03-13 */
...
cur = (uint8_t *)&g_dat_63370;
fn_0428f8(name, (const uint8_t *)"stage%d.stg", &cur);           /* 0x37c58-68 */
```

so `--warp-episode 1 --warp-stage 2` — *stage 2 of episode 1* — correctly asks for **`stage1.stg` / `st1s.spr` /
`night1.chr`** and walks the room chain to the 2nd node (`node+0x34c`, `room[+0x40] = 2` ✓ docs/308).
`--warp-episode 2` asks for `stage2.*` ✓ Nothing is mis-selected: the episode chooses the files, the stage
number chooses the room node. A playtest that "advances to stage 2" by warping is therefore looking at episode
1's art and should be ✓

## 2. Two runs of mine were windows that ended before the event they were measuring

I re-ran the stage-clear route (`scripts/rig/drives/ep1_roomclear3.txt`, `--rig-hz 5`, `--frames 12000`,
`NIGHT_RESLOG=1`), once with `NIGHT_SKIP_INTRO=1` and once without, and asked which stage resources appear
after the stage counter flips:

| run | flip witnessed | stage-2 lookups | frames presented |
|---|---|---|---|
| `/tmp/rig/s2res1` (skip on) | `memstat stage=2 room=1` at frame 5751 | **none** — `stage1.pcx/.stg/.wal` only | 12000, "game main unwound" |
| `/tmp/rig/s2res2` (skip off) | `memstat stage=2 room=1` at frame 6054 | **none** — `stage1.*` only | 12000, `rc = 0` |

I nearly wrote that up as "the port advances the stage counter and never loads the new stage". **It is wrong,
and it is a window artifact** ✓ Both runs stopped at the frame cap, while docs/286 §3 says this route's
next-stage loader completes near **frame 15951**. `--skip-intro` was not involved: the flag-on and flag-off
runs behave identically here (flip at frame 5751 vs 6054, both with no stage-2 lookup ✓).

That is docs/228's rule hitting me a second time: *the harness quits 2.5 s after the drive's last event, so a
drive must cover the whole measurement window.* A run that ends early does not show "nothing happened", it
shows "I was not there".

## 3. With the window covered, the natural tail loads stage 2 and faults nothing

New drive `scripts/rig/drives/ep1_roomclear4.txt` = `ep1_roomclear3.txt` + key events extended to 900 s
(Shift and Enter to clear the level-up screen and its slides, Right and Space in the stage-2 rooms), run
`--frames 40000 --rig-hz 2` under the pinned env with `NIGHT_SKIP_INTRO=1 NIGHT_RESLOG=1` (`/tmp/rig/s2full2`):

```
memstat stage=1 room=0 frames=55
memstat stage=2 room=1 frames=5701        (318 samples stay in stage 2 room 1 to the end of the run)
resource lookups after the flip:  2s1.pcx 1   2s2.pcx 1   stage2.pcx 1   stage2.wal 1
                                   stage2.stg 4   night2.chr 4   st2s.spr 2
faults: 0 AddressSanitizer / 0 UBSan runtime errors / 0 SIGSEGV / 0 heap-chain probe hits, frames presented 40000
```

So on this basis: **stage 2's own resources are requested** — `stage2.pcx`, `stage2.wal`, `stage2.stg`,
`night2.chr`, `st2s.spr` and the stage-2 start slides `2s1.pcx`/`2s2.pcx` ✓ — and the run lives for 40 000
frames (~340 s inside stage 2) with no fault ✓

**The reported crash is not reproduced.** That does not clear the port: it means the rig's route to stage 2
(a poked clear request, `poke 0x6336c 3`, docs/224) is not the route a player takes, and nothing in this run
looks like the reported black screen yet (§5).

## 4. What the natural arms actually are (why the rig's route is not a playthrough)

`DAT_0006336c` is the stage/scene event request (`f.c:682`), and `fn_03a829`'s arms (`f.c:23825-23835`) are:

| value | set by | meaning |
|---|---|---|
| 1 | `f.c:26744` (`e == g_player_ent`) | player died → the room loop breaks |
| 2 | `f.c:26883` | **exit rect**: `fn_00012dcc` (0x12e8f-0x12ef0) found the player inside the room's rect `g_dat_5d0a0[0..0x0c]` while a direction key was held (`g_dat_63ed5` right / `g_dat_63ed3` left), and it sets `DAT_63398` to the next node's `+0x11` in the same branch |
| 3 | `f.c:26745` (`e == g_latched_ent`) | the latched entity died — **the arm every rig route has used**, by poking it |
| 4 | `f.c:26754` (`quota <= g_dat_639bc`) | kill quota met |

Two corrections come out of reading this:

1. **`DAT_00063398` has writers.** docs/223 said it "is read once at 0x3ac6e and never written anywhere in
   NIGHT.EXE". It is written — in `fn_00012dcc`'s exit-rect arms (0x12eb2, 0x12ed3), and `fn_03a829`'s
   request-2 arm re-reads it from the next node's `+0x11` (`f.c:23830`). My shipped-side census missed it
   because I looked for a displacement pattern rather than every store to the cell — the same failure class as
   docs/264 §2 (`memset` via a call) and docs/254 §3 (accessor-macro stores) ✓
2. **The rig has never walked arms 2 or 4.** A natural playthrough clears a room by reaching its exit rect or
   meeting the kill quota; the rig pokes 3. So the rig's stage-2 evidence — docs/224, docs/285-287, and §3
   above — all rests on an arm a player does not use ✓ That is the gap the report sits in ✓

My first attempt at a natural route (`scripts/rig/drives/ep1_stage2_nat.txt`: `ep1_kill3`'s proven start
section, then long Right-holds with attacks, no pokes, `/tmp/rig/s2nat2`) never left **`stage=1 room=1`** in
230 s: holding Right did not find the exit rect, and the drive does not meet the kill quota. The exit rect's
coordinates come from the room record (`g_dat_5d0a0[0..0x0c]`), so a natural drive has to be built from those
coordinates rather than guessed as "hold Right" ✓ That is the next drive to write.

## 5. Not yet measured

The screen half of the report — "solid black background" — is still unmeasured on this run. My first census
attempt hand-rolled a `cells.bin` parser, got nonsense (episode values like 33 554 432 ✓ a stride error), and
I stopped rather than quote it (docs/289's rule: never hand-roll a dump parser ✓). The next pass should use
`scripts/rig/cells_reader.py` for the phase (samples with `DAT_63370 == 2`) and the existing screen census
scripts for ink per row band, all inside `/tmp/rig/s2full2` ✓

## 6. Rules this turn produced

1. **A run that ends before the event is not evidence about that event.** Compare the frame where the event is
   expected (docs/286: frame 15951) with the frame where the run stopped (12000 ✓) before writing a sentence
   about what "never happens". `--frames`, the harness's 2.5 s quit and the drive's last event are all part of
   the basis (docs/228 restated).
2. **Run the control for a new flag.** `--skip-intro` looked like the explanation for a missing lookup until
   the flag-off run showed the identical absence ✓ Every new switch needs its off-branch measured on the same
   route, not assumed neutral.
3. **A poke is a different arm.** Every rig route to stage 2 pokes `DAT_6336c = 3`. Arms 2 (exit rect) and 4
   (quota) are what a player triggers, and a bug can live in one arm and not the other ✓ State which arm a
   stage-2 claim was produced through.
4. **"No writer exists" needs a store census, not a pattern grep.** docs/223's claim about `DAT_00063398` was
   wrong for the third time in this project's history for the same reason ✓
5. **When a hand-rolled parser returns numbers that look wrong, it is the parser.** The garbage episode values
   above were my stride arithmetic, not the dump ✓ (docs/289 §4 restated.)
