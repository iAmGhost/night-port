# The player action-chain resolver (0x03272f)

`fn_0003272f` is the player half of the entity layer: `fn_00012616` advances whatever action the player is
already in, this function decides *which action comes next*. Its only caller is `fn_0003a829`, and it takes the
`.act` chain table of the current action in EAX.

## Half A - the state / frame / key machine

Key bytes come from `fn_0003493e()` = the previous key-ring slot (`DAT_00063f14[DAT_00063f50 - 1]`), and the
direction keys are the mirrored latches `DAT_00063ed0` / `DAT_00063ed8` (horizontal, reversed / plain) and
`DAT_00063ed3` / `DAT_00063ed5` (vertical, reversed / plain). `DAT_00063ec1` / `DAT_00063eb4` are the dash
keys, `DAT_00063ea5` / `DAT_00063ec0` the two "release" keys.

| State(s) | Meaning |
|---|---|
| 1, 2, 0x28 | walking (LAB_0003284e): step = `g_tab_52ffc[(DAT_63360 % 4) * 3 + gun slot]`, applied through `fn_01586b` / `fn_01545e`; the attack key opens 0x83 (or 0x84 when a vertical key is held), the dash key opens 0x97 |
| 3 | the "hurt / knocked down" tail: 0xa3 (dash) or 0x85 (attack); frames 0x13 / 0x2d may turn back into state 4 |
| 5 | 0x9c at frame 8, ring slot released at frame 0x11 |
| 0x15, 0x16 | attack wind-ups -> 0x98 / 0x9b (and set entity+0x30 = 2 for a wounded player) |
| 0x1e | clears the hold at frames 0xa..0x3a unless wounded |
| 0x29, 0x47 | -> 0x85 |
| 0x83, 0x84, 0x85 | attack ready: charge (`g`) at frame 0x1c -> 0x8c / 0x8d (weapon `+0x26`), sub-attacks `8`/`0x1d` -> 0x89 or 0x8a / 0x88, `i` (only from 0x83, weapon > 3) -> 0x8b; `g` before frame 0x1c requests the charge in `entity+0x11f` |
| 0x88..0x8d | the interrupt bodies; when `+0x42 == 0x39` the interrupted attack (`+0x3e`) is restored at that frame |
| 0x97 | dash: while the key is held the frame is decremented and `+0x42` incremented; once their sum passes 0x14 it leaves through 0xa8 (event 0x26) or 0xa9 (event 0x21) |
| 0x98..0x9b | push the next chain level into the saved-state queue (`+0x32` = 0x2d / 0x2e) |
| 0xa5 | the finisher: -> 0xa6, or into the level chain 0x29 / 0x2a / 0x2b by frame band |
| anything in the neutral/walking/hurt ranges | the dash key opens 0x97, the attack key opens 0x83/0x84 |

All direction keys released and state < 3 pulls the player back to the idle state 1.

## Half B - the chain table (LAB_000339c5)

The table (100 slots, struct of arrays; the inner pointers are heap offsets like every other stored pointer in
the port):

```
+0x000 uint32  entry count
+0x004 uint8   chain length in key steps
+0x068 uint32* -> the chain's key codes
+0x1f8 uint32* -> the chain's per-step level bytes
+0x388 uint16  target state id
+0x450 uint8   required weapon level (entity+0x27)
```

Matching walks the 10-slot key ring backwards from `DAT_00063f50`: the chain starts at slot
`(DAT_63f50 - len) mod 10` whose code must equal `DAT_00063f14[slot]`, the slot just before the head must not
already be consumed (`DAT_00063f1e`), and then every step must match `DAT_00063f14` while its level byte is
not below `DAT_00063f28[slot]`. A full match plus `+0x450 <= entity+0x27` fires one of:

* **direct** - window states (0x33..0x50, 0x0b..0x13, 0xa4, 0xa6, 0xa7, 0x2f, 0xab..0xad, 0xb5..0xb8) with a
  target of 0x65 / 0x30 / 0x31 switch immediately;
* **hurt** - 0x0b..0x14 with target 0x99 switches and re-latches the element event from the new record's
  `+0x69` / `+0x6a`;
* **pair** - target 0x70 counts the partner's `+0x52`; at five both halves move to 0x71;
* **queued** - idle states (1, 2, 0x66) accept a target whose chain level in `DAT_00062b90` is one above the
  current id's, and everything else in the same `DAT_00062ac8` group queues it through `fn_031f10`.

`DAT_00062d58` (current chain id) and `DAT_00062d5c` (chain level byte) are written on every successful fire.
The loop is skipped entirely for states 0x15..0x1e and 5.
