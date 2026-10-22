# 0x016b8f — the three-phase step/return walk (L11, 1117 B)

Ported as `fn_016b8f(uint8_t *e, int32_t step, int32_t cmp_ebx, int32_t cmp_ecx, int32_t *out_a,
int32_t *out_b)`; test `test_fn_016b8f` (13 checks).

Keystone #2: a direct callee of **20 still-unported** per-type handlers. Semantically it is a
small creature's scripted movement: **approach (phase 0) → pause (phase 1) → walk back
(phase 2)**, with a per-call step budget and a "did the target escape the box?" abort.

## ABI (note the four register inputs *and* two stack out parameters)

| source | meaning |
|---|---|
| EAX | entity (`[ebp-0x10]`) |
| EDX | `step`, the per-call movement amount (`[ebp-0xc]`) |
| EBX | `[ebp-0x8]`, compared against `fn_01519f(entity)` = `[e+1] - DAT_639dc` (entity Y rel. to the room centre) |
| ECX | `[ebp-0x4]`, compared against the room's mid-Y |
| `[ebp+0x10]` | `int32_t *out_a` → accumulates into `[e+0xa3]` |
| `[ebp+0x14]` | `int32_t *out_b` → accumulates into `[e+0xa7]` (except in phase 2, see below) |

The original's stack args 1 and 2 (`[ebp+8]`, `[ebp+0xc]`) are **never read** — the two out
pointers are stack args 3 and 4. The port collapses that into one signature.

`fn_00041f76` is `abs32`, so Ghidra's "two-value" return in this function is an artifact: the
second operand it shows is simply the `EDX` the code had already loaded (`[e+0x11f] >> 1`) and
`abs32` does not touch EDX.

## Entry / aim latch

`[e+0x17] = 2` is written **unconditionally** on every call — the same word `fn_01545e` reads to pick
the wide room band, so calling this function also changes which band applies to the entity.

If `[e+0x8f] == 0` the walk is (re)aimed:

```
mid       = [63354+0x12] + (([63354+0x16] - [63354+0x12]) >> 1)     ; sar, i.e. room mid-Y
[e+0x9f]  = (ECX <  mid) ? -1 : 1
[e+0x9b]  = ((int32_t)fn_01519f(e) < EBX) ? 1 : -1
[e+0x8f]  = 1 ; [e+0x93] = [e+0xa3] = [e+0xa7] = 0                  ; phase := 0
```

**Test trap:** because the aim branch also zeroes `[e+0x93]`, `[e+0xa3]`, `[e+0xa7]`, any test that
wants to enter a specific phase must pre-set `[e+0x8f] = 1` and set `[e+0x9b]`/`[e+0x9f]` itself.

## Phase 0 — approach (0x16c7e)

Runs while `abs32([e+0xa3]) < ([e+0x11f] >> 1) + 0xf` (distance walked vs half the entity's range
plus a 15 px tolerance).

* `out_a = ±step`: negated when `[e+0x9b] < 0`.
* `out_b = ±step`: **negated when `[e+0x9f] >= 0`** — the opposite polarity rule from `out_a`
  (0x16cc5-0x16ce6). This asymmetry is in the machine code, not a Ghidra artifact.
* `[e+0x97] < 0` (the budget, e.g. remaining frames/steps) → the out params **are** written, then
  `return 1` **without** decrementing.
* otherwise `[e+0x97]--`, `[e+0xa3] += out_a`, `[e+0xa7] += out_b`.
* Out of range → `[e+0x93] = 1`, both accumulators cleared, `[e+0x8f]` is **kept** (no re-aim).

## Phase 1 — pause (0x16d53)

While `abs32([e+0xa3]) < 0x3c`: `out_b` is forced to 0, only `[e+0xa3]` accumulates, budget
decremented, and then, **only if `[e+0x9f] != 0`** (0 disables the check — a latent feature), the
caller's (EBX, ECX) is tested against the box `[e+0xb3] ± 0x28` / `[e+0xb7] ± 7`:

```
if (EBX < b3-0x28 || b3+0x28 < EBX || ECX < b7-7 || b7+7 < ECX) { [e+0x93] = 1; [e+0x8f] = 0; return 1; }
```

Returning 1 here means "the target moved out of range, re-aim next time" (`[e+0x8f] = 0` forces the
aim branch on the next call). When the pause expires (`abs ≥ 0x3c`) → phase 2 + clear accumulators.

## Phase 2 — walk back (0x16ef0)

* Completes when `abs32([e+0xa3]) >= ([e+0x11f] >> 1) + 0xf` → `[e+0x93] = 1`, `[e+0x8f] = 0`,
  `return 1`.
* While walking back the `[e+0x9f]` polarity is **flipped** relative to phase 0 (`< 0` → negate).
* `[e+0xa3] += out_a` but **`[e+0xa7]` is never accumulated**, even though `out_b` is written to the
  caller's out parameter (0x16f7b-0x16f83 is the last store). Latent original quirk, kept as-is.

Any other phase value falls through to `return 0` (Ghidra's "unreachable block" at 0x16fd8 is the
`default` arm of the phase switch).
