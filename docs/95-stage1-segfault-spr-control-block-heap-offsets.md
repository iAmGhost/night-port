# 95 — Stage-1 "keep it open" segfault = the .spr control-block cells hold HEAP OFFSETS; fn_03ace9/fn_03b718 read them as widened host pointers
Playtest log item "Segfault when keep stage-1 opened (maybe related with hit detection)".
Follows docs/94 (which already fixed the gauge-latch pointer bug on the same path).

## 1. Reproduction (before the fix)

The scripted combat+hold drive file (`/tmp/dbgauto/hold_port.txt`: fight inputs for ~3.5 min of
game time, then 40 idle shots to t=800 s) crashes in both builds:

* plain build: `Segmentation fault: 11`, lldb backtrace `fn_03b718` (f.c:13811) <- `fn_03b953`
  <- the stage-1 task; fault address `0x000c017c000c014c`.
* ASan build (`-fsanitize=address,undefined`):
  `AddressSanitizer: SEGV f.c:13811 in fn_03b718` — same point.

The `NIGHT_HITTRACE` sweep witness (added to fn_03b953) shows the list itself is healthy at that
moment: `VD i=0 n=1 cur=<valid heap ptr> age=0 val=0 l11=0x0 l19=0x0` — the crash is inside the
FIRST draw of the first floating value display, not in the post-free link read the old
fn_03b953 comment blamed ("latent stale-link bug"). No guest-side link corruption is involved.

## 2. Root cause

`DAT_000639c0` / `DAT_000639c4` (`system.spr`, `system2.spr`) are the 0x3c-byte parallel-array
structs built by `fn_03d8d4 -> fn_03d9cf -> fn_03de85`. The constructor stores the 14 sub-object
handles verbatim: `s[i] = heap_malloc(size)` (orig 0x03ded9-0x03def2, `fn_0418ee`/0x041c00 group).
In the guest `heap_malloc` returns a flat 32-bit pointer, and every reader dereferences it
directly. In this port `heap_malloc` returns a **heap offset** (the arena may live above 4 GiB),
so the cells +0x04/+0x08/+0x34/+0x38 hold offsets, and the code that already works
(`fn_0137c1`, `fn_038b30`: `(uint8_t *)g_heap_base + *(uint32_t *)(font + 0x38)`) converts.

`fn_03b718` (floating damage numbers) and `fn_03ace9` (HUD composite) instead cast the block to a
port invention, `struct blit_ctl` with four 8-byte host pointers. On the real loaded block that
reads the +0x34 glyph-offset offset **and the +0x38 RLE-base offset together** as one 64-bit
value: low dword `0x000c014c` (glyph object), high dword `0x000c017c` (RLE base) -> the fault
address `0x000c017c000c014c` is literally the two guest heap offsets side by side. The first
glyph-table read `*(char_table - 0xc0 + digit*4)` then SEGVs.

Why it looks "related with hit detection" and to "keeping stage 1 open": the draw only happens
when a value-display node exists, and those nodes are spawned by the combat damage path
(fn_01441e -> fn_011d5e/fn_011da6, docs/94). So the crash fires seconds after the first landed
hit — i.e. as soon as you keep a stage-1 room open with combat in it. With docs/94's box-index
fix the hits started landing, which is why this crash became reliably reachable in the port.

## 3. Fix (shipped representation, no host-side guard)

* `struct blit_ctl` deleted (f.h). The globals are plain `uint32_t *` blocks:
  `g_dat_639c0`, `g_dat_639c4` (f.c/f.h; loader line 2476 drop-cast).
* `fn_03b718`: `char_table = g_heap_base + f[13]`, `rle_base = g_heap_base + f[14]`,
  terminator fields `(g_heap_base + f[1] + 0x2c)`, `(f[2] + 0x2c)`, `(f[13] + 0x2c)` —
  the same conversion convention as fn_0137c1.
* `fn_03ace9`: identical conversion for the +0x2c/+0x30 column/row/glyph fields (cb[1], cb[2],
  cb[13], cb[14]).
* test_f1: `test_fn_03b953`, `test_fn_03b718` and `test_fn_03ace9` now build the control blocks
  exactly the way `fn_03de85` leaves them (arena cells `b[1]/b[2]/b[13]/b[14]` = heap offsets of
  arena-allocated objects), pinning the offset representation. Suite ALL PASS.

## 4. Second fault behind the same symptom: removing the LAST value-display node

With the glyph draw fixed, the same hold script ran further (19->24 hits) and then ASan caught a
second, different fault at the same user-visible moment:

```
ASan: SEGV on unknown address 0x000000000019 (WRITE), fn_0003b67e f.c:1039 <- fn_03b718 <- fn_03b953
VD i=0 n=1 cur=<node> age=40 val=0 l11=0x0 l19=0x0      (sweep witness, right before the fault)
```

This is the `fn_0003b67e` "latent bug" the port comment blamed. Ghidra disassembly of the shipped
function (scripts/ghidra_scripts/DumpListRemove.java, output /tmp/gh_out.txt):

```
0003b6a5  MOV EAX,[node] ; MOV EAX,[EAX+0x11] ; MOV [0x639cc],EAX    ; head = next
0003b6b0  MOV EAX,[node] ; MOV EAX,[EAX+0x11] ; MOV [EAX+0x15],0     ; next->prev = 0
```

With the list down to its last node (`n=1`, `next == 0`) the guest executes
`MOV dword ptr [0x15], 0` — a write to **guest linear 0x15**, its own mapped IVT page. It is
inert in the guest (nothing ever reads it back) and happens constantly: every floating-number
list drains to empty this way. It was never a guest crash; only the port's unmapped NULL made
it one.

Fix (faithful representation, not a guard): the port models that low page as `g_ds_ivt[0x200]`
and a NULL next/prev link routes the write there — same data, same offsets, harmless page,
exactly like the guest. test_f1 `test_fn_0003b67e_lastnode` pins: head branch wins for a
single node (shipped if-order), count/head update, the write lands at `g_ds_ivt+0x19` with the
neighbouring byte untouched. (The old "LATENT BUG ... null-derefs" comment was wrong and is
corrected in f.c.)

## 5. Verification after the fixes

Plain scripted combat run with glyph witness (`route_fight8.txt`, `NIGHT_GLYPHDBG=1
NIGHT_HITTRACE=1`): 19 hits and `valuedisp` glyph blits at `fn_03bb42` (log
/tmp/dbgauto/port_hold5.log) — damage numbers draw instead of faulting.

Long ASan hold (the exact scenario of the playtest report — combat then keep the stage-1 room
open; drive file /tmp/dbgauto/hold_port.txt: fight inputs to t=210 s, then 40 idle shots to
t=800 s):

```
ASAN_OPTIONS=detect_leaks=0 SDL_VIDEODRIVER=dummy NIGHT_HITTRACE=1 ./build/night \
  --script /tmp/dbgauto/hold_port.txt --shots /tmp/dbgauto/port_hold3 --no-audio --frames 15000
-> done_rc=0 ; grep -c "ERROR: AddressSanitizer" = 0 ; 24 x "HIT " ; 242 value-display sweep
   visits incl. removals (VD ages reach >0x28, list stays consistent); stage=1 room=1 alive at
   frame 36758.
```

Before the fixes the same script aborted: plain build SEGV at `fn_03b718` (f.c:13811, fault
address 0x000c017c000c014c = the two packed heap offsets), ASan build after the draw fix SEGV
WRITE 0x19 in `fn_0003b67e`. Both gone.

Remaining UBSan (not ASan, pre-existing, listed for honesty): x86-benign unaligned dword access
through guest-record layouts (~620 in the long hold, same class already documented in earlier
docs), one `28457*86400` int overflow in the audio clock, and two bounded-table reads whose
guest originals are contiguous DS reads (f.c:13760 tick-table walk = docs/83's modelled span;
f.c:24418 `DAT_00053ea8 + (cur+0x0d)*0x88` — in the guest `cur+0x0d=48` reads DS beyond the
4-entry table, i.e. real DS bytes the port models as nothing yet). None faults; they are
modelling gaps to close when their owners (music/sfx census) get traced.

## 6. Reproduce

```sh
make -j8 build/test_f1 && ./build/test_f1      # ALL PASS (adds test_fn_0003b67e_lastnode)
make -j8 build/night CFLAGS="-std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter" LDFLAGS="-fsanitize=address,undefined"
ASAN_OPTIONS=detect_leaks=0 SDL_VIDEODRIVER=dummy NIGHT_HITTRACE=1 ./build/night \
  --script /tmp/dbgauto/hold_port.txt --shots /tmp/dbgauto/port_hold3 --no-audio --frames 15000
grep -cE "ERROR: AddressSanitizer" /tmp/dbgauto/port_hold3.log   # -> 0
```

Ghidra: `scripts/ghidra_scripts/DumpListRemove.java` (disasm + decompile of 0x0003b67e).

