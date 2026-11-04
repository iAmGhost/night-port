# PORT FRONTIER NOTES (post-compaction handoff)

State: **413/528 done** (78.2%), suite **1664 PASS, 0 FAIL** (non-ASan + ASan green). 115 pending.
Goal: mvamgfqk-xs4w9s. Build: `make -C port test`. Count: `python3 scripts/status_report.py`.

## This turn's ports (all committed)
- **0x03e0a0 (L10)** — build+append doubly-linked list node. Offset model (matches siblings
  fn_03e14c/fn_03ee0a: g_dat_63e18/63e1c are heap OFFSETS feeding heap_free_guard). node[0x51]=next,
  node[0x55]=prev (offsets). node[0]=fn_041c8f(io), node[1..80]=fn_04192e(node+1,4,0x14,io) (80 B).
  node size 0x58. `fn_03e0a0(uint32_t node_off, uint8_t *io_node)->node_off`.
- **0x036190 (L11)** — screen RLE loader into CALLER 64000-byte buffer (no board_out). table->new
  global g_dat_633b8 (>>2). `fn_036190(name, dest)`.
- **0x036417 (L11)** — screen RLE loader into caller 0x4b000=311296-byte buffer, dl->board_out.
  `fn_036417(name, dest, dl)` (dl in EBX low byte, reg ABI). table->g_dat_636b8 (shared).

## THE KEY FINDING: "all-callees-ported" clean wins are mostly ILLUSORY
The `analysis/pseudo/*.c` callee scan (FUN_HHHH refs all in the STATUS [x] set) finds ~30 "clean
wins", but most are SUBSYSTEM-ENTANGLED, not isolated. Verified this turn:
- **0x446c3 (L7,132B) + 0x428f8 (L5,47B)** — call **fn_044c23 which is a register-ABI format
  processor: EAX=buf, EDX=fmt, EBX=src, ECX=cb** (NOT cdecl 4-stack-args, despite the C port
  signature). 0x446c3 passes through EBX(src)/EDX(fmt) from ITS caller and sets ECX=0x446b2 (cb).
  So both are format-string-subsystem wrappers, not leaves. BLOCKED on modeling the fmt subsystem.
- **0x010cbd (L12,137B)** — mem_copy(0x041e6c) writes 1 byte to the DPMI far-pointer g_dat_53c5c
  -> flat-model-fragile (already deferred, cf. STATUS line 169).
- **0x03fc1d (L10,159B)** — needs board_out (fn_040042) output model.
- **0x417f3 (L11,106B)** — needs fn_036626(name)->value table populated AND fn_04247a table-seek
  (g_639b8 setup) to test. The empty-table trick (pre-set g_639b8) FAILS here: A=fn_036626=0 ->
  count A-0x1a huge.
- **0x3c72b (L10,171B)** — heap-pointer model (deferred before).
- **0x4262a (L6,199B)** — node-pool pattern.

Genuinely-isolated clean wins remaining (the I/O/screen-loader family, proven pattern): mostly done
now. The screen-loader family is now FULLY disambiguated (5 siblings).

## The 5 screen-loader siblings (disambiguated this turn)
All: canary; fn_0367c9(name) lookup; fn_036853 seeks; fn_04192e(768 B table, >>2); fn_041c8f RLE
(FILL byte&0xc0==0xc0: count=byte&0x3f,value / else literal) into a target.
- **0x035f1b** — into VIDEO (g_video_ram), 64000 B, + board_out if dl. `fn_035f1b(name,dl)`.
- **0x036041** — into VIDEO, 64000 B, + **0xff-skip** (FILL value==0xff skips write; literal 0xff
  branch is DEAD: 0xff&0xc0==0xc0 always -> FILL), + board_out if dl. `fn_036041(name,dl)`.
- **0x036190** — into CALLER 64000 B buffer (no board_out). `fn_036190(name,dest)`.
- **0x036417** — into CALLER 311296 B buffer, dl->board_out. `fn_036417(name,dest,dl)`.
- **0x03fa3b** — 200-row per-row-count format (different RLE: per-row op count, inverted ctl).

## Test conventions (proven)
- RLE control byte for FILL count N = `0xC0 | N` (N in 0..63). count 3=0xC3,4=0xC4,10=0xCA,63=0xFF.
  **PITFALL:** 0xCC = count 12 (not 4); 0xFF is ALWAYS a FILL byte (count 63), never a literal.
- Empty-table trick: g_dat_633a0=0 -> fn_0367c9/fn_036626/fn_036853 no-op, g_639b8 unchanged ->
  pre-set g_639b8 = my io_node. io_node on heap_arena: [0]=data_off(cursor), [4]=remaining,
  [8]=end, [0xc]=0x41 (bulk+open).
- Large write buffers (>=64000 B) MUST be heap: calloc or heap_arena, never a static.
- **Screen-loader overshoot:** the RLE `while(pos<BND)` loop does NOT clip the final FILL run, so it
  writes up to 62 B past the bound. Test buffers need margin (calloc(BND+512)); verify in-bounds
  PREFIX, not the exact tail (data often fills > BND, leaving node[4]>0 at the bound).

## Ghidra disasm method (works)
`mcp__ghidra__run_script_inline` compiles JAVA (no helper methods, no AddressSetIterator/Reference/
getInstructions). Use `currentProgram.getMemory().getBytes(toAddr("XXXXXX"), b, 0, len)` + println
hex, then `uv run --with capstone python3` to disassemble (authoritative — the decompiler's
EAX:EDX 64-bit merge is unreliable for register-ABI calls).

## Next-turn candidates (fresh context)
1. Re-verify the ~30 "clean wins" list (rebuild from STATUS [x] set + analysis/pseudo callees,
   excluding self) and pick truly-isolated ones; expect fewer than the scan suggests.
2. **fmt subsystem** (fn_044c23 reg-ABI + cb 0x446b2 / LAB_000428e5) would unblock 0x428f8 (N-1
   for 0x38cde L12) and 0x446c3 (N-1 for 0x42394 L8 -> 0x3f15f L11 + 0x36537 L14). High leverage
   but needs the format-string model.
3. N-1 levers (single missing callee): 0x0426f2(L7)->0x04270d(L8); 0x041ffc(L6)->0x042023(L7)
   ->0x032464(L11); 0x0446c3(L7)->0x042394(L8).
4. Critical path: **0x0461cc (L5, 5909B FPU engine)** — all callees ported, unblocks 0x0473bf (L6).
   Massive; dedicated multi-session.
5. 0x049e80 (L4,547B) — printf-format output, called by 0x045c79 (L5,220B).

## 2026-07-13b checkpoint (420/528, suite 1694 PASS 0 FAIL, git clean @ fec2b58)

Landed this turn: 0x036537 (L14) + flat_ofs sign-extension fix, 0x04262a (L6),
0x04589e (L6). STATUS summary table now auto-verified against detail rows.

### Recurring harness traps (READ BEFORE WRITING A TEST)
1. **Never call heap_arena_init() in a new test** unless the test runs before
   test_fn_041e94_04123d -- it wipes free-list state that later heap_malloc(16)
   checks depend on (heap_grow is a stub). Bite #2 this turn.
2. **fn_042576 reads modestr[1] and [2] unguarded** (harmless in the original:
   mode strings are data-segment). Test mode strings must be padded arrays
   (static const char m[4] = {'r',0,0,0}) or ASan faults.
3. **32-bit pointer arrays in the original must be widened** to sizeof(char*) when
   the port stores host pointers, and any size-derived terminator index must be
   recomputed (see 0x04589e: the trailing `mov [edx],0` was exactly out[count]).
4. printf-style test headers: escape literal % as %%.
5. Fixed data-segment globals used as io nodes work now: use flat_ofs() (SIGN
   extend) instead of (uint8_t*)g_heap_base + off.

### Remaining clean wins (all callees [x]) -- 30 total by tier
L4:1 0x049e80(547B FPU float formatter, needs FPU-composite care)
L5:2 0x043a7f(544B) 0x0461cc(5909B FPU engine = CRITICAL PATH, unlocks 0x0473bf)
L7:1  L10:7 (0x03c72b recursive-cleanup, 0x0366a4 needs live CD/file reader backend,
      0x03d75c float physics, 0x010181(500B), 0x010efc/0x01545e/0x03c231 ~1KB)
L11:8 (0x0417f3 107B mode-0-seek needs real file handle -> weak test,
      0x03628e 0x03f885 0x03ace9 0x03b718 0x03f15f 0x01348f 0x016b8f)
L12:6 (0x010cbd 137B DPMI far-ptr struct = flat-fragile, 0x040b83 0x040fb3
      0x038cde 0x03c853 0x03d9cf)
L13:3 (0x03d0c0 0x013908 geometry 0x04097e)
L17:1 0x034c40(104B: Ghidra shows EFLAGS arithmetic -- it is a PUSHFD/POPF +
      far-call-through-SUB_00063334 idiom; needs flag-semantics modelling, skip)

### N-1 levers still open
0x0426f2 (L7) -> unlocks 0x04270d (L8). 0x041ffc (L6) -> 0x042023 (L7) -> 0x032464 (L11).
0x0461cc (L5, 5909B) is the biggest unlock (FPU engine, 20 callees all [x]).

### Tooling
- scripts/frontier.py N  -> clean wins at level N (STATUS [x] set + callgraph).
- scripts/status_report.py -> authoritative totals; also check the summary table
  against per-section detail counts (drift happened twice).
- uv run --with capstone python3 + mcp__ghidra__read_memory for authoritative disasm
  (Ghidra decomp is unreliable for reg-ABI/extraout_* functions -- all 3 ports this
  turn needed disasm to resolve).

## 2026-07-13c checkpoint (426/528, suite 1726 PASS 0 FAIL, clean @ a4ea840)

Landed: 0x0426f2+0x04270d (L7/L8), 0x0417f3 (L11), 0x010cbd (L12), 0x03c72b (L10),
0x03d0c0 (L13). L8 tier is COMPLETE.

### New reusable techniques (proven this turn)
1. **Node-arena observability for fn_03ba2c / fn_034aba callers**: each fn_03ba2c call
   allocates 200 x 11-byte nodes, so `g_dat_62d64` counts calls (2200 per build) and
   `nstart(11*i)` gives row i's c_init. Reset with
   `memset(nar(),0,0x2000); g_dat_62d64=0; memset(g_dat_62d68,0,sizeof g_dat_62d68);`
   -> pins every branch of a rendering setup function exactly (see test_fn_03d0c0).
2. **Resource-chain tests (fn_0367c9/036626/036853/04192e)**: sys_file_lseek stub returns
   0, so fn_04247a mode-0 seek consumes `value + remaining` chunk bytes. Choose the entry
   offset (e.g. -0xC0) so the seeks stay inside the prefetched chunk -> the bulk read then
   serves the payload. fn_04192e RETURNS total_read/count (elements, not bytes).
3. **Heap pre-seed must be sized big**: `(void)heap_malloc(0x2000)` fails by the known
   shortfall but leaves a *splittable* region; the usual 0x400 pre-seed cannot serve
   0x350+ blocks. heap_arena_alloc() and heap_acquire_region share the bump pointer, so
   mixing bump slots + heap blocks is safe. A test may call heap_arena_init() for
   determinism if it hands on the known state (init + 0x400 pre-seed) at the end.
4. **DPMI family** (0x010cbd etc.): ROM templates -> `static const uint8_t g_*_tpl_*[]`,
   fn_10570 is a documented no-op, so flag-read results are constants; mem_copy ABI is
   EAX=dst, EDX=src, **EBX=n**.
5. **Hazard: entity sibling fields ent[0x34c] / ent[0x350] OVERLAP** once widened to 8-byte
   host pointers (4 bytes apart in the original). One sibling per entity until a 32-bit
   handle/translation scheme exists. Tests must set the sibling per branch.
6. `#define` with do/while(0) must stay on ONE line (no implicit continuation).

### Remaining clean wins (all callees [x]) -- 24 left
L4:1 0x049e80(547B FPU float formatter)
L5:2 0x043a7f(544B) 0x0461cc(5909B FPU engine = CRITICAL PATH; unlocks 0x0473bf)
L10:4 0x0366a4(292B needs live CD reader) 0x03d75c(306B float) 0x010181(500B)
      0x010efc(1024B) 0x01545e(1036B) 0x03c231(1065B)   [6 actually]
L11:6 0x03628e(392B) 0x03ace9(420B) 0x03f885(437B) 0x03b718(570B) 0x03f15f(606B)
      0x01348f(817B) 0x016b8f(1116B)
L12:5 0x040b83(389B) 0x040fb3(549B) 0x038cde(606B) 0x03c853(639B) 0x03d9cf(749B)
L13:2 0x013908(420B geometry) 0x04097e(516B)
L17:1 0x034c40(104B PUSHFD/POPF + far-call idiom -- flag semantics, low value)
N-1 lever still open: 0x041ffc (L6) -> 0x042023 (L7) -> 0x032464 (L11).

### Suggested next step (cheap, high info)
Port 0x040b83 (L12, 389B) or 0x03628e (L11, 392B) with the node-arena/viewport test
patterns above; then start 0x0461cc (the 5909B FPU engine) as a dedicated multi-session
task -- it is the single biggest unlock (caseD dispatch 0x046d32/0x046d47/0x0473e5,
0x0473bf, and the L14+ game-state tiers).

## 2026-07-13d checkpoint (431/528, suite 1782 PASS 0 FAIL, clean @ 6f1079a)

Landed this run: 0x03628e (L11 video-window RLE loader), 0x040b83 (L12 DMA chunk),
0x04097e (L13 DMA arm/start -- was DEFERRED, unblocked by the new globals), 0x010181
(L10 record deserializer), 0x03f885 (L11 resource-op dispatcher).

### New reusable knowledge (verified, not guessed)
1. **Device/DMA globals now defined**: g_dat_63f8c (armed 0xffff), 63f94 (remaining),
   63f98 (chunk len; device is programmed with len-1), 63f9c (cmd selector),
   63fa8 (src cursor) + 63fb4 (device buffer) kept as HOST POINTERS (a 32-bit flat mirror
   truncates on arm64), 63fac (24-bit DMA addr). 0x04097e and 0x040b83 duplicate the same
   chunk body in the original -- kept duplicated for 1:1 fidelity.
2. **fn_041340 command chains are observable through the port model**: it polls bit 0x80 of
   the SAME port it writes (g_dat_53334+0xc = 0x30c in tests), so a chain continues only
   while the last written byte is < 0x80. Careful with bit arithmetic: 0x75/0x77/0x14/0x17
   do NOT gate (bit7 clear); 0x93/0x99/0xfe DO gate. outb is ungated, so DMA port values are
   always programmed even when the command chain stalls. Save/restore sys ports
   (sys_inb/sys_port_set) in device tests.
3. **sys_hist() is a fixed 0x10000 FIFO that saturates late in main()** -> late palette tests
   cannot assert exact counts; assert growth + last-byte content + the index port instead.
4. **Heap tests late in main() need the test_fn_03c72b pattern**: `heap_arena_init();
   (void)heap_malloc(0x2000);` pre-seed, and hand off `heap_arena_init();
   (void)heap_malloc(0x400);`. The bump allocator (heap_arena_alloc) shares the arena with
   heap_malloc and heap_grow is a stub, so a preceding ~10 KB bump allocation starves
   heap_malloc (returns 0) -- that is how a 0x010181 test failed first.
5. **fn_043280/fn_041e20 fill counts BYTES** (align loop + memsetw of count/4 words), so
   fn_041e20(b,0x80,0xffff) touches indices 0..0xfffe only (off-by-one test trap).
6. **Ghidra offset traps**: it mixes pointer arithmetic with byte offsets
   (`piVar2+0x17` = +0x5c but `(int)piVar2 + 99` = +0x63); always re-derive field offsets
   from capstone. Also `cmp X, 0xffff / jle` == `< 0x10000` (decompiler prints the latter).
7. **Ported-name gotchas** (grep f.h before calling): 0x03fade = **fn_0003fade**,
   0x040042 = board_out, 0x04231d = outb, 0x041e6c = mem_copy, 0x041af8 = heap_malloc,
   0x041e20 = memset-style, 0x04564f = delay_units, 0x040948 = two-cmd device write.
8. Multi-line `#define` needs a trailing backslash -- prefer a static test helper function.

### Remaining clean wins (all callees [x]) -- ~20 left
L4:1 0x049e80(547B FPU float formatter)
L5:2 0x043a7f(544B) 0x0461cc(5909B FPU engine = CRITICAL PATH; unlocks 0x0473bf + L14+)
L10:5 0x03d75c(306B float) 0x010efc(1024B) 0x01545e(1036B) 0x03c231(1065B) 0x0366a4(292B, needs live CD reader)
L11:5 0x03ace9(420B -- needs DAT_000633b0/633b4 double-buffer + DAT_000639c4 ptr-struct;
      callees fn_03bb42/fn_035ec2/delay_units all ported) 0x03b718(570B) 0x03f15f(606B)
      0x01348f(817B) 0x016b8f(1116B)
L12:4 0x038cde(606B) 0x03c853(639B) 0x03d9cf(749B) (+0x040fb3 549B)
L13:2 0x013908(420B geometry, deferred struct model) 0x03c231-adjacent
L17:1 0x034c40(104B PUSHFD/POPF + far-call idiom)
N-1 lever: 0x041ffc (L6) -> 0x042023 (L7) -> 0x032464 (L11).

### Suggested next step
Take 0x03ace9 (L11, 420 B) next: it is a 20-iteration screen refresh (mem_copy of two
64000 B buffers + two fn_03bb42 sprite blits + fn_035ec2 + 1024 x delay_units + 2 final
blits). Model DAT_000633b0/633b4 as two host pointers and DAT_000639c4 as a byte struct with
pointer fields at +4/+8/+0x34 and a dword at +0x38 (watch the widening/overlap hazard that
bit fn_03d0c0's ent[0x34c]/[0x350] pair). Then start 0x0461cc (5909 B FPU engine) as a
dedicated multi-session task -- it is the single biggest unlock for L14+.

## 2026-07-13e checkpoint (435/528, suite 1810 PASS 0 FAIL, clean @ 8d8ad0a)

Landed: 0x03ace9 (L11 HUD composite), 0x040fb3 (L12 IRQ-hook/probe), 0x040d09 (L13 device
bring-up), 0x03658d (L14 master bring-up). The 0x040fb3 -> 0x040d09 -> 0x03658d chain is an
example of using an N-1 lever to land a *named subsystem* function; worth repeating.

### New facts worth reusing
1. **Screen model**: DAT_000633b0/DAT_000633b4 (the two off-screen 64000 B buffers, malloc'd by
   0x03658d) are modelled as flat addresses in `g_screen_ram` (2 x SCREEN_SIZE=0xfa00) placed at
   SCREEN_BASE = 0xb0000, and `video_map()` resolves that range -- so fn_03bb42/fn_03bebb keep
   working unchanged. Declared in f.h (SCREEN_BASE/SCREEN_SIZE there, NOT in f.c, to avoid a
   macro redefinition).
2. **Widened-field overlap bites again** (2nd time): DAT_000639c4 stores 4 pointer fields 4 bytes
   apart (+4/+8/+0x34/+0x38); reading them as host pointers overlapped (0x606000000200 became
   0x026000000200) and SIGBUS'd. RULE: any original dword-pointer field adjacent to another
   pointer field -> model the block as a named C struct (`struct blit_ctl` in f.h), never an
   in-place widening.
3. **Interrupt ABI**: isr_get(EAX=vec) -> EAX=offset, EDX=segment; isr_set(EAX=vec, EBX=off,
   ECX=seg). Handlers at 0x41486/0x414d3/0x41520/0x4156d/0x415c9 are LABELS (not Ghidra
   functions, not in the 528 list) -> pass by literal offset with CS = 0x1f, like 0x3ee88/0x3fd40.
4. **Ready gates** (device): fn_041390 = ready when inb(base+0xc) bit7 CLEAR; fn_0412e9 = ready
   when inb(base+0xe) bit7 SET; fn_0412a3 returns inb(base+0xa); fn_040f4f scans 8 bases for
   signature 0xaa (base+0x10 steps) and 0x210 means "wrong controller".
5. **SYS-LAYER GAPS FOUND (needed by the mid-subsystems task, blocking deeper device/CD chains):**
   (a) the device command port and the status port are the SAME port_val cell, so after
   fn_041340 writes a command >= 0x80 the ready poll is permanently busy -> split read/write
   latches per port; (b) fn_000104c7's DPMI-alloc stub leaves the CD far-pointer DAT_53c5c NULL
   -> fn_010927 mem_copies into NULL (crash) unless a caller supplies the 7-byte response
   buffer; (c) fn_00040788 (DPMI alloc) returns 0 so 64 KB device buffers never exist.
6. **Test harness trap (bit #4)**: fixed-size save/restore port arrays must be sized generously
   (a 26-entry array + 2 extra writes = stack-buffer-overflow caught instantly by ASan).

### Banked spec for the next port: 0x049e80 (L4, 547 B) -- printf float formatter
Decompilation now saved at analysis/pseudo/514_FUN_00049e80_049e80.c. Signature per Ghidra:
`FUN_00049e80(char *param_1, undefined4 *param_2 /*double ptr*/, int param_3 /*precision*/,
char param_4, int param_5, int param_6, undefined4 param_7, char param_8, byte param_9 /*fmt char*/)`.
It is the %e/%f/%g path: uses fx_double_naninf(), log2() * 0.3010299956639812 (log10 conversion),
FUN_0004a32b (fx128 digit emitter) + fpu_rndint/ROUND, chooses 'E'/'F' style (local_10 = 0x45/0x46)
and rewrites param_3 by the exponent, then walks the digit string in a 20-byte local buffer.
Check that 0x04a32b / 0x04a48f / fx_double_naninf / fpu_rndint are all [x] first (0x04a48f is
ported; 0x04a32b status unknown). Approach: same as the 0x04a48f port -- drive it through the
fx128 helpers, verify against formatted-output vectors, and remember to escape '%' in test
printf headers.

### Remaining clean wins (all callees [x]) after 435
L4:1 0x049e80(547B float fmt)   L5:2 0x043a7f(544B) 0x0461cc(5909B FPU engine = CRITICAL PATH)
L10:5 0x03d75c(306B) 0x010efc(1024B) 0x01545e(1036B) 0x03c231(1065B) 0x0366a4(292B, live CD reader)
L11:5 0x03b718(570B) 0x03f15f(606B, DPMI/INT21 flavour) 0x01348f(817B) 0x016b8f(1116B) +1
L12:4 0x038cde(606B) 0x03c853(639B) 0x03d9cf(749B) +1
L13:3 (0x013908 geometry 420B, ...)
N-1 levers now: 0x041ffc(L6) -> 0x042023(L7) -> 0x032464(L11); 0x0426f2/0x04270d done.
Blocked chain: 0x03690b (2554 B master data loader) still needs 0x010010, 0x032464, 0x0366a4,
0x03d8d4, 0x042023 -- so take the 0x041ffc lever before attempting it.

## 2026-07-13f checkpoint (438/528, suite 1853 PASS 0 FAIL, clean @ af55033)

Landed in this turn: **0x04a7fa/0x04a32b port-bug fix** (x87 FSUB ST(i),ST(0) semantics ->
0x04a32b is *floor*, not a remainder; only caller is 0x049e80, so blast radius was nil),
**0x049e80 (L4, printf-style float formatter)**, **0x045c79 (L5, printf float conversion
handler)**, **0x043a7f (L5, emit/parse format-string scanner)**. L4 is 13/13; L5 has exactly
one function left: **0x0461cc (5909 B FPU engine)**.

### New reusable facts
1. **x87 stack effects decide semantics, not the pseudo-code.** FSUB ST(1),ST(0) leaves the
   stack intact and stores ST(0) with the following FSTP: 0x04a7fa => `*out = rndint(a)` and
   leaves `a - rndint(a)` on the stack for the caller. Trace FLD/FLDG/FCOMPP/FSTP by hand for
   every x87 helper before writing C.
2. **log10 idiom**: `FLD x; FLDLG2; FXCH; FYL2X` = log10(2) * log2(x) = log10(x). (FLDLG2 =
   log10(2)=0.30103, FLDL2T = log2(10)=3.3219 -- easy to mix up.)
3. **Two format engines coexist**: (a) the *output* float path 0x045c79 -> 0x049e80 -> 0x04a48f
   + float_str_sci (W=0x27 field, M=3 exponent digits, precision in rec[8], radix char inserted
   by 0x049e80, rec[0x16]='*' on overflow, rec[0x15]=format char, unhandled chars are a no-op);
   (b) the *input* scanner 0x043a7f over `struct emit_ctx` (next/emit callbacks, count +0xc,
   flags +0x10; conversions write through a `uintptr_t **pos` slot stream).
4. **emit/parse flag bits** (from the callee docs, confirmed in use): bit0 write, bit1 stop/peek,
   bit2 far, bit4 word-store, 0x20 near/word variants; cmdline_parse REWRITES ctx->flags from the
   format's own flag chars, so test init flags are not preserved.
5. **Original quirks deliberately kept**: 0x049e15's space-fill overwrites a moved sign (so a
   padded negative prints unsigned); 0x04a48f's digit extractor degenerates for |x|<1e-? making
   1e-7 print "0000.00-004"; a space inside a 0x043a7f format eats a *stream* word.
6. **Test trap (bit #5)**: when a test prints a produced field with `%s`, NUL-bound the scratch
   buffer first -- ASan caught a global-buffer-overflow in the test harness, not in the port.

### Next queue (in order)
1. **0x0461cc (L5, 5909 B, FPU engine)** -- the last L5 function and the unlock for
   0x046d00/0x046d32/0x046d47/0x0473e5 (L14+) and 0x042023/0x032464. Dedicated multi-session:
   start with a capstone map of its blocks + the helper set it calls, port in slices, keep the
   suite green after each slice.
2. 0x041ffc (L6) -> 0x042023 (L7) -> 0x032464 (L11) lever, then 0x03690b (2554 B master data).
3. Clean wins still open: L10 0x03d75c/0x010efc/0x01545e/0x03c231, L11 0x03b718/0x03f15f/
   0x01348f/0x016b8f, L12 0x038cde/0x03c853/0x03d9cf, L17 0x034c40.

## 2026-07-13g checkpoint (441/528, suite 1880 PASS 0 FAIL, clean @ 1d72d45)

Landed: **0x043a7f** (L5 format scanner), **0x041ffc + 0x042023** (L6/L7 scan front end, L7 tier
complete), **0x032464** (L11 numeric text-table loader), plus the *untracked* helpers 0x041fa6
(next-char), 0x041ff4/0x0439fd (io-node put-back) and the constants kXlat_5302c / g_dat_62ac8 /
g_dat_62b90. Also banked the full x87-emulator analysis (docs/12-functions/0461cc-fpu-emulator.md,
analysis/disasm/FUN_000461cc.txt) and **scripts/vad.py** (Ghidra VA = NIGHT.EXE file offset + 0x2600;
local capstone disasm / hex / word / dword dumps -- use it instead of round-tripping Ghidra).

### New reusable facts
1. **The scan/emit family is a scanf, not printf**: 0x043a7f walks a format string, literal chars
   must equal ctx->next() (mismatch = put-back + end), and %<conv> *writes through* a slot stream
   (`dest = **pos; *pos += 1`). 0x042023's EBX is the caller's vararg slot array = scanf's arg list.
2. **emit_chars' count sentinel**: ctx+0xc == 0xffffffff means "exactly one char" (0x43df7). The
   0x041ffc front-end leaves count as *uninitialised stack residue* in the original; the game only
   works because that residue is 0xffffffff, so the port pins the sentinel. Do not zero it.
3. **Widened-ctx layout clash (3rd occurrence)**: the port's emit_ctx/cmd_ctx widen the two callback
   slots to 8 bytes, so callees own +0x00..+0x18 (they write num/count/flags). Any extra per-ctx
   payload (e.g. the io node pointer that lived at orig ctx+0x08) MUST go in a tail field past
   +0x18, otherwise cmdline_parse's ctx->num write shreds the pointer's high half (SEGV).
4. **Overlapping-store quirks are load-bearing in 0x032464**: id words at +0x388 get DWORD stores
   (stride 2) and buffer B / block tail values get DWORD stores at BYTE stride 1 -- port them as-is
   (pass the byte address; the callee does the dword store).
5. **io nodes need 0x20 bytes in tests**: the refill chain fn_041c8f -> fn_041d18 -> fn_041d47 reads
   node+0x14+ (capacity etc.); a 16-byte res_reader on the stack trips ASan.
6. **kXlat_5302c** (DS 0x302c) is pure image constants: exactly one reference program-wide (the read
   in 0x032464), so no builder to port.
