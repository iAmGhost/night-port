# 123 — First ASan/UBSan sweep over the stored fuzz corpus: no memory errors, but the flat-model heap does unaligned dword accesses everywhere

Date: 2026-10-29. Drives: the 12 earliest fuzz seeds from `/tmp/rig/fz9` (stages 1-9), built binary
`port/test/night-asan` (`-fsanitize=address,undefined`, `make -C port asanapp`), `--frames 2500`,
150 s cap each, 4 parallel.

## 1. Result

- **No `AddressSanitizer` memory errors** (no heap-buffer-overflow / use-after-free / SEGV) in any run.
- **UBSan reports are voluminous: ~140-800 per run.** Distinct sites, by frequency over the 12 runs:

| site | report |
|---|---|
| `f.c:413` (`mem_copy`'s dword loop) | `store to misaligned address …35ac2 for type 'uint32_t'` — hundreds of hits |
| `f.c:8334, 8335, 8336` | `store to misaligned address … uint32_t` |
| `f.c:8337, 8338, 8339` | `load of misaligned address … uint32_t` |
| `f.c:5339` | `load of misaligned address … uint32_t` |
| `f.c:6648` | `signed integer overflow: 28457 * 86400 cannot be represented in type 'int'` |

The faulting addresses all end in `…ac2` — i.e. `+2 mod 4`: dwords being read and written at
two-byte-aligned addresses inside heap records.

## 2. Why this is a real port issue, not sanitizer noise

`mem_copy` (the port of the guest's `REP MOVSD`-style copy) is:

```c
uint32_t *d = (uint32_t *)(uintptr_t)a;      /* a is a flat-model offset, often only 2-byte aligned */
const uint32_t *s = (const uint32_t *)(uintptr_t)b;
while (n--) { *d = *s; d++; s++; }
```

On the guest (32-bit x86) an unaligned dword access is legal and the shipped program relies on it. In C on
arm64 the same thing through a typed pointer is **undefined behaviour**: clang is allowed to vectorize or
reorder it, and `-O2` builds have produced crashes in other ports the same way. So the port reproduces the
shipped *behaviour* by relying on a host property the language does not guarantee.

## 3. Fix plan (next step, small and mechanical)

1. Add unaligned-safe flat-model accessors next to the heap helpers:
   `ld32u(p)` / `st32u(p,v)` implemented with `__builtin_memcpy`, and use them in `mem_copy` and at the
   sites above (`f.c:413`, `5339`, `8334-8339`).
2. Pin the arithmetic overflow at `f.c:6648` to the guest's wrapping 32-bit semantics (unsigned multiply),
   instead of signed UB.
3. Re-sweep the same 12 drives and require **0 UBSan reports**; then widen the soak (see §4) as part of
   closeout.

## 4. Note for the closeout soak

4 of the 12 runs did not reach the 2500-frame budget within 150 s under ASan, so the nine-episode ASan soak
in the closeout contract needs a larger `--cap` (or fewer frames) per seed; the sweep above is a *smoke*
pass, not the soak.

## 5. Fix applied to `mem_copy`, and the honest scope of the problem

`mem_copy`'s dword loop now copies through byte pointers into a local `uint32_t` (two `__builtin_memcpy`
calls). Semantics unchanged, and its own sites are clean: grepping the re-swept log for `f.c:41x` now yields
**0 reports** (it was the single hottest site before).

But the sweep also shows the problem is **systemic, not local**. Re-sweeping the same seeds:

| seed | before | after |
|---|---|---|
| r0s0_st1 | 433 | 404 (389 **distinct** sites) |
| r0s1_st2 | 385 | 312 |
| r0s2_st3 | 424 | 364 (this seed now aborts under UBSan, `rc=134`) |

So the remaining reports come from ~390 *different* lines per run. That is the port's central abstraction:
guest memory is addressed as flat offsets and read through typed pointers
(`*(const uint32_t *)(hb + rec + 0x20 + 4*i)` and friends) everywhere in `f.c`. On x86 that is what the
shipped program does; in C it is undefined wherever the offset is not a multiple of the accessed type —
which the guest's packed record layouts routinely violate (the faulting addresses cluster at `+2 mod 4`).

Consequence for the closeout contract ("nine-episode ASan soak, 0 reports"): reaching 0 requires routing the
flat-model accesses through unaligned-safe accessors across the whole f-layer — a mechanical but large
refactor (or a per-record `packed` view type), not a spot fix. That should be scheduled as its own work item
rather than claimed here. `make -C port test` is ALL PASS with the `mem_copy` change.
