# 317. Item E measured on a legitimate stage-3 opening: one press advances one card; the CG dialogue path is a different routine, and the rig cannot arm it

> **RETRACTED as a basis for item E (2026-11-04, docs/322 / docs/323).**  This document measures the
> **title-path slide show** (`fn_00035dfc`'s card gates, `g_tab_52f2c[stage]` card counts, `<stage>s<card>.pcx`),
> not what the playtester reported.  Item E is an **overlay drawn over the live stage in the last room of a map**
> (`fn_03a70e`'s `na<stage>####.pcx` composite at 0x3a764-0x3a777, whose `fn_03a594` copy leaves the previous
> screen standing wherever the new picture is 0xff), armed only when the guardian record is inside the camera
> window (0x3aa4c-0x3aa8f).  Two basis errors compound here: (1) `--warp-episode 3` was read as "episode 1
> stage 3" when `DAT_00063370` is the **stage/map** counter, so this run measured **map 3**; and (2) a "room" is
> a chain node selected by **column** (`node[+0x40]`), not a chain ordinal, so no measurement in this document
> sits in the reported room.  The card-gate numbers (6 gates, each ended by a press) remain true of the slide
> show and are quoted nowhere as evidence about item E.  Item E was re-attacked and fixed on the corrected basis
> in docs/323 (localisation, red) and docs/324 (fix, green).  Its "no defect found" verdict is void.

Goal task-9 (round-4 item E: "stage 3's first-time boss dialogue — one Enter press skips the whole dialogue
instead of advancing it line by line").  Continues docs/313 / docs/314 / docs/315 / docs/316.

## 1. There are two "dialogue" mechanisms, and the shipped bytes name both

`scripts/audit/call_xrefs.py` over the whole code span (4503 resolved direct calls in 0x10000..0x4a000):

| routine | what it shows | its key gate |
|---|---|---|
| `fn_03af18` tail, label `LAB_0003b05a` (0x3b05a) | `"%ds%d.pcx"` cards, count `g_tab_52f2c[DAT_63370]` (stage 1: 3, **stage 3: 6**, stage 10: 0x16) | `CALL 0x00035dfc` at **0x3b0f1**, also 0x3b26d and 0x3b4dc (the `"%de%d.pcx"` end-slide copies) |
| `fn_03a70e` (0x3a70e, called from `fn_03a829` at **0x3aa8f**, once per room) | `na%d%04d.pcx` CG strip, count `g_dat_52f08[DAT_63370]` (stage 3: 6) | its own inline gates: press 0x3a785-0x3a7a4, release 0x3a7cd-0x3a7df |

`fn_00035dfc` (the routine item E's contract names) has exactly three call sites in the whole binary, all inside
`fn_03af18`; `fn_00035d5d` has four (`fn_010c3e` 0x10ca2, `fn_038f3d` 0x391d6 / 0x39491 / 0x3964a).  So a
"dialogue advanced by Enter" is one of these two loops, and both are modelled.

`fn_03a70e` re-read store-for-store against 0x3a70e-0x3a824 and the port matches every store and both gates:
`mov [0x2ff8],0` (0x3a73d, keyboard pump off ✓ `f.c`), `sprintf("na%d%04d.pcx", DAT_63370, idx+1)` (0x3a744-59 ✓),
`fn_03a594(0xaa8c0, DAT_633b0+0xa8c0, 0x5140)` (0x3a761-77 ✓), `fn_036041` (0x3a780 ✓), the press chain
0x3a785 `cmp [0x13ec1],0 / jne 0x3a797` … 0x3a7a4 `jmp 0x3a785` (✓), the abort test 0x3a7a6 `cmp [0x13e89],0 /
jne 0x3a7ff` (✓), `if (idx != count) fn_04185e(DAT_53dd8)` (0x3a7af-0x3a7c8 ✓), the release chain
0x3a7cd-0x3a7df (✓), `cmp idx, [DAT_52f08 + stage*4] / jl 0x3a744` (0x3a7f3-0x3a7f9 ✓), the reveal wipe
`fn_03a5f7(DAT_633b0)` (0x3a804 ✓) and `mov [0x2ff8],1` (0x3a819 ✓).  **Neither gate clears a latch**, and the
press gate accepts *whatever is already in* `DAT_63ec1`/`DAT_63ea4` — so "one press consumes the whole strip"
is only possible if something re-sets a latch between cards.

## 2. A stage-3 basis exists, and on it one press advances one card

`--warp-episode 3` writes `DAT_00063370`, which is the shipped's own stage counter, so this is the shipped's
**stage 3** (the run's records prove it: the constructed kinds/types are 71/72/76, which are stage 3's spawn
types, not stage 1's 11/31/32/36/37/41/42/46).  Skip-intro ON, pinned env
(`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), drives
`scripts/rig/drives/ep1_stage3_onepress.txt` and `ep1_stage3_tap.txt`, witnesses `NIGHT_SLIDEDBG=1`.

`/tmp/rig/e318g` (one Enter pair after the stage is up):

```
slide: card 1/6 wait pre-latch enter=0 space=0 punch=0 kick=0 esc=0 pump=1
keywait: press arm taken at spin 6441 (enter=1 space=0 punch=0 kick=0)
slide: card 1 gate returned 0 post-latch enter=0 ... spins=6441 edges=0
slide: card 2/6 wait pre-latch enter=0 ...
keywait: press arm taken at spin 6464 (enter=1 ...)
slide: card 2 gate returned 0 ... spins=12905 edges=0
... card 3 spins=15379, card 4 spins=19566, card 5 spins=99077   (card 6 never returned)
```

`/tmp/rig/e318h` (60 ms tap, no `--rig-dump`): the same shape — five gates each exited through the press arm
with `enter=1` after 1616 / 1207 / 3308 … spins, and the scripted press count in that run's log is
`script: key 0x1c down` × 8 / `up` × 8.

The two numbers the contract asks for are therefore **one card per press on both runs**: no card entered its
gate with a latch already held (`gate-with-no-press` census = 0 ✓ the `slide: card N gate held a latch with NO
press` line never printed), and no press-latch rising edge was observed inside a gate without a press
(`edges=0` in every gate ✓ a latch never pulsed by itself).  Each card's wait consumed thousands of spins
before a make arrived, which is what a real gate waiting for a real key looks like.

**Item E's symptom did not reproduce on this basis.**  The card count matched the shipped table (6 ✓) and the
key behaviour was one-press-one-card, so the rig found no red and no fix was made.

## 3. Why the *other* dialogue (the CG strip) cannot be measured at stage 3 yet

`fn_03a70e` is armed from `fn_03a829`'s per-room test at 0x3aa3b-0x3aa9a: it needs `DAT_00053c80` non-null with
the record inside the camera band (`camY <= rec+9 < camY+200` and `camX+0x32 <= rec+1 <= camX+0x10e`), and
`DAT_00053c80` is written by exactly one shipped site — `fn_010efc` at 0x10f1d-0x10f2b:
`MOV EAX,[EBP+0x10]` (the type index the constructor stores at `+0x0d`) `CMP EAX,[0x13370]` /
`JNE 0x10f30` / `MOV [0x3c80],EAX`.  So the latch arms only when a record whose **type index equals the stage
counter** is constructed.

Measured on the stage-3 basis (`NIGHT_STGDBG=1`, `/tmp/rig/e318a`):

```
stgdbg stage=3 records=128 tag1: 3:1 71:23 72:13 76:19 77:6 81:6 83:8
stgboss stage=3 rec=0030c71c sub=1 x=6403 y=186 zone=0 posmode=1
latch: <none>      na*.pcx: 0      show: <none>      (114 samples at memstat stage=3 room=1)
```

So stage 3 does declare exactly one type-3 record — the boss the latch mechanism needs — at `x=6403 y=186`, and
in the rig's stage-3 window no record of that type is ever constructed, so the latch stays null and the CG strip
never arms.  The same census at the shipped side, on the canonical 150 s episode-1 route
(`/tmp/rig/rb15/orig/state_*.ds.bin`, flat 0x16000-byte DS windows, 412 samples at 4 Hz):
`DAT_00053c80 = 0` in **412/412** samples, while the port's own dump on the same drive holds 0 in 1500/1500
(10 Hz).  Cadences differ, so the quotable form is the predicate, not a percentage: *neither side ever held a
latched record in stage 1's first window*.  That is agreement, not divergence.

Two further facts about the arming window, both from bytes: `DAT_639e0` (camY) only ever *increments* in the
shipped (`imm_scan --imm 0x139e0`: one `mov` at 0x3cb1f and two `inc` at 0x3cd42 / 0x3cdfc, no `dec`), and stage
3's room installs `camY = 200` (docs/314 §2 band census: stage 3 `320..395 camY=200` ✓ `fn_03cad3` 0x3cb1f ✓).
A boss record at `y = 186` can never satisfy `camY <= 186` once camY is 200 and cannot come back down.  Getting
the CG strip on screen therefore means reaching the boss's room **before** its camera band is 200 — which the rig
has not yet produced.

## 4. Verdict and what is owed

* Item E on the `%ds%d.pcx` basis: **measured, one press = one card, no defect found, no fix made.**  Recorded
  as "the rig could not reproduce the reported symptom", *not* as "there is no defect" — the report may describe
  the CG strip (`na30001..na30006.pcx`) rather than the cards, and the rig cannot yet arm that path.
* Owed from the playtester: the specific stage-3 run behind the report — whether the skipped dialogue was the
  six-card opening (cards change wholesale on one press) or the CG strip (a picture strip with a text line per
  picture), and whether Enter was tapped or held.  A held Enter legitimately advances several cards, because the
  shipped's own key map credits a re-make while the key-state slot is still armed (`fn_0347e3` family,
  0x3fdf2-0x3fe33, modelled at `port/src/app/keymap.c:66-79` with `g_dat_62cd8` as the repeat credit) — docs/281.
* If the CG strip is the thing: the reachable defect is upstream of it — the stage-3 type-3 record is declared
  but never admitted, which is the same gate that produced round-2 item B (a stage whose boss never exists).

## 5. Witnesses this item added (all in the committed binary)

* `fn_03a70e` (`NIGHT_SHOWDBG`): show entry with stage/count/latches, per-picture pre-gate latch values,
  `press gate satisfied with NO press` when a picture enters its gate with a latch already held, which byte
  ended the wait, and an exit census `shows_run / pictures / press_gate_with_no_press`.
* `fn_00035dfc` (`NIGHT_SLIDEDBG`): per-card pre-gate latch values + pump state, per-card gate return value and
  post-gate latches, the gate's own spin count, a rising-edge counter for the four press latches with
  `DAT_00062c58[0x1c]` alongside, and exit censuses `cards / gate-with-no-press` and
  `rising edges seen inside the gate`.
* `fn_03272f` invocation counter (`g_act_resolver_calls`, exposed as `f_act_resolver_calls()`) and the drive
  predicate `wait state resolver` (docs/316 §4), which is what makes a key window land in a frame the player
  action resolver owns instead of inside a menu or a card wait.
* `scripts/rig/drives/ep1_stage3_slides.txt` (Enter per card, death arm `play 0x30 1`),
  `ep1_stage3_onepress.txt`, `ep1_stage3_tap.txt`, `ep1_cgshow.txt`.
