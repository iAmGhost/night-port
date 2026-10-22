# The entity AI dispatch map (0x031a8e)

Source: `analysis/pseudo/082_FUN_00031a8e_031a8e.c` (Ghidra) read against the pending set on
2026-07-13. Nothing here is ported yet: `0x031a8e` cannot link before its handler set exists, so this
note exists so the handler ports have a fixed map to work toward.

## Shape of the function

ABI: `EAX = entity block` (Ghidra renders the `PUSH imm; CALL 0x0418ee` stack probe as
`iVar1 = FUN_000418ee()`, but the probe preserves EAX, so `iVar1` is the entity). Ghidra's function length is
991 B; the STATUS row lists 1153 B for the entry (the extra bytes are the trailing epilogue/next-entry gap) —
re-derive the exact span with `scratch/fdis.py 31a8e` before porting.

Two shared state-machine arms run for every entity, then the per-type handler is dispatched:

1. **State 2 (patrol/hesitation)** — `entity[0x17] == 2`: increments `entity[0x103]`; on the 8th call
   it resets that counter and compares the stored position words `entity[0x107]` / `entity[0x10b]`
   against the live position from `fn_0001519f` (x) and `fn_000151d5` (y). If *both* are unchanged the
   entity is considered stuck/idle: `entity[0x17] = 1`, `entity[0x1f] = 0`, `entity[0xaf] = 1`,
   `entity[0xc7] = 1`. Otherwise the live position is stored into `0x107` / `0x10b` for the next round.
2. **State 3 (approach)** — `entity[0x17] == 3`: `entity[0x6b]` is an approach timer. It starts only
   while the entity is horizontally within +/-200 px of the player (`g_dat_53c7c`). Once running, the
   timer times out to state 6 (`entity[0x17] = 6`, `entity[0x11] = 0`) at **> 999** frames, or much
   earlier (**> 99**) when the entity's per-type table entry says the type is fast:
   `*(int32 *)(DAT_53eac + entity[0xd] * 0x88) >= 0x1f`. Proximity also forces state 6: within
   0x50 px horizontally *and* 0x28 px vertically, or within 10 px horizontally.

The player block itself never dispatches: the whole switch is guarded by `entity != g_dat_53c7c`, and
just before it, a player-only fix-up clears `player[0x4a]` whenever the player state word
`player[0x17]` is in `0xb4..0xb9`.

## The per-type behaviour table and the case table

The switch selector is **not** the entity type byte; it is the first word of the type's config record:

```
behaviour = *(int32_t *)(g_dat_53eac + entity[0xd] * 0x88)
```

so `DAT_53eac` is a `0x88`-stride table indexed by `entity[0xd]` (the .stg type byte, e.g. 0x01, 0x0b,
0x1f, 0x20, 0x24, 0x25, 0x29, 0x2a, 0x2e in stage 1) — the same records `fn_010efc` (the entity
constructor) reads. `behaviour` then selects the handler:

| behaviour | handler | size | extra arg |
|---|---|---|---|
| 0x00 | 0x017f13 | 3956 | |
| 0x01 | 0x018e88 | 4031 | |
| 0x02 | 0x019e48 | 4223 | |
| 0x03 | 0x01aec8 | 4063 | |
| 0x04 | 0x01bea8 | 4283 | |
| 0x05 | 0x01cf64 | 4134 | 2 |
| 0x06 | 0x01df8b | 4312 | |
| 0x07 | 0x01f064 | 4363 | |
| 0x08 | 0x020170 | 4139 | |
| 0x09 | 0x01cf64 | 4134 | 0 |
| 0x0a | 0x01cf64 | 4134 | 1 |
| 0x0b | 0x02119c | 4123 | |
| 0x15 | 0x0221b8 | 4067 | |
| 0x1f | 0x02319c | 6748 | 2 |
| 0x20 | 0x024bf9 | 6578 | |
| 0x21 | 0x0265ac | 5695 | 0 |
| 0x22 | 0x027bec | 6940 | 0 |
| 0x23 | 0x029709 | 5867 | 0 |
| 0x24 | 0x02adf5 | 6722 | 0 |
| 0x25 | 0x02c838 | 6803 | 0 |
| 0x26 | 0x02e2cc | 6993 | 0 |
| 0x27 | 0x02fe1e | 7279 | 0 |

Unlisted behaviour values (0x0c-0x14, 0x16-0x1e, >0x27) fall through with no handler.

Notes for the ports:

* `0x01cf64` is shared by behaviours 0x05 / 0x09 / 0x0a and receives the behaviour value as a second
  argument (2 / 0 / 1 respectively — note the remap: behaviour 0x05 passes **2**).
* `0x02319c` (behaviour 0x1f) passes 2; `0x0265ac`..`0x02fe1e` (0x21-0x27) all pass 0.
* That is 22 behaviour slots over **18 distinct handler functions** — all pending, all reported clean
  by the capstone dependency scan (no pending callees), i.e. each depends only on already-ported code.
* Porting order suggestion: start with the smallest (`0x017f13`, 3956 B) and keep one handler per
  session; the disassembly alone is ~900-1700 instructions each.
* Once at least one handler per needed behaviour exists, `0x031a8e` itself can be ported (it also
  unblocks `0x0322f5`, 133 B).

## Recon pass: 0x017f13 (behaviour 0x00) -- ready to port

`scratch/fnskel.py 17f13 0f64` (condensed skeleton; keep the script for the other 17 handlers).

* 3940 B, **978 instructions**, **55 calls**, and **every callee is already ported** -- confirmed
  against the STATUS `[x]` rows: `fn_01519f`/`fn_0151d5` (entity get-x / get-y), `fn_041f76` (abs32),
  `fn_041f7e`, `fn_041e9f` (PRNG), `fn_015a1f`, and the twelve action routines it dispatches to:
  `0x016959, 0x016a60, 0x016b8f, 0x016fec, 0x017218, 0x01730b, 0x0174a2, 0x017530, 0x017637, 0x01776f,
  0x017927, 0x017ca6`. So this handler is a pure composition of ported pieces -- no new leaves needed.
* Globals touched: only **DAT_00053c7c** (the player entity, 11 reads) and **DAT_00063354** (the current
  room node, 8 reads) -- everything else is entity-field access.
* Hottest entity fields: `+0x1f` x33, `+0xaf` x24, `+0xc3` x17, `+0xd` x13 (type), `+0x17` x13 (state),
  `+0xbf` x12, `+0x8b` x10, `+0xab` x8, `+0xc7`/`+0xcb`, `+0x113`/`+0x11b`, `+0xcf`; the type-config
  table is reached as `DAT_53eac + [e+0xd]*0x88` (fields `+0x3eac..+0x3f20` relative to the entity base
  register in the listing, i.e. the 0x88-stride record), and another table at `+0x1c9c/+0x1d5c`.
* Shape: a state machine over `[e+0x17]` that loads four register args (EAX/EDX/EBX/ECX) from stack
  locals `[EBP-0x54..-0x28]` before each action-routine call (same 4-arg ABI as `fn_016b8f`, which also
  returns two `int32_t *` out params at `[EBP+0x10]`/`[EBP+0x14]` -- re-check that per callee).
* Test angle that worked for the sibling handlers: drive one entity + the player (`DAT_53c7c`) and the
  current room (`DAT_63354`), pin the PRNG, then step the handler and assert the entity fields above
  rather than the screen.

## Campaign shortcut: the handlers are near-clones

`fnskel.py` on 0x018e88 (4031 B, 1067 insns, 59 calls) and 0x019e48 (4223 B, 1052 insns, 60 calls)
shows the **same** opening (get-x/get-y of self and of `DAT_53c7c`, three `fn_041f7e` sign tests,
`fn_015a1f`, PRNG `fn_041e9f`), the **same** twelve action-routine calls with the same 4-register ABI in
the **same order** (0x017218, 0x016a60, 0x016b8f, 0x016fec, 0x016959, 0x01730b, 0x0174a2, 0x017530,
0x017637, 0x01776f, 0x017927, 0x017ca6), and the same tail (`DAT_53eac + type*0x88` byte lookup,
abs32 distance tests, then `fn_01545e` for movement in the 0x01 case). Differences are in the state
machine around them (thresholds, extra PRNG draws, which fields get stamped).

Consequence for the campaign: port 0x017f13 first and build the shared skeleton once; the siblings become
diffs of the same 400-line pattern rather than fresh 1000-instruction ports.

## Port progress (updated 2026-07-13)

Ported handlers (6 of 18 -- 0x01cf64/behaviours 0x05+0x09+0x0a added: the MODE-in-EDX handler, a recon gate, and a script-row-based spot chooser; use `scratch/aicases.py` to diff the family per switch-case, and `scratch/gen_01cf64.py` shows the body-generation pattern).

Older progress: (5 of 18 -- 0x01bea8/behaviour 0x04 added: a split slot draw `v%%100<0x32 -> v%%8+2 (2..9)` else `prng%%2 (0/1)`, and the player-Y write parked before keep_x/keep_y):
  Use `python3 scratch/aicdiff.py <addrA> <addrB>` -- it canonicalizes Ghidra local names per function and drops rename/declaration-only hunks, which is the whole trick: a 45-hunk raw diff becomes the 1-2 hunks that actually differ.

Older progress: (4 of 18): **0x017f13** (behaviour 0x00), **0x018e88** (behaviour 0x01),
**0x019e48** (behaviour 0x02), **0x01aec8** (behaviour 0x03 -- the chooser alternates on entity[0x5f]
instead of drawing a slot, and its screen clamp runs before the too-close test: sibling ordering
must always be re-derived from disasm). The family's shared skeleton is now in `port/src/f/f.c` three times;
the fastest route to the rest is the pseudo-level diff against the previous handler (see
`scratch/`-free recipe below), not the instruction diff.

Recipe that worked for 0x018e88 and 0x019e48 (~10x cheaper than reading the whole function):
1. `python3` normalized diff of `analysis/pseudo/NNN_FUN_000XXXX.c` against the previously ported
   handler's pseudo (strip the `piStack_84 = (int *)0x...` noise lines and absolute labels first);
   the ratio is ~0.87 and the surviving hunks are the real differences.
2. Verify each hunk against `analysis/disasm/FUN_000XXXX.txt` (immediates, divisors, table
   offsets, and the ORDER of the clamp vs the distance test).
3. Where the previous handler's C body already matches, generate the new function from its text
   with targeted replacements (see the 0x019e48 generation), then hand-check the hunks.

Shared facts confirmed for all three handlers:
- Register/stack ABI: EAX = entity, EDX = rounded speed, EBX/ECX = player screen x/y,
  `[EBP+0x10]`/`[EBP+0x14]` = addresses of the handler's own two delta locals (mv_a / mv_b).
- Per-type record base DS 0x53e9c, stride 0x88: +0x04 and +0x80 = speed-float indices into the
  table at DS 0x51d5c (0.5, 0.75, 0.88, 1.0, 1.25, ...), +0x14/+0x18 = action timers,
  +0x84 = scripted-action timer, +0x1c..+0x7f = the 100-byte script row (DAT_00053eb8 view).
- Jitter table DS 0x51c9c, stride 0x10 per slot: (x lo, y lo, x hi, y hi) as signed offsets around
  the player for slots 0..5 (x spans 15/15/30/30/30/30, y span 10) and absolute screen rectangles
  for slots 6..9; no span is ever zero, so the `prng % span` idiv cannot fault.
- Tail is identical: mv_a -> fn_01545e, mv_b -> fn_01586b, entity[0x1b] = facing vs the player.

Remaining handlers (12): 0x01cf64 (shared by behaviours 5/9/10, arg 2/0/1),
0x01df8b, 0x01f064, 0x020170, 0x02119c, 0x0221b8, 0x02319c (arg 2), 0x024bf9, 0x0265ac, 0x027bec,
0x029709, 0x02adf5, 0x02c838, 0x02e2cc, 0x02fe1e, 0x03272f. Then the dispatcher 0x031a8e (1153 B)
and 0x0322f5 (133 B).

## Ported: 0x01cf64 (behaviours 0x05 / 0x09 / 0x0a) -- the first non-clone handler

Derived facts (from `analysis/disasm/FUN_0001cf64.txt`, size 4134, 1080 insns):

* **It takes a second argument that the other handlers do not.** The dispatcher sets **EDX** to the
  behaviour index flavour before the call: behaviour 0x05 -> `mov edx, 2` (0x00031d4c), behaviour
  0x09 -> `xor edx, edx` (0x00031d85), behaviour 0x0a -> `mov edx, 1` (0x00031d94); EAX = entity in
  all three. The handler stores it at `[EBP-0x04]` (0x0001cf7e) and reads it only at
  0x0001d32a / 0x0001d339 / 0x0001d348. So the port ABI is `fn_01cf64(uint8_t *e, int32_t mode)`
  and EDX is *not* the usual rounded speed here.
* Prologue matches the family: entity via 0x418ee, player screen x/y into `[EBP-0x30]`/`[EBP-0x2C]`,
  and (unlike most siblings) the entity's own screen x/y into `[EBP-0x10]`/`[EBP-0x0C]` are live.
* **The chooser (case 0x00, `e[0xaf] == 4`, `e[0x8f] != 1`, entity screen x in 0..0x13f, `e[0xc7] == 1`)
  is mode-driven, not jitter-table driven** (0x0001d32a-0x0001d436):
  - mode 0 -> local_64 = 0; mode 1 -> local_64 = 1; mode 2 -> `do { local_64 = (int)prng() % 2; }
    while (local_64 == e[0xff]); e[0xff] = local_64;`
  - `e[0x6f] = (local_64 == 0 || local_64 == 1)`; keep_x/keep_y = e[0xbf], e[0xc3];
  - local_64 == 0: `e[0xbf] = p_x - (uint8_t) g_dat_53eb8[ty*0x88 + e[0x113]*10 + 0]`
    local_64 == 1: `e[0xbf] = p_x + (uint8_t) g_dat_53eb8[ty*0x88 + e[0x113]*10 + 0]`
    (both then `e[0xc3] = p_y`) -- the offset comes from the type's SCRIPT ROW, not the jitter table.
  - clamp `< 0 -> 0x14`, `>= 0x140 -> 0x12c` (0x0001d436), then the usual too-close test
    (|dx| < 10 && |dy| < 5) restoring keep_x/keep_y, otherwise clearing the e[0xc7] dword.
* **case 0x00 opens with an entity[0x8b]-gated recon block** that the fn_018e88/019e48/01bea8
  siblings lack: `if (e[0x8b] == 0) { if (cfg(record +0x10) == 0) { four position calls with the
  arguments (p_x - 0x41) - v, p_x + v + 0x41, (p_y - 10) - v % 5, v % 5 + p_y + 10 } ... break; }`
  (disasm 0x0001d150-0x0001d250). Ported polarity: the probes chain requires `p_x-0x41-r <= screen_x <= p_x+0x41+r`
  and `screen_y >= p_y-10-r%5`; the fourth probe runs but its result is discarded and its compare
  (0x0001d1ea) is dead code; `[EBP-0x18]` is never written on that path (uninitialised in the
  original, pinned to 0).
* The rest of the state machine is the family's: the pseudo of 0x01cf64 contains exactly one
  `e[0xaf] =` assignment for each of 1, 2, 3, 4(x4), 5, 6, 8, 9, 10(x2), 0x14, 0x15, 0x16, i.e. all
  twelve action routines are reached with the same register ABI, and the tail (mv_a -> 0x1545e,
  mv_b -> 0x1586b, facing vs the player) is the same. case 0x0c differs from the siblings (no
  `prng % 0x14 == 2` split there -- see the 0x0001d.. case-0x0c block).
* Tooling for the next pass: `python3 scratch/aicdiff.py <addrA> <addrB>` canonicalizes Ghidra local
  names per function and drops rename-only hunks (it reduced 0x01bea8 vs 0x01aec8 to the chooser
  alone); for handlers that diverge as much as this one, prefer
  `uv run --quiet --with capstone python3 scratch/fnskel.py <addr>` and diff the condensed skeletons.
