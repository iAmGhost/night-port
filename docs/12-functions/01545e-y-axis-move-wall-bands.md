# 0x01545e — the Y-axis move with wall bands and trigger areas (L11, 1037 B)

Ported as `fn_01545e(uint8_t *e, int32_t d)` in `port/src/f/f.c`; test `test_fn_01545e`
(16 checks) in `port/test/test_f1.c`.

This is the **keystone of the movement/entity layer**: it is a direct callee of **21 still-unported
functions** (the per-type AI handlers and the entity frame update chain), and it is the **Y mirror of
`fn_01586b`** (already ported), which performs the same job on the X axis over the *same* table
`g_tab_5c8bc`.

## ABI

| reg | meaning |
|---|---|
| EAX | entity pointer (`uint8_t *`, the same block `fn_010efc` builds) |
| EDX | `d`, the Y delta for this frame — **`d == 0` returns immediately, touching nothing** |

Prologue is the usual `PUSH 0x2c; CALL 0x418ee` stack-guard; the args live in `[ebp-8]` (EAX) and
`[ebp-4]` (EDX) and the guard does not overwrite EDX, so both are inputs.

## Room band (the vertical play area)

Selected once, before anything moves, from the room Y centre `DAT_000639dc` (`fn_03cad3` fills it
from the room node):

| condition (evaluated in this order) | lo | hi |
|---|---|---|
| `[e+0x0d] == 0` | `centre + 0xf` | `lo + 0x122` |
| else `[e+0x17] == 2` | `centre - 0x64` | `lo + 0x208` |
| otherwise | `centre + 0xa` | `lo + 0x12c` |

## The band table pass (`g_tab_5c8bc`, count `g_dat_62a8c`)

Runs only when the kind byte `e[0] <= 1` (so bullets/special entities ignore walls entirely).
For entry `t = &g_tab_5c8bc[i]` (stride 0x14; the port already banks this struct from `fn_01586b`:
`{a1_lo, v_lo, a1_hi, v_hi, _pad}` — the original indexes it at `DAT_0005c8c0 + i*0x14`, i.e. shifted
by 4 relative to `DAT_0005c8bc`, which is why the X gate reads `v_lo/v_hi` and the Y band reads
`a1_lo/a1_hi`):

1. X gate: `[e+9]` must be **strictly** inside `v_lo..v_hi` (the entity stands in this column).
2. Y gate: the **proposed** Y `[e+1] + d` must be strictly inside `a1_lo..a1_hi`.
3. If both hold the delta is **rewritten**:
   * `d > 0`  → `d = a1_lo - [e+1]`
   * `d <= 0` → `d = a1_hi - [e+1]`
   Note the rewrite ignores sign consistency: an upward (`d <= 0`) move that proposes a point inside
   the band is rewritten to land on `a1_hi`, which can point the move back downward. That is what the
   original does (0x15638-0x15644) and the port keeps it.
4. `d` is a plain loop local, so the **last** matching entry wins, and the rewritten delta is what
   the position update below uses.

## The "caught" latch (repeated verbatim at four sites: 0x155ad, 0x15671, 0x1573b, 0x157f4)

Condition: `[e+0x17] == 0x16` **and** `e != DAT_00053c7c` (the player, set by `fn_0376c8`) **and**
`0 <= [e+0x11] <= 0x15`. Effects:

* `DAT_00063364 = 0x12`, `DAT_00063368 = 1` — the same level-event latch `fn_031f5f` writes.
* Three **DWORD** stores: `[e+0x17] = 0x20`, `[e+0x11] = 0`, `[e+5] = 0`.
  Ghidra's pseudo shows a dozen byte writes (`0x17..0x1a`, `0x11..0x14`, `5..8`); the machine code is
  three 4-byte stores (`mov dword ptr [eax+0x17], 0x20` at 0x155c1 etc.), so the neighbouring bytes
  zero only as a side effect. The port stores dwords, which also dodges the widened-struct overlap
  problem in the port model.
* If the entity is asleep (`e[0] == 0`) it is woken to `1` — **never** the player.

## Applying the position

* Old Y **outside** `[lo, hi]` → `[e+1] += d`, done (no clamp at all — 0x156c9-0x156e8). This is how
  an entity that spawned outside the band can travel freely through it.
* Old Y **inside** → `[e+1] += d`, then hard-clamp to `lo` (0x156f6-0x1570b) or `hi`
  (0x157af-0x157c4). A clamped entity that satisfies the "caught" condition is additionally nudged by
  `(prng % 0x29) - 0x14` — the original uses `mov ebx,0x29; idiv`, so it is a *signed* remainder;
  `prng_next()` returns a 15-bit non-negative value so C's `%` matches, giving a −20..+20 jitter.

## Test notes

* The band expectations must respect the gate order: a band only bites when the *proposed* Y is
  inside it, and the clamp only happens when the *old* Y is inside the room band — a test that sets
  the old Y outside the band sees a plain `+= d` instead (that asymmetry is asserted explicitly).
* The catch writes `[e+5]` as a dword, so the test pre-fills `0x5a5a5a5a` and asserts the whole word
  becomes 0.
* The jitter is asserted as a range (`lo ± 20`) because `prng_next()` state is shared with other
  tests; the exact value is not pinned.
