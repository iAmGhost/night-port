# docs/308 — the 2026-11-02 verdict

The 2026-11-02 playtest (`docs/manual-playtest/04-2026-11-02.md`) reported five things. Two of them
were real defects in the port, one was a defect in my own earlier fix, one was a harness-semantics gap
rather than a game defect, and one was a real defect that had already been fixed but only for one of the
two things it was made of. This document is the closing verdict: per item, what the rig saw blind, what
was actually wrong, what the fix was, and the before/after numbers with the run path behind each number.
Then the regression table for the older six items, the sweep numbers, and an honest open list.

---

## 1. Item 1 — the music changes, but late

**Reported:** "music now changes, but with a bit of delay."

**Instrument built for it:** `NIGHT_CDDADBG=1` in `port/src/app/main.c` writes `port/cdda_pipe.jsonl`
(play / stop / flush / first_feed markers plus a 40 ms pipe sample carrying
`SDL_GetAudioStreamAvailable`, pushed/pulled sectors, stream position and the budget), and
`scripts/rig/cdda_delay.py` turns that into queued-ahead bytes → seconds of audio sitting in front of the
device, feed and drain rates against the real-time rate of 75 sectors/s, frame coupling, and underrun
samples.

**Red (before, `docs/307`):** `/tmp/rig/mt1`, `/tmp/rig/mt2` — switch delays **4.17 s to 9.96 s**, peak
queue **3.75 MB = 19.5 s of old-track audio**, reader feed 1.10–1.11× real time while the device drained
at 0.95×, and only **1 of 4** track switches flushed the queue at all.

**Two defects, both in the reader, not in the schedule:**
1. the reader's read-ahead was bounded by wall time only, never by what the device had consumed, so it
   built a queue of the *previous* track in front of every new track;
2. the flush on a track switch was conditional on `g_cdda_sectors != 0`, so some switches never flushed.

`CDDA_CUSHION_SECTORS 30` is now both the allowance and the queue ceiling (measured against
`SDL_GetAudioStreamAvailable` through `cdda_dev_bytes_per_sector()`), and the flush is unconditional.

**Green (after):** `/tmp/rig/mt4`, `/tmp/rig/mt5` — switch delays **0.35 s to 0.44 s**, peak queue
**114 632 / 115 656 B = 0.60 s**, feed 1.04×, drain 1.03×, **0 underruns** of 2 775 / 2 762 pipe samples,
3 of 4 switches flushed.

**Re-measured on HEAD this session** (`/tmp/rig/mw1`, canonical `ep1_kill3` drive, 9 000 frames,
pinned env): chain `index 1 @ 7.87 s → 9 @ 38.12 s → 11 @ 40.93 s → 2 @ 60.32 s`; delays
**0.00 / 0.41 / 0.42 / 0.37 s**; peak **114 632 B = 0.60 s**; pushed 11 079 sectors at 77.98 sectors/s
(1.04×), drained 11 043 at 77.73 sectors/s (1.04×); **0 underruns of 2 779 pipe samples** — identical to
mt4/mt5, so the schedule and rate gates held through the later changes.

A second drive (`/tmp/rig/tm2`, `ep1_title_music`, 12 000 frames) shows four switches (1 @ 9.5 s,
9 @ 64.0 s, 1 @ 107.9 s, 9 @ 193.0 s) all at **delay 0.00 s** and the same 0.60 s ceiling, but it feeds at
0.67× real time and shows **1 150 of 3 547 pipe samples with an empty device buffer while a track was
live**. That is recorded as open (section 6), not as a pass: on this basis nothing proves an audible
dropout, and the shipped side cannot confirm or deny it.

**Gates kept green:** the track chain is identical in every run; the stream is byte-identical to the disc
(`scripts/rig/disc_search.py`, NCC **+1.000** against `TG_NIGHT.IMG` at the requested LBAs — `docs/304`).
Content comparison against the shipped side is **not gateable at all** on this harness: the emulator's own
capture contains no CD-DA (`docs/304`), and `TG_NIGHT.CUE`'s single `FILE` block does supply the audio
tracks through their INDEX offsets (TRACK 02 index 08:14:62 = IMG sector 37 112 = disc LBA 37 262, the
sector the shipped actually requests).

## 2. Item 2 — one attack make landing several hits on the same enemy

**Reported:** one attack make lands several hits on the same enemy at once.

**Oracle built for it:** `scripts/rig/hits_per_attack.py`, dump-side. An *attack make* is a transition of
the player's state field `+0x17` into {0x33, 0x34, 0x35, 0x36}; a *hit* is a fall of the victim's
`+0x23` (the pending-HP term, read as a signed 16-bit word because that is how the shipped and the port's
own damage witness read it) inside that window. Victim identity is `(record offset, kind, span)`, which is
what makes the count survive slot reuse.

**Red, and its cause:** the count of hits per (make, victim) on the then-HEAD build —
`/tmp/rig/h306/head1`: 99 makes, 4 hit events, **1 make hitting one victim twice**
(`o=1632236`, debt 28→20→6 over ticks 901–903);
`/tmp/rig/h306/head2`: 114 makes, 7 hit events, **1 make hitting one victim three times**
(`o=1631896`, debt 35→23→9→2 over ticks 980–982).
The pre-`docs/263` baseline (`/tmp/base263` at `e4a1f4e`, same instrumentation) gives
`base1`: 44 makes / 11 events / **max 1 per victim**, `base2`: 47 makes / 5 events / **max 1** — so
`docs/263` is what opened the path (`docs/306`).

`docs/263` was wrong about the shipped's jump structure: it read `0x14685 jg 0x1469f` as an accept branch,
but the shipped's reject-to-next-row path is the `jmp 0x146a9`, and the row-selection test in
`fn_01441e` is a window test —

```
w1 <= frame && w2 > frame          (accept row i)
```

not "either threshold". Disassembly: `scripts/audit/disasm.py --entry 0x14640 --end 0x14710`.

**Fix** (`df831be`): `port/src/f/f.c` restores the window test. Fixture in `port/test/test_f1.c` case (2b)
— frame 3 with row 0 = (20, 30) in the future and row 1 = (0, 10) live must select **row 1**: red on the
unfixed line (it was the only FAIL), green after (`== ALL PASS ==`).

**What the fix actually gates, stated precisely.** After the fix, a make can still land more than one hit
on one victim, and that is not a defect: a make passes through several attack states, and the shipped's own
dedupe latch is keyed on state (`0x14706` / `0x14716`), so one make legitimately carries several hits as
the animation advances. The oracle therefore also reports the **latch-key census** over
`(make, victim, player state)`:

| run | makes | hit events | pairs | multi-hit makes | max/victim | latch-key distribution | latch misses |
|---|---|---|---|---|---|---|---|
| `head1` (before fix) | 99 | 4 | 3 | 1 | 2 | `[(1,4)]` | 0 |
| `head2` (before fix) | 114 | 7 | 5 | 1 | **3** | `[(1,5),(2,1)]` | **1** (`state 0x35`, `23→9`, `9→2` in ticks 981–982) |
| `base1` (pre-docs/263) | 44 | 11 | 11 | 0 | 1 | `[(1,11)]` | 0 |
| `base2` (pre-docs/263) | 47 | 5 | 5 | 0 | 1 | `[(1,5)]` | 0 |
| `fix1` (after fix) | 98 | 15 | 12 | 2 | 2 | `[(1,15)]` | 0 |
| `fix2` (after fix) | 118 | 9 | 8 | 1 | 2 | `[(1,9)]` | 0 |

So the defect that existed was *two applications of damage inside a single latch key* (one victim, one
attack state, consecutive ticks) — that is what `docs/263`'s relaxation produced and what the fixture
catches — and after the fix **no latch key ever yields more than one hit** (`fix1`: 15 keys, all 1;
`fix2`: 9 keys, all 1), which is the shipped's behaviour. The residual multi-hit makes
(`fix1` 2, `fix2` 1) sit on distinct states and are legitimate under `0x14706/0x14716`. A shipped-side
latch census would need the entity records on the shipped dump and does not exist yet (section 6).

## 3. Item 3 — walking straight down was broken

Reported on 2026-11-02 and blind-detected the same day (`docs/300`): the player could not walk down.

The defect was **mine**: `docs/291`'s entry-guard fix for `LAB_0003284e` removed the shipped's
`cur < mn` test from the unclamped branch of `fn_01586b`, which is the *vertical* mover (Up → `−step` at
`0x328c5`, Down → `+step` at `0x328dd`). `docs/300` records the axis confusion in several port comments
and the rule that came from it: before "fixing" a guard by removing a test the shipped performs, disassemble
the guard's **first** instruction, not the one that matches the symptom you are chasing; and a red/green
pair that *reduces* the amount of movement measured is a warning sign, not a pass — `docs/292`'s own
red/green pair (`{172:580, 195:91}` → `{172:912}`) was measuring the damage my fix did.

Red `/tmp/rig/do1`: Y distribution `{172: 912}`, no downward movement at all. Green `/tmp/rig/do4`.
Re-measured on HEAD this session (`/tmp/rig/do6`, `ep1_downonly`, 10 500 frames, 887 player samples):
**Y = 195 in 660 samples**, 172 in 225 — the player reaches and holds the room's lower bound ✓

## 4. Item 4 — `--warp-stage 2` means episode 2

Not a game defect: a harness-semantics gap. `fn_037b03`'s warp cell is **DAT_00063370**, Ghidra's
`g_dwEpisodeCounter` (1..9). It is the number every per-stage name in the loader is built from
(`stage%d.wal`, `st%ds.spr`, `stage%d.stg`, `night%d.chr`) and what the spawn and clamp tables index with
(`room_index + (counter−1)*10` at `f.c:2362`, `g_tab_52c3c` at `f.c:5569 / 5653 / 5693`). The shipped
calls that counter "stage", so `--warp-stage 2` could only ever mean episode 2 — "episode 1, stage 2" was
not expressible (`docs/305` §4 open item).

**Split (`27e763e`):** `--warp-episode N` sets `DAT_00063370` (the old behaviour, correctly named);
`--warp-stage M` selects the M-th stage node *of that episode* after the room chain exists, by walking the
shipped's own next-room link (`node+0x34c`, set at `0x3caa2`, `docs/90`) and installing the node exactly as
`fn_03cc6b` does when the game clears a room (`g_dat_63354 = c` at `0x3ccb5 / 0x3cd6f`, mirrored to
`g_dat_63358` at `0x3cccf / 0x3cd89`, which the room loop prefers over the chain head at
`0x377e3-37802`), then re-arming camera and band with `fn_03cad3` as the loader does for the head at
`0x37bfe`. Nothing is skipped in the data: the chain is read whole, only the current room is chosen.
Call sites that meant the episode were renamed (`scripts/rig/fuzz.py` ×3, `asan_soak.sh`,
`asan_o2_sweep.sh`, `playtest_asset_census.py` ×3, `playtest_ep8_show.py`).

Verified from dumped stage/room cells (`ep1_kill3` drive, 3 600 frames, `--rig-hz 10`, pinned env,
`/tmp/rig/w308`):

| run | witness | DS 63370 | DS 639dc (camX) | DS 639e4 (band) | DS 639d8 (rooms) |
|---|---|---|---|---|---|
| `--warp-episode 1 --warp-stage 1` | none (stage 1 = the head) | 1 in 600/600 | 0 in 600/600 | 0 | 4 |
| `--warp-episode 1 --warp-stage 2` | `episode=1 stage=2 node#1 hops=1 rooms=4 camX=3280 camY=0 band=1` | **1 in 600/600** | **3280 in 179** | **1 in 179** | 4 |
| `--warp-episode 2 --warp-stage 2` | `episode=2 stage=2 node#1 hops=1 rooms=4 camX=2000 camY=0 band=0` | reaches 2 (173) | 2000 in 173 | 0 | 4 |

The stage-2 basis is unchanged: the **natural** room-clear chain on HEAD (`/tmp/rig/w308/natural`,
`ep1_roomclear3`, 18 000 frames, no warp) gives `DS 63370 = 1` in 958 samples then `2` in 2 029,
`DS 639d8 = 4`, and 2 484 player samples — the port still reaches the shipped's stage 2 by itself, and its
stage-2 rooms carry ink (603 witness frames, 87 sampled: **0 near-black**, mean 0.915 non-zero byte
fraction, min 0.251).

## 5. Item 5 — a sprite fragment drawn at the right edge

A real defect, and it was the second half of the older item 6 that `docs/298` had wrongly closed as
"not producible" (`docs/301` records that blind spot). `docs/303`: `fn_03bb42`'s mirrored
width-overflow clip anchored its clipped run at `db − col + 1`, while the shipped
(`0x3be55-0x3be58`) uses `db − col_acc` and shortens only the count; `cpy_back` (`0x040475`) copies
**downward** from its first argument, so with `dest_off < 0` the tail landed on the previous row's
columns 319, 318, … — a fragment at the right edge of the row above. Guard aligned to the shipped's
`cmp 0x140 / jg` at `0x3bb6a` and `0x3bee3`. `fn_03bebb` and `fn_03c231` contain the same-looking
`db − col + 1` and are **correct**: their helper `0x41e20` fills forward.

Witness and numbers: `NIGHT_BLITEDGE=1` logs every write whose byte span crosses a 320-byte row boundary,
labelled 1–6 by call site. Before: `/tmp/rig/le12` **486 crossings, site 5 only**. After: `/tmp/rig/le14`
**0 crossings** with the phase matched (556 samples at X ∈ {15,16,17} with mirror = 1). Re-measured on
HEAD this session: `/tmp/rig/le16` (`ep1_leftedge4`, 13 200 frames, 1 105 player samples) — **0
crossings**, phase matched again (X = 15 in 768 samples, mirror = 1 in 447 samples) ✓

---

## 6. Regression table — the six 2026-10-29 items on HEAD

Each row names the gate, the run path, and the number as measured this session.

| item | gate on HEAD | run path | number |
|---|---|---|---|
| 1 intro→title music switches | track chain + request-to-audible delay + stream rate + disc-byte identity | `/tmp/rig/mw1`, `/tmp/rig/tm2` | chain 1→9→11→2 identical to `mt4/mt5`; delays 0.00/0.41/0.42/0.37 s; queue ceiling 0.60 s; feed 1.04×, drain 1.04×; 0 underruns of 2 779 |
| 2a EXP text garbled | label-blit footprint witness | `/tmp/rig/gw2` (`NIGHT_GLYPHDBG=1`) | `valuedisp_tail font=A count=11 arraysize=0x2c used[+0x28]=**idx10** … neighbour[+0x2c]=idx11` — the code reads index 10, the shipped's index (`docs/221`→`docs/227`) |
| 2b EXP per enemy type | player-side EXP chain instrument (`docs/297`) | `/tmp/rig/gw2` | `+0x2b` (the HUD number) 0 → 20 in 951 samples, `+0x52` accumulator max 9; the award value is the first dword of the shipped's type-table row (`docs/284`) |
| 3 upgrade menu lights every item | `NIGHT_LUMENU=1` row census + lit-without-purchase classification | `/tmp/rig/lu9` (`ep1_lumenu4.txt`, warp at frame 6 200) | **10 326 row visits** (1 721 per row × 6); rows 0 and 1 lit 3 290 visits (the two the drive purchases), rows 2–5 dark 6 220; **816 legitimate cursor previews; 0 rows lit without their own purchase**. `docs/228`'s red shape (rows 4 and 5 lit unpurchased) does not appear |
| 4 level-up intro skips a keypress | card-wait witness | `/tmp/rig/kw4` (`NIGHT_SLIDEDBG=1`) | `slide: card 1/3 wait`, `2/3 wait`, `3/3 wait` — every card waits for a press (`fn_00035dfc`, `docs/231`) |
| 5 stage-2 crash + black floor | fault oracle + natural chain + ink presence | `/tmp/rig/soak309`, `/tmp/rig/w308/natural` | soak 9/9 stages `rc=0 ubsan=0 asan=0 segv=0` at 4 000 frames; chain reaches episode 2 (2 029 samples); 0 near-black frames of 87 sampled |
| 6 limb drawn on the wrong side at the left edge | row-crossing witness on a phase-matched left-edge run | `/tmp/rig/le16` | **0 crossings** (was 486 in `/tmp/rig/le12`, site 5 only) |
| docs/300 straight-down walking | down-only drive reaches the room's lower bound | `/tmp/rig/do6` | Y = 195 in 660 of 887 samples (red `do1` was `{172: 912}`) |

## 7. Sweeps on HEAD

* `make -C port test` → `== ALL PASS ==` (the target rebuilds `port/test/night`, `docs/181` §5).
* Nine-episode ASan/UBSan soak `/tmp/rig/soak309` (`scripts/rig/asan_soak.sh`, which rebuilds the asan
  binary first, `docs/265` §2): stages 1–9, each `rc=0 ubsan=0 asan=0 segv=0`, 4 000 frames presented.
  This is also the run that proves the renamed `--warp-episode` flag works in the soak harness.
* Canonical route `/tmp/rig/rb16` (`scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150`) compared
  against the `/tmp/rig/rb15` baseline:
  * state tally **identical**: `SIZE? 34, PTR 52, SAME 70, REPR 19, DIFF-SIZE 14, DIFF 27` over 346 cells /
    274 phases;
  * activity profiles: **271 cells, `tally={'SAME': 271}`** in both runs;
  * audio: **275 `[SAME]` windows** in both, `divergent runs (score < 0.88): 8` in both, channel-order
    0.973 (rb15: 0.982), port whole-route onsets 3 = 3, `active_frac` 0.997 = 0.997;
  * pacing: shipped 2.67 picture changes/s (unchanged, same capture) vs port **20.18 → 16.51**, ratio
    **0.13 → 0.16**. Both runs presented 9 000 of 9 000 frames, so this is not a frame-drop artifact — it
    is the pacing side effect of the CD-DA pacing fixes (`docs/302` measured the same sign on the left-edge
    route, 0.54 → 1.36). The pacing residual remains the largest un-attributed difference in the rig.
* Coverage-guided fuzz census: `/tmp/rig/fuzz309` (9 episodes, 24 seeds) — **no sanitizer or SEGV
  witness** in any of its 24 `run.log` files, 23 runs `rc=0` and **1 run `rc=124`** (`r0s12_st4`, killed by
  the 120 s `--cap` while the machine was running the soak and a route concurrently); coverage
  **237/503** against `fuzz305`'s 254/503 on the same 24-seed basis. A deeper 48-seed census
  `/tmp/rig/fuzz310` is running; its number is quoted when it lands. Coverage under load is not
  comparable to coverage on an idle machine, so the drop is recorded as a measurement-basis difference,
  not a coverage regression — and not as a pass either.

## 8. The shipped side could not be re-captured, and what that changes

On 2026-11-03 the OrbStack/Docker engine stopped answering: `docker version`, `docker ps`,
`docker images`, `docker run --rm dbga:ready echo ok` and `scripts/rig/bringup.sh` all hang to their
timeouts (rc=124, empty stdout and stderr), the app itself is running (`OrbStack … vmgr`), and port 8386
is still forwarded but answers nothing (`http_code=000`). Since `dosbox-automation` in a container is the
only route that produces shipped frames on this machine (`docs/46`), no fresh shipped capture exists for
this goal turn.

`scripts/rig/route.sh` therefore gained `REUSE_ORIG=<previous route out dir>`: the shipped halves
(`orig/`, `orig_screen/`) are taken from an earlier run and the run writes `REUSE_ORIGIN.txt` saying so.
The shipped side is pinned (`cycles 120000`, `core = normal`, fixed drive, `docs/100`), so a cached
capture is a legitimate reference — but what a reused run proves is narrower than what a fresh one proves:
it measures **port change against a fixed shipped reference**, which is exactly what a regression gate
needs, and it re-verifies nothing about the shipped side itself. `rb16` is such a run, and every number in
section 7 that comes from it is quoted with that limitation.

## 9. Open list

1. **Pacing residual 0.13 → 0.16** on the canonical route, i.e. the port changes picture 6.2× more often
   per second than the shipped. It moved with the audio fixes and is only partially attributed.
2. **Item 1's content half cannot be gated on this harness** (`docs/304`): the shipped-side capture
   carries no CD-DA at all. The gates are schedule, stream rate, queue ceiling and disc-byte identity.
3. **The `/tmp/rig/tm2` underrun witness** (1 150 of 3 547 pipe samples with an empty device buffer while
   a track was live) is unexplained. The title drive is where the 0.60 s ceiling gets tightest; whether it
   is audible is not measured.
4. **No shipped-side latch census** for item 2: the hits-per-attack oracle reads the port's
   `records.jsonl`. A shipped-side equivalent would need the entity records (`RIG_ENTS=12` ent## cells do
   exist in route runs) turned into the same identity rule.
5. **Fuzz coverage** 237–254 of 503 shipped-named functions; the never-entered list (249–266 names) is
   untriaged.
6. **Item 2's audible garble** is closed only at the blit level (`docs/227`'s footprint probe, `valuedisp`
   index 10), not as a rendered-text comparison against the shipped (`docs/305` §2).
7. **g_tab_52b84 declaration width** stays at 90 bytes against a 360-byte read from a different declaration
   in `f.h` (`docs/270`), an unreconciled declaration rather than a fixed one.
8. The old item-5 "bus error at stage-2 entry" was never reproduced on any build (18 000 frames, `rc=0`);
   the earlier UBSan fault attributed to it was a startup misaligned `char *` store in `fn_04589e`,
   already fixed by `docs/265` (`docs/299` §1).

## 10. Retractions recorded along the way

Kept visible because each one cost a measurement that looked like a result:

- `docs/216` (cpy_dw direction reversed) — retracted by `docs/218`: the cdecl pushes were read backwards.
- `docs/215` §2 pixel-diff tables — void: the shipped frame payload stride is 2560 bytes/row, not 1280.
- `docs/196` §1 "the glyph witness printed nothing" — corrected by `docs/201` §4: `NIGHT_GLYPHDBG=1`
  witnesses exist and decode as well-formed 5×7 glyphs.
- `docs/186` admissibility gate excluding every enemy (HP at `+0x29` = 0 for enemies) and the
  `CellStream`-keyed census collapsing 12 `ent##` slots to 1 — corrected in `docs/186` §1 / `docs/188` §3.
- `docs/232` §4 "`state_*.ds.bin` is a per-cell stream, not a flat DS window" — superseded by `docs/264` §3:
  it is a flat 0x16000-byte DS window readable at `DS − 0x50000`.
- `docs/290` per-state facing shares (7% vs 22-32%) and "five handlers the shipped never touches" —
  retracted by `docs/291`: a 4 Hz vs 10 Hz cadence artifact, and `body_bytes` was 93 bytes short of
  `body_end`, hiding the shipped's flip tails.
- `docs/291` §2 "`0x3285f-0x3287c` is a je-chain that never tests `DAT_00063ed8`" — both halves wrong
  (`docs/300`): the chain starts at `0x32856` and its first test is `63ed8` (Down). My guard fix caused the
  straight-down bug.
- `docs/292`'s red/green pair for that guard fix — retracted by `docs/300`: it measured the movement my
  fix removed.
- `docs/296` "item 5's crash half blind-detected" — withdrawn (`docs/299` §1).
- `docs/298` "item 6 closed as not producible" — withdrawn (`docs/301`): the right-edge fragment is real
  and was fixed in `docs/303`.
- `docs/262`'s soak line number `f.c:7374` — a blank line in the source at the time, proving the soak had
  soaked a stale binary (`docs/265` §2); the soak now rebuilds first.
- `/tmp/rig/le13`'s "898 changed frames vs 5 311" — void run, never entered a room (`docs/303`).
- The audio lines quoting `corr = 0.22-0.32 [NOT FOUND]` — that is the instrument's floor against a capture
  that contains no CD-DA, not a port defect (`docs/304`).
- `docs/305`'s "all six blind-detected" — corrected to four detected and localized (`docs/295`), and item 1
  and item 2 have since been shown to have real defects (`docs/302`/`docs/307`, `docs/306`) measured on the
  unfixed build.

## 11. Rules these failures produced

1. **A drive is part of the measurement basis, and every wait's accepted keys are part of it.** The item-3
   census printed nothing on `ep1_lumenu2` because `fn_038f3d` opens with a card phase whose wait accepts
   only Enter/CTRL/ALT/Space (`docs/97`) and that drive's last Enter is at t = 88 s while the warp fires at
   frame 5 614 (~94 s): the call stalls in the card wait and the bar loop never runs. `ep1_lumenu4.txt`
   adds Enter pairs after the warp frame and produced 10 326 row visits. A zero-witness run is a drive bug
   until proven otherwise.
2. **Grep the witness arm before quoting a zero.** `NIGHT_SLIDEDBG`, not `NIGHT_SLIDEBG`; `lumenu frame=`,
   not `bar`. Two runs were spent before I read the `getenv` line.
3. **Classify before counting.** "Rows lit" is ambiguous: 816 of the 10 326 visits in `lu9` are legitimate
   cursor previews. The gate is *lit without its own purchase*, and it must be separated from the preview.
4. **Heavy runs do not share a machine.** Eight concurrent heavy processes cut the harness to ~12 fps and
   produced three runs (`le15`, `do5`, `gw1`) that presented 2 090 of 13 200 frames and produced **no
   dumps at all** — they looked like "no activity" findings. Re-run sequentially; check
   `frames presented` before believing a zero.
5. **`frames presented` is the load discriminator for pacing comparisons.** rb16 and rb15 both presented
   9 000/9 000, so the 20.18 → 16.51 shift is a real port change and not host contention.
6. **State which half of a comparison is cached.** `REUSE_ORIG` measures port change against a fixed
   shipped reference; it cannot re-verify the shipped side, and a report must say so in its own file.
7. **Name the gate the fix actually holds.** For item 2 that is the state-keyed latch census
   (`0x14706/0x14716`), not "no multi-hit makes" — a make that advances through several attack states is
   allowed to hit more than once, and claiming otherwise would have been an overclaim the shipped contradicts.
8. **A warp that lands before a drive's pokes measures a different state.** `NIGHT_SHOWSCREEN_AT=6200` sits
   after `play 0x27 1` / `play 0x28 2` and measures a menu with two purchased rows; `AT=2600` sits before
   the player exists and dereferences `g_player_ent` (`docs/288`: a rig artifact, not a port guard).
9. **Under load, coverage and rate numbers are not comparable across runs.** `fuzz309` (237/503, one seed
   killed by the 120 s cap) against `fuzz305` (254/503) is a basis difference first and a signal second.
