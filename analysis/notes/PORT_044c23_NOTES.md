# Fresh-context target map (leaf-tier residual, as of 0x044ab5 commit fcb7770)

State: TOTALS 376/528, suite 1479 PASS 0 FAIL. Current task = finish-leaf-tiers (L0-L4);
0x044ab5 (L6) went to finish-mid-tiers scope.

## Key insight (probed this session, not previously recorded)
The remaining SMALL L6/L7 functions are NOT isolated leaves -- they form a
**callback-invocation subsystem**:
- **0x042023** (L7, 13 insn, 32B): reg/stack wrapper. 3 stack args (arg1/arg2/arg3).
  Calls 0x041ffc with EAX=arg1, EDX=arg2, EBX=&(local { &arg3 }) (local at [ESP] = &arg3).
- **0x041ffc** (L6, 14 insn, 38B): `0x041ffc(arg)`. Builds a local 3-dword struct
  `{0x41fa6, 0x41ff4, arg}` on the stack and calls **0x043a7f(EAX=&local)**.
  0x41fa6 / 0x41ff4 are constants in the code segment (fn-ptr-ish) -> 0x043a7f is a
  "call a callback / dispatch" helper. Chain: **0x042023 -> 0x041ffc -> 0x043a7f -> ...**.
So porting 0x042023/0x041ffc requires the whole chain incl. 0x043a7f's callee set and
callback semantics. Do it as a small subsystem pass, not 3 isolated ports.

## Every remaining leaf-tier function is either a large composite or part of that chain:
- **L1 {0x0440a9}**: 733B parser, hidden indirect `CALL [0x53648]` (gated; needs the
  0x53648 slot parser + FPU-interp EBP frame model).
- **L2 {0x046d32, 0x046d47, 0x0473e5}**: caseD_0/8/4 dispatch, indirect tail-jump
  `JMP [EBP+0x76]` through the FPU-interp EBP frame (needs that frame model).
- **L3 {0x04a48f}**: 485B self-contained FPU composite (largest single leaf; ->
  0x04a2a7/04a367/0x04a7fa/0x04a8b9/0x04a9da, all [x]).
- **L4 {0x044c23, +1}**: 0x044c23 = 223-insn callback-based data parser (reg-ABI
  EAX=?, EBX=iter-state, ECX=callback fnptr, EDX=buf; stores ECX at [ESP+0x44], calls
  it x9; direct CALLs fn_044ea4 + fn_0451bc [both x]; parses 'n'-tagged format, LES
  far-ptr writes). **KEY UNLOCK** -> frees 0x0446c3 -> 0x042394 -> 0x036537 (L11) +
  0x0428f8 (L5).

## Recommended fresh-context order
1. **0x044c23** (L4, key unlock; 4 fns behind it) -- or
2. **callback subsystem {0x042023, 0x041ffc, 0x043a7f}** (small; get the 0x043a7f
   callee set first).
Then 0x04a48f (L3 FPU composite), 0x0440a9 (L1 [0x53648] parser), caseD family (L2).

## Memory model (settled this session, commit fcb7770)
Node-pool family: nodes in **g_heap_base** (heap_malloc), records in **g_ndpool** at
offset 0x53410 (26 x 0x1a, flags @+0xc); overflow record in-heap at node+8. See
fn_044ab5 (f.c) + scratch/PORT_044ab5_SPEC.md.
