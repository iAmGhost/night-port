# L0 primitives (batches 1–2) — semantics

Ported: `port/src/f/f.c` · Verified: `port/test/test_f1.c` (differential, all PASS)

Argument letters follow original PUSH order: a=[EBP+8], b=[EBP+C], …
"Word fill" = STOSW: each 16-bit word gets the byte replicated (`f | f<<8`),
**not** a 32-bit replicate.

## 0x040024 `put_px(a, b, c, d)`
`a[c * DAT_00040010 + b] = d`
a = frame base, b = column, c = row, d = pixel. DAT_00040010 = frame pitch
(320 for the 320×200 mode; global, set at video init — pending video port).
Note: the 0x53394/0x5333c canary globals and this stride live in the same BSS region; do not confuse them.

## 0x040475 `cpy_back(a, b, n)`
`do { *a = *b; b++; a--; n--; } while (n)`
a = dst, b = src. Traverses dst **downward**, src upward. Net effect:
`dst[-i] = src[i]` (src[0] lands at dst itself, src[n-1] at dst-n+1).

## 0x0404c6 `cpy_dw(a, n, c)`
`for (; n; n--) { *c = *a; a++; c++; }` — REP MOVSD.
a = src, n = dword count, c = dst.

## 0x040491 `blt_rows(a, s, w, h, c, t)`
h rows of w bytes each: one byte if w odd, then w>>1 words (REP MOVSW);
per row src += s-w, dst += t-w.
a = src, s = src pitch, w = width, h = rows, c = dst, t = dst pitch.

## 0x0404db `hline_set(a, b, c, d, e, f)`
p = a + d*b + c; fill e bytes (odd byte, then e>>1 words of f|f<<8).
a = base, b = row, c = column, d = row length, e = line length, f = pixel.
(Asm: EDI = [EBP+8] + [EBP+14]*[EBP+C] + [EBP+10]; len = [EBP+18];
 fill byte = [EBP+1C]; AH=AL before STOSW.REP → 16-bit replicated word.)

## 0x040506 `vline_set(a, b, c, d, e, f)`
p = a + d*b + c; `do { *p = f; p += b; e--; } while (e)`
a = base, b = step (pitch), c = column, d = row, e = length, f = pixel.

## 0x04052c `rect_fill(a, b, c, d, e, f)`
p = a + c*stride + b; e rows of d bytes (odd byte + words of f|f<<8);
p += stride - d per row. stride = DAT_00040010 (global).
a = base, b = column, c = row, d = width, e = height, f = pixel.

## 0x040042 `board_out(a, b, c)`
`outb(0x3C8, a); REP OUTSB: 3*b bytes of c -> port 0x3C9`
DIP/input board. a = control byte, b = n, c = buffer.
Called from key handlers (0x35f1b, 0x36041, …) — semantics pinned when
those land. Port routes I/O through `sys_outb` (virtual port map).

## 0x040061 `board_in(a, b, c)`
`outb(0x3C7, a); REP INSB: 3*b bytes from port 0x3C9 -> c`
Companion of board_out (read direction).

## 0x041f76 `abs32(a)`
`if (a < 0) a = -a; return a;`

## 0x0418e6 `canary_ss_save()`
`DAT_0005333c = SS` — snapshot of the SS register at startup.
Called via a function-pointer table (entry at DAT_000537de, dispatched from
0x44618) — an init hook, not a direct call from game code.

## 0x041901 `canary_check(frame_size)` — STACK-DEPTH GUARD
**Not a pointer canary.** Every game function prologue is:
`PUSH <frame_size>; CALL 0x418ee` (261 call sites, L0–L23). The pushed
immediate (0x38–0x7c, the caller's declared frame) is moved into EAX by
`XCHG [ESP+4], EAX` in 0x418ee, so this function receives a **frame size**:
```
d = ESP - frame_size            ; projected stack bottom after the frame
if (frame_size >= ESP || d <= DAT_00053394) {
    if (SS == DAT_0005333c)  FATAL: print "Stack Overflow!\r\n"
                             (via 0x429ea; original then falls through
                              into FUN_0004192e = process death)
    return SS                   ; foreign SS (DPMI 16-bit callback stack):
                                ; report, do not kill
}
return d
```
DAT_00053394 (the bound) is set once at entry, 0x420bc → 0x4226c:
`MOV [0x53394], EDI` where EDI is the linear address just past the copied
DOS header (~0x640xx) — i.e. the stack must never grow below the data area.
**Port deviations (documented):** the original compares the real 32-bit ESP;
the port uses the address of a local in this function with native-width
arithmetic. The fatal fall-through (undefined) is `sys_fatal` = print + abort
(a test hook, `sys_set_fatal_handler`, can intercept it).

## 0x0418ee `canary_id(eax, frame_size)`
`XCHG [ESP+4], EAX; CALL 0x41901; MOV EAX, [ESP+4]; RET 4`
= `canary_check(frame_size); return eax;` — runs the guard, preserves the
caller's EAX. The prologue pattern `PUSH imm; CALL 0x418ee` delivers the
frame size in EAX via the XCHG; RET 4 pops the pushed immediate.

## 0x041df2 `seg_save(uint16_t *a)`
`a[0]=ES; a[1]=CS; a[2]=SS; a[3]=DS; a[4]=FS; a[5]=GS` (arg in EAX).
DPMI real-mode transition: a caller (e.g. 0x103e0) saves the six segment
registers into a local before a DPMI service call (0x41e38(0x31, …)) so it
can restore them after the real-mode hop. **Port:** the modern flat model has
no meaningful per-register values, so this reads a fixed table from
`sys_get_segments` (ES=0, CS=0x23, SS=0x2b, DS=0, FS=0, GS=0). The DPMI
real-mode hop itself is replaced by the sys layer, so the saved values are
vestigial in the port.

## 0x041e6c `mem_copy(a, b, c)` → returns `a`
`REP MOVSD (c>>2)` then `REP MOVSB (c&3)` — a plain memcpy of `c` bytes from
`b` to `a`, returning the **original** dst (PUSH/POP'd around the copy).
Args are register-passed (EAX=dst, EDX=src, EBX=n). Byte-identical to
`memcpy`; kept as an explicit function for 1:1 fidelity. (The original
`PUSH ES; MOV EAX,DS; MOV EAX,ES` shuffles are a cross-segment guard in the
large memory model — no-op in the flat port.)

## 0x041e99 `rng_state_ptr()` → returns `&g_rng_state`
Returns a pointer to the 32-bit PRNG state word (DAT_0005335c). The state is
advanced by 0x41e9f (`state = state*0x41c64e6d + 0x3039`; returns
`state>>16 & 0x7fff`, 15 bits) and re-seeded by 0x41ec3. Mirrored as
`g_rng_state`.

## 0x041ef8 `itoa32(a, b, c)` → returns `b`
Unsigned integer-to-string: writes the base-`c` (2..36) representation of
`a` into `b`, NUL-terminated, returns the original `b`. Digit table is
`0-9a-z` (lowercase). Args register-passed (EAX=value, EDX=out, EBX=base).
Original idiom: a local digit buffer with a NUL sentinel at [0], digits
appended at [1..k], then copied into `b` in reverse (the sentinel becomes the
terminator). Ported as a clean do/while + reverse copy (same observable
output). Note: `a` is unsigned — negative values are not handled (the game
only formats non-negative integers through this path).

## 0x041f5d `str_len(a)`
`strlen` with a 2³² underflow guard: counts from 0xffffffff down, reads one
byte per step, stops at NUL (or after 2³² steps if unterminated). Returns
`~counter - 1` == the exact length for any valid (NUL-terminated) string.

## 0x041f7e `fpu_rndint(double a)`
x87 `FRNDINT` on the top of the FPU stack: save the control word, set
RM=round-to-nearest-even (CW=0x1f), `FRNDINT`, restore the CW. Net effect:
round the FPU-top value to an integer, half-to-even. The decompiler shows it
as a no-op because it can't model the x87 stack — it is **not** a no-op. It is
always called between `FLD` and `FISTP` (128 sites), i.e.
`int v = (int)round_nearest_even(float_val)`. **Port:** the x87 stack has no
C analogue; callers pass the value as an argument and `fpu_rndint` returns
`rint(a)` (matches x87 round-to-nearest-even under the default FE_TONEAREST
rounding mode, which is the mode the game always runs in).

## Global: DAT_0005335c
32-bit PRNG state word, read/written via `rng_state_ptr()`. Mirrored as
`g_rng_state`. LCG multiplier 0x41c64e6d, increment 0x3039, output 15 bits.

## Global: DAT_00040010
Frame pitch, int32 at 0x40010. Written by video init (not yet ported);
read by put_px / rect_fill. Mirrored as `g_video_stride`.
