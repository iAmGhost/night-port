# 0x0428f8 deferral note (2026-07-13)

## What it is (disasm verified, 18 insns)
`0x0428f8` is a **streaming-printf wrapper** (a `vsprintf`-like fn). Prologue
= `PUSH EBX/ECX/EDX; SUB ESP,0x4` (16 bytes), so `[ESP+0x14]=arg1`,
`[ESP+0x18]=arg2`, `[ESP+0x1c]=arg3`.

    000428fe  LEA EAX,[ESP+0x1c]      ; EAX = &arg3
    00042902  MOV EDX,[ESP+0x18]      ; EDX = arg2 (fmt)
    00042906  MOV EBX,ESP             ; EBX = ESP  (src slot)
    00042908  MOV ECX,0x428e5         ; ECX = callback = 0x428e5 (= fmt_emit, [x])
    0004290d  MOV [ESP],EAX           ; *src = &arg3  (first data item read = arg3's value)
    00042910  MOV EAX,[ESP+0x14]      ; EAX = arg1 (buf)
    00042914  CALL 0x044c23           ; fn_044c23(buf=arg1, fmt=arg2, src=(u8**)ESP, cb=0x428e5)
    00042919  MOV EDX,[ESP+0x14]      ; EDX = arg1 (buf)
    0004291d  MOV [EDX+EAX],0         ; buf[ret] = 0  (NUL-terminate; EAX=ret=st->count)
    00042921..27 ADD/POP/POP/POP/RET   ; returns EAX = the emitted count

So: **`uint32_t fn_0428f8(uint8_t *buf, const uint8_t *fmt, uint32_t arg3)`**
= `n = fn_044c23(src->&arg3, fmt_emit, buf, fmt); buf[n]=0; return n;`
where `*src` starts at `&arg3` (the first 32-bit data value the format consumes is
`arg3` itself — e.g. a string ptr for `%s`, or the value for `%c`/`%u`).

## The callback 0x0428e5 = fmt_emit (ALREADY [x])
    0x0428e5: BL=ch; p=*st; *st=p+1; *p=ch; st[0x10]++   (11 insns)
Ported as `void fmt_emit(fmt_state *st, uint8_t byte)` (f.c:1909) using the
`fmt_state` struct (f.h:253). test_fmt_emit uses a **stack** buffer as `pos` and
passes, so `pos` is a full **64-bit** pointer.

## WHY DEFERRED — descriptor layout inconsistency (the blocker)
`fmt_state` (C natural alignment, VERIFIED: sizeof=24) is:
    pos   @ 0   (8 bytes)
    width @ 8
    slen  @ 12
    zeros @ 16
    count @ 20   (0x14)

But the descriptor as used by the printf callees (fn_044ea4, fn_0451bc, and the
freshly-ported fn_044c23 — all raw `uint8_t*`/offset style, mirroring the 32-bit
original) is:
    pos   @ 0   (4 bytes in the original)
    width @ 4
    slen  @ 8
    zeros @ 12
    count @ 16   (0x10)  <-- fmt_emit's `INC [EAX+0x10]`
    flags @ 20   (0x14)
    type  @ 21   (0x15)
    fill  @ 22   (0x16)
    inline@ 23+  (0x17)

So **fmt_emit's `count` (offset 20) collides with the printf's `flags` (offset 20)**;
every field after `pos` is shifted by +4 because C widens `pos` to 8 bytes. The two
sides of the port use INCOMPATIBLE descriptor layouts.

`0x0428f8` requires `fn_044c23` and `fmt_emit` to agree on ONE layout (fn_044c23 sets
`st->pos`/`st->count`; fmt_emit reads/writes them). They currently don't.

## Recommended resolution (fresh context)
Pick ONE canonical 64-bit descriptor layout and migrate the whole printf unit to it:
- **Option A (align to `fmt_state`):** shift fn_044ea4/fn_0451bc/fn_044c23 field
  offsets by +4 (pos 8B@0, width@8, slen@12, zeros@16, count@20, flags@24, type@25,
  fill@26, inline@27+). Matches the existing `fmt_state` struct + fmt_emit. Requires
  updating the 3 fns AND their tests' raw-offset checks. Higher regression surface.
- **Option B (raw layout, 32-bit pos):** keep pos a 32-bit value@0 (low-4GB offset);
  fix fmt_emit to raw offsets (pos@0, count@16) + change its test (can't use a high
  stack buffer as pos then). Less shift, but pos no longer a real 64-bit pointer.

Either way this is a project-wide decision affecting already-verified functions ->
do it deliberately, not as a drive-by. Then 0x0428f8 becomes a ~5-line wrapper
(fn_044c23 + fmt_emit + NUL-term) and 0x0446c3 (L6, 132B) + 0x042394 (L11, 33B) +
0x036537 (L11) become reachable.

## Test plan (once layout is settled)
`test_fn_0428f8` (value-based, no <4GB pointer deref):
  - plain "Hi" (arg3=x)        -> buf=="Hi",   ret 2, buf[2]==0
  - "A%c" arg3='Z'             -> buf=="AZ",   ret 2
  - "Q%u" arg3=100             -> buf=="Q100", ret 4
(Each checks the NUL terminator + return count + the full fn_044c23->fn_0451bc->
fmt_emit pipeline.)
