# 0x02e2cc (behaviour-0x26, 6993 B, 1815 insns) — analysis in progress

Generated target: `fn_02e2cc` from the **fn_02c838** body (95.6% of 0x02e2cc's instruction stream aligns
with 0x02c838 in order; `uv run --with capstone python3 scratch/aidiff_insns.py 0x02e2cc 0x02c838`).

## A. Constant substitutions (from `--consts`, A = 0x02e2cc, B = 0x02c838)

| what | B (ported) | A (0x02e2cc) | note |
|---|---|---|---|
| drop arm sprite | 0x2a | **0x2c** | A 0x0002e5a4 |
| drop latch frame | 0x32 | **1** | A 0x0002e5b1 |
| drop divisor | DS 0x5005c (20.0) | **DS 0x5007c (20.0)** | A 0x0002e60d |
| drop window | 0x33..0x46 | **2..0x14** (`cmp 1` / `cmp 0x14`) | A 0x0002e621/0x0002e62a |
| drop k expression | `frame - 0x32` | **`frame` directly** (`fild dword [e+0x11]`, no sub) | A 0x0002e635 |
| swing arm sprite | 0x2c | **0x2d** | A 0x0002e671 |
| swing window upper | 0x32 | **0x37** | A 0x0002e76a |
| swing divisors | DS 0x50064/0x5006c/0x50074 (all 50.0) | **DS 0x50084, 0x50084, 0x5008c, 0x50094 (all 55.0)** | A 0x0002e7c5/0x0002e7fe/0x0002e852/0x0002e8a3 |
| ambush distance | 0x3c | **0x46** | A 0x0002e937 |
| chooser draw | `prng()%4 + 6` | **`prng()%8 + 2`** | A 0x0002eaeb |
| chooser sub-draw | +6 base | **+2 base** | A 0x0002eaf7 |
| landing offset on e[0xb3] | ±0x46 (70) | **±0x32 (50)** | A 0x0002f527/0x0002f533 |
| proximity test | 0x4b (75) | **0x37 (55)** | A 0x0002f6ba |
| frame locals | p_x=[ebp-0x34], p_y=[ebp-0x30] | **p_x=[ebp-0x30], p_y=[ebp-0x2c]** (frame is 4 B smaller: `push 0x88`, `sub esp,0x68`) | cosmetic only |

## B. Structural diffs (opcode-summary regions, A side unless noted)

1. **Chooser rewritten** — A 0x0002eacc-0x0002eb56:
   ```
   if (prng() % 100 < 50) {                 /* 0x0002eacc-0x0002eae2, signed idiv */
       slot = prng() % 8 + 2;               /* 0x0002eae4-0x0002eafa */
       if (slot == 2) slot += prng() % 2;   /* 0x0002eb03-0x0002eb16 -> 2 or 3 */
       else if (slot == 3) { slot = 4; slot += prng() % 2; }   /* 0x0002eb1b-0x0002eb3b -> 4 or 5 */
   } else {
       slot = prng() % 2;                   /* 0x0002eb40-0x0002eb53 -> 0 or 1 */
   }
   if (slot == e[0xff]) goto redraw;        /* 0x0002eb56-0x0002eb62 */
   ```
   i.e. six slots 0..5 (0x01bea8/0x018e88 shape with different arithmetic), no entity[0x5f] phase latch.
   The slot *arms* after it are the same 0/1, 2..5, 6..9 arms as 0x02c838 **but slots 6..9 are then
   unreachable** — verify before generating (read A 0x0002ec51..0x0002ed80).
2. **X clamp BEFORE the too-close test** (B: after). Proved by an 11-insn delete at A 0x0002ee26-0x0002ee5a
   matching an 11-insn insert at B 0x0002d3bc-0x0002d3f1: A = clamp (0x0002ee26-0x0002ee56) then the
   too-close test (0x0002ee5d+); B = test then clamp. So swap the two blocks of the fn_02c838 chooser
   and keep `e[0xc7] = 0` on the accepted path (the paired substitution at A 0x0002eeb7 vs B 0x0002d3f6 is
   an alignment artifact of this reorder, re-check).
3. **Extra 31-insn decision chain** at A 0x0002f40c-0x0002f467: a chain over the *player's* sprite
   (`g_dat_53c7c[0x17]` == 0x16/0x17/0x18/0x1a/0x1d/0x1e/0x20, first tested on the entity's own
   `e[0x17] == 0x16`) that sets the transition local `[ebp-0x40] = 4` (fallthrough leaves 2). The consumer
   block right after (0x0002f467+) reads that local: 2 → `e[0x1f]=1, e[0xab]=0xa`; 1 → `e[0xaf]=4,
   e[0x1f]=0`; 3 → `e[0xaf]=4`; **need to read whether a `== 4` arm exists** (A 0x0002f4da..0x0002f551).
4. **Extra 9-insn marker gate** at A 0x0002f96f-0x0002f993: `if (script_col1(row) != '-') goto 0x0002fabd;`
   — a `'-'` (0x2d) marker row gate around the block 0x0002f999..0x0002fabd (walk/run related, need read).
5. RESOLVED: the ordinary walk marker stays **',' (0x2c)** (A 0x0002f96a `cmp eax, 0x2c` matched 0x02c838
   with no substitution), and the new 9-insn block adds a SECOND gate testing `'-'` (0x2d) at
   A 0x0002f990 -- `if (script_col1 != '-') goto 0x0002fabd;` skipping the block 0x0002f999..0x0002fabd.

## C. Recipe for the next session

1. `sed -n` the fn_02c838 body out of `port/src/f/f.c` (starts at `void fn_02c838(uint8_t *e)`, ends at the
   next `\n}\n`), follow `scratch/gen_02c838.py` as the template, and write `scratch/gen_02e2cc.py`.
2. Apply section A substitutions, then the two chooser changes (draw + clamp/test order) and the two new
   blocks (B3, B4) after reading their seams (addresses above).
3. Test: model on `scratch/test_02c838.c` (drop/swing/ambush/walk checks; remember `g_dat_62a8c = 0` for
   exact moves, re-arm `entity[0xc7]` each chooser frame, and that the ambush gate's state 0x0e rung runs
   in the same call so only `entity[0xaf] == 0x0a` + the consumed gate are assertable).
4. `make -C port test` twice + ASan build (`cc -std=c11 -O1 -g -fsanitize=address -o /tmp/asan26
   test/test_f1.c src/f/f.c src/f/fx_04794a.c src/sys/sys.c -lm`).
5. After 0x02e2cc: only **0x03272f** (behaviour-0x1c? outlier, 11.4% shared) remains before the dispatcher
   **0x031a8e + 0x0322f5** can link.

## D. RESOLVED (from the previous context, do not re-derive)

- Item B3's consumer has a **flag == 4 arm** at A 0x0002f4da-0x0002f551: `e[0xb3] = fn_01519f(player)`,
  `e[0xb7] = fn_0151d5(player)`, then `e[0xb3] += 0x32` when `fn_01519f(player) - fn_01519f(e) < 0`
  else `e[0xb3] -= 0x32`, then `e[0xaf] = 0`, `e[0x1f] = 0x6a`.  Arms 2/1/3 (0x0002f467-0x0002f4da) are
  shared with 0x02c838 (there the +-0x46 offsets sit in one of those arms).
- The flag-4 setter chain (A 0x0002f40c-0x0002f467) is an `if/else if` ladder over the entity's own sprite
  (`e[0x17] == 0x16`) then the player's (`pl[0x17]` in {0x17,0x18,0x1a,0x1d,0x1e,0x20}); a match jumps to
  the store of 4, otherwise the local keeps the value the shared code computed.

## E. CORRECTION (found by the test, ported 2026-07-13)

Item B1's claim that the draw yields only slots 0..5 (and so the 6..9 rectangle arm is dead) is WRONG.
`idiv` remainders keep the dividend's sign and `prng_next()` is read as a signed int, so `prng % 8 + 2`
spans **-5..9**: slots 6..9 are live and negative slots make the 2..5 arm read the DS 0x51c9c rectangles
several strides early.  The port keeps all four arms; the test pins -5..9, the +-col0 mirror of the side
slots, entity[0x6f] == (slot in {0,1}) and the no-repeat re-roll.
