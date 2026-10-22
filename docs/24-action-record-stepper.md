# The action-record stepper (0x012616)

`fn_00012616` is the per-frame engine of the entity layer: every per-frame entity sweep ends here, and it is
what turns a `.act` record into movement. Ghidra's pseudo is one long chain; this note keeps the model.

## Record resolution

```
P1  = e[+0x123]                 action container   (heap offset)
P2  = *(P1 + 4)                 record-pointer array (heap offset)
rec = P2[e[+0x17]]              the record of the current state (heap offset)

rec[+0x00]  frame count          rec[+0x17] BYTE hold length
rec[+0x69]/+0x6a  element event  rec[+0x6c] BYTE: X-move flavour
rec[+0x71]  Y delta array       rec[+0x75]  X delta array   (both heap offsets of `count` dwords)
```

Entity fields used: `+0x01`/`+0x09` position, `+0x05` vertical accumulator, `+0x11` frame, `+0x16` step-back
counter (BYTE), `+0x17` state, `+0x1b` facing, `+0x30` dying flag, `+0x31` saved-state count,
`+0x32...` saved-state queue, `+0x3e` secondary state, `+0x42` secondary frame, `+0x46`+`+0x4e/4f` interval
link, `+0x4a` pair entity (heap offset), `+0x50` hold countdown (WORD), `+0x123` action container.

## Order of one step

1. `state == 0x29` is re-written (a no-op that clears the upper bytes).
2. Sleep -> wake: kind 0 becomes 1 when `state == 0x1d && action > 0x2d`, or `state == 0x16 && action > 0x12`.
   The player is excluded.
3. Kind < 2 first runs `fn_01194c` (the state/type/action dispatch).
4. `e[+0x46]` (interval list link) is cleared, together with `+0x4e/+0x4f`, when `fn_01435f` reports the
   interval no longer straddles.
5. **If the hold countdown `e[+0x50]` is 0:** increment the frame, test the three chained state windows
   (0xab..0xad, 0xb5..0xb8, 0xbf..0xc7 -- each advancing one state and restarting the frame once
   `frame >= rec[+0x17]`), then handle the frame limit:
   * `rec[+0x00] <= frame` and no saved states -> restart: states 0x6f/0x71 first drop the pair link and go
     to state 1; state 5 clears `+0x05`; state 0x70 hands the pair entity to state 0x1f and gives both sides
     a 200-frame hold; state 0x28 stays, anything but state 2 goes to state 1. Then `frame = 0`.
   * saved states pending (`e[+0x31] != 0`) -> `fn_031f5f` pops the queue (and latches the element event from
     the record of the state it switches **to**).
   * `e[+0x30] == 1` -> death: DAT_6336c = 1 (player) / 3 (the DAT_53c80 entity), DAT_53c88 dropped if it
     points here, `++DAT_639bc` for kind 1 with DAT_6336c = 4 once the room quota `g_tab_52a1c[serial +
     (stage-1)*10]` is reached, then `fn_0112fd` + free and an early return.
6. **Otherwise (hold running):** if `frame < rec[+0x00] - 1` just step the frame; else decrement the WORD
   countdown, apply the 3-frame breathing jitter to states 0x0b..0x14 (`DAT_63360 % 3`, alternate -1/+1), and
   when the countdown hits 0 clear the pair link on both entities.
7. Y move: states 0x84/0x85 move by `state - 0x82`; states 0x88..0x8b use the record of `e[+0x3e]` and count
   `e[+0x42]` (resetting the action to state 1 when that record is exhausted); states 0x8c/0x8d use the fixed
   2/3 (from `e[+0x3e` = 0x84/0x85) until frame 0x35, after which the delta array takes over. Everything
   goes through `fn_01545e`, mirrored by `e[+0x1b]`; the player's delta is also added to `room[+0x1e]`.
8. X move: with `rec[+0x6c] == 1` the entity is nudged by `+/- e[+0x42]`, `fn_015295(e, e[+0x3e])` runs, and
   `fn_015295` again with the delta array (or the step-back counter `e[+0x16]` rewinds the frame instead);
   otherwise `fn_01586b(e, rec[+0x75][frame])`.
9. If the player is in state 2, `room[+0x22]` absorbs the X delta, and standing inside the rectangle that
   script opcode 6 published in `g_dat_5d0a0` (x0, y0, x1, y1, mode) requests the transition:
   DAT_6336c = 2, with DAT_63398 set to 0 or 1 (only when the room has a child) depending on the mode word
   and on which of DAT_63ed3 / DAT_63ed5 is held.

## Port notes

* The frame counter is bumped **before** the delta arrays are read: a frame-2 entity reads array index 3.
* `DAT_639bc` is a DWORD (`inc dword ptr [0x139bc]` at 0x00012a33); the port had it as a byte.
* `g_tab_52a1c` occupies DS 0x52a1c..0x52f08, so the earlier guessed bound for `DAT_0005274c`
  (up to 0x52c3c) is wrong -- 0x5274c stops where 0x52a1c starts.
* `DAT_00053c88` holds an entity pointer (compared against the stepped entity at 0x00012a10); the port keeps
  it as a `uint32_t`, so the comparison goes through a cast until its producer is ported.
