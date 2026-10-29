# 128 — Fuzz drives under the sanitizer: 8/9 clean, and one crash that is NOT rig-induced

## 1. Cross-product the two rigs: sanitizer × fuzz corpus

docs/127 proved the nine episodes are clean under ASan+UBSan on *scripted* drives. The same build over the
stored fuzz corpus (`/tmp/rig/fz10/*/drive.txt`, 9 drives, `CAP=200`, `--frames 3000`) gives:

| drive | rc | UBSan | ASan | SEGV |
|---|---|---|---|---|
| r0s0_st1 | **134** | 0 | **1** | 1 |
| r0s10_st2 | 0 | 0 | 0 | 0 |
| r0s11_st3 | 0 | 0 | 0 | 0 |
| + 6 more | 0 | 0 | 0 | 0 |

## 2. Correction to docs/120's crash attribution

docs/120 concluded that *all 8 fuzz SIGSEGVs were injection-induced*, based on a counterfactual that removed
`poke/ent/play` lines. That conclusion does not generalize: **this drive contains zero injection verbs**
(`grep -cE "^(poke|ent|play) " = 0`), and stripping state writes anyway still crashes identically. So random
input alone reaches a real fault — first such case, and it means the fuzz corpus is finding something the
injection counterfactual could not exonerate.

Stack (ASan):

```
SEGV READ on unknown address 0x000000000030
#0 fn_012efa  f.c:14324      <-- *(e + 0x30), e == NULL
#1 fn_03203d  f.c:14397      <-- the entity DRAW SWEEP
#2 fn_03801b -> fn_03a829 -> fn_03af18 -> main
```

## 3. The invariant that broke, measured rather than guessed

`fn_03203d` walks `g_dat_53c70` through the `+0x13f` next links for `g_dat_53c68` iterations, re-reading the
count every test and never re-validating the walk pointer — the port's own header comment says so. A witness
added here prints the first violation:

```
draw sweep: entity slot 3 of count 8 has a NULL record (list head 0x101308898, list shorter than DAT_53c68)
```

**DAT_53c68 claims 8 entities; the live `+0x13f` chain ends after 3.**

## 4. Host-safety fix that models the shipped behaviour instead of papering over it

The guest would address DS-offset 0x30 of a null-segment pointer; under a DOS extender linear 0 is mapped to a
zero page, which is exactly what the port already models for the null resource node in `fn_040839`
(`if (!cursor) cursor = sys_linear0();`). Same modelling applied to the two list walks that trust the count
(`fn_03203d`, and `fn_013aad`'s sort walk where `nxt` is deref'd before its null check):

```c
if (!cur) { g_sweep_null_hits++; cur = sys_linear0(); }   /* zero page, docs/128 */
```

After it: UBSan 0 reports, the sweep no longer faults at the NULL slot — and a *second*, more interesting
fault surfaces:

```
SEGV READ on 0x00017f719047   #0 fn_012efa f.c:14335   (dangling entity pointer, not a heap address)
```

i.e. past the null slot there is an entity pointer that points **outside the flat heap** — a block that was
freed and whose link bytes were overwritten by a later allocation. ASan cannot see this class: the flat model's
allocator is the port's own arena (`heap_malloc` at `f.c:3351`), so freed blocks never return to the host
allocator.

## 5. What was checked and ruled out on the way

* **Stale widened-pointer offsets** — the port moves `prev` from the shipped `+0x143` to `+0x147` (8 B). No
  site anywhere still touches `0x143`; all 18 accesses use `0x147`.
* **Allocation size vs the widening** — entity blocks are allocated `fn_00041bdc(0x14f)` (three sites), which
  exactly fits `prev@+0x147 + 8`, so the widened field does not run into the next block.
* **Free-without-unlink** — every entity free path (`fn_0323f8`, the conditional drain at `0x03237b`, the
  resource-drain path at `f.c:1117`) calls `fn_0112fd` before `heap_free_guard`, and `fn_0112fd` decrements
  `g_dat_53c68` on every branch (head/tail/middle/count==1).

## 6. Open item, with the two concrete next steps

1. **Make count-vs-chain an oracle on both sides.** The port now has the witness; the shipped side can be
   measured by walking `+0x13f` from `0x53c70` through the memory API in `replay_original.py`'s sampling loop
   and comparing against `0x53c68`. If the shipped game also shows `count > chain` on this same drive, the port
   is faithful and only the fault *landing* differs (zero page vs stale bytes); if it never does, the port is
   corrupting the list and the divergence is a genuine bug.
2. **Give the arena its own sanitizer.** Free entity blocks with a magic word (`0xdeadbeef`) and verify it at
   the deref sites; without that, ASan is blind to lifetime errors inside `g_heap_base`, which is where this
   crash lives.
