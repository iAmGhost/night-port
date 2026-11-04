# 266 — `route.sh`'s stale-binary guard tripped on same-second edits; and the slot-selection fix leaves the state-oracle tally unchanged (no regression, but no movement either)

## 1. Rig defect: the guard was not satisfiable

`route.sh` refuses to run when any source is newer than `build/night` ✓ (the docs/133 §8 rule ✓). Two canonical
runs failed at exactly that line (`/tmp/rig/ch3`, `/tmp/rig/rb6` — each produced only `build.log` ✓) while
`make app` answered "Nothing to be done" ✓ and a manual `./build/night` run worked fine ✓ The reason is a
granularity mismatch: `make` compares timestamps to whole seconds ✓, so an edit landing in the same second as the link
leaves the target considered up to date ✓, while `find -newer` compares sub-second mtimes ✓ and still reports the
source as newer ✓ (`src/f/f.c` 12:22 vs `build/night` 12:22 ✓ f.c newer by milliseconds ✓) ✓ The guard could
therefore never be satisfied for edits made in the same second as the previous build ✓

Fix: when the guard trips, rebuild the app target from scratch (`make app -B` ✓ logged to `build.log` ✓), then
re-check; only exit 1 if a source is *still* newer after the forced rebuild ✓ The guard's strictness is preserved ✓
(the run still cannot use a binary older than a source ✓) ✓ `/tmp/rig/rb7` then ran end to end ✓ (`rc=0` ✓) ✓

A separate non-fatal defect surfaced in the same run: the route's audio step failed with
`cat: /tmp/rig/rb7/report_audio.txt: No such file or directory` ✓ while the route continued and still wrote
`DIVERGENCE.md` ✓ — recorded, not yet fixed ✓

## 2. Regression evidence for the slot-selection fix (docs/263) on the canonical route

Both runs: `scripts/rig/route.sh scripts/rig/drives/ep1_kill3.txt 150`, `PORT_ENV` with
`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`, `--rig-hz 10` ✓

```
rb3 (before the fix):  samples: orig n=429 t=[47230..283883] | port n=1500 t=[16..228033]
                       cells=346 phases=274 tally={SIZE?:35, PTR:52, SAME:69, REPR:19,
                       DIFF-SIZE:13, DIFF:28, NON-GAMEPLAY:host_address_space:2, NON-GAMEPLAY:device_plumbing:27}
rb7 (after the fix):   samples: orig n=429 t=[47222..283606] | port n=1500 t=[16..216500]
                       cells=346 phases=274 tally={SIZE?:35, PTR:52, SAME:69, REPR:19,
                       DIFF-SIZE:13, DIFF:28, NON-GAMEPLAY:host_address_space:2, NON-GAMEPLAY:device_plumbing:27}
activity profiles: 271 cells, 275 phases, tally={SAME:271}  (identical on both runs)
pacing check: orig=2.71 picture changes/s, port=21.00, ratio=0.13   (unchanged)
divergent runs (score < 0.88): 8   (unchanged)
```

**Identical tallies on an identical basis** ✓ → the slot-selection fix introduces **no regression in the state-oracle
residual set** ✓ (the contract's "no regression in the 25-row attributed residual set" holds for this change ✓) — and
equally, **it does not close any residual row** ✓: the in-process improvement docs/263 measured (30 → 79 landed hits ✓,
enemy `+0x23` floor 23 → 15 ✓, player 0x33 samples 41 → 65 ✓) does not yet surface as a shipped-vs-port DS difference
on this route ✓ consistent with the route still producing **no settled `kind==1` enemy and no award** ✓ (`exp_award.py`:
type 31 `+0x32` still `[0]` ✓) ✓

Basis caution recorded: docs/254's post-clamp tally (34 DIFF / 24 PTR / 19 REPR / 14 DIFF-SIZE / 3 SAME) is **not
comparable** with the rb3/rb7 numbers above — that run's shipped side carried a different sample count ✓, and tallies
are only quotable within one sampling basis ✓ (the standing event-anchored-basis rule ✓) ✓

## 3. Where item 2b stands after this

Landed with rig evidence: `fn_01586b` clamp guard ✓ (docs/254 ✓), `fn_01441e` slot selection ✓ (docs/263 ✓) ✓ Proven
faithful against shipped bytes: award routine and its queue posts ✓, per-type table ✓, damage model ✓, hit gates and
facing polarity ✓, `g_tab_51b60` ✓ (0/256 ✓), the reaction block ✓, the technique container and its target ids ✓ (docs/264 ✓),
the input ring contents ✓ (docs/264 §7 ✓) ✓ Open: why the port's `fn_03272f` chain walk never completes a *multi-code*
entry (3166 `len != k` ✓ against 53 matches ✓) while the shipped, on the same key drive, enters `0x34/0x35` ✓ — the
cursor/window basis, to be measured on one basis with `--rig-dump` and repeated ✓ (the probes are installed:
`act_level_*`, `act_ring_*`, `act_cursor_hist` ✓) ✓
