# 217 — Item 4 measured on both sides from one route: the shipped holds a picture for 4.5-7.5 s while the DS keeps advancing, the port produces a new picture every 0.2 s

Route `k1` = `scripts/rig/drives/ep1_keywait.txt`, 140 s, `scripts/rig/route.sh`, canonical
`PORT_ENV` (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`), port dumped at 10 Hz, shipped
state replay at 4 Hz and shipped screen capture at 25 Hz.  Drive = the 94 s ep1 level-up prefix, then
`94000 poke 0x6336c 2` (the exit latch that starts the level-up transition: guest 0x0003ac2a tests
`DAT_0006336c == 2` and calls `fn_03d244`), then one confirm press, then a key-free tail, then a second press.

## 1. Two rig corrections this route required (tool changes, not port changes)

**(a) The shipped-side runners ignored every injected write.**  `replay_original.py` and `capture_original.py`
parsed only `key` and `wait ds`; a drive containing `poke` aborted with `unknown drive step`, and `play`/`ent`
(player-record pokes) were unusable on the shipped side at all.  Both scripts now execute them: `poke ds val`
is a 4-byte PUT at `phys_ds_base + ds`, and `play off val` is a write to `off` of the record that DS 0x53c7c
points at (the memory API answers guest-linear addresses directly, which is what `read_at` already relies on).
Executions are recorded in `timeline.json` as `pokes` with the value read back.

**(b) The screen-capture half of a route used to drop everything after its first anchor.**  `capture_original.py`
posted `post_segment(0)` and never advanced `seg_i`, so with a drive whose gameplay keys sit after
`60000 wait ds 0x53e9c >=1`, the *screen* half of every such route was a key-free run of the title screen while
the *state* half played the game.  It now walks segments and executes pokes exactly like `replay_original.py`.

## 2. `wait ds 0x53e9c >=1` is not an anchor on the shipped side

`DS 0x53e9c` is the resident entity-type table (docs/199), non-zero from the moment the DS is loaded.  On the
shipped side the condition is true the first time it is tested, so the rebase rule fires immediately:

```
pokes: [{"t_ms": 35078, "nominal": 94000, "ds": "0x6336c", "want": 2, "read_back": 2}]
```

i.e. the shipped timeline was rebased by **-58.9 s** (the poke's nominal 94.0 s ran at wall 35.1 s).  The port,
which anchors the same drive on `wait res title1.pcx`, reported
`wait ds 0x53e9c >=1 timed out at t=54916ms val=0 (continuing)`.  So for every `ep1_*` drive the two sides are
*not* aligned at the mid-drive anchor, and any comparison that assumes "the same moment" between a port cell
sample and a shipped state sample has to be re-derived from an event the two sides can both see.  It does not
invalidate the comparison below (both sides are compared in the *state* they were in: live gameplay, latch just
consumed, no keys since), but it does mean the shared key times are not shared moments.  Recorded as a basis
caveat for earlier routes that used this anchor.

## 3. The measurement: picture stability with no key event

Both sides were verified to be in live gameplay when the latch was consumed: shipped `DS 0x63354 = 3678252`
(room record) non-zero from 3.14 s, `DS 0x63370 = 1`, `DS 0x53c7c = 5723848`, `DS 0x6336c = 2` at
35.34-36.28 s and 0 afterwards (consumed); port `memstat stage=1 room=1` through the same window and
`script: poke ds 0x6336c = 2 (ok) at t=83050ms`.

| side | window (key-free) | samples | distinct pictures | longest hold | new pictures / s |
|---|---|---|---|---|---|
| shipped (state replay, 4 Hz) | 35.2-50.0 s after the latch | 43 | 26 | 18 states = **4.5 s** | 1.73 |
| shipped (state replay, 4 Hz) | 50.0-80.0 s | 84 | 26 | 30 states = **7.5 s** | 0.87 |
| port (`k1/port`, resampled at the shipped 6 Hz cadence) | 94.5-108.5 s after latch+press | 81 | 79 | 1 frame = **0.17 s** | 5.64 |
| port (`l7`, level-up card window, 6 Hz) | 100.7-103.9 s after press release | 19 | 19 | 1 frame = 0.17 s | 5.94 |
| port (`l7`) | 104.7-115.0 s after the 2nd press | 59 | 15 | 45 frames = 7.5 s | 1.46 |
| port (`l7`) | 115.0-130.0 s tail | 86 | 1 | 86 frames = 14.4 s | 0.07 |

Shipped runs are state-replay states, which are recorded *only when the DS or the picture changes*: a run of 18
states is 18 consecutive DS-changed samples whose picture is the same image, i.e. the game logic is advancing
while the screen does not.  A separate shipped capture (`/tmp/dbgauto/orig_kw2`, 6 Hz, `drive_orig_kw2.txt`,
one press then three key-free windows) gives the same shape in the slideshow path: `k1_after_press` 18 frames /
2 distinct / longest run 17, then `k2`, `k3`, `k4` = 18 frames each, **1 distinct**, and those 54 consecutive
samples hash identically (`md5 e8c9c08fb4c1e360092e50a7740003bc`).  The shipped's level-up card windows from
docs/209 (`/tmp/dbgauto/orig_lu6`, one press per window) hold 13-14 of 15 samples.

The port does not hold: in the level-up card window (`l7`, `res help8.pcx` looked up at t=100017 ms, the press
released at 100600 ms, nothing pressed until 104000 ms) a different picture appears 5.94 times per second, and
no picture survives more than 12 raw frames (0.2 s).  The same is true of the `k1` window: 79 distinct pictures
in 81 samples.

## 4. Localized, and what is still open

The shipped mechanism is the slideshow key-wait of docs/97: `fn_038f3d` presents one picture and waits for one
key.  The port's card path returns from the card display call instead of holding it: `fn_035f1b("help8.pcx", 1)`
at `port/src/f/f.c:25750` returns, and the key-release drains (`up_ctr` / `down_ctr`, the 0x13 -> 0x14 guards at
`f.c:25783-25797`, guest 0x000396a0-0x00039767) then feed the next card without a new press.  That is the fix
target for item 4 and it is a *wait condition*, not cursor logic: the port's cursor does not self-advance (page 0
for all 1726 menu frames of `l6`, 0 page changes with no keys, docs/209), it is the *picture* that does not hold.

Not claimed: the "EXP not enough" message has not been observed on either side by the rig (no drive reaches the
purchase path on this route); item 4's gate is met for the level-up introduction, and the same wait condition is
what that message goes through (`fn_038f3d` presents, then waits) which is why it is listed as the same fix
target rather than a separate measurement.

Clue carried forward to item 5: on this route the port unwound its game main fatally 1530 times
(`game main unwound (quit / fatal)`, `port.log` from t=120916 ms press onward) while `memstat` still reported
`stage=1 room=1`, and the rig reported `record reads refused (outside arena)=4964`.  That is a fatal path
reachable from the level-up/exit-latch transition, distinct from the stage-cell poke of docs/208, and it is the
first rig-side fatal on an unfixed-build route since docs/208 said the poke route does not fault.
