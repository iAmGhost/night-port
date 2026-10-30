# 182 — The six 03-2026-10-29 items, restated as rig targets on a frozen build

The new playtest sheet has six items.  Before any of them is touched, this document fixes what the rig will treat as
"detected" for each one, on which route, and at what cadence -- so that a later green result means the same
measurement was taken on both sides.

## 1. Frozen pre-fix build

```
git rev (port sources)        574c636   2026-10-30 17:18:08 +0900
port/test/night               sha256 70e8b8cf8cdb11cadbebb1d8...  mtime Oct 30 17:07
built by                      `make -C port test` (which now also builds the app binary, docs/181 §5)
port env default              SDL_AUDIODRIVER=dummy NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60
```

The only later commit is `34e8697`, which changes `scripts/rig/capture_original.py` (rig side, not port code), so the
binary above still describes the port under test.  Canonical comparison route for quotable counts:

```
scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150     ->  /tmp/rig/g33
port 1500 samples at RIG_HZ=10   |   shipped 413 states over t=[47234..284054] ms
```

## 2. Target table

| # | Item | Oracle that will observe it | Route / drive needed | Cadence + sample basis | Rig gap to close |
|---|---|---|---|---|---|
| 1 | Intro music plays, keypress skips it, **title track never starts**, but end-of-track advance works | `cd_schedule.py` play/stop witness (LBAs, lengths, anchor deltas) + `music_melody.py` peak-set fingerprint on the shipped mixer capture + `port_mix.py` for the port's CD-DA timeline | **new drive**: `title_basic.txt` extended with the intro->title keypress, long enough to cover both tracks | 150 s route; audio at the board rate from the port records, shipped mixer at 44100 (docs/130) | the drive must actually perform the skip, and the fingerprint needs the *title* track as a reference (docs/131 showed track 11 never audible on the shipped side either -- audibility, not selection, is the question) |
| 2a | EXP text after a kill is garbled | `diff_screen.py` grid diff restricted to the HUD number-display region + the port's glyph/blit witness; the shipped draw site is `fn_0137c1` (0x0137c1), which *prints* `entity[+0x2b]` as a 5-glyph field and ramps it while `+0x52` drains (docs/125 §5) | existing `ep1_kill3.txt` | screen at 10 Hz on both sides; port frames from `frames.bin`, shipped from the capture session | the screen oracle has no region cropping yet, and the capture session must survive the mode change (fixed in `34e8697`) |
| 2b | EXP gain is not per-enemy-type | record-field oracle: shipped victim record award field vs the port's award expression `*(P+0x52) += victim[+0x32]` (guest 0x15453, `f.c:7938`) | `ep1_kill3.txt` plus a drive that kills **different entity types** | 10 Hz state, 1500 port samples / 413 shipped states, live-record counts quoted | per-type coverage: the census must report the award value grouped by `type`, and both sides need the same type mix |
| 3 | Spending an upgrade on combo lights **every** menu item | DS cell census of the upgrade/level state cells + `diff_screen.py` on the menu region | **new drive**: reach the level-up menu, select the combo/technical row, spend once | 10 Hz | no drive reaches the menu today; needs `wait ds` on the menu-open cell and the menu navigation keys |
| 4 | Level-up intro and "EXP not enough" advance with no keypress | event-anchored timing: transition observed without a consumed key event, compared with the shipped key-wait mechanism (`fn_00035dfc`, docs/97) | same new menu drive | anchored transitions, not raw frame numbers | the drive language has no "assert no key was consumed" verb; the transition must be taken from a DS predicate on both sides |
| 5 | Entering stage 2 crashes (`bus error`), floor black before the crash | fault attribution under ASan/UBSan (address + site + counterfactual) + screen oracle for the unrendered floor | **new route**: kill-quota route to stage 2 (`quota = [DAT_00052a1c + 4*([DAT_00063354]+0x40) + 0x28*(DAT_00063370-1)]`, `kills >= quota -> DAT_0006336c = 4`, docs/99), plus `--warp-stage 2` on the port side and a shipped-side memory-API poke (docs/74) | 10 Hz state, ASan soak frames 4000 | neither side reaches stage 2 on any stored route: `DS 0x63370` is `0x1` for all 1500 port samples **and** all 413 shipped states of the canonical route |
| 6 | Attacking at the far left draws the limb on the right side | one-blit screen diff: sprite x vs camera and the view-geometry basis (docs/69, docs/82) at the left-edge case | **new drive**: walk to the left map edge, then attack | 10 Hz, both sides | the current drives never put the player at the left edge while attacking |

## 3. What is already measurable on the frozen build

- Item 2b is measurable today: the award mechanism and both operands are known (`victim[+0x32]`, `P[+0x52]`), and the
  award witnesses exist (`adds=2 added=20 commits=0`, docs/125 §6b).  What is missing is grouping by enemy type.
- Item 1 is half-measurable: the schedule oracle and the fingerprint exist, and docs/131 already showed a shipped
  track that is *never audible* (track 11, IoU 0.19-0.40, replicated 0.380) -- the new drive must reproduce the
  keypress skip, which no stored drive does.
- Items 3, 4, 5, 6 have **no route at all**.  Their first rig work is a drive, not an oracle change.

## 4. Standing rules carried into this cluster

- Quote every count with run id, cadence and live-sample count (docs/125 §6); 45 s routes produce misleadingly low
  residual counts (docs/176 §7).
- Anchor on a transition, never on a level measurement (docs/158).
- "Zero callers" is not a reachability verdict: check CALL edges, body contiguity *and* the boundary instruction, and
  remember the event table at `DS 0x537dc` (docs/179-181).
- A fixture that asserts "this cell is zero because nothing writes it" is a hypothesis (docs/176 §4).
- Transcribe shipped bytes only where shipped and port spans agree (docs/133 §9).
- Confirm a new witness string exists in the binary (`strings -a port/test/night | grep -c <label>`) before believing
  a silent measurement (docs/169), and remember `make -C port test` now rebuilds the app binary (docs/181 §5).
