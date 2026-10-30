# 244 — Item 2b: the walk step, its table, and the fact that the port reaches the walk line 4x later than the shipped (which ties item 2b to the pacing residual)

Same route, same pinned clock (`ep1_kill3.txt`, 665 step-calls/frame, 60 fps, 10 Hz dumps).

## 1. Which call site produces the player's vertical delta (temporary site-tag witness)

To name the site I temporarily wrapped every `fn_01586b` call in a `fn_01586b_at(e, d, __LINE__)` shim (32 sites,
witness-only, **reverted after the measurement** ✓ the committed tree keeps only the `[yclamp]` / `[clamprows]` /
`[ydelta]` witnesses in `fn_01586b`). Counts over a 9 000-frame run:

```
kind=1 (enemies):  694 x d=-1 + 627 x d=+1   from f.c:17247   (fn_017xxx AI family, mv_b from the speed accumulator)
                   586 x d=-1 + 484 x d=+1   from f.c:17945
kind=0 (player):   170 x d=-1                from f.c:27272
                    54 x d=+1                from f.c:27270     <- lab_0003284e, the walk/running body
kind=2 (effects):   58 / 58                  from f.c:27270/27272 area and 17945
```

So the player's vertical motion comes from **`lab_0003284e` (0x0003284e, states 1 / 2 / 0x28)**, not from the
action record's delta arrays ✓ the `[ydelta]` witness confirms it independently: for the player and for enemies
the action-record array reads **zero** while the passed delta is `+-1`:

```
[ydelta] kind=0 state=0x1 frame=53 arr[frame]=0 passed_d=-1
[ydelta] kind=0 state=0x2 frame=1..7 arr[frame]=0 passed_d=-1 (then +1)
[ydelta] kind=1 ... arr[frame]=0 passed_d=+-1        (604 + 558 of them)
[ydelta] kind=2 ... arr[frame]=+1 passed_d=+1 / arr[frame]=-1 passed_d=-1   (34 + 34, i.e. these DO come from the array)
```

## 2. `lab_0003284e` is byte-faithful, and its step table is tiny by design

Shipped 0x32856-0x328f5 vs `f.c:27255-27279`:

```
gate:  (DAT_00063ed8 != 0) || (DAT_00063ed0 != 0) || (DAT_00063ed3 != 0) || (DAT_00063ed5 != 0)   [0x32856-0x32882]
step = DAT_00052ffc[ (DAT_00063360 % 4) * 3 + byte[player + 0x25] ]     [0x32887-0x328af: sar/idiv 4, lea [edx+edx*2], add cl, shl 2]
Y:     if (DAT_00063ed0 != 0) edx = -step; call 0x1586b                 [0x328b2-0x328c5]
       else if (DAT_00063ed8 != 0) edx = +step; call 0x1586b            [0x328cc-0x328dd]
X:     mirrored on DAT_00063ed3 / DAT_00063ed5 with 0x1545e             [0x328e2-0x328f5]
```

The table (transcribed at `f.c:27222`, 65 dwords) is `1,2,2,1,1,2,1,2,1,1` for indices 0..9 and **zero from index
10 on** ✓ so a legal index yields a 1- or 2-pixel step, and an index >= 10 yields `d = 0` (which is why
`fn_01586b`/`fn_01545e` return early so often ✓ docs/171's "3167 of 3715 player calls with d == 0" is that table,
not a bug ✓)

## 3. The player's resting Y: both sides start at 172, the shipped gets to 139 in ~9 s, the port in ~36 s

From `scripts/rig/player_y.py` on the whole dumps (docs/242 §3):

```
shipped: [172 x46] [166] [162 x3] [167 x18] [157 x12] [154] [147 x12] [144] [139 x878]      ~94 samples = 9.4 s to settle
port:    [172 x365] [164 x17] [158 x15] ... [139 appears 253 times, only after the 172 block]  ~365 samples = 36.5 s
```

Both spawn on the same walk line value (172 at room entry ✓ the shipped's own first 46 samples are 172 ✓), the
step magnitudes are identical (1-2 px ✓ §2), and the port's `fn_01586b` bounds are `mn = 139`, `mx = 195`
(`[yclamp] ... mn=139 mx=195 room[+0x10]=0 camY=0 rows=9`) ✓ so the port is not blocked from reaching 139 ✓
it simply takes it 3.9x longer in wall time ✓ 3.9 ~ the measured screen-change pacing ratio family
(docs/168, docs/205 §: 5.20 shipped vs 25.08 port changes/s) ✓

## 4. The clamp table is real but belongs to other rooms' coordinate ranges

`[clamprows]` (printed at the first `fn_01586b` call):

```
0 a1[3765,3920] v[289,398]    1 a1[2025,2916] v[283,335]    2 a1[3550,3598] v[79,240]
3 a1[3550,3637] v[232,289]    4 a1[3619,3678] v[283,302]    5 a1[3675,3776] v[289,317]
6 a1[1998,2053] v[274,346]    7 a1[3280,3598] v[83,151]     8 a1[3280,3333] v[75,231]
```

`fn_01586b` keys a row on `e[+0x01]` (world X ✓ shipped 0x15919/0x1592b) and every row's X range is 1998-3920,
while every entity on this route has X in 0-516 ✓ -> **no row can match on this route, on either side** ✓
`clamp_row = -1` in all 2411 `[yclamp]` lines ✓ The rows are decoded faithfully too: the shipped's opcode-3 fill
(0x3eb8f-0x3ebe1) copies `record[+0x09], record[+0x0d], record[+0x11], record[+0x15]` into the row's four dwords
and the port's `case 3` (0x0003eb9c/ebaf/ebc2/ebd5 ✓ `f.c:27166-27170`) maps them to
`a1_lo = rx, v_lo = ry, a1_hi = gate, v_hi = cgate` in the same order ✓ So the wall/clamp system is simply not
engaged in room 1 -- it is not an item-2b cause ✓

## 5. What this does to the item-2b plan

Item 2b's proximate cause is now stated as a rate, not a branch: **the port lands too few hits on enemies
because its player is still 33 px above the walk line when the enemies are being engaged, and it gets there
3.9x slower** (270 of 404 player attacks rejected by the byte-faithful `dl = 7` Y window ✓ docs/242 §3). With
1-2 px per step and identical tables, "making the player descend faster" is not a transcribable fix -- the
shipped's own numbers are already in the port ✓ Therefore:

1. Item 2b's red/green pair must be **event-anchored**, not wall-time anchored (docs/234 §3 already forbids
   pacing claims from dumps ✓): compare the player's Y and the enemy-contact census at "N player walk steps after
   first room entry", both sides, not at a fixed t_ms ✓
2. If the Y-vs-logic-step curves coincide, item 2b is a consequence of the pacing residual (docs/168, docs/203,
   docs/205) and the honest next task is the pacing/clock item, with item 2b gated after it ✓
3. If they diverge per logic step, the divergence is in how often `lab_0003284e` runs (the state-1/2/0x28
   occupancy and the four direction latches `DAT_00063ed8/ed0/ed3/ed5` ✓ their producers are the key-handling
   path, i.e. the same family as the fixed key-wait item 4 ✓ docs/231) ✓
