# 385 — the shim now speaks the driver's dialect, and geometry anchors turn out to be the wrong bootstrap

docs/384 showed why feeding the original's drive file to the shim produced nothing: **a different dialect**.  The route
files are timestamped (`20000 key enter down`); the drive files are a sequential program paced by the game's own
geometry (`wait_mode 480 180`, `record slides 22000`, `key enter 150`).  The shim now understands both.

## 1. What was added

`pocket/src/night/main_of.c` gained a sequential executor next to the timestamped one: a program counter, `key <name>
[press_ms]` firing when the counter reaches it (make codes through the same `g_script_keys` table, hold on the 8 ms
pad grid), `wait_mode <height> [timeout_s]` parking the whole program until the view the host is presenting has that
height, and `record <label> <ms>` dwelling for that long because the driver blocks while it samples.  Dialect is
detected by the first token: a number means timestamped, a verb means sequential.

It parses the real drive file:

```
script: scripts/playtest/drive_orig_episode1.txt -> sequential drive dialect: 8 keys, 5 `wait_mode` geometry anchors,
        9 `record` dwells, 2 lines skipped
```

## 2. What it measured: the anchors dominate the program

In a 14 000-frame run (about 112 s) **only 2 of the 8 keys ever fired**:

```
script: key enter at poll 2501 (hold 18 polls)
script: key enter at poll 5251 (hold 18 polls)
script: parked waiting for a 480-pixel-tall view (1 polls held)
script: parked waiting for a 480-pixel-tall view (513 polls held)
script: parked waiting for a 480-pixel-tall view (1025 polls held)
```

and the alignment basis stayed exactly on its floor at every window -- `0.5590` at `--win` 90, 400 and 4000, against a
floor of 0.5590 (0.0 % of the way to identical), while docs/384's warp and warp+ESC runs rose to 0.7786 and 0.8056 at
`--win` 4000.

## 3. Why geometry anchoring cannot bootstrap this comparison

The driver's anchor is **the captured emulator frame's pixel height**; the shim's anchor is **its own presentation
height**.  They agree only when both sides are already in the same phase at the same moment -- which is precisely the
thing the comparison is supposed to establish.  So a geometry anchor cannot be used to *bring* two runs into the same
phase: it presumes it, and where it does not hold it parks the program for thousands of polls and starves the route.

Resource-lookup anchors are the anchor that works on this host, and docs/380 §3 already proved it (13 parks, 0
timeouts, run reaching stage play): a lookup *names what the game is loading* -- `logo.pcx`, `opening.fli`,
`stage1.pcx` -- which is phase information that does not depend on the clock or on presentation geometry.

## 4. The next concrete step, with its blocker named

A port route in the timestamped dialect, anchored with `wait res` on the same phase sequence the drive file walks
(boot card → `.fli` intro → title melt → slides → stage → ESC menu), would be a same-content comparison the alignment
basis can legitimately score.  It needs the lookup names, and the probe here shows they are not currently readable:
`records.jsonl` carries `tick/player/ents/refused` and **no resource field**, so the host's resource ring (`g_res_trace`
/ the ring `res_seen` searches) has no dump path into the rig.  Adding one -- a `DIAG` bit that logs each announced
lookup, or a `res` field in `records.jsonl` -- is a small host change and the precondition for that route.

The sequential dialect stays in the shim: it is inert unless a drive file is passed, and it is what a future
pad-driven device run would use if geometry anchoring is ever wanted on hardware.  Harness-only again: card 0.1.68,
`src/f` untouched, fixture suite `== ALL PASS ==`.
