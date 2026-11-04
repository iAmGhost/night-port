# 282 — Item 2b goes green on the chain gates: pulse-shaped drive repeats make the port select targets 0x33/0x34/0x35/0x36 like the shipped, and a settled `kind==1` enemy now carries an award

## 1. The change

`src/app/main.c` (`pt_drive`, docs/281): the scripted drive's synthesised typematic repeat now emits a
**break+make pair** per repeat (`NIGHT_KEY_REPEAT_MODE=pulse|make`, default `pulse` ✓) — the shape IBM BIOS typematic
actually queues ✓ — instead of a bare make ✓ Motivation (docs/281): `night_key_apply` credits `g_dat_62cd8` only for a
make that arrives while the slot state is non-zero and the held byte is clear (shipped `0x3fdf2-0x3fe26` ✓), and a bare
make never clears the held byte (`keymap.c:56-58` ✓ `0x3fe69-6e` ✓) — so with bare makes the credit path was
unreachable for every key except tapped `0x1c` ✓

## 2. Before / after on the canonical route (`ep1_kill3.txt 150`, in-room samples only, dump-side, docs/279 §4's gates)

| gate | shipped rb8 | shipped rb9 | **port rb8** (bare makes) | **port rb9** (pulse) |
|---|---|---|---|---|
| `DAT_00062d58` chain target census | `{51:256, 53:83, 0:42, 52:42, 54:5}` | `{51:251, 53:87, 0:42, 52:40, 54:3}` | `{0:929, 51:571}` ✗ | **`{0:914, 53:206, 51:153, 54:149, 52:78}`** ✓✓ all four targets now selected |
| ring ≥8 of 10 filled | 85.4 % | 77.9 % | 57.3 % | **61.4 %** ✓ |
| `0x1d` window at `cursor-1`, L=2 | 22.6 % | 23.6 % | 12.6 % | **22.4 %** ✓ (parity ✓) |
| `0x1d` window at `cursor-1`, L=3 | 12.9 % | 14.0 % | 5.8 % | **16.3 %** ✓ (parity ✓) |
| ring completely empty | 8.9 % | 9.0 % | 32.2 % | 31.5 % ✗ (unchanged) |
| cursor at 0 | 16.9 % | 17.6 % | 33.6 % | 38.1 % ✗ (still high) |

State oracle tally on rb9: `SAME 69 / DIFF 28 / SIZE? 32 / PTR 52 / REPR 19 / DIFF-SIZE 16` ✓ against rb8's
`SAME 70 / DIFF 27 / SIZE? 33 / PTR 52 / REPR 19 / DIFF-SIZE 15` ✓ — one row moved `SAME→DIFF` and one
`SIZE?→DIFF-SIZE`, activity profiles **271 SAME (unchanged)** ✓, divergent runs 8 (rb8: 10 ✓) ✓ no new unexplained row
class ✓ Screen-capture leg of the route failed this time (`HTTP 409 Conflict` on the container's capture endpoint ✗), so
the pacing line for rb9 (`orig 2.73 vs port 24.59 changes/s ✓`) is the replay-stream basis, not the 25 Hz screen basis ✓

## 3. Record-field oracle, before/after (`scripts/rig/exp_award.py`)

* **port rb8 (before):** no dying record of `kind==1` with a non-zero award ✓ (docs/239-241's finding stands ✓)
* **port rb9 (after):** `kind=1 type_idx=31 award(+0x32)=10 state=0x15 ×44` ✓✓ a settled real enemy carrying an award,
  plus per-type award values `type 0 → [0, 5, 52, 53, 54]`, `type 31 → [0, 10]`, `type 36 → [0, 5]`,
  `type 201/250 → [0]` ✓
* **shipped rb9:** dying records `kind=0 type_idx=0 award(+0x32)=54 ×45` and `×9` in state `0x15`/`0x1` ✓, dying
  `kind=1 type 31 → award 0` ✓, per-type `type 0 → [0, 53, 54, …]`, `type 31/36/201/250 → [0]` ✓

**Reading:** the shipped puts a **chain target id** (`54 = 0x36` ✓, `53 = 0x35` ✓) in `+0x32` on its dying records —
docs/188 ("the 53/54 in `+0x32` are state ids, not EXP") ✓ **re-confirmed on this basis** ✓ — while the port now also
puts `52/53/54` there for `kind==0` records ✓ (so that half is now shared behaviour ✓) but *additionally* writes `10`
for `kind==1 type 31` and `5` for `type 36`, where the shipped writes `0` ✗✓ So the chain-selection half of item 2b is
closed, and the remaining open half is the **award-value semantics**: where the shipped's actual per-kill EXP number is
computed and stored (not `+0x32` on the dying record ✓, and docs/189's "no per-type EXP table exists" ✓ still stands ✓)

## 4. Still open for this item (recorded, not hand-waved)

1. Player-side accumulation was **not** demonstrated: reading `+0x52` as a dword out of the record dumps gives
   `640553484` (pointer-shaped ✓) constantly on the shipped side and `0` on the port side ✗ — so that offset is not the
   player EXP cell in the record-snapshot basis, and the "player EXP rises" gate has no valid instrument yet ✓ Find the
   shipped's EXP storage (fn_015295's destination ✓ `player[+0x52]` per docs/254 §3 ✓ vs the docs/125 record layout ✓)
   and re-gate ✓
2. Two residual divergences the pulse mode did **not** move: ring empty-fraction (31.5 % vs 9.0 % ✓) and
   cursor-at-0 (38.1 % vs 17.6 % ✓) ✓ consistent with docs/281's cadence finding (the port's pump runs ~100 room-loop
   turns per frame ✓, so the ring cursor spends most samples parked) ✓
3. `route.sh`'s screen-capture leg failing with HTTP 409 ✗ must be fixed before rb9-style runs can support screen-basis
   claims (the state and audio legs ran fine ✓, the audio step's `report_audio.txt` is still missing ✗ standing issue) ✓
Suite status: `make test` **ALL PASS** at `60ba822` (the commit carrying the `pulse` emitter ✓); soak and fuzz
re-runs are queued as part of `gate3-regression` ✓
