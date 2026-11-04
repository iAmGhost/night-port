# 0x03bebb (fill) + 0x03bb42 (copy) row-RLE blit — NEAR-COMPLETE, 1 SUBTLETY OPEN

## What's DONE and CORRECT (verified by 8 passing tests)
Both fns ported in f.c (fn_03bebb @~4670, fn_03bb42 @~4758), declared in f.h, tested in test_fn_03rle (10 checks, 8 PASS).
- RLE format: row = [N runs][per run: skip_byte, run_len, <run_len data>]. col_acc accumulates skip+run.
- Dest is a FLAT 0xa0000 video addr, deref via video_map(). Row pitch 0x140, 200 rows.
- mode0 forward: mem_copy (copy fn) / fn_041e20 memset (fill fn). left/right clip.
- mode1 reverse: cpy_back (REVERSED copy) / fn_041e20. Verified: fill m1 [15,20)=0x77, copy m1 [15,20)=[C4..C0].
- Bounds: dest_off<=0x13f, dest_off+col>=0, start_row<=0xc8, start_row+row_count>=0.
- skip-loop (neg start_row) advances by (col_byte+2) — faithful quirk replicated.

## THE OPEN SUBTLETY (why 2 tests fail)
Tests fill mode0 "dest_off=5" and "dest_off=-5" FAIL. When dest_off=0 (test1) it PASSES.
So the fill DESTINATION formula with nonzero dest_off is wrong in my C port.

EMPIRICAL GROUND TRUTH (dest_off=5, col=20, run skip=10 len=20, base=0xa0000):
  fill lands at [5,25) = 20 bytes. i.e. dest byte 5 (= dest_off), count 20 (= col).
My C port currently: db = base + start_row*0x140 + dest_off; fill dest = video_map(db + col_acc)
  -> for col_acc=10 that's byte 15, count right-col_acc=10. WRONG.
INSTRUMENTED run gave: db=0xa0005, dest_off=5, col_acc=10, rl=20, left=0, right=20.
  (left/right setup: left=0 since dest_off>=0; right=col=20 since dest_off+col<=0x140.)

DECOMPILER (unreliable here — merges EAX:EDX into bogus 64-bit lVar, uses (int)lVar=low32 as base):
  local_20 = (int)lVar + local_18*0x140 + iVar3    [iVar3=(int)(lVar>>32)]
  full:   FUN_00041e20(local_20 + local_34, fill, uVar2)
  right:  FUN_00041e20(local_20 + local_34, fill, uVar2 - iVar3)   [iVar3=local_34+uVar2-local_14]
  left:   FUN_00041e20(local_20 + local_24, fill, uVar2 - uVar4)   [uVar4=local_24-local_34]
  local_34=col_acc, local_24=left, local_14=right, uVar2=run_len.

KEY QUESTION to resolve: what is the TRUE 32-bit base? The canary thunk (0x0418ee) PRESERVES caller's
EAX. Caller FUN_00012efa @0x0133c5 does `MOV EAX,[EBP-0x4]; CALL 0x03bebb` — EAX is a caller LOCAL,
NOT obviously 0xa0000. EDX (2nd reg arg) = [obj+1] - [table+idx*4] - 0x639dc (a sprite/dest offset).
So my C sig `fn_03bebb(base, dest_off, start_row, col, row_count, rle, mode, fill)` may have the WRONG
mapping of the register args (EAX vs EDX vs the base). Re-derive the ABI from the caller's register setup
(EBX=start_row, ECX=col, EAX=?, EDX=?) and the mode0 fill dest in the disasm (0x0003c059-0x0003c0be).

## NEXT (fresh context, 1 focused pass)
1. Re-derive exact register->param mapping from FUN_00012efa (0x0133b5-0x0133c8) + fn prologue reg-save.
2. Fix the mode0 fill/copy DESTINATION (and mode1 if affected) so dest_off=5 -> [5,25), dest_off=-5 -> left-clip.
3. Re-verify all 10 tests; update test expected values to the FAITHFUL behavior (the port is the source of truth
   once the dest formula matches the disasm).
4. STATUS.md: mark 0x03bebb + 0x03bb42 [x] (L10 66->68, TOTALS 391->393). Commit.
