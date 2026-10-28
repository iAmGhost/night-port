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
