# 134 — The 32 remaining DIFF cells split into four families (measured on /tmp/rig/g9)

For each still-flagged cell: the shipped value range, the port value range, and how many write sites the
port has for it in `f.c`.

| DS | port symbol | shipped (first..last, distinct) | port (first..last, distinct) | writes | family |
|---|---|---|---|---|---|
| 0x53350 | `g_heap_head` | `0x19d008` (1) | `0x40` (1) | 4 | heap arena base (docs/94 offset-vs-pointer) |
| 0x53354 | `g_heap_cursor` | `0x4ea008..0x37e008` (4) | `0x40..0x170600` (6) | 4 | heap cursor: **both sides move**, only the base differs |
| 0x53358 | `g_heap_highwater` | `0x3fd4..0x0` (5) | 11 distinct | 6 | heap watermark: both sides move |
| 0x53378 | `g_dat_53378` | `0x1` (1) | `0x0` | 3 | extender/DOS internals |
| 0x5337c | `g_dat_5337c` | `0x19d0d1` (1) | `0x0` | 3 | selector-form address (seg:off) |
| 0x53384 | `g_heap_seg_limit` | `0x21a0a0` (1) | `0x0` | 5 | extender heap segment limit |
| 0x5338c | `g_dat_5338c` | `0x2190c1` (1) | `0x0` | 2 | selector-form address |
| 0x53390 | `g_dat_53390` | `0x28` (1) | `0x0` | 3 | extender config byte (word-sized, see c9) |
| 0x53394 | `g_stack_bound` | `0x2190ce` (1) | `0x0` | 1 | extender stack bound |
| 0x533a0 | `g_dat_533a0` | `0x21a094` (1) | `0x0` | 4 | selector-form address |
| 0x533b2 | `g_heap_cfg_b2` | `0x1` (1) | `0x0` | 2 | extender heap config |
| 0x533b9 | `g_dat_533b9` | `0x50030` (1) | `0x0` | 1 | memory-map address |
| 0x533bb | `g_dat_533bb` | `0xca070005` (1) | `0x0` | 2 | device/driver identifier |
| 0x531f4 | `g_dat_531f4` | `0x10000` (1) | `0xa0000` (1) | 2 | **framebuffer geometry** |
| 0x531f8 | `g_dat_531f8` | `0xffff` (1) | `0x9ffff` (1) | 2 | **framebuffer geometry** (limit = size-1) |
| 0x53451 | `g_dat_53451` | `0x2000004` (1) | `0x10000` (1) | 3 | capability/version word |
| 0x53100 | `g_dat_53100` | `0x3` (1) | `0x3..0x2` (2) | 5 | **real value divergence: the port changes a mode the shipped side never changes** |
| 0x51c24 | `g_blk_51c24` | `0x27` | `0x27` | 0 | values agree; flagged only because no shipped span exists (rig basis) |
| 0x51c98 | `g_dat_51c98` | `0x1..0x2` | `0x1..0x2` | 5 | values agree; rig basis |

## Reading

1. **DOS-extender / real-mode memory-map state (10 cells: 0x53378, 0x5337c, 0x53384, 0x5338c, 0x53390,
   0x53394, 0x533a0, 0x533b2, 0x533b9, 0x533bb).** The shipped values are selector-form addresses
   (`0x2190c1`, `0x21a0a0`, `0x2190ce`), a config byte, and a device id: state the DOS4GW extender filled at
   process start. The port has write sites for every one of them but no path ever executes, because the port
   does not model an extender. Nothing in the game logic reads these values back in a way that changes
   behaviour, so they should be **classified as NON-GAMEPLAY in the oracle** rather than chased: they are the
   bulk of docs/132 §3's "port holds 0 where shipped holds a value" family, and counting them as DIFF is what
   keeps the state oracle from ever reading green.
2. **Heap representation (0x53350 / 0x53354 / 0x53358).** The absolute addresses cannot match (the port's
   flat heap is its own arena, docs/94) but the *dynamics* do: the cursor and highwater cells take 4 and 5
   distinct values on the shipped side and 6 and 11 on the port side, i.e. both sides allocate and grow. The
   honest oracle form for these is relative (offset from the heap base), which `diff_state.py` does not yet
   do — that is the rig change to make here, not a port fix.
3. **Framebuffer geometry (0x531f4 = 0x10000 vs 0xa0000, 0x531f8 = 0xffff vs 0x9ffff).** The shipped game
   publishes a 64 KiB video window and its last offset; the port publishes 0xa0000 / 0x9ffff for its own
   screen arena. Unlike family 1, these two *are* read by blit/clipping code, so a mismatch here is a real
   candidate for the pacing/clip divergences recorded in docs/103 and docs/105. Next measurement: which
   functions read them, and whether the port's clip windows shift by the same 0x90000 delta.
4. **One clean behavioural divergence: `g_dat_53100`.** Shipped holds `0x3` for the entire route; the port
   starts at 3 and drops to 2. Both sides have write sites, so this is a guard or an event the port takes a
   different path through — the first item in the remaining set worth a root-cause pass.
5. **Two cells are rig basis, not divergence** (0x51c24, 0x51c98): the values agree on both sides and the
   verdict comes from a missing/oversized shipped span. Closing them is the docs/132 §4(a) width import.

So the 32-cell DIFF set is: ~10 extender-state cells to *classify*, 3 heap cells to *rebase*, 2 framebuffer
cells to *investigate*, 1 behavioural divergence to *fix*, 2 rig-basis rows to *re-verdict*, plus the
CD-selection pair 0x53c40/0x53c44/0x53c48 already open under docs/131 and the entity-record rows
(`REC-DIFF 36`, `REC-NULL 3`) tracked separately.
