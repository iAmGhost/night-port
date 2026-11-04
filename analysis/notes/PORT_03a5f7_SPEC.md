# 0x03a5f7 — progressive horizontal reveal of a 200-row source buffer into the A0000 arena

L10, 278 B (0x03a5f7..0x03a70d). Callee set: 0x0418ee (canary thunk), 0x041e6c (mem_copy).
Only external state: `g_dat_53104` (divisor), `g_dat_63e24` (shared VBLANK/timer counter).
CLEAN — no unported callees.

## Caller
`FUN_0003a70e` @ 0x03a804: `MOV EAX,[0x633b0]; CALL 0x03a5f7`. So **EAX (base) = g_dat_633b0**
(the source sprite buffer). Model as `void fn_03a5f7(uint32_t base)`.

## Globals
- `g_dat_53104` (0x53104): divisor for the row-progress gate. Written `= 0x46` (70) by
  FUN_0003a829 @ 0x03a852, and also by VESA-mode setup @ 0x0150fa. `n = 0x1ea / g_dat_53104`
  (signed IDIV) = 490/70 = 7 in the default config.
- `g_dat_63e24` (0x63e24): shared VBLANK/timer counter. Init `= 0` here; SUB `n` each outer
  iter. ALSO written by timer/irq handlers (0x03d244, 0x038098, 0x0381d7, 0x039c03, ...).

## Structure (ground truth from raw disasm)
```
canary(base, 0x28); [EBP-4]=base; [EBP-8]=colAcc=0; g63e24=0
outer:                        ; colAcc = 0,4,8,...,316  (colAcc < 0x140, +4; 80 iters)
  n = (int)(0x1ea / g_dat_53104)
  if (n > g63e24):            ; CMP n,g63e24; JLE skip
    for (row=0; row<0xc8; row++):
      if (row%2==0):          # EVEN
        mem_copy( 0xa0000 + (row+1)*0x140 - colAcc - 1,   # dst  (video)
                  base   + row*0x140,                     # src  (buffer)
                  colAcc + 1 )                            # cnt
      else:                   # ODD
        mem_copy( 0xa0000 + row*0x140,                    # dst  (video)
                  base   + (row+1)*0x140 - colAcc - 1,    # src  (buffer)
                  colAcc + 1 )                            # cnt
  [busy-wait: while (n > g63e24) {}]   # 0x03a6d9 JG 0x03a6c0 — VBLANK sync
  g63e24 -= n; colAcc += 4; if (colAcc >= 0x140) break
```

### mem_copy operand mapping (EAX=dst, EDX=src, EBX=cnt)
Decompiler SWAPPED dst/src via its EAX:EDX 64-bit merge; raw disasm is authoritative.

| branch | dst (video) | src (buffer) | cnt |
|--------|-------------|--------------|-----|
| EVEN (row%2==0) | 0xa0000 + (row+1)*0x140 - colAcc - 1 | base + row*0x140 | colAcc+1 |
| ODD  (row%2!=0) | 0xa0000 + row*0x140 | base + (row+1)*0x140 - colAcc - 1 | colAcc+1 |

## BEHAVIORAL NOTE (busy-wait omitted)
The `JG 0x03a6c0` busy-wait spins while `n > g63e24`, waiting for a VBLANK/timer INTERRUPT to
advance `g63e24`. With n=7 and g63e24 starting at 0 (and only SUB'd by n after the wait), the
condition stays true and the loop NEVER terminates without external modification. In the port
there is no such interrupt, so the wait is modeled as a **no-op**. This has zero effect on the
memory-copy sequence (g63e24 only feeds the gate; with n>0 it is always satisfied once, driving
all 80 outer iterations). Observable memory operations are replicated 1:1.

### Resulting behavior (n=7, g63e24 goes 0,-7,-14,...):
- 80 outer iterations (colAcc 0..316), inner 200-row loop runs each → 16000 mem_copy calls.
- Each reveals a (colAcc+1)-byte run; colAcc grows 4/iter → progressive left-to-right wipe.

## Bounds (for the test source buffer)
- src max addr = base + 200*0x140 - 1 = base + 0xd000 - 1 → buffer >= 0xd000 bytes.
- dst max addr = 0xa0000 + 200*0x140 - 1 = 0xad000 - 1 → within g_video_ram (0x10000). OK.

## Test plan
- Set g_dat_53104 = 0x46 (default). Source buffer of distinct bytes.
- Verify a few known (row, colAcc-iter) destinations got the right source bytes:
  - colAcc grows each iter, so the LAST write wins. Final colAcc=316 run covers the region.
  - E.g. after the full call, check g_video_ram at EVEN/ODD row offsets for the expected
    source pattern (last-iteration colAcc=316 wins for the visible window).
- Verify g_dat_63e24 final value = 0 - 80*7 = -560 (as uint32_t).
- Edge: g_dat_53104 large enough that n=0 → inner loop never runs, g_video_ram untouched,
  g63e24 unchanged (0). Verify no writes.
