# 316. The follow code does run; no stage-walk run has yet held Down inside episode 1's stage 2

Goal task-8 (round-4 item D).  Continues docs/315 §2.  Everything below is episode 1's own chain (no warp flag),
skip-intro ON, pinned env (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`),
`scripts/rig/drives/ep1_walk_st2.txt`, 5 Hz dumps, witnesses `NIGHT_ROOMDBG=1 NIGHT_YCLAMP=1`.

## 1. The positive result: `fn_03cc6b` and the vertical mover both run in the port

`/tmp/rig/w316a` (poke tail cut at t > 26000, `--frames 12000`):

```
stage-walk census: menu-owned frames 0 of 12000 (0.0%), room-chain advances 0, menu posts 0
walk census: walking-body (LAB_0003284e) calls 1837, player DOWN fn_01586b calls 94
[vgate] visits: 41
[band] #1 kind=1 x=319 y=150 d=1 mn=139 mx=195 camY=0 room=+0x18d6ec next=0
```

For the first time the run shows `fn_03cc6b` reached (41 visits ✓) and the player's Down branch calling
`fn_01586b(+step)` 94 times ✓ So item D is **not** "the code never runs".  It is also not yet green: the window
lives in `+0x18d6ec` (stage 1), whose gate legitimately cannot fire — the shipped's arm needs
`[next+0x0c] - 0xc8 > DAT_639e0` and stage 1's next node has `+0x0c = 200`, so `200 - 200 = 0 > 0` is false ✓
(docs/314 §2 measured the same 49 visits / 0 gate-true ✓)  Item D can only be gated in **stage 2**, whose next
node is `+0x18ddd4` with `+0x0c = 320` → gate `320 - 200 = 120 > camY` ✓ fires while `camY < 120` ✓

## 2. Three runs, three different phase occupancies — the drive is still the problem

| run | poke tail | frames | advances | menu frames | `wk` / `wv` | `[vgate]` | where the 30 s Down window actually sat |
|---|---|---|---|---|---|---|---|
| `/tmp/rig/w315d16` | full | 16000 (15946 presented) | 2 | 3539 (22.1 %) | 1616 / 1 | 0 | **stage 3** — `DAT_63354` = `+0x18d6ec` → `+0x18da60` at 26.0 s → `+0x18ddd4` at 52.8 s, and the window ran 49.2–79.0 s |
| `/tmp/rig/w316a` | cut > 26000 ms | 12000 | 0 | 0 (0.0 %) | 1837 / **94** | **41** | stage 1, gameplay-owned ✓ but the wrong stage |
| `/tmp/rig/w316b` | cut > 26000 ms | 12000 (1000 samples) | 0 | 0 | 1930 / 0 | 0 | stage 1, but the resolver was never reached in the window: `md=0`, state `0x1`, admissions frozen 1930→1930 ✓ a card/menu wait owned the frame loop |

Two independent failure modes, both drive defects rather than game defects:

* **Too many requests.** The full poke series lands *two* chain advances, and the `wait state advance 1` rebase
  then pulls the Down window into stage 3 ✓ (docs/314's "release Right on the anchor line" fix is not enough:
  the leftover requests are what advance, and they are all scheduled before the anchor ✓).
* **Too few requests, and no key to clear a card.** Cutting the tail removes the advance entirely (advances 0 ✓),
  and the window then lands in a `fn_00035dfc`-style card wait that swallows the frame loop (w316b ✓) — the same
  class as the menu swallow in docs/315 §2, second member of the family.

`DAT_639e0` in every run changed at most once, 0 → 200, and that single change is `fn_03cad3`'s install
(0x3cb1f `mov [0x139e0], eax` ✓ = `camY = [node+4]` ✓), never the follow arms 0x3cd42 / 0x3cdfc ✓

## 3. The 16000-frame fault did not reproduce

`/tmp/rig/w315k` (edited drive, witnesses on, 16000 frames) died with `rc=139`.  The same drive and env then ran
clean three times: `/tmp/rig/w315c16` (`--frames 16000`, no witnesses, rc=0, 15991 frames), `/tmp/rig/w315d16`
(witnesses on, rc=0, 15946), and the 800/1600/3200/6400/9600/12800 bracket was all `rc=0` ✓ ASan at 500 frames:
`rc=0`, no sanitizer report ✓

So the fault is **not deterministic on this drive** ✓ which is expected and must be said out loud: the drives are
wall-clock anchored, so the same script reaches different game phases in different runs (docs/191 §1), and a fault
that depends on reaching a particular phase inherits that variance ✓ A bracket table like docs/315 §4 is therefore
only quotable per-run, and the crash hunt needs either a repeat-until-fault loop with the phase counters recorded,
or ASan run long enough to cover the phase (`--frames` ≥ 13000 ✓ ASan is ~4× slower ✓ i.e. a ~20 min wall budget ✓).
The `[vgate]`/`[band]` witnesses were checked for unguarded NULL derefs (`c + 0x4`, `c + 0xc` at the chain
terminator ✓ `f.c:9914-9915`); that guard is owed and is the first thing to add before the next crash hunt,
because a witness that faults looks exactly like a game fault.

## 4. Next concrete rig step

Add a *gameplay-owned* predicate to the drive language, so a key window can wait for the thing it must land in:
`fn_03272f` already counts its own invocations per call (`g_walk_body_calls` etc. ✓ `f.h`/`rig_sink.c` expose them
as `wk`/`wa`/`wv`), so a counter `g_act_resolver_calls` plus `wait state resolver` (the harness waits until that
counter *advances*, like `wait state advance N` does today) makes the Down window land in a frame the resolver
owns, wherever that falls in the clock.  That kills the menu/card swallow failure mode; the stage-selection
failure mode then needs the request phase capped by *advances* rather than by time (e.g. stop poking once
`f_rig_chain_advances()` reaches 1 — a new PT_MENU-style verb `stop when advance 1`).
