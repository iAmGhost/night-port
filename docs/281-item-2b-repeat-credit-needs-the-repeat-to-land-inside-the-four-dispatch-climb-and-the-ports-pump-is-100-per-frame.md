# 281 — Item 2b: the repeat credit can only be earned *inside* the four-dispatch climb, and the port's keyboard pump runs ~100 × per frame — so realistic DOS typematic intervals can never earn it

`night_key_apply` (`src/app/keymap.c:52-70`) credits `g_dat_62cd8` **only** on a make that arrives while the slot
state is still non-zero *and* the held byte is clear (shipped `0x3fdf2-0x3fe26`, the `re-make` arm ✓; the first-make arm
`0x3fe33` just arms to 1 ✓) ✓ Combining that with the climb (`fn_034735`: `state < 4 → state++`, so **4 pump entries**
`1→2→3→4`, then the post disarms to 0 when the credit is 0 ✓) gives the rule:

> a repeat must arrive within ~4 pump entries of the make, or the slot has already posted and disarmed and the credit
> is unreachable ✓

Measured, canonical drive, `NIGHT_ACTTRACE=1`, the drive's typematic emitter given two shapes (`main.c:700-720`, new
`NIGHT_KEY_REPEAT_MODE=pulse|make`; `pulse` emits `break,make` per repeat — the shape IBM BIOS typematic actually
queues ✓ — `make` emits a bare make ✓):

| drive repeat shape | delay/every | posts (`fn_034353`) | posts **with credit > 0** | `fn_0348e2` null pushes | `fn_03442e` | keys ever credited |
|---|---|---|---|---|---|---|
| `make` | 250/100 | 106 | **0** | 9 | 201 | `0x1c` only |
| `pulse` | 250/100 | 327 | **0** | 33 | 513 | `0x1c` only |
| `pulse` | 100/30 | 1124 | 1 | 472 | 1974 | `0x1c:50`, `0x1d:1` |
| `pulse` | 60/20 | 1227 | 2 | **530** | 2178 | `0x1c:67`, `0x1d:1`, `0x4b:1`, `0x4d:1` |

and the pump census in the same runs: `fn_0347e3` = **866 k – 933 k** entries in the 150 s run ✓ ≈ **100 entries per
60 fps frame** ✓ — i.e. the climb `1→2→3→4` completes in a few milliseconds ✓ A repeat at the real BIOS rate (100 ms ✓)
lands ~6000 pump entries later ✓ after the slot has already posted and disarmed ✓ That is why `credit > 0` posts sit at
exactly 0 for every realistic setting ✓ and why forcing the interval down to 20 ms does unlock chain-key credits
(`0x1d:1 ✓`) but overshoots the shipped's ring census wildly (530 null pushes against the shipped's total of 129 ring
*write events* ✓ docs/279 §4 ✓) — so **shrinking the repeat interval is not the fix** ✓

## What this localizes, stated precisely

* The keyboard state machine and its credit rule are **byte-faithful** (docs/280 §1 ✓, `0x3fdf2-0x3fe26`, `0x347b5-0x347d5` ✓) ✓
* The chain keys can only be credited if the pump's cadence is comparable to the input event rate ✓ In the shipped,
  `fn_0347e3` is one call per room-loop tick (`fn_03a829` ✓ `0x3aae5` immediately precedes the chain call ✓); in the
  port the room loop turns ~100 times per frame ✓ ⇒ the shipped's climb spans ~4 *ticks* (comparable to a typematic
  interval ✓), the port's spans ~4 pump entries (milliseconds ✓) ✓ **That cadence mismatch is the mechanism** ✓
* Consequence for the rig: a state census of `g_dat_62c58` cannot gate this (docs/280 §2 ✓), and neither can the
  `0→code` post metric (docs/277 §1 ✓) ✓ The admissible gates remain the *per-sample state predicates* docs/279 §4
  established: in-room ring ≥8-filled (shipped 85.4 % ✓), `0x1d` window at `cursor-1` for L=2/L=3 (22.6 % / 12.9 % ✓),
  cursor-at-0 (17 % ✓), then `DAT_00062d58` gaining `0x34/0x35/0x36` ✓ and the award census ✓

## Not yet decided (recorded, with the cheap test that decides it)

`NIGHT_KEY_REPEAT_MODE=pulse` with the BIOS defaults is the *shape-faithful* drive model (BIOS typematic queues
break+make pairs ✓) and triples the post rate to 2.2 /s against the shipped's ≥1.08 /s ✓ — plausible but unproven ✓
Decisive test queued: run the canonical route with `pulse` and read the dump-side gates (fill 85.4 %, L=2 22.6 %,
L=3 12.9 %, `DAT_00062d58` targets, `scripts/rig/exp_award.py` ✓, repeated ≥2× per side ✓) ✓ If the gates do not move,
the next candidate is the room-loop cadence itself — `fn_03a829` turning 100× per frame instead of once per tick —
which is tied to the pacing basis (docs/168, docs/244) and must not be changed blind ✓ Suite after this change: ALL
PASS ✓
