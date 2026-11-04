# PORT SPEC: 0x0440a9 (L1, 733 B, 241 insns) — stream token parser → list append

**Status:** fully analyzed. All callees PORTED. C mirror + diff test deferred to a
fresh context (241 insn, intricate digit state-machine, list-pool model, complex
test setup).

## What it is
Reads one whitespace-delimited **token** from an `emit_ctx` stream reader, parses it
(integer / fractional / exponent / sign / hex-like), builds it in a local char buffer,
(optionally converts it via the `0x53648` converter slot), appends a **4- or 8-byte
element** to a caller-managed list-pool, and returns the char count.

## Signature (reg-ABI, flat model)
```c
uint32_t fn_0440a9(struct emit_ctx *ctx, uintptr_t *pool_cursor);
```
- arg1 (EAX) = `ctx` — the `emit_ctx` stream reader: `next`@+0x00, `emit`@+0x04,
  `count`@+0x0c, `flags`@+0x10 (see f.h `struct emit_ctx`).
- arg2 (EDX) = `pool_cursor` — pointer to the running cursor into a caller-set-up
  array of 32-bit (or 64-bit, flags&0x4) POINTERS. Each slot holds a pointer to a
  value location; the fn advances the cursor and writes the parsed value there.
- Returns EDI (char count) in EAX.

## emit_ctx flags (byte at +0x10) — parsed behavior
- `0x01` — "produce output": if clear, the finalize/append phase is skipped
  (return only the count).
- `0x02` — after the leading-whitespace skip, jump straight to the near-end
  (0x0442c9); and `emit`/`emit_step` peek (do not consume).
- `0x10` — "integer/native" mode: the parsed value is kept as the 32-bit numeric
  accumulator `[ESP+0x68]` (NOT run through the 0x53648 converter); the sign is
  applied by `NEG [ESP+0x68]` when the first char is '-'.
- `0x20` / `0x40` — store the 64-bit converter result (result64) as two dwords.
- else (default) — store `fx64_to_f32(result64)` (a 32-bit float).
- `0x04` — 8-byte (far) pool entries (cursor advances by 8).
- `0x08` — 4-byte pool entries (cursor advances by 4).
- default — 4-byte pool entries.

## List-pool model (finalize, 0x044300-0x044379)
```
old = *pool_cursor
if (flags & 0x4) { *pool_cursor = old + 8; }   // far entry (flat: use offset only)
else             { *pool_cursor = old + 4; }   // near entry
uintptr_t elem = *(uintptr_t *)old;            // the pointer stored in the slot
// write the value:
if (flags & 0x10)      *(uint32_t *)elem = acc_num;              // [ESP+0x68]
else if (flags&0x20 || flags&0x40) { *(uint32_t *)elem = r64_lo; *(uint32_t *)(elem+4) = r64_hi; }
else                   *(uint32_t *)elem = fx64_to_f32(r64_lo, r64_hi);
```
(Far/near `LES`/`ES:[x]` collapse to flat 32-bit in the port; the value is deref'd
through `elem`.)

## Converter slot (0x0442fa `CALL [0x53648]`)
- Only invoked when `flags & 0x1` set AND `flags & 0x10` clear.
- `CALL (buffer, &result64)` where buffer = local char buffer (NUL-terminated),
  result64 = `&[ESP+0x50]` (a 64-bit OUT slot: lo at +0x50, hi at +0x54).
- Static value at 0x53648 = 0x045dac = **fn_045dad** ("report FPU-not-loaded"
  error thunk, PORTED) — the DEFAULT/placeholder. At runtime the real game sets it
  to a string→64-bit converter. Model as a global `g_fp_0442fa` init'd to a stub
  (writes 0 to result64), overridable in the test.

## Control-flow phases (exact addresses)
- **P1 skip leading whitespace (0x0440c0-0x0440dc):** `for(;;){ c=ctx->next(ctx);
  if (kCharClass[(c+1)&0xff] & 0x2) { ws_count++; continue; } else break; }`
  `EBP=EBX=c` (the first non-ws char). `ws_count` = `[ESP+0x6c]`.
- **P2 flags/count/sign (0x0440de-0x044121):** if `flags&0x2` → goto near-end
  (0x0442c9). `count=ctx->count; ctx->count--; if(count==0) goto 0x0442c2.`
  if `c=='+'||c=='-'`: `buf[0]=c; c=emit_step(ctx); buf_ptr++; ws_count++; [ESP+0x6c]=ws_count; if(c==-1) goto 0x0442c9;`
- **P3 integer-part digits (0x04413c-0x04419a):** if `kCharClass[(c+1)&0xff]&0x20`
  (digit) or `c=='.'`: `acc_num=0; int_flag=0;` if digit: `int_flag=1;` then loop:
  `buf[i++]=c; EDI++(count); c=emit_step(ctx); if(c==-1) goto 0x0442c9;` while
  `kCharClass[(c+1)&0xff]&0x20` (next is digit) → loop. (NOTE: the original does
  NOT accumulate into acc_num here — acc_num stays 0; see P5.)
- **P4 fractional digits (0x04419c-0x0441fa):** if `c=='.'`: `buf[i++]=c; c=emit_step(ctx);
  if(c==-1) goto 0x0442c9;` if `int_flag==0 && !digit(c) goto 0x0442c2;` loop:
  `EDI++; if(digit(c)){ EDI++; buf[i++]=c; c=emit_step(ctx); if(c==-1) goto 0x0442c9; }`
- **P5 fractional recompute (0x0441fc-0x044234):** if `flags&0x10`: scan `buf`
  backwards from `i-1`; for each char: if '.' break; else `acc2 = acc2/10` (DIV by
  `[ESP+0x60]=10`, EDX:acc2; **the digit is discarded**); `acc_num=acc2`.
  **LATENT BUG:** acc2 starts 0 and the digit is never added → acc_num is always 0
  on this path. Replicate faithfully.
- **P6 exponent (0x044239-0x0442c0):** if `c=='e'||c=='E'` (and `!(flags&0x10)`):
  `buf[i++]=c; c=emit_step; if(c==-1) goto 0x0442c9;` if `c=='+'||c=='-'`:
  `buf[i++]=c; c=emit_step; if(c==-1) goto 0x0442c9;` loop: `if(digit(c)){ EDI++;
  buf[i++]=c; c=emit_step; if(c==-1) goto 0x0442c9; } else { EDI=0; goto 0x0442c2; }`
- **P7 consume last char (0x0442c2):** `ctx->emit(c, ctx)` (putback/consume the
  last-read char; EAX=c, EDX=ctx, CALL [ctx+4]).
- **P8 finalize (0x0442c9-0x044382):** `if(EDI<=0) goto return; EDI+=ws_count;`
  `if(!(flags&0x1)) goto return; buf[i]=0;` if `flags&0x10`: `if(buf[0]=='-')
  acc_num = -acc_num;` else `g_fp_0442fa(buf, &r64)`. Then the list-pool append
  (see above). Return EDI.

## Local frame (ESP after `SUB ESP,0x70`)
- ESI = ESP (char buffer base); buffer NUL-terminated at end.
- `[ESP+0x50]:[ESP+0x54]` = result64 (lo,hi) — OUT of the converter.
- `[ESP+0x58]` = arg2 (pool_cursor).
- `[ESP+0x60]` = 0xa (DIV base). `[ESP+0x64]` = acc2 (P5). `[ESP+0x68]` = acc_num.
- `[ESP+0x6c]` = ws_count.
- EDI = char count (return). ESI = buffer write ptr. EBX = current char. EBP =
  first non-ws char / P1 counter.

## digit() helper
`digit(c) := kCharClass[(c+1)&0xff] & 0x20`  (kCharClass PORTED at f.c:2393).

## Callees (ALL ported)
- `ctx->next` / `ctx->emit` (vtable) — caller-provided callbacks.
- `emit_step` (0x0445a6, f.c:2527) — guarded next with EOF/peek.
- `kCharClass[256]` (0x5185c, f.c:2393).
- `g_fp_0442fa` (0x53648) — global fn-ptr, default stub (see above); real target
  unported (game sets it at runtime).
- `fx64_to_f32` (0x049132, f.c) — double→float (PORTED, 6 diff tests).
- Tail `JMP 0x043e2e` (shared epilogue = `return EDI`).

## Test plan (test_fn_0440a9)
- Build an `emit_ctx` with a `next` callback reading from a C string buffer (returns
  0xffffffff at EOF) and an `emit` callback that is a no-op (or putback).
- Set `flags`, `count` appropriately.
- Set up the pool: an array of `uintptr_t` entries, each a pointer to a `uint32_t`
  value slot; `pool_cursor` = &array_base.
- Set `g_fp_0442fa` to a test converter (string→r64; e.g. parse the integer into
  r64) so the default-path and float-path can be exercised.
- Cases: (a) integer "123" flags&0x10 → acc_num 123 stored, count 3; (b) integer
  "42" default flags → fx64_to_f32(r64) stored; (c) negative "-7" flags&0x10 →
  acc_num -7; (d) "no-output" flags (0x1 clear) → only count returned, no append;
  (e) EOF mid-token → near-end return.
- Verify: return count, pool_cursor advanced by entry_size, value slot written.

## Notes for the porter
- This is a faithful replica of the (possibly buggy) original — do NOT "fix" P5's
  always-0 acc_num or the missing integer accumulation in P3; replicate exactly.
- The `IMUL EAX,[ESP+0x6a],0xa` at 0x04416a writes `[ESP+0x6a]` but its result is
  only used to adjust EBX when `flags&0x10` — replicate the exact sequence.
- Model 0x53648 as an overridable global so the test can drive the converter path.
