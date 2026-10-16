/* f.c — ported original functions (batch 1: L0 primitives).
 * Bodies are 1:1 translations of the decompilation in analysis/pseudo/,
 * cross-checked against the disassembly (scratch/batch1_asm*.txt).
 * Semantics: docs/12-functions/primitives.md */
#include "f.h"
#include "../sys/sys.h"
#include <math.h>
#include <string.h>

/* ---- globals (mirrored BSS) ---- */
int32_t  g_video_stride = 0;
uint16_t g_ss_saved = 0;
uintptr_t  g_stack_bound = 0;
uint32_t g_rng_state = 0;      /* DAT_0005335c: PRNG state (LCG in 0x41e9f) */

/* put_px  orig: 0x040024
 *   base[c * DAT_00040010 + b] = d
 * a = frame base, b = column, c = row, d = pixel
 */
void put_px(uint8_t *a, int b, int c, uint8_t d) {
    a[c * g_video_stride + b] = d;
}

/* cpy_back  orig: 0x040475
 *   do { *a = *b; b++; a--; n--; } while (n)
 * a = dst (first write lands AT a), b = src, n = count
 */
void cpy_back(uint8_t *a, const uint8_t *b, int n) {
    do {
        *a = *b;
        b = b + 1;
        a = a - 1;
        n = n - 1;
    } while (n != 0);
}

/* cpy_dw  orig: 0x0404c6
 *   for (; n; n--) { *c = *a; a++; c++; }   (REP MOVSD)
 * a = src, n = dword count, c = dst
 */
void cpy_dw(uint32_t *a, int n, uint32_t *c) {
    for (; n != 0; n = n - 1) {
        *c = *a;
        a = a + 1;
        c = c + 1;
    }
}

/* blt_rows  orig: 0x040491
 *   h rows of w bytes: odd byte, then w>>1 words (REP MOVSW);
 *   src += s - w, dst += t - w per row
 * a = src, s = src pitch, w = width, h = rows, c = dst, t = dst pitch
 */
void blt_rows(uint8_t *a, int s, uint32_t w, int h, uint8_t *c, int t) {
    do {
        uint32_t uw = w >> 1;
        if ((w & 1) != 0) {
            *c = *a;
            a = a + 1;
            c = c + 1;
        }
        for (; uw != 0; uw = uw - 1) {
            *(uint16_t *)(uintptr_t)c = *(const uint16_t *)(uintptr_t)a;
            a = a + 2;
            c = c + 2;
        }
        a = a + (s - (int)w);
        c = c + (t - (int)w);
        h = h - 1;
    } while (h != 0);
}

/* hline_set  orig: 0x0404db
 *   p = a + d*b + c; fill e bytes: odd byte, then e>>1 words (each f|f<<8)
 * a = base, b = row, c = column, d = row length, e = line length, f = pixel
 * (asm: EAX=[EBP+14]=[rowlen]*[EBP+C]=[row], EDI=[EBP+8]+EAX+[EBP+10];
 *  ECX=[EBP+18]=len, fill byte=[EBP+1C])
 */
void hline_set(uint8_t *a, int b, int c, int d, uint32_t e, uint8_t f) {
    uint8_t *p = a + d * b + c;
    uint32_t uw = e >> 1;
    if ((e & 1) != 0) {
        *p = f;
        p = p + 1;
    }
    for (; uw != 0; uw = uw - 1) {
        *(uint16_t *)(uintptr_t)p = (uint16_t)(f | (f << 8));
        p = p + 2;
    }
}

/* vline_set  orig: 0x040506
 *   p = a + d*b + c; do { *p = f; p += b; e--; } while (e)
 * a = base, b = step (pitch), c = column, d = row, e = length, f = pixel
 */
void vline_set(uint8_t *a, int b, int c, int d, int e, uint8_t f) {
    uint8_t *p = a + d * b + c;
    do {
        *p = f;
        p = p + b;
        e = e - 1;
    } while (e != 0);
}

/* rect_fill  orig: 0x04052c
 *   p = a + b + c*stride; e rows of d bytes (odd byte, then d>>1 words
 *   of f|f<<8); p += stride - d per row
 * a = base, b = column, c = row, d = width, e = height, f = pixel
 */
void rect_fill(uint8_t *a, int b, int c, uint32_t d, int e, uint8_t f) {
    uint8_t *p = a + c * g_video_stride + b;
    int adv = g_video_stride - (int)d;
    do {
        uint32_t uw = d >> 1;
        if ((d & 1) != 0) {
            *p = f;
            p = p + 1;
        }
        for (; uw != 0; uw = uw - 1) {
            *(uint16_t *)(uintptr_t)p = (uint16_t)(f | (f << 8));
            p = p + 2;
        }
        p = p + adv;
        e = e - 1;
    } while (e != 0);
}

/* board_out  orig: 0x040042
 *   outb(0x3C8, a); REP OUTSB: 3*b bytes of c -> port 0x3C9
 * a = control byte, b = n, c = buffer
 */
void board_out(uint8_t a, int b, const uint8_t *c) {
    sys_outb(0x3c8, a);
    int n = b * 3;
    while (n != 0) {
        n = n - 1;
        sys_outb(0x3c9, *c);
        c = c + 1;
    }
}

/* board_in  orig: 0x040061
 *   outb(0x3C7, a); REP INSB: 3*b bytes from port 0x3C9 -> c
 * a = control byte, b = n, c = buffer
 */
void board_in(uint8_t a, int b, uint8_t *c) {
    sys_outb(0x3c7, a);
    int n = b * 3;
    while (n != 0) {
        n = n - 1;
        *c = sys_inb(0x3c9);
        c = c + 1;
    }
}

/* abs32  orig: 0x041f76 */
int32_t abs32(int32_t a) {
    /* x86 NEG wraps two's-complement: NEG(INT_MIN) == INT_MIN.
     * C negation of INT_MIN is UB, so emulate the CPU exactly. */
    if (a < 0) a = (int32_t)(-(int64_t)a);
    return a;
}

/* canary_ss_save  orig: 0x0418e6
 *   DAT_0005333c = SS
 */
void canary_ss_save(void) {
    g_ss_saved = sys_ss();
}

/* canary_check  orig: 0x041901 — STACK-DEPTH GUARD (see primitives.md)
 * Every game function prologue is:  PUSH <frame_size>; CALL 0x418ee
 * The pushed immediate becomes EAX via XCHG in 0x418ee, so the parameter
 * here is the caller's declared frame size, NOT a stack pointer.
 *
 *   d = ESP - frame_size                (projected stack bottom)
 *   if (frame_size >= ESP || d <= DAT_00053394) {
 *       if (SS == DAT_0005333c)  FATAL "Stack Overflow!\r\n"
 *                                (print via 0x429ea; original then falls
 *                                 through into FUN_0004192e = death;
 *                                 port: sys_fatal)
 *       return SS                       (foreign SS: DPMI 16-bit callback
 *                                        stack — report, do not kill)
 *   }
 *   return d
 *
 * Port notes: the original compares the real 32-bit ESP; the port uses the
 * address of a local in this function with native-width arithmetic.
 * g_stack_bound mirrors DAT_00053394, set at entry (0x420bc: the value is
 * the linear address where the copied DOS header ended, ~0x640xx — i.e.
 * the stack must never grow below the data area).
 */
uintptr_t canary_check(uint32_t frame_size) {
    volatile int marker;
    uintptr_t esp = (uintptr_t)&marker;
    uintptr_t d;
    if (esp > (uintptr_t)frame_size)
        d = esp - (uintptr_t)frame_size;
    else
        d = 0;
    if (frame_size < esp && d > (uintptr_t)g_stack_bound)
        return d;
    if (sys_ss() != g_ss_saved)
        return (uintptr_t)sys_ss();
    sys_fatal("Stack Overflow!\r\n");
    return 0;
}

/* canary_id  orig: 0x418ee
 *   XCHG [ESP+4], EAX; CALL 0x41901; MOV EAX, [ESP+4]; RET 4
 *   = canary_check(frame_size); return the caller's EAX unchanged.
 * The prologue pattern PUSH imm; CALL 0x418ee moves imm into EAX via the
 * XCHG; the old EAX (caller's register value) is restored before RET 4.
 */
uintptr_t canary_id(uintptr_t eax, uint32_t frame_size) {
    canary_check(frame_size);
    return eax;
}

/* seg_save  orig: 0x041df2
 *   out[0]=ES; out[1]=CS; out[2]=SS; out[3]=DS; out[4]=FS; out[5]=GS
 * (arg in EAX). DPMI real-mode transition: caller saves segments before a
 * DPMI call so it can restore them after (see 0x103e0 -> 0x41e38(0x31,...)).
 * Port: the flat model has fixed segment values (sys_get_segments).
 */
void seg_save(uint16_t *a) {
    sys_get_segments(a);
}

/* mem_copy  orig: 0x041e6c
 *   REP MOVSD (c>>2 dwords) then REP MOVSB (c&3 bytes); returns dst.
 * (args in registers: EAX=dst, EDX=src, EBX=n.) Byte-identical to memcpy;
 * kept explicit for 1:1 fidelity. dst is PUSHed/POP'd so the return value
 * is the ORIGINAL dst, not the advanced pointer.
 */
uint8_t *mem_copy(uint8_t *a, const uint8_t *b, uint32_t c) {
    uint8_t *dst0 = a;
    uint32_t n = c >> 2;
    uint32_t *d = (uint32_t *)(uintptr_t)a;
    const uint32_t *s = (const uint32_t *)(uintptr_t)b;
    while (n != 0) {
        n = n - 1;
        *d = *s;
        d = d + 1;
        s = s + 1;
    }
    a = (uint8_t *)(uintptr_t)d;
    b = (const uint8_t *)(uintptr_t)s;
    while ((c & 3) != 0) {
        c = c - 1;
        *a = *b;
        a = a + 1;
        b = b + 1;
    }
    return dst0;
}

/* rng_state_ptr  orig: 0x041e99
 *   return &DAT_0005335c   (the 32-bit PRNG state word)
 */
uint32_t *rng_state_ptr(void) {
    return &g_rng_state;
}

/* itoa32  orig: 0x041ef8
 *   Unsigned itoa: writes the base-c representation of a into b, reversed
 *   via a local digit buffer, NUL-terminated; returns the original b.
 * (args in registers: EAX=value, EDX=out, EBX=base; base 2..36.)
 * Original idiom: buffer[0]=0 sentinel, digits at buffer[1..k], then copy
 * buffer[k..0] into out (the sentinel 0 becomes the NUL terminator).
 */
char *itoa32(uint32_t a, char *b, uint32_t c) {
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buf[33];
    int n = 0;
    do {
        uint32_t q = a / c;
        buf[n] = digits[a % c];
        n = n + 1;
        a = q;
    } while (a != 0);
    char *o = b;
    for (int i = n - 1; i >= 0; i = i - 1)
        *b++ = buf[i];
    *b = '\0';
    return o;
}

/* str_len  orig: 0x041f5d
 *   strlen with a 2^32 underflow guard: counts from 0xffffffff down, reads
 *   a byte per step, stops at NUL (or after 2^32 steps if unterminated).
 *   Returns ~counter - 1 == exact length for valid strings.
 */
uint32_t str_len(const char *a) {
    uint32_t c = 0xffffffffu;
    char ch;
    do {
        if (c == 0) break;
        c = c - 1;
        ch = *a;
        a = a + 1;
    } while (ch != '\0');
    return ~c - 1;
}

/* fpu_rndint  orig: 0x041f7e
 *   x87 FRNDINT on the top of the FPU stack: save CW, set RM=round-to-
 *   nearest-even (CW=0x1f), FRNDINT, restore CW. So it rounds the FPU-top
 *   value to an integer, half-to-even. Always called between FLD and FISTP
 *   (128 call sites), i.e. `int v = (int)round_nearest_even(float_val)`.
 * Port: the x87 stack has no C analogue; callers pass the value as an arg.
 * rint() matches x87 round-to-nearest-even under the default FE_TONEAREST.
 */
double fpu_rndint(double a) {
    return rint(a);
}

/* seg_ds_load  orig: 0x042310
 *   MOV DS, word ptr CS:[0x42319]; RET
 * Loads the real-mode DS from a linker-provided global (DAT_00042319) before
 * a DPMI/DOS service call so the real-mode code uses the right segment. In
 * the flat long-mode model DS is a constant (effectively 0) and is ignored,
 * so this is a documented no-op.
 */
void seg_ds_load(void) { /* no-op: flat model, DS is constant */ }

/* irq_cli  orig: 0x04231b
 *   CLI; RET
 * Disables maskable interrupts. Used around raw port I/O (PIT/PIC) to keep
 * the sequence atomic. The flat model has no interrupt flag, so no-op.
 */
void irq_cli(void) { /* no-op: no interrupt flag in the flat model */ }

/* outb  orig: 0x04231d
 *   MOV EDX, EAX; MOV AL, CL; OUT DX, AL
 * Single-byte port write: OUT `port` (EAX/DX), `val` (EDX/AL & 0xff).
 * Register-passed (EAX=port, EDX=val). The original PUSH/POP's EBX/ECX
 * around the OUT to preserve them. Routed to the sys virtual port map.
 */
void outb(uint16_t port, uint8_t val) {
    sys_outb(port, val);
}

/* irq_sti  orig: 0x04232b
 *   STI; RET
 * Re-enables maskable interrupts (pairs with irq_cli). No-op in flat model.
 */
void irq_sti(void) { /* no-op: no interrupt flag in the flat model */ }

/* isr_get  orig: 0x04232d
 *   INT 0x21h get-interrupt-vector: returns the current handler for `vector`
 *   (EAX) as a far pointer — EAX=offset, EDX=segment. Callers (e.g.
 *   0x0003ef29) save this to globals so they can restore the original
 *   handler later. The flat model has no real OS vectors, so it returns 0
 *   and the later restore is likewise a no-op.
 */
uint64_t isr_get(uint8_t vector) {
    return sys_isr_get(vector);
}

/* isr_set  orig: 0x042361
 *   INT 0x21h set-interrupt-vector: installs handler_seg:handler_off as the
 *   handler for `vector`. Used to hook INT 0x1C (the 18.2 Hz tick) to the
 *   game's timing ISR. The original passes the vector in two registers
 *   (EAX and EDX) for the DPMI real-mode frame; collapsed to one arg here.
 *   No-op now — the timer subsystem (0x0003ee88/0x0003ef29) will register a
 *   real tick driver through sys_isr_set.
 */
void isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg) {
    sys_isr_set(vector, handler_off, handler_seg);
}

/* inb  orig: 0x04238c
 *   MOV EDX, EAX; SUB EAX, EAX; IN AL, DX
 * Single-byte port read: AL = IN(port) with port in EAX/DX. The original
 * PUSH/POP's EDX to preserve it. Routed to the sys virtual port map
 * (companion to outb).
 */
uint8_t inb(uint16_t port) {
    return sys_inb(port);
}

/* cb_noop  orig: 0x0423b6
 *   RET
 * A single-byte RET: the default (no-op) handler for the 3-slot callback
 * table at DAT_0005336c/70/74. The dispatchers 0x000423b7/0x000423cf invoke
 * each slot; at boot all three point here, and the game installs real
 * handlers later. Does nothing regardless of the args the dispatchers pass.
 */
void cb_noop(void) { /* RET only — does nothing */ }

/* str_icmp  orig: 0x0423e5
 * Bounded case-insensitive compare of up to `n` bytes. Each byte is lowered
 * if it is 'A'..'Z' (0x41..0x5a), then compared. Returns 0 if all `n` bytes
 * match (or n==0), else the signed difference of the first differing pair.
 * Register-passed (EAX=a, EDX=b, EBX=n).
 */
int str_icmp(const char *a, const char *b, int n) {
    for (; n != 0; n--, a++, b++) {
        int ca = (uint8_t)*a, cb = (uint8_t)*b;
        if (ca >= 0x41 && ca <= 0x5a) ca += 0x20;
        if (cb >= 0x41 && cb <= 0x5a) cb += 0x20;
        if (ca != cb) return ca - cb;
    }
    return 0;
}

/* res_consume  orig: 0x042438
 * Consume `amount` bytes from a chunked resource reader. Clears flag bit
 * 0x10 on entry. Success requires (end - pos) <= amount <= remaining:
 * pos += amount, remaining -= amount, return 0. Otherwise the reader is
 * exhausted (pos = end, remaining = 0) and 1 is returned.
 */
int res_consume(res_reader *r, int amount) {
    r->flags &= 0xef;                            /* clear bit 0x10 */
    if (amount <= (int)r->remaining &&
        amount >= (int)(r->end - r->pos)) {
        r->pos += amount;
        r->remaining -= (uint32_t)amount;
        return 0;
    }
    r->pos = r->end;
    r->remaining = 0;
    return 1;
}

/* digit_chr  orig: 0x0427e7
 *   ADD EAX, 0x30; if (EAX > 0x39) EAX += 0x27
 * Maps a digit value 0..35 to its ASCII char: 0-9 -> '0'-'9', 10-35 ->
 * 'a'-'z'. This is the arithmetic form of the itoa32 digit table.
 */
int digit_chr(int d) {
    int r = d + 0x30;
    if (0x39 < r) r += 0x27;
    return r;
}

/* fmt_emit  orig: 0x0428e5
 *   pos = st->pos; st->pos = pos + 1; *pos = byte; st->count++
 * The byte-sink callback used by the streaming printf (FUN_00044c23): for
 * each output byte it stores it at the current write position, advances the
 * position, and increments the byte count (the printf's return value).
 */
void fmt_emit(fmt_state *st, uint8_t byte) {
    uint8_t *p = st->pos;
    st->pos = p + 1;
    *p = byte;
    st->count++;
}

/* =================================================================== */
/* input + compare + no-op primitives (0x4295d, 0x429a8, 0x42a07)       */
/* =================================================================== */

uint32_t g_keybuf;   /* DAT_000533a4: pending-key buffer */

/* DOS INT 21h AH=08 with a 1-char cache: return the buffered key if present
 * (clearing it), else a (blocking in the original) read from the input
 * backend. */
uint8_t get_key(void) {
    uint32_t c = g_keybuf;
    g_keybuf = 0;
    if (c != 0) return (uint8_t)c;
    return sys_getkey();
}

/* Bounded memcmp returning -1 / 0 / +1 at the first differing byte. */
int mem_cmp(const void *a, const void *b, int n) {
    const uint8_t *pa = a, *pb = b;
    for (; n != 0; n--, pa++, pb++) {
        if (*pa != *pb) return *pa < *pb ? -1 : 1;
    }
    return 0;
}

/* 1-byte RET no-op default handler. */
void cb_noop2(void) { /* RET only */ }

/* =================================================================== */
/* batch 6: heap-grow stub, echo input, dword fill                       */
/* =================================================================== */

/* DOS INT 21h AH=01 (cooked/echo char input), sharing g_keybuf with get_key. */
uint8_t get_key_echo(void) {
    uint32_t c = g_keybuf;
    g_keybuf = 0;
    if (c != 0) return (uint8_t)c;
    return sys_getkey_echo();
}

/* Outer heap memory-acquire. Stub: always fails (returns 0, no growth). */
uint32_t heap_grow(uint32_t size) { (void)size; return 0; }

/* Fill `n` 32-bit words at `dst` with `value`; returns `dst + n`. The
 * original's 32-byte alignment / 8-dword unrolling is an optimization that
 * does not change the observable bytes. */
uint32_t *memsetw(uint32_t *dst, uint32_t value, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) dst[i] = value;
    return dst + n;
}

/* =================================================================== */
/* Heap (two-level DPMI allocator) — batches 7-8                        */
/* =================================================================== */
/*
 * See f.h for the representation. Every heap pointer is a uint32 offset from
 * g_heap_base. hw(o) = word at offset o. Block fields: size@0 (bit 0 =
 * allocated), prev@4, next@8, user data@4. Region fields: total@0, root@0x0c,
 * largest@0x10, high_water@0x14, alloc_count@0x18, free_count@0x1c, sentinel
 * at +0x20 (size/prev/next). The original is 32-bit; the port keeps the exact
 * byte layout and uses offsets as the faithful 32->64-bit pointer adaptation.
 */

#define NIGHT_HEAP_ARENA (1u << 20)   /* 1 MiB static arena */
static uint8_t s_heap_arena[NIGHT_HEAP_ARENA];

uintptr_t g_heap_base      = 0;
uint32_t  g_heap_head      = 0;   /* DAT_00053350 */
uint32_t  g_heap_cursor    = 0;   /* DAT_00053354 */
uint32_t  g_heap_highwater = 0;   /* DAT_00053358 */
uint32_t  g_heap_seg_limit = 0;   /* DAT_00053384 */
uint8_t   g_heap_cfg_b2    = 0;   /* DAT_000533b2 */
uint8_t   g_heap_cfg_b4    = 0;   /* DAT_000533b4 */
uint8_t   g_heap_flag      = 0;   /* DAT_00063fbd (wrappers zero; never read) */
uint32_t  g_ds_segment     = 0;   /* DPMI DS (flat model: 0) */
static uint32_t s_arena_used = 0; /* next free offset in the arena */
/* NOTE: the arena reserves offset 0 as a NULL guard. Region/block offsets
 * are 32-bit values stored in 4-byte fields (to keep the 12/44-byte layout),
 * and the region chain is NULL-terminated with 0. A region at offset 0 would
 * be indistinguishable from "empty", so no heap pointer may be 0. The DPMI
 * original never has this issue (its region pointers are real, non-null
 * addresses); reserving the first 64 bytes of the static arena gives the same
 * guarantee. */

static inline uint32_t *hw(uint32_t off) { return (uint32_t *)(g_heap_base + off); }
/* block fields */
#define B_SIZE(o) hw(o)[0]   /* +0x00 */
#define B_PREV(o) hw(o)[1]   /* +0x04 */
#define B_NEXT(o) hw(o)[2]   /* +0x08 */
/* region fields */
#define R_ROOT(r)    hw(r)[3]   /* +0x0c */
#define R_LARGEST(r) hw(r)[4]   /* +0x10 */
#define R_HWM(r)     hw(r)[5]   /* +0x14 */
#define R_ALLOC(r)   hw(r)[6]   /* +0x18 */
#define R_FREE(r)    hw(r)[7]   /* +0x1c */
#define R_SENT(r)    ((r) + 0x20)
#define R_SENT_PREV(r) hw(r)[9]   /* +0x24 */
#define R_SENT_NEXT(r) hw(r)[10]  /* +0x28 */

void heap_arena_init(void) {
    g_heap_base      = (uintptr_t)s_heap_arena;
    g_heap_head      = 0;
    g_heap_cursor    = 0;
    g_heap_highwater = 0;
    g_heap_seg_limit = 0;
    g_heap_cfg_b2    = 0;
    g_heap_cfg_b4    = 0;
    g_heap_flag      = 0;
    g_ds_segment     = 0;
    s_arena_used     = 0x40;  /* reserve offset 0 (NULL guard) */
}

/* Place a `size`-byte region (64-byte aligned) in the arena; 0 if no room.
 * The slot is zeroed to replicate DPMI's zeroed fresh pages (the original's
 * insert_init leaves high_water unset, relying on that zeroing). */
uint32_t heap_arena_alloc(uint32_t size) {
    uint32_t a   = (size + 0x3f) & ~0x3fu;
    uint32_t off = s_arena_used;
    if (off + a > NIGHT_HEAP_ARENA) return 0;
    s_arena_used = off + a;
    memset(s_heap_arena + off, 0, a);
    return off;
}

/* Unlink a region from the all-blocks chain. Disasm (0x042c8d): advance
 * cursor and head if they are the node, then prev->next = next and
 * next->prev = prev, each NULL-guarded (full double unlink). */
void heap_unlink(uint32_t node) {                                  /* orig: 0x042c8d */
    if (g_heap_cursor == node) g_heap_cursor = B_NEXT(node);
    if (g_heap_head   == node) g_heap_head   = B_NEXT(node);
    uint32_t prev = B_PREV(node);                                  /* 0x042cb5 */
    uint32_t next = B_NEXT(node);                                  /* 0x042cb8 */
    if (prev) B_NEXT(prev) = next;                                 /* 0x042cbd/cbf */
    if (next) B_PREV(next) = prev;                                 /* 0x042cc2/cc6 */
}

/* Best-fit allocation within one region.  orig: 0x042ada */
uint32_t heap_alloc(uint32_t region, uint32_t size) {
    uint32_t eax, esi, edi, edx, ecx;
    if (size == 0) return 0;                                   /* 0x42ae0/42ae2 */
    if ((uint64_t)size + 7 > 0xFFFFFFFFu) return 0;            /* 0x42ae4 ADD,7; 0x42ae7 JC */
    { uint32_t t = size + 7;                                    /* 0x42ae4 */
      eax = (t & 0xFFFFFF00u) | ((t & 0xFFu) & 0xFCu); }        /* 0x42ae9 AND AL,0xfc (LOW byte only) */
    if (eax < 0xc) eax = 0xc;                                  /* 0x42aeb-42af7 max(eax,0xc) */
    if (eax > R_HWM(region)) return 0;                         /* 0x42af9 CMP; 0x42afc JA */
    esi = R_ROOT(region);                                      /* 0x42afe */
    edi = R_LARGEST(region);                                   /* 0x42b01 */
    if (eax <= edi) { esi = R_SENT_NEXT(region); edi = 0; }    /* 0x42b04/06,08,0b */
    for (;;) {
        edx = B_SIZE(esi);                                     /* 0x42b0d */
        if (eax <= edx) {                                      /* 0x42b0f CMP; 0x42b11 JBE */
            uint32_t rem = edx - eax;                          /* 0x42b2d SUB EDX,EAX */
            R_LARGEST(region) = edi;                           /* 0x42b2f */
            R_ALLOC(region)++;                                 /* 0x42b32 */
            ecx = B_NEXT(esi);                                 /* 0x42b35 next */
            if (rem < 0xc) {                                   /* 0x42b38 CMP; 0x42b3b JC (no-split) */
                R_FREE(region)--;                              /* 0x42b5b */
                uint32_t prev = B_PREV(esi);                   /* 0x42b62 */
                B_NEXT(prev) = ecx;                            /* 0x42b65 prev.next = next */
                B_PREV(ecx) = prev;                            /* 0x42b68 next.prev = prev */
                R_ROOT(region) = prev;                         /* 0x42b6b root = prev */
            } else {                                           /* split (0x42b3d) */
                uint32_t rb   = esi + eax;                     /* 0x42b3f rem_block */
                uint32_t prev = B_PREV(esi);                   /* 0x42b48 */
                R_ROOT(region) = rb;                           /* 0x42b41 root = rb */
                B_SIZE(rb)   = rem;                            /* 0x42b44 rb.size = remainder */
                B_SIZE(esi)  = eax;                            /* 0x42b46 block.size = aligned */
                B_PREV(rb)   = prev;                           /* 0x42b4b rb.prev = prev */
                B_NEXT(rb)   = ecx;                            /* 0x42b4e rb.next = next */
                B_NEXT(prev) = rb;                             /* 0x42b51 prev.next = rb */
                B_PREV(ecx)  = rb;                             /* 0x42b56 next.prev = rb */
            }
            B_SIZE(esi) |= 1;                                  /* 0x42b70 mark allocated */
            return esi + 4;                                    /* 0x42b73/42b75 */
        }
        if (edx > edi) edi = edx;                              /* 0x42b13-42b1a max(edi,edx) */
        esi = B_NEXT(esi);                                     /* 0x42b1f */
        if (esi == R_SENT(region)) {                           /* 0x42b22 CMP; 0x42b24 JNZ */
            R_HWM(region) = edi;                               /* 0x42b26 */
            return 0;                                          /* 0x42b29 */
        }
    }
}

/* Free a data offset within a region.  orig: 0x042b82 */
void heap_free(uint32_t region, uint32_t data) {
    if (data == 0) return;                              /* 0x42b88 OR; 0x42b8a JZ */
    uint32_t block = data - 4;                            /* 0x42b8c/42b8e */
    uint32_t sz    = B_SIZE(block);                      /* 0x42b91 */
    if ((sz & 1) == 0) return;                           /* 0x42b93 TEST AL,1; 0x42b95 JZ */
    sz &= 0xFFFFFFFEu;                                   /* 0x42b97 AND AL,0xfe */
    uint32_t next = block + sz;                          /* 0x42b99/42b9b */
    uint32_t insert_before;                              /* EDI at FINALIZE */

    if (B_SIZE(next) & 1) {                              /* 0x42b9d/42ba3 JNZ (next ALLOCATED) */
        B_SIZE(block) = sz;                              /* 0x42bc4 */
        if (block >= R_ROOT(region)) {                   /* 0x42bc9/42bcb JNC */
            uint32_t it = B_NEXT(R_ROOT(region));        /* 0x42bdb root.next */
            if (block < it) { insert_before = it; goto finalize; }         /* 0x42bde/0x42be0 */
            if (block > R_SENT_PREV(region)) { insert_before = R_SENT(region); goto finalize; } /* 0x42be2/04/08 */
        } else {                                          /* block < root (0x42bcd) */
            if (block > B_PREV(R_ROOT(region))) { insert_before = R_ROOT(region); goto finalize; } /* 0x42bcd/0x42bd0 */
            if (block < R_SENT_NEXT(region)) { insert_before = R_SENT_NEXT(region); goto finalize; } /* 0x42bd2/0x42bd7 */
        }
    } else {                                              /* next FREE -> forward coalesce */
        if (next == R_ROOT(region)) R_ROOT(region) = block; /* 0x42ba5/42baa */
        sz += B_SIZE(next);                               /* 0x42bad */
        B_SIZE(block) = sz;                               /* 0x42baf */
        uint32_t p = B_PREV(next);                        /* 0x42bb2 next.prev */
        uint32_t r = B_NEXT(next);                        /* 0x42bb5 next.next */
        B_NEXT(p) = r;                                    /* 0x42bb8 */
        B_PREV(r) = p;                                    /* 0x42bbb */
        R_FREE(region)--;                                 /* 0x42bbf */
        insert_before = r;                                /* EDI = next.next */
        goto finalize;                                    /* 0x42bc2 */
    }

    /* fragmentation "rebalance" walk.  orig: 0x042bea */
    {
        uint32_t alloc_c = R_ALLOC(region);               /* 0x42bec */
        uint32_t fc      = R_FREE(region);                /* 0x42bef */
        uint32_t q       = alloc_c / (fc + 1);            /* 0x42bf2/42bf3 DIV (fc+1>=1) */
        if (q >= fc) goto insert;                         /* 0x42bf6/42bf8 JNC */
        uint32_t diff = alloc_c - fc;                     /* 0x42bfa/42bfd */
        uint32_t cnt  = (diff > fc) ? (q * 2) : 0;        /* 0x42bff-42c05 */
        uint32_t cur  = block + B_SIZE(block);            /* 0x42c07/42c09 */
        while (cnt) {
            uint32_t csz = B_SIZE(cur);                   /* 0x42c0b */
            if ((csz & 1) == 0) { insert_before = cur; goto finalize; }  /* 0x42c0d/42c10 JZ */
            if (csz == 0xFFFFFFFFu) goto insert;          /* 0x42c12/42c15 JZ */
            cur += csz & 0xFFFFFFFEu;                     /* 0x42c17/42c1a */
            cnt--;                                        /* 0x42c1c */
        }
    }

insert:   /* address-sorted insert point.  orig: 0x042c1f */
    {
        uint32_t it = R_ROOT(region);                     /* 0x42c1f */
        if (block < it) it = R_SENT_NEXT(region);         /* 0x42c22/24/26 */
        for (;;) {
            if (block < it) break;                        /* 0x42c29/42c2b JC */
            it = B_NEXT(it);                              /* 0x42c2d */
        }
        insert_before = it;
    }

finalize: /*  orig: 0x042c40 */
    {
        uint32_t r    = insert_before;                    /* EDI at entry */
        uint32_t p    = B_PREV(r);                        /* 0x42c40 */
        uint32_t bs   = B_SIZE(block);                    /* 0x42c43 */
        uint32_t p_end = p + B_SIZE(p);                   /* 0x42c47/42c49 */
        if (p_end == block) {                             /* 0x42c4b/42c4d JNZ (back coalesce) */
            bs += B_SIZE(p);                              /* 0x42c4f */
            B_SIZE(p) = bs;                               /* 0x42c51 */
            if (block == R_ROOT(region)) R_ROOT(region) = p; /* 0x42c53/42c58 */
            block = p;                                    /* 0x42c5b ESI = P */
        } else {                                          /* normal insert */
            R_FREE(region)++;                             /* 0x42c5f */
            B_NEXT(block) = r;                            /* 0x42c62 */
            B_PREV(block) = p;                            /* 0x42c65 */
            B_NEXT(p) = block;                            /* 0x42c68 */
            B_PREV(r) = block;                            /* 0x42c6b/42c6d */
        }
        R_ALLOC(region)--;                                /* 0x42c70 */
        if (block < R_ROOT(region) && bs > R_LARGEST(region)) R_LARGEST(region) = bs; /* 0x42c73-7d */
        if (bs > R_HWM(region)) R_HWM(region) = bs;       /* 0x42c80/42c85 */
    }
}

/* Top check.  orig: 0x042ea1 */
uint32_t heap_top_check(void) {
    uint32_t region = g_heap_head;                        /* 0x42ea3 */
    if (region == 0) return 0;                            /* 0x42ea9/42eab */
    uint32_t last = R_SENT_PREV(region);                  /* 0x42ead [head+0x24] */
    uint32_t size = B_SIZE(last);                         /* 0x42eb0 */
    uint32_t end  = last + size + 4;                      /* 0x42eb2/42eba */
    if (g_heap_cfg_b2 >= 2 && g_heap_cfg_b2 <= 8 && g_heap_cfg_b4 == 0) { /* 0x42ebd-ec7 */
        g_heap_seg_limit = (g_ds_segment + 1) * 0x10000u; /* 0x42ed0-42ed8 */
    }
    if (end != g_heap_seg_limit) return 0;                /* 0x42edd/42ee3 */
    return size;                                          /* 0x42ee5 */
}

/* Insert+init a region into the all-blocks chain.  orig: 0x042e2d */
uint32_t heap_insert_init(uint32_t region) {
    uint32_t cur  = g_heap_head;                          /* 0x42e31 EAX */
    uint32_t prev = 0;                                    /* 0x42e36 EBX */
    while (cur) {                                         /* 0x42e43/42e45 */
        if (region < cur) break;                          /* 0x42e3a/42e3c JC */
        prev = cur;                                       /* 0x42e3e */
        cur  = B_NEXT(cur);                               /* 0x42e40 */
    }
    B_PREV(region) = prev;                                /* 0x42e47 */
    B_NEXT(region) = cur;                                 /* 0x42e4a */
    if (prev) B_NEXT(prev) = region; else g_heap_head = region; /* 0x42e4f/51/56 */
    if (cur)  B_PREV(cur)  = region;                      /* 0x42e5c/5e/60 */
    uint32_t sentinel = R_SENT(region);                   /* 0x42e63 LEA EBX,[EDX+0x20] */
    uint32_t total    = hw(region)[0];                    /* 0x42e66 preset total_size */
    R_LARGEST(region) = 0;                                /* 0x42e72 */
    R_ALLOC(region)   = 0;                                /* 0x42e79 */
    R_FREE(region)    = 0;                                /* 0x42e80 (free_count, NOT high_water) */
    hw(region)[8]     = 0;                                /* 0x42e6b sentinel.size */
    /* NOTE: the original does NOT initialize high_water (+0x14); a fresh DPMI
     * region is zeroed-page memory, so it starts at 0. The arena zeroes each
     * slot (heap_arena_alloc) to replicate that. The acquire path then sets
     * high_water = 0xffffffff explicitly. */
    R_SENT_PREV(region) = sentinel;                       /* 0x42e87 */
    R_SENT_NEXT(region) = sentinel;                       /* 0x42e8a */
    R_ROOT(region) = sentinel;                            /* 0x42e90 */
    uint32_t block0size = total - 0x2c;                   /* 0x42e8d */
    hw(region + 0x2c)[0] = block0size;                    /* 0x42e93 */
    hw(region + total)[0] = 0xFFFFFFFFu;                  /* 0x42e95 end marker */
    return region + 0x2c;                                 /* 0x42e9c */
}

/* Create one usable region (the original's creation pattern). Replaces the
 * DPMI acquirer FUN_00043071 (DPMI -> arena) for the port. */
uint32_t heap_acquire_region(uint32_t size) {
    uint32_t data_area = (size + 3) & 0xFFFFFFFCu;        /* round up to 4 */
    if (data_area < 0xc) data_area = 0xc;
    uint32_t total  = 0x2c + data_area;                   /* header + data area */
    uint32_t region = heap_arena_alloc(total);            /* arena slot */
    if (region == 0) return 0;                            /* no room */
    hw(region)[0] = total;                                /* preset total_size */
    heap_insert_init(region);                             /* insert + init */
    hw(region + 0x2c)[0] |= 1;                            /* mark big block ALLOCATED */
    R_ALLOC(region)++;                                    /* alloc_count++ */
    R_HWM(region) = 0xFFFFFFFFu;                          /* high_water = 0xffffffff */
    uint32_t data = region + 0x2c + 4;                    /* data pointer */
    heap_free_host(data);                                 /* free it (links into free list) */
    return data;
}

/* malloc wrapper over the whole all-blocks chain.  orig: 0x041af8 */
uint32_t heap_malloc(uint32_t size) {
    if (size == 0) return 0;                              /* 0x41b08/41b0a */
    if (size > 0xFFFFFFD4u) return 0;                     /* 0x41b0c/41b0f */
    uint8_t tried_grow = 0;                               /* [ESP] flag */
    for (;;) {                                            /* 0x41b2b top */
        uint32_t aligned = (size + 3) & 0xFFFFFFFCu;      /* 0x41b1d/41b27 */
        if (aligned < 0xc) aligned = 0xc;                 /* 0x41b2d/41b32 */
        uint32_t start;
        if (aligned <= g_heap_highwater) {                /* 0x41b37/41b3d JBE */
            g_heap_highwater = 0;                         /* 0x41b49 */
            start = g_heap_head;                          /* 0x41b4f */
        } else {                                          /* aligned > high_water */
            start = g_heap_cursor;                        /* 0x41b3f */
            if (start == 0) { g_heap_highwater = 0; start = g_heap_head; } /* 0x41b47/49/4f */
        }
        uint32_t result = 0;
        uint32_t region = start;
        for (;;) {                                        /* 0x41b8f */
            if (region == 0) break;                       /* 0x41b8f TEST; 41b91 JNZ */
            g_heap_cursor = region;                       /* 0x41b5f */
            result = heap_alloc(region, size);            /* 0x41b6f */
            if (result) return result;                    /* 0x41b74/76/78 */
            if (R_HWM(region) > g_heap_highwater)         /* 0x41b7a/7f/82/84 */
                g_heap_highwater = R_HWM(region);         /* 0x41b86 */
            region = B_NEXT(region);                      /* 0x41b8c */
        }
        if (tried_grow == 0) {                            /* 0x41b93/41b97 */
            if (heap_acquire_region(size)) { tried_grow = 1; continue; } /* 0x41b99/9b/a0 -> bb9 */
        }
        if (heap_grow(size)) { tried_grow = 0; continue; } /* 0x41ba4/a6/ab -> bb4 */
        return result;                                    /* 0x41bc2/41bc4 */
    }
}

/* free wrapper: find the region containing `data`, free it.  orig: 0x041c31 */
void heap_free_host(uint32_t data) {
    if (data == 0) { g_heap_flag = 0; return; }           /* port safety net */
    uint32_t region = g_heap_head;                        /* 0x41c37 */
    for (;;) {
        uint32_t next = B_NEXT(region);                   /* 0x41c4a */
        if (next == 0) break;                             /* last region (fallback) */
        if (region > data) { region = next; continue; }   /* 0x41c3f/41c41 JA */
        if (data < next) break;                           /* 0x41c43/41c45 JC -> found */
        region = next; continue;                          /* 0x41c47 */
    }
    heap_free(region, data);                              /* 0x41c5e/41c60/41c62 */
    if (region != g_heap_cursor) {                        /* 0x41c67/41c6d */
        if (R_HWM(region) > g_heap_highwater)             /* 0x41c6f/75/78/7a */
            g_heap_highwater = R_HWM(region);             /* 0x41c7c */
    }
    g_heap_flag = 0;                                      /* 0x41c82/41c84 */
}

/* ---- L0 batch A (9): real-mode/DOS stubs + command-line parser ---- */

/* DPMI real-mode callback trampoline A. The original sets ES/BP from [EBX],
 * loads a register context, and far-calls thunk 3*idx+0x43402 to run a
 * real-mode driver. The modern flat port never leaves protected mode; the CD /
 * keyboard I/O is served by the sys backend, so this is a faithful no-op.  orig: 0x04335c */
void rm_switch_a(uint32_t thunk_idx, const uint32_t *saved_ctx)
{ (void)thunk_idx; (void)saved_ctx; }

/* DPMI real-mode trampoline B: 32-byte register context.  orig: 0x0433ce */
void rm_switch_b(uint32_t key, const uint32_t *saved_ctx)
{ (void)key; (void)saved_ctx; }

/* FSAVE [dst]; WAIT: real-mode FPU state save. No-op in the flat model.  orig: 0x043979 */
void fpu_state_save(uintptr_t dst)
{ (void)dst; }

/* DOS INT 21h AH=25h: install the int 0x35 soft-timer handler. The modern port
 * installs its timer via SDL.  orig: 0x043958 */
void dos_install_timer_int(void) { /* no-op: timer handled by sys backend */ }

/* Character-classification table (DAT_0005185c): indexed by char, bitmask.
 * bit 0x01 control, 0x02 whitespace, 0x04 symbol, 0x08 printable-word,
 * 0x10 (a-k / A-K / digit class), 0x20 digit, 0x40 uppercase, 0x80 lowercase.
 * Used by the command-line parser and the token scanner. */
static const uint8_t kCharClass[256] = {
  0, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 3, 3, 3, 3, 3, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 10, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 56, 56, 56, 56, 56, 56, 56,
  56, 56, 56, 12, 12, 12, 12, 12,
  12, 12, 88, 88, 88, 88, 88, 88,
  72, 72, 72, 72, 72, 72, 72, 72,
  72, 72, 72, 72, 72, 72, 72, 72,
  72, 72, 72, 72, 12, 12, 12, 12,
  12, 12, 152, 152, 152, 152, 152, 152,
  136, 136, 136, 136, 136, 136, 136, 136,
  136, 136, 136, 136, 136, 136, 136, 136,
  136, 136, 136, 136, 12, 12, 12, 12,
  1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

/* DOS command-line switch parser.  orig: 0x043ca0 */
const char *cmdline_parse(const char *line, struct cmd_ctx *ctx)
{
    uint8_t f = ctx->flags;
    ctx->num = 0xffffffffu;                /* 0x43cae: [ctx+0xc] = -1 */
    f = (f & 3) | 1;                       /* 0x43cab/43cb7/43cba: (f&3)|1 */
    if (*line == '*') { line++; f &= 0xfe; } /* 0x43cc0-43ccb: '*' clears bit 0 */
    uint8_t c = (uint8_t)*line;            /* 0x43cd0 */
    if (kCharClass[(c + 1) & 0xff] & 0x20) {   /* 0x43ce1: next-class is digit */
        uint32_t n = 0;                    /* 0x43cea: ECX = 0 */
        for (;;) {
            n = n * 10u + (uint32_t)((int32_t)c - 0x30);  /* 0x43cec/43cf2/43cf6 */
            line++;                       /* 0x43cf5 */
            c = (uint8_t)*line;           /* 0x43cf8 */
            if (!(kCharClass[(c + 1) & 0xff] & 0x20)) break; /* 0x43d06 */
        }
        ctx->num = n;                     /* 0x43d0f */
    }
    if (*line == 'N') { f |= 8; line++; }           /* 0x43d32/43d34/43d36 */
    else if (*line == 'F') { f |= 4; line++; }      /* 0x43d3d/43d3f/43d41 */
    c = (uint8_t)*line;
    if (c < 0x68) {                              /* 0x43d30 */
        if (c == 'L') { f |= 0x40; line++; }      /* 0x43d47/43d49/43d4b */
    } else if (c < 0x69) {                       /* c == 0x68 'h' */
        f |= 0x10; line++;                       /* 0x43d4e/43d50/43d52 */
    } else if (c > 0x6b && (c < 0x6d || c == 0x77)) { /* 0x43d39/43d42/43d44: 'l' or 'w' */
        f |= 0x20; line++;                       /* 0x43d5b/43d5d/43d5f */
    }
    ctx->flags = f;
    return line;
}

/* Token scanner: count chars from ctx->next() whose next class is whitespace;
 * emit the last char unless flags&0x2.  orig: 0x043d79 */
uint32_t cmdline_count(struct cmd_ctx *ctx)
{
    uint32_t count = 0;                 /* ECX */
    uint16_t last;
    for (;;) {                          /* 0x43d81 */
        last = ctx->next(ctx);          /* CALL [ctx+0] with EAX=ctx */
        if (!(kCharClass[((uint8_t)last + 1) & 0xff] & 0x2)) break; /* 0x43d8e */
        count++;                        /* 0x43d97 */
    }
    if (!(ctx->flags & 0x2)) ctx->emit(last, ctx);  /* 0x43d9a-43da4 */
    return count;
}

/* ---- L0 batch B (9): buffered emit + event dispatch ---- */

/* Event table (DAT_000537dc). Keys/flags from the original; fn pointers are
 * wired to the ported handlers once they land (NULL = not yet ported; the
 * dispatch skips NULL fns, matching `OR EAX,EAX; JZ exit`). */
struct ev_entry g_evtab[10] = {
    { 0, 0x20, NULL },  /* 0x537dc: 0x418e6 */
    { 0, 0x20, NULL },  /* 0x537e2: 0x43702 */
    { 0, 0x01, NULL },  /* 0x537e8: 0x41f9c */
    { 0, 0x02, NULL },  /* 0x537ee: 0x439bd */
    { 0, 0x20, NULL },  /* 0x537f4: 0x4561d */
    { 0, 0x20, NULL },  /* 0x537fa: 0x4589e */
    { 0, 0x20, NULL },  /* 0x53800: 0x4933a */
    { 0, 0x20, NULL },  /* 0x53806: 0x4aa49 */
    { 0, 0x01, NULL },  /* 0x5380c: 0x41fa1 */
    { 0, 0x20, NULL },  /* 0x53812: 0x493b2 */
};

uint32_t emit_chars(struct emit_ctx *ctx, uintptr_t **pos)
{
    uint8_t fl = ctx->flags;
    uintptr_t dest = 0;
    if (fl & 0x1) {                                   /* 0x43db5 */
        if (fl & 0x4) { dest = *(*pos + 1); *pos += 2; } /* 0x43dc3/43dca far */
        else          { dest = **pos;     *pos += 1; } /* 0x43dd4/43de4 near */
    }
    uint32_t count = ctx->count;                       /* 0x43df2 */
    uint32_t written = 0;                              /* 0x43df5 ESI=0 */
    if (count == 0xffffffffu) count = 1;               /* 0x43df7/43dfc */
    while (count > 0) {                                /* 0x43e01/43e03 */
        uint16_t c = ctx->next(ctx);                   /* 0x43e05/43e07 CALL [ctx] */
        if (fl & 0x2) break;                           /* 0x43e09/43e0d peek */
        written++;                                     /* 0x43e0f */
        count--;                                       /* 0x43e10 */
        if (!(fl & 0x1)) continue;                     /* 0x43e11/43e15 skip mode */
        if (fl & 0x20) { *(uint16_t *)dest = c; dest += 2; } /* 0x43e17/43e1d word */
        else           { *(uint8_t *)dest = (uint8_t)c; dest += 1; } /* 0x43e26 byte */
    }
    return written;                                    /* 0x43e2c */
}

void emit_value(struct emit_ctx *ctx, uintptr_t **pos, uint32_t value)
{
    uint8_t fl = ctx->flags;
    if (!(fl & 0x1)) return;                           /* 0x43f29/43f2c */
    uintptr_t dest;
    if (fl & 0x4) { dest = *(*pos + 1); *pos += 2; }   /* 0x43f33 far */
    else          { dest = **pos;     *pos += 1; }     /* 0x43f44/43f54 near */
    if (fl & 0x10) *(uint16_t *)dest = (uint16_t)value; /* 0x43f67/43f6c word */
    else           *(uint32_t *)dest = value;           /* 0x43f78 dword */
}

uint32_t emit_step(struct emit_ctx *ctx)
{
    uint32_t value = ctx->count;                       /* 0x445aa */
    ctx->count = value - 1;                            /* 0x445b0 */
    if (value == 0) return 0xffffffffu;                /* 0x445b3/445b5 */
    uint16_t r = ctx->next(ctx);                       /* 0x445b7/445b9 */
    if (ctx->flags & 0x2) return 0xffffffffu;          /* 0x445bb/445bf/445c1 */
    return r;                                          /* 0x445c6 */
}

void dispatch_a(uint32_t key)
{
    for (;;) {                                          /* 0x4461f outer */
        uint8_t al = (uint8_t)key;                     /* 0x4462d AL=DL */
        int best = -1;
        for (int i = 0; i < 8; i++) {                  /* 0x4462f scan 8 entries */
            if (g_evtab[i].flag == 2) continue;        /* 0x44633 */
            if (g_evtab[i].key > al) continue;         /* 0x44638 JA */
            best = i; al = g_evtab[i].key;             /* 0x4463d/4463f */
        }
        if (best < 0) break;                           /* 0x44647/44649 EBX==EDI */
        void (*fn)(void) = g_evtab[best].fn;
        if (!fn) break;                                /* 0x44650 OR/JZ */
        fn();                                          /* 0x44655 */
        g_evtab[best].flag = 2;                        /* 0x44658 */
        /* 0x4465b JMP loop: re-init, re-scan */
    }
}

void dispatch_b(uint32_t eax_arg, uint32_t edx_arg)
{
    uint8_t dh = (uint8_t)edx_arg;                     /* 0x44667 DH=DL */
    for (;;) {                                          /* 0x4466b outer */
        uint8_t al = (uint8_t)eax_arg;                 /* 0x44679 AL=DL(=AL) */
        int best = -1;
        for (int i = 8; i < 10; i++) {                 /* 0x4467b scan 2 entries */
            if (g_evtab[i].flag == 2) continue;        /* 0x4467f */
            if (al > g_evtab[i].key) continue;         /* 0x44684 JA (al > key) */
            best = i; al = g_evtab[i].key;             /* 0x44689/4468b */
        }
        if (best < 0) break;                           /* 0x44693/44695 */
        if (!(al > dh)) {                              /* 0x44697/44699 */
            void (*fn)(void) = g_evtab[best].fn;
            if (fn) fn();                              /* 0x4469b-446a7 */
        }
        g_evtab[best].flag = 2;                        /* 0x446a8 */
        /* 0x446ab JMP loop: re-scan */
    }
}

/* ---- L0 batch C (9): char/string/count primitives + DPMI constants ---- */
int to_lower(int c) { return (c >= 0x41 && c <= 0x5a) ? c + 0x20 : c; }

size_t strn_len(const uint8_t *src, size_t limit) {     /* 0x45032 */
    size_t n = 0;
    while (n < limit && src[n] != 0) n++;
    return n;
}

size_t word_nlen(const uint16_t *src, size_t limit) {   /* 0x45056 */
    size_t n = 0;
    while (n < limit && src[n] != 0) n++;
    return n;
}

uint32_t const_64008(void) { return 0x64008; }         /* 0x45a6a */
uint32_t const_64004(void) { return 0x64004; }         /* 0x45a70 */

int g_dat_53390 = 0;                                    /* DAT_00053390 */
uint32_t (*g_cb_53644)(void) = NULL;                    /* DAT_00053644 */
int get_dat_53390(void) { return g_dat_53390; }         /* 0x44be6 */
uint32_t cb_indirect_53644(void) { return g_cb_53644 ? g_cb_53644() : 0; } /* 0x451b5 */

/* ---- L0 batch D (9): node list, modifier-prefix parser, debug hook ---- */
uint8_t  *g_ndpool = NULL;
uint32_t  g_node_list_active = 0;    /* DAT_00063ffc */
uint32_t  g_node_list_done = 0;      /* DAT_00063ff8 */

void node_deactivate(uint32_t rec_off)   /* 0x44b5d */
{
    /* Note: the original's node/record addresses are DPMI linear addresses and
     * never 0, so a 0 list head == empty. The offset pool mirrors this by
     * reserving offset 0 as the NULL sentinel (real nodes live at offset > 0). */
    uint8_t *base = g_ndpool;
    uint32_t *prev_next = &g_node_list_active;   /* 0x44b62 EDX=0x63ffc */
    uint32_t cur_off = g_node_list_active;        /* 0x44b67 EAX=[active] */
    while (cur_off != 0) {
        struct nd_node *cur = (struct nd_node *)(base + cur_off);
        if (cur->rec == rec_off) {                /* 0x44b70/44b72 found */
            base[rec_off + 0xc] |= 0x3;           /* 0x44b78-44b7e rec->flags |= 3 */
            *prev_next = cur->next;               /* 0x44b83 [prev]=next (unlink) */
            cur->next = g_node_list_done;         /* 0x44b90 found->next = done */
            g_node_list_done = cur_off;           /* 0x44b8b done = found */
            return;
        }
        prev_next = &cur->next;                   /* 0x44b74 EDX=cur (prev) */
        cur_off = cur->next;                      /* 0x44b76 EAX=cur->next */
    }
}

void parse_mod_prefix(const uint8_t *s, uint8_t *out)   /* 0x44fd7 */
{
    out[0x14] = 0;                                 /* 0x44fd9 init flags */
    for (;;) {
        uint8_t c = *s++;                          /* 0x44fdd BL=[EAX] */
        if (c == 0x2d) { out[0x14] |= 0x8; continue; }        /* '-' 0x44fdf */
        if (c == 0x23) { out[0x14] |= 0x1; continue; }        /* '#' 0x44fea */
        if (c == 0x2b) { out[0x14] = (out[0x14] | 0x4) & ~0x2; continue; } /* '+' 0x44ff5 */
        if (c == 0x20) { if (out[0x14] & 0x4) continue; out[0x14] |= 0x2; continue; } /* ' ' 0x4500d */
        if (c == 0x30) { out[0x16] = c; continue; }           /* '0' 0x45024 */
        break;                                         /* 0x45027/4502f exit */
    }
}

uint8_t g_debug_break = 0;               /* DAT_00053624 */
uint32_t debug_break_hook(uint32_t eax, uint32_t edx)   /* 0x45a44 */
{
    if (g_debug_break) sys_breakpoint(eax, (uint16_t)edx);   /* 0x45a4d-45a55 INT3 */
    return 0;                                         /* 0x45a67 XOR EAX; RET */
}

/* ---- L0 batch D2 (2): DOS clock calibration + delay ---- */
uint32_t g_clock_ticks = 0;                    /* DAT_00064000 */

void calibrate_clock(void)   /* 0x4561d */
{
    /* Original: wait for the DOS second to roll over, then count INT 21h/0x2Ch
     * ticks until the second changes again; store the count (≈18). Port: take
     * the rate from the sys layer (default 18). */
    g_clock_ticks = sys_clock_ticks_per_sec();
}

void delay_units(uint32_t arg)   /* 0x4564f */
{
    if (arg == 0) return;                             /* 0x45653/45655 */
    uint32_t v = arg * g_clock_ticks + 0x1f4;         /* 0x45657/45663 */
    uint32_t es_i = v / 0x3e8;                        /* 0x4566a DIV 1000 */
    if (es_i == 0) es_i = 1;                          /* 0x4566e-45672 */
    while (es_i-- > 0) sys_clock_read();              /* 0x45679-45680 */
}

/* ---- L0 batch E (2): swap-global + jump-table field accessor ---- */
uint32_t g_dat_5363c = 0;                       /* DAT_0005363c */
uint32_t swap_dat_5363c(uint32_t v)   /* 0x45ae8 */
{
    uint32_t old = g_dat_5363c;              /* 0x45aef EAX=[0x5363c] */
    g_dat_5363c = v;                         /* 0x45af3 [0x5363c]=EAX */
    return old;
}

uint32_t case_field_0x28(uintptr_t s)   /* 0x4642a */
{
    return *(uint32_t *)(s + 0x28);        /* 0x4642b ADD EBX,[EBP+0x28] */
}

/* ---- L0 batch F (6): misc primitives ---- */
uint32_t stack_depth(void)   /* 0x4917e */
{
    int local;
    return (uint32_t)((uintptr_t)&local - g_stack_bound);
}
uint32_t g_dat_53640 = 0;                    /* DAT_00053640 */
void set_53640(void) { g_dat_53640 = 0x8000; }              /* 0x49187 */
uint32_t to_upper(uint32_t c) { return (c>=0x61 && c<=0x7a) ? c-0x20 : c; } /* 0x494bd */
uint32_t *g_arr_53730 = NULL;                /* DAT_00053730 */
void store_arr53730(uint32_t index, uint32_t value)   /* 0x49325 */
{
    g_arr_53730[index] = value | 0x40000;    /* OR DH,0x40 sets bit22 */
}
void noop_9109(void) {}                          /* 0x49109: RET */
uint32_t ioctl_device(uint32_t handle)   /* 0x492b2 */
{
    uint32_t dx, cf;
    sys_dos_ioctl(handle, &dx, &cf);             /* AL=0 AH=0x44 INT 21 (stub) */
    uint32_t rcl = (dx << 1) | cf;               /* RCL EDX,1 */
    uint32_t ror = (rcl >> 1) | (rcl << 31);     /* ROR EDX,1 */
    return (ror & 0x80) ? 1 : 0;                 /* TEST DL,0x80; SETNZ AL */
}

/* ---- L0 batch G (2): fixed-point number constructor ---- */
/* Shared normalize: BSR to find MSB bitpos, shift so bit31 is set, store the
 * normalized value in BOTH lo and hi (the assembly writes EAX to [out] and EDX
 * (=EAX) to [out+4]); exp = bitpos + expbase. v==0 -> all-zero. */
static void fx_normalize(struct fxnum *out, uint32_t v, uint16_t expbase)
{
    if (v == 0) { out->lo = 0; out->hi = 0; out->exp = 0; return; }   /* JZ 0x47f42 */
    int p = 31 - __builtin_clz(v);            /* BSR ECX,EAX */
    uint32_t norm = v << (31 - p);            /* SHL EAX, 31-bitpos */
    out->lo = norm;                           /* MOV [out],EAX */
    out->hi = norm;                           /* MOV [out+4],EDX (EDX=EAX) */
    out->exp = (uint16_t)(p + expbase);       /* exp = bitpos + base, low16 */
}
void fx_from_u32(struct fxnum *out, uint32_t v)   /* 0x47f1f (base 0x3fff) */
{
    fx_normalize(out, v, 0x3fff);
}
void fx_from_s32(struct fxnum *out, int32_t v)    /* 0x47f0e (sign thunk) */
{
    if (v >= 0) fx_normalize(out, (uint32_t)v, 0x3fff);   /* JNS: positive */
    else        fx_normalize(out, 0u - (uint32_t)v, 0xbfff); /* NEG EAX, base|0x8000 */
}

/* ---- L0 batch H (4) ---- */
/* Compare two fxnum by (exp, hi, lo); 1 if a < b lexicographically. */
uint32_t fx_less(const struct fxnum *a, const struct fxnum *b)   /* 0x498c6 */
{
    if (a->exp < b->exp) return 1;          /* 0x498d5 CMP ECX,ESI; JL */
    if (a->exp > b->exp) return 0;          /* 0x498d8 JNZ -> 0 */
    if (a->hi < b->hi)   return 1;          /* 0x498dd */
    if (a->hi > b->hi)   return 0;          /* 0x498e0 */
    if (a->lo < b->lo)   return 1;          /* 0x498e5 CMP EAX,[EDX]; JGE */
    return 0;
}
/* Parse leading decimal digits (v = v*10 + digit) until a non-digit. */
void parse_decimal(const char *s, uint32_t *out)   /* 0x49a48 */
{
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') {         /* 0x49a4e CMP JC/JA */
        v = v * 10u + (uint32_t)(*s - '0');  /* IMUL EDX,EDX,0xa; ...; SUB 0x30 */
        s++;
    }
    *out = v;
}
void dos_realmode_setup(void) { sys_dos_realmode_setup(); }  /* 0x490b0 */
uint16_t fpu_init(void) { return sys_fpu_control_word(); }   /* 0x4910a */

/* ---- L0 batch I (1 real + 1 spurious) ---- */
int32_t strn_casecmp(const uint8_t *a, const uint8_t *b, uint32_t n)   /* 0x4a7a2 */
{
    while (n) {
        uint8_t ca = *a, cb = *b;
        if (ca >= 0x41 && ca <= 0x5a) ca += 0x20;      /* 0x4a7c5: lowercase a */
        if (cb >= 0x41 && cb <= 0x5a) cb += 0x20;      /* 0x4a7d8: lowercase b */
        if (ca != cb) return (int32_t)ca - (int32_t)cb; /* 0x4a7e0: return a-b */
        if (cb == 0) return 0;                          /* 0x4a7aa: NUL stop */
        a++; b++; n--;
    }
    return 0;
}

/* ---- L0 batch J (1 real) ---- */
uint32_t tokenize(char *s, char **out, char **end_out)          /* 0x4595b */
{
    uint32_t count = 0;      /* iVar4 */
    char *pc3 = s;           /* pcVar3: final scan position */
    uint32_t ret = 0;        /* iVar5: return count */
    for (;;) {
        while (*s == ' ' || *s == '\t') s++;   /* skip leading space/tab */
        pc3 = s;
        ret = count;
        if (*s == '\0') { *end_out = pc3; return ret; }   /* 0x45a34 */
        char *pc6 = s;        /* in-place write pointer */
        char *tok;            /* local_18: token start to store */
        if (*s == '"') {        /* quoted token (0x4598c) */
            char *p = s + 1;
            pc6 = s + 1;
            while (*p != '"' && *p != '\0') {
                char *q = p;
                if (*p == '\\' && (p[1] == '"' || p[1] == '\\')) q = p + 1;
                p = q + 1;
                if (out) { *pc6 = *q; pc6++; }
            }
            pc3 = p; tok = s + 1;
        } else {                /* bare token (0x459c5) */
            char *p = s;
            tok = s;
            while (*p != '\0' && *p != ' ' && *p != '\t') {
                char *q = p;
                if (*p == '\\' && p[1] == '"') q = p + 1;
                p = q + 1;
                if (out) { *pc6 = *q; pc6++; }
            }
            pc3 = p;
        }
        s = pc3 + 1;
        ret = count + 1;
        if (out) {
            out[count] = tok;                        /* store token start */
            if (*pc3 == '\0') { *pc6 = '\0'; *end_out = pc3; return ret; }
            *pc6 = '\0';                              /* NUL-terminate token */
            count = ret;
        } else {
            count = ret;
            if (*pc3 == '\0') break;
        }
    }
    *end_out = pc3;
    return ret;
}
