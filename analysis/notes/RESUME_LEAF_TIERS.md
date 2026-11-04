# RESUME — leaf-tier gating map + 0x044ab5 spec (fresh-context handoff)

Written after the 0x049613 C-mirror landed (commit 828b6bc). Suite 1464 PASS 0 FAIL, TOTALS 375/528.
Date-arithmetic cluster now fully [x]: 0x04574a / 0x049506 / 0x0495eb / 0x049613.

## Why finish-leaf-tiers is still not done
Contract: L0 112/112 (DONE), L1 47/47, L2 32/32, L3 21/21.
Remaining leaf fns and what blocks each (verified via Ghidra disasm this turn):

| fn (tier, size) | blocked on (unported/deferred callee) |
|---|---|
| 0x0440a9 (L1, 733B) | hidden indirect `CALL [0x53648]` (gated, dedicated pass) |
| 0x046d32 / 0x046d47 / 0x0473e5 (L2, caseD_0/8/4) | indirect tail-jump `JMP [EBP+0x76]` (FPU-interp EBP frame model) |
| 0x04a48f (L3, 485B) | self-contained FPU composite (all 5 callees [x]) — LARGEST remaining leaf |
| 0x044c23 (L4, 223 insn) | callback data parser; needs fn_0451bc + fn_044ea4 (both [x]) — the KEY L4 unlock |
| 0x044ab5 (L6, 167B) | **NONE — clean single win (see spec below)** |
| 0x0426f2 (L7, 26B) | 0x044ab5 (clean) + 0x04262a |
| 0x04270d (L8, 9B)  | 0x0426f2 |
| 0x04262a (L6, 199B) | **0x044859, 0x044b5d (both UNPORTED)** — blocks the 0x0426f2/0x04270d chain |
| 0x042023 (L7, 32B) | 0x041ffc |
| 0x041ffc (L6, 38B) | 0x043a7f (L5, 544B, deferred) |
| 0x0428f8 (L5, 47B) | 0x044c23 (the L4 callback parser) |
| 0x0446c3 (L7, 132B) | **0x044c23** (called @0x04470b) — blocks 0x042394 (L8) → 0x036537 (L11) |

### Unblocking order (cheapest first)
1. **0x044ab5** (clean, self-contained) — spec below. Then 0x0426f2/0x04270d become reachable
   ONLY if 0x04262a is also done (needs 0x044859+0x044b5d first).
2. **0x044c23** (223 insn) — unlocks 0x0446c3 → 0x042394 → 0x036537 (L11) AND 0x0428f8 (L5).
   Biggest bang: 4 fns behind it.
3. **0x04a48f** (485B FPU composite) — self-contained, largest single leaf.
4. **0x0440a9** (733B [0x53648] parser) + **caseD family** (needs FPU-interp frame).

## 0x044ab5 (L6, 167B) — CLEAN SINGLE WIN (disasm captured, 57 insns)
Node-pool allocator. All 3 callees ported: fn_041af8 (heap_malloc), fn_041e20 (memset),
fn_042a0d (g_dat_64008 = x, the alloc-fail error sentinel).

**Globals to model (2 new):** `g_dat_63ff8` = [0x00063ff8] (pool head ptr), `g_dat_63ffc`
= [0x00063ffc] (pool tail ptr). Node layout (0x1a bytes): [+0]=prev, [+4]=next, [+0xc]=flags.
**Pool table** @0x53410: 26 entries, stride 0x1a, ends at 0x53618; entry is "free" iff
`[entry+0xc] & 0x3 == 0`.

Pseudocode (EAX = returned node ptr, or 0 on alloc-fail):
```
head = g_dat_63ff8
if (head):                      # warm: reuse head->next
    node = head->next           # [head+4]
    flags = (node->flags & 0x4003) | 0x3
    g_dat_63ff8 = head->prev    # [head+0]
else:                           # cold: scan pool for a free slot
    for e in [0x53410 .. 0x53618 step 0x1a]:
        if (e->flags & 0x3)==0:
            node = heap_malloc(8);  if(!node) fatal(5)
            flags = 0x3; break
    else:
        node = heap_malloc(0x22);  if(!node) fatal(5)
        node += 8; flags = 0x4003
mem16(node, 0, 0x1a)            # fn_041e20
node->flags = flags
# relink (careful: ESI keeps = ORIGINAL head across the warm path):
g_dat_63ffc->?  # see note — the relink writes need exact reg tracking
g_dat_63ffc = original_head
original_head->next = node
original_head->prev = <old tail [0x63ffc]>
return node
```

**CAUTION (the subtle bit — verify before committing the C):** in the warm path ESI keeps
pointing at the ORIGINAL head; the tail relink writes are `[ESI+4]=ECX`, `[0x63ffc]=ESI`,
`[ESI]=EAX` where `EAX=[0x63ffc]` (old tail) read AFTER `[0x63ff8]` was already set to
`head->prev`. Trace register-by-register into a Python ref (`scratch/fp_044ab5_ref.py`)
and diff-test against a hand-built 2-3 node pool before trusting it. The `memset 0x1a`
zeroes the returned node's own next/prev — confirm whether the list expects the returned
node to be detached (dangling head->next) or re-linked; this decides the exact relink form.

## Test plan for 0x044ab5
- Build a small pool: set g_dat_63ff8/g_dat_63ffc + a couple of node slots; exercise warm
  (head!=0) and cold (head==0, one free slot in table; head==0, no free slot → heap_malloc(0x22)).
- Assert returned ptr + node->flags + head/tail relink after each call.
- fatal(5) path: force heap_malloc to return 0 (if the port's heap arena can be exhausted,
  else assert g_dat_64008==5 via a controlled arena).
