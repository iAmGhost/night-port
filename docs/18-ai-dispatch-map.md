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

## SHARED FEATURE: THE RECON GATE (found the hard way -- check it on every handler)

Three of the if-chain handlers -- 0x029709, 0x02fe1e and 0x02319c -- open the action-dispatch region
with a gate the first porting pass dropped: `if (entity[0x8b] == 0) { entity[0x8b] = 1; goto tail; }`
at 0x00029b42 / 0x0003078d / 0x00023902. Effect: the first frame in that state does NOTHING except
raise the flag -- no chooser, no walk start, no entity[0xab] bookkeeping -- then jumps to the move
tail. 0x01bea8 and 0x0265ac genuinely have no gate (they only clear the flag), and 0x024bf9 has none
either; that inconsistency is what made the copies look right. Before declaring a handler understood,
grep its disassembly for `cmp dword ptr [eax + 0x8b], 0`, and do NOT preset entity[0x8b] = 1 in every
test case -- pin both frames instead (see test_fn_029709's recon-gate checks).

### Ported: 0x02adf5 (behaviour-0x24)
The cheapest sibling job so far (97.8% of its instructions matched 0x024bf9): only 37 new instructions to
read, all in the swing -- a frame-1 facing latch (entity[0x1b] -> entity[0x87]) and the facing-consistency
veto at 0x0002b317-0x0002b37f.  Two family-wide facts this pinned: the ambush gate is armed by sprite
state 5, but that arm ends in a `return`, so the gate is evaluated only on a LATER frame; and when the
gate fires it stores entity[0xab] = 0 and enters state 0x0e, whose rung decrements it in the same frame,
so the '0' sprite (0x30) appears immediately.

### Ported: 0x024bf9 (behaviour-0x20)
Generated from fn_02319c; only two regions needed a fresh read (the six-slot chooser 0x0002549f-0x0002571e
and the swing's frame-0 latch/window).  Family facts it adds: the DS 0x51c9c approach rectangles are
{x0,y0,x1,y1} with stride 0x10 -- slot 2 = (-80,-30,-50,-20), 3 = (50,-30,80,-20), 4 = (-80,20,-50,30),
5 = (50,20,80,30) -- and here BOTH axes take an absolute-valued prng remainder (0x01bea8 left y = p_y).
It also confirms the recon gate is per-handler: 0x024bf9 has none.

## Port progress (updated 2026-07-13)

Ported handlers (12 of 18 -- 0x0265ac/behaviour 0x16 added, the first if-chain member. Its shared skeleton was confirmed by an instruction-stream diff against FUN_0001bea8 (693 of 1326 instructions shared once immediates are normalised); the room-band clamp + landing tests + move tail splice verbatim from the fn_01bea8 body, so only ~630 instructions had to be read: the 200px bail to sprite 0x2f, the entity[0x7f] ambush gate that re-enters the range chain in the same frame, the '2'/'1'/'0' countdown ladder (states 0x0c/0x0d/0x0e -> sprites 0x32/0x31/0x30), the grid probe forced to 4 by an attacking player (aim 0x46 past the player, state 0x6a), and state 0x6a's re-derivation of the step from the DS 0x51d68 float. NOTE the stack map differs: the per-frame step is [EBP-0x2c] and the grid result [EBP-0x44], so Ghidra's pseudo local names must not be trusted across handlers.

Older: (11 of 18 -- 0x02119c/behaviour 0x0b added: entity[0x5f] latch WITHOUT a re-roll loop (player-offset rect for slots 0/1; slot 0xb/0xa chosen by the player's screen half and drawn from the absolute rect with a SIGNED remainder), and a case-0x0b walk with no prng()%0x14 coin flip on failure).

Older progress: (10 of 18 -- 0x0221b8/behaviour 0x15 added: 0x020170's draw with a SINGLE spot arm (X = the slot's jitter rectangle offset by the player, Y = the player's own Y); the entity[0xc3] pre-write for slots 0/1 is redundant but faithful).

Older progress: (9 of 18 -- 0x020170/behaviour 0x08 added: draw prng()%4 + 2/3 sub-draws (slots 0..5 only), script-row offsets for 0/1, player-offset jitter for 2..5, NO absolute arm, no 1-in-6, clamp before the distance test).

Tail equivalence method: a mnemonic-only stream comparison of the disasm from the room-band clamp on (257 instructions, FUN_00020170 @0x206f5 vs FUN_0001bea8 @0x1c4bb) shows ZERO differences, which is what licenses reusing the fn_01bea8 C body verbatim for the shared part of a family handler.

Older progress: (8 of 18 -- 0x01f064/behaviour 0x07 added: a four-arm chooser (script-row offsets for slots 0/1, player-relative jitter for 2..5, absolute for 6..9) with the clamp before the distance test and the restore jumping past it).

Older progress: (7 of 18 -- 0x01df8b/behaviour 0x06 added: the entity[0x5f] phase-latch chooser (script-byte side spots alternating with absolute jitter slots 6..9)).

PORTED (see STATUS 0x02319c) -- the recipe below is what produced it, kept as a worked example.

READ-IN-PROGRESS NOTES -- 0x02319c (behaviour 0x1f, 6748 B, dispatcher arg 2). 98.9% of its 1746
instructions are shared with 0x029709 in order; only 9 substitutions and 9 inserts (252 insns), so it is
a body-generation job. Substitutions (all inside the rope arm, whose sprite is 0x2c not 0x2b): the state
counter is compared against 0x48 instead of 1 (latch frame = 0x48, window 0x49..0x71), the numerator is
player_x - self_x, and the floats land in entity[0xdb] (slope dy/dx, 100000.0f when dx==0), entity[0xe3]
and entity[0xe7] instead of entity[0xe7]. The arm is a SWING, not the 0x02fe1e climb:
  latch frame (0x0002348b-0x0002368b): latch entity[0xd3]/[0xd7]=self, [0xcb]/[0xcf]=player, slope
    entity[0xdb]=dy/dx; then ax=|dx|, ay=|dy| (fn_041f76); if ax>ay: entity[0xe3] = (dx<0?-7.0f:+7.0f)
    (0xc0e00000/0x40e00000) and entity[0xe7] = (float)dy / <float DS 0x14 or 0x18 by the sign of dx>;
    else: entity[0xe3] = (float)dx / <float DS 0x1c or 0x20 by the sign of dy> and entity[0xe7] =
    (dy<0?-4.0f:+4.0f) (0xc0800000/0x40800000).  [the four float divisors are DS 0x50014/0x50018/0x5001c/0x50020 = 31.0f]
  window frames (0x000236aa-0x00023866): if |entity[0xe7]| < |entity[0xe3]| (fabs/fcomp) then
    tx = rndint((e[0x11]-0x48) * entity[0xe3]); ty = rndint(tx_pre * entity[0xdb]);
    else ty = rndint((e[0x11]-0x48) * entity[0xe7]); tx = rndint(ty_pre / entity[0xdb]);
    then tx += entity[0xd3]; ty += entity[0xd7].
  containment (0x00023762): if (tx<0 && entity[0xd3]>0xa) || (tx>=0x140 && entity[0xd3]<0x136) -> NUDGE;
    else if (room[0x12]+g_dat_639e0 > ty) || !(room[0x16]+g_dat_639e0 >= ty) -> NUDGE; else full move
    (fn_01545e(e, tx-fn_01519f(e)); fn_01586b(e, ty-fn_0151d5(e))).
    NUDGE = fn_01545e(e, sign(tx-entity[0xd3]) * 3) then fn_01586b(e, sign(ty-entity[0xd7]) * 1)
    (only called when the difference is nonzero).
  WALK REGION NOW READ (0x00024712-0x00024a27), so 0x02319c needs no further reading:
    * the guard at 0x0002474b (`cmp eax,-1` right after `and eax,0xff`) can never match, so the whole
      block 0x00024757-0x00024840 is DEAD CODE -- the live walk start is LAB 0x00024894;
    * reached = |screen_x - p_x| < col0 && |screen_y - p_y| < 10 (ten, not the family's six);
    * on advancing to the next row byte it ALSO latches the player anchors: entity[0x17]=byte,
      entity[0x11]=0, entity[0xcb]=p_x, entity[0xcf]=p_y, cursor++ (the 0x00024825 insert);
      otherwise action 0x0a + state 0;
    * not reached -> fn_041e9f(), `idiv 0x14`, remainder == 2 -> action 4 else action 0x0a, state 0
      (same 1-in-20 coin flip as fn_029709, no change needed);
    * entity[0xab] >= 0 -> dec and sprite 1; the 0x0c rung is the shared '2' ladder (sprite 0x32 on
      expiry, entity[0x11]=0, action 0x0a, state 0).
  Port recipe: take the fn_029709 body, swap the 0x2b rope arm for the swing above (its four float
  divisors are the 31.0f constants in g_blk_50010), and swap the walk block for the version above. Walk findings so far: the code at 0x00024757 is
  DEAD (its guard is `cmp eax,-1` right after `and eax,0xff`, so it can never match); the live walk
  start is 0x00024894, which reads the tolerance from col 0 and tests the X gap ONLY; the not-reached
  path (0x00024842) calls fn_041e9f, then `idiv 0x14` and picks action 4 when the remainder == 2 else
  action 0x0a (so this handler keeps a 1-in-20 coin flip with a different helper than 0x02119c).
  CAUTION when reading the listing: `mov eax, dword ptr [ebp-8]` lines are load-bearing (they restore the
  entity pointer between table-indexed accesses) -- do not filter them out when re-reading.

INSTRUCTION-STREAM TOOL (scratch/aidiff_insns.py, use this instead of the pseudo diff): pass two VAs.
It disassembles both with capstone, masks EVERY immediate/displacement, and SequenceMatchers the
mnemonic stream (so stack-slot renames and branch-target shifts vanish); `--consts` then re-compares
the matched pairs with only branch/call targets masked, i.e. prints the genuine constant
substitutions. Measured: 0x02fe1e contains 100% of 0x029709's 1509 instructions (2 inserts, 5
substitutions); 0x02e2cc/0x02319c 98.9%; 0x02c838 96.2%; 0x027bec 95.7%; 0x02adf5/0x024bf9 93.8-93.9%;
only 0x03272f is an outlier (11.4%). Read the inserts and substitutions only.

SIBLING-DIFF SHORTCUT (used for 0x029709, use it for the rest): the if-chain handlers are near-clones.
Normalise each disassembly to `mnemonic + operands-with-immediates-masked`, run difflib
SequenceMatcher against an already-ported sibling, then read ONLY (a) inserted blocks, (b) deleted
blocks, (c) aligned pairs whose difference survives stripping branch targets and [ebp-...] slot renames.
For 0x029709 vs 0x0265ac that is 1213/1342 shared, two inserted blocks (0x000299e1-0x00029aa8,
0x0002a9ba-0x0002aac1) and exactly one semantic substitution (probe fn_0161f9 vs fn_015b5b) -- so the
port is the sibling body plus four edits. Family variants pinned so far: the X clamp sits AFTER the
too-close restore in 0x0265ac (both paths clamped) but BEFORE it in 0x029709 (restored X stays raw);
0x0265ac has an entity[0x5f] two-arm latch (side slots vs absolute jitter 6..9) while 0x029709 draws
only prng()%2 side slots; a script row whose col-1 byte is '.' (0x2b) drops the walk's 6 px Y
tolerance in 0x029709. RESOLVED, do not re-derive: the family's x87 divisors are ordinary initialised image data. Capstone prints DS-relative displacements without Ghidra's +0x50000 image base, so `qword ptr [0x44]` is DS 0x50044. Pool (scripts/extract_ds.py 0x50010 192 -> g_blk_50010): DS 0x50014/0x50018/0x5001c/0x50020 = float 31.0 x4, 0x50044 = 20.0, 0x5009c/0xa4/0xac = 30.0, 0x500b4/0xbc/0xc4 = 27.0, 0x500cc = 20.0.

HANDLER FRONTIER (recomputed with a corrected pending/callee scan, scratch/frontier-style): every one of
the 9 remaining handlers is gated ONLY on folded helpers (0x0418ee canary prologue, 0x041e9f prng,
0x041f76 abs, 0x041f7e modulo) plus 0x01586b / 0x017927, which are BOTH already ported. None of them is
waiting on real work, so the AI campaign is a straight reading queue, ordered cheapest first:
0x029709 5867, 0x024bf9 6578, 0x02adf5 6722, 0x02319c 6748 (takes arg 2), 0x02c838 6803,
0x027bec 6940, 0x02e2cc 6993, 0x03272f 7013, 0x02fe1e 7279. Markers show they are the same family with
an if-chain instead of a switch: all 9 call the same 12 action helpers, all have the single
`entity[0xc7] == 1` chooser gate and the `entity[0xff]` slot store, all read the script rows far more
heavily than the switch family (9-13 DAT_00053eb8 reads vs 3), and 0x0265ac additionally uses the
entity[0x5f] latch (4 reads). Downstream of them sit 0x031a8e (needs all of them) then 0x0322f5 (133 B
per-frame AI loop) and 0x03a829 (the frame driver), so the handler queue is the critical path to a
loading stage.

FAMILY TEMPLATE STATUS (scratch/aicases.py, comparing each pending handler's canonical pseudo case-by-case against fn_01bea8): 0x01f064, 0x020170, 0x0221b8 differ ONLY in case 0x00, and 0x02119c also differs in case 0x0b -- so those five are body-generation jobs (pattern: scratch/gen_01df8b.py) rather than fresh reads. 0x02319c, 0x024bf9, 0x0265ac, 0x027bec (and probably the other 6-7 KB entries) have no `switch(` in their pseudo at all -- they use if-chains, so they must be read on their own.

Older progress: (6 of 18 -- 0x01cf64/behaviours 0x05+0x09+0x0a added: the MODE-in-EDX handler, a recon gate, and a script-row-based spot chooser; use `scratch/aicases.py` to diff the family per switch-case, and `scratch/gen_01cf64.py` shows the body-generation pattern).

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

Remaining handlers (6 of the table + 0x03272f): 0x01cf64 (shared by behaviours 5/9/10, arg 2/0/1),
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
