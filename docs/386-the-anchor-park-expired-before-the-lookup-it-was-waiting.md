# 386 — the anchor park expired before the lookup it was waiting for, and the rig now dumps the lookups

docs/385 §4 named the precondition for a same-content route: the host's heard lookups had no dump path into the rig.
Both halves are done, and the route they enabled exposed a timing defect in the anchor mechanism itself.

## 1. `res.jsonl`, the rig's resource sidecar

`records.jsonl` carries `tick/player/ents/refused` and docs/288 keeps that layout frozen for existing readers, so the
lookups go in their own file (the `player_recs.jsonl` precedent).  `res_log_sample()` runs every poll, watches
`g_res_trace_seq`, and writes one line per change:

```
$ head -4 /tmp/port82/res.jsonl          # --warp-stage 1
{"frame":1,"tick":4,"res":"logo.pcx","seq":137}
{"frame":137,"tick":1239,"res":"title1.pcx","seq":138}
{"frame":205,"tick":280,"res":"1s1.pcx","seq":139}
{"frame":389,"tick":389,"res":"stage1.pcx","seq":142}
```

That is the vocabulary an anchored route needs, and it also says what an unscripted run does: three lookups
(`logo.pcx`, `opening.fli`, `title1.pcx`) repeating in the attract loop for 9 000 frames.

## 2. The defect: a park that expires just before its own anchor lands

`scripts/playtest/route_mirror_episode1.txt` walks the original drive file's phase sequence with `wait res` anchors on
those names.  Two anchors timed out even though the lookups happened:

```
script: `wait res 1s3.pcx` timed out after 8001 held polls (continuing)
script: `wait res stage1.pcx` timed out after 8002 held polls (continuing)
res.jsonl:  {"frame":7140,"res":"1s3.pcx"}   {"frame":7796,"res":"stage1.pcx"}
```

The arithmetic says why.  The slot is `60000 wait res 1s3.pcx` = poll 7500.  This host runs one pad poll per presented
frame but at 8 ms per poll against ~54.5 presentations/s, so **polls advance ~2.1× faster than frames**: the lookup at
frame 7140 corresponds to poll ~15 200, and the park's ceiling was 8 001 held polls (ending at ~poll 15 500 counted from
the park's start, i.e. a few hundred frames *before* the lookup arrived).  The anchor expired a moment before the event
that would have satisfied it, and the route continued without its anchor.

The ceiling is now 60 000 polls (~8 min of holding) and tunable with `NIGHT_SCRIPT_PARK_MAX`.  A park is not expensive
-- the game keeps playing while the program waits -- so a short ceiling only manufactures failures: with the raised
ceiling the same route reports **0 timeouts**.

## 3. What the mirror route reached, and what the alignment basis still says

With 0 timeouts the route parked at `wait res 1s3.pcx` and stayed there: the lookups it recorded end at `1s1.pcx`
(frame 2549), so its `enter` presses start the slide show but do not walk it to the last slide the way the original's
three presses plus soft mode waits do.  And the alignment basis still returns exactly its floor for this route --
`0.5590` at `--win` 1000 and 4000 against a floor of 0.5590 (0.0 % of the way to identical).

So the honest state of the sequence-alignment basis, after docs/384 measured its floor and docs/385 its dialect
problem: it needs a port route that walks the same phases in the same order *at comparable times*, and the anchor
machinery is now correct enough to try again (0 timeouts, heard lookups dumped).  The picture-level results of
docs/381-383 (stage play 55/55 at 0.9998, slides 130/133, the pause menu 41/41 at NCC 1.0000) remain the measured
accuracy basis in the meantime, and they are unaffected by any of this.

Harness-only: card 0.1.68 unchanged, `src/f` untouched, fixture suite `== ALL PASS ==`.

## 5. The sidecar's first version broke the core build (0.1.69)

`g_rig_dir` lives inside the `OF_PC`-guarded rig block near the top of `main_of.c`, and `res_log_sample()` reached for
it unguarded.  The desktop shim compiled it and the **RISC-V build failed**:

```
main_of.c:1601:10: error: 'g_rig_dir' undeclared (first use in this function)
make: *** [pocket/.obj/night/main_of.o] Error 1
```

which is the asymmetry docs/371 already warns about -- a passing `app_pc` build is not evidence the core builds.  The
file write is `OF_PC`-only now (a rig sidecar is a desktop artifact); the lookup ring the anchors search is fed on both
builds.  `make -C pocket/src/night all` and `make deploy` both succeed, the card is at **0.1.69** with ELF md5
`817131e7041de58be7f3bf6857a3f016` identical to the build output, and the deployed version string matches `core.json`.

