# The actor hit-box model (fn_013c4d and friends)

`fn_013c4d` (0x013c4d, 1809 B, called only from `fn_01441e` at 0x0001472b) is the actor-vs-actor hit
test. Its shape is worth writing down once, because `fn_01441e` (pending) drives it per entity pair.

## Inputs

| arg | role |
|-----|------|
| EAX | A, the attacker; its animation frame index and facing pick the **hit box** |
| EDX | B, the victim; its animation record picks the **hurt box** |
| EBX | the shared ACT record for the pair (`rec[0]` = A's frame divisor, `rec+0x6d` = A's uint16 frame array) |
| ECX | a byte offset added to EBX -> the per-frame sub-record: `+0x3e` x nudge, `+0x44` contact offset, `+0x56` Y tolerance, `+0x5d` state of the effect actor to spawn |

## Gates (in order)

1. `byte B[0x30] == 1` - spawned this frame, inert.
2. B's animation record word `+4` / `+6`: B is intangible exactly while `word[+4] <= B[0x11] < word[+6]`.
   Frames outside that window are *not* rejected (a common mis-reading).
3. A in state `0x67`/`0x68` (the carry pair) only hits the entity linked in `B[0x4a]`.
4. `|B[9] - A[9]| <= byte(rec+off+0x56)` - the cheap Y gate; failing it returns 0 **without** the facing
   fix-up that other rejections perform.

## Boxes

Both sides come from the per-type sprite struct `g_dat_639ec[type]` as eight parallel dword arrays,
indexed by the frame slot (`A[0x11] % rec[0]` for A, `B[0x11] % anim[0]` for B, both signed remainders):

| offset | used for |
|--------|----------|
| `+0x14`, `+0x18` | B's two X corners |
| `+0x24`, `+0x28` | B's two Y corners |
| `+0x1c`, `+0x20` | A's two X corners |
| `+0x2c`, `+0x30` | A's two Y corners |

Facing (`A[0x1b]`) mirrors everything: facing 0 subtracts B's offsets and adds A's; facing 1 adds B's and
subtracts A's, and the two X pairs swap roles. Y values are anchored at `entity[9] + entity[5]`
(feet + record height), so the arrays are offsets from the feet, not from the sprite origin.

## Overlap is corners-or-straddle

Per axis the test first asks whether either of A's two corners falls inside B's span; if not, it accepts a
full straddle (A's left corner at or left of B's left **and** A's right corner at or right of B's right).
A big actor therefore swallows a small one rather than missing it. Both axes must pass.

## Success side effects

* contact X = `A[1] +/- byte(rec+off+0x44)`, where bit 7 of that byte means a **negative** offset
  (`v = (b & 0x7f) - 0x80`), clamped into B's X span.
* if `byte(rec+off+0x5d)` is non-zero, a type-`0xc9` (201) effect actor is allocated and built by
  `fn_010efc` at that X, `y = A[9] + dl + 1`, with `+0x30 = 1` and `+0x17` = that byte; its facing is
  copied from A, flipped when A is the player in state `0x38`/`0xa4`. Allocation failure is *not* checked
  by the original, which writes `+0x30`/`+0x17` through the null block.
* if A is the player in state `0x65`/`0x99` the animation record byte `+0x6c` decides: `1` puts A into
  `0xa1` (frame 0, word `+0x50` cleared) and mirrors B; any other value reacts only when A and B face the
  same way, going to `0xab` when `player[0x27] == 4` else `0xa0`. Both return **0** - the hit was consumed
  by the reaction.
* otherwise B is turned to face A (only when that agrees with B's current facing rule) and the function
  returns 1.

## The combat layer on top (fn_01441e)

`fn_01441e` (0x01441e, 3456 B, called from the `fn_0320f5` sweep) is the only caller of `fn_013c4d`. It
adds the three things a hit test does not cover:

* **Which attack.** The attacker's ACT record holds six slots; slot `i` is live while
  `word(rec[i*4]+8) <= frame < word(rec[i*4]+0xa)`. The first live slot wins; none means the animation is
  not on a hit frame and nothing happens. The attack type is `byte(rec + slot + 0x38)`, the damage is
  `dword(rec + slot*4 + 0x20)`, and `0x0b..0x14` is the "strong" class that unlocks extra reactions.
* **Damage bookkeeping.** Hit points are the word at `+0x29`; `+0x23` is a debt word and `+0x56` a dword
  overshoot accumulator for non-players. Byte `+0x30` becomes 1 when a hit is lethal and 2 while the debt
  word is still draining, and byte `+0x5a` marks the first death so the type-table sound (`+0x0c`) only
  plays once.
* **Reactions.** `byte(victim_record + 0x6c)` selects the "flagged" (player-style) reaction: state 0x1e,
  `-15` off `+5`, frame index `(v5+99)/3` with a mod-3 nudge, and knock-back steps through `fn_01545e`
  driven by the PRNG. Otherwise the victim just takes a stun word from `word(rec + slot*2 + 0x4a)` and a
  state from the `DAT_00051b60` attack-type table.

Both `DAT_00051b60` (attack type -> state) and `DAT_00051c68` (entity type -> sound slot, inside the
existing `g_blk_51c24` block at +0x44) are initialised image data, not runtime tables.
