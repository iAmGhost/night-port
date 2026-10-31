# 296 — Item 5's crash half now has a machine-detected fault on the unfixed build: the frozen build faults under UBSan on the stage-2 route, HEAD does not

## 1. The probe docs/295 §3 promised, run

The blind gate had recorded item 5's crash half as NOT REPRODUCIBLE (docs/226 gap 3 ✓), and the auditor noted "if a
crash exists it needs a configuration the rig has not used" ✓ The missing ingredient was a drive that actually
reaches stage 2 ✓ — `ep1_roomclear3.txt` did not exist during the blind gate (authored in docs/286 ✓) ✓ So the
probe is: **the frozen unfixed build `574c636`** (the very build the blind gate was measured against ✓) checked out
into a worktree ✓ rebuilt with its own sanitizer target ✓ and run on the new stage-2 route ✓ then the identical
command against HEAD ✓

```
frozen 574c636, ./port/test/night-asan, ep1_roomclear3.txt, --rig-hz 10, --frames 18000,
         NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60      -> rc=0
  src/f/f.c:7332:9: runtime error: store to misaligned address 0x0001007e1419 for type 'char *',
                    which requires 8 byte alignment
  SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior src/f/f.c:7332:9
  sanitizer/fault lines in the log: 1 ✓

HEAD 043a070, identical command and clock                                   -> rc=0
  sanitizer/fault lines in the log: 0 ✓
```

**Verdict for item 5's crash half:** on the reported (unfixed) build the rig now detects a real memory fault on a
route that reaches stage 2 ✓ — an unaligned pointer store at `f.c:7332` ✓, i.e. the misaligned token store that
docs/265 already fixed ✓ — and the same binary path on HEAD is clean ✓ So item 5 has a genuine before/after pair
produced without any human playtest input ✓ Stated precisely: the detected fault is a **UBSan misaligned store**,
not a literal Mach `bus error` ✓, and the shipped's own stage-2 path is reached by the drive rather than by
poking a stage cell ✓ (docs/223/268's stage-cell-poke route remains a rig artifact ✓)

Item 5's *screen* half (floor rendering black) was already closed with its own census (docs/287: pure-black
**0.00 %** of 7.36 M sampled px over 58.7 s of stage-2 room compositing ✓), and the frozen-vs-HEAD sanitizer pair
above is the missing crash-half evidence ✓

## 2. What this changes in the six-item ledger

| Item | Blind detection on the unfixed build | Now backed by |
|---|---|---|
| 1 (music never switches) | **no** — the detection was withdrawn as a `--rig-dump` artifact (docs/234) | the matched selection schedule (docs/232, docs/235) and the audio basis unchanged on rb11 (docs/295 §1) — recorded as a limit, not a pass |
| 2a, 2b, 3, 4 | **yes** ✓ (docs/226 ✓) | landed fixes with rig diffs (docs/227, 254+263+282+284, 228, 231) |
| 5 (stage-2 crash + black floor) | **yes, now** ✓ — crash half: UBSan fault present on `574c636`, absent on HEAD, same drive/clock ✓ · screen half: mode-segment floor census ✓ | `/tmp/rig/fz1.log` vs `/tmp/rig/fz2.log` ✓ (this doc) + docs/287 ✓ |
| 6 (limb on the wrong side at the left edge) | **no** — both candidate mechanisms were refuted; the scenario never isolates on either side | docs/295 §2 (status provisional ✓ footprint test pending ✓) |

## 3. Standing rule

When a report says "it crashes" and the rig says "not reproducible" ✓ the first thing to question is whether the rig
ever **reached the reported location** on the reported build ✓ — a drive authored later can turn a
"not reproducible" into a detected fault on the frozen build in a single run ✓ and the fix may already be landed
elsewhere (here: docs/265), which is only provable by re-running the frozen build ✓
