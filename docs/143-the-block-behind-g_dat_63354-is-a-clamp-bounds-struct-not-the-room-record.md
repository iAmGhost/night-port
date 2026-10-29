# 143 — What is actually behind `g_dat_63354`: a clamp-bounds struct, so docs/142 §4's field names are void

docs/142 §4 read `room[+0x32]` / `room[+0x36]` out of the record block dumped for the pointer cell
`g_dat_63354` and called them the room's scroll limits. A live witness now contradicts that reading, and the
contradiction is the useful part.

Added to `e20f_spawn` under `NIGHT_SPAWNTRACE` (printed from the **live pointer**, not from a dump):

```
SPAWN t=31 stamp=0 rec=00374594 sub=1 gate=0 cgate=0 rx=324 ry=150 room11=0 cnt=1 cap=9 anchor=350
ROOM off=0018d6ec b00=0 d00=0 b08@1=64 d08=1600 d11=35584 d32=0 d36=0 d40=0
```

Two facts:

1. `port/src/f/f.h:1146` already describes this pointer as **"pointer to clamp-bounds struct: lo=P+0x12,
   hi=P+0x16 (double-deref)"**, and the live block does look like bounds: `dword[+0x00] = 0`,
   `dword[+0x08] = 1600` -- a `[0, 1600]` span, i.e. a room width, not a script record. So docs/142 §4's
   `x_lo`/`x_hi` were bounds fields, and `+0x32`/`+0x36` in that block are **not** the `room[+0x32]`/
   `room[+0x36]` the scroll guard reads.
2. The witness is internally inconsistent about offset `+0x11`: `SPAWN room11=0` (byte read through the live
   pointer, the value the spawn arm actually branches on) versus `ROOM d11=35584` (dword read at the same
   offset, whose low byte would have to be 0x40). A byte and a dword cannot disagree at the same address, so
   the dword print is reading a different object -- `g_dat_63354` and the interpreter's `room` argument are
   **not the same block**. `fn_03e20f` receives its room pointer as a parameter; `g_dat_63354` is the clamp
   struct the camera functions use.

Consequences, stated as corrections:

* **docs/142 §4 is withdrawn as written.** Its `room[+0x36] = 0 vs 617` was a comparison between two different
  things: the shipped side read it as the `i*352`-th block of the concatenated heap blob (an offset heuristic
  `ent_list.py` also uses), the port side through the cell's own `rec_offset`. Neither is the scroll-limit
  field. The `0` vs `617` difference is real data, but it is a difference between the shipped clamp block and
  the port's clamp block at offset +0x36 -- worth re-examining as a *clamp* difference, not a script difference.
* **docs/142's camera claims stand unchanged**, because they read DS cells directly with no record-block
  interpretation: shipped `DAT_000639dc` in {0×590, 27×179, 24×174, 12×30, 26×3, 25×2} (max 27) vs the port
  reaching 298 over 74 distinct values; `DAT_000639e0` = 0 everywhere on both sides.
* docs/142 §5's suspicion was right in kind and wrong in target: this is a record-identity problem, and it is
  in **our own reading layer**, not in the game.

## Queued

1. Name the block `fn_03e20f` actually receives: instrument the call from `fn_03ed6f` with the record pointer
   **and** the room pointer it passes, and dump the *parameter*, not `g_dat_63354`. Then read the scroll guard's
   `room[+0x32]` / `room[+0x36]` from that block on the port side.
2. Re-attribute the shipped-vs-port difference at clamp-block offset +0x36 (0 vs 617): the clamp struct is
   written by `fn_03cad3` (which also writes the camera at 0x3cb02/0x3cb14), and its bounds feed the scroll
   clamp inside `fn_03cb38`/`fn_03cbd9` -- the same functions whose camera census diverges. A 617 bound where
   shipped has 0 is a plausible direct cause of a camera that scrolls to 298 instead of parking at 27.
3. Fix the rig's shipped-side record addressing so both sides address record blocks by the dump's own offsets
   rather than `index * rec_bytes` (docs/124's rule, still not applied everywhere).
