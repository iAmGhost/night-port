# 319. Item G: the health purchase read a byte offset as an element index, so one purchase jumped the HP term from 100 to 500 and then to 0

Goal task-5 (round-4 item G: "HP is drawn as stacked bars (red, yellow, …) and a single purchase should step
2 bars, but one purchase gained more than that").

## 1. Red, measured on the unfixed build

`scripts/rig/drives/ep1_buy_g.txt` (arm-2 room clears: `poke 0x6336c 2` → fn_03a829's loop tail → fn_03d244 →
`fn_038f3d(1)` at 0x3d2ac; EXP injected with the documented player-record poke `play 0x2b 5000`, docs/118;
`NIGHT_BUYDBG=1 NIGHT_BUYDBG_ROW=1`), pinned env (`NIGHT_SEED=13579 NIGHT_RIG_STEP_CALLS=665 NIGHT_RIG_FPS=60`,
skip-intro ON), `/tmp/rig/g319h/port`:

```
[buy] row=1 sel 0->1 cost=600  exp 5000->4400  hp23 100->500  level28 0->0  max56 0->0
[buy] row=1 sel 1->2 cost=2000 exp 4400->2400  hp23 500->0    level28 0->0  max56 0->0
buydbg census: attempts=2 committed=2 rejected_max=0 rejected_poor=0 refund=0
```

The level steps by one per purchase ✓ but **the pending-HP term `P[+0x23]` jumps 100 → 500**, and the next
purchase takes it to **0**.  With the shipped's bar quantum (`mov edx, 0x32` = 50, 0x39b16) that is +8 bars for
the first purchase and the bar gone for the second — the reported "more than 2 bars", and worse than reported.

## 2. Root cause, from the shipped's bytes

`fn_038f3d`'s purchase block, rows 1 and 4 (0x3954f-0x39558 selects them):

```
0x39513  mov  eax, P                    ; [0x3c7c]
0x39518  mov  edx, [ebp-0x60]  ; page          ; 0x3951b shl edx,2
0x3951e  mov  ebx, [edx + ebp - 0x11c]  ; sel[page]
0x39525  shl  ebx, 2                             ; BYTE offset sel*4
0x39528  imul edx, [ebp-0x60], 0x14             ; BYTE offset page*0x14
0x3952c  add  edx, ebx
0x3952e  mov  edx, [edx + 0x3c94]               ; cost  (element page*5 + sel on a dword table)
0x39534  sub  [eax + 0x2b], edx                 ; pay
0x39547  inc  dword [eax + ebp - 0x11c]         ; ONE level per purchase
0x3956e  mov  edx, [ebp-0x118]  ; sel[1]   ; 0x39574 shl edx,2
0x3957c  movsx eax, word [P + 0x23]
0x39580  cmp  eax, [edx + 0x2738]               ; HP table entry, element sel[1]
0x395d0  mov  dx, word [edx + 0x2738]
0x395d7  mov  word [P + 0x23], dx               ; the bar count follows this entry
```

`g_tab_52738` is a `uint32_t[]` in the port, so the shipped's *byte* offset `sel*4 + 0x2738` is element
`sel[1]`.  The port had:

```c
int32_t ship = (int32_t)g_tab_52738[(uint32_t)sel[1] * 4u];   /* four entries too far */
```

Element 1 is 200 ✓ the port read element 4 = 500 ✓; element 2 is 300 ✓ the port read element 8 = `0x3f000000`,
whose word is **0** ✓✓ — exactly the two red lines.  The same mistake sat in the refund at 0x39588-0x395a9:

```c
+ (int32_t)g_dat_53c94[(uint32_t)sel[page] * 4u + (uint32_t)page * 0x14u];   /* element form: page*5 + sel */
```

Both fixed to the element form (`g_tab_52738[sel[1]]`, `g_dat_53c94[page*5 + sel[page]]`); the cost read at
f.c:26564 already used the right form, which is why the *prices* were never wrong and only the HP write was.

## 3. Green, same drive, same env, `/tmp/rig/g319i/port`

```
[buy] row=1 sel 0->1 cost=600  exp 5000->4400  hp23 90->200   level28 0->0  max56 0->0
[buy] row=1 sel 1->2 cost=2000 exp 4400->2400  hp23 200->300  level28 0->0  max56 0->0
buydbg census: attempts=2 committed=2 rejected_max=0 rejected_poor=0 refund=0
```

One purchase = one level (`sel 0->1->2` ✓) and the HP term moves by exactly the table row (+100 each time ✓),
which at the shipped's 50-unit bar quantum is **+2 bars** ✓ — the playtester's expectation.  `make test`
is ALL PASS with the new fixture.

## 4. The tables, dumped from the shipped

Read out of the shipped's own DS window (`/tmp/rig/rb15/orig/state_0000.ds.bin`, flat 0x16000 bytes at
`DS - 0x50000`; identical in samples 0000, 0001 and 0411 — these are load-time constants):

| cell | shipped value | meaning |
|---|---|---|
| `DAT_00052738[0..4]` | `100, 200, 300, 400, 500` | per-level HP term written to `P[+0x23]` (0x395d0-d7) and to `P[+0x56]` as `& 0xffff` (0x3778b-0x3779c) |
| `DAT_00053c94` rows 0-4 | `300,1000,4000,5000,5000` / `600,2000,4000,5000,5000` / `2000,5000,…` / `5000,5000,…` / `1000,1000,…` | exptab.dat's 6×5 cost table (row stride 0x14 bytes = 5 dwords) |
| `DAT_000531b0[0..5]` | `4, 4, 2, 1, 1, 1` | per-row level caps, the `selMax[page]` gate at 0x39505 / 0x39617 |
| `DAT_00062aa0[0..4]` | `0x19d140, 0x19d6e8, 0x19f03c, 0x19f5b4, 0x23b038` | **guest heap pointers**, the key-table selector picked by `sel[0]` (0x39a6b-0x39a7a) |
| `DAT_00062ab4[0..4]` | `0x24b110, 0x24b5f8, 0x24bb58, 0x24c0b8, 0x24c618` | **guest heap pointers**, the catch-table selector picked by the same row (0x39a7f-0x39a8e) |

**Correction to docs/312.** That note called `DAT_00062a98 / 0x00062a9c` "the per-step HP pair from
`DAT_00062aa0[sel[0]]` and `DAT_00062ab4[sel[0]]`".  They are the *active key table* and *active catch table*
pointers (the port already declares them that way: `extern uint8_t *g_dat_62a98; /* alias of g_dat_62aa0 */`),
and the shipped's live values were `0x19d140 / 0x24b110` — pointers, not HP numbers.  The health arithmetic lives
in `DAT_00052738`, not in that pair.

`fn_038f3d`'s commit path (0x39a56-0x39b10) re-read from bytes: `P[+0x27] = sel[0]` (skipped when unchanged, and
then the two table pointers are re-selected), `P[+0x28] = sel[1]` unconditionally, `P[+0x26] = sel[2]` when
changed (plus `DAT_53818 = [sel[2]*4 + 0x3c28]` and `P[+0x123] = DAT_53818`), `P[+0x25] = sel[3]` ✓ — matches
f.c's commit block store for store.

## 5. Fixture (contract: "dumped from the shipped and printed in a fixture")

`test/test_f1.c` scenario C inside `test_fn_038f3d`:

```
[PASS] 038f3d's HP table DAT_00052738 rows 0..4 = 100,200,300,400,500 (shipped DS window read)
[PASS] 038f3d row-1 purchase raises the level byte player[+0x28] 0 -> 1 (one step per purchase)
[PASS] 038f3d row-1 purchase writes player[+0x23] = DAT_00052738[1] = 200, the shipped's element
       (the byte-offset-as-index bug gave 500)
[PASS] 038f3d spends the row-1/column-0 cost 600 from DAT_00053c94
```

The scripted keys are `Down` held on ticks 1-3, released at 4, `confirm` at 5-6, `Esc` at 7-8: fn_038f3d's own
key waits (`fn_00035d5d` at 0x391d6 / 0x39491 / 0x3964a) consume scripted ticks, and a single-tick press was
never seen by the cursor test — the first fixture version failed all three purchase checks.

## 6. Rig work this item needed, and what it taught

* **`menurow <ms> <pattern>` (new drive verb, `PT_MENU_ROW`).** `menu` posts Enter/Esc only, so a purchase always
  landed on row 0.  The verb holds (without rebasing, docs/314's -60000 ms slide) up to 15 s for the level-up
  screen to open after the request lands, then posts its keys.  Its off-branch (`menu`) is unchanged.
* **Row targeting is cadence-bound, and none of the macro's cadences land row 1.** An 800 ms arrow hold moves the
  cursor **twice** (the shipped repeats a held row key every 0x13 screen iterations, 0x396d4-0x39767):
  `/tmp/rig/g319d` bought on row 2.  A 400 ms or 0 ms hold moves it **not at all**: `/tmp/rig/g319e`,
  `/tmp/rig/g319f`, `/tmp/rig/g319g` produced three purchases, all on row 0.  Rows 1 and 4 are the only rows
  that take the HP branch, and the Down cycle is `0→1→2→3→0` (page 4 clamps to 0 when `variant == 0`, 0x39714-1a),
  so an even number of moves can never reach row 1.  The measurement therefore used the declared probe below.
* **`NIGHT_BUYDBG_ROW=<n>` is a probe, not shipped behaviour.** It parks the row cursor at entry so the
  purchase arithmetic on row 1 becomes reachable; the witness prints a line saying so at every screen entry, and
  every claim in §1/§3 is labelled as a probe-armed, poke-armed claim.  The shipped's own cursor remains
  reachable only by held arrows — the playtester's hands do that trivially, a scripted drive does not.
* **`NIGHT_BUYDBG` witness** (f.c, `buydbg census` at exit): logs per attempt the row, `sel` before/after, the
  cost actually read, `P[+0x2b]` before/after, `P[+0x23]`, `P[+0x28]`, `P[+0x56]`, and the reject reason
  (`not enough EXP` vs `row at max level`).  It also settled a wrong assumption: `inc dword [eax+0x2b]`
  (0x138e1, fn_0137c1) looked like a per-frame EXP raiser, but `/tmp/rig/g319b` reached the purchase gate with
  **exp = 0** on a keys-only drive — EXP arrives through the award chain into `P[+0x52]` and is folded into the
  total at 0x38fff-0x39003 when the screen opens.
* **New failure class recorded:** *a shipped byte offset used directly as a C element index*.  Third cousin of
  docs/218 (2560-byte row stride), docs/270 (declaration-width caps) and docs/254 §3 (accessor-macro stores).
  The tell is a `shl reg, 2` (or `imul …, 0x14`) immediately before `[reg + disp]` on something the port
  declares as a typed array.  The fix is to divide the byte offset by the element size, and the check is to
  compare against the *value* the shipped's own window holds at that address.
