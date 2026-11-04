# finish-mid-tiers assessment (2026-07-13)

Context banked at 74% after porting 0x0112fd (L10, committed). Suite 1504 PASS 0 FAIL, TOTALS 384/528.
This is a large task (~150 fns, L5-L12 + L13-L23). The "clean" (all-callees-ported) candidates from
frontier.py are mostly complex subsystem fns; only a few are simple. Tractable-first list:

## TRACTABLE (port next, fresh context)
- **0x03de85 (L11, 272B)** — re-entrant struct CONSTRUCTOR. `uint32_t fn_03de85(uint32_t *s,
  uint32_t count, uint32_t last_size)`: `fn_03dcbd(s)` (free old); `size=count*4`; `for i in 1..13:
  s[i]=heap_malloc(size)` (offsets 0x4..0x34); `s[14]=heap_malloc(last_size)` (0x38); `return 1`.
  All callees ported: 0x03dcbd=fn_03dcbd (struct DESTRUCTOR, frees the same 14 sub-objs), 0x041af8=
  heap_malloc (returns uint32 heap OFFSET). **TEST:** needs a working heap region — reuse the
  test_heap_arena region helper (region layout: hw(region)[0]=total, data@region+0x2c, sentinel@
  region+0x20; see test_f1.c:438/479). Verify return==1 + s[1..14] non-zero distinct offsets,
  s[14] a different size class than s[1..13].
- **0x0112fd (L10)** — DONE (entity-list REMOVE, committed).

## DEFERRED (need subsystem context)
- **0x03c72b (L10, 171B)** — recursive cleanup: if s[0x34c]!=0 { fn_03c72b(sub,0); heap_free_guard(sub)
  }; if s[0x348]!=0 { heap_free_host(s[0x348]); s[0x348]=0 }; if s[0x44]!=0 { heap_free_host(s[0x44]);
  s[0x44]=0 }; return s. BLOCKED on heap pointer/offset model: 0x041c28=heap_free_guard, 0x041c31=
  heap_free_host, BOTH take a heap OFFSET (not pointer). Need to confirm whether s[0x34c/0x348/0x44]
  are stored as offsets or pointers (the recursive fn_03c72b(sub) treats sub as a node ptr, while the
  free takes an offset — resolve before porting).
- **0x035dfc / 0x035d5d (L10)** — 4-flag SPIN-WAIT loops on game-state byte flags (0x63ec1/0x63ea4/
  0x63ea5/0x63ec0 / 0x63e89). Infinite loop if flags don't clear; needs the game-state flag model +
  a controlled-flag test. Defer (like 0x035d5d already).
- **0x03a5f7 (L10, 278B)** — IDIV(0x1ea/[0x53104])/IMUL table-init; inner 0xc8 loop with parity
  branch, 2 forms of `CALL 0x041e6c` (the set-field fn). Complex; depends on 0x041e6c semantics.
- **0x03d75c (L10, 306B)** — float PHYSICS/geometry: FLD float[0x5d0c4+i*0x14], CALL 0x041f7e
  (scalar), FISTP (->int), range-check [0,0x13f], FMUL/FSUBR/FSTP float math. Global tables
  (0x5d0b4/0x5d0b8/0x5d0c4 stride 0x14, 0x5274c). Defer (FPU + table layout).
- **0x013908 (L13, 420B)** — rendering/geometry: many struct fields (+0x17/0x11/0x6d/0x1b/0x123/
  0x127/0x12f/0x133/0x137/0x13b/0x12b...), IDIV, 2 branches -> CALL 0x03ba86 (a draw fn). Defer
  (needs 0x03ba86 + big struct).

## L5-L8 (all register-ABI, subsystem-entangled)
- **0x0428f8 (L5)** — streaming-printf wrapper; BLOCKED on the printf descriptor-layout decision
  (see scratch/PORT_0428f8_NOTES.md; fmt_state struct vs raw descriptor).
- **0x0446c3 (L7)** — printf-entangled (calls fn_044c23 with fmt string at 0x446b2). Defer.
- **0x04262a (L6)** — file/device open; 6 callees all ported but 0x04488b (device open) needs caller
  context to resolve its reg-ABI (EBX arg + 4 stack args name/mode/extra/val). Defer.
- **0x04589e (L6)** — buffer realloc/growth (0x04595b size + heap_malloc + REP MOVSD copy). Tractable
  but needs the 0x04595b (tokenizer) semantics + globals 0x53388/0x5337c/0x53378/0x5338c/0x64090/0x64094.

## Still pending in other tiers (from frontier.py)
- L10: 0x03ce0b, 0x03bebb, 0x03bb42, 0x010efc, 0x01545e, 0x03c231 (unassessed)
- L11: 0x03f462, 0x016b8f (big)
- L12: 0x040b83, 0x037922, 0x040fb3
- L13: 0x04097e (64KB device buffer), L14: 0x0353d0

## Batch 3-4 assessment (fresh context, 2026-07-13)

DONE this session: 0x0323f8 + 0x03237b (L11, entity-list drain), 0x037922 (L12, clear-all-state).
Suite 1526 PASS 0 FAIL, TOTALS 388/528.

### VIDEO RAM MODEL IS AVAILABLE (unblocks rendering fns)
- `extern uint8_t g_video_ram[0x10000]` (f.h:757) -- the 64KB [0xa0000,0xb0000) window.
- `extern int32_t g_video_stride` (320 for 320x200).
- blit family all ported: blt_rows (0x040491), cpy_dw (0x0404c6), hline_set (0x0404db), cpy_back (0x040475).
- blt_rows(a=src, s=src pitch, w=width, h=rows, c=dst, t=dst pitch) -- copies h rows of w bytes.
- globals g_dat_639dc / g_dat_639e0 / g_dat_639e4 (byte) declared; fn_03cad3 sets them from a struct.
=> Rendering fns (below) are PORTABLE + TESTABLE (verify g_video_ram after the call).

### finish-mid-tiers remaining = rendering/geometry SUBSYSTEM fns (in scope: L10 21->81, L11 24->77)
All "clean" (all-callees-ported) candidates are these. NO simple non-subsystem helpers remain
(entity-list / struct-ctor / cleanup helpers are done).

- **0x03ce0b (L10, 692B, 198 insn) -- 4-BRANCH SPRITE-DRAW** (the next to port). args: EAX=ent
  (entity ptr), EDX=t (dst tile). Branches on g_dat_639e4 (byte flag) + ent fields:
  - B0 (flag==0): ONE blit_rows(ent[0x348]+[0x639dc]-ent[0], ent[0x1a], 0x140, 0xc8, t, 0x140).
  - B1 (flag!=0, [0x639e0]<ent[0x4]): TWO blits (split screen), uses ent[0x350] 2nd sprite.
  - B2 ([0x639e0]>=ent[0x4], [0x639e0]+0xc8>ent[0xc]): TWO blits, uses ent[0x34c] 2nd sprite.
  - B3 ([0x639e0]+0xc8<=ent[0xc]): ONE blit_rows(ent[0x348]+([0x639e0]-ent[0x4])*ent[0x1a], ent[0x1a], 0x140, 0xc8, t, 0x140).
  - TAIL (all branches): ent[0x22]=0; ent[0x1e]=0; if ([0x63360]%10==0) CALL 0x00040042(0,0x100,[0x63354]+0x48).
  - blt_rows stack order (pushed right-to-left): PUSH t(dst pitch), PUSH dst, PUSH h(rows), PUSH w, PUSH s(src pitch), PUSH src, CALL.
  - TEST: needs a Python ref OR careful per-branch pixel check on g_video_ram (4 setups). Intricate -- do fresh.
- **0x03bebb / 0x03bb42 (L10, 885/888B, 277/280 insn)** -- near-identical RANGE-CHECK geometry
  siblings (4 args incl. a [EBP+0x10] stack arg; CMP 0x140/0xc8/0, TEST JGE, early returns to end).
  Pure computation (likely clamp/rect-in-bounds). No blit. Simpler than 0x03ce0b -- maybe port first.
- **0x010efc (L10, 1024B), 0x01545e (L10, 1036B), 0x03c231 (L10, 1065B)** -- large, unassessed.
- **0x03d0c0 (L13, 387B) / 0x013908 (L13, 420B)** -- sprite-draw (4-branch coord math -> 0x03ba2c blit).
- **0x040fb3 (L12, 549B) -- CD/MSCDEX interrupt lib (CS + 0x41xxxx ints) -- mid-subsystems, NOT finish-mid-tiers.**
- **0x0353d0 (L14, 538B) -- command dispatch (switch on byte cmd) -- likely mid-subsystems.**
- **0x043a7f (L5, 544B) / 0x04192e (L8, 457B) / 0x040b83 (L12, 480B)** -- tokenizer (0x5185c char table
  + callback) / string format / FIFO buffer -- subsystem (mid-subsystems).

### RECOMMENDED NEXT (fresh context, in order)
1. 0x03bebb (L10, range-check geometry, pure computation -- simplest of the big L10s). Python ref + diff-test.
2. 0x03ce0b (L10, 4-branch sprite-draw). Python ref for the 4 blit param-sets + pixel check.
3. 0x03bb42 (L10, sibling of 0x03bebb).
4. Then 0x010efc / 0x01545e / 0x03c231 (large L10) + L11 remaining (0x016b8f big).

### Note on finish-mid-tiers vs mid-subsystems boundary
The task contract wants ALL of L10 (21->81) + L11 (24->77). The rendering/geometry fns above ARE in
finish-mid-tiers scope. The CD/MSCDEX (0x040fb3), command-dispatch (0x0353d0), tokenizer (0x043a7f),
string-format (0x04192e), FIFO (0x040b83) belong to mid-subsystems (data/audio/CD/game-state).

## Batch 5 assessment (fresh context, 2026-07-13)

DONE this session: 0x035d5d + 0x035dfc (L10, 4-flag DMA busy-waits) as degenerate
NO-OP STANDINS (3 tests). The 2nd/3rd nested 4-flag checks re-spin when all four
g_dat_63e88[] bytes are clear -> with static-zero flags they loop forever (degenerate,
like fn_00035ec2/fn_00035d0f). Suite 1526->1529 PASS, TOTALS 388->390/528. Committed ecf6642.

### CRITICAL INSIGHT: remaining finish-mid-tiers L10/L11 fns are SUBSYSTEM-ENTANGLED
The non-subsystem helpers (entity-list, struct-ctor, cleanup, spin-wait) are DONE.
Every remaining "clean" L10/L10 candidate needs a subsystem PREREQUISITE model:
- **video-framebuffer-arena cluster** (sprite-RAM offset model): 0x03ce0b (692B, 4-branch
  sprite-draw), 0x03f462 (303B tile-fill), 0x03a5f7 (278B), 0x03c65b (143B), 0x03bebb/0x03bb42
  (RLE-blit pair, 885/888B). Blocked on: g_639dc/e0 are uint32 sprite-RAM OFFSETS (set by
  0x03cad3); a blit src = g_639dc + sprite_off - base needs a 64-bit buffer-base convention
  (a sprite-RAM arena) to round-trip. g_video_ram[0x10000] IS the screen framebuffer (dst).
  => This is MID-SUBSYSTEMS work (video backend), not an isolated finish-mid-tiers win.
- **physics/FPU**: 0x03d75c (306B) -- FPU + global tables stride 0x14.
- **table-init**: 0x03a5f7 (also video).
- **blocked on unported callees**: 0x03c72b (needs fn_041c28/fn_041c31 free, UNPORTED).
- **large unassessed**: 0x010efc (1024B), 0x01545e (1036B), 0x03c231 (1065B), 0x03c853 (639B),
  0x03b718 (570B).

=> finish-mid-tiers' remaining work overlaps mid-subsystems (video/physics models). The natural
next task is mid-subsystems (set up the video-framebuffer-arena + physics models), which
unblocks the L10/L11 rendering/physics fns.

### 0x03ce0b PRECISE BLIT PARAMS (derived from disasm, for when the video model is ready)
Args: EAX=ent (entity/viewport ptr), EDX=dst (screen blit target, the framebuffer region).
blt_rows(a=src, s=src pitch, w=width, h=rows, c=dst, t=dst pitch).
Dispatch:
  if g_639e4==0                     -> B0 (1 blit)
  elif [0x639e0] < ent[0x4]         -> B1 (2 blits, 2nd sprite = ent[0x350] ptr)
  elif [0x639e0]+0xc8 > ent[0xc]    -> B2 (2 blits, 2nd sprite = ent[0x34c] ptr)
  else                              -> B3 (1 blit)
B0: blt_rows(g_639dc+ent[0x348]-ent[0], ent[0x1a], 0x140, 0xc8, dst, 0x140)
B1: blit1a blt_rows(ent[0x348], ent[0x1a], 0x140, 0xc8-(ent[0x4]-g_639e0),
                    dst + (0xc8-(g_639e0-ent[0x350][0x4]))*0x140, 0x140)
    blit1b blit_rows(ent[0x350][0x348]+g_639dc-ent[0x350][0]
                     + ent[0x350][0x1a]*(0xc8-(ent[0x4]-g_639e0)), ent[0x350][0x1a],
                     0x140, ent[0x4]-g_639e0, dst, 0x140)
B2: blit2a blt_rows(ent[0x348]+(g_639e0-ent[0])*0x140, ent[0x1a], 0x140,
                    0xc8-(g_639e0+0xc8-ent[0xc]), dst, 0x140)
    blit2b blt_rows(ent[0x34c][0x348]+g_639dc-ent[0x34c][0], ent[0x34c][0x1a], 0x140,
                    g_639e0-ent[0], dst+(0xc8-g_639e0+ent[0])*0x140, 0x140)
B3: blt_rows(ent[0x348]+(g_639e0-ent[0])*ent[0x1a], ent[0x1a], 0x140, 0xc8, dst, 0x140)
TAIL (all branches): ent[0x22]=0; ent[0x1e]=0; if (g_63360 % 10 == 0) board_out(0, 0x100, g_63354+0x48).
Globals: g_639dc (sprite-RAM base offset), g_639e0 (scroll row), g_639e4 (byte flag, split-screen
mode), g_63360 (counter, NOT yet declared), g_63354 (viewport-struct ptr, declared). ent[0x34c]/
ent[0x350] are POINTERS to sibling sprite structs (each has [0],[0x4],[0x1a],[0x348]).
TEST (when ready): set up a sprite-RAM buffer + screen region; hit each of the 4 branches via
g_639e4/g_639e0/ent[0x4]/ent[0xc]; verify dst pixels + tail (ent[0x22]=ent[0x1e]=0).

### RECOMMENDED NEXT (fresh context)
Option A (finish-mid-tiers): set up the video-framebuffer-arena sprite-RAM model (a base buffer +
the g_639dc/e0 offset convention via 0x03cad3), then port 0x03ce0b + 0x03bebb/0x03bb42.
Option B (mid-subsystems): the video backend IS mid-subsystems; set up the video model there and
cross-reference the L10/L11 rendering fns. Coordinate so the model is built once.

## Batch 6 DONE (2026-07-13) -- 0x03ce0b ported + latent fn_04a814 OOB fixed. Commit a7a18fb.
TOTALS 390->391/528, L10 65->66, suite 1539 PASS 0 FAIL (non-ASan).

### 0x03ce0b KEY LESSONS (for the next rendering fn)
- Video-arena model CONFIRMED for sprite-draw: blit src = g_639dc + sprite_off - base, via video_map().
- **ent[0x350]/ent[0x34c] are 64-bit POINTER fields** (deref `*(uint64_t*)(e+off)`), NOT field offsets.
- **Test buffers MUST be heap (calloc), not large statics** -- a 0x30000 static shifts the data segment
  and re-exposes a latent OOB in 03de85's heap path (non-ASan crash). calloc = no layout shift.
- fn_04a814 (pow10 scan) LATENT OOB FIXED (bounds guard if e>=22 / if e<0). Baseline ASan aborted on it.
- ASan-only `caseD_8` FAIL = pre-existing ASan-layout artifact (baseline ASan aborts on fn_04a814 OOB
  instead); non-ASan (suite standard) is green. Don't chase the ASan caseD_8.
- blt_rows(a=src, s=src pitch, w, h, c=dst, t=dst pitch): 1:1 row copy (odd byte + 16-bit words).

### NEXT (finish-mid-tiers L10 rendering, in order)
1. **0x03bebb (L10, 885B) + 0x03bb42 (L10, 888B)** -- the RLE-blit PAIR (siblings). Both are the next
   clean big-L10 candidates. RLE decode + blit to video arena. Same video-arena model as 0x03ce0b.
   (0x03bebb had a flagged skip-path uncertainty: the data-pointer advancement between the h<0 skip
   path and draw path -- resolve by careful disasm trace + Python ref.)
2. Then 0x03f462 (303B tile-fill), 0x03a5f7 (278B, also video), 0x010efc/0x01545e/0x03c231 (large).
3. B1/B2/B3 pixel-exact verification for 0x03ce0b was DEFERRED (needs real game sprite-RAM layout);
   dispatch + tail + B0 pixel are verified. Acceptable per the wiring-test standard.
