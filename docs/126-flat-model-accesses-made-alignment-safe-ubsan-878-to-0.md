# 126 — Every flat-model typed access made alignment-safe; UBSan on a full route: 878 → 0

## 1. Scale of the problem, and why the fix is a cast swap

docs/123 measured ~140–800 UBSan reports per fuzz seed, docs/125 §1 showed why: the shipped record layout is
packed and deliberately unaligned (the entity constructor `fn_010efc` stores dwords at `+0x01`, `+0x05`,
`+0x17` …), so *every* typed C access into that layout is a misaligned access on the host ABI. Measured on the
`ep1_kill3` route under `-fsanitize=address,undefined`: **878 reports over 771 distinct sites**.

Rewriting each site as a memcpy accessor would have to restructure assignment, `+=`, `++`, and `&(...)`.
A packed struct whose member has the original type is an alignment-safe **lvalue** view on gcc/clang, so only
the cast changes:

```c
*(uint32_t *)(e + 0x17)      ->   ((struct ua32 *)(e + 0x17))->v      /* struct UA_ATTR ua32 { uint32_t v; } */
```

That preserves every expression form *and* the exact byte width the shipped instruction used — width is what
the flat model depends on, which is why `*(uint8_t **)` sites become an 8-byte view rather than 4.

## 2. The transform

`scripts/port/unaligned_casts.py` (committed) rewrites `src/f/f.c` (5825 sites) plus `main.c` (7),
`rig_sink.c` (2), `cd_iso.c` (1); `f.h`, `sys.c`, `fx_04794a.c` had none. Six view types: `ua16/sa16`,
`ua32/sa32`, `ua_uint8tp` (pointer store), `uafl` (float). Four things the transform has to get right, each
found by breaking it first:

* **star count** — `*(T **)x` reads a `T *`, not a `T`; ignoring the second `*` silently changed widths.
* **cast-chain operands** — `*(uint16_t *)(uintptr_t)c` has no parenthesised address; taking `(uintptr_t)` as
  the whole operand produced `((struct ua16 *)(uintptr_t))->vc` garbage.
* **recursion into the address operand** — a nested `*(const uint32_t *)(...)` inside an address stays
  misaligned unless the operand is transformed too; and when the outer cast needs no view (8-bit, `void *`)
  the scanner must advance only past the cast head, or nested sites are skipped.
* **comments and strings** — the transcription quotes shipped code inside comments; an early version invented
  `struct ua_u8tp { u8_t * v; }` from a `*(u8_t**)` that existed only in a comment. Comment/string spans are
  now skipped.

Result on the same route, same drive, same flags:

```
runtime errors 878 -> 0        AddressSanitizer errors 0        rc=0
make test  == ALL PASS ==
make selftest  ALL PASS (0 failures)
```

## 3. One genuine out-of-bounds read found on the way (docs/112/113 family)

```
src/f/f.c:13988: runtime error: index 61 out of bounds for type 'uint32_t[61]'
```

`fn_03f6bc`'s timed-event walk compares a slot against the DWORD at `DAT_53200 + 4*cursor` and lets `cursor`
run to `TAB532_WALK_LIMIT = 0x44` (68). The port models the tick table as `g_tab_53200[61]` (it ends exactly
at `DAT_532f4`), so cursors 61..67 index past the array — the shipped code has no array, it just addresses DS
bytes that happen to be the *next* DS field. Fixed by reading through the contiguous block that already models
that span:

```c
((const struct ua32 *)((const uint8_t *)&g_ds_53200 + 4u * cursor))->v
```

Values were already identical by luck (the struct's next member begins at the same offset the OOB index
reached), so this is a legality fix that also removes a latent trap: C gives no guarantee that the two
modelled arrays stay adjacent, which is the same "one DS span modelled as separate arrays" family as
docs/112 / docs/113.

## 4. Two arithmetic UB sites, fixed because they are real bugs not style

* `f.c:6727` — `28457 * 86400` overflows `int32_t` (signed overflow is UB; x86 wraps). Now done in the
  unsigned domain and converted back, matching the shipped wrap.
* `sys.c:250` — `((int32_t)b - 128) << 8` shifts a negative value (UB). Replaced by `* 256`, same value.

## 5. Equivalence evidence for a 5835-site edit, and an honest caveat

* Test suites: `port test` ALL PASS, `port selftest` ALL PASS (0 failures) — these are byte-exact fixtures.
* State oracle vs the shipped original on a fresh full route (`/tmp/rig/g3`, 928 original states / 274 windows):
  the post-refactor DIFF set is a **strict subset** of the pre-refactor run (`/tmp/rig/g2`): **0 cells newly
  divergent, 4 divergent cells cleared** (`0x5302c`, `0x53100`, `0x6336c`, `0x633b8`).
* Caveat that the caveat deserves: the port is still not reproducible run-to-run on identical input, even with
  `NIGHT_SEED=7` — three runs of the same binary/drive gave `hp = 0x3e8` in all 676 live samples, then
  `0x3e8/0x3e7`, then `0x3e8/0x3d4/0x3d0`, with EXP totals 0x14 / 0xa / 0xa. So a record-level before/after
  claim from a single pair of runs proves nothing; set-inclusion (above) and the fixture suites are the sound
  form of the argument, and port-side determinism remains an open rig item (docs/101 residuals).
* Full-tally note so numbers are not cherry-picked: on this longer route the oracle's per-window tally is
  `SAME 55 / DIFF 107 / PTR 52 / SIZE? 33 / REPR 22 / ENTROPY 14 / REC-SAME 10 / REC-DIFF 36 / REC-NULL 3`.
  The 107 DIFFs are long-window dynamic divergence (the two sides are not frame-locked across 300 s), not new:
  the pre-refactor run shows 111. The "269/269 SAME" figure from docs/122 was the invariant-cell subset.

## 6. What this unblocks

The closeout contract's nine-episode ASan soak (0 reports / 0 SEGV / all rc=0) is now reachable: the previous
blocker was this alignment class, and the soak needs a larger per-seed `--cap` because ASan costs roughly the
frame budget (docs/123 §6: 4/12 seeds missed 2500 frames in 150 s).
