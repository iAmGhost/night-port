/* f.c — ported original functions (batch 1: L0 primitives).
 * Bodies are 1:1 translations of the decompilation in analysis/pseudo/,
 * cross-checked against the disassembly (scratch/batch1_asm*.txt).
 * Semantics: docs/12-functions/primitives.md */
#include <stdio.h>
#include <stdlib.h>
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

/* fn_040660  orig: 0x040660 (L10, 52 insns) -- VGA DAC row-blit "fade in" (ascending).
 *   arg1 (EAX) = 0x300-byte source row, arg2 (EDX) = frame count (>0). For each
 *   frame i in 0..arg2-1: local[j] = (uint8)arg1[j] * i / arg2  (j in 0..0x2ff),
 *   then fn_00035ec2 (no-op VGA sequencer ready-wait) + board_out(0, 0x100,
 *   local) = outb(0x3C8, 0) + OUTSB 0x300 bytes of local to 0x3C9. So the DAC
 *   data port (0x3C9) receives arg2 frames of 768 scaled bytes each: a fade from
 *   all-zeros (i=0) toward arg1 (i -> arg2). Disasm-verified (PUSH 0x334 canary,
 *   local @ EBP-0x310, i=[EBP-0x10] 0..arg2, j=[EBP-0xc] 0..0x2ff).
 *   FAITHFULNESS: arg1[j] is zero-extended (MOV DL) before IMUL, so the product
 *   is non-negative and the SAR+IDIV reduces to an unsigned divide; the result
 *   fits a byte (i < arg2 => arg1[j]*i/arg2 < 256). */
void fn_040660(const uint8_t *arg1, uint32_t arg2) {
    (void)canary_id((uintptr_t)arg1, 0x334);
    uint8_t local[0x300];
    for (uint32_t i = 0; i < arg2; i++) {
        for (uint32_t j = 0; j < 0x300; j++)
            local[j] = (uint8_t)(((uint32_t)arg1[j] * i) / arg2);
        fn_00035ec2();
        board_out(0, 0x100, local);
    }
}

/* fn_0406f5  orig: 0x0406f5 (L10, 52 insns) -- mirror of 0x040660 (fade out,
 *   descending): frames i = arg2, arg2-1, ..., 0 (arg2+1 frames). Same per-frame
 *   scale local[j] = (uint8)arg1[j] * i / arg2 and the same board_out blit; the
 *   final frame (i=0) is all zeros. LATENT: each frame divides by arg2, so
 *   arg2 == 0 would raise #DE on the i=0 frame (the game always calls with
 *   arg2 > 0). Disasm-verified (i=[EBP-0x10] init to arg2, while i>=0, i--).
 *   FAITHFULNESS: arg1[j] is zero-extended (MOV DL) before IMUL, so the product
 *   is non-negative and the SAR+IDIV reduces to an unsigned divide; the result
 *   fits a byte (i <= arg2 => arg1[j]*i/arg2 <= 255). */
void fn_0406f5(const uint8_t *arg1, uint32_t arg2) {
    (void)canary_id((uintptr_t)arg1, 0x334);
    uint8_t local[0x300];
    for (int32_t i = (int32_t)arg2; i >= 0; i--) {
        for (uint32_t j = 0; j < 0x300; j++)
            local[j] = (uint8_t)(((uint32_t)arg1[j] * (uint32_t)i) / arg2);
        fn_00035ec2();
        board_out(0, 0x100, local);
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

/* itoa32_tbl  orig: 0x049459
 *   Table-based base-c itoa using the runtime digit table at 0x049434
 *   ("0123456789abcdefghijklmnopqrstuvwxyz"). Observable output is byte-identical
 *   to itoa32 (0x041ef8); the original is a separate function that computes the
 *   same reversed-digit string via a local scratch buffer and a fixed NUL
 *   sentinel. Delegated to itoa32. */
char *itoa32_tbl(uint32_t a, char *b, uint32_t c) { return itoa32(a, b, c); }

/* is_leap  orig: 0x0494cb
 *   Gregorian leap-year test: (year%4==0 && (year%100!=0 || year%400==0)).
 *   Returns (ctx << 32) | is_leap. The original does the two remainder checks
 *   with signed IDIV (SAR EDX,0x1f; IDIV), so a positive year behaves like an
 *   unsigned mod; EBX/ECX/EDX are preserved so the caller's EDX (ctx) is
 *   returned in the high 32 bits and the 0/1 flag in EAX. */
uint64_t is_leap(uint32_t year, uint32_t ctx) {
    int y = (int)year;
    int leap = ((year & 3u) == 0) && ((y % 100 != 0) || (y % 400 == 0));
    return ((uint64_t)ctx << 32) | (uint32_t)leap;
}

/* fn_0498f6  orig: 0x0498f6
 *   Date/time decoder (108 ins). Decodes an absolute day count (param_1, adjusted
 *   by param_3 delta-days per a sub-noon borrow rule) + seconds (param_2) into
 *   hour/min/sec + year/month/day + day-of-week + day-of-year. Year is a count
 *   from a 1900 epoch (is_leap is queried with year+1900); day 0 = Jan 1 1900.
 *   Month is a /31 guess + cumulative-day-table compare. The m==11 (December)
 *   slot reads the byte PAST the table (0x51978=365 non-leap / 0x51998=0 leap);
 *   the 13th table element below replicates that read so the leap-year December
 *   latent bug (month becomes 12) is preserved 1:1. is_leap(year,ctx) returns
 *   (ctx<<32)|leap; the ctx register (year_est) is echoed back and reused as the
 *   running year (register-ABI), and the year-length is a CONSTANT (365/366 from
 *   the estimated year's leap status) subtracted in the countdown loop. */
struct dec_datetime *fn_0498f6(int32_t base_day, uint32_t seconds, int32_t delta, struct dec_datetime *out)
{
    /* 13th slot = the original's out-of-table 16-bit read (0x51978=365 / 0x51998=0) */
    static const int16_t nonleap_tbl[13] = { 0,31,59,90,120,151,181,212,243,273,304,334,365 }; /* DAT_00051960 */
    static const int16_t leap_tbl[13]    = { 0,31,60,91,121,152,182,213,244,274,305,335,0   }; /* DAT_0005197a */

    uint32_t sec;
    int32_t day_total;
    if ((seconds < 0xa8c0u) && (delta > 0)) {                    /* 0x499ff-49909 */
        sec = seconds + 0x15180u - (uint32_t)delta;               /* 0x4990b-49911 */
        day_total = base_day + (int32_t)(sec / 0x15180u) - 1;     /* 0x49915-49920 */
    } else {
        sec = seconds - (uint32_t)delta;                          /* 0x49928 */
        day_total = base_day + (int32_t)(sec / 0x15180u);         /* 0x4992c-49930 */
    }

    uint32_t s = sec % 0x15180u;                                  /* 0x49932-4993d */
    out->hour = (int32_t)(s / 0xe10u);                            /* 0x4994c */
    uint32_t rem = s % 0xe10u;
    out->min  = (int32_t)(rem / 0x3cu);                           /* 0x49962 */
    out->sec  = (int32_t)(rem % 0x3cu);                           /* 0x49970 */

    int32_t year_est = day_total / 365;                            /* 0x49974 */
    int32_t day_in_year = day_total - year_est * 365;              /* 0x49989-4998b */
    if (year_est != 0)
        day_in_year -= (year_est - 1) / 4;                         /* 0x49991-49997 */
    uint64_t r1 = is_leap((uint32_t)(year_est + 0x76c), (uint32_t)year_est); /* 0x4999f */
    int32_t year_len = (int32_t)(r1 & 0xffffffffu) + 0x16d;        /* 0x499a4: 365 or 366 */
    int32_t year = (int32_t)(r1 >> 32);                            /* echoed ctx = year_est */
    while (day_in_year >= year_len) {                              /* 0x499a9-499b0 */
        day_in_year -= year_len;
        year++;
    }
    out->doy  = day_in_year;                                       /* 0x499b7 */
    out->year = year;                                              /* 0x499c0 */

    uint64_t r2 = is_leap((uint32_t)(year + 0x76c), 0);            /* 0x499c3 */
    const int16_t *tbl = (r2 & 0xffffffffu) ? leap_tbl : nonleap_tbl;
    int32_t m = day_in_year / 0x1f;                                /* 0x499da */
    if ((uint16_t)tbl[m + 1] <= (uint16_t)day_in_year)             /* 0x499de-499e8 straddle */
        m++;
    out->month = m;                                                /* 0x499e9 */
    out->day   = day_in_year - (int32_t)tbl[m] + 1;                /* 0x499f0-499f8 */

    out->dow = (day_total + 1) % 7;                                /* 0x499f5-49a04 */
    return out;
}

/* fn_04322f  orig: 0x04322f
 *   4-byte constant function: `mov eax, -1; ret`. Returns -1 (0xffffffff). */
int32_t fn_04322f(void) {
    return -1;
}

/* fn_044848  orig: 0x044848
 *   lseek wrapper (8 ins). Sets EBX=1 (whence), EDX=0 (offset), then
 *   fn_044807(EAX=handle, EDX=0, EBX=1) = absolute seek to 0 (SEEK_SET). The
 *   caller's param_2/param_3 (EDX/EBX) are clobbered and param_4 (ECX) unused,
 *   so only the handle is observable. Returns fn_044807's result. */
int32_t fn_044848(uint32_t handle, uint32_t a2, uint32_t a3, uint32_t a4) {
    (void)a2; (void)a3; (void)a4;
    return fn_044807(handle, 0, 1);
}

/* fn_049a0b  orig: 0x049a0b
 *   unix-time decoder wrapper (11 ins). Writes out->mode=0 (+0x20) then
 *   fn_0498f6(0x63df, *seconds, 0, out): base_day=0x63df (25567 = the 1900->1970
 *   epoch offset in days), seconds=*[param_1], delta=0. param_3/param_4 (ECX /
 *   extra register) are passed through and unused. */
void fn_049a0b(uint32_t *seconds, struct dec_datetime *out, uint32_t p3, uint32_t p4) {
    (void)p3; (void)p4;
    out->mode = 0;                                                /* 0x49a0f */
    fn_0498f6(0x63df, *seconds, 0, out);                         /* 0x49a1f */
}

/* fn_03eff2  orig: 0x03eff2
 *   Install the IRQ 0x1c (FPU) handler, guarded by the pending-install flag
 *   DAT_00063e4a. Runs with interrupts disabled (irq_cli/irq_sti, both no-op
 *   standins in the flat model); isr_set/outb are the port's BIOS wrappers.
 *   Effective no-op when the flag is not 1. No stack params are read. */
void fn_03eff2(void)
{
    irq_cli();                                              /* 0x3f00a */
    if (g_dat_63e4a == 1) {                                /* 0x3f011-19 */
        isr_set(0x1c, g_dat_63e44, g_dat_63e48);           /* 0x3f027-032 */
        outb(0x43, 0x36);                                  /* 0x3f037-41 (PIT ch0 mode) */
        outb(0x40, 0xff);                                  /* 0x3f046-50 (ch0 low) */
        outb(0x40, 0xff);                                  /* 0x3f055-5f (ch0 high) */
        g_dat_63e4a = 0;                                   /* 0x3f064 */
    }
    irq_sti();                                              /* 0x3f06b */
}

/* fn_03ffab  orig: 0x03ffab
 *   Install the IRQ 9 handler, guarded by the pending-install flag DAT_00063e86.
 *   On install: isr_set(9, off, seg), clear the flag, and mirror the IRQ-9 source
 *   byte (DAT_00063f51) into the real-mode low-memory flag DAT_00000417. The
 *   original's stack params (ECX/param_3) are not read. No-arg in effect. */
void fn_03ffab(void)
{
    if (g_dat_63e86 == 1) {                                /* 0x3ffc5-cd */
        isr_set(9, g_dat_63e80, g_dat_63e84);              /* 0x3ffcf-ff6 */
        g_dat_63e86 = 0;                                   /* 0x3ffeb */
        g_dat_00417 = g_dat_63f51;                         /* 0x3fff2-4002 */
    }
}

/* fn_03575d  orig: 0x03575d
 *   PIT channel-0 rate setup. The divisor is passed in EAX (register-ABI; the
 *   original's `PUSH 0x20; CALL 0x418ee` canary prologue preserves it). Stores
 *   the (clamped) divisor in g_dat_63318, clears g_dat_6332c, then writes PIT
 *   mode 0x34 (ch0, rate interval, lo/hi, binary) and the 16-bit divisor via
 *   outb (no-op standin in the flat model). Clamp: 0 -> 0x10000; >0x10000 ->
 *   0x10000 stored but 0 written to the hardware (divisor 0 = stop). */
void fn_03575d(uint32_t divisor)
{
    g_dat_6332c = 0;                          /* 0x35778 */
    g_dat_63318 = divisor;                    /* 0x35782-85 */
    if (divisor == 0) g_dat_63318 = 0x10000;  /* 0x3578a-90 */
    if (0x10000 < (int32_t)g_dat_63318) {     /* 0x3579a-a4 */
        g_dat_63318 = 0x10000;
        divisor = 0;
    }
    outb(0x43, 0x34);                         /* 0x357b7-c1 (PIT ch0 mode) */
    outb(0x40, (uint8_t)divisor);             /* 0x357c6-d4 (ch0 lo) */
    outb(0x40, (uint8_t)(divisor >> 8));      /* 0x357d9-e4 (ch0 hi) */
}

/* fn_03f3be  orig: 0x03f3be  (and twin fn_03f410, index at 0x63e58)
 *   VGA palette-register setter (INT 10h, AX=0x4f05 "Set Palette Register").
 *   `color` is passed in EAX (register-ABI). value = color << (g_dat_63e4c&0x1f);
 *   if value != g_dat_63e50[index], store it and issue the INT 10h call
 *   (no-op standin in the flat model). Only the palette cache update is
 *   observable here. The two twins differ solely in which index global they use
 *   (fn_03f3be -> g_dat_63e5c, fn_03f410 -> g_dat_63e58). */
static void palette_set(uint32_t index, uint32_t color)
{
    uint32_t v = color << (g_dat_63e4c & 0x1f);          /* 0x3f3f8 */
    if (v != g_dat_63e50[index]) {                       /* 0x3f3fa-fc */
        g_dat_63e50[index] = v;                          /* 0x3f3fe */
        /* INT 0x10 AX=0x4f05 (EBX=index, EDX=v): VGA set-palette-register. */
        /* No-op in the flat model (no VGA); only the cache update is real. */
    }
}
void fn_03f3be(uint32_t color) { palette_set(g_dat_63e5c, color); }
void fn_03f410(uint32_t color) { palette_set(g_dat_63e58, color); }

/* fn_1053f  orig: 0x01053f
 *   DPMI INT 0x31h AX=0x101 (Set Debug Registers): DX = word at (DAT_00053c5c - 2).
 *   No DPMI in the flat model -> no-op. The DAT_00053c5c-2 read only feeds the
 *   DPMI argument and has no observable flat-model effect, so it is not
 *   dereferenced; the global is tracked for fidelity. */
uint32_t g_dat_63e14 = 0;   /* DAT_00063e14: 2-node swap counter (fn_03e14c) */
uint32_t g_dat_63e18 = 0;   /* DAT_00063e18: cached prev-node offset (fn_03e14c) */
uint32_t g_dat_63e1c = 0;   /* DAT_00063e1c: cached next-node offset (fn_03e14c) */
uint32_t g_dat_63e20 = 0;   /* DAT_00063e20: 3rd cache slot (zeroed, not read) */
uint32_t g_dat_62a8c = 0;   /* DAT_00062a8c: zeroed by fn_03ee0a (list-destroy) */
uint32_t g_dat_63364 = 0;   /* DAT_00063364: removed-element flag byte (fn_031f5f) */
uint32_t g_dat_63368 = 0;   /* DAT_00063368: removed-element value word (fn_031f5f) */
uint8_t g_dat_53c4c[9] = {0};   /* DAT_00053c4c: 9-byte real-mode command staging area */
uintptr_t g_dat_53c5c = 0;   /* DAT_00053c5c: DPMI far-pointer (debug-reg source) */
void fn_1053f(void)
{
    (void)g_dat_53c5c;   /* DPMI INT 0x31 AX=0x101: no-op in the flat model. */
}

/* ---- DPMI memory-region / real-mode-call initializers (fn_0003fade, 0x40570,
 *        0x405a7, 0x407f8) ---- */
/* fn_0003fade  orig: 0x0003fade (L10, 47B)
 *   DPMI memory-region initializer: fn_041e20(&DAT_000a0000, 0, 64000) = zero
 *   64000 bytes at the high-memory / DPMI region DAT_000a0000. DAT_000a0000 is
 *   not modeled in the flat port (no other reference), so the zeroing is a
 *   no-op. canary prologue omitted. */
void fn_0003fade(void) {}

/* fn_00040570  orig: 0x00040570 (L10, 54B)
 *   DPMI real-mode request: builds a 14-word stack struct (word0=3) and calls
 *   fn_041e51(0x10, &struct, &struct) — the DPMI INT 31h real-mode trampoline
 *   (a flat-model no-op). The struct is stack-local and the trampoline does
 *   nothing observable, so the whole function is a no-op. canary omitted. */
void fn_00040570(void) {}

/* fn_000405a7  orig: 0x000405a7 (L10, 54B)
 *   Same as fn_00040570 but the request struct word0=0x13. Flat-model no-op. */
void fn_000405a7(void) {}

/* fn_000407f8  orig: 0x000407f8 (L10, 64B)
 *   DPMI real-mode request: builds a stack struct (dword0=0x101, +canary) and
 *   calls fn_041e51(0x31, &struct, &struct). Flat-model no-op. */
void fn_000407f8(void) {}

/* fn_00010dde  orig: 0x00010dde (L12, 54B)
 *   Builds a ~2KB stack request struct (seeded from DAT_00051b54/51b58,
 *   dword@+0xf=0x85000d) and calls fn_00010570(&struct, 0) — a flat-model no-op
 *   trampoline. The struct is stack-local; no observable effect. */
void fn_00010dde(void) {}

/* fn_0003fcbd  orig: 0x0003fcbd (L10, 72B)
 *   DPMI real-mode request: builds a stack struct (dword0=0x1012, +param_3) and
 *   calls fn_041e51(0x10, &struct, &struct) — the no-op trampoline. No-op. */
void fn_0003fcbd(uint32_t p1, uint32_t p2, uint32_t p3) { (void)p1;(void)p2;(void)p3; }

/* fn_00040788  orig: 0x00040788 (L10, 111B)
 *   DPMI memory allocation (INT 31h AX=0x100 via the fn_041e51 trampoline) with
 *   request size 0x100. Returns the allocated linear address (segment<<4) on
 *   success or 0 on failure. No DPMI in the flat model -> the allocation never
 *   happens; return 0. */
int fn_00040788(void) { return 0; }

/* fn_000104c7  orig: 0x000104c7 (L10, 119B)
 *   DPMI memory allocation + setup: builds a request struct (word0=0x1500),
 *   calls the fn_041e51 (INT 31h) trampoline, then writes the returned segment
 *   into DAT_00053c58/DAT_00053c5c. No DPMI in the flat model -> the allocation
 *   and the writes to the fake segment do not happen. No-op. */
void fn_000104c7(void) {}

/* fn_0003f645  orig: 0x0003f645 (L11, 118B)
 *   VGA palette + frame-buffer init loop: fn_03f3be(0), then while
 *   (offset <= DAT_00063e64*DAT_00063e68): fn_041e20(&DAT_000a0000, canary,
 *   DAT_000531f4) (memset the A0000 region, not modeled) + fn_03f3be(i) (VGA
 *   palette, no-op). Every body effect is a flat-model no-op, so the whole
 *   loop is a no-op. */
void fn_0003f645(void) {}

/* fn_00035ec2  orig: 0x00035ec2 (L10, 88B)
 *   VGA 0x3DA sequencer ready-wait: do inb(0x3da) until (b&9)!=9, then until
 *   (b&9)==9. In the flat model inb(0x3da) is a port stub returning a constant
 *   (default 0), so the second spin (wait for the ready bit) would never see
 *   it and would loop forever. No-op standin (documented degenerate case). */
void fn_00035ec2(void) {}

/* fn_00041bdc  orig: 0x00041bdc (L6, 74B)
 *   heap_malloc wrapper: if (size == 0) size = 1; return heap_malloc(size).
 *   The original returns a 64-bit (EDX:EAX); EDX (param_2) is the caller's
 *   register (register-ABI), so the meaningful result is the 32-bit pointer. */
uint32_t fn_00041bdc(uint32_t size) { if (size == 0) size = 1; return heap_malloc(size); }

/* fn_00035d0f  orig: 0x00035d0f (L10, 77B)
 *   DMA channel busy-wait: three memory-poll spin loops on DAT_00063e88[AL]
 *   (wait ==0, then !=0, then ==0 -- the classic DMA busy/idle handshake). In
 *   the flat model that byte is a static global (init 0), so the "wait for
 *   !=0" spin never sees it change and would loop forever. No-op standin
 *   (documented degenerate case). No flat-model observable effect. */
void fn_00035d0f(void) {}

int32_t  g_dat_530b4 = 0;  /* DAT_000530b4: timer rate multiplier (set by game) */
int32_t  g_dat_530b8 = 0;  /* DAT_000530b8: timer rate multiplier #2 (used by fn_035b91) */
int32_t  g_dat_530bc[16]; /* DAT_000530bc: 16-entry command table; fn_035b0c writes [a0&0xf]=a2 */
uint32_t g_dat_63304 = 0;  /* DAT_00063304: timer divisor (denominator in fn_03565a) */
uint32_t g_dat_63330 = 0;  /* DAT_00063330: last timer tick value (written by fn_0356ba) */
uint32_t g_dat_63324 = 0;  /* DAT_00063324: IRQ/device enabled flag (gate + cleared by fn_034eb0) */
uint32_t g_dat_63334 = 0;  /* DAT_00063334: IRQ8 handler offset (isr_set arg) */
uint32_t g_dat_63338 = 0;  /* DAT_00063338: IRQ8 handler segment (isr_set arg, low 16) */
uint32_t g_dat_6333c = 0;  /* DAT_0006333c: memory block freed via fn_041c31 (fn_034eb0) */

/* fn_000356ba  orig: 0x000356ba (L11, 76B)
 *   Timer rate setup (register-ABI, input ticks in EAX). Stores
 *   DAT_00063330 = ticks, computes the PIT ch0 divisor = (ticks*DAT_000530b4)/100
 *   (32-bit signed IMUL, then sign-extended 64-bit IDIV 100), and calls
 *   fn_03575d(divisor) to program the PIT. The IDIV remainder (EDX) is
 *   clobbered by the CALL and is unused. fn_03575d writes PIT port stubs. */
void fn_000356ba(uint32_t ticks) {
    g_dat_63330 = ticks;
    int32_t prod = (int32_t)ticks * g_dat_530b4;          /* IMUL (32-bit) */
    int32_t q    = (int32_t)((int64_t)prod / 100);        /* IDIV 100 (signed) */
    fn_03575d((uint32_t)q);
}

/* fn_00010a56  orig: 0x00010a56 (L12, 131B)
 *   CD track-entry submit (register-ABI, index in EAX): if (index <
 *   DAT_00053c60) copy a 5-dword entry from DAT_00051af0 into a stack struct,
 *   compute an offset from DAT_00053c64[index*5], then fn_00010570(&struct,0)
 *   -- a flat-model no-op trampoline. The only observable effect (the trampoline
 *   call) is a no-op, so the whole function is a no-op. The register-ABI index
 *   only feeds the no-op. */
void fn_00010a56(void) {}

/* fn_0003f592  orig: 0x0003f592 (L11, 178B)
 *   VGA vertical-line plotter (register-ABI: pixel in CL, width/stride from
 *   DAT_00063e64): computes a start offset, issues fn_03f3be (VGA palette,
 *   no-op) and writes param_3 strided pixels into the A0000 video buffer
 *   (&DAT_000a0000). DAT_000a0000 is not modeled in the flat port and fn_03f3be
 *   is a no-op, so every body effect is a no-op. No-op standin. */
void fn_0003f592(void) {}

uint8_t g_dat_5d08c[0x14];  /* DAT_0005d08c: 20-byte state buffer (zeroed by fn_0003c7d7) */
uint8_t g_dat_5d0a0[0x14];  /* DAT_0005d0a0: 20-byte state buffer (zeroed by fn_0003c7d7) */

/* fn_0003c7d7  orig: 0x0003c7d7 (L10, 123B)
 *   Struct-field reset (register-ABI, struct pointer in EAX). Zeroes six fields
 *   of *s (dwords at +0x2a/+0x2e/+0x32/+0x36, bytes at +0x3a/+0x3b) and memsets
 *   the two 20-byte buffers DAT_0005d08c / DAT_0005d0a0 (fn_041e20). */
void fn_0003c7d7(uint8_t *s) {
    *(uint32_t *)(s + 0x2a) = 0;
    *(uint32_t *)(s + 0x2e) = 0;
    *(uint32_t *)(s + 0x32) = 0;
    *(uint32_t *)(s + 0x36) = 0;
    s[0x3a] = 0;
    s[0x3b] = 0;
    fn_041e20(g_dat_5d0a0, 0, 0x14);
    fn_041e20(g_dat_5d08c, 0, 0x14);
}

/* 0x03c6ec: reset one struct node (register-ABI, node pointer in EAX). canary
 * prologue PUSH 0x24; CALL 0x418ee. Calls fn_0003c7d7(s) (zero 6 fields + memset
 * 2 buffers), then zeroes the dword at s+0x44, and returns s (the node). */
uint8_t *fn_03c6ec(uint8_t *s) {
    (void)canary_id((uintptr_t)s, 0x24);   /* 0xc6ec-0xc6f1 */
    fn_0003c7d7(s);                         /* 0xc70a */
    *(uint32_t *)(s + 0x44) = 0;            /* 0xc712 */
    return s;                               /* 0xc71f */
}

/* 0x03d88f: reset every node in the linked list rooted at DAT_00063350. canary
 * prologue PUSH 0x20; CALL 0x418ee, no args. Walks next = node+0x34c from
 * g_dat_63350, calling fn_0003c7d7(node) on each; at loop exit EAX holds the null
 * next pointer, so it returns 0. POINTER-WIDTH: the original 4-byte next field at
 * +0x34c is read as a full 64-bit pointer in the flat port (cf. fn_0003b5c0). */
uint8_t *g_dat_63350 = NULL;  /* DAT_00063350: reset-list head */
uint32_t fn_03d88f(void) {
    (void)canary_id(0, 0x20);                    /* 0xd88f-0xd894 */
    uint8_t *node = g_dat_63350;                 /* 0xd8a7/0xd8ac */
    while (node != NULL) {                       /* 0xd8af/0xd8b3 */
        fn_0003c7d7(node);                       /* 0xd8b8 */
        node = *(uint8_t **)(node + 0x34c);      /* 0xd8c0/0xd8c6 */
    }
    return 0;                                     /* 0xd8cb (EAX = null next) */
}

uint8_t *g_dat_639cc = NULL;  /* DAT_000639cc: doubly-linked list head */
uint8_t *g_dat_639d0 = NULL;  /* DAT_000639d0: doubly-linked list tail */
uint8_t *g_dat_639d4 = NULL;  /* DAT_000639d4: drain cursor (fn_03b9b7) */
int32_t  g_dat_639c8 = 0;     /* DAT_000639c8: list node count */

/* fn_0003b5c0  orig: 0x0003b5c0 (L10, 189B)
 *   Linked-list INSERT (register-ABI). The node is caller-allocated and passed
 *   in EAX; its payload fields come from the other live registers/stack:
 *   node+0 = flag (stack byte), node+1 = ECX, node+5 = 0, node+9 = EDX,
 *   node+0xd = EBX. Then the node is appended: if the list is empty it becomes
 *   both head and tail (next=prev=0), else tail->next = node, node->prev =
 *   tail, node->next = 0, tail = node. DAT_000639c8 (count) is incremented.
 *   Returns the node.
 *   POINTER-WIDTH ADAPTATION: the original node is 24 bytes with 4-byte
 *   next@+0x11 / prev@+0x15 (32-bit pointers). The flat port is 64-bit, so the
 *   next/prev fields are widened to 8 bytes (next@+0x11, prev@+0x19) -- a
 *   necessary 32->64-bit adaptation; the payload fields keep their offsets. */
uint8_t *fn_0003b5c0(uint8_t *node, uint8_t flag, uint32_t d_ecx, uint32_t d_edx, uint32_t d_ebx) {
    node[0] = flag;
    *(uint32_t *)(node + 0x1) = d_ecx;
    *(uint32_t *)(node + 0x5) = 0;
    *(uint32_t *)(node + 0x9) = d_edx;
    *(uint32_t *)(node + 0xd) = d_ebx;
    if (g_dat_639cc == NULL) {
        g_dat_639cc = node;
        g_dat_639d0 = node;
        *(uint8_t **)(node + 0x19) = NULL;
        *(uint8_t **)(node + 0x11) = NULL;
    } else {
        *(uint8_t **)(g_dat_639d0 + 0x11) = node;
        *(uint8_t **)(node + 0x19) = g_dat_639d0;
        *(uint8_t **)(node + 0x11) = NULL;
        g_dat_639d0 = node;
    }
    g_dat_639c8++;
    return node;
}

/* fn_0003b67e  orig: 0x0003b67e (L10, 153B)
 *   Linked-list REMOVE (register-ABI, node in EAX). If node is head, head =
 *   node->next and node->next->prev = 0; if node is tail, tail = node->prev and
 *   node->prev->next = 0; else splice (prev->next = next, next->prev = prev).
 *   DAT_000639c8 is decremented. Returns the node. next@+0x11, prev@+0x19
 *   (widened to 8 bytes, see fn_0003b5c0).
 *   LATENT BUG (faithfully reproduced): the head/tail branches do not NULL-check
 *   the successor/predecessor, so removing the last remaining node null-derefs
 *   (writes next->prev through next==0, or prev->next through prev==0). The game
 *   presumably never removes the final node via this path. */
uint8_t *fn_0003b67e(uint8_t *node) {
    uint8_t *next = *(uint8_t **)(node + 0x11);
    uint8_t *prev = *(uint8_t **)(node + 0x19);
    if (g_dat_639cc == node) {
        g_dat_639cc = next;
        *(uint8_t **)(next + 0x19) = NULL;   /* latent null-deref if next == NULL */
    } else if (g_dat_639d0 == node) {
        g_dat_639d0 = prev;
        *(uint8_t **)(prev + 0x11) = NULL;   /* latent null-deref if prev == NULL */
    } else {
        *(uint8_t **)(prev + 0x11) = next;
        *(uint8_t **)(next + 0x19) = prev;
    }
    g_dat_639c8--;
    return node;
}

/* 0x03b9b7 (113 B): drain the entity linked list -- remove and free every node.
 * reg-ABI: no args (operates on globals). canary_id(0, 0x28). Disassembly-verified
 * (41 insns; only direct CALLs to 0x0418ee/0x03b67e/0x041c28, no indirect CALL/JMP):
 *   g_dat_639d4 = g_dat_639cc;
 *   while (g_dat_639d4 != 0) {
 *       next = *(uint8_t**)(g_dat_639d4 + 0x11);   // entity next (host ptr)
 *       fn_0003b67e(g_dat_639d4);                  // remove from list
 *       heap_free_guard(off(g_dat_639d4));         // 0x041c28; off = ptr - g_heap_base
 *       g_dat_639d4 = next;
 *   }
 * Entity nodes are host pointers (= g_heap_base + offset, cf. fn_0003b5c0 callers);
 * heap_free_guard takes a heap offset, so convert.
 * TESTING LIMITATION: the drain is untestable past an empty list because
 * fn_0003b67e's head branch NULL-derefs next->prev when removing the last node
 * (documented latent bug in fn_0003b67e); the game never drains to empty here. */
void fn_03b9b7(void) {
    (void)canary_id(0, 0x28);                                  /* PUSH 0x28;CALL 0x418ee */
    g_dat_639d4 = g_dat_639cc;                                  /* 0x3b9cf/d4 */
    while (g_dat_639d4 != NULL) {
        uint8_t *next = *(uint8_t **)(g_dat_639d4 + 0x11);      /* 0x3b9e7 */
        fn_0003b67e(g_dat_639d4);                               /* 0x3ba00 remove */
        uint32_t off = (uint32_t)((uint8_t *)g_dat_639d4 - (uint8_t *)g_heap_base);
        heap_free_guard(off);                                    /* 0x3ba05 (0x041c28) */
        g_dat_639d4 = next;                                      /* 0x3ba1e */
    }
}

uint8_t *g_dat_53c78 = NULL;  /* DAT_00053c78: entity linked-list cursor */
uint8_t  g_dat_53ea8[0x88 * 4]; /* DAT_00053ea8: 0x88-byte entry table (idx -> first dword copied to node+0x32) */

/* fn_00035c5c  orig: 0x00035c5c (L10, 178B)
 *   Entity-list init pass. Walks the entity linked list (head=DAT_00053c70,
 *   cursor=DAT_00053c78, next@+0x13f). For each node whose flag at +0 is 1 and
 *   whose flag at +0x30 is not yet 1: set +0x30 = 1, +0x17 = 0x15 (21), +0x11 =
 *   0, and +0x32 = *(DAT_00053ea8 + *(node+0xd) * 0x88) (first dword of the
 *   0x88-byte table entry selected by the node's index field). No-op when the
 *   list is empty (the flat model's state before data load).
 *   POINTER-WIDTH ADAPTATION: next@+0x13f widened to an 8-byte read. */
void fn_00035c5c(void) {
    g_dat_53c78 = g_dat_53c70;
    while (g_dat_53c78 != NULL) {
        uint8_t *next = *(uint8_t **)(g_dat_53c78 + 0x13f);
        if (g_dat_53c78[0] == 1 && g_dat_53c78[0x30] != 1) {
            g_dat_53c78[0x30] = 1;
            *(uint32_t *)(g_dat_53c78 + 0x17) = 0x15;
            *(uint32_t *)(g_dat_53c78 + 0x11) = 0;
            uint32_t idx = *(uint32_t *)(g_dat_53c78 + 0xd);
            *(uint32_t *)(g_dat_53c78 + 0x32) = *(uint32_t *)(g_dat_53ea8 + idx * 0x88);
        }
        g_dat_53c78 = next;
    }
}

/* fn_013aad  orig: 0x00013aad (L10, 415B)
 *   Entity-list bubble-sort: sort the doubly-linked entity list ascending by
 *   node->+0x9 (int32 key), relinking nodes in place and maintaining head
 *   (g_dat_53c70) + tail (g_dat_53c74). g_dat_53c7c = special-node marker
 *   (stable-sort tie-breaker). No callees (canary only).
 *   Node layout (real C pointers; POINTER-WIDTH ADAPTATION): +0x0 type(byte),
 *   +0x9 key(int32), +0x13f next(8-byte ptr, widened from 4), +0x147 prev
 *   (8-byte ptr; orig 0x143, shifted past the 8-byte next). */
void fn_013aad(uint32_t a0) {
    (void)a0;
    for (int32_t i = 0; ; i++) {
        if (g_dat_53c68 - 1 <= i) break;                 /* 0x13ad7 */
        uint8_t *cur = g_dat_53c70;                      /* 0x13ae6 */
        for (int32_t j = 0; ; j++) {
            if (g_dat_53c68 - i - 1 <= j) break;         /* 0x13afd */
            uint8_t *nxt = *(uint8_t **)(cur + 0x13f);   /* 0x13b12 */
            int32_t ck = *(int32_t *)(cur + 0x9);
            int32_t nk = *(int32_t *)(nxt + 0x9);
            if (ck < nk) { cur = nxt; continue; }        /* 0x13b21 */
            if (nxt == g_dat_53c7c && nk == ck) { cur = nxt; continue; }  /* 0x13b4e */
            if (nxt[0] == cur[0] && nk == ck) { cur = nxt; continue; }    /* 0x13b7b */
            /* full swap (0x13b8c): */
            uint8_t *nxt_next = *(uint8_t **)(nxt + 0x13f);
            uint8_t *cur_prev = *(uint8_t **)(cur + 0x147);
            if (nxt_next) *(uint8_t **)(nxt_next + 0x147) = cur;   /* 0x13ba4 */
            if (cur_prev) *(uint8_t **)(cur_prev + 0x13f) = nxt;   /* 0x13bc2 */
            *(uint8_t **)(cur + 0x13f) = nxt_next;                 /* 0x13bd4 */
            *(uint8_t **)(nxt + 0x147) = cur_prev;                 /* 0x13be6 */
            *(uint8_t **)(cur + 0x147) = nxt;                      /* 0x13bf2 */
            *(uint8_t **)(nxt + 0x13f) = cur;                      /* 0x13bfe */
            if (*(uint8_t **)(nxt + 0x147) == NULL) g_dat_53c70 = nxt;  /* 0x13c04 */
            if (*(uint8_t **)(cur + 0x13f) == NULL) g_dat_53c74 = cur;  /* 0x13c18 */
        }
    }
}

/*
 * 0x00047f54: fixed-point normalize (register-ABI). Inputs EAX:EDX = 64-bit
 * fixed-point value (EAX=low, EDX=high); EBX = 12-byte output pointer
 * { uint32 lo, uint32 hi, uint16 field }. Returns void.
 *
 * Disasm trace:
 *   ecx=EDX; (EDX:EAX)<<=11 (shld edx,eax,11 + shl eax,11); ecx=(int32)EDX>>20 (sar);
 *   cx=ecx&0x7ff (11-bit exponent candidate). Branches:
 *     cx==0x7ff: cx=0x7fff; if value nonzero -> (spurious 0x0463dc call, EAX
 *                saved+restored -> no-op) edx|=0x40000000; edx|=0x80000000.
 *     0<cx<0x7ff: cx+=0x3c00 (bias); edx|=0x80000000.
 *     cx==0 & value==0: ecx=0 (zero result).
 *     cx==0 & value!=0 (subnormal): cx=0x3c01; if edx==0 { xchg eax,edx; cx-=0x20 };
 *        then left-shift (edx:eax) by 1 (with carry) until edx bit31 set, cx-- each.
 *   out[0]=eax; out[1]=edx; cf=(ecx>>31)&1 (add ecx,ecx); out[2]=(cx<<1)|cf (rcr cx,1).
 * All cx values stay < 0x8000 so cf is always 0. The 0x0463dc call is spurious data
 * in the 0x0461cc interpreter region -- its EAX return is popped/discarded, so it is
 * a no-op standin here (documented approximation of the spurious call).
 * Verified differential against scratch/fp_047f54_ref.py (2009 vectors).
 */
void fn_047f54(uint32_t lo_in, uint32_t hi_in, uint32_t *out)
{
    uint32_t ecx = hi_in;                             /* mov ecx,edx */
    uint64_t v = ((uint64_t)hi_in << 32) | lo_in;
    v <<= 11;                                         /* shld edx,eax,11; shl eax,11 */
    uint32_t eax = (uint32_t)v;
    uint32_t edx = (uint32_t)(v >> 32);
    ecx = (uint32_t)((int32_t)ecx >> 20);             /* sar ecx,20 */
    uint16_t cx = (uint16_t)(ecx & 0x7ff);            /* and cx,0x7ff */
    if (cx == 0x7ff) {
        cx = 0x7fff;
        if (edx != 0 || eax != 0) {
            edx |= 0x40000000;                         /* 0x0463dc call no-op'd (EAX restored) */
        }
        edx |= 0x80000000;
    } else if (cx != 0) {
        cx = (uint16_t)(cx + 0x3c00);
        edx |= 0x80000000;
    } else if (edx != 0 || eax != 0) {
        cx = 0x3c01;
        if (edx == 0) { uint32_t t = eax; eax = edx; edx = t; cx = (uint16_t)(cx - 0x20); }
        while (!(edx & 0x80000000)) {
            uint32_t cf = (eax >> 31) & 1;
            eax = (eax << 1) & 0xffffffff;
            edx = ((edx << 1) + cf) & 0xffffffff;
            cx = (uint16_t)(cx - 1);
        }
    } else {
        ecx = 0;                                        /* sub ecx,ecx (zero result) */
    }
    out[0] = eax;
    out[1] = edx;
    uint32_t cf = (ecx >> 31) & 1;
    out[2] = (uint16_t)(((cx << 1) | cf) & 0xffff);
}

/* fn_048053  orig: 0x00048053 (L1, 74 insns)
 *   48-bit fixed-point -> double normalizer (register-ABI). Input: 12-byte
 *   struct { f0 u32 @0, f1 u32 @4, f2 u16 @8 } (a fixed-point value, at
 *   [EDI+EBP+0x1c] in the caller). Returns a 64-bit (EDX:EAX) IEEE-754-ish
 *   representation; callers store EAX->low, EDX->high. Called from the
 *   0x0461cc interpreter (0x046a1e / 0x046a51).
 *   The single CALL 0x0463dc (mid-range branch, side effects only, EAX
 *   save/restore) is a net no-op: 0x0463dc is a dispatcher that copies a
 *   27-dword state buffer, calls [0x533bd]=0x042a07 (a no-op primitive), and
 *   copies it back -- no observable effect, so no-op'd here (same treatment
 *   as fn_047f54's identical spurious call).
 *   Faithful instruction emulation; only the four live carries are tracked:
 *   SHL EBX,22; ADD EAX,0x800; ADC EDX,0; ADD BX,BX->RCR EDX,1.
 *   Differential vs scratch/fp_048053_ref.py (2,000,000 random vectors). */
uint64_t fn_048053(uint32_t f0, uint32_t f1, uint16_t f2)
{
    uint32_t eax = f0, edx = f1, ecx = f2, esi = 0xfffff800, ebx = f0;
    /* SHL EBX,22 */
    uint32_t cf1 = (eax >> 10) & 1;              /* f0 bit 10 */
    ebx = (eax << 22) & 0xffffffff;
    uint32_t z_shl = (ebx == 0);
    if (cf1) {
        if (z_shl) esi = (esi + esi) & 0xffffffff;   /* ADD ESI,ESI */
        uint32_t s = eax + 0x800;                    /* ADD EAX,0x800 */
        uint32_t cf2 = (s < eax) ? 1 : 0;
        eax = s;
        uint32_t s2 = edx + cf2;                     /* ADC EDX,0 */
        uint32_t cf3 = (s2 < edx) ? 1 : 0;
        edx = s2;
        if (cf3) {
            edx = 0x80000000;                         /* 0x48079 */
            ecx = (ecx + 1) & 0xffff;                 /* INC CX */
        }
    }
    eax = eax & esi;                                   /* AND EAX,ESI */
    ebx = ecx;                                         /* MOV EBX,ECX (pre-bias) */
    ecx &= 0x7fff;                                     /* AND CX,0x7fff */
    ecx = (ecx + 0xc400) & 0xffff;                     /* ADD CX,0xc400 */
    uint32_t cx = ecx;
    if (cx < 0x7ff) {                                  /* small exponent */
        if (cx == 0) {
            uint64_t p = (((uint64_t)edx << 32) | eax) >> 12;  /* SHRD EAX,EDX,12 */
            eax = (uint32_t)p;
            edx = (edx << 1) & 0xffffffff;            /* ADD EDX,EDX */
            edx >>= 12;                               /* SHR EDX,12 */
        } else {
            uint64_t p = (((uint64_t)edx << 32) | eax) >> 11;  /* SHRD EAX,EDX,11 */
            eax = (uint32_t)p;
            edx = (edx << 1) & 0xffffffff;            /* ADD EDX,EDX */
            p = (((uint64_t)ecx << 32) | edx) >> 11;  /* SHRD EDX,ECX,11 */
            edx = (uint32_t)p;
        }
        uint32_t bx = ebx & 0xffff;
        uint32_t cf4 = (bx >> 15) & 1;                /* ADD BX,BX carry */
        edx = ((edx << 1) | cf4) & 0xffffffff;        /* RCR EDX,1 */
    } else if (cx < 0xc400) {                         /* mid range (0x480f0) */
        uint64_t p = (((uint64_t)edx << 32) | eax) >> 11;  /* SHRD EAX,EDX,11 */
        eax = (uint32_t)p;
        edx = (edx << 1) & 0xffffffff;                /* ADD EDX,EDX */
        edx >>= 11;                                   /* SHR EDX,11 */
        uint32_t bx = ebx & 0xffff;
        uint32_t cf4 = (bx >> 15) & 1;
        edx = ((edx << 1) | cf4) & 0xffffffff;        /* RCR EDX,1 */
        edx |= 0x7ff00000;                            /* OR EDX,0x7ff00000 */
        /* CALL 0x463dc no-op (handler 0x42a07 is a no-op primitive) */
    } else if (cx < 0xffcc) {                         /* zero (0x480ea) */
        eax = 0;
        edx = 0;
    } else {                                          /* large range (0x480c3) */
        cx = (cx - 0xc) & 0xffff;                     /* SUB CX,0xc */
        cx = (-cx) & 0xffff;                          /* NEG CX (16-bit) */
        uint32_t cl = cx & 0xff;
        if (cl >= 0x20) {                             /* CMP CL,0x20; JC not taken */
            cl = (cl - 0x20) & 0xff;
            esi = eax;
            eax = edx;
            edx = 0;
        }
        uint64_t p = (((uint64_t)eax << 32) | esi) >> cl;  /* SHRD ESI,EAX,cl */
        esi = (uint32_t)p;
        p = (((uint64_t)edx << 32) | eax) >> cl;      /* SHRD EAX,EDX,cl */
        eax = (uint32_t)p;
        edx >>= cl;                                   /* SHR EDX,cl */
        uint32_t cf5 = (esi >> 31) & 1;               /* ADD ESI,ESI carry */
        esi = (esi << 1) & 0xffffffff;
        uint32_t s = eax + cf5;                       /* ADC EAX,0 */
        uint32_t cf5b = (s < cf5) ? 1 : 0;
        eax = s;
        edx = edx + cf5b;                             /* ADC EDX,0 */
    }
    return ((uint64_t)edx << 32) | eax;
}

/* ---- IRQ 0x1c / IRQ 9 installers + timer-DMA state (fn_03ef29 / fn_03feb8) ---- */
uint8_t  g_dat_63e88[0x80]; /* DAT_00063e88: 128-byte DMA/channel status (zeroed on IRQ9 install) */
uint32_t g_dat_63f4c = 0;   /* DAT_00063f4c: -1 sentinel (set to 0xffffffff on IRQ9 install) */

/* fn_03ef29  orig: 0x03ef29
 *   Install IRQ 0x1c (timer) handler (complement of fn_03eff2, which uninstalls).
 *   `timer_value` is passed in EAX (register-ABI; canary prologue preserves it).
 *   Skips if g_dat_63e4a already set. Stores the divisor in g_dat_63e2c, clears
 *   g_dat_63e30, saves the current ISR-0x1c (isr_get) into g_dat_63e44(off)/63e48(seg),
 *   installs the new vector (isr_set with CS = DPMI flat code selector), programs
 *   PIT ch0 (mode 0x36 + 16-bit divisor), sets the flag, and STI. */
void fn_03ef29(uint32_t timer_value)
{
    if (g_dat_63e4a != 0) return;                    /* 0x3ef33-3f */
    g_dat_63e2c = (uint16_t)timer_value;             /* 0x3ef42-4a */
    g_dat_63e30 = 0;                                 /* 0x3ef4c-54 */
    uint64_t isr = isr_get(0x1c);                    /* 0x3ef57-62 (EAX=off, EDX=seg) */
    g_dat_63e48 = (uint16_t)(isr >> 32);             /* 0x3ef68-6f [63e48]=seg */
    g_dat_63e44 = (uint32_t)isr;                     /* 0x3ef72-77 [63e44]=off */
    isr_set(0x1c, 0x3ee88, 0x1f);                    /* 0x3ef7a-93 (CS=flat selector) */
    outb(0x43, 0x36);                                /* 0x3ef96-a1 PIT ch0 mode */
    outb(0x40, (uint8_t)timer_value);                /* 0x3efa4-b6 ch0 divisor low */
    outb(0x40, (uint8_t)(timer_value >> 8));         /* 0x3efb9-c4 ch0 divisor high */
    g_dat_63e4a = 1;                                 /* 0x3efc7 */
    irq_sti();                                       /* 0x3efca-d1 */
}

/* fn_03feb8  orig: 0x03feb8
 *   Install IRQ 9 handler (complement of fn_03ffab) and initialize the timer/DMA
 *   channel state. Skips if g_dat_63e86 already set. Saves ISR-9 into
 *   g_dat_63e80(off)/63e84(seg), installs the vector (isr_set CS), zero-initializes
 *   the DMA status block 0x63e88 (0x80 B), slot tables 0x63f14 (0xa B, =0) /
 *   0x63f1e (0xa B, =1) / 0x63f28 (0x28 B, =0), resets the slot base
 *   (63f0c=0 -> 63f08=0 -> 63f50=0), sets 63f4c=-1, sets the flag, mirrors the
 *   low-mem byte 0x417 into 63f51, then clears bit 0x20 of 0x417. */
void fn_03feb8(void)
{
    if (g_dat_63e86 != 0) return;                    /* 0x3fec7-d3 */
    uint64_t isr = isr_get(9);                       /* 0x3fed5-e2 (EAX=off, EDX=seg) */
    g_dat_63e84 = (uint16_t)(isr >> 32);             /* 0x3fee5-ec [63e84]=seg */
    g_dat_63e80 = (uint32_t)isr;                     /* 0x3feef-f4 [63e80]=off */
    isr_set(9, 0x3fd40, 0x1f);                       /* 0x3fef1-18 (CS=flat selector) */
    fn_041e20(g_dat_63e88, 0, 0x80);                 /* 0x3fd1b-2c memset(63e88,0,0x80) */
    fn_041e20(g_dat_63f14, 0, 0xa);                  /* memset(63f14,0,0xa) */
    fn_041e20(g_dat_63f1e, 1, 0xa);                  /* memset(63f1e,1,0xa) */
    fn_041e20((uint8_t *)g_dat_63f28, 0, 0x28);      /* memset(63f28,0,0x28) */
    g_dat_63f0c = 0;                                 /* 0x3fd.. */
    g_dat_63f08 = g_dat_63f0c;                       /* [63f08] = [63f0c] = 0 */
    g_dat_63f50 = (uint8_t)g_dat_63f08;              /* [63f50] = byte 63f08 = 0 */
    g_dat_63f4c = 0xffffffffu;                       /* [63f4c] = -1 */
    g_dat_63e86 = 1;                                 /* installed */
    g_dat_63f51 = g_dat_00417;                       /* [63f51] = byte at 0x417 */
    g_dat_00417 &= 0xdf;                             /* clear bit 0x20 of 0x417 */
}

/* fn_0485eb  orig: 0x0485eb
 * 64-bit fixed-point align-subtract-normalize (register-ABI). Faithful asm-mirror.
 * in : eax=A_lo, edx=A_hi, ebx=B_lo, ecx=B_hi, esi=expA, edi=expB.
 * out: lo=EAX, hi=EDX, exp=ESI, ebx=EBX(=expA&0x8000), bx=BX(=(expA&0x8000)^(expB&0xffff)),
 *      edi=EDI (neg flag; NEG EDI if bit7(bx) set).
 * Verified differentially against scratch/fp_0485eb_ref.py (469 vectors). */
void fn_0485eb(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi,
               uint32_t expA, uint32_t expB, fx_sub_t *out)
{
    uint32_t eax = a_lo, edx = a_hi, ebx = b_lo, ecx = b_hi;
    uint32_t esi = expA, edi = expB;
    uint32_t esi0 = expA, edi0 = expB;
    uint32_t cf = 0;

    esi &= 0x7fff;                       /* AND ESI,0x7fff */
    edi &= 0x7fff;                       /* AND EDI,0x7fff */
    cf = esi < edi; esi -= edi;          /* SUB ESI,EDI */
    if ((int32_t)esi < 0) goto L_04864a; /* JL 0x04864a */
    edi = 0;                             /* SUB EDI,EDI */
L_0485ff:
    cf = ecx < edx;                      /* CMP ECX,EDX */
    if (ecx != edx) goto L_04860b;       /* JNZ 0x04860b */
    cf = ebx < eax;                      /* CMP EBX,EAX */
    if (ebx == eax) goto L_04860d;       /* JZ 0x04860d */
L_04860b:
    if (!cf) goto L_048612;              /* JNC 0x048612 */
L_04860d:
    cf = eax < ebx; eax -= ebx;          /* SUB EAX,EBX */
    { uint32_t prev = edx; uint32_t sub = ecx + cf; cf = prev < sub; edx -= sub; } /* SBB EDX,ECX */
    cf = 1;                              /* STC */
L_048612:
    { uint64_t v = (uint64_t)edi * 2 + cf; cf = v > 0xFFFFFFFFull; edi = (uint32_t)v; } /* ADC EDI,EDI */
    esi -= 1;                            /* DEC ESI */
    if ((int32_t)esi < -1) goto L_048627;/* JL 0x048627 */
    { uint64_t v = (uint64_t)eax * 2; cf = v > 0xFFFFFFFFull; eax = (uint32_t)v; } /* ADD EAX,EAX */
    { uint64_t v = (uint64_t)edx * 2 + cf; cf = v > 0xFFFFFFFFull; edx = (uint32_t)v; } /* ADC EDX,EDX */
    if (cf) goto L_04860d;               /* JC 0x04860d */
    if (edx & 0x80000000u) goto L_0485ff;/* OR EDX,EDX; JS 0x0485ff */
    goto L_048612;                        /* JMP 0x048612 */
L_048627:
    esi = 0;                             /* SUB ESI,ESI */
    if (eax) goto L_048633;              /* OR EAX,EAX; JNZ 0x048633 */
    if (edx) goto L_048633;              /* OR EDX,EDX; JNZ 0x048633 */
    goto L_048648;
L_048633:
    if (edx & 0x80000000u) goto L_04863e;/* OR EDX,EDX; JS 0x04863e */
    { uint64_t v = (uint64_t)eax * 2 + cf; cf = v > 0xFFFFFFFFull; eax = (uint32_t)v; } /* ADC EAX,EAX */
    { uint64_t v = (uint64_t)edx * 2 + cf; cf = v > 0xFFFFFFFFull; edx = (uint32_t)v; } /* ADC EDX,EDX */
    esi -= 1;                            /* DEC ESI */
    goto L_048633;
L_04863e:
    ecx = edi0;                          /* POP ECX (saved edi0) */
    ecx &= 0x7fff;                       /* AND ECX,0x7fff */
    esi += ecx;                          /* ADD ESI,ECX */
L_048648:
    goto L_04864e;
L_04864a:
    esi += edi;                          /* ADD ESI,EDI */
    edi = 0;                             /* SUB EDI,EDI */
L_04864e:
    ecx = edi0;                          /* POP ECX */
    ebx = esi0 & 0x8000u;                /* POP EBX; AND EBX,0x8000 */
    esi |= ebx;                          /* OR ESI,EBX */
    if (edx == 0) esi = 0;               /* OR EDX,EDX; JNZ 0x04865e; else SUB ESI,ESI */
    {
        uint16_t bx = (uint16_t)(ebx & 0xFFFFu) ^ (uint16_t)(ecx & 0xFFFFu); /* XOR BX,CX */
        if (bx & 0x80) edi = (uint32_t)(-(int32_t)edi); /* JNS 0x048665; NEG EDI */
        out->bx = bx;
    }
    out->lo  = eax;
    out->hi  = edx;
    out->exp = esi;
    out->ebx = ebx;
    out->edi = (int32_t)edi;
}

/* fn_048585  orig: 0x048585
 * Thin wrapper over fn_0485eb. a,b are fx128 {lo@0,hi@4,exp@8(word)}.
 * if a==0 (lo,hi,exp all 0) -> return 0. if b==0 -> *a=0, return 0.
 * else EAX=a.lo,EDX=a.hi,EBX=b.lo,ECX=b.hi,SI=a.exp,DI=b.exp; CALL 0x0485eb;
 * a.lo=r.lo, a.hi=r.hi, a.exp=(uint16_t)r.exp; return EDI (r.edi). */
uint32_t fn_048585(fx128 *a, fx128 *b)
{
    if (a->exp == 0 && a->hi == 0 && a->lo == 0) return 0;      /* 0x048585..0x048599 */
    if (b->exp == 0 && b->hi == 0 && b->lo == 0) {              /* 0x04859a..0x0485b9 */
        a->lo = 0; a->hi = 0; a->exp = 0;
        return 0;
    }
    fx_sub_t r;
    fn_0485eb(a->lo, a->hi, b->lo, b->hi, a->exp, b->exp, &r);  /* 0x0485ba..0x0485d3 */
    a->lo = r.lo; a->hi = r.hi; a->exp = (uint16_t)r.exp;       /* 0x0485d9..0x0485de */
    return (uint32_t)r.edi;                                     /* 0x0485e2: EAX=EDI */
}

/* fn_048e60  orig: 0x048e60
 * 128-bit fixed-point normalize + Newton-Raphson reciprocal division (L0 leaf, pure).
 * Input via p (uint32_t*): p[0]=low mantissa, p[1]=high mantissa (top byte = status flags),
 * p[2] low16 = exponent field. On the compute path it normalizes the 64-bit mantissa
 * (tracking bits shifted out into a 128-bit low half), divides by the reciprocal estimate
 * via a Newton iteration, and writes back p[0]/p[1] (64-bit result) + p[2] (exponent).
 * Guard/overflow paths zero the value and OR 0xc0 into the status byte (offset 7 = top byte of p[1]).
 * P-code CARRY4/CONCAT44/CONCAT14 translated to 64-bit integer ops; u2 wraps as a 16-bit field.
 * Verified differentially against an independent reference (scratch/fp_048e60.py). */
void fn_048e60(uint32_t *p)
{
    uint16_t u2 = (uint16_t)p[2];      /* exponent field (low16 of p[2]) */
    uint32_t u5 = p[0];                 /* low mantissa */
    uint32_t u3 = 0, u4 = 0, u7 = 0, u1 = 0, b8 = 0;
    int32_t  i6 = 0;

    if (u5 == 0) {
        if (p[1] != 0) {
            if (u2 == 0x7fff) return;
            /* else: fall into MAIN */
        } else {
            if ((u2 & 0x7fff) == 0) return;
            goto fallthrough;
        }
    }
    /* MAIN */
    if (((u2 & 0x7fff) == 0x7fff) || ((u2 >> 15) & 1)) goto ovp;   /* check1: overflow */
    if (((u2 & 0x7fff) == 0) || ((p[1] >> 31) & 1)) {              /* check2 */
        u3 = p[1]; u7 = 0;
        u2 = (uint16_t)((u2 & 0x7fff) + 0xc001);
        u4 = u5;
        if ((u2 & 1) == 0) {                                        /* divide mantissa by 2 */
            u7 = u3 & 1;
            u3 = u3 >> 1;
            u4 = (uint32_t)(((((uint64_t)(u7 != 0)) << 32) | u5) >> 1);
            u7 = (uint32_t)((uint64_t)((u5 & 1) != 0) << 0x1f);
        }
        p[2] = (p[2] & 0xffff0000u) | (uint16_t)(((int16_t)u2 >> 1) + 0x3fff);
        u5 = (uint32_t)(((((uint64_t)u3) | 0x100000000ull) >> 1));
        if (u3 == 0xffffffffu) {                                     /* denom == -1 special case */
            i6 = -1;
            if (u4 == 0xffffffffu) goto done;
            u3 = (uint32_t)(((((uint64_t)u4) << 32) | u7) / 0xffffffffull);
            u5 = 0xffffffffu;
        } else {                                                     /* Newton-Raphson reciprocal */
            for (;;) {
                u1 = (uint32_t)(((((uint64_t)u3) << 32) | u4) / (uint64_t)u5);
                if ((u5 - 1u) <= u1) break;
                { uint32_t s = u5 + u1, c = (s < u5) ? 1u : 0u;
                  u5 = (uint32_t)(((((uint64_t)c) << 32) | s) >> 1); }
            }
            { uint64_t r = ((((uint64_t)u3) << 32) | u4) % (uint64_t)u5;
              u3 = (uint32_t)((((r << 0x20) | (uint64_t)u7) / (uint64_t)u5)); }
            u5 = u5 + u1;
        }
        i6 = (int32_t)(((uint64_t)u5 | 0x100000000ull) >> 1);
        u4 = (uint32_t)(((((uint64_t)((u5 & 1) != 0)) << 32) | u3) >> 1);
        u5 = (uint32_t)((u3 & 1) != 0);
        b8 = (((uint64_t)u4 + u5) >= 0x100000000ull) ? 1u : 0u;
        u4 = u4 + u5;
done:
        p[0] = u4;
        p[1] = (uint32_t)i6 + b8;
        return;
    }
    goto fallthrough;
fallthrough:
    p[2] = (p[2] & 0xffff0000u) | 0xffffu;
    p[1] = 0;
    p[0] = 0;
ovp:
    p[1] |= 0xc0000000u;                  /* status byte (offset 7) |= 0xc0 */
    return;
}

/* ---- CD/DPMI real-mode I/O cluster (fn_103e0 / fn_10570 + dependents) ----
 * These perform DPMI INT 0x31h real-mode CD/DOS I/O (via the no-op trampoline
 * fn_041e38) with register-ABI arguments. Their observable outputs depend on
 * caller register state (the canary-passed EAX) and the no-op I/O results, so
 * they have no deterministic flat-model effect -- documented no-op standins
 * (per the DPMI-trampoline convention, cf. fn_043381 / fn_04391d). */
void fn_103e0(void)   { /* 0x0103e0: DPMI INT 0x31 real-mode I/O wrapper (no-op). */ }
void fn_10570(const uint8_t *buf, uint32_t len) { (void)buf; (void)len; }   /* 0x010570: 0x100-byte DPMI real-mode read via 0x103e0 (no-op). */
/* DAT_00051a14: 24-byte real-mode serial/DMA command template (extracted from the binary). */
static const uint8_t g_serial_tpl_51a14[24] = {
    0x18,0x00,0x0c,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x01,0x00, 0x00,0x00,0x00,0x00
};
/* 0x0105f7: build a 24-byte real-mode serial/DMA command: copy the DAT_00051a14
 * template, patch bytes 20..23 from g_dat_53c5c (b20=g&0xf, b21=0, b22=(g>>4)&0xff,
 * b23=(g>>12)&0xff), write the informal AL arg into byte 0 of the buffer pointed to
 * by g_dat_53c5c (mem_copy(g_dat_53c5c,&al,1) -- g_dat_53c5c is a DPMI far-pointer),
 * then submit via fn_10570(buf,1) (no-op in the flat model). The only observable
 * effect is (*g_dat_53c5c) byte 0 = al (the 24-byte cmd is discarded by the no-op
 * callee). g_dat_53c5c must point at a valid buffer (0 -> *0 crash, as in the original). */
void fn_0105f7(uint8_t al) {
    (void)canary_id(0, 0x38);
    uint8_t buf[24];
    for (int i = 0; i < 24; i++) buf[i] = g_serial_tpl_51a14[i];
    buf[20] = (uint8_t)((uint16_t)g_dat_53c5c & 0xf);
    buf[21] = 0;
    buf[22] = (uint8_t)((g_dat_53c5c >> 4) & 0xff);
    buf[23] = (uint8_t)((g_dat_53c5c >> 12) & 0xff);
    mem_copy((uint8_t *)g_dat_53c5c, (const uint8_t *)&al, 1);   /* byte 0 of buffer @ g_dat_53c5c = al */
    fn_10570(buf, 1);                /* no-op in the flat model */
}
/* DAT_00051a58: 26-byte real-mode command template (extracted from the binary). */
static const uint8_t g_serial_tpl_51a58[26] = {
    0x1a,0x00,0x03,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x09,0x00, 0x00,0x00,0x00,0x00
};
/* 0x01072f: build a 26-byte real-mode command (copy the DAT_00051a58 template, patch
 * bytes 20..23 from the g_dat_53c5c pointer value), set g_dat_53c4c[0]=4, then
 * round-trip 9 bytes between the g_dat_53c4c staging area and the buffer pointed to by
 * g_dat_53c5c: first copy staging->*g_dat_53c5c, submit via fn_10570(cmd,9) (no-op),
 * then copy *g_dat_53c5c->staging back. Observable: g_dat_53c4c[0]=4 and *g_dat_53c5c
 * byte 0..8 = the staging area (with byte 0 = 4). The 26-byte cmd is discarded by the
 * no-op callee. g_dat_53c5c must point at a valid >=9-byte buffer (0 -> *0 crash). */
void fn_01072f(void) {
    (void)canary_id(0, 0x38);
    uint8_t cmd[26];
    for (int i = 0; i < 26; i++) cmd[i] = g_serial_tpl_51a58[i];
    cmd[20] = (uint8_t)((uint16_t)g_dat_53c5c & 0xf);
    cmd[21] = 0;
    cmd[22] = (uint8_t)((g_dat_53c5c >> 4) & 0xff);
    cmd[23] = (uint8_t)((g_dat_53c5c >> 12) & 0xff);
    g_dat_53c4c[0] = 4;                       /* MOV byte [0x53c4c], 4 */
    mem_copy((uint8_t *)g_dat_53c5c, g_dat_53c4c, 9);   /* staging -> *g_dat_53c5c */
    fn_10570(cmd, 9);                       /* no-op in the flat model */
    mem_copy(g_dat_53c4c, (const uint8_t *)g_dat_53c5c, 9);   /* *g_dat_53c5c -> staging */
}
/* 0x03e14c: 2-node doubly-linked prev/next manipulation (fields 0x51=prev, 0x55=next),
 * gated by a counter. a0 is a heap offset; a1 is stored but never read. If
 * g_dat_63e14==1: zero the three cache globals. Else: if g_dat_63e18==a0 -> cache a0's
 * prev and clear that prev's next; else if g_dat_63e1c==a0 -> cache a0's next and clear
 * that next's prev; else -> swap a0's prev<->next links (a 2-node rotation). Decrement
 * the counter; return a0. */
uint32_t fn_03e14c(uint32_t a0, uint32_t a1) {
    (void)canary_id(0, 0x24);
    (void)a1;   /* stored to [EBP-4], never read */
    uint8_t *hb = (uint8_t *)g_heap_base;
    if (g_dat_63e14 == 1) {
        g_dat_63e18 = 0;
        g_dat_63e1c = 0;
        g_dat_63e20 = 0;
    } else {
        if (g_dat_63e18 == a0) {
            g_dat_63e18 = *(uint32_t *)(hb + a0 + 0x51);
            *(uint32_t *)(hb + *(uint32_t *)(hb + a0 + 0x51) + 0x55) = 0;
        } else if (g_dat_63e1c == a0) {
            g_dat_63e1c = *(uint32_t *)(hb + a0 + 0x55);
            *(uint32_t *)(hb + *(uint32_t *)(hb + a0 + 0x55) + 0x51) = 0;
        } else {
            uint32_t n0x55 = *(uint32_t *)(hb + a0 + 0x55);   /* a0->next */
            uint32_t n0x51 = *(uint32_t *)(hb + a0 + 0x51);   /* a0->prev */
            *(uint32_t *)(hb + n0x55 + 0x51) = n0x51;         /* next->prev = prev */
            *(uint32_t *)(hb + n0x51 + 0x55) = n0x55;         /* prev->next = next */
        }
    }
    g_dat_63e14--;   /* DEC [0x63e14] (always) */
    return a0;
}

/* 0x03ee0a (L11, 123 B): destroy a prev-linked list.
 *   g_63e20 = g_63e18;
 *   while (g_63e20 != 0) {
 *       uint32_t next = node->0x51;              // read BEFORE mutating neighbors
 *       heap_free_guard(fn_03e14c(g_63e20, 0));  // fn_03e14c returns a0; guard frees it
 *       g_63e20 = next;
 *   }
 *   g_62a8c = 0;
 * fn_03e14c removes the node from the doubly-linked list (cache/swap) and the
 * guard frees the block; the walk follows the (pre-read) 0x51 prev links to 0. */
void fn_03ee0a(void) {
    (void)canary_id(0, 0x28);                                   /* PUSH 0x28;CALL 0x418ee */
    uint8_t *hb = (uint8_t *)g_heap_base;
    g_dat_63e20 = g_dat_63e18;                                  /* 0x3ee22/27 */
    for (;;) {
        if (g_dat_63e20 == 0) break;                            /* 0x3ee2c/33 */
        uint32_t next = *(uint32_t *)(hb + g_dat_63e20 + 0x51); /* 0x3ee35/3a */
        uint32_t p = g_dat_63e20;                               /* 0x3ee40/45 */
        if (p != 0)                                             /* 0x3ee48/4c */
            heap_free_guard(fn_03e14c(p, 0));                   /* 0x3ee4e/53/58 */
        g_dat_63e20 = next;                                     /* 0x3ee69/6c */
    }
    g_dat_62a8c = 0;                                            /* 0x3ee73 */
}
/* 0x01435f: find the first interval i (0..5) whose [w8..w10] straddles thr=a0->0x11
 * (w8 <= thr < w10), then return 0 iff (a1 == a0->0x17) && (a2 == i), else 1. a0 is a
 * heap offset; pointer chain: a0->0x123=P1, P1->0x4=P2 (an array of base offsets),
 * base = P2[a0->0x17 & 0x7fffffff]. base is a flat array with uint16 w8 at [i*4+8] and
 * uint16 w10 at [i*4+0xa] for i=0..5. a1 (byte) compared to a0->0x17 (raw 32-bit),
 * a2 (byte) compared to i. */
uint32_t fn_01435f(uint32_t a0, uint8_t a1, uint8_t a2) {
    (void)canary_id(0, 0x2c);
    uint8_t *hb = (uint8_t *)g_heap_base;
    uint32_t thr     = *(uint32_t *)(hb + a0 + 0x11);
    uint32_t idx_raw = *(uint32_t *)(hb + a0 + 0x17);   /* raw (return compare) */
    uint32_t idx     = idx_raw & 0x7fffffffu;           /* sign-cleared (base index) */
    uint32_t p1      = *(uint32_t *)(hb + a0 + 0x123);  /* a0->0x123 */
    uint32_t p2      = *(uint32_t *)(hb + p1 + 0x4);    /* P1->0x4 */
    uint32_t base    = *(uint32_t *)(hb + p2 + idx * 4);/* P2[idx] */
    int32_t i = 0;
    for (;;) {
        if (i >= 6) break;
        uint16_t w8 = *(uint16_t *)(hb + base + i * 4 + 8);
        if (w8 > (uint16_t)thr) { i++; continue; }        /* 0x143cb JG -> continue */
        uint16_t w10 = *(uint16_t *)(hb + base + i * 4 + 0xa);
        if (w10 > (uint16_t)thr) break;                    /* 0x143e3 JG -> break (i stays) */
        i++;                                               /* continue */
    }
    if ((uint32_t)a1 == idx_raw && (uint32_t)a2 == (uint32_t)i) return 0;
    return 1;
}
/* 0x031f5f: remove the first element of a0's array (4-byte slots at a0+0x32): save
 * slot0 (a0->0x32) to a0->0x17, shift the rest left by one slot (slot[i]=slot[i+1] for
 * i in 0..count-2), and decrement the count (a0->0x31, byte). Then via P1=a0->0x123,
 * P2=P1->0x4, base=P2[a0->0x17 & 0x7fffffff]: if base->0x69 != 0, set g_dat_63364=
 * base->0x69 (byte) and g_dat_63368=base->0x6a (word). a0->0x11 = 0. NOTE: the leading
 * JLE on the zero-extended count byte is dead (never <= 0), so count==0 underflows to
 * 255 and shifts 255 slots (latent bug, not exercised in the test). */
void fn_031f5f(uint32_t a0) {
    (void)canary_id(0, 0x28);
    uint8_t *hb = (uint8_t *)g_heap_base;
    uint8_t *a = hb + a0;
    a[0x31]--;                                            /* a0->0x31-- (byte, wraps 0->255) */
    *(uint32_t *)(a + 0x17) = *(uint32_t *)(a + 0x32);    /* a0->0x17 = a0->0x32 (saved slot0) */
    uint32_t idx  = *(uint32_t *)(a + 0x17) & 0x7fffffffu;
    uint32_t p1   = *(uint32_t *)(a + 0x123);             /* P1 */
    uint32_t p2   = *(uint32_t *)(hb + p1 + 0x4);         /* P1->0x4 */
    uint32_t base = *(uint32_t *)(hb + p2 + idx * 4);     /* P2[idx] */
    uint8_t *b = hb + base;
    if (b[0x69] != 0) {
        g_dat_63364 = b[0x69];                            /* byte -> int32 */
        g_dat_63368 = *(uint16_t *)(b + 0x6a);            /* word -> int32 */
    }
    *(uint32_t *)(a + 0x11) = 0;                          /* a0->0x11 = 0 */
    int32_t cnt = (int32_t)a[0x31];                       /* count after DEC */
    for (int32_t i = 0; i < cnt; i++)
        *(uint32_t *)(a + 0x32 + i * 4) = *(uint32_t *)(a + 0x36 + i * 4);  /* slot[i]=slot[i+1] */
}
/* 0x03d67c: move element a2 to index a1 of a 3-byte-stride array at a0, shifting the
 * range [a1, a2-1] right by one. Saves element[a2] (3 bytes) to locals, then for
 * i = a2-1 down to a1: element[i+1] = element[i] (right-shift, high->low so the source
 * is read before the dest is overwritten), then element[a1] = the saved element.
 * a0 is a heap offset (base at g_heap_base + a0); a1 < a2. */
void fn_03d67c(uint32_t a0, uint32_t a1, uint32_t a2) {
    (void)canary_id(0, 0x30);
    uint8_t *a = (uint8_t *)g_heap_base + a0;
    uint8_t b0 = a[a2 * 3 + 0];   /* save element[a2] */
    uint8_t b1 = a[a2 * 3 + 1];
    uint8_t b2 = a[a2 * 3 + 2];
    for (int32_t i = (int32_t)a2 - 1; i >= (int32_t)a1; i--)
        for (int k = 0; k < 3; k++)
            a[(i + 1) * 3 + k] = a[i * 3 + k];   /* element[i+1] = element[i] (stride 3) */
    a[a1 * 3 + 0] = b0;   /* element[a1] = saved element */
    a[a1 * 3 + 1] = b1;
    a[a1 * 3 + 2] = b2;
}

/* 0x03d606 (L11, 117 B): index-driven reorder. a0 is a g_heap_base heap offset;
 * struct: count@0x3c (int32), pairs_off@0x44 (heap offset to 2-byte pairs), and a
 * 3-byte-stride data array inline at a0+0x48. Loop i in 0..count-1: a1 = pairs[i*2],
 * a2 = pairs[i*2+1]; fn_03d67c(a0+0x48, a1, a2). (orig args: EAX=a0+0x48 base,
 * EDX=a1, EBX=a2 -> fn_03d67c(a0=EAX, a1=EDX, a2=EBX).) */
void fn_03d606(uint32_t a0) {
    (void)canary_id(0, 0x24);                                   /* PUSH 0x24;CALL 0x418ee */
    uint8_t *hb = (uint8_t *)g_heap_base;
    uint8_t *s = hb + a0;                                       /* struct at heap offset a0 */
    int32_t count = *(int32_t *)(s + 0x3c);                     /* 0x3d636 */
    uint32_t po = *(uint32_t *)(s + 0x44);                      /* pairs heap offset */
    uint8_t *pairs = hb + po;
    for (int32_t i = 0; i < count; i++) {
        uint32_t a1 = pairs[i * 2 + 0];                         /* 0x3d65e: EDX = a1 */
        uint32_t a2 = pairs[i * 2 + 1];                         /* 0x3d648: EBX = a2 */
        fn_03d67c(a0 + 0x48, a1, a2);                           /* 0x3d66c */
    }
}
/* 0x017218: set the sign of an output value pair by a 2-bit index. Args: a0 (heap
 * offset of a state struct), a1 (int32 value), ptr1/ptr2 (heap offsets of int32 output
 * slots). Sets a0->0x17 = 2. Then on idx = a0->0xbb (0..3):
 *   0: *ptr1= a1, *ptr2= a1   1: *ptr1=-a1, *ptr2= a1
 *   2: *ptr1= a1, *ptr2=-a1   3: *ptr1=-a1, *ptr2=-a1   (idx>3: no write).
 * Finally: if a0->0xab (int32) < 0 return 1 (unchanged); else a0->0xab-- and return 0.
 * NOTE: the original also passes 2 extra register args (EBX, ECX) that are stored but
 * never read; omitted here as they have no observable effect. */
int32_t fn_017218(uint32_t a0, int32_t a1, uint32_t ptr1, uint32_t ptr2) {
    (void)canary_id(0, 0x28);
    uint8_t *hb = (uint8_t *)g_heap_base;
    uint8_t *a = hb + a0;
    int32_t *o1 = (int32_t *)(hb + ptr1);
    int32_t *o2 = (int32_t *)(hb + ptr2);
    *(uint32_t *)(a + 0x17) = 2;                       /* a0->0x17 = 2 */
    uint32_t idx = *(uint32_t *)(a + 0xbb);            /* switch index */
    switch (idx) {
        case 0: *o1 = a1;  *o2 = a1;  break;
        case 1: *o1 = -a1; *o2 = a1;  break;
        case 2: *o1 = a1;  *o2 = -a1; break;
        case 3: *o1 = -a1; *o2 = -a1; break;
        default: break;                                 /* idx > 3: no write */
    }
    int32_t cnt = *(int32_t *)(a + 0xab);
    if (cnt < 0) return 1;                             /* a0->0xab < 0: no decrement */
    *(int32_t *)(a + 0xab) = cnt - 1;                  /* a0->0xab-- */
    return 0;
}
/* 4-byte RIFF chunk markers (DAT_00051470 / DAT_00051475). */
static const uint8_t s_mthd_marker[4] = {0x4d, 0x54, 0x68, 0x64};  /* "MThd" (MIDI header) */
static const uint8_t s_mtrk_marker[4] = {0x4d, 0x54, 0x72, 0x6b};  /* "MTrk" (MIDI track) */
/* 0x034f31: MIDI RIFF deserializer. a0 = offset of the "MThd" chunk, a1 = offset of a
 * record array (0xe bytes each), a2 = offset of the output struct. Validates the MThd
 * magic (else return -1). Sets a2->0x0=a0, a2->0x1c=BE16(MThd+0xa) clamped<=16 (ntrks),
 * a2->0x4=BE16(MThd+0xc) (division), then advances past the MThd chunk (+= BE32(+4)+8).
 * For each of the ntrks MTrk chunks: validate "MTrk" (else -2); rec[0]=track-data ptr
 * (cur+8) as a heap offset, rec[6]=BE32(len), rec[4]=rec[5]=0x90, rec[0xa]=LEB128 delta
 * from rec[0] (advancing rec[0]), cur += len+8. After the loop: a2->0xc=0, a2->0x8=0x7a120,
 * a2->0x20=0 (byte), a2->0x18=0x10000, then fn_035329(a1,a2). Returns 1.
 * NOTE: a0/a1/a2 are heap offsets; the 32-bit pointer fields (a2->0x0, rec[0]) store
 * offsets (g_heap_base-relative), so the source buffer and record array must be in the
 * arena for these offsets to be valid. */
int32_t fn_034f31(uint32_t a0, uint32_t a1, uint32_t a2) {
    (void)canary_id(0, 0x34);                                   /* PUSH 0x34; CALL 0x418ee */
    uint8_t *hb  = (uint8_t *)g_heap_base;
    const uint8_t *src = hb + a0;          /* MThd chunk */
    uint8_t *recs = hb + a1;               /* record array */
    uint8_t *dst  = hb + a2;               /* output struct */
    if (fn_0350aa(src, s_mthd_marker) < 0) return -1;   /* bad MThd magic */
    *(uint32_t *)(dst + 0x0)  = a0;                            /* a2->0x0  = a0 (offset) */
    uint32_t v1c = fn_03510b(src + 4);                        /* BE32(MThd+4) = chunk len */
    uint32_t count = fn_03518a(src + 0xa);                    /* BE16(MThd+0xa) = ntrks */
    if (count > 16) count = 16;                               /* clamp */
    *(uint32_t *)(dst + 0x1c) = count;                        /* a2->0x1c = count */
    *(uint32_t *)(dst + 0x4)  = fn_03518a(src + 0xc);         /* a2->0x4  = division */
    uint8_t *cur = (uint8_t *)src + v1c + 8;                  /* skip MThd -> first MTrk */
    for (uint32_t i = 0; i < count; i++) {
        if (fn_0350aa(cur, s_mtrk_marker) < 0) return -2;     /* bad MTrk magic */
        uint8_t *rec = recs + i * 0xe;
        uint8_t *track = cur + 8;                             /* track data ptr */
        *(uint32_t *)(rec + 0x0) = (uint32_t)(track - hb);    /* rec[0] = cur+8 (offset) */
        uint32_t len = fn_03510b(cur + 4);                    /* BE32(MTrk+4) = track len */
        *(uint32_t *)(rec + 0x6) = len;                       /* rec[6] = len */
        cur = cur + len + 8;                                  /* cur += len+8 */
        rec[0x4] = 0x90;                                      /* rec[4] = 0x90 */
        rec[0x5] = 0x90;                                      /* rec[5] = 0x90 */
        uint8_t *lc = hb + *(uint32_t *)(rec + 0x0);          /* LEB128 cursor (real ptr) */
        uint32_t lval = fn_0351dc(&lc);                       /* decode LEB128 delta */
        *(uint32_t *)(rec + 0x0) = (uint32_t)(lc - hb);       /* rec[0] advanced */
        *(uint32_t *)(rec + 0xa) = lval;                      /* rec[0xa] = delta */
    }
    *(uint32_t *)(dst + 0xc)  = 0;                            /* a2->0xc  = 0 */
    *(uint32_t *)(dst + 0x8)  = 0x7a120;                      /* a2->0x8  = 0x7a120 */
    dst[0x20] = 0;                                            /* a2->0x20 = 0 (byte) */
    *(uint32_t *)(dst + 0x18) = 0x10000;                      /* a2->0x18 = 0x10000 */
    fn_035329(recs, dst);                                     /* scan records for min */
    return 1;
}
uint64_t fn_10876(void) { (void)0; return 0; } /* 0x010876: CD dir-entry read+checksum (no-op -> 0). */
void fn_107b9(void)   { /* 0x0107b9: CD dir-entry read into large buffer (no-op). */ }
void fn_10664(void)   { /* 0x010664: CD request build + DPMI read (no-op; EAX-gated). */ }
void fn_10b1d(void)   { /* 0x010b1d: CD track/TOC read via 0x10570 (no-op). */ }

/* 0x010d47 (L13, 150 bytes): periodic CD-refresh tick.
 *   canary 0x24; q = (int32)g_63e38 / 490  (signed, truncated: IDIV 0x1ea).
 *   if (q <= g_53c48) return;  (JLE signed)
 *   else: g_63e38 = 0;  g_52ff8 = 0;  ...; fn_10b1d();  g_63e24/g_63e28 restored; g_52ff8 = 1.
 * The original then computes e = *(g_63354+0x40); idx = e*4 + (g_63370-1)*40;
 *   v = *(idx + 0x52da4);  and CALL fn_10b1d (passing v in EAX).  In the port fn_10b1d is a
 *   no-op CD read that ignores its argument, so the 0x52da4 data-table read is dead and is
 *   omitted (pointer/data-table wall, same class as 0x03c65b/0x03c72b).  The observable
 *   effects are just the gate + global writes + the fn_10b1d call. */
void fn_010d47(void) {
    (void)canary_id(0, 0x24);
    int32_t q = (int32_t)g_dat_63e38 / 490;   /* IDIV EBX(0x1ea), signed truncated */
    if (q <= g_dat_53c48) return;
    g_dat_63e38 = 0;
    int32_t s1 = (int32_t)g_dat_63e24, s2 = (int32_t)g_dat_63e28;  /* saved/restored (net unchanged) */
    g_dat_52ff8 = 0;
    fn_10b1d();   /* CD track/TOC read (no-op); dead data-table read omitted above */
    g_dat_63e24 = (uint32_t)s1;
    g_dat_63e28 = (uint32_t)s2;
    g_dat_52ff8 = 1;
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

/* 0x041439: IRQ handler (24 ins, ends IRETD). Saves all regs (PUSHAD + 4 segs),
 * CLD, then seg_ds_load(); irq_cli(); DAT_00063f84 = 2; inb(DAT_00053334 + 0xe);
 * outb(0x20, 0x20) (8259 PIC EOI); irq_sti(); restores regs; IRETD.
 *
 * In the flat model the device I/O and the interrupt context are handled by the
 * OS/SDL: seg_ds_load / irq_cli / irq_sti / inb / outb are no-op sys stubs and the
 * IRETD has no C equivalent (the handler just returns to its caller). So the only
 * observable effect is DAT_00063f84 = 2. The decompile's param_1/param_2 are the
 * interrupt-entry registers (EAX:EDX), not real parameters. */
uint32_t g_dat_63f84 = 0;              /* DAT_00063f84: IRQ/device state (0x041439 sets 2) */
uint32_t g_dat_53334 = 0;              /* DAT_00053334: device port base */
void fn_041439(void)
{
    seg_ds_load();                                /* 0x41449 */
    irq_cli();                                    /* 0x4144e */
    g_dat_63f84 = 2;                              /* 0x41453 */
    (void)inb((uint16_t)(g_dat_53334 + 0xe));     /* 0x4145d-41465 */
    outb(0x20, 0x20);                             /* 0x4146a-41474 (PIC EOI) */
    irq_sti();                                    /* 0x41479 */
    /* IRETD: not represented in the flat C model (handler returns). */
}

/* 0x03ee88: timer/IRQ tick handler (38 ins, ends IRETD). On each tick it advances
 * several global counters with wrap logic, then acks the PIC. All callees
 * (seg_ds_load/irq_cli/irq_sti/outb) are ported flat-model no-op sys stubs; IRETD
 * has no C equivalent (the handler returns).
 *   63e28++ ; if >7   : 63f08++, 63e28-=7     (8-tick coarse counter)
 *   63e24++                                                  (per-tick)
 *   63e3c++ ; if >0x11: 63e40++, 63e3c-=0x11  (18-tick coarse counter)
 *   63e38++                                                  (per-tick)
 *   63e30 += 63e2c ; if >0xffff: 63e30-=0xffff, 63e34++ (frame/time, 18-bit wrap)
 *   irq_sti(); outb(0x20,0x20) (PIC EOI) */
uint32_t g_dat_63e28 = 0;   /* DAT_00063e28: tick counter (wraps at 8) */
uint32_t g_dat_63e24 = 0;   /* DAT_00063e24: per-tick counter */
uint32_t g_dat_63e3c = 0;   /* DAT_00063e3c: tick counter (wraps at 18) */
uint32_t g_dat_63e38 = 0;   /* DAT_00063e38: per-tick counter */
uint32_t g_dat_63e40 = 0;   /* DAT_00063e40: coarse counter (every 18) */
uint32_t g_dat_63e30 = 0;   /* DAT_00063e30: frame/time counter (18-bit) */
uint32_t g_dat_63e2c = 0;   /* DAT_00063e2c: tick rate increment */
uint32_t g_dat_63e34 = 0;   /* DAT_00063e34: high frame counter (63e30 wraps) */
int32_t  g_dat_53c48 = 0;   /* DAT_00053c48: CD-refresh threshold (cmp g_63e38/490, 0x010d47) */

/* ---- IRQ-handler install state (fn_03eff2 / fn_03ffab) ---- */
uint8_t  g_dat_63e4a = 0;   /* DAT_00063e4a: IRQ 0x1c (FPU) pending-install flag (1=install) */
uint32_t g_dat_63e44 = 0;   /* DAT_00063e44: IRQ 0x1c handler offset */
uint16_t g_dat_63e48 = 0;   /* DAT_00063e48: IRQ 0x1c handler segment */
uint8_t  g_dat_63e86 = 0;   /* DAT_00063e86: IRQ 9 pending-install flag (1=install) */
uint32_t g_dat_63e80 = 0;   /* DAT_00063e80: IRQ 9 handler offset */
uint16_t g_dat_63e84 = 0;   /* DAT_00063e84: IRQ 9 handler segment */
uint8_t  g_dat_63f51 = 0;   /* DAT_00063f51: IRQ 9 source byte (copied to low-mem 0x417) */
uint8_t  g_dat_00417 = 0;   /* DAT_00000417: real-mode low-memory flag (IRQ 9 source) */

/* ---- PIT ch0 / VGA palette state (fn_03575d / fn_03f3be / fn_03f410) ---- */
uint32_t g_dat_6332c = 0;   /* DAT_0006332c: PIT ch0 status (cleared on set) */
uint32_t g_dat_63318 = 0;   /* DAT_00063318: PIT ch0 divisor (clamped to [1,0x10000]) */
uint32_t g_dat_63e50[256];  /* DAT_00063e50: per-register palette cache (color<<shift) */
uint32_t g_dat_63e5c = 0;   /* DAT_00063e5c: palette index (fn_03f3be) */
uint32_t g_dat_63e58 = 0;   /* DAT_00063e58: palette index (fn_03f410) */
uint8_t  g_dat_63e4c = 0;   /* DAT_00063e4c: palette color shift amount */
void fn_03ee88(void)
{
    seg_ds_load();                          /* 0x3ee98 */
    irq_cli();                              /* 0x3ee9d */
    g_dat_63e28++;                          /* 0x3eea2 */
    if (g_dat_63e28 > 7) { g_dat_63f08++; g_dat_63e28 -= 7; }  /* 0x3eea8-eeb7 */
    g_dat_63e24++;                          /* 0x3eebe */
    g_dat_63e3c++;                          /* 0x3eec4 */
    g_dat_63e38++;                          /* 0x3eeca */
    if (g_dat_63e3c > 0x11) { g_dat_63e40++; g_dat_63e3c -= 0x11; }  /* 0x3eed0-eedf */
    g_dat_63e30 += g_dat_63e2c;            /* 0x3eee6-eeb */
    if (g_dat_63e30 > 0xffff) { g_dat_63e30 -= 0xffff; g_dat_63e34++; }  /* 0x3eef1-ef07 */
    irq_sti();                              /* 0x3ef0d */
    outb(0x20, 0x20);                       /* 0x3ef12-ef1c (PIC EOI) */
    /* IRETD: not represented (handler returns). */
}

/* 0x043381: DPMI real-mode-return handler (34 ins). Calls rm_switch_b (0x433ce),
 * then saves the full register/flag state (EAX, flags, ECX, EDX, EBP, ESI, EDI,
 * DS, ES, FS, GS) into a saved_ctx struct. In the flat model rm_switch_b is a
 * no-op (no real mode) and the register/flag state is register-ABI (cannot be
 * captured in C), so there is no observable effect. The decompile's param_1/param_3
 * are register passthroughs; param_2 is the saved_ctx (unused here). */
void fn_043381(void)
{
    /* no-op: DPMI real-mode return + register/flag save; flat-model no effect */
}

/* 0x04391d: DOS/DPMI timer-vector setup (17 ins). swi(0x21) AH=0x25 AL=0x04:
 * Set Interrupt Vector 4 (18.2 Hz timer) = DAT_00053408:DAT_00053404. If
 * DAT_000533b2 >= 3: dos_install_timer_int(); else manipulate CR0.ET (real-mode
 * FPU trap bit). All of it is DPMI/DOS/CR0-specific with no flat-model effect, and
 * dos_install_timer_int is already a ported no-op, so this is a no-op standin. */
void fn_04391d(void)
{
    /* swi(0x21) AH=0x25 AL=0x04: set int vector 4 = [0x53408]:[0x53404] (no flat effect) */
    if (g_heap_cfg_b2 >= 3) dos_install_timer_int();   /* 0x43934-4393d */
    /* else: CR0.ET bit set/clear (0x43944-43954) -- DPMI-specific, no flat effect. */
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
/* 0x5d884: RODATA string->value table, 21-byte entries {char str[13]; int32 value@13; pad[4]}.
 * 0x633a0 bounds the walk (runtime count). Sized generously for the real table. */
uint32_t g_dat_633a0 = 0;
uint8_t  g_tab_5d884[128 * 21];
res_reader *g_639b8 = NULL;   /* resource-reader node pointer (0x639b8) */
/* 0x036626: case-insensitive lookup of s in the 0x5d884 table (bound g_dat_633a0),
 * returning the matched entry's int32 value (offset 13) or 0. s=EAX. The match
 * compares str_len(s) bytes via str_icmp (0x423e5); a shorter query that is a
 * prefix of a table string still matches (faithful to the original). */
int32_t fn_036626(const char *s) {
    int32_t result = 0;
    uint32_t len = str_len(s);
    for (uint32_t i = 0; i < g_dat_633a0; i++) {
        const uint8_t *e = g_tab_5d884 + i * 21;
        if (str_icmp((const char *)e, s, (int)len) == 0) {
            int32_t v; memcpy(&v, e + 13, 4); result = v;
        }
    }
    return result;
}

/* fn_0367c9 orig: 0x0367c9 (L10, 137B)
 * Resource lookup by name. Walk the g_tab_5d884 string->value table (bound
 * g_dat_633a0, 21-byte entries {str[13]; int32 value@13; int32 value2@17}).
 * On the FIRST entry whose name matches a0 (str_icmp over str_len(a0) bytes,
 * case-insensitive, prefix matches as in fn_036626), submit the entry's int32
 * at offset 17 (0x5d895-0x5d884=0x11) to the resource reader g_639b8 via
 * fn_04247a(r, value, mode=0) and return 1. The fn_04247a return value is
 * discarded (the original ignores it). Returns 0 if no entry matches (or
 * g_dat_633a0 == 0). */
int32_t fn_0367c9(const char *a0) {
    int32_t len = (int32_t)str_len(a0);
    for (uint32_t i = 0; i < g_dat_633a0; i++) {
        const uint8_t *e = g_tab_5d884 + i * 21;
        if (str_icmp((const char *)e, a0, len) == 0) {
            int32_t v; memcpy(&v, e + 17, 4);
            fn_04247a(g_639b8, v, 0);
            return 1;
        }
    }
    return 0;
}

/* fn_036853 orig: 0x036853 (L10, 183B)
 * Resource lookup by name with a value transform. Walk the g_tab_5d884 table
 * (bound g_dat_633a0, 21-byte entries {str[13]; int32 value13@13; int32
 * value2@17}). On the FIRST str_icmp match over str_len(a0) bytes (case-
 * insensitive, prefix matches as in fn_036626), branch on a2:
 *   a2==0: submit v = value2 + a1            to g_639b8, return fn_04247a result.
 *   a2==2: submit v = value2 + value13 + a1  to g_639b8, return fn_04247a result.
 *   else:  no submit, return 0.
 * No match: return g_dat_633a0 (the loop's final i). Both submits use
 * fn_04247a(r, v, mode=0) -- the str_len result in EBX is cleared (XOR EBX,EBX)
 * before the call, so mode is 0. */
int32_t fn_036853(const char *a0, int32_t a1, int32_t a2) {
    int32_t len = (int32_t)str_len(a0);
    for (uint32_t i = 0; i < g_dat_633a0; i++) {
        const uint8_t *e = g_tab_5d884 + i * 21;
        if (str_icmp((const char *)e, a0, len) != 0) continue;   /* no match */
        int32_t v13, v2; memcpy(&v13, e + 13, 4); memcpy(&v2, e + 17, 4);
        if (a2 == 0)       return fn_04247a(g_639b8, v2 + a1, 0);
        if (a2 == 2)       return fn_04247a(g_639b8, v2 + v13 + a1, 0);
        return 0;   /* match but a2 not 0/2: no submit */
    }
    return (int32_t)g_dat_633a0;   /* no match: return count (loop's final i) */
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

/* 0x04247a (L5, 251 B): io_state node write/seek, dispatched on r->flags & 0x6 and mode.
 * r = node (res_reader, 16-bit layout), value = ESI, mode = EBX.
 *   if (r->flags & 0x6):                       // CASE A
 *     if (r->flags2 & 0x10) {                  // CASEA1: flush
 *       if (fn_044748(r) != 0) { if (mode==0 && value<0) fn_042a0d(9); return -1; }
 *     } else {                                  // CASEA2
 *       if (mode == 1) value -= (int32_t)r->remaining;
 *       r->pos = r->end; r->remaining = 0;
 *     }
 *     r->flags &= 0xeb;                         // TAIL_COMMON
 *     return (fn_044807(r->id, value, mode) == -1) ? -1 : 0;
 *   else:                                       // CASE D
 *     mode 0: cur=fn_044848(r->id); if (res_consume(r, value-(cur-r->remaining))==0) return 0;
 *              return (fn_044807(r->id, value, 0)==-1)?-1:0;
 *     mode 1: rem=r->remaining; if (res_consume(r, value)==0) return 0;
 *              return (fn_044807(r->id, value-rem, 1)==-1)?-1:0;
 *     mode 2: r->pos=r->end; r->flags&=0xef; r->remaining=0;
 *              return (fn_044807(r->id, value, 2)==-1)?-1:0;
 *     else:   fn_042a0d(9); return -1;
 * All callees ported; lseek (fn_044807/fn_044848) is a no-op returning 0 in the flat
 * model, so the lseek paths yield 0. Observable effects: return value + node field
 * writes (pos/remaining/flags). */
int32_t fn_04247a(res_reader *r, int32_t value, int32_t mode) {
    if (r->flags & 0x6) {                        /* 0x42481-85 */
        if (r->flags2 & 0x10) {                  /* 0x42487-8b */
            if (fn_044748((io_state_t *)r) != 0) {   /* 0x4248d-94 */
                if (mode == 0 && value < 0) fn_042a0d(9); /* 0x42498-a0 */
                return -1;                        /* 0x424aa-af */
            }
        } else {                                 /* 0x424b3 */
            if (mode == 1) value -= (int32_t)r->remaining; /* 0x424b3-b8 */
            r->pos = r->end;                      /* 0x424c5 */
            r->remaining = 0;                     /* 0x424be */
        }
        r->flags &= 0xeb;                        /* 0x424c7 TAIL_COMMON */
        return (fn_044807(r->id, value, mode) == -1) ? -1 : 0; /* 0x424cb-d8 */
    }
    if (mode == 0) {                             /* 0x424f0 */
        int32_t cur = fn_044848(r->id, 0, 0, 0); /* 0x4251b-1e */
        int32_t amount = value - (cur - (int32_t)r->remaining); /* 0x42523-2a */
        if (res_consume(r, amount) == 0) return 0; /* 0x42530-35 */
        return (fn_044807(r->id, value, mode) == -1) ? -1 : 0; /* 0x42539-43 */
    }
    if (mode == 1) {                             /* 0x424f6 */
        int32_t rem = (int32_t)r->remaining;     /* 0x424fa (pre-consume) */
        if (res_consume(r, value) == 0) return 0; /* 0x424fd-04 */
        return (fn_044807(r->id, value - rem, mode) == -1) ? -1 : 0; /* 0x42506-12 */
    }
    if (mode == 2) {                             /* 0x4254c */
        r->pos = r->end;                         /* 0x42552 */
        r->flags &= 0xef;                        /* 0x4255a */
        r->remaining = 0;                        /* 0x4255f */
        return (fn_044807(r->id, value, mode) == -1) ? -1 : 0; /* 0x42566-6b */
    }
    fn_042a0d(9);                                /* 0x424a0 (mode>2 / <0) */
    return -1;
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
uint8_t   g_heap_cfg_b3    = 0;   /* DAT_000533b3 */
uint8_t   g_heap_cfg_b4    = 0;   /* DAT_000533b4 */
uint8_t   g_dat_5340c      = 0;   /* DAT_0005340c: system-init "armed" flag (1 = run init) */
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
    g_heap_cfg_b3    = 0;
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

/* 0x0431b5: heap size-adjuster. Adjusts the u32 size field of a heap block *p
 * and returns 1 on success, 0 if the block is empty (aligns to 0) or the size
 * arithmetic overflows. Register ABI: EAX=p; returns EAX (0/1).
 *   1. a = align_up(*p, 4)   = (*p+3) &~ 3                  (0x431bc-431bf, AND DL,0xfc)
 *   2. if a==0 -> 0
 *   3. (cfg_b2==1 && cfg_b3==0) || cfg_b2==9 ? a+=8 : a-=heap_top_check()
 *   4. *p = a
 *   5. t = a+0x3c; if t<a (unsigned wrap) -> 0
 *   6. if t < g_dat_53640 (limit): t = limit &~ 1           (0x43208, AND DL,0xfe)
 *   7. *p = t
 *   8. u = t+0xfff; if u<t (unsigned wrap) -> 0
 *   9. u = align_up(u, 0x1000) = u &~ 0xfff                 (0x43217 AND DH,0xf0 + 0x4321a XOR DL)
 *  10. *p = u; return u!=0
 * The original applies the low-bit masks via byte ops (AND DL/.., AND DH,..) that
 * touch only the low/second byte, but every affected bit lies within those bytes,
 * so &~ 3 / &~ 1 / &~ 0xfff are exact. */
uint32_t heap_size_adjust(uint32_t *p)
{
    uint32_t a = (*p + 3u) & ~3u;                        /* 0x431bc-431bf */
    if (a == 0) return 0;                                /* 0x431c2-431cb */
    if ((g_heap_cfg_b2 == 1 && g_heap_cfg_b3 == 0) || g_heap_cfg_b2 == 9) {
        a += 8u;                                         /* 0x431e7 */
    } else {
        a -= heap_top_check();                           /* 0x431ec-431f1 */
    }
    *p = a;                                              /* 0x431f3 */
    uint32_t t = a + 0x3cu;                              /* 0x431f5 */
    if (t < a) return 0;                                 /* 0x431f8-431fa */
    if (t < g_dat_53640) t = g_dat_53640 & ~1u;          /* 0x43202-43208 */
    *p = t;                                              /* 0x4320b */
    uint32_t u = t + 0xFFFu;                             /* 0x4320d */
    if (u < t) return 0;                                 /* 0x43213-43215 */
    u &= ~0xFFFu;                                        /* 0x43217/0x4321a align_up(u,0x1000) */
    *p = u;                                              /* 0x4321c */
    return (u != 0) ? 1u : 0u;                           /* 0x4321e-43220 */
}

/* 0x0423cf: registered-callback dispatcher. PUSH EDX; EDX=EAX; CALL [g_dat_53370];
 * CALL [g_dat_53374]; EAX=EDX; JMP 0x422d3 (shared epilogue: POP EDX; RET).
 * Calls g_dat_53370(input), then g_dat_53374(first's return), returns original.
 * Both slots are called unconditionally (the original requires them set; a NULL
 * slot faults, as in the original). */
u32_cb_fn g_dat_53370 = NULL;   /* DAT_00053370 */
u32_cb_fn g_dat_53374 = NULL;   /* DAT_00053374 */
u32_cb_fn g_dat_5336c = NULL;   /* DAT_0005336c */
uint32_t fn_0423cf(uint32_t input) {
    uint32_t r = g_dat_53370(input);   /* 0x423d2 CALL [0x53370] */
    (void)g_dat_53374(r);              /* 0x423d8 CALL [0x53374] */
    return input;                        /* 0x423de EAX=EDX (original input) */
}

/* 0x0423b7: PUSH EDX; EDX=EAX; CALL [0x5336c]; CALL [0x53370]; EAX=EDX; CALL
 * 0x423cf; POP EDX; RET. Chain: g_dat_5336c(param_1) -> r1; g_dat_53370(r1);
 * fn_0423cf(param_1). param_2/3/4 are register-passed (unused by the u32_cb chain). */
void fn_0423b7(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4) {
    (void)param_2; (void)param_3; (void)param_4;
    uint32_t r1 = g_dat_5336c(param_1);   /* 0x423ba CALL [0x5336c] */
    (void)g_dat_53370(r1);                 /* 0x423c0 CALL [0x53370] */
    fn_0423cf(param_1);                    /* 0x423c8 CALL 0x423cf (EAX=EDX=param_1) */
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

/* Emit-subsystem numeric tokenizer.  orig: 0x044387.
 * arg1 (EAX) ctx  = emit context (next@+0, emit@+4, count@+0xc, flags@+0x10).
 * arg2 (EDX) pos    = value-buffer stream cursor (uintptr_t **; mirrors emit_value).
 * arg3 (EBX) base   = 0 auto-detect (0x->16 / lone 0->8 / else 10) else 8/10/16.
 * arg4 (ECX) allow_sign = non-zero permits a leading +/-.
 * Returns total chars consumed. See f.h for the full contract. */
int fn_044387(struct emit_ctx *ctx, uintptr_t **pos, int base, int allow_sign)
{
    int skip = 0;          /* EBP: leading-skip + sign + prefix + colon count */
    int have_digit = 0;    /* EBX: number of digits folded into acc            */
    int acc = 0;           /* [ESP+8]: accumulator                             */
    int sign = 0x2b;       /* [ESP]: sign char (default '+')                   */
    int b = base;          /* EDI: effective base                              */
    int c = (int)ctx->next(ctx);   /* 0x43a6: first char                       */

    for (;;) {                                        /* 0x43a4-0x43bb */
        if ((kCharClass[(uint8_t)(c + 1)] & 0x2) == 0) break;
        skip++;
        c = (int)ctx->next(ctx);
    }
    if (ctx->flags & 0x2) goto epilogue;              /* 0x43bd: peek mode */

    int cnt = ctx->count;
    ctx->count = (uint32_t)(cnt - 1);                 /* 0x43c7-0x43cd */
    if (cnt == 0) {                                   /* 0x43d2 */
        if (ctx->emit) ctx->emit((uint16_t)c, ctx);   /* 0x44f6: emit trailing */
        goto epilogue;
    }

    if (allow_sign && (c == 0x2b || c == 0x2d)) {     /* 0x43dd-0x43f0 */
        sign = c;
        skip++;
        c = (int)emit_step(ctx);                      /* 0x43f8 */
        if (c == -1) goto epilogue;                   /* 0x4402 */
    }

    if (b != 0) {                                     /* 0x4460 */
        if (b == 16 && c == 0x30) {                   /* 0x4465 */
            have_digit = 1;
            c = (int)emit_step(ctx);
            if (c == -1) goto epilogue;               /* 0x447d */
            if (c == 0x78 || c == 0x58) { skip += 2; have_digit = 0; c = (int)emit_step(ctx); if (c == -1) goto epilogue; }
        }
        goto digits;
    }
    if (c == 0x30) {                                  /* 0x440c */
        have_digit = 1;
        c = (int)emit_step(ctx);
        if (c == -1) goto epilogue;                   /* 0x4424 */
        if (c == 0x78 || c == 0x58) { skip += 2; have_digit = 0; c = (int)emit_step(ctx); if (c == -1) goto epilogue; b = 16; goto digits; }
        b = 8;                                        /* 0x4452 */
        goto digits;
    }
    b = 10;                                           /* 0x4459 */

digits:                                              /* 0x44a0 */
    {
        int v = hex_digit_value(c);
        if (v < b) { acc = acc * b + v; have_digit++; c = (int)emit_step(ctx); if (c == -1) goto epilogue; goto digits; }
    }
    if (c == 0x3a && (ctx->flags & 0x80)) {           /* 0x44c2/0x44c7: ':' */
        for (;;) {
            have_digit++;
            c = (int)emit_step(ctx);                  /* 0x44d0 */
            if (c == -1) goto epilogue;               /* 0x44dc */
            {
                int v = hex_digit_value(c);
                if (v < b) { acc = acc * b + v; continue; }   /* 0x44e5 */
            }
            break;
        }
    }
    if (ctx->emit) ctx->emit((uint16_t)c, ctx);       /* 0x44f6: emit trailing */

epilogue:                                            /* 0x44fd */
    if (sign == 0x2d) acc = -acc;                    /* 0x4503 */
    if (have_digit > 0 && (ctx->flags & 0x1)) {       /* 0x4507/0x4514 */
        uintptr_t dest = **pos;                       /* 0x4556: dest = slot value */
        *pos += 1;                                    /* 0x4552: advance stream cursor */
        if (ctx->flags & 0x10) *(uint16_t *)dest = (uint16_t)acc;
        else                    *(uint32_t *)dest = (uint32_t)acc;
    }
    return skip + have_digit;                        /* 0x4512/0x4575 */
}

/* 0x043fc3: character-set filter/extract (91 insns).
 *
 * Parses a character set from *src (an optional '^' escape marker followed by a
 * run of bytes up to ']' or NUL, built into a 32-byte presence bitmap via
 * fn_043f80), then reads consecutive chars from ctx->next and keeps only those
 * matching the set: without '^' it keeps chars PRESENT in the set, with '^' it
 * keeps chars NOT present (the caret inverts). Matching chars are written to
 * the output buffer (dest = **pos; *pos += 1 -- all near/far/0x8 modes collapse
 * in the flat 64-bit model, as in emit_value) when flags&0x1, and null-
 * terminated when flags&0x1 and count>0. A non-matching char is passed to
 * ctx->emit (put back) and stops the scan; peek mode (flags&0x2) reads one char
 * then exits without storing. Returns the number of matching chars written.
 *
 * ABI (register): ctx=EAX, pos=EDX (output cursor, uintptr_t **), src=EBX
 * (source cursor, const uint8_t **). The 0x51854 table = {1,2,4,8,16,32,64,128}
 * = 1<<n. Depends on fn_043f80 (presence bitmap). orig: 0x043fc3 */
uint32_t fn_043fc3(struct emit_ctx *ctx, uintptr_t **pos, const uint8_t **src)
{
    const uint8_t *sp = *src;                  /* [EBX]: source position        */
    int is_caret = (sp[0] == 0x5e) ? 1 : 0;    /* byte at *src == '^'?          */
    if (is_caret) *src = sp + 1;              /* skip the '^' marker           */
    uint8_t bitmap[0x20];
    *src = fn_043f80(*src, bitmap);           /* build presence bitmap         */

    uint8_t *dest = NULL;
    if (ctx->flags & 0x1) { dest = (uint8_t *)**pos; *pos += 1; }  /* output buf */

    uint32_t count = ctx->count;              /* [ECX+0xc]                     */
    uint32_t written = 0;
    while (count > 0) {
        int c = ctx->next(ctx);
        if (ctx->flags & 0x2) break;          /* peek mode: read one, exit     */
        int not_present = !(bitmap[c >> 3] & (1 << (c & 7)));
        if (not_present != is_caret) {        /* non-matching char             */
            if (ctx->emit) ctx->emit((uint16_t)c, ctx);
            break;
        }
        if (ctx->flags & 0x1) dest[written] = (uint8_t)c;
        written++;
        count--;
    }
    if ((ctx->flags & 0x1) && written > 0) dest[written] = 0;   /* null-term   */
    return written;
}

/* 0x043e34: emit-subsystem whitespace-delimited token reader (93 insns).
 *
 * Reads a whitespace-delimited token: skips leading whitespace (kCharClass[(c+1)]
 * bit 0x02 = whitespace, each counted in `consumed`), then stores a run of
 * non-whitespace chars to the output buffer, stopping at the next whitespace or
 * EOF. `count` is a token budget: if count>0 it is decremented once and the token
 * is read+stored; if count==0 the first non-ws char is emitted back (put) and
 * nothing is stored. The first token char is read via ctx->next (in the skip
 * loop); subsequent chars via emit_step (which decrements count again per char and
 * returns 0xffff on EOF/peek). A terminating whitespace char is emitted back; an
 * EOF terminator is not. Output slot: dest = (uint8_t*)(old *pos), *pos += 8
 * (flags&4, far) else 4 (near / flags&8) -- all flat in 64-bit. Store size = mode
 * (flags&0x20 ? word : byte), dest advances by mode per char; null-terminated at
 * dest_end when flags&1 and consumed>0. Peek mode (flags&2) returns 0 after
 * skipping. Returns total chars consumed (leading whitespace + token chars).
 *
 * ABI (register): ctx=EAX, pos=EDX (output cursor, uintptr_t *). Depends on
 * emit_step (0x0445a6) and kCharClass (0x5185c). orig: 0x043e34 */
uint32_t fn_043e34(struct emit_ctx *ctx, uintptr_t *pos)
{
    int mode = (ctx->flags & 0x20) ? 2 : 1;         /* [ESP]: 1=byte 2=word  */

    uint8_t *dest = NULL;
    if (ctx->flags & 0x1) {                          /* output slot (flat)     */
        dest = (uint8_t *)(uintptr_t)*pos;          /* old *pos = slot start  */
        *pos += (ctx->flags & 0x4) ? 8 : 4;         /* far=8, near/0x8=4      */
    }

    uint32_t consumed = 0;                           /* ESI                    */
    int c = (int)ctx->next(ctx);                     /* skip loop            */
    while (kCharClass[(uint8_t)(c + 1)] & 0x2) {
        consumed++;
        c = (int)ctx->next(ctx);
    }
    if (ctx->flags & 0x2) return 0;                  /* peek mode             */

    uint32_t cnt = ctx->count;
    ctx->count = cnt - 1;
    if (cnt == 0) {                                  /* token budget exhausted */
        if (ctx->emit) ctx->emit((uint16_t)c, ctx);  /* put back first char    */
        goto end;
    }

    for (;;) {
        consumed++;
        if (ctx->flags & 0x1) {
            if (mode == 1) { dest[0] = (uint8_t)c; dest += 1; }
            else           { *(uint16_t *)dest = (uint16_t)c; dest += 2; }
        }
        c = (int)emit_step(ctx);
        if (c == -1) goto end;                       /* EOF: no put-back      */
        if (kCharClass[(uint8_t)(c + 1)] & 0x2) {    /* whitespace terminator */
            if (ctx->emit) ctx->emit((uint16_t)c, ctx);   /* put back          */
            goto end;
        }
        /* non-ws: continue */
    }
end:
    if ((ctx->flags & 0x1) && consumed > 0) {
        if (mode == 1) dest[0] = 0;
        else           *(uint16_t *)dest = 0;
    }
    return consumed;
}

/* 0x042ccc: DPMI free-all heap teardown (41 insns).
 *
 * Walks the g_heap_head region chain. For each region r, if it is "freeable" --
 * its sentinel-prev block fills the whole region, i.e. B_SIZE(R_SENT_PREV(r)) +
 * 0x2c == B_SIZE(r) (the region holds a single full free block) -- it is unlinked
 * via heap_unlink and its DPMI memory is freed (INT 31h AH=0x05 AL=0x02, or INT
 * 21h AH=0x49 depending on a saved prev-field check). Both INT frees are no-ops
 * in the flat model, so the sole observable effect is the heap_unlink of each
 * freeable region. Non-freeable regions are left in place. The chain advances via
 * B_NEXT(r) (saved before unlink). orig: 0x042ccc */
void fn_042ccc(void)
{
    uint32_t r = g_heap_head;
    while (r != 0) {
        uint32_t next = B_NEXT(r);                       /* save before unlink  */
        if (B_SIZE(R_SENT_PREV(r)) + 0x2c == B_SIZE(r)) {    /* freeable?        */
            heap_unlink(r);                              /* remove from chain   */
            /* INT 31h AH=5 AL=2 / INT 21h AH=49 free: no-op in flat model     */
        }
        r = next;
    }
}

/* 0x042eef: free a DPMI heap block (84 insns).
 *
 * In the flat model the only observable effect is the initial CALL fn_042ccc
 * (unlink freeable regions from g_heap_head). The DPMI/DOS memory services
 * (INT 31h AH=05 AL=01 free, INT 21h AH=48 alloc, INT 31h AH=06 remap) never
 * happen (no DPMI) and fail (CF=1), so the success-path far-pointer
 * bookkeeping is unreachable and the function bails with EAX=0. The conditional
 * write DAT_0006400c=0xfffff is guarded by (DAT_0006400e & 0xfff0); DAT_0006400e
 * is never set anywhere in the port (stays 0), so that write is unreachable too.
 * arg is the block far-pointer/handle (unused in the flat model). orig: 0x042eef */
uint32_t fn_042eef(uint32_t arg)
{
    fn_042ccc();   /* 0x42ef5: unlink freeable regions (the only real effect)  */
    (void)arg;     /* block handle: only feeds the DPMI free (no-op)           */
    return 0;      /* DPMI/DOS free+alloc fail (CF=1) -> bail with EAX=0       */
}

/* 0x042fc2 reuses existing DPMI/heap config globals: g_heap_seg_limit (DAT_00053384),
 * g_heap_cfg_b2 (DAT_000533b2), g_dat_5363c (DAT_0005363c). */

/* 0x042fc2 (L3, 174 B): DPMI/DOS heap block allocator (INT 0x21 AH=48 AL=0x8000).
 * canary-free (PUSH regs; shared epilogue 0x042fb9: ADD ESP,4; POPs; RET; return EAX).
 * Flat model: no DOS/DPMI -> INT 0x21 AH=48 always fails (CF=1, masked EAX=0) and the
 * b2==1 free path (fn_042eef) also returns 0, so every branch returns 0 (alloc failed);
 * the heap_insert_init/heap_free_host bookkeeping (EAX=1) is unreachable. Faithful
 * control flow: guard on g_dat_5363c==0 / g_heap_seg_limit==-2, heap_size_adjust(&size) (0->fail),
 * then the b2==1 free path (fn_042eef) or the INT 0x21 AH=48 alloc (no-op -> 0).
 * Observable side effects preserved via the ported callees: heap_size_adjust's
 * size-cell write and, in the b2==1 path, fn_042eef -> fn_042ccc (heap unlink).
 * Return is EAX (0=fail, 1=success) -- the decompiler's undefined8 was an EAX:EDX
 * artifact (EDX holds only a DPMI segment in the unreachable success path). orig: 0x042fc2 */
uint32_t fn_042fc2(uint32_t size)
{
    if (g_dat_5363c == 0) return 0;                 /* 0x042fc8-cf */
    if (g_heap_seg_limit == 0xfffffffe) return 0;   /* 0x042fd5-fc (== -2) */
    uint32_t s = size;
    if (heap_size_adjust(&s) == 0) return 0;        /* 0x042fde-fe7 */
    if (g_heap_cfg_b2 == 1) {                       /* 0x042fe9: free-a-block path */
        uint32_t r = fn_042eef(size);               /* 0x042ff2-ff5 */
        if (r == 0) return 0;                       /* 0x042ffe-00 (always, in flat) */
        s = *(uint32_t *)r;                         /* 0x043002-04 (unreachable in flat) */
        return 1;                                   /* 0x043024+ bookkeeping -> success */
    }
    /* 0x043009-1f: EAX=0x80004800; INT 0x21 (AH=48 alloc); EAX &= (CF ? 0 : ~0) -> 0 (fail) */
    (void)s;
    uint32_t seg = 0;                               /* masked DPMI segment (0 in flat) */
    if (seg == 0) return 0;                         /* 0x043024-26 */
    return 1;                                       /* 0x043028-6c bookkeeping (unreachable) */
}

/* 0x043071 (L4, 323 B): DPMI/DOS core heap acquirer -- the real allocator the
 * original heap_malloc (0x041af8) delegated to. The port's heap_malloc is a
 * self-contained arena and does NOT call this; this is 1:1 coverage only.
 * canary-free (PUSH regs; shared epilogue 0x0431ac: ADD ESP,4; POPs; RET; return EAX).
 * Disassembly-verified (110 insns): direct CALLs to 0x042fc2 (fn_042fc2, DPMI path),
 * 0x0431b5 (heap_size_adjust), 0x045bb7 (fn_045bb7), plus the UNREACHABLE 0x042e2d
 * (heap_insert_init) / 0x041c31 (heap_free_host) in the success path. NO indirect.
 * FLAT MODEL = always return 0 (alloc failed):
 *   - DPMI path (b2==1&&b3==0 || b2==9): fn_042fc2(size) -> 0.
 *   - heap-walk path: guard g_dat_5363c==0 -> 0; guard g_heap_seg_limit==-2 -> 0;
 *     heap_size_adjust(&s)==0 -> 0; then total = s+g_heap_seg_limit (overflow -> -2);
 *     fn_045bb7(total) -> -1 (flat) -> 0. The g_heap_head (0x53350) block walk,
 *     adjacency extension, and new-block creation (heap_insert_init + heap_free_host,
 *     EAX=1 success) are all unreachable because fn_045bb7 fails first.
 * The 2<=b2<=8 && b4==0 branch recomputes g_heap_seg_limit = DS-seg-limit+1 (LSL);
 * dead in the flat model (b2==0 default) -- modeled via the heap_top_check equivalent.
 * Side effects preserved via ported callees: heap_size_adjust size-cell write,
 * fn_045bb7 (g_dat_64008=5). Reuses existing g_heap_seg_limit (0x53384) +
 * g_heap_cfg_b2/b3/b4 (0x533b2/b3/b4); NO new globals. orig: 0x043071 */
uint32_t fn_043071(uint32_t size)
{
    if ((g_heap_cfg_b2 == 1 && g_heap_cfg_b3 == 0) || g_heap_cfg_b2 == 9) {
        return fn_042fc2(size);                        /* 0x043092-95 (0 in flat) */
    }
    if (g_dat_5363c == 0) return 0;                    /* 0x04309f-a6 */
    if (g_heap_seg_limit == 0xfffffffe) return 0;      /* 0x0430af-b6 (== -2) */
    uint32_t s = size;
    if (heap_size_adjust(&s) == 0) return 0;           /* 0x0430b8-c1 */
    if (g_heap_cfg_b2 >= 2 && g_heap_cfg_b2 <= 8 && g_heap_cfg_b4 == 0) {
        g_heap_seg_limit = (g_ds_segment + 1) * 0x10000u;   /* 0x0430e0-e8 (dead in flat) */
    }
    uint32_t total = s + g_heap_seg_limit;             /* 0x0430ed-f6 */
    if (total < g_heap_seg_limit) total = 0xfffffffe;  /* 0x0430f8-fc overflow -> -2 */
    int32_t r = fn_045bb7(total);                      /* 0x043101-03 (-1 in flat) */
    if (r == -1) return 0;                             /* 0x043108-0b (always in flat) */
    /* 0x04310d+: unreachable (r != -1) -- block walk + adjacency + creation -> 1 */
    return 1;
}

/* 0x0438a3: DPMI/DOS core system-init dispatcher (122 B; NOT in the 528 list).
 * Guarded by DAT_0005340c: if it is not 1, return immediately (no change).
 * Otherwise dispatch on DAT_000533b2 (b2) / DAT_000533b4 (b4); every branch is
 * DPMI/DOS/CR0-specific with no flat-model effect:
 *   b4 != 0        -> dos_install_timer_int()                        (0x043958)
 *   else b2 == 9   -> DPMI call [DAT_00053364]+0x30, EAX=0xe02,EBX=0
 *   else b2 == 0   -> INT 0x21 AH=0xf3, AL=low8(DAT_0005340a)
 *   else b2 == 1   -> DX=DAT_00053368, EAX=0, dos_realmode_setup()   (0x0490b0, arg ignored)
 *   else           -> EDX=0, dos_realmode_setup(); if (AL==1) fn_04391d()
 * then clears DAT_0005340c = 0. All three callees (dos_install_timer_int,
 * dos_realmode_setup, fn_04391d) are already ported no-ops, so the sole
 * flat-model effect is the DAT_0005340c clear. 0x041fa1 is a 5-byte
 * `JMP 0x0438a3` tail thunk whose behavior is exactly this function.
 * orig: 0x0438a3 (0x0438a3..0x04391c) */
void fn_0438a3(void)
{
    if (g_dat_5340c != 1) return;                       /* 0x0438a3-a8 guard  */
    if (g_heap_cfg_b4 != 0) {
        dos_install_timer_int();                        /* 0x0438be (0x043958) */
    } else if (g_heap_cfg_b2 == 9) {
        /* 0x0438c9-d9: DPMI call [DAT_00053364]+0x30, EAX=0xe02, EBX=0 (no flat effect) */
    } else if (g_heap_cfg_b2 == 0) {
        /* 0x0438e2-ea: INT 0x21 AH=0xf3, AL=low8(DAT_0005340a) (DOS, no flat effect) */
    } else if (g_heap_cfg_b2 == 1) {
        dos_realmode_setup();                           /* 0x0438f2-fb (0x0490b0), arg ignored */
    } else {
        dos_realmode_setup();                           /* 0x043902-04: EDX=0 */
        /* 0x043909-0d: if (AL==1) fn_04391d(); the flat DOS stub leaves AL=0 -> not
         * taken, and fn_04391d is a no-op either way. */
    }
    g_dat_5340c = 0;                                    /* 0x043912           */
}

/* 0x041fa1: 5-byte tail thunk `JMP 0x0438a3` -- its behavior is exactly
 * fn_0438a3(); a straight forward. orig: 0x041fa1 */
void fn_041fa1(void)
{
    fn_0438a3();
}

/* 0x042d2a: heap region expand / absorb-adjacent (93 insns).
 *
 * In the flat model: CALL fn_042ccc (unlink freeable regions), then walk
 * g_heap_head. For the (first, offset-sorted) region r where (r + 0x2c) == arg1
 * AND alloc_count hw(r)[6] == 1, call heap_unlink(r); if hw(r-4)[0] != 0 (the
 * [region-8]+4 check) it returns 0 without the DPMI step. Otherwise it attempts
 * the DPMI realloc (INT 31h AH=05 AL=03), which never happens in the flat model
 * and fails (CF=1), so the success-path bookkeeping is unreachable and the
 * function returns 0. Non-matching regions advance via B_NEXT; a region with
 * r > arg1 (JA) bails immediately. arg2 (EBP) only feeds the unreachable
 * success path. orig: 0x042d2a */
uint32_t fn_042d2a(uint32_t arg1, uint32_t arg2)
{
    fn_042ccc();                                       /* 0x42d33              */
    (void)arg2;
    uint32_t r = g_heap_head;                          /* 0x42d38              */
    while (r != 0) {
        if (r > arg1) return 0;                        /* 0x42d45 JA           */
        if (r + 0x2c == arg1 && hw(r)[6] == 1) {       /* 0x42d50 / 0x42d5a    */
            heap_unlink(r);                            /* 0x42d62              */
            if (hw(r - 4)[0] != 0) return 0;           /* 0x42d6a/6e           */
            /* INT 31 AH=5 AL=3 realloc: fails (CF=1) -> return 0 (0x42dba)   */
            return 0;
        }
        r = B_NEXT(r);                                 /* 0x42e1a advance      */
    }
    return 0;
}

/* 0x048fe6: DPMI / real-mode IRQ installer (86 insns), register-ABI
 * (arg1 = EAX, arg2 = EDX = [ESP], returns AL).
 *
 * Under DPMI (the original's real environment) this queries DPMI (INT 2f
 * AH=0x16); if DPMI is present and passes its checks it runs a sequence of
 * INT 2f AH=0xfa interrupt-gate calls, sets DAT_000536c9 = 1, and returns 1.
 * In the non-DPMI fallback it installs a real-mode IRQ handler (isr_set(7,
 * 0x461a8, CS)) gated behind a DPMI INT 31 AX=0xa00 call and returns 0.
 *
 * Flat model: no DPMI -> the initial INT 2f AH=0x16 query returns AL=0, so the
 * function takes the fallback; there the INT 31 AX=0xa00 gate call fails
 * (CF=1, no DPMI), so the isr_set and the DAT_000536c8/6c9 writes are
 * unreachable and it returns 0. DAT_000536c9 is not read anywhere in the port
 * and the function has no direct callers (DPMI callback), so the sole
 * observable behavior is the return value 0 -> no-op standin. The real-mode ISR
 * it would install does not apply to the flat address space. orig: 0x048fe6 */
uint32_t fn_048fe6(uint32_t arg1, uint32_t arg2)
{
    (void)arg1; (void)arg2;
    return 0;
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

/* ---- 0x42a08 family: writers into the DAT_64008/64004 globals ---- */
uint32_t g_dat_64008 = 0;      /* DAT_00064008 */
uint32_t g_dat_64004 = 0;      /* DAT_00064004 */
u32_cb_fn  g_dat_533bd = NULL; /* DAT_000533bd */
/* 0x042a08: MOV EAX,0xd; fall through the 0x042a0d body -> g_dat_64008 = 13. */
void fn_042a08(void) { g_dat_64008 = 0xd; }
/* 0x042a0d: PUSH EDX; MOV EDX,EAX; CALL const_64008; MOV [EAX],EDX; POP EDX; RET. */
void fn_042a0d(uint32_t x) { g_dat_64008 = x; }

/* 0x041e94: pure JMP thunk to 0x041bdc (heap_malloc). Same reg-ABI (EAX=size,
 * high word ignored). Behaviourally identical to heap_malloc(size). */
uint32_t fn_041e94(uint32_t size) { return heap_malloc(size); }

/* 0x04123d: device-port init (no stack args, RET 0). Prologue canary PUSH 0x20;
 * CALL 0x0418ee (canary_id, no-op guard in flat model). Then outb(port,1); 10 x
 * inb(port); outb(port,0) where port = g_dat_53334 + 6. The loop counter (EAX=[EBP-4],
 * INC) is recomputed/overwritten with the port before each inb, so it only bounds the
 * 10 identical inb(port) calls (dead as an argument). */
void fn_04123d(void) {
    (void)canary_id(0, 0x20);                        /* 0x4123d-41242 */
    uint16_t port = (uint16_t)(g_dat_53334 + 6);      /* [0x53334] + 6 */
    outb(port, 1);                                     /* 0x4125a-41262 */
    for (int i = 0; i < 10; i++) (void)inb(port);      /* 0x41267-41289 */
    outb(port, 0);                                     /* 0x4128b-41295 */
}

/* 0x0412e9: CD-ROM/device status spin-poll. canary prologue PUSH 0x24; CALL 0x418ee.
 * No args. Polls inb(g_dat_53334 + 0xe) up to 0x7fff times: a counter (init 0x7fff)
 * is decremented before each poll; the first poll whose byte has bit 0x80 SET returns
 * 1, and if the counter reaches -1 (0x7fff polls, bit never set) it returns 0.
 * EDX is push/pop'd around inb (the original inb clobbers EDX, then restores it), so
 * the decompiler's "extraout EDX" is an artifact -- the only output is EAX.
 * A 2026-07-08 note deferred this to the "CD subsystem" batch, but it is a
 * self-contained verified leaf: ported here (wiring g_dat_53334 to a real device is a
 * separate sys-layer task). */
uint32_t fn_0412e9(void) {
    (void)canary_id(0, 0x24);
    int32_t n = 0x7fff;
    for (;;) {
        n--;                                            /* 0x41308 DEC [EBP-4] */
        if (n == -1) return 0;                          /* 0x4130b/4130f */
        uint8_t r = inb((uint16_t)(g_dat_53334 + 0xe)); /* 0x41311-41319 */
        if (r & 0x80) return 1;                         /* 0x4131e/41320/41322 */
    }
}

/* 0x041390: mirror of 0x0412e9, but polls inb(g_dat_53334 + 0xc) for bit 0x80 CLEAR.
 * canary prologue PUSH 0x24; CALL 0x418ee. No args. The first poll whose byte has bit
 * 0x80 CLEAR returns 1; if the 0x7fff-iter counter exhausts (bit never cleared) 0. */
uint32_t fn_041390(void) {
    (void)canary_id(0, 0x24);
    int32_t n = 0x7fff;
    for (;;) {
        n--;                                            /* 0x413af DEC [EBP-4] */
        if (n == -1) return 0;                          /* 0x413b2/413b6 */
        uint8_t r = inb((uint16_t)(g_dat_53334 + 0xc)); /* 0x413b8-413c0 */
        if (!(r & 0x80)) return 1;                      /* 0x413c5/413c7/413c9 */
    }
}

/* 0x0412a3: read a device status byte, gated on the ready-poll. canary prologue
 * PUSH 0x20; CALL 0x418ee, no args. Calls fn_0412e9() (poll +0xe for bit7 set);
 * if that returns 0 (not ready) it returns 0; otherwise it returns
 * inb(g_dat_53334 + 0xa) (the status register). */
uint32_t fn_0412a3(void) {
    (void)canary_id(0, 0x20);                            /* 0x412a3-0x412a8 */
    if (fn_0412e9() == 0) return 0;                      /* 0x412bb/412c0/412c2 */
    return inb((uint16_t)(g_dat_53334 + 0xa));           /* 0x412cd/412d5 */
}

/* 0x041340: write a device command byte, gated on the ready-poll. canary prologue
 * PUSH 0x24; CALL 0x418ee, command in EAX. Calls fn_041390() (poll +0xc for bit7
 * clear); if that returns 0 (not ready) it returns 0; otherwise it issues
 * outb(g_dat_53334 + 0xc, cmd) and returns 1. */
uint32_t fn_041340(uint32_t cmd) {
    (void)canary_id((uintptr_t)cmd, 0x24);               /* 0x41340-0x41345 */
    if (fn_041390() == 0) return 0;                      /* 0x4135b/41360/41362 */
    outb((uint16_t)(g_dat_53334 + 0xc), (uint8_t)cmd);   /* 0x4136d-0x41378 */
    return 1;                                            /* 0x4137d */
}

/* 0x0358c7: device port-wait + write (register-ABI, byte in AL). canary prologue
 * PUSH 0x28; CALL 0x418ee. Polls inb(g_dat_530ac + 1) up to 0x2000 times, counting
 * consecutive reads with bit 0x40 SET; when the bit clears (i < 0x2000) it writes
 * outb(g_dat_530ac, arg) and returns 0; if the bit stays set for all 0x2000 reads it
 * returns -1 (timeout) and does not write. */
uint32_t g_dat_530ac = 0;  /* DAT_000530ac: device port base */
int32_t fn_0358c7(uint8_t arg) {
    (void)canary_id((uintptr_t)arg, 0x28);               /* 0x358c7-0x358cc */
    int32_t i = 0;
    while (i < 0x2000 && (inb((uint16_t)(g_dat_530ac + 1)) & 0x40)) /* 0x358f1-0x35907 */
        i++;
    if (i >= 0x2000) return -1;                          /* 0x3590d/0x35916 */
    outb((uint16_t)g_dat_530ac, arg);                    /* 0x35924-0x35929 */
    return 0;                                            /* 0x3592e */
}

/* 0x0326c1: free all per-element heap buffers embedded in a struct. canary
 * PUSH 0x28; CALL 0x418ee. base (EAX) is a real pointer to a struct laid out
 * as: count@0 (int32), then two parallel arrays of `count` 32-bit HEAP OFFSETS
 * at +0x68 and +0x1f8 (4-byte spacing). For i in 0..count-1 it frees both
 * *(base+0x68+i*4) and *(base+0x1f8+i*4) via heap_free_host, then returns base.
 * (The EDX arg is unused. Faithful to the 4-byte offset spacing.) */
uint8_t *fn_0326c1(uint8_t *base) {
    (void)canary_id((uintptr_t)base, 0x28);              /* 0x326c1-0x326c6 */
    int32_t n = *(int32_t *)base;                        /* 0x326e5: count at +0 */
    for (int32_t i = 0; i < n; i++) {                    /* 0x326ed-0x3271e */
        heap_free_host(*(uint32_t *)(base + 0x68 + (size_t)i * 4));   /* +0x68 array */
        heap_free_host(*(uint32_t *)(base + 0x1f8 + (size_t)i * 4));  /* +0x1f8 array */
    }
    return base;                                         /* 0x3271e-0x32721 */
}
/* 0x042a30: same shape, but CALL const_64004 -> writes [0x64004]. */
void fn_042a30(uint32_t x) { g_dat_64004 = x; }
/* 0x042a3c: CALL [0x533bd]; RET. */
uint32_t fn_042a3c(uint32_t x) { return g_dat_533bd(x); }

/* 0x047d24: register-ABI wrapper around fx_mul64 (0x47d4e). Entry: a=EAX, b=EDX,
 * out=EBX. esi = (B[8]<<16) | A[8]. The POP-EBX idiom: EBX (the out ptr) is
 * pushed, temporarily reused to hold B[0] (the b_lo arg to fx_mul64), then POPped
 * back before writing the result. Writes out[0]=rlo, out[4]=rhi, and out[8](word)
 * = SI = the returned ESI low16 (the multiply's normalized result exp), NOT A[8] --
 * the earlier port wrote A[8] (latent bug: 0x47d4e clobbers ESI to return the exp,
 * so the true out->exp is the result exp). */
void fn_047d24(uint32_t *a, uint32_t *b, uint32_t *out) {
    uint16_t a8 = *(uint16_t *)(a + 2);                 /* 0x47d33 A[8] */
    uint16_t b8 = *(uint16_t *)(b + 2);                 /* 0x47d27 B[8] */
    uint32_t esi = ((uint32_t)b8 << 16) | a8;           /* 0x47d30/47d33 */
    fx64_t o;
    fn_047d4e(a[0], a[1], b[0], b[1], esi, &o);         /* 0x47d3c CALL 0x47d4e (ESI=result exp) */
    out[0] = o.lo;                                      /* 0x47d42 MOV [EBX],EAX */
    out[1] = o.hi;                                      /* 0x47d44 MOV [EBX+4],EDX */
    *(uint16_t *)(out + 2) = (uint16_t)o.exp;           /* 0x47d47 MOV word[EBX+8],SI (returned exp) */
}

/* 0x044bb4: node flag-setter. param_1 (EAX) = node ptr.
 * if (node[0x0d] & 0x20) return;
 * if (ioctl_device(node[0x10]) == 0) return;   ; node[0x10] = handle
 * node[0x0d] |= 0x20;
 * if ((node[0x0d] & 7) == 0) node[0x0d] |= 0x2;
 * (The original tests BL = b|0x20 for the low-3-bit check; 0x20 doesn't affect bits
 * 0-2, so the second store yields b|0x22 only when b&7==0.) */
void fn_044bb4(uint8_t *node) {
    if (node[0x0d] & 0x20) return;                                /* 0x044bb9 */
    if (ioctl_device(*(uint32_t *)(node + 0x10)) == 0) return;    /* 0x044bbf/0x044bc2 */
    node[0x0d] |= 0x20;                                           /* 0x044bcb/bce/bd1 */
    if ((node[0x0d] & 7) == 0) node[0x0d] |= 0x2;                 /* 0x044bd4/bd9/bdb/bde */
}

/* 0x042a43: device/heap node initializer (38 ins). Byte offsets: +0x0 data(out),
 * +0x4 (out=0), +0x8 data ptr, +0xc heap flag byte (|=0x8 on malloc success), +0xd
 * flags byte, +0x10 device handle, +0x14 alloc size, +0x18 inline fallback storage.
 * fn_044bb4(node); if size==0 pick size from flags (0x86 if &0x2, 1 if &0x4, else
 * 0x1000); data=heap_malloc(size); on success node[0xc]|=0x8; on failure use inline
 * storage (+0x18), size=1, flags=(flags&0xf8)|0x4. Finally +0x4=0, +0x0=data.
 * Returns data (EAX at RET). */
uint32_t fn_042a43(uint8_t *node) {
    fn_044bb4(node);                                            /* 0x042a47 */
    uint32_t size = *(uint32_t *)(node + 0x14);
    if (size == 0) {                                            /* 0x042a4c */
        uint8_t f = node[0x0d];
        if (f & 0x2) size = 0x86;                               /* 0x042a5a */
        else if (f & 0x4) size = 1;                             /* 0x042a68 */
        else size = 0x1000;                                     /* 0x042a71 */
        *(uint32_t *)(node + 0x14) = size;
    }
    uint32_t data = heap_malloc(size);                          /* 0x042a78/2a7b */
    *(uint32_t *)(node + 0x08) = data;                          /* 0x042a80 */
    if (data == 0) {                                            /* 0x042a85 */
        *(uint32_t *)(node + 0x14) = 1;                          /* 0x042a8d */
        *(uint32_t *)(node + 0x08) = (uint32_t)(uintptr_t)(node + 0x18); /* 0x042a87/2a97 */
        node[0x0d] = (node[0x0d] & 0xf8) | 0x4;                 /* 0x042a9a/2aa2 */
    } else {
        node[0x0c] |= 0x8;                                      /* 0x042aa7 */
    }
    uint32_t d_final = *(uint32_t *)(node + 0x08);              /* 0x042aab: EAX=[EDX+0x8] (re-read) */
    *(uint32_t *)(node + 0x04) = 0;                             /* 0x042aae */
    *(uint32_t *)(node + 0x00) = d_final;                       /* 0x042ab5 */
    return d_final;
}

/* 0x042928: node lseek-offset compute (25 ins). node = arg (byte offsets:
 * +0x4 "bytes/offset" field, +0xd flags, +0x10 device handle).
 * r = fn_044848(device) (lseek position; -1 on error).
 * if (r == -1) return -1;
 * if (node[+0x4] == 0) return r;
 * if (node[+0xd] & 0x10) return node[+0x4] + r;
 * return r - node[+0x4]; */
int32_t fn_042928(uint8_t *node) {
    int32_t r = fn_044848(*(uint32_t *)(node + 0x10), 0, 0, 0);  /* 0x04292d-30 */
    if (r == -1) return -1;                                       /* 0x042937-3a */
    int32_t field4 = (int32_t)*(uint32_t *)(node + 0x04);         /* 0x04293c */
    if (field4 == 0) return r;                                     /* 0x04293f-41 */
    if (node[0x0d] & 0x10) return field4 + r;                      /* 0x042943-49 */
    return r - field4;                                             /* 0x042950-52 */
}

/* 0x04285c (L6, 136 B): io_state / node destructor. a0 = node (real ptr), a1 = close flag (EDI).
 *   if (*(u32*)(a0+0xc) == 0) return -1;          // gate: flags dword
 *   esi = 0;
 *   if (a0[0xd] & 0x10) esi = fn_044748(a0);      // flush (F13 bit4)
 *   r = fn_042928(a0);                            // lseek-offset compute
 *   if (r != -1) fn_044807(a0->0x10, r, 0);       // lseek(handle, r, whence=0)
 *   if (a1) esi |= fn_044bec(a0->0x10);           // close handle
 *   if (a0[0xc] & 0x8) { heap_free_host(a0->0x8); a0->0x8 = 0; }   // free region
 *   if (a0[0xd] & 0x8) { tmp_filename(buf, a0[0x19]); fn_044c1e(buf); } // delete temp file
 *   return esi;
 * All callees are ported; the sys file ops (lseek/close/delete) are no-ops in the flat
 * model, so the observable effects are the return value (esi) and the a0->0x8 nulling.
 * a0->0x8 holds a heap offset (heap_free_host takes an offset, cf. 0x034eb0). */
int32_t fn_04285c(uint8_t *a0, int32_t a1) {
    if (*(uint32_t *)(a0 + 0xc) == 0) return -1;        /* 0x42867-72 */
    int32_t esi = 0;
    if (a0[0xd] & 0x10) esi = fn_044748((io_state_t *)a0);  /* 0x42879-85 */
    int32_t r = fn_042928(a0);                          /* 0x42887-8e */
    if (r != -1) fn_044807(*(uint32_t *)(a0 + 0x10), r, 0); /* 0x42890-9a */
    if (a1 != 0) esi |= fn_044bec(*(uint32_t *)(a0 + 0x10)); /* 0x4289f-ab */
    if (a0[0xc] & 0x8) {                                /* 0x428ad-b1 */
        heap_free_host(*(uint32_t *)(a0 + 0x8));        /* 0x428b3-b6 */
        *(uint32_t *)(a0 + 0x8) = 0;                    /* 0x428bb */
    }
    if (a0[0xd] & 0x8) {                                /* 0x428c2-c6 */
        char buf[0x10];
        tmp_filename(buf, a0[0x19]);                    /* 0x428c8-cf */
        fn_044c1e(buf);                                 /* 0x428d4-d6 */
    }
    return esi;                                          /* 0x428db */
}

/* 0x042717 (L7, 103 B): find the pool node whose record == a0.
 *  - Walk the ACTIVE list (head g_node_list_active=0x63ffc); if node->rec==a0 and
 *    the record's flags byte (rec+0xc) has bit 0x3 set, destruct the record via
 *    fn_04285c(rec,1). Return a0.
 *  - Else walk the DONE list (head g_node_list_done=0x63ff8); if node->rec==a0,
 *    move the node to the front of the active list (reactivate) and return a0.
 *    (Original overwrites the done head with node->next regardless of position --
 *    only correct when the found node is the done head; latent quirk, ported 1:1.)
 *  - Else not found: fn_042a0d(4) (g_64008=4), return 0.
 * Nodes are nd_node {next@+0, rec@+4} at g_ndpool+off; the record is io_state at
 * g_ndpool+rec. */
uint32_t fn_042717(uint32_t a0) {
    uint8_t *base = g_ndpool;
    uint32_t cur = g_node_list_active;                     /* 0x63ffc */
    while (cur != 0) {                                      /* 0x42721 */
        struct nd_node *node = (struct nd_node *)(base + cur);
        if (node->rec == a0) {
            if (base[node->rec + 0xc] & 0x3) {             /* 0x4272c-39 */
                fn_04285c(base + node->rec, 1);
            }
            return a0;                                     /* 0x4273e-43 */
        }
        cur = node->next;                                  /* 0x42744-46 */
    }
    uint32_t dcur = g_node_list_done;                      /* 0x63ff8 */
    while (dcur != 0) {                                    /* 0x4274d */
        struct nd_node *node = (struct nd_node *)(base + dcur);
        if (node->rec == a0) {                             /* 0x42753-56 */
            uint32_t old_active = g_node_list_active;      /* 0x4275c */
            g_node_list_done = node->next;                 /* 0x4275a (latent quirk) */
            node->next = old_active;                       /* 0x42767 */
            g_node_list_active = dcur;                     /* 0x42762 */
            return a0;                                     /* 0x42769 */
        }
        dcur = node->next;                                 /* 0x4276b-6d */
    }
    fn_042a0d(4);                                          /* 0x4276f-74 */
    return 0;
}

/* 0x0427d2 (L7, 20 B): node destruct + deactivate, returning the destructor's
 * result.  r = fn_04285c(a0, a1); node_deactivate(a0); return r.  Dual-convention:
 * the original passes the same EAX (a 32-bit node pointer) to both fn_04285c (node
 * ptr) and node_deactivate (rec offset) -- identical in the DPMI 32-bit flat model.
 * In the 64-bit port a0 is truncated to 32 bits for the node_deactivate call to
 * mirror the original EAX (a real 64-bit ptr can't double as a small rec offset). */
int32_t fn_0427d2(uint8_t *a0, int32_t a1) {
    int32_t r = fn_04285c(a0, a1);
    node_deactivate((uint32_t)(uintptr_t)a0);   /* same 32-bit value as orig EAX */
    return r;
}

/* 0x0427a3 (L7, 46 B): walk the ACTIVE node list; find the node whose rec == a0
 * and destruct+deactivate it via fn_0427d2(base+rec, 1); return that result. If
 * the active list is empty or no node matches, return -1.
 * orig: EAX=a0; head=[0x63ffc] (a pointer in the 32-bit model); node->next@+0,
 * node->rec@+4; fn_0427d2(rec_ptr, 1). Port: head is an offset into g_ndpool, so
 * the record passed to fn_0427d2 is base+node->rec. */
int32_t fn_0427a3(uint32_t a0) {
    uint8_t *base = g_ndpool;
    uint32_t cur = g_node_list_active;
    while (cur != 0) {
        struct nd_node *node = (struct nd_node *)(base + cur);
        if (node->rec == a0) {
            return fn_0427d2(base + node->rec, 1);
        }
        cur = node->next;
    }
    return -1;
}

/* 0x037306 (L11, 64 B): resource-reader cleanup / shutdown. If g_639b8 (the
 * resource-reader node) is non-NULL, destruct+deactivate it via
 * fn_0427a3(g_639b8), then run the DPMI real-mode cleanup fn_00040570() and
 * re-install the IRQ 9 (fn_03ffab) and IRQ 0x1c (fn_03eff2) handlers; else
 * return early. All four callees are flat-model no-ops (DPMI/IRQ installers), so
 * the only flat-model observable is the guard -> smoke test. g_639b8 is a
 * res_reader *; the original passes it (a 32-bit ptr) as fn_0427a3's rec, so it
 * is truncated to 32 bits here (dual-convention, cf. fn_0427d2). */
void fn_037306(void) {
    (void)canary_id(0, 0x1c);
    if (g_639b8 == NULL) return;
    fn_0427a3((uint32_t)(uintptr_t)g_639b8);
    fn_00040570();
    fn_03ffab();
    fn_03eff2();
}

int g_dat_53390 = 0;                                    /* DAT_00053390 */
uint32_t (*g_cb_53644)(void) = NULL;                    /* DAT_00053644 */
int get_dat_53390(void) { return g_dat_53390; }         /* 0x44be6 */
char **g_dat_537d4 = NULL;                                /* DAT_000537d4: config table */
uint32_t cb_indirect_53644(void) { return g_cb_53644 ? g_cb_53644() : 0; } /* 0x451b5 */

/* ---- L0 batch D (9): node list, modifier-prefix parser, debug hook ---- */
/* 0x010927 (L13, 302 B): CD/MSCDEX root-directory read. canary PUSH 0x44;CALL 0x418ee.
 * Disassembly-verified (63 insns): only direct CALLs (0x0418ee canary x2, 0x041e6c
 * mem_copy x2, 0x010570/0x0107b9 CD no-ops, 0x041c28 heap_free_guard, 0x041e94
 * heap_malloc thunk), NO indirect. bufA(7B) <- ROM 0x51acc; bufB(26B) <- ROM 0x51ad4;
 * [0x53c5c] <- bufA; fn_010570(bufB,7) [no-op]; bufA <- [0x53c5c]; count=(bufA[2]-bufA[1])
 * +1 -> DAT_00053c60; free old DAT_00053c64; DAT_00053c64 = heap_malloc((count+1)*5);
 * for i<count fn_0107b9(bufA[1]+i, DAT_00053c64+i*5) [no-op; port fn_0107b9 takes void,
 * so frame/dest are discarded]; final dword at g_heap_base + DAT_00053c64 + count*5 + 1
 * = bufA[3] + bufA[4]*0x4b + bufA[5]*0x1194 - 0x96. ROM bufA={0x0a,0,0,0,0,0,0} -> count=1. */
uint32_t g_dat_53c64 = 0;        /* DAT_00053c64: dir-entry buffer heap offset */
static const uint8_t g_rom_51acc[7]  = {0x0a, 0, 0, 0, 0, 0, 0};
static const uint8_t g_rom_51ad4[26] = {0x1a, 0, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
void fn_010927(void) {
    (void)canary_id(0, 0x44);
    uint8_t bufA[7], bufB[26];
    uint8_t *rsp = (uint8_t *)(uintptr_t)g_dat_53c5c;            /* DPMI far-pointer */
    mem_copy(bufA, g_rom_51acc, 7);                              /* 0x01093f-4a */
    mem_copy(bufB, g_rom_51ad4, 26);                             /* 0x010950-5a */
    mem_copy(rsp, bufA, 7);                                      /* 0x010977-84: *g_dat_53c5c <- bufA */
    fn_10570(bufB, 7);                                           /* 0x010989-91 (no-op) */
    mem_copy(bufA, rsp, 7);                                      /* 0x010996-a4: bufA <- *g_dat_53c5c */
    g_dat_53c60 = (uint32_t)bufA[2] - (uint32_t)bufA[1] + 1u;    /* 0x0109a9-b6 */
    if (g_dat_53c64 != 0) heap_free_guard(g_dat_53c64);          /* 0x0109bc-ca */
    g_dat_53c64 = heap_malloc((g_dat_53c60 + 1) * 5);            /* 0x0109cf-dd */
    for (uint32_t i = 0; i < g_dat_53c60; i++) {                 /* 0x0109e2-a18 */
        (void)((uint32_t)bufA[1] + i);                           /* frame (discarded) */
        (void)(g_dat_53c64 + i * 5);                             /* dest (discarded) */
        fn_107b9();                                              /* 0x0107b9 no-op */
    }
    uint32_t val = (uint32_t)bufA[3] + (uint32_t)bufA[4]*0x4b + (uint32_t)bufA[5]*0x1194 - 0x96u;
    *(uint32_t *)(g_heap_base + g_dat_53c64 + g_dat_53c60 * 5 + 1) = val;  /* 0x010a18-4a */
}

/* 0x04933a (119 B): rebuild the active node list from the file-structure table.
 * No canary (only PUSH EBX/ECX/EDX). Disassembly-verified (38 insns, disassemble_bytes):
 * only direct CALLs to 0x041af8 (heap_malloc) and 0x0429ea (error msg), no indirect.
 *   g_dat_53451 = (g_dat_53451 & 0xf8) | 0x4;
 *   ecx = 0x53410;   // file-structure table base (offset within g_ndpool)
 *   while (*(g_ndpool + ecx + 0xc) != 0) {
 *       off = heap_malloc(8) [retry once; on 2nd fail fn_0429ea(msg,1) then fall
 *           through with off==0 -- flat orig crashes writing addr 4; the port writes
 *           g_heap_base+4 (flat-model artifact, never exercised since malloc succeeds)].
 *       node = g_heap_base + off;
 *       node[+4] = ecx;             // rec -> record offset (0x0427a3 reads g_ndpool+rec)
 *       g_node_list_active = off;   // prepend
 *       ecx += 0x1a;
 *       node[+0] = old_head;        // next
 *   }
 *   g_node_list_done = 0;           // = the zero sentinel that ended the loop
 * Data model: 0x53410 is a data-segment record table (0x1a-byte records); the record
 * base is g_ndpool + 0x53410 (0x0427a3 consumes node->rec as g_ndpool+rec). Nodes are
 * heap-allocated (g_heap_base + off). */
extern void fn_0429ea(const char *msg, uint32_t param2);
uint8_t  g_dat_53451 = 0;   /* DAT_00053451: file-structure flag */

void fn_04933a(void) {
    g_dat_53451 = (g_dat_53451 & 0xf8) | 0x4;                 /* 0x4933d-56 */
    uint8_t *base = g_ndpool;
    uint32_t ecx = 0x53410;                                   /* 0x49351 */
    for (;;) {
        if (*(int32_t *)(base + ecx + 0xc) == 0) break;        /* 0x493a1-a6 */
        uint32_t off = heap_malloc(8);                         /* 0x4935e-63 */
        if (off == 0) {
            off = heap_malloc(8);                             /* 0x4936e-73 retry */
            if (off == 0)
                fn_0429ea("Not enough memory to allocate file structures\r\n", 1); /* 0x4937e-88 */
        }
        uint32_t old_head = g_node_list_active;               /* 0x4938d */
        *(uint32_t *)(g_heap_base + off + 4) = ecx;           /* 0x49393 rec */
        g_node_list_active = off;                             /* 0x49396 prepend */
        ecx += 0x1a;                                          /* 0x4939c */
        *(uint32_t *)(g_heap_base + off) = old_head;          /* 0x4939f next */
    }
    g_node_list_done = 0;                                     /* 0x493a8 sentinel */
}

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

/* 0x043234: walk the active node list (head g_node_list_active, offset-based via
 * g_ndpool). For each node, rec = base + node->rec (the io_state record). If
 * (rec[0xc] & mask) count++; if additionally (rec[0xd] & 0x10) call fn_044748(rec).
 * Returns the count of matching records. (orig: EAX=mask, returns EBX.) */
/* 0x041d47 (170 B): I/O-state "fill/prime" step. node = io_state record
 * (raw offsets: f0@+0 data ptr, f4@+4 count, f8@+8 buffer, f12@+0xc/0xd flags,
 * f16@+0x10 handle, +0x14 reserved count).
 *   if (node[+8] == 0) fn_042a43(node);                    // lazy-init buffer
 *   if ((node[0x0d]&0x20) && (node[0x0d]&0x6)) fn_043234(0x2000); // flush active list
 *   f12 = node[+0xc]; node[+0] = node[+8]; node[0x0c] &= 0xfb;
 *   if ((f12 & 0x2400) == 0x2400 && node[+0x10] == 0) {    // key-echo branch
 *       node[+4] = 0;
 *       key = get_key_echo();                              // DOS AH=01, -1 on no-key
 *       if (key != -1) { *node[+0] = key; node[+4] = 1; }
 *   } else {                                               // device-read branch
 *       count = (node[0x0d] & 0x4) ? 1 : node[+0x14];
 *       node[+4] = fn_042aba(node[+0x10], node[+0], count);
 *   }
 *   if (node[+4] == 0) node[0x0c] |= 0x10;                 // empty-result flag
 *   else if (node[+4] < 0) { node[+4] = 0; node[0x0c] |= 0x20; } // error flag
 *   return node[+4];
 * Buffer (node[+0]/[+8]) is dereferenced directly as a host pointer (flat model),
 * matching fn_044748. param_2 (EDX input) is unused. No canary prologue. */
/* 0x041d18 (46 B): pull one byte from the node buffer. Calls fn_041d47 to
 * fill/prime the node; if node[+4] (result count) is 0, return -1 (empty).
 * Else read the byte at the buffer (node[+0], a heap offset -> g_heap_base+off),
 * advance node[+0] by 1, decrement node[+4], and return the byte. The original
 * 8-byte return (CONCAT44) has high32 = the EDX input (the node ptr) -- an
 * ABI artifact; only the low 32 (byte / -1) is meaningful. Sole callee:
 * fn_041d47 (ported this pass). */
/* 0x049192 (163 B): putc -- write one char to a node I/O buffer.
 * reg-ABI: EBX=char (param_1), EDX=node (param_2, io_state_t*). Returns the
 * char (0-255) on success, -1 on error. Buffer fields f0/f8 are heap offsets
 * (dereferenced via g_heap_base). Disassembly-verified (mask is ECX, callee-
 * saved; the decompiler's "uVar1 = extraout_ECX" is a register artifact):
 *   if (node[0x0c] & 0x2)==0: fn_042a0d(4); node[0x0c]|=0x20; return -1;
 *   if (f8==0) fn_042a43(node);                 // lazy init
 *   mask = 0x400;
 *   if (ch==10 && (node[0x0c]&0x40)==0) {       // LF, no f12 bit6: CRLF expand
 *       mask = 0x600; node[0x0d]|=0x10;
 *       *(u8*)(base+f0)=0x0d; f0++; f4++;
 *       if (f4==cap && fn_044748(node)!=0) return -1;
 *   }
 *   node[0x0d]|=0x10; *(u8*)(base+f0)=ch; f0++; f4++;
 *   if ((mask & f12)!=0 || f4==cap) { if (fn_044748(node)!=0) return -1; }
 *   return ch;
 * cap = node[+0x14]. Callees: fn_042a0d, fn_042a43, fn_044748 (all ported). */
uint32_t fn_049192(uint32_t ch, uint8_t *node) {
    uint32_t *n = (uint32_t *)node;              /* n[0]=f0 n[1]=f4 n[2]=f8 n[3]=f12 n[5]=cap */
    if ((node[0x0c] & 0x02) == 0) {              /* f12 byte0 bit1 clear -> error */
        fn_042a0d(4);
        node[0x0c] |= 0x20;
        return 0xffffffffu;
    }
    if (n[2] == 0) {                              /* f8 (buffer start) unset -> init */
        fn_042a43(node);
    }
    uint32_t mask = 0x400;                        /* ECX */
    if ((int32_t)ch == 10 && (node[0x0c] & 0x40) == 0) {   /* LF, no f12 bit6 */
        mask = 0x600;
        node[0x0d] |= 0x10;                        /* f12 byte1 bit4 */
        uint32_t o = n[0];                         /* f0 cursor */
        *(uint8_t *)(g_heap_base + o) = 0x0d;      /* buffer = '\r' */
        n[0] = o + 1;
        n[1] = n[1] + 1;
        if (n[1] == n[5]) {                        /* f4 == capacity */
            if (fn_044748((io_state_t *)node) != 0) return 0xffffffffu;
        }
    }
    node[0x0d] |= 0x10;                            /* f12 byte1 bit4 */
    uint8_t c = (uint8_t)ch;
    uint32_t o = n[0];                             /* f0 cursor */
    *(uint8_t *)(g_heap_base + o) = c;             /* buffer = char */
    n[0] = o + 1;
    n[1] = n[1] + 1;
    if (((mask & n[3]) != 0) || (n[1] == n[5])) {  /* (mask & f12) or full */
        if (fn_044748((io_state_t *)node) != 0) return 0xffffffffu;
    }
    return c;
}

/* 0x0446b2 (16 B): putc + node write-counter. reg-ABI (arg1 = EAX = p, arg2
 * = EDX = ch). EAX <- ch, EDX <- [p] (the node pointer, stored as a heap
 * offset), CALL fn_049192 (putc to that node), then INC [p+0x10] (the node's
 * write counter). Returns fn_049192's result (EAX, untouched by the INC).
 * orig: 0x0446b2 */
uint32_t fn_0446b2(uint8_t *p, uint32_t ch)
{
    uint32_t r = fn_049192(ch, (uint8_t *)g_heap_base + *(uint32_t *)p);   /* 0x0446b7-b9 */
    (*(uint32_t *)(p + 0x10))++;                                            /* 0x0446be */
    return r;
}

/* 0x041c8f (136 B): getch -- read one char from a node I/O buffer (with
 * CR-consume and EOF handling). reg-ABI: node in EAX (copied to EDX); returns the
 * byte (0-255) or -1 (0xffffffff) in EAX. f0 (node[+0]) is a heap offset
 * (dereferenced via g_heap_base); f4 (node[+4]) = remaining count; f12 byte0
 * (node[+0xc]) holds flags. Disassembly-verified (55 insns):
 *   if (node[0x0c] & 0x1)==0: fn_042a0d(4); node[0x0c]|=0x20; return -1;
 *   count=f4; f4=count-1;
 *   if (count-1<0) b=fn_041d18(node);          // refill
 *   else { b=*(u8*)(base+f0); f0++; }
 *   if ((node[0x0c] & 0x40)==0) {
 *     if (b==0x0d) { c=f4; f4=c-1;               // CR: consume next byte
 *       if (c-1<0) b=fn_041d18(node); else { b=*(u8*)(base+f0); f0++; } }
 *     if (b==0x1a) { node[0x0c]|=0x10; b=-1; }    // EOF
 *   }
 *   return b;
 * Callees: fn_042a0d, fn_041d18 (all ported). The decompiler's second param is
 * an ABI artifact -- disassembly uses only EAX (node) and returns EAX. */
uint32_t fn_041c8f(uint8_t *node) {
    uint32_t *n = (uint32_t *)node;              /* n[0]=f0 n[1]=f4 */
    if ((node[0x0c] & 0x01) == 0) {              /* f12 byte0 bit0 clear -> error */
        fn_042a0d(4);
        node[0x0c] |= 0x20;
        return 0xffffffffu;
    }
    int32_t count = (int32_t)n[1];               /* f4 */
    n[1] = (uint32_t)(count - 1);
    uint32_t b;
    if (count - 1 < 0) {                          /* buffer empty -> refill */
        b = (uint32_t)fn_041d18(node, 0);
    } else {
        b = *(uint8_t *)(g_heap_base + n[0]);
        n[0] = n[0] + 1;
    }
    if ((node[0x0c] & 0x40) == 0) {               /* f12 byte0 bit6 clear */
        if (b == 0x0d) {                           /* CR: consume next byte */
            int32_t c2 = (int32_t)n[1];
            n[1] = (uint32_t)(c2 - 1);
            if (c2 - 1 < 0) {
                b = (uint32_t)fn_041d18(node, 0);
            } else {
                b = *(uint8_t *)(g_heap_base + n[0]);
                n[0] = n[0] + 1;
            }
        }
        if (b == 0x1a) {                           /* EOF */
            node[0x0c] |= 0x10;
            b = 0xffffffffu;
        }
    }
    return b;
}

int32_t fn_041d18(uint8_t *node, uint32_t param_2) {
    (void)param_2;
    int32_t res = fn_041d47(node, (uint32_t)(uintptr_t)node);   /* 0x041d1e */
    if (res == 0) return -1;                                       /* empty */
    uint32_t o = *(uint32_t *)(node + 0x00);                       /* buffer offset */
    int32_t b = (int32_t)*(uint8_t *)(g_heap_base + o);            /* *pbVar1 */
    *(uint32_t *)(node + 0x04) = *(uint32_t *)(node + 0x04) - 1u;  /* count-- */
    *(uint32_t *)(node + 0x00) = o + 1;                            /* advance */
    return b;
}

int32_t fn_041d47(uint8_t *node, uint32_t param_2) {
    (void)param_2;
    if (*(uint32_t *)(node + 0x08) == 0) fn_042a43(node);                 /* 0x41d4d-53 */
    uint8_t b13 = node[0x0d];
    if ((b13 & 0x20) && (b13 & 0x6)) fn_043234(0x2000);                   /* 0x41d58-6a */
    uint32_t f12 = *(uint32_t *)(node + 0x0c);                            /* 0x41d6f EDX=[+0xc] */
    uint32_t buf = *(uint32_t *)(node + 0x08);                            /* 0x41d75 EAX=[+8] */
    node[0x0c] = (uint8_t)f12 & 0xfb;                                     /* 0x41d72/78/83 BL&0xfb */
    *(uint32_t *)(node + 0x00) = buf;                                     /* 0x41d7b f0 = f8 */
    if ((f12 & 0x2400) == 0x2400 && *(uint32_t *)(node + 0x10) == 0) {     /* 0x41d86-93 */
        *(uint32_t *)(node + 0x04) = 0;                                    /* 0x41d95 */
        int32_t key = (int32_t)(int8_t)get_key_echo();                    /* 0x41d98 */
        if (key != -1) {                                                    /* 0x41d9d */
            *((int8_t *)(g_heap_base + *(uint32_t *)(node + 0x00))) = (int8_t)key; /* 0x41da4: *buf = key (buf=heap offset) */
            *(uint32_t *)(node + 0x04) = 1;                                /* 0x41da6 */
        }
    } else {                                                                /* 0x41daf */
        uint32_t count = (node[0x0d] & 0x4) ? 1u : *(uint32_t *)(node + 0x14);
        int32_t r = fn_042aba(*(uint32_t *)(node + 0x10),
                              (void *)(g_heap_base + *(uint32_t *)(node + 0x00)),
                              count);                                        /* 0x41dc4 */
        *(uint32_t *)(node + 0x04) = (uint32_t)r;                          /* 0x41dc9 */
    }
    int32_t res = (int32_t)*(uint32_t *)(node + 0x04);                     /* 0x41dcc */
    if (res == 0) node[0x0c] |= 0x10;                                      /* 0x41dd5 */
    else if (res < 0) { *(uint32_t *)(node + 0x04) = 0; node[0x0c] |= 0x20; } /* 0x41ddb-e7 */
    return (int32_t)*(uint32_t *)(node + 0x04);                            /* 0x41dea */
}

uint32_t fn_043234(uint32_t mask) {
    uint8_t *base = g_ndpool;
    uint32_t cur = g_node_list_active;            /* 0x43239 EDX=[0x63ffc] */
    uint32_t count = 0;                            /* 0x4323f XOR EBX,EBX */
    while (cur != 0) {                             /* 0x43259 TEST EDX,EDX / JNZ 0x43243 */
        struct nd_node *node = (struct nd_node *)(base + cur);
        uint8_t *rec = base + node->rec;           /* 0x43243 EAX=[EDX+4] */
        if (rec[0xc] & mask) {                     /* 0x43246 TEST [EAX+0xc],ECX */
            count++;                                /* 0x4324b INC EBX */
            if (rec[0xd] & 0x10)                   /* 0x4324c TEST [EAX+0xd],0x10 */
                fn_044748((io_state_t *)rec);       /* 0x43252 CALL 0x044748 */
        }
        cur = node->next;                          /* 0x43257 EDX=[EDX] */
    }
    return count;                                  /* 0x4325d MOV EAX,EBX */
}

const uint8_t *parse_mod_prefix(const uint8_t *s, uint8_t *out)   /* 0x44fd7 */
{
    out[0x14] = 0;                                 /* 0x44fd9 init flags */
    for (;;) {
        uint8_t c = *s++;                          /* 0x44fdd BL=[EAX] */
        if (c == 0x2d) { out[0x14] |= 0x8; continue; }        /* '-' 0x44fdf */
        if (c == 0x23) { out[0x14] |= 0x1; continue; }        /* '#' 0x44fea */
        if (c == 0x2b) { out[0x14] = (out[0x14] | 0x4) & ~0x2; continue; } /* '+' 0x44ff5 */
        if (c == 0x20) { if (out[0x14] & 0x4) continue; out[0x14] |= 0x2; continue; } /* ' ' 0x4500d */
        if (c == 0x30) { out[0x16] = c; continue; }           /* '0' 0x45024 */
        return s - 1;                                     /* 0x4502f: EAX = break char pos */
    }
}

/* 0x044ea4: parse a numeric conversion specifier following the flags (from
 * parse_mod_prefix). param_1 = format position, param_2 = vararg cursor (pointer
 * to a pointer to the current int vararg), param_3 = spec struct (byte offsets):
 *   [4] width (int), [8] precision (int, -1 default), [0x14] flags, [0x16] width-fill
 *   (default ' ', '0' if '0' flag), [0x17] (cleared at start).
 * Returns the position after the parsed suffix (or at an unrecognized suffix char). */
uint8_t *fn_044ea4(const uint8_t *s, uint32_t **va, uint8_t *spec) {
    spec[0x17] = 0;                                   /* init */
    spec[0x16] = 0x20;                                /* default width-fill ' ' */
    const uint8_t *p = parse_mod_prefix(s, spec);
    *(uint32_t *)(spec + 4) = 0;                      /* width = 0 */
    if (*p < 0x30 || *p > 0x39) {                     /* not a digit */
        if (*p == 0x2a) {                             /* '*' width from vararg */
            uint32_t *arg = (uint32_t *)(*va);        /* P = *va */
            *va = arg + 1;
            int32_t v = (int32_t)*arg;
            *(uint32_t *)(spec + 4) = (uint32_t)v;
            if (v < 0) { *(uint32_t *)(spec + 4) = (uint32_t)(-v); spec[0x14] |= 8; }
            p = p + 1;                                /* consume '*' */
        }
    } else {
        do {                                           /* parse decimal width */
            uint8_t c = *p;
            p = p + 1;
            *(uint32_t *)(spec + 4) = *(uint32_t *)(spec + 4) * 10 + (c - 0x30);
        } while (*p >= 0x30 && *p <= 0x39);
    }
    *(uint32_t *)(spec + 8) = 0xffffffff;             /* precision = -1 (default) */
    const uint8_t *q = p;
    if (*p == 0x2e) {                                 /* '.' precision */
        q = p + 1;
        *(uint32_t *)(spec + 8) = 0;
        if (*q == 0x2a) {                             /* '.' '*' from vararg */
            uint32_t *arg = (uint32_t *)(*va);
            *va = arg + 1;
            int32_t v = (int32_t)*arg;
            *(uint32_t *)(spec + 8) = (uint32_t)v;
            if (v < 0) *(uint32_t *)(spec + 8) = 0xffffffff;
            q = p + 2;                                /* consume '.' '*' */
        } else {
            while (*q > 0x2f && *q < 0x3a) {          /* '.' digits */
                *(uint32_t *)(spec + 8) = *(uint32_t *)(spec + 8) * 10 + (*q - 0x30);
                q = q + 1;
            }
        }
    }
    uint8_t sc = *q;                                  /* suffix char */
    const uint8_t *after = q + 1;
    if (sc < 0x4e) {                                  /* < 'N' */
        if (sc > 0x45) {                              /* > 'E' */
            if (sc < 0x47) spec[0x14] |= 0x80;        /* 'F' */
            else {
                if (sc != 0x4c) return (uint8_t *)q;  /* 'G'..'M' except 'L': at char */
                spec[0x14] |= 0x40;                   /* 'L' */
            }
            return (uint8_t *)after;                  /* 'F' or 'L': consumed */
        }
        return (uint8_t *)q;                          /* <= 'E': at char */
    }
    if (sc > 0x4e) {                                  /* > 'N' */
        if (sc < 0x6c) {                              /* < 'l' */
            if (sc == 0x68) { spec[0x14] |= 0x10; return (uint8_t *)after; } /* 'h' */
            return (uint8_t *)q;
        }
        if (sc > 0x6c && sc != 0x77) return (uint8_t *)q;  /* > 'l' and != 'w' */
        spec[0x14] |= 0x20;                           /* 'l' or 'w' */
        return (uint8_t *)(q + 1);
    }
    spec[0x14] |= 0x40;                               /* 'N' */
    return (uint8_t *)after;
}

/* 0x049b84: parse a [J|M]N[.N[.N]][/month:minute:second] date/time string.
 * Output struct (uint32_t*, byte offsets):
 *   [0x20] era (1='J', 0='M', -1=neither)
 *   [0x1c] (J/neither) first number; (M) 0
 *   [0x10] (M) first number - 1
 *   [0x0c] (M, after 1st '.') second number
 *   [0x18] (M, after 2nd '.') third number
 *   [0x08] month (from /part, default 2)
 *   [0x04] minute (from /part)
 *   [0x00] second (from /part)
 * Untouched fields are left as-is. Returns position after the parsed string. */
const char *fn_049b84(const char *s, uint32_t *out) {
    uint32_t era = 0xffffffff;                        /* ESI = -1 (neither J nor M) */
    if (*s == 'J') { era = 1; s = s + 1; }           /* 0x49b95 */
    if (*s == 'M') { s = s + 1; era = 0; }           /* 0x49ba1 */
    out[0x20 / 4] = era;                             /* [ECX+0x20] = era */
    uint32_t val;
    const char *p = parse_decimal(s, &val);          /* first number */
    if (era == 0) {                                  /* 'M' case (ESI==0) */
        out[0x10 / 4] = val - 1;                     /* [ECX+0x10] = first - 1 */
        if (*p == '.') {
            p = parse_decimal(p + 1, &val);          /* 2nd number */
            out[0x0c / 4] = val;                     /* [ECX+0x0c] */
            if (*p == '.') {
                p = parse_decimal(p + 1, &val);      /* 3rd number */
                out[0x18 / 4] = val;                 /* [ECX+0x18] */
            }
        }
        out[0x1c / 4] = 0;                           /* [ECX+0x1c] = 0 */
    } else {                                         /* 'J' or neither */
        out[0x1c / 4] = val;                         /* [ECX+0x1c] = first */
    }
    uint32_t second = 0, minute = 0, month = 2;      /* [ESP+0]=0 [ESP+4]=0 [ESP+8]=2 */
    if (*p == '/') {
        p = parse_decimal(p + 1, &month);            /* [ESP+8] = month */
        if (*p == ':') {
            p = parse_decimal(p + 1, &minute);       /* [ESP+4] = minute */
            if (*p == ':') {
                p = parse_decimal(p + 1, &second);   /* [ESP+0] = second */
            }
        }
    }
    out[0] = second;                                 /* [ECX+0x00] */
    out[1] = minute;                                 /* [ECX+0x04] */
    out[2] = month;                                  /* [ECX+0x08] */
    return p;                                        /* position after parsed string */
}

uint32_t g_dat_536dc = 0;                 /* DAT_000536dc: node index bound */
uint32_t *g_dat_53730 = NULL;            /* DAT_00053730: node table */
uint32_t g_dat_53780 = 0;                /* DAT_00053780: TZ range diff */
uint32_t g_dat_53784 = 0;                /* DAT_00053784: TZ has-second-part flag */
uint32_t g_dat_53788 = 0;                /* DAT_00053788: TZ time prefix buf (4B) */
uint32_t g_dat_537a7 = 0;                /* DAT_000537a7: TZ time2 prefix buf (4B) */
uint32_t g_dat_5377c = 0;                /* DAT_0005377c: TZ seconds */
uint32_t g_dat_53734 = 0;                /* DAT_00053734: TZ count */
uint32_t g_dat_53758 = 0;                /* DAT_00053758: TZ index */
uint8_t  g_dat_533bb = 0;                /* DAT_000533bb: classify-enable threshold */
uint32_t g_dat_53619 = 0;                /* DAT_00053619: 0x200 => default binary */
uint32_t g_dat_536d8 = 0;                /* DAT_000536d8: create-mode xor (load-time 0) */

/* ---- 0x04574a / 0x049506 / 0x0495eb: recurring-event date sub-unit ----
 * These operate on struct dec_datetime with an "Nth weekday of month" recurrence
 * model: [+0x0c]=day is a WEEK index (1-5) and [+0x18]=dow the weekday.
 * 0x04574a normalizes a struct (via fn_0498f6) and returns a unix-epoch-ish
 * offset; 0x049506 maps a recurrence to a day-of-year offset; 0x0495eb compares
 * two. The 0x049613 helper (236 ins) is reached only when mode<0 and is stubbed
 * (deferred); the sub-unit tests run with mode>=0. */
static const int16_t g_nonleap_tbl[13] = { 0,31,59,90,120,151,181,212,243,273,304,334,365 }; /* DAT_00051960 */
static const int16_t g_leap_tbl[13]    = { 0,31,60,91,121,152,182,213,244,274,305,335,366 };  /* DAT_0005197a */

/* 0x049613: recurring-event date arithmetic (236 ins). DEFERRED to a fresh
 * context. Called by fn_04574a only when d->mode<0; the date-compare sub-unit
 * always uses mode>=0, so this stub is never reached by the differential tests
 * and exists only to satisfy the link. */
void fn_049613(struct dec_datetime *d) { (void)d; }

int32_t fn_04574a(struct dec_datetime *d) {
    int32_t r_raw = d->month % 12;                 /* 0x45757-60: IDIV rem (sign of dividend) */
    int32_t r = r_raw;
    int32_t q = d->month / 12;
    int32_t year = d->year;
    int32_t EDI;                                   /* running day count */
    int32_t total_sec;

    if (year < (int32_t)0xf4fb7ea1) return -1;     /* 0x4576c-79: sanity */
    year += q;                                     /* 0x4577e-8d */
    while (r < 0) { year--; r += 12; }             /* 0x4578f-97 */
    if (year < 0) return -1;                       /* 0x45799-9b */

    int32_t leap = (int32_t)is_leap((uint32_t)(year + 0x76c), (uint32_t)r_raw); /* 0x4579d-a8 */
    const int16_t *tbl = leap ? g_leap_tbl : g_nonleap_tbl;  /* 0x457ac/b1 */

    int32_t absbase = year * 365 + (year + 3) / 4; /* 0x457b1-ca (Gregorian /100//400 NOT applied) */
    EDI = d->day + (int32_t)tbl[r] + absbase - 1;  /* 0x457cd-d8 */
    if (year != 0) EDI--;                          /* 0x457d9-db */

    total_sec = d->sec + d->min * 60 + d->hour * 3600;   /* 0x457de-fd */
    while (total_sec < 0) { EDI--; total_sec += 0x15180; }  /* 0x457ff-80a */

    fn_0498f6(EDI, (uint32_t)total_sec, 0, d);     /* 0x4580c-14: normalize d in place */
    fn_049a35(0, 0, 0);                            /* 0x45819: refresh TZ globals */
    total_sec += (int32_t)g_dat_5377c;             /* 0x4581e-27 */
    if (d->mode < 0) fn_049613(d);                 /* 0x45824-2f */
    if (d->mode > 0) total_sec -= (int32_t)g_dat_53780;  /* 0x45834-3a */
    while (total_sec < 0) { EDI--; total_sec += 0x15180; }  /* 0x45840-4b */
    if (EDI < 0x63de) return -1;                   /* 0x4584d-53 */
    if (EDI != 0x63de) {                           /* 0x45859 */
        total_sec += (EDI - 0x63df) * 0x15180;     /* 0x45877-93 */
        return total_sec;                          /* 0x45895 */
    }
    int32_t g = (int32_t)g_dat_5377c;              /* 0x4585b (EDI == 0x63de) */
    total_sec -= 0x15180;                          /* 0x45860 */
    if (g <= 0) return -1;                         /* 0x45866-68 */
    if (total_sec >= 0) return total_sec;          /* 0x4586e-70 */
    return -1;                                     /* 0x45872 */
}

int32_t fn_049506(struct dec_datetime *d, int32_t ref_year) {
    int32_t mode = d->mode;                        /* 0x49510 */
    if (mode == 1) return d->doy - 1;              /* 0x495d3-d8 */
    if (mode != 0) return d->doy;                  /* 0x495df */

    int32_t month = d->month;
    int32_t leap = (int32_t)is_leap((uint32_t)(ref_year + 0x76c), (uint32_t)ref_year);  /* 0x4951b-21 */
    int32_t days_in_month = leap                   /* 0x4952a-54 */
        ? (g_leap_tbl[month + 1] - g_leap_tbl[month])
        : (g_nonleap_tbl[month + 1] - g_nonleap_tbl[month]);

    struct dec_datetime temp;                      /* 0x49556-79 */
    temp.sec = 0; temp.min = 0; temp.hour = 0;
    temp.day = 1; temp.month = month; temp.year = ref_year;
    temp.dow = 0; temp.doy = 0; temp.mode = 0;
    fn_04574a(&temp);                              /* 0x4957d: normalizes temp->dow/doy */

    int32_t dow_diff = (d->dow - temp.dow + 7) % 7; /* 0x49586-98: IDIV rem (quotient discarded) */
    int32_t day = d->day;
    int32_t dayval;
    if (day == 5) {                                /* 0x4959d-a0 */
        int32_t cmp = dow_diff + 29;               /* 0x495a2-af: (dow_diff+1)+28 */
        dayval = (cmp <= days_in_month) ? 4 : 3;   /* 0x495b1-bd */
    } else {
        dayval = day - 1;                          /* 0x495c1 */
    }
    return temp.doy + dow_diff + dayval * 7;       /* 0x495c2-cf */
}

int32_t fn_0495eb(struct dec_datetime *x, struct dec_datetime *y, int32_t ref_year) {
    int32_t rx = fn_049506(x, ref_year);           /* 0x495ee-f7 */
    int32_t ry = fn_049506(y, ref_year);           /* 0x495f9-02 */
    return (rx > ry) ? 1 : 0;                      /* 0x49604-0d */
}

/* 0x049a35: timezone ("TZ") config parser. Reads the "TZ" config value (a
 * time-range string), parses it via parse_time_str, and fills the g_dat_537xx
 * TZ globals. The 3 params are part of a callback signature and unused. */
void fn_049a35(uint32_t p1, uint32_t p2, uint32_t p3) {
    (void)p1; (void)p2; (void)p3;
    const char *val = config_value_ptr("TZ");   /* DAT_00051840 = "TZ" */
    if (!val) return;                             /* no TZ config -> early return */
    g_dat_53784 = 0;
    char *pcVar1 = parse_time_str(val, (char *)&g_dat_53788, &g_dat_5377c);
    if (*pcVar1 == 0) {
        g_dat_537a7 = 0;
    } else {
        g_dat_53784 = 1;
        uint32_t local_10 = g_dat_5377c - 0xe10;  /* 0xe10 = 3599 */
        pcVar1 = parse_time_str(pcVar1, (char *)&g_dat_537a7, &local_10);
        g_dat_53780 = g_dat_5377c - local_10;
        if (*pcVar1 == ',') {
            pcVar1 = (char *)fn_049b84(pcVar1 + 1, &g_dat_53734);
        }
        if (*pcVar1 == ',') {
            fn_049b84(pcVar1 + 1, &g_dat_53758);
        }
    }
}

/* 0x049c76: timezone range parser (variant of fn_049a35) taking the string
 * directly. Parses a second time with an initial offset of 0x3840 (14400); the
 * range diff is 18000 - <second secs>. p2/p3/p4 are a callback signature (unused). */
void fn_049c76(const char *s, uint32_t p2, uint32_t p3, uint32_t p4) {
    (void)p2; (void)p3; (void)p4;
    g_dat_53784 = 0;
    char *pcVar1 = parse_time_str(s, (char *)&g_dat_53788, &g_dat_5377c);
    if (*pcVar1 == 0) {
        g_dat_537a7 = 0;
    } else {
        g_dat_53784 = 1;
        uint32_t local_10 = 0x3840;                    /* 14400 (4h) */
        pcVar1 = parse_time_str(pcVar1, (char *)&g_dat_537a7, &local_10);
        g_dat_53780 = 18000u - local_10;               /* 5h - second secs */
        if (*pcVar1 == ',') {
            pcVar1 = (char *)fn_049b84(pcVar1 + 1, &g_dat_53734);
        }
        if (*pcVar1 == ',') {
            fn_049b84(pcVar1 + 1, &g_dat_53758);
        }
    }
}

/* 0x045a82: byte classifier. Sets g_dat_64004 = param_1 (low byte) via fn_042a30,
 * then classifies: if param_1 < 0x100, (when g_dat_533bb > 2) map 'P'->0xe, [0x20,0x22)->5,
 * >=0x22->0x13; clamp to <=0x13; g_dat_64008 = tbl[bl]. Else g_dat_64008 = (param_1>>8)&0xff.
 * Returns (param_2 << 32) | 0xffffffff. tbl = byte at 0x53625+(bl+3). */
uint64_t fn_045a82(uint32_t param_1, uint32_t param_2) {
    static const uint8_t tbl[20] =
        {0,9,1,1,11,6,4,5,5,5, 2,3,0xff,0xff,7,8, 6,8,1,0xff};
    uint8_t bl = (uint8_t)(param_1 & 0xff);
    fn_042a30(bl);                              /* g_dat_64004 = bl */
    uint32_t uVar1;
    if (param_1 < 0x100) {
        if (g_dat_533bb > 2) {
            uint8_t b = (uint8_t)(param_1 & 0xff);
            if (b == 0x50) bl = 0xe;
            else if (b < 0x22) { if (b >= 0x20) bl = 5; }
            else bl = 0x13;
        }
        if (bl > 0x13) bl = 0x13;
        uVar1 = tbl[bl];
    } else {
        uVar1 = (param_1 >> 8) & 0xff;
    }
    fn_042a0d(uVar1 & 0xff);                    /* g_dat_64008 = uVar1 & 0xff */
    return ((uint64_t)param_2 << 32) | 0xffffffff;
}

/* 0x045a76: thunk to fn_045a82. If param_2 (EDX) == 0, return 0; else tail-jump
 * into fn_045a82 with the same EAX/EDX. */
uint64_t fn_045a76(uint32_t param_1, uint32_t param_2) {
    if (param_2 == 0) return 0;
    return fn_045a82(param_1, param_2);
}

/* 0x042576: file-mode parser. c = to_lower(s[0]) must be r/w/a (else fn_042a0d(9), 0).
 * Builds flags from s[1]/s[2] modifiers: +/b/t; 0x40=binary, 3=R/W, then w|2 / a|0x82 / r|1.
 * Returns (param_2<<32) | flags. g_dat_53619==0x200 selects the default-binary path. */
uint64_t fn_042576(const char *s, uint32_t param_2) {
    char c = to_lower((uint8_t)s[0]);
    uint32_t ebx = 0;
    if (c != 'r' && c != 'w' && c != 'a') {
        fn_042a0d(9);                              /* invalid mode */
        return ((uint64_t)param_2 << 32);          /* ebx = 0 */
    }
    uint8_t m1 = (uint8_t)s[1], m2 = (uint8_t)s[2];
    if (m1 == '+') {
        ebx = 3;
        if (m2 == 'b' || (m2 != 't' && g_dat_53619 == 0x200)) ebx = 0x43;
    } else {
        if (m1 == 'b') {
            ebx = 0x40;
            if (m2 == '+') ebx = 0x43;
        } else if (m1 == 't') {
            if (m2 == '+') ebx = 3;
        } else if (g_dat_53619 == 0x200) {
            ebx = 0x40;
        }
    }
    if (c == 'w') ebx |= 2;
    else if (c == 'a') ebx |= 0x82;
    else ebx |= 1;
    return ((uint64_t)param_2 << 32) | ebx;
}

/* ---- L10 trivial accessors (the canary prologue PUSH 0x20;CALL 0x418ee is not replicated) ---- */
uint32_t g_dat_53c60 = 0;
uint32_t g_dat_40010 = 0;
void fn_043280(uint8_t *dest, uint32_t pattern, uint32_t count) {      /* 0x043280: byte-pattern fill (EBX param unused) */
    if (count == 0) return;
    while (count != 0 && ((uintptr_t)dest & 3) != 0) {                 /* align dest to 4 bytes */
        *dest = (uint8_t)pattern;
        dest++;
        pattern = (pattern >> 8) | (pattern << 24);                    /* ROR EDX,8 */
        count--;
    }
    uint32_t words = count / 4;
    if (words != 0) {
        dest = (uint8_t *)(uintptr_t)memsetw((uint32_t *)(uintptr_t)dest, pattern, words);
    }
    count &= 3;
    if (count != 0) {
        *dest = (uint8_t)pattern;                                      /* DL */
        dest++; count--;
        if (count != 0) {
            *dest = (uint8_t)(pattern >> 8);                           /* DH */
            dest++; count--;
            if (count != 0) {
                *dest = (uint8_t)pattern;                              /* DL (latent bug: should be pattern>>16) */
            }
        }
    }
}

uint32_t g_dat_62d64 = 0;   /* node-arena bump cursor (offset into s_node_arena) */
uint32_t g_dat_62d60 = 0;   /* DAT_00062d60: node-arena base (set to g_node_arena_base by fn_034990) */

/* Node arena — 32->64 pointer adaptation. The original stores full 32-bit node
 * addresses; the port stores base-relative uint32 offsets into s_node_arena and
 * translates via nd(). Node (11 bytes): [0]=start(int32) [4]=next(uint32 offset)
 * [8]=len(int16) [10]=flag(byte). g_dat_62d68[idx] = head-node offset per list. */
#define NIGHT_NODE_ARENA (1u << 16)
static uint8_t s_node_arena[NIGHT_NODE_ARENA];
uintptr_t g_node_arena_base = (uintptr_t)s_node_arena;
uint32_t  g_dat_62d68[200] = {0};   /* DAT_00062d68: head-node offset per list idx (fn_03ba2c/03c65b index 0..199) */
static inline uint8_t *nd(uint32_t off) { return s_node_arena + off; }
/* byte-wise node field accessors (alignment-safe: nodes are 11 bytes, so int16/int32
 * fields land on unaligned byte offsets — direct casts fault/corrupt on arm64). */
static inline int32_t  node_start_get(uint32_t o){ const uint8_t*p=s_node_arena+o;   return (int32_t)((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24)); }
static inline void     node_start_set(uint32_t o, int32_t v){ uint8_t*p=s_node_arena+o;   uint32_t u=(uint32_t)v; p[0]=(uint8_t)u; p[1]=(uint8_t)(u>>8); p[2]=(uint8_t)(u>>16); p[3]=(uint8_t)(u>>24); }
static inline uint32_t node_next_get(uint32_t o){  const uint8_t*p=s_node_arena+o+4; return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static inline void     node_next_set(uint32_t o, uint32_t v){ uint8_t*p=s_node_arena+o+4; p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static inline int16_t  node_len_get(uint32_t o){   const uint8_t*p=s_node_arena+o+8; return (int16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8)); }
static inline void     node_len_set(uint32_t o, int16_t v){ uint8_t*p=s_node_arena+o+8; uint16_t u=(uint16_t)v; p[0]=(uint8_t)u; p[1]=(uint8_t)(u>>8); }
static inline uint8_t  node_flag_get(uint32_t o){  return s_node_arena[o+10]; }
static inline void     node_flag_set(uint32_t o, uint8_t v){ s_node_arena[o+10]=v; }

/* ---- Video-RAM model (flat 0xa0000 VGA graphics segment) ----
 * The original renders into the VGA video segment at flat 0xa0000 (DAT_000a0000).
 * The flat 64-bit port has no such segment, so we model the 64 KB video window
 * [0xa0000, 0xb0000) as an explicit buffer. A flat video address va maps to
 * g_video_ram[va - 0xa0000]. This unblocks the video-flatten consumer (0x03c65b)
 * and the blit family. Out-of-window va is clamped to the buffer base (defensive;
 * real game stays within the window). */
#define VIDEO_BASE   0xa0000u
#define VIDEO_SIZE   0x10000u
uint8_t g_video_ram[VIDEO_SIZE];
static inline uint8_t *video_map(uint32_t va) {
    uint32_t off = va - VIDEO_BASE;
    if (off >= VIDEO_SIZE) off = 0;
    return g_video_ram + off;
}

/* 0x03c65b (L10, 143 B): video flatten. EAX = a0 (packed-records dest buffer).
 * For each of the 200 interval lists (i = 0..0xc7) seeded at fn_034990, walk the
 * node chain from g_dat_62d68[i], mem_copy(node->start video RAM, node->len bytes)
 * into the growing dest record, advancing the in-record offset by node->len, until
 * the record is 0x140 bytes full; then advance the dest by 0x140 (one record per
 * list). node->start is a flat video address (0xa0000 base) mapped via video_map.
 * Both callees (canary, mem_copy) ported. Canary PUSH 0x2c. */
void fn_03c65b(uint8_t *a0) {
    (void)canary_id(0, 0x2c);
    uint8_t *out = a0;
    for (uint32_t i = 0; i < 0xc8; i++) {
        uint32_t cur = g_dat_62d68[i];
        int32_t   off = 0;
        while (off < 0x140) {
            int32_t  len   = node_len_get(cur);      /* MOVSX EBX,word[cur+8] */
            uint32_t start = (uint32_t)node_start_get(cur); /* dword[cur+0] video addr */
            mem_copy(out + off, video_map(start), (uint32_t)len);
            off += len;
            cur = node_next_get(cur);                /* dword[cur+4] */
        }
        out += 0x140;
    }
}

uint32_t fn_010e15(void) { return g_dat_53c60; }                       /* 0x010e15 */
uint32_t fn_040631(uint32_t param) { uint32_t v = param & 0xffff; g_dat_40010 = v; return v; }  /* 0x040631 */
uint32_t g_dat_40014 = 0, g_dat_40018 = 0, g_dat_4001c = 0, g_dat_40020 = 0;
/* 0x0405de: store 4 16-bit values. ABI p1=EAX, p2=EDX, p3=EBX, p4=ECX. Each is
 * zero-extended (XOR EAX,EAX; MOV AX,reg) and stored to consecutive globals:
 * g_dat_40014=low16(p1), g_dat_40018=low16(p2), g_dat_4001c=low16(p3),
 * g_dat_40020=low16(p4). Ghidra's 0xffff0000ffff mask is 4 separate 16-bit
 * stores, not a bit-pack. */
void fn_0405de(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4) {
    g_dat_40014 = p1 & 0xffff;
    g_dat_40018 = p2 & 0xffff;
    g_dat_4001c = p3 & 0xffff;
    g_dat_40020 = p4 & 0xffff;
}
uint32_t fn_01520b(const uint8_t *p) { return *(const uint32_t *)(p + 5); }  /* 0x01520b */
uint32_t fn_034a87(void) { uint32_t v = g_dat_62d64; g_dat_62d64 += 11; return v; }  /* 0x034a87 */

/* 0x034990 (L11, 126 B): node-pool / video-arena INIT. Original:
 *   g_62d60 = heap_malloc(0x1adb0); g_62d64 = g_62d60;          // arena base + cursor
 *   for (i = 0; i < 0xc8; i++) { node = fn_034a87();            // node = g_62d64; g_62d64 += 11
 *       node->len = 0x140; node->start = 0xa0000; node->next = 0;
 *       g_62d68[i] = node; }
 * The port stores base-relative offsets into the static s_node_arena, so the cursor
 * g_62d64 starts at offset 0 and g_62d60 records the arena base (g_node_arena_base).
 * Node layout (11 B): start@+0, next@+4, len@+8 (word), flag@+10. Each head list i is
 * seeded with a single full-arena node [0,0x140) at video base 0xa0000. No canary.
 */
void fn_034990(void) {
    g_dat_62d60 = (uint32_t)(uintptr_t)g_node_arena_base;  /* arena base (orig heap_malloc(0x1adb0)) */
    g_dat_62d64 = 0;                                        /* cursor = base-relative offset 0 */
    for (uint32_t i = 0; i < 0xc8; i++) {
        uint32_t node = fn_034a87();   /* node = 11*i; g_dat_62d64 += 11 */
        node_len_set(node, 0x140);     /* [node+8] = 0x140 */
        node_start_set(node, 0xa0000); /* [node+0] = 0xa0000 */
        node_next_set(node, 0);        /* [node+4] = 0 */
        g_dat_62d68[i] = node;         /* head of list i */
    }
}

/* 0x03dcbd (L10, 455 B): STRUCT DESTRUCTOR. a0 is a real (64-bit) pointer to a
 * struct whose fields +0x4..+0x38 (14 words, step 4) each hold a heap offset of a
 * sub-object. For each of the 14, if non-zero: heap_free_host(field) (orig 0x041c31)
 * then null the field. Field +0x0 is untouched. Repetitive unrolled loop (no branch
 * table). No canary beyond the standard prologue marker. */
void fn_03dcbd(uint32_t *a0) {
    (void)canary_id(0, 0x20);
    for (int k = 1; k <= 14; k++) {    /* fields at +0x4 .. +0x38 */
        if (a0[k] != 0) {
            heap_free_host(a0[k]);
            a0[k] = 0;
        }
    }
}

/* 0x03d999 (L10, 53 B): wrapper -- destruct a0 via fn_03dcbd, then return a0.
 * a1 (EDX) is stored to the frame but never read. */
uint32_t *fn_03d999(uint32_t *a0, uint32_t a1) {
    (void)a1;
    (void)canary_id(0, 0x24);
    fn_03dcbd(a0);
    return a0;
}

/* 0x034aba: 3-way interval-list splitter. Splits the interval containing `a`
 * (within list g_dat_62d68[idx]) to carve out [a, a+b) as a new node (start=c,
 * flag=flag), clamping a+b to 0x140. Returns void. */
void fn_034aba(int32_t a, int32_t idx, int32_t b, int32_t c, int32_t flag) {
    uint32_t head = g_dat_62d68[idx];
    uint32_t newnode1 = fn_034a87();
    int32_t acc = 0;
    if (a < 0) { c -= a; b += a; a = 0; g_dat_62d68[idx] = newnode1; }
    if (a + b >= 0x140) b = 0x140 - a;
    int32_t end = a + b;
    for (;;) {                                    /* walk to node containing a */
        acc += node_len_get(head);
        if (acc >= a) break;
        head = node_next_get(head);
    }
    node_len_set(newnode1, (int16_t)b);
    node_start_set(newnode1, c);
    node_flag_set(newnode1, (uint8_t)flag);
    int16_t svar2 = (int16_t)(node_len_get(head) - (acc - a));
    if (end >= 0x140) {                           /* interval reaches 0x140: 2-way */
        node_len_set(head, svar2);
        node_next_set(head, newnode1);
    } else {                                      /* 3-way split */
        uint32_t pivar1 = head;
        for (;;) {                                /* walk to node containing end */
            if (acc > end) break;
            head = node_next_get(head);
            acc += node_len_get(head);
        }
        uint32_t newnode2 = fn_034a87();
        node_len_set(newnode2, (int16_t)(acc - end));
        int32_t ns2 = node_start_get(head) + (int32_t)node_len_get(head)
                    - (int32_t)node_len_get(newnode2);
        node_start_set(newnode2, ns2);
        node_next_set(newnode2, node_next_get(head));
        node_flag_set(newnode2, node_flag_get(head));
        node_len_set(pivar1, svar2);
        node_next_set(pivar1, newnode1);
        node_next_set(newnode1, newnode2);
    }
}
/* 0x03ba2c: build 200 interval lists. c_init=EAX, inc=EDX. Each iteration i
 * (0..199) carves [0,0x140) into list i with node-start=c_init+i*inc, flag=0.
 * (The a=0>=0 path never touches g_dat_62d68[i]; it links after node@0, so the
 * observable output lives in the arena nodes at offsets 11*i.) */
void fn_03ba2c(uint32_t c_init, uint32_t inc) {
    for (uint32_t i = 0; i < 200; i++) { fn_034aba(0, (int32_t)i, 0x140, (int32_t)c_init, 0); c_init += inc; }
}
/* 0x03ba86: nested-loop interval-list builder reading a variable-length byte
 * stream. ABI a_base=EAX, idx_base=EDX, row_count=EBX, p=ECX. For each row:
 * count=*p++; then count times: acc+=*p++; len=*p++; fn_034aba(a_base+acc,
 * idx_base+row, len, p, flag); p+=len; acc+=len. The flag (5th arg) is read
 * from [EBP+0x10] — the pushed 0x38 canary in the prologue's stack layout —
 * so it is the constant 0x38. Assumes the lists are pre-initialized (non-zero
 * a would otherwise infinite-loop on an empty node@0). */
void fn_03ba86(int32_t a_base, int32_t idx_base, int32_t row_count, const uint8_t *p) {
    for (int32_t row = 0; row < row_count; row++) {
        uint32_t count = *p++;
        int32_t acc = 0;
        for (uint32_t i = 0; i < count; i++) {
            acc += *p++;
            uint32_t len = *p++;
            fn_034aba(a_base + acc, idx_base + row, (int32_t)len, (int32_t)p, 0x38);
            p += len;
            acc += (int32_t)len;
        }
    }
}
/* 0x03a594: filtered byte copy. ABI dest=EAX, src=EDX, count=EBX.
 * for i in 0..count-1: if (src[i] != 0xff) dest[i] = src[i]. Bytes equal to
 * 0xff are NOT copied (dest[i] is left unchanged). */
void fn_03a594(uint8_t *dest, const uint8_t *src, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (src[i] != 0xff) dest[i] = src[i];
    }
}
uint32_t g_dat_639dc = 0;
uint32_t g_dat_639e0 = 0;
uint32_t fn_01519f(const uint8_t *p) { return *(const uint32_t *)(p + 1) - g_dat_639dc; }  /* 0x01519f */
uint32_t fn_0151d5(const uint8_t *p) { return *(const uint32_t *)(p + 9) - g_dat_639e0; }  /* 0x0151d5 */

uint8_t  g_dat_63f14[16] = {0};
uint32_t g_dat_63f28[16] = {0};
uint8_t  g_dat_63f50 = 0;
uint32_t g_dat_63f08 = 0;
uint8_t  g_dat_62cd8[128] = {0};
uint8_t  g_dat_62c58[128] = {0};
uint32_t g_dat_62d5c = 0;  /* DAT_00062d5c: frame counter (decremented by fn_0347e3) */
uint8_t  g_dat_52ff8 = 0;  /* DAT_00052ff8: keyboard pump enabled (gate for fn_0347e3) */
uint8_t  g_dat_62c75 = 0;   /* 0x62c75 */
uint8_t  g_dat_62c82 = 0;   /* 0x62c82 */
uint8_t  g_dat_62c8e = 0;   /* 0x62c8e */
uint8_t  g_dat_62c90 = 0;   /* 0x62c90 */
uint32_t g_dat_62a94 = 0;      /* 0x62a94 */
uint8_t *g_dat_53c7c = 0;      /* 0x53c7c */
uint8_t *g_dat_53c80 = 0;      /* 0x53c80 */
int32_t  g_dat_53c6c = 0;      /* 0x53c6c */
uint8_t *g_dat_53c70 = 0;      /* 0x53c70 (entity list head) */
uint8_t *g_dat_53c74 = 0;      /* 0x53c74 (entity list tail) */
int32_t  g_dat_51c98 = 0;   /* DAT_00051c98 (collision mode flag: 1=reinit->2, 2=active) */
int32_t  g_dat_53c90 = 0;   /* DAT_00053c90 (player-event latch) */
int32_t  g_dat_53c68 = 0;      /* 0x53c68 (entity count) */

/* 0x015295 (L11, 456 B): ENTITY TICK.
 *   1. a0->5 += a1; if (a0->5 > 0) a0->5 = 0;        (clamp the counter to <= 0)
 *   gate: proceed iff  a0[0x30]==1 && a0[0x25]!=0xff && a0->5==0 && a0->11>0x15;
 *         else return (only the a0->5 clamp applied).
 *   2. a0[0x25] = 0xff;  if (a0[0] != 1) return;      (set active, then exit if not type 1)
 *   3. if (a0 == g_dat_53c80): node = heap_malloc(0x21); if (node) fn_0003b5c0(node, 1, a0->32, a0->1, a0->9-0x1e);
 *      else:                   node = heap_malloc(0x21); if (node) fn_0003b5c0(node, 0, a0->32, a0->1, a0->9-0x1e);
 *   (heap_malloc returns a heap OFFSET; the node pointer is g_heap_base + offset, and the
 *    failure check is on the offset, per the port's heap convention.)
 *   4. *(g_dat_53c7c + 0x52) += a0->32;               (accumulator -- reached only in step 3)
 * NOTE: the original computes a clamped offset (tmp) from (a0->1 - g_dat_639dc) in each
 * branch (bounds 0x104 / 0x136) but NEVER consumes it -- dead code, omitted here.
 * POINTER-WIDTH ADAPTATION: the original allocates the node with heap_malloc(0x19) (25 B,
 * 4-byte prev@+0x15); the port widens prev to 8 B@+0x19, so the node needs +8 B = 0x21.
 * reg-ABI: a0=EAX (struct ptr), a1=EDX. Only real callees: heap_malloc (0x41bdc) and
 * fn_0003b5c0 (list insert); 0x418ee is the canary. */
void fn_015295(uint8_t *a0, int32_t a1) {
    (void)canary_id(0, 0x48);
    int32_t *p5 = (int32_t *)(a0 + 5);
    *p5 += a1;                                     /* 0x152b8 */
    if (*p5 > 0) *p5 = 0;                          /* 0x152c4 */
    if (a0[0x30] != 1) return;                     /* 0x152d1/152dc */
    if (a0[0x25] == 0xff) return;                  /* 0x152e9 */
    if (*p5 != 0) return;                          /* 0x152f9 (a0->5 must be 0) */
    if (*(int32_t *)(a0 + 0x11) <= 0x15) return;   /* 0x15304 (a0->11 must be > 0x15) */
    a0[0x25] = 0xff;                               /* 0x1530e */
    if (a0[0] != 1) return;                        /* 0x1531c */
    uint32_t a0_1   = *(uint32_t *)(a0 + 1);       /* a0->1  */
    uint32_t a0_9   = *(uint32_t *)(a0 + 9);       /* a0->9  */
    uint32_t a0_32  = *(uint32_t *)(a0 + 0x32);    /* a0->32 */
    uint32_t node_off = heap_malloc(0x21);  /* orig 0x19 +8 (widened prev) */
    if (node_off)
        fn_0003b5c0((uint8_t *)(g_heap_base + node_off),
                    a0 == g_dat_53c80 ? 1 : 0, a0_32, a0_1, a0_9 - 0x1e);
    *(uint32_t *)(g_dat_53c7c + 0x52) += a0_32;    /* 0x15453 accumulator */
}
uint8_t  g_dat_53eb8[0x2000] = {0}; /* 0x53eb8: zeroed runtime table */

void fn_031f10(uint8_t *p, uint32_t val) {                      /* 0x031f10 */
    uint8_t c = p[0x31];
    if (c < 5) { *(uint32_t *)(p + 0x32 + c*4) = val; p[0x31] = c + 1; }
}
void fn_0348e2(void) {                                          /* 0x0348e2 */
    g_dat_63f14[g_dat_63f50] = 0;
    g_dat_63f28[g_dat_63f50] = g_dat_63f08;
    g_dat_63f50++;
    if (g_dat_63f50 > 9) g_dat_63f50 = 0;
}
/* 0x03493e: read a slot byte. No args; returns AL. The slot index is g_dat_63f50
 * (wraps 0..9). if g_dat_63f50==0: return the byte at 0x63f1d = g_dat_63f14[9]
 * (a default). else: return the byte at 0x63f13+g_dat_63f50 = g_dat_63f14[g_dat_63f50-1]
 * (valid for 1..9). The standalone byte at 0x63f13 is never read (index 0 is
 * special-cased to 0x63f1d). */
uint8_t fn_03493e(void) {
    return (g_dat_63f50 == 0) ? g_dat_63f14[9] : g_dat_63f14[g_dat_63f50 - 1];
}
void fn_0343d1(uint8_t i) {                                     /* 0x0343d1 */
    if (g_dat_62cd8[i] == 0) g_dat_62c58[i] = 0;
    else { g_dat_62c58[i] = 1; g_dat_62cd8[i]--; }
}

/* 0x03442e: rotating 10-slot event buffer.
 * Param x in AL (EAX preserved by the 0x418ee stack-guard prologue).
 * Dispatches on x to stamp a marker into g_dat_63f14[g_dat_63f50] and pick y;
 * when a branch fires (z==1) it records delta=g_dat_63f08-g_dat_63f0c into the
 * current slot, advances the slot (wrap 9->0), then calls fn_0343d1(x) plus
 * fn_0343d1(y)/fn_0343d1(0x36) depending on y. */
uint8_t *fn_041e20(uint8_t *dest, uint8_t byte, uint32_t count) {  /* 0x041e20: memset via fn_043280 (dest=EAX,byte=DL,count=EBX) */
    uint32_t p = ((uint32_t)byte<<24)|((uint32_t)byte<<16)|((uint32_t)byte<<8)|(uint32_t)byte;
    fn_043280(dest, p, count);
    return dest;
}
int32_t fn_0350aa(const uint8_t *a, const uint8_t *b) {            /* 0x0350aa: 4-byte prefix equal? (a=EAX,b=EDX) -> 1/-1 */
    for (int i = 0; i < 4; i++) { if (a[i] != b[i]) return -1; }
    return 1;
}

uint32_t fn_03510b(const uint8_t *p) {                       /* 0x03510b: big-endian 32-bit read */
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3];
}
uint64_t fn_0408c5(const uint8_t *p) {                       /* 0x0408c5: little-endian 24-bit (3-byte) read */
    return (uint64_t)p[0]|((uint64_t)p[1]<<8)|((uint64_t)p[2]<<16);
}

uint32_t fn_0351dc(uint8_t **cursor) {                     /* 0x0351dc: unsigned LEB128 (base-128, 7-bit) decoder */
    uint8_t *p = *cursor;
    uint32_t val = (uint32_t)(*p & 0x7f);
    while ((*p & 0x80) != 0) { p++; val = (val << 7) | (uint32_t)(*p & 0x7f); }
    *cursor = p + 1;
    return val;
}

void fn_034735(uint8_t idx) {                                    /* 0x034735: per-key state machine (62c58/62cd8) */
    if (g_dat_62c58[idx] == 4) {
        fn_034353(idx, g_dat_63f08);
        if (g_dat_62cd8[idx] != 0) { g_dat_62c58[idx] = 1; g_dat_62cd8[idx]--; }
        else                       { g_dat_62c58[idx] = 0; }
    } else if (g_dat_62c58[idx] < 4) {
        g_dat_62c58[idx]++;
        fn_03442e(idx);
    }
}

/* 0x0347e3 (L13, 254 B): keyboard dispatch pump. Gated on g_52ff8. For each of the
 * 128 per-key state slots g_62c58[i] that is non-zero, dispatch on the index: the
 * scan-code keys {0x1d,0x2a,0x2c,0x36,0x38} drive fn_034735 (modifier state
 * machine) and {0x48,0x4b,0x4d,0x50} drive fn_034635 (key state machine); any other
 * index is ignored. After the loop, decrement the frame counter g_62d5c if non-zero.
 * No return value. */
void fn_0347e3(void) {
    (void)canary_id(0, 0x24);                                   /* PUSH 0x24;CALL 0x418ee */
    if (g_dat_52ff8 == 0) return;                               /* 0x347fb/cf */
    for (uint32_t i = 0; i < 0x80; i++) {                       /* 0x34808-0x3481e */
        if (g_dat_62c58[i] == 0) continue;                      /* 0x34824-0x34834 */
        switch (i) {                                            /* 0x3483a-0x348c3 */
        case 0x1d: case 0x2a: case 0x2c: case 0x36: case 0x38:
            fn_034735((uint8_t)i); break;                       /* 0x34851 */
        case 0x48: case 0x4b: case 0x4d: case 0x50:
            fn_034635((uint8_t)i); break;                       /* 0x34842 */
        default:
            break;
        }
    }
    if (g_dat_62d5c != 0) g_dat_62d5c--;                        /* 0x348ca-0x348d3 */
}

void fn_03442e(uint8_t x) {                                      /* 0x03442e */
    uint8_t y = 0;
    uint8_t z = 0;
    if (x == 0x1d) {
        if (g_dat_62c90 != 0) { g_dat_63f14[g_dat_63f50] = 0x67; y = 0x38; z = 1; }
        else if (g_dat_62c82 != 0 || g_dat_62c8e != 0) { g_dat_63f14[g_dat_63f50] = 0x68; y = 0x2a; z = 1; }
    } else if (x == 0x38) {
        if (g_dat_62c75 != 0) { g_dat_63f14[g_dat_63f50] = 0x67; y = 0x1d; z = 1; }
        else if (g_dat_62c82 != 0 || g_dat_62c8e != 0) { g_dat_63f14[g_dat_63f50] = 0x69; y = 0x2a; z = 1; }
    } else if (x == 0x2a || x == 0x36) {
        if (g_dat_62c75 != 0) { g_dat_63f14[g_dat_63f50] = 0x68; y = 0x1d; z = 1; }
        else if (g_dat_62c90 != 0) { g_dat_63f14[g_dat_63f50] = 0x69; y = 0x38; z = 1; }
    }
    if (z != 1) return;
    g_dat_63f1e[g_dat_63f50] = 0;
    g_dat_63f28[g_dat_63f50] = g_dat_63f08 - g_dat_63f0c;
    if (g_dat_63f50 == 9) g_dat_63f50 = 0; else g_dat_63f50++;
    fn_0343d1(x);
    if (y == 0x1d) { fn_0343d1(0x1d); }
    else if (y == 0x2a) { fn_0343d1(0x2a); fn_0343d1(0x36); }
    else if (y == 0x38) { fn_0343d1(0x38); }
}

uint8_t  g_dat_639e4 = 0;
uint8_t  g_dat_63f1e[16] = {0};
uint32_t g_dat_63f0c = 0;

void fn_03cad3(const uint8_t *p) {                                /* 0x03cad3 */
    uint32_t a = *(const uint32_t *)p;
    uint32_t b = *(const uint32_t *)(p + 4);
    uint32_t c = *(const uint32_t *)(p + 8);
    if (p[17] == 0) g_dat_639dc = a; else g_dat_639dc = c - 0x140;
    g_dat_639e0 = b;
    g_dat_639e4 = p[16];
}
void fn_034353(uint8_t a, uint32_t b) {                            /* 0x034353 */
    g_dat_63f14[g_dat_63f50] = a;
    g_dat_63f28[g_dat_63f50] = b - g_dat_63f0c;
    g_dat_63f1e[g_dat_63f50] = 0;
    g_dat_63f0c = b;
    g_dat_63f50++;
    if (g_dat_63f50 > 9) g_dat_63f50 = 0;
}

void fn_010376(uint8_t *s, uint32_t unused) {                        /* 0x010376 */
    (void)unused;
    uint32_t f;
    f = *(uint32_t *)(s + 0x6d); if (f) heap_free_host(f);
    f = *(uint32_t *)(s + 0x71); if (f) heap_free_host(f);
    f = *(uint32_t *)(s + 0x75); if (f) heap_free_host(f);
}

uint32_t fn_03518a(const uint8_t *p) { return ((uint32_t)p[0] << 8) | (uint32_t)p[1]; }  /* 0x03518a */

void fn_04a7fa(double a, double *out) { *out = a - fpu_rndint(a); }  /* 0x04a7fa */

/* DAT_0005184c = -1.0 (0xBFF0000000000000). 0x04a32b: 3-arg call to 0x04a7fa
 * (arg1, arg2=unused, &local) -> local = arg1 - fpu_rndint(arg1) = toward-
 * nearest-even remainder; 0x04a7fa leaves arg1 on the FPU stack. FLDZ; FCOMPP
 * compares arg1 (ST1) vs 0.0 (ST0); C0 = (ST1 < ST0) = (arg1 < 0.0); JBE (arg1
 * >= 0.0) skips the FADD. So: if (arg1 < 0.0) local += DAT_0005184c (-1.0).
 * Returns the double in FPU ST. arg2 is passed through but unused (0x04a7fa
 * only reads arg1 and the out ptr). orig: 0x04a32b */
double fn_04a32b(double arg1, double arg2)
{
    (void)arg2;
    double local;
    fn_04a7fa(arg1, &local);       /* local = arg1 - fpu_rndint(arg1) */
    if (arg1 < 0.0) local += -1.0; /* 0x4a34c JBE: DAT_0005184c = -1.0  */
    return local;
}

/* ---- 0x04a3a8 (76 insns) ----
 *
 * FLD/FCOMP/FDIV/FMUL: a clamp + decimal-power fn.
 *
 *   ABI (RET 0xc, 12 bytes of args): arg1 = double @ [ESP+0x20] (after the
 *         5 PUSHes + SUB 8), arg2 = int32_t = ECX @ [ESP+0x28]. Returns a
 *         double in FPU ST(0).
 *
 *   (1) clamp local = arg1:
 *         arg1 == +inf -> +DAT_000519a4 (0x7fefffffffffffff = +DBL_MAX)
 *         arg1 is NaN  -> -DAT_000519ac (0xffefffffffffffff = -DBL_MAX)
 *         else (finite/-inf) -> local = arg1 (passthrough)
 *         (FCOMP vs 0x51994=+inf then 0x5199c=-inf; the JNZ after SAHF tests
 *         C0=ZF: taken iff C0==0, i.e. the equal/unordered cases.)
 *
 *   (2) power-scale: let s = DAT_000519be = table[0].mult (10^216-ish).
 *         arg2 < 0 : edx = -arg2;  if edx > 0x134 { local /= s; edx -= 0xd8; }
 *         arg2 >= 0: edx =  arg2;  if arg2 > 0x134 { local *= s; edx -= 0xd8; }
 *         (0x134 = 308, 0xd8 = 216.)
 *
 *   (3) table-power over {int16 count; double mult} @ 0x519bc (8 entries, the
 *         last count==1 is the catch-all terminator; each mult ~= 10^count as a
 *         nearest power of 2): local2 = 1.0; walk entries: if edx >= count {
 *         local2 *= mult; edx -= count; }; if edx == 0 done; if count == 1
 *         re-loop the same entry (consume the remainder one "digit" at a time);
 *         else next entry.
 *
 *   (4) local = arg2 < 0 ? local / local2 : local * local2; return local.
 *
 *   Net: ~= arg1 * 10^arg2 (power-of-10 approximation) with the clamps above.
 *   Flat 64-bit: all FPU ops are ordinary C double ops; the table/clamp/scale
 *   constants are embedded as their exact 64-bit patterns (a3a8_dbl) so the
 *   result is bit-exact. Differential vs an independent Python reference.
 */
static double a3a8_dbl(uint64_t b) { double d; memcpy(&d, &b, 8); return d; }
static const int kA3a8_counts[8] = { 216, 108, 54, 27, 14, 8, 4, 1 };
static const uint64_t kA3a8_mults[8] = {
  0x6cc734e940c6f9c6ULL, 0x565b403dcc834e12ULL, 0x4b24e1878814c9ceULL,
  0x4589d971e4fe8402ULL, 0x42d6bcc41e900000ULL, 0x4197d78400000000ULL,
  0x40c3880000000000ULL, 0x4024000000000000ULL,
};
#define K_A3A8_CLAMP_MAX 0x7fefffffffffffffULL /* +DBL_MAX (DAT_000519a4) */
#define K_A3A8_CLAMP_MIN 0xffefffffffffffffULL /* -DBL_MAX (DAT_000519ac) */
#define K_A3A8_SCALE     0x6cc734e940c6f9c6ULL /* 10^216-ish (DAT_000519be) */

double fn_04a3a8(double arg1, int32_t arg2) {
  double local;
  if (arg1 == INFINITY)
    local = a3a8_dbl(K_A3A8_CLAMP_MAX);
  else if (isnan(arg1))
    local = a3a8_dbl(K_A3A8_CLAMP_MIN);
  else
    local = arg1;

  int32_t edx;
  double scale = a3a8_dbl(K_A3A8_SCALE);
  if (arg2 < 0) {
    edx = -arg2;
    if (edx > 0x134) { local = local / scale; edx -= 0xd8; }
  } else {
    edx = arg2;
    if (arg2 > 0x134) { local = local * scale; edx -= 0xd8; }
  }

  double local2 = 1.0;
  int i = 0;
  for (;;) {
    if (edx >= kA3a8_counts[i]) {
      local2 = local2 * a3a8_dbl(kA3a8_mults[i]);
      edx -= kA3a8_counts[i];
    }
    if (edx == 0) break;
    if (kA3a8_counts[i] == 1) continue; /* re-consume remainder via 10^1 */
    i++;
  }

  local = arg2 < 0 ? local / local2 : local * local2;
  return local;
}

int32_t fn_016959(uint8_t *s, int32_t param2, int32_t *param3) {    /* 0x016959 */
    *(int32_t *)(s + 0x17) = 2;
    int32_t F = (int32_t)fn_01519f(s);
    int32_t X = *(int32_t *)(s + 0xbf);
    int32_t ret;
    int32_t cnt = *(int32_t *)(s + 0xab);
    if (F > X) {
        if (F - param2 > X) { *param3 = -param2; if (cnt < 0) ret = 1; else { *(int32_t*)(s+0xab) = cnt - 1; ret = 0; } }
        else { *param3 = 0; ret = 1; }
    } else {
        if (F >= X) { *param3 = 0; ret = 1; }
        else {
            if (param2 + F < X) { *param3 = param2; if (cnt < 0) ret = 1; else { *(int32_t*)(s+0xab) = cnt - 1; ret = 0; } }
            else { *param3 = 0; ret = 1; }
        }
    }
    return ret;
}

int32_t fn_017530(uint8_t *s, int32_t param2, int32_t *param3) { /* 0x017530: byte-identical twin of 0x016959 (verified: same instr mix, REL offsets, callee fn_01519f) */
    return fn_016959(s, param2, param3);
}

uint8_t *g_dat_63354 = NULL;   /* 0x63354: POINTER to clamp-bounds struct; lo=P+0x12, hi=P+0x16 (double-deref) */
uint8_t *g_dat_63358 = NULL;   /* 0x63358: mirror of g_dat_63354 (0x03cc6b) */
uint32_t g_dat_63370 = 0;      /* 0x63370: mode flag (0x03cc6b) */
uint32_t g_dat_63374 = 0;      /* 0x63374: = g_dat_53c7c->0x2b (0x03cc6b) */
uint32_t g_dat_63378 = 0;      /* 0x63378: = (byte)g_dat_53c7c->0x27 (0x03cc6b) */
uint32_t g_dat_6337c = 0;      /* 0x6337c: = (byte)g_dat_53c7c->0x26 (0x03cc6b) */
uint32_t g_dat_63380 = 0;      /* 0x63380: = (byte)g_dat_53c7c->0x25 (0x03cc6b) */
uint32_t g_dat_63384 = 0;      /* 0x63384: = (byte)g_dat_53c7c->0x28 (0x03cc6b) */

/* ---- 0x01586b (L10, 435 B): clamp a0[9] to [min,max] after adding a1 ----
 * canary PUSH 0x2c;CALL 0x418ee. a0 (EAX) = item byte-buffer, a1 (EDX) = delta.
 *   p = g_dat_63354
 *   min = *(s32*)(p+0x12) + g_dat_639e0
 *   max = (p[0x10]==0) ? *(s32*)(p+0x16) + g_dat_639e0
 *                       : *(s32*)( *(u8_t**)(p+0x34c) + 0x16 ) + g_dat_639e0
 *   pre-pass (only if a0[0] <= 1): for i in 0..g_dat_62a8c, t=&g_tab_5c8bc[i]:
 *     if (a0[1] > t->a1_lo && a0[1] < t->a1_hi):            // 0x1591f JLE cont / 0x15931 JL match
 *       if (a1 > 0) { if (t->v_lo < a0[9]+a1 < t->v_hi) a1 = t->v_lo - a0[9]; }   // 0x1596a-1597a
 *       else        { if (t->v_lo < a0[9]+a1 < t->v_hi) a1 = t->v_hi - a0[9]; }   // 0x159ab-159bb
 *   final: if (a0[9] < min || a0[9] > max) a0[9] += a1;                       // 0x159d9 (no clamp)
 *          else { a0[9] += a1; clamp a0[9] to [min,max]; }                     // 0x159e4
 * The EAX at exit is inconsistent across paths (a1 on the no-clamp path, clamped
 * value on the clamp path), so the return value is incidental -> void. a0 fields:
 * a0[0] byte, a0[1]/a0[9] unaligned s32 (cf. fn_0151d5 = *(s32*)(p+9)). */
struct clamp_tab_5c8bc g_tab_5c8bc[512];   /* 0x5c8bc ROM clamp table; count = g_dat_62a8c (struct def in f.h) */
void fn_01586b(uint8_t *a0, uint32_t a1) {
    (void)canary_id(0, 0x2c);                                /* 0x1586b/15870 */
    int32_t d = (int32_t)a1;
    if (d == 0) return;                                       /* 0x15888/1588c */
    uint8_t *p = g_dat_63354;
    int32_t mn = (int32_t)(*(int32_t *)(p + 0x12) + g_dat_639e0);   /* 0x15892-158a0 */
    int32_t mx;
    if (p[0x10] == 0) mx = (int32_t)(*(int32_t *)(p + 0x16) + g_dat_639e0);     /* 0x158b4-158c2 */
    else { uint8_t *q = *(uint8_t **)(p + 0x34c); mx = (int32_t)(*(int32_t *)(q + 0x16) + g_dat_639e0); } /* 0x158c7-158db */
    if (a0[0] <= 1) {                                          /* 0x158e1-158eb (a0[0] > 1 skips) */
        for (uint32_t i = 0; i < g_dat_62a8c; i++) {
            struct clamp_tab_5c8bc *t = &g_tab_5c8bc[i];
            int32_t key = *(int32_t *)(a0 + 1);
            int32_t v = *(int32_t *)(a0 + 9) + d;
            if (key > t->a1_lo && key < t->a1_hi) {
                if (d > 0) { if (v > t->v_lo && v < t->v_hi) d = t->v_lo - *(int32_t *)(a0 + 9); }
                else        { if (v > t->v_lo && v < t->v_hi) d = t->v_hi - *(int32_t *)(a0 + 9); }
            }
        }
    }
    int32_t cur = *(int32_t *)(a0 + 9);
    if (cur < mn || cur > mx) {
        *(int32_t *)(a0 + 9) = cur + d;                        /* 0x159df (no clamp) */
    } else {
        int32_t nv = cur + d;                                  /* 0x159ea */
        if (nv < mn) nv = mn;                                  /* 0x159fe */
        else if (nv > mx) nv = mx;                             /* 0x15a14 */
        *(int32_t *)(a0 + 9) = nv;
    }
}

/* 0x03cc6b (L10, 415 B): state machine, no real callees (frame only). c = a0->0x34c
 * (64-bit ptr). if g_639e4==0: if g_639dc==c->0x0: g_63354=c; g_639e4=1; if g_63370!=1
 *   copy kb-config. else: if c->0x4-1==g_639e0: g_63354=c; g_639e4=0; if g_63370!=1
 *   copy kb-config. Both branches: if c->0xc - 0xc8 > g_639e0: g_639e0++.
 * kb-config copy: g_63358=g_63354; g_63374=g_53c7c->0x2b; g_63384=(b)g_53c7c->0x28;
 *   g_63378=(b)g_53c7c->0x27; g_6337c=(b)g_53c7c->0x26; g_63380=(b)g_53c7c->0x25. */
void fn_03cc6b(uint8_t *a0) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    uint8_t *c = *(uint8_t **)(a0 + 0x34c);                     /* 0x3cc98 */
    if (g_dat_639e4 == 0) {                                     /* 0x3cc8f */
        if (g_dat_639dc == *(uint32_t *)c) {                    /* 0x3cca4 */
            g_dat_63354 = c;                                    /* 0x3ccb5 */
            g_dat_639e4 = 1;                                    /* 0x3ccba */
            if (g_dat_63370 != 1) {                             /* 0x3ccc1 */
                g_dat_63358 = g_dat_63354;                      /* 0x3cccf */
                g_dat_63374 = *(uint32_t *)(g_dat_53c7c + 0x2b);/* 0x3ccdc */
                g_dat_63384 = *(uint8_t *)(g_dat_53c7c + 0x28); /* 0x3ccee */
                g_dat_63378 = *(uint8_t *)(g_dat_53c7c + 0x27); /* 0x3cd00 */
                g_dat_6337c = *(uint8_t *)(g_dat_53c7c + 0x26); /* 0x3cd12 */
                g_dat_63380 = *(uint8_t *)(g_dat_53c7c + 0x25); /* 0x3cd24 */
            }
        }
        if ((int32_t)(*(uint32_t *)(c + 0xc)) - 0xc8 > (int32_t)g_dat_639e0)
            g_dat_639e0++;                                       /* 0x3cd42 */
    } else {                                                     /* 0x3cd4d */
        if ((int32_t)(*(uint32_t *)(c + 0x4)) - 1 == (int32_t)g_dat_639e0) {  /* 0x3cd60 */
            g_dat_63354 = c;                                    /* 0x3cd6f */
            g_dat_639e4 = 0;                                    /* 0x3cd74 */
            if (g_dat_63370 != 1) {                             /* 0x3cd7b */
                g_dat_63358 = g_dat_63354;                      /* 0x3cd89 */
                g_dat_63374 = *(uint32_t *)(g_dat_53c7c + 0x2b);/* 0x3cd96 */
                g_dat_63384 = *(uint8_t *)(g_dat_53c7c + 0x28); /* 0x3cda8 */
                g_dat_63378 = *(uint8_t *)(g_dat_53c7c + 0x27); /* 0x3cdba */
                g_dat_6337c = *(uint8_t *)(g_dat_53c7c + 0x26); /* 0x3cdcc */
                g_dat_63380 = *(uint8_t *)(g_dat_53c7c + 0x25); /* 0x3cdde */
            }
        }
        if ((int32_t)(*(uint32_t *)(c + 0xc)) - 0xc8 > (int32_t)g_dat_639e0)
            g_dat_639e0++;                                       /* 0x3cdfc */
    }
}

int32_t fn_016a60(uint8_t *s, int32_t param2, int32_t *out) {          /* 0x016a60: clamped move-toward-target */
    *(int32_t *)(s + 0x17) = 2;
    int32_t F = (int32_t)fn_0151d5(s);
    int32_t X = *(int32_t *)(s + 0xc3);
    int32_t g12 = *(int32_t *)(g_dat_63354 + 0x12);
    int32_t g16 = *(int32_t *)(g_dat_63354 + 0x16);
    int32_t cnt = *(int32_t *)(s + 0xab);
    if (F > X) {
        if (F - param2 <= X) { *out = 0; return 1; }
        if (F - param2 <= g12) { *out = 0; return 1; }
        *out = -param2;
        if (cnt < 0) return 1;
        *(int32_t *)(s + 0xab) = cnt - 1; return 0;
    } else {
        if (F >= X) { *out = 0; return 1; }
        if (param2 + F >= X) return 1;      /* latent: no *out write */
        if (param2 + F >= g16) return 1;    /* latent: no *out write */
        *out = param2;
        if (cnt < 0) return 1;
        *(int32_t *)(s + 0xab) = cnt - 1; return 0;
    }
}

int32_t fn_017637(uint8_t *s, int32_t param2, int32_t *out) {          /* 0x017637: variant of fn_016a60; Branch-B reached writes *out=0 (no latent bug) */
    *(int32_t *)(s + 0x17) = 2;
    int32_t F = (int32_t)fn_0151d5(s);
    int32_t X = *(int32_t *)(s + 0xc3);
    int32_t g12 = *(int32_t *)(g_dat_63354 + 0x12);
    int32_t g16 = *(int32_t *)(g_dat_63354 + 0x16);
    int32_t cnt = *(int32_t *)(s + 0xab);
    if (F > X) {
        if (F - param2 <= X) { *out = 0; return 1; }
        if (F - param2 <= g12) { *out = 0; return 1; }
        *out = -param2;
        if (cnt < 0) return 1;
        *(int32_t *)(s + 0xab) = cnt - 1; return 0;
    } else {
        if (F >= X) { *out = 0; return 1; }
        if (param2 + F >= X) { *out = 0; return 1; }
        if (param2 + F >= g16) { *out = 0; return 1; }
        *out = param2;
        if (cnt < 0) return 1;
        *(int32_t *)(s + 0xab) = cnt - 1; return 0;
    }
}

int32_t fn_0174a2(uint8_t *s, int32_t param2, int32_t *out) {          /* 0x0174a2: clamp-check to [0,0x140); adjust out if out of range */
    *(int32_t *)(s + 0x17) = 2;
    int32_t F = (int32_t)fn_01519f(s);
    if (F >= 0 && F < 0x140) return 1;
    if ((int32_t)fn_01519f(s) < 0) *out += param2;
    if ((int32_t)fn_01519f(s) > 0x13f) *out -= param2;
    return 0;
}

void fn_034635(uint8_t key) {                                  /* 0x034635: keyboard input state machine */
    key = key & 0xff;
    uint8_t state = g_dat_62c58[key];
    if (state == 4) {
        if (key == 0x4d || key == 0x4b) {
            if (g_dat_62a94 == 0) fn_034353((uint8_t)(0xb1 - key), g_dat_63f08);
            else                   fn_034353((uint8_t)(key + 0x19), g_dat_63f08);
        } else {
            fn_034353(key, g_dat_63f08);
        }
        if (g_dat_62cd8[key] != 0) { g_dat_62c58[key] = 1; g_dat_62cd8[key]--; }
        else                       { g_dat_62c58[key] = 0; }
    } else {
        if (state == 1) g_dat_62a94 = *(int32_t *)(g_dat_53c7c + 0x1b);
        g_dat_62c58[key]++;
    }
}

uint32_t fn_0100d9(uint32_t *s) {                                   /* 0x0100d9: free container {count, array} */
    uint32_t count = s[0];
    uint32_t array_off = s[1];
    if (array_off != 0) {
        uint32_t *array = (uint32_t *)(g_heap_base + array_off);
        for (uint32_t i = 0; i < count; i++) {
            uint32_t off = array[i];
            if (off != 0) {
                fn_010376((uint8_t *)(g_heap_base + off), 0);   /* destroy struct fields */
                heap_free_guard(off);                            /* free the struct */
            }
        }
        heap_free_host(array_off);                               /* free the array */
    }
    return (uint32_t)s;
}

int32_t fn_015a1f(uint8_t *s) {                                      /* 0x015a1f: state machine + table distance check */
    int32_t tX = (int32_t)fn_01519f(g_dat_53c7c);
    int32_t tY = (int32_t)fn_0151d5(g_dat_53c7c);
    if ((int32_t)(*(uint32_t *)(s + 0x17)) == 0x1f) {                /* reached target state */
        g_dat_53c6c = 1;
        return 0;
    }
    if ((*(uint32_t *)(s + 0x17)) >= 0xb && (*(uint32_t *)(s + 0x17)) <= 0x14) {  /* in [0xb,0x14] */
        *(uint32_t *)(s + 0x117) = 1;
        g_dat_53c6c = 1;
        *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        return 2;
    }
    /* out of [0xb,0x14] */
    *(uint32_t *)(s + 0x117) = 0;
    *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        int32_t idx = (int32_t)(*(int32_t *)(s + 0x113)) * 10 + (int32_t)(*(int32_t *)(s + 0xd)) * 0x88;
        /* LATENT BUG: idx can be negative (signed prng%10) -> out-of-bounds table read (1:1). */
        if (abs32((int32_t)fn_01519f(s) - tX) < (int32_t)g_dat_53eb8[idx]) {
            if (abs32((int32_t)fn_0151d5(s) - tY) < 6) return 1;
        }
    }
    return 0;
}

static int32_t fn_015b5b_core(uint8_t *s, const uint8_t *magics, int32_t nmagics) {   /* 0x015b5b family core (magic-char SET) */
    int32_t tX = (int32_t)fn_01519f(g_dat_53c7c);
    int32_t tY = (int32_t)fn_0151d5(g_dat_53c7c);
    if ((int32_t)(*(uint32_t *)(s + 0x17)) == 0x1f) {
        g_dat_53c6c = 1;
        return 0;
    }
    if ((*(uint32_t *)(s + 0x17)) >= 0xb && (*(uint32_t *)(s + 0x17)) <= 0x14) {
        *(uint32_t *)(s + 0x117) = 1;
        g_dat_53c6c = 1;
        *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        return 2;
    }
    *(uint32_t *)(s + 0x117) = 0;
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        if ((int32_t)(*(uint32_t *)(s + 0x83)) == 0) {
            *(uint32_t *)(s + 0x83) = 1;
            *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        }
    } else {
        *(uint32_t *)(s + 0x83) = 0;
    }
    int32_t idx = (int32_t)(*(int32_t *)(s + 0x113)) * 10 + (int32_t)(*(int32_t *)(s + 0xd)) * 0x88;
    int32_t tbl0 = (int32_t)g_dat_53eb8[idx];
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        int32_t tbl1 = (int32_t)g_dat_53eb8[idx + 1];
        int32_t is_magic = 0;
        for (int32_t i = 0; i < nmagics; i++) if ((uint32_t)tbl1 == magics[i]) { is_magic = 1; break; }
        if (is_magic) {
            if (abs32((int32_t)fn_01519f(s) - tX) < tbl0) return 1;
        } else {
            if (abs32((int32_t)fn_01519f(s) - tX) < tbl0) {
                if (abs32((int32_t)fn_0151d5(s) - tY) < 10) return 1;
            }
        }
    }
    return 0;
}

static const uint8_t fn_mg_dot[1]      = {0x2e};
static const uint8_t fn_mg_plus[1]     = {0x2b};
static const uint8_t fn_mg_comma[1]    = {0x2c};
static const uint8_t fn_mg_comma_dash[2] = {0x2c, 0x2d};   /* ',' '-' */
static const uint8_t fn_mg_14dot[3]    = {0x31, 0x34, 0x2e}; /* '1' '4' '.' */
int32_t fn_015b5b(uint8_t *s) { return fn_015b5b_core(s, fn_mg_dot, 1); }
int32_t fn_01603e(uint8_t *s) { return fn_015b5b_core(s, fn_mg_plus, 1); }
int32_t fn_0161f9(uint8_t *s) { return fn_015b5b_core(s, fn_mg_plus, 1); }
int32_t fn_0163b4(uint8_t *s) { return fn_015b5b_core(s, fn_mg_comma, 1); }
int32_t fn_01656f(uint8_t *s) { return fn_015b5b_core(s, fn_mg_comma_dash, 2); }
int32_t fn_016750(uint8_t *s) { return fn_015b5b_core(s, fn_mg_14dot, 3); }

int32_t fn_016fec(uint8_t *s, int32_t yref) {                                /* 0x016fec: grid-collision detector */
    if (g_dat_51c98 == 1) {                                            /* reinit step */
        g_dat_51c98 = 2;
        g_dat_53c90 = 0;
        return 0;
    }
    *(int32_t *)(s + 0x17) = 1;
    int32_t Xg = (int32_t)fn_01519f(s) / 0x14;                          /* X / 20 (signed) */
    int32_t Yg = (int32_t)fn_0151d5(s) / 0xc;                           /* Y / 12 (signed) */
    uint8_t *node = g_dat_53c70;
    for (int32_t i = 0; i < g_dat_53c68; i++) {
        if (node != g_dat_53c7c && node != s &&
            (int32_t)(*(uint32_t *)(node + 0x1f)) == 4) {
            int32_t nXg = (int32_t)fn_01519f(node) / 0x14;
            int32_t nYg = (int32_t)fn_0151d5(node) / 0xc;
            if (nXg == Xg && nYg == Yg) {
                *(int32_t *)(s + 0xbb) = (int32_t)prng_next() % 4;
                g_dat_51c98 = 1;
                return 2;
            }
        }
        node = *(uint8_t **)(node + 0x13f);
    }
    if (g_dat_53c90 == 0 &&
        0x83u <= *(uint32_t *)(g_dat_53c7c + 0x17) && *(uint32_t *)(g_dat_53c7c + 0x17) <= 0x85u) {
        if (abs32((int32_t)fn_0151d5(s) - yref) < 6) {
            if ((int32_t)prng_next() % 100 < 0x32) {
                g_dat_51c98 = 1;
                g_dat_53c90 = 1;
                return 3;
            }
            g_dat_53c90 = 1;
        }
    }
    if ((int32_t)(*(uint32_t *)(s + 0xab)) < 0) {
        g_dat_51c98 = 1;
        return 1;
    }
    *(int32_t *)(s + 0xab) -= 1;
    return 0;
}

int32_t fn_017ca6(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2) {   /* 0x017ca6: Bresenham diagonal stepper (primary axis = larger |velocity|) */
    *(int32_t *)(s + 0x17) = 2;
    int32_t Vx = *(int32_t *)(s + 0xf3);
    int32_t Vy = *(int32_t *)(s + 0xf7);
    if (abs32(Vx) > abs32(Vy)) {                                    /* X-dominant */
        int32_t X = (int32_t)fn_01519f(s);
        if (Vx < 0) { if (X < *(int32_t *)(s + 0xbf)) return 1; }
        else         { if (X > *(int32_t *)(s + 0xbf)) return 1; }
        *out1 = (Vx < 0) ? -param2 : param2;
        *out2 = 0;
        *(int32_t *)(s + 0xfb) += abs32(Vy);
        if (abs32(Vx) < *(int32_t *)(s + 0xfb)) {
            *(int32_t *)(s + 0xfb) -= abs32(Vx);
            *out2 = (Vy < 0) ? -param2 : param2;
        }
    } else {                                                        /* Y-dominant */
        int32_t Y = (int32_t)fn_0151d5(s);
        int32_t g12 = *(int32_t *)(g_dat_63354 + 0x12);
        int32_t g16 = *(int32_t *)(g_dat_63354 + 0x16);
        if (Vy < 0) {
            if (Y < *(int32_t *)(s + 0xc3) || Y - param2 <= g12) return 1;
        } else {
            if (Y > *(int32_t *)(s + 0xc3) || *(int32_t *)(s + 0xef) + param2 >= g16) return 1;
        }
        *out2 = (Vy < 0) ? -param2 : param2;
        *(int32_t *)(s + 0xfb) += abs32(Vx);
        *out1 = 0;
        if (abs32(Vy) < *(int32_t *)(s + 0xfb)) {
            *(int32_t *)(s + 0xfb) -= abs32(Vy);
            *out1 = (Vx < 0) ? -param2 : param2;
        }
    }
    return 0;
}

int32_t fn_01730b(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2) {   /* 0x01730b: 2-phase X-then-Y move-toward-target */
    *(int32_t *)(s + 0x17) = 2;
    int32_t X = (int32_t)fn_01519f(s);
    int32_t Y = (int32_t)fn_0151d5(s);
    int32_t step = param2;
    int32_t st = *(int32_t *)(s + 0xaf);   // 2-bit done counter (bit0=X, bit1=Y)
    if (st != 1) {                                          // phase 1: X toward s[0xb3]
        if (X < *(int32_t *)(s + 0xb3)) {
            if (X + step < *(int32_t *)(s + 0xb3)) *out1 = step;
            else st++;
        } else if (X > *(int32_t *)(s + 0xb3)) {
            if (X - step > *(int32_t *)(s + 0xb3)) *out1 = -step;
            else st++;
        } else {
            st++;
        }
    }
    if (st != 2) {                                          // phase 2: Y toward s[0xb7] (g_dat_63354 bounds)
        int32_t g12 = *(int32_t *)(g_dat_63354 + 0x12);
        int32_t g16 = *(int32_t *)(g_dat_63354 + 0x16);
        if (Y < *(int32_t *)(s + 0xb7)) {
            if (Y + step < *(int32_t *)(s + 0xb7) && Y + step < g16) *out2 = step;
            else st += 2;
        } else if (Y > *(int32_t *)(s + 0xb7)) {
            if (Y - step > *(int32_t *)(s + 0xb7) && Y - step > g12) *out2 = -step;
            else st += 2;
        } else {
            st += 2;
        }
    }
    *(int32_t *)(s + 0xaf) = st;
    return st >= 3;
}

int32_t fn_017927(uint8_t *s, int32_t val, int32_t *out1, int32_t *out2) {   /* 0x017927: 2D move/AI (double-deref bounds) */
    *(int32_t *)(s + 0x17) = 2;
    int32_t lo   = *(int32_t *)(g_dat_63354 + 0x12);
    int32_t hi   = *(int32_t *)(g_dat_63354 + 0x16);
    int32_t mid  = lo + ((hi - lo) >> 1);
    int32_t s67  = *(int32_t *)(s + 0xcf) - *(int32_t *)(s + 0xc3);
    *(int32_t *)(s + 0x67) = s67;
    int32_t bf = *(int32_t *)(s + 0xbf);
    int32_t cb = *(int32_t *)(s + 0xcb);
    int32_t c3 = *(int32_t *)(s + 0xc3);
    int32_t cf = *(int32_t *)(s + 0xcf);
    int32_t target = ((bf + cb) >> 1) + s67;
    int32_t X = (int32_t)fn_01519f(s);
    if (bf < cb) {
        int32_t d = cb - X;
        if ((d % 3) == 0) {
            if (X > target) { if (cf < mid) *out1 = val; else *out1 = -val; }   /* T1 */
            else            { if (cf < mid) *out1 = -val; else *out1 = val; }    /* T2 */
        }
        if ((X - val) <= (bf + 0x1e)) {
            int32_t Y = (int32_t)fn_0151d5(s);
            if (cf < mid) { if ((Y - val) <= c3) { *out1 = 0; *out2 = 0; return 1; } }
            else          { if ((val + Y) >= c3) { *out1 = 0; *out2 = 0; return 1; } }
        }
    } else {
        int32_t d = X - cb;
        if ((d % 3) == 0) {
            if (X > target) { if (cf > mid) *out1 = val; else *out1 = -val; }   /* E1 */
            else            { if (cf > mid) *out1 = -val; else *out1 = val; }    /* E2 */
        }
        if ((val + X) >= (bf - 0x1e)) {
            int32_t Y = (int32_t)fn_0151d5(s);
            if (cf < mid) { if ((Y - val) <= c3) { *out1 = 0; *out2 = 0; return 1; } }
            else          { if ((val + Y) >= c3) { *out1 = 0; *out2 = 0; return 1; } }
        }
    }
    if (bf < cb) *out2 = -val; else *out2 = val;   /* common tail (0x017c69) */
    return 0;
}

int32_t fn_01776f(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2) {   /* 0x01776f: 2D movement delta computer */
    *(int32_t *)(s + 0x17) = 2;
    int32_t target = *(int32_t *)(s + 0xc3);
    int32_t end    = *(int32_t *)(s + 0xcf);
    int32_t F      = (int32_t)fn_0151d5(s);
    int32_t mid    = (target + end) / 2;
    if (F > mid) {
        *out1 = (*(int32_t *)(s + 0xcb) < 0xa0) ? param2 : -param2;
    } else {
        *out1 = (*(int32_t *)(s + 0xcb) < 0xa0) ? -param2 : param2;
    }
    F = (int32_t)fn_0151d5(s);
    if (target < end) {
        if (F - param2 <= target) { *out2 = 0; *out1 = 0; return 1; }
        *out2 = -param2;
    } else {
        if (F + param2 >= target) { *out2 = 0; *out1 = 0; return 1; }
        *out2 = param2;
    }
    return 0;
}

void fn_032158(void) {                                            /* 0x032158: set entity targets to player + prng jitter */
    if (g_dat_53c6c != 1) return;
    uint8_t *node = g_dat_53c70;
    for (int32_t i = 0; i < g_dat_53c68; i++) {
        if (node != g_dat_53c7c &&
            (int32_t)(*(uint32_t *)(node + 0x117)) == 0 &&
            (int32_t)(*(uint32_t *)(node + 0x1f)) == 4 &&
            (int32_t)(*(uint32_t *)(node + 0xaf)) != 6) {
            int32_t tx = (int32_t)fn_01519f(g_dat_53c7c);
            int32_t ty = (int32_t)fn_0151d5(g_dat_53c7c);
            *(int32_t *)(node + 0xb3) = tx;
            *(int32_t *)(node + 0xb7) = ty;
            if ((int32_t)(*(uint32_t *)(g_dat_53c7c + 0x1b)) == 0) {
                *(int32_t *)(node + 0xb3) -= 0x2d;
                *(int32_t *)(node + 0xb7) -= 5;
                *(int32_t *)(node + 0xb3) += (int32_t)prng_next() % 10;
                *(int32_t *)(node + 0xb7) += (int32_t)prng_next() % 10;
            } else {
                *(int32_t *)(node + 0xb3) += 0x2d;
                *(int32_t *)(node + 0xb7) -= 5;
                *(int32_t *)(node + 0xb3) -= (int32_t)prng_next() % 10;
                *(int32_t *)(node + 0xb7) += (int32_t)prng_next() % 10;
            }
            *(uint32_t *)(node + 0xaf) = 0;
            *(uint32_t *)(node + 0x1f) = 6;
        }
        node = *(uint8_t **)(node + 0x13f);
    }
    g_dat_53c6c = 0;
}

int32_t fn_015ed1(uint8_t *s) {                                      /* 0x015ed1: family variant (s[0x83] gate, no char, Y-limit 10) */
    int32_t tX = (int32_t)fn_01519f(g_dat_53c7c);
    int32_t tY = (int32_t)fn_0151d5(g_dat_53c7c);
    if ((int32_t)(*(uint32_t *)(s + 0x17)) == 0x1f) {
        g_dat_53c6c = 1;
        return 0;
    }
    if ((*(uint32_t *)(s + 0x17)) >= 0xb && (*(uint32_t *)(s + 0x17)) <= 0x14) {
        *(uint32_t *)(s + 0x117) = 1;
        g_dat_53c6c = 1;
        *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        return 2;
    }
    *(uint32_t *)(s + 0x117) = 0;
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        if ((int32_t)(*(uint32_t *)(s + 0x83)) == 0) {
            *(uint32_t *)(s + 0x83) = 1;
            *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        }
    } else {
        *(uint32_t *)(s + 0x83) = 0;
    }
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        int32_t idx = (int32_t)(*(int32_t *)(s + 0x113)) * 10 + (int32_t)(*(int32_t *)(s + 0xd)) * 0x88;
        int32_t tbl0 = (int32_t)g_dat_53eb8[idx];
        if (abs32((int32_t)fn_01519f(s) - tX) < tbl0) {
            if (abs32((int32_t)fn_0151d5(s) - tY) < 10) return 1;
        }
    }
    return 0;
}

int32_t fn_015d16(uint8_t *s) {                                      /* 0x015d16: family variant (X-gate + char-selects-Y-limit) */
    int32_t tX = (int32_t)fn_01519f(g_dat_53c7c);
    int32_t tY = (int32_t)fn_0151d5(g_dat_53c7c);
    if ((int32_t)(*(uint32_t *)(s + 0x17)) == 0x1f) {
        g_dat_53c6c = 1;
        return 0;
    }
    if ((*(uint32_t *)(s + 0x17)) >= 0xb && (*(uint32_t *)(s + 0x17)) <= 0x14) {
        *(uint32_t *)(s + 0x117) = 1;
        g_dat_53c6c = 1;
        *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        return 2;
    }
    *(uint32_t *)(s + 0x117) = 0;
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        if ((int32_t)(*(uint32_t *)(s + 0x83)) == 0) {
            *(uint32_t *)(s + 0x83) = 1;
            *(int32_t *)(s + 0x113) = (int32_t)prng_next() % 10;
        }
    } else {
        *(uint32_t *)(s + 0x83) = 0;
    }
    int32_t idx = (int32_t)(*(int32_t *)(s + 0x113)) * 10 + (int32_t)(*(int32_t *)(s + 0xd)) * 0x88;
    int32_t tbl0 = (int32_t)g_dat_53eb8[idx];
    if ((int32_t)(*(uint32_t *)(s + 0x1f)) == 4) {
        if (abs32((int32_t)fn_01519f(s) - tX) < tbl0) {              /* X-gate first */
            int32_t tbl1 = (int32_t)g_dat_53eb8[idx + 1];
            if (tbl1 == 0x2b) {                                       /* '+' -> Y limit 40 */
                if (abs32((int32_t)fn_0151d5(s) - tY) < 0x28) return 1;
            } else {                                                  /* else Y limit 10 */
                if (abs32((int32_t)fn_0151d5(s) - tY) < 10) return 1;
            }
        }
    }
    return 0;
}

/* 0x0492d0: node/device-table accessor. g_dat_53730 points to a uint32 array
 * (4 bytes per node); each node's byte1 holds flags: 0x40=initialized, 0x20=open.
 * param_1 (EAX) = index, param_2 (EDX) = ctx carried in the return's high dword.
 * Lazily initializes index<6 nodes (set 0x40, call ioctl_device, set 0x20 on success).
 * Returns (ctx<<32)|node_value (0 low dword if index >= bound). */
uint64_t fn_0492d0(uint32_t index, uint32_t ctx) {
    if (index >= g_dat_536dc) return ((uint64_t)ctx) << 32;     /* 0x492dd */
    if (index < 6) {                                            /* 0x492e6 */
        uint8_t *node = (uint8_t *)(g_dat_53730 + index);      /* base + index*4 */
        if ((node[1] & 0x40) == 0) {                           /* 0x492f7 */
            node[1] |= 0x40;                                   /* 0x49301 */
            if (ioctl_device(index) != 0) node[1] |= 0x20;     /* 0x49306/314 */
        }
    }
    return (((uint64_t)ctx) << 32) | g_dat_53730[index];       /* 0x4931e */
}

/* 0x044a50: reg-ABI EAX=index, EDX=ctx. 46 insns. Lazy-initialize the node via
 * fn_0492d0 (low dword = g_arr_53730[index]); mark the slot via store_arr53730
 * (low|0x40); if the marked value's bit13 (0x2000) is set, two AH=0x44 ioctls
 * (AL=0 then AL=1) each with the RCL/ROR sign reconstruction; a negative result
 * routes to fn_045a82 (error, returns -1). Success returns 0. */
int32_t fn_044a50(uint32_t index, uint32_t ctx) {
    uint64_t r = fn_0492d0(index, ctx);              /* 0x44a59 */
    uint32_t eax = (uint32_t)r | 0x40;                /* OR AL,0x40 */
    store_arr53730(index, eax);                       /* 0x44a66 */
    if (eax & 0x2000u) {                              /* TEST BH,0x20; JZ */
        uint32_t dx, cf, rcl, ror;
        sys_dos_ioctl(index, &dx, &cf);               /* AL=0 AH=0x44 INT 21 */
        rcl = (dx << 1) | cf; ror = (rcl >> 1) | (rcl << 31);
        if (ror & 0x80000000u)                        /* TEST EDX,EDX; JGE not taken */
            return (int32_t)fn_045a82(ror & 0xffffu, ror);
        sys_dos_ioctl(index, &dx, &cf);               /* AL=1 AH=0x44 INT 21 */
        rcl = (dx << 1) | cf; ror = (rcl >> 1) | (rcl << 31);
        if (ror & 0x80000000u)                        /* TEST EDX,EDX; JL taken */
            return (int32_t)fn_045a82(ror & 0xffffu, ror);
    }
    return 0;                                          /* XOR EAX,EAX */
}

/* 0x049236: reg-ABI EAX=index, EDX=ctx, EBX=count. 55 insns. fn_0492d0 lazy-init;
 * if the low byte's bit7 (0x80) is set, AH=0x42 lseek(whence=2,end, offset=0) with
 * a 32-bit sign gate -> fn_045a82 (-1); then AH=0x40 write(handle=index, buf=ctx,
 * count) with a sign gate -> fn_045a82; if written != count, fn_042a0d(0xc); return
 * the written count. */
int32_t fn_049236(uint32_t index, uint32_t ctx, uint32_t count) {
    uint64_t r = fn_0492d0(index, ctx);               /* 0x49244 */
    if ((uint32_t)r & 0x80u) {                         /* TEST AL,0x80; JZ */
        int32_t pos = sys_file_lseek(index, 0, 2);     /* AH=0x42 end */
        if (pos < 0)                                   /* TEST EDX,EDX; JGE not taken */
            return (int32_t)fn_045a82((uint32_t)pos & 0xffffu, (uint32_t)pos);
    }
    int32_t written = sys_file_write(index, (const void *)(uintptr_t)ctx, count); /* AH=0x40 */
    if (written < 0)                                   /* TEST EAX,EAX; JGE not taken */
        return (int32_t)fn_045a82((uint32_t)written & 0xffffu, (uint32_t)written);
    if ((uint32_t)written != count) fn_042a0d(0xc);    /* 0x4929a CMP/JZ */
    return written;                                    /* 0x492a8 EAX=EDX */
}

/* 0x044748: reg-ABI EAX=io_state_t*. Flush path (f12 byte1 bit4): clear byte1 bit1;
 * if byte0 bit1 set and f8!=0, fn_049236(handle=f16,buffer=f8,count=f4); if -1 or
 * !=f4, set byte0 bit5 + return -1 (partial also fn_042a0d(0xc)). Else seek-back
 * path: if f8!=0 clear byte0 bit1; if byte1 bit5 clear and f4!=0, fn_044807(f16,-f4,1);
 * if -1 set byte0 bit5 + return -1. Tail always sets f0=f8, f4=0. Returns 0/-1. */
int32_t fn_044748(io_state_t *in) {
    int32_t esi = 0;
    uint32_t b13 = (in->f12 >> 8) & 0xff;          /* byte @0x0d */
    if (b13 & 0x10u) {                              /* flush path */
        in->f12 = (in->f12 & 0xffff00ffu) | ((((in->f12 >> 8) & 0xff) & 0xefu) << 8); /* byte1 &= 0xef */
        uint32_t b12 = in->f12 & 0xff;              /* byte @0x0c */
        if ((b12 & 0x2u) && in->f8 != 0) {
            int32_t r = fn_049236(in->f16, in->f8, in->f4);   /* handle, buffer, count */
            if (r == -1) { in->f12 |= 0x20u; esi = -1; }
            else if (r != (int32_t)in->f4) { fn_042a0d(0xc); in->f12 |= 0x20u; esi = -1; }
        }
    } else {                                        /* seek-back path */
        if (in->f8 != 0) {
            in->f12 = (in->f12 & 0xffffff00u) | ((in->f12 & 0xff) & 0xefu); /* byte0 &= 0xef */
            uint32_t b13b = (in->f12 >> 8) & 0xff;
            if ((b13b & 0x20u) == 0) {              /* byte1 bit5 clear */
                int32_t r;
                if (in->f4 != 0) r = fn_044807(in->f16, -(int32_t)in->f4, 1);
                else r = 0;                          /* f4==0: EAX=0, skip call */
                if (r == -1) { in->f12 |= 0x20u; esi = -1; }
            }
        }
    }
    in->f0 = in->f8;                                /* 0x0447fe */
    in->f4 = 0;                                     /* 0x0447f7 */
    return esi;
}

/* 0x04488b: reg-ABI, 4 stack args (name, mode, extra, val). File open/create handler.
 *
 * - skip leading spaces in name
 * - open (AH=3d, mode=(mode&0x83)|extra); handle=res&0xffff if res>=0 else -1
 * - if (mode&3) && handle!=-1 && ioctl_device(handle)==0:
 *     - if (mode>>8)&4: close, fn_042a0d(7), return -1
 *     - if (mode&0x40): write 0 bytes; if res<0: close, return fn_045a82(res, buf)
 * - if handle==-1 (open failed):
 *     - if !(mode&0x20) || open_res!=2: return fn_045a82(open_res, name)
 *     - create (AH=3c, CX=((val|0x180)^g_dat_536d8)&0x80 ? 0 : 1), close, reopen (AH=3d);
 *       any res<0 -> return fn_045a82(res, name); else handle=res&0xffff
 * - node flags = fn_0492d0(handle,0) & 0x3c; accumulates into the stored value:
 *     value = flags | (ioctl_device(handle)!=0 ? 0x2000 : 0)   (OR DH,0x20)
 *           | (m==0?0x1 : m==1?0x2 : m==2?0x3 : 0)             (OR DL,x), m=(mode&0x83|extra)&0x7f
 *           | (mode&0x10 ? 0x80 : 0)                           (OR DL,0x80)
 * - stored = value|0x40 if ((mode_hi&3)&&(mode_hi&2)) || ((mode_hi&3)==0 && g_dat_53619==0x200)
 *            else value.  store_arr53730(handle, stored); return handle.
 */
int32_t fn_04488b(const uint8_t *name, uint16_t mode, uint8_t extra, uint32_t val)
{
    while (*name == 0x20) name++;                                   /* 0x044897-9d */
    uint32_t open_mode = (mode & 0x83u) | (uint32_t)extra;          /* 0x04489f-b2 */
    int32_t open_res = sys_file_open((const char *)name, open_mode);/* 0x0448bb-bd */
    int32_t handle = (open_res >= 0) ? (int32_t)(open_res & 0xffff) : -1;  /* 0x0448c7-cd */

    if ((mode & 0x3) && handle != -1 && ioctl_device((uint32_t)handle) == 0) {  /* 0x0448d2-ed */
        if ((mode >> 8) & 0x4) {                                     /* 0x0448ef-f4 */
            sys_file_close((uint32_t)handle);                        /* 0x0448f6-fa */
            fn_042a0d(7);                                            /* 0x044900-05 */
            return -1;                                               /* 0x04490a-0f */
        }
        if ((mode & 0x40)) {                                         /* 0x044914-17 */
            uint8_t wbuf;                                            /* 0x044919: [ESP+8] */
            int32_t wres = sys_file_write((uint32_t)handle, &wbuf, 0); /* 0x04491d-23 */
            if (wres < 0) {                                          /* 0x04492d-2f */
                sys_file_close((uint32_t)handle);                    /* 0x044931-35 */
                return (int32_t)fn_045a82((uint32_t)(wres & 0xffff), (uint32_t)(uintptr_t)&wbuf); /* 0x04493b-42 */
            }
        }
    }

    if (handle == -1) {                                              /* 0x04494c-4f */
        if (((mode & 0x20) == 0) || ((open_res & 0xffff) != 2)) {   /* 0x044955-62 (CMP WORD) */
            return (int32_t)fn_045a82((uint32_t)(open_res & 0xffff), (uint32_t)(uintptr_t)name); /* 0x04493b */
        }
        uint32_t cmode = ((val == 0) ? 0x180u : val) ^ g_dat_536d8;  /* 0x044964-77 */
        uint32_t ccreate = (cmode & 0x80u) ? 0u : 1u;                /* 0x04497d-88: CX */
        int32_t cres = sys_file_create((const char *)name, ccreate); /* 0x044990-92 */
        if (cres < 0)                                                /* 0x04499c-9e */
            return (int32_t)fn_045a82((uint32_t)(cres & 0xffff), (uint32_t)(uintptr_t)name);
        (void)sys_file_close((uint32_t)(cres & 0xffff));            /* 0x0449a0-a8 */
        int32_t ores = sys_file_open((const char *)name, open_mode); /* 0x0449b4-c7 */
        if (ores < 0)                                                /* 0x0449cb-cd */
            return (int32_t)fn_045a82((uint32_t)(ores & 0xffff), (uint32_t)(uintptr_t)name);
        handle = (int32_t)(ores & 0xffff);                           /* 0x0449d3-d5 */
    }

    uint32_t value = (uint32_t)(fn_0492d0((uint32_t)handle, 0) & 0x3c); /* 0x0449da-e3 */
    if (ioctl_device((uint32_t)handle) != 0) value |= 0x2000u;        /* 0x0449e5-f0: OR DH,0x20 */
    uint32_t m = open_mode & 0x7fu;                                  /* 0x0449f3 */
    if      (m == 0) value |= 0x1;                                   /* 0x044a00-06 */
    else if (m == 1) value |= 0x2;                                   /* 0x044a09-0f */
    else if (m == 2) value |= 0x3;                                   /* 0x0449f7-fd */
    if (mode & 0x10) value |= 0x80;                                  /* 0x044a12-19 */
    uint8_t mhi = (uint8_t)((mode >> 8) & 0xff);                     /* 0x044a24 */
    uint32_t stored;
    if (((mhi & 0x3) && (mhi & 0x2)) || ((mhi & 0x3) == 0 && g_dat_53619 == 0x200))
        stored = value | 0x40;                                       /* 0x044a1c-22+3c: EDX=EAX */
    else
        stored = value;                                              /* 0x044a3e: EDX unchanged */
    store_arr53730((uint32_t)handle, stored);                        /* 0x044a3e-40 */
    return handle;                                                   /* 0x044a45-47 */
}

/* 0x044867: 4 stack args (p1, p2, name, val); p1/p2 unused. Thin wrapper that
 * calls fn_04488b(name, mode=0, extra=val&0xff, val=val). (PUSH name,0,val,val) */
int32_t fn_044867(uint32_t p1, uint32_t p2, const uint8_t *name, uint32_t val)
{
    (void)p1; (void)p2;
    return fn_04488b(name, 0, (uint8_t)(val & 0xff), val);   /* 0x044873-0x044880 */
}

/* 0x042a19: MOV EAX,0xe; JMP 0x042a0d -> g_dat_64008 = 0xe. */
void fn_042a19(void) { g_dat_64008 = 0xe; }

/* 0x044b96: drain the done-node list. Each node (8B at g_ndpool+off: {next,rec})
 * is freed via heap_free_host and the head advances to node->next until empty.
 * (The original passes EDX=node->next to heap_free_host, but that fn only uses EAX.) */
void fn_044b96(void) {
    while (g_node_list_done != 0) {                    /* 0x44b97/b9d/b9f */
        uint32_t node_off = g_node_list_done;          /* 0x44ba1 */
        struct nd_node *node = (struct nd_node *)(g_ndpool + node_off);
        g_node_list_done = node->next;                 /* 0x44ba3/baa */
        heap_free_host(node_off);                      /* 0x44ba5 */
    }
}

/* 0x045079: format value as hex (itoa32s, base 16) then right-justify + zero-pad
 * into buf to exactly `width` chars, null-terminated at buf[width]. ABI: value=EAX,
 * buf=EDX, width=EBX. (The original computes strlen via a SCASB countdown and pads
 * the leading part with '0'.) */
void fn_045079(uint32_t value, char *buf, int width) {
    itoa32s((int32_t)value, buf, 0x10);                /* 0x4508c */
    int len = 0;
    while (buf[len] != 0) len++;                       /* 0x45091-0x450a0 strlen */
    int pad = width - 1;                               /* 0x450a8 EAX = width-1 */
    int src = len;                                     /* 0x450ac EDX = buf+len */
    int dst = width - 1;                               /* 0x450af EBX = buf+width-1 */
    while (src != 0) {                                 /* 0x450b9: EDX != buf */
        src--;                                         /* 0x450b1 */
        pad--;                                         /* 0x450b5 EAX-- */
        buf[dst] = buf[src];                           /* 0x450b3/b6 */
        dst--;                                         /* 0x450b8 */
    }
    for (int i = pad; i > -1; i--)                     /* 0x450bd+ fill '0' */
        buf[i] = '0';
    buf[width] = 0;
}

/* 0x0450da: 16.16 fixed-point value -> decimal string. value = signed 16.16;
 * spec[2] = decimal places (default -1 -> 4). Writes to out, leading '-' if
 * negative. Integer part via itoa32s(value>>16); fractional digits extracted by
 * repeated frac*10 / 65536. Rounds up (with carry) when the discarded fractional
 * remainder has bit 15 set. */
void fn_0450da(char *out, int32_t value, int *spec) {
    char *num_start = out;
    uint32_t local_10;
    if (value < 0) {
        local_10 = (uint32_t)(0u - (uint32_t)value);   /* magnitude (INT_MIN wrap-safe) */
        num_start = out + 1;
        out[0] = '-';                                  /* 0x2d */
    } else {
        local_10 = (uint32_t)value;
    }
    if (spec[2] == -1) spec[2] = 4;                    /* default 4 places */
    itoa32s((int32_t)(local_10 >> 16), num_start, 10); /* integer part */
    char *pc = num_start;
    while (*pc) pc++;                                  /* pc = end of integer part */
    if (spec[2] != 0) {
        *pc = '.';
        char *p = pc + 1;
        for (int i = 0; i < spec[2]; i++) {
            local_10 &= 0xffff;
            uint32_t uVar5 = local_10 * 10u;
            *p++ = (char)((uVar5 >> 16) & 0xffff) + '0';
            local_10 = uVar5;
        }
        *p = 0;
        pc = p;
    }
    if ((local_10 & 0x8000) != 0) {                    /* round up (carry) */
        char *pcVar3 = pc;
        char *pcVar4;
        while (pcVar3 != num_start) {
            pcVar4 = pcVar3 - 1;
            if (*pcVar4 == '.') pcVar4 = pcVar3 - 2;
            if (*pcVar4 != '9') { *pcVar4 = (char)(*pcVar4 + 1); return; }
            *pcVar4 = '0';
            pcVar3 = pcVar4;
        }
        *num_start = '1';                              /* all 9s: prepend 1 */
        char *q = num_start + 1;
        do { pcVar3 = q; q = q + 1; } while (*pcVar3 == '0');
        if (*pcVar3 == '.') {
            *pcVar3 = '0'; pcVar3[1] = '.';
            for (pcVar3 = pcVar3 + 2; *pcVar3 == '0'; pcVar3++);
        }
        *pcVar3 = '0';
        pcVar3[1] = 0;
    }
}

/* 0x043f80: 8-bit presence bitmap builder (66B).
 *
 * Clears a 32-byte bitmap `dest`, then for each byte b in `src` (starting at byte 0)
 * sets dest[b>>3] |= 1<<(b&7). Byte 0 is processed unconditionally if non-zero; the scan
 * then stops at the first 0 or 0x5d (']') byte (that byte is NOT processed). Returns the
 * source cursor position at exit (EAX=ECX).
 *
 * ABI: src=EAX, dest=EDX. The 0x51854 table = {1,2,4,8,16,32,64,128} = 1<<n. Depends on
 * fn_041e20 (memset). Faithful quirk: byte 0 skips the 0x5d check (only bytes >=1 are
 * tested against 0x5d); the cursor at exit is the zero byte's address (not past it) when
 * stopped by 0, but past the 0x5d byte when stopped by 0x5d.
 */
const uint8_t *fn_043f80(const uint8_t *src, uint8_t *dest) {
    static const uint8_t tbl[8] = {1,2,4,8,16,32,64,128};   /* 0x51854 */
    fn_041e20(dest, 0, 0x20);   /* clear 32 bytes */
    uint8_t b = src[0];
    src++;
    if (b != 0) {
        for (;;) {
            dest[b >> 3] |= tbl[b & 7];
            uint8_t bn = src[0];
            if (bn == 0) break;      /* cursor at the zero byte */
            src++;
            if (bn == 0x5d) break;   /* cursor past the ']' byte */
            b = bn;
        }
    }
    return src;
}

/* 0x049e15: within-buffer string range shift + sign/zero fixup + space-fill (106B).
 *
 * param_1 = char buffer (dest), param_2 (count2) and param_3 (count3) are small counts.
 * If count2 != count3: a backward in-buffer copy moves dest[0..count2-1] to end at
 * dest[count3-1] (i.e. dest[count3-count2+k] = dest[k], done high-to-low); then a fixup
 * based on dest[0]: '.' -> dest[iVar5-2]='0'; '+'/'-' with dest[1]=='.' -> dest[iVar5-1]
 * ='0', dest[iVar5-2]=dest[0]; finally fn_041e20(dest,' ',cur3) space-fills the front.
 * iVar5 = count3-(count2-1), cur3 = count3-count2 at loop exit (faithful register model).
 * Depends on fn_041e20. The backward copy reads/writes dest[0] too (count2 iterations).
 */
void fn_049e15(char *dest, int count2, int count3) {
    if (count2 != count3) {
        char c1 = dest[1];
        char *s = dest + count2;
        char *d = dest + count3;
        int cur3 = count3;
        int iVar5;
        do {
            iVar5 = cur3;
            s--; d--;
            *d = *s;
            cur3 = iVar5 - 1;
        } while (s != dest);
        char c2 = dest[0];
        if (c2 == '.') {
            dest[iVar5 - 2] = '0';
        } else if ((c2 == '+' || c2 == '-') && c1 == '.') {
            dest[iVar5 - 1] = '0';
            dest[iVar5 - 2] = c2;
        }
        fn_041e20((uint8_t *)dest, 0x20, (uint32_t)cur3);
    }
}

/* 0x042aba: DOS AH=3fh file read (30B). handle=EAX, buffer=EDX, count=EBX(16-bit).
 * Original sets EBX=handle, ECX=count, AH=3f, INT 21, then RCL/ROR/TEST/JGE: on a
 * negative (error) result it does AND EAX,0xffff and calls 0x045a82 (error handler);
 * otherwise returns the byte count in AX. Routed to sys_file_read; the no-op backend
 * returns 0 bytes so the (dead) error path never fires.
 */
int32_t fn_042aba(uint32_t handle, void *buf, uint32_t count) {
    return sys_file_read(handle, buf, count);
}

/* 0x044807: DOS AH=42h lseek (64B). handle=EAX, whence=EBX(16-bit), offset=EDX
 * (DX low : (offset>>16) high, CX). Builds a 2-word local (AX:DX = new position),
 * RCL/ROR/TEST/JGE; on error AND EAX,0xffff + call 0x045a82 and return -1, else
 * return the 32-bit new position. Routed to sys_file_lseek (no-op: pos 0).
 */
int32_t fn_044807(uint32_t handle, int32_t offset, int32_t whence) {
    int32_t pos = sys_file_lseek(handle, offset, whence);
    return pos < 0 ? -1 : pos;   /* mirrors the RCL/ROR/TEST/JGE sign gate */
}

/* 0x044bec: DOS AH=3Eh close-file (23 ins). INT 21h AH=0x3e closes the handle;
 * the RCL/ROR preserves the INT's error flag (CF). On success (CF clear) it marks
 * the handle's slot free via store_arr53730(handle,0) and returns 0; on error it
 * calls fn_042a0d(4) and returns -1. The DOS close maps to sys_file_close. */
int32_t fn_044bec(uint32_t handle)
{
    if (sys_file_close(handle) == 0) {          /* 0x44bf4-44bf8: INT 21h AH=0x3e, CF clear */
        store_arr53730(handle, 0);              /* 0x44c02: mark slot free */
        return 0;                                /* 0x44c07 */
    }
    fn_042a0d(4);                               /* 0x44c11 */
    return -1;                                   /* 0x44c16 */
}

/* 0x049415: DOS AH=41h file delete (30B). name=EAX (moved to EDX for the INT).
 * INT 21, RCL/ROR/TEST/JGE: success -> XOR EAX,EAX (return 0); error -> AND EAX,0xffff
 * + call 0x045a82. Routed to sys_file_delete (no-op: 0 = success).
 */
int32_t fn_049415(const char *name) {
    return sys_file_delete(name);
}

/* 0x041e38: DPMI real-mode trampoline B (24B).
 *
 * Shuffles its four register args into the 32-byte register context at `ctx`
 * (EBX) and calls 0x043324, the DPMI INT 31h dispatcher. In the flat model that
 * call is a no-op (no real-mode transition — see the L0 batch A convention), so the
 * context is left untouched. The function returns the first dword of the context
 * (MOV EAX,[EBP] where EBP=EBX=ctx), which for the no-op is simply *ctx.
 * Arg mapping (original): a=EAX, b=EDX, ctx=EBX, d=ECX.
 */
uint32_t fn_041e38(uint32_t a, uint32_t b, uintptr_t ctx, uint32_t d) {
    (void)a; (void)b; (void)d;
    return *(uint32_t *)ctx;
}

/* 0x044c1e: tail-call thunk (4B) -- JMP 0x049415.
 *
 * Unconditional tail jump to fn_049415 (DOS AH=41 file delete). Same signature,
 * so the flat port is a straight pass-through. */
int32_t fn_044c1e(const char *name) { return fn_049415(name); }

/* 0x041e51: seg-save + DPMI real-mode call (26B), register-ABI.
 *
 * Original: MOV ECX,EAX; MOV EAX,ESP; CALL 0x041df2 (save segments to the stack
 * frame); MOV EAX,ECX; MOV ECX,ESP; CALL 0x041e38 (DPMI INT 31h real-mode
 * trampoline); RET. Its return is fn_041e38's EAX, which the original computes as
 * *EBX (EBX = the caller's register-ABI context pointer) -- i.e. the real-mode
 * result. Both callees are flat-model no-ops (no segment switches, no real-mode
 * transition), so the real-mode result is never produced; the return (caller's
 * *EBX, register-ABI) is undefined in the flat model and approximated as 0.
 * Params model the register-ABI inputs (a=EAX, b=EDX, c=EBX, d=ECX). */
uint32_t fn_041e51(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    (void)a; (void)b; (void)c; (void)d;
    return 0;
}

/* 0x0422d5: DOS file open + seek + terminate wrapper (58B).
 *
 * Original: INT 21h AH=03h/AL=01 open file 0x420b8 (handle in AX); compute
 * strlen(param_2); INT 21h AH=40 seek to that offset; dispatch_b(0, 0xff);
 * INT 21h AH=4c terminate with status param_5. Every INT 21h call is a
 * flat-model no-op stub (swi(0x21)) and the AH=4c terminate is a no-op, so the
 * strlen only feeds the no-op seek and the sole observable effect is the real
 * event dispatch dispatch_b(0, 0xff). Returns the terminate result (0 from the
 * no-op stub). Params model the original stack args (b = the path string). */
uint32_t fn_0422d5(uint32_t a, const char *b, uint32_t c, uint32_t d, uint8_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    dispatch_b(0, 0xff);
    return 0;
}

/* 0x0429c8: fatal-error reporter (28B). Original: PUSH EBX/ECX; EBX=EAX (msg),
 * ECX=EDX (param2), EDX=DS; CALL 0x045a44 (debug_break_hook: fires INT3 if
 * g_debug_break set, always returns 0); TEST/JNZ; if 0 restore EAX/EDX and CALL
 * 0x0429e5 (thunk -> fn_0422d5 fatal-exit path). Flat model: debug_break_hook
 * returns 0, so this always takes the fatal-exit branch (dispatch_b(0,0xff)).
 * Register-ABI inputs: EAX=msg (string), EDX=param2 (unused downstream). */
void fn_0429c8(const char *msg, uint32_t param2) {
    (void)param2;
    if (debug_break_hook((uint32_t)msg, 0) == 0) {
        fn_0422d5(0, msg, 0, 0, 0);
    }
}

/* 0x0429e5: 4B JMP thunk -> fn_0422d5. Same signature, straight pass-through. */
uint32_t fn_0429e5(uint32_t a, const char *b, uint32_t c, uint32_t d, uint8_t e) {
    return fn_0422d5(a, b, c, d, e);
}

/* 0x0429ea: fatal-error reporter (28B), identical to 0x0429c8 but calls
 * fn_0422d5 directly (not via the 0x0429e5 thunk). */
void fn_0429ea(const char *msg, uint32_t param2) {
    (void)param2;
    if (debug_break_hook((uint32_t)msg, 0) == 0) {
        fn_0422d5(0, msg, 0, 0, 0);
    }
}

/* 0x0418c6: 11B -- report "undefined constructor or destructor called!" via
 * fn_0429c8 (the C++ guard for an undefined ctor/dtor). */
void fn_0418c6(void) {
    fn_0429c8("undefined constructor or destructor called!", 0);
}

/* 0x045dad: 16B -- report "Floating-point support not loaded" via fn_0429ea. */
void fn_045dad(void) {
    fn_0429ea("Floating-point support not loaded\r\n", 1);
}

/* 0x043324: DPMI real-mode transition, protected-mode (save) side (55B).
 *
 * Original reads EDI (context ptr) and EBX (real-mode far ptr: [EBX]=ES, [EBX+6]=BP)
 * from the caller's registers (register-ABI), saves EAX/EBX/ECX/EDX/ESI/(popped)/CF
 * into the 7-word context, sets [EBX]=ES and [EBX+6]=AX, then CALLs rm_switch_a
 * (0x04335c) to run the real-mode thunk. In the flat model rm_switch_a is a no-op and
 * no real-mode code runs between save and restore, so the context write is never read
 * back — the whole transition is a no-op. The register-ABI save cannot be reproduced
 * in C (it writes the caller's live registers), so this is a faithful no-op standin.
 */
void fn_043324(uintptr_t context, uintptr_t rm_ptr) {
    (void)context; (void)rm_ptr;   /* flat port never switches to real mode */
}
/* 0x04335c: DPMI real-mode callback trampoline A (restore side) — ported as
 * rm_switch_a (L0 batch A); see that stub. No separate fn_04335c. */

/* 0x03f084: DPMI INT 31h register-context wrapper (218B).
 *
 * Reads 5 16-bit words from `arg2` and one from `arg4` into a local register
 * context, builds the DPMI frame, and calls fn_041e38 (the real-mode trampoline B)
 * with key 0x31. In the flat model seg_save (0x41df2), fn_041e20 (memset), and the
 * DPMI call are no-ops, so the local words are never transformed. The observable
 * protected-mode effect is therefore: copy 4 of the 5 words (byte offsets 0,4,8,0xc)
 * from arg2 to arg3 (the 5th, arg2[0x14], is read into the DPMI offset but NOT written
 * back — a faithful quirk), and return arg2[0xc] zero-extended. arg1/arg4 only feed the
 * (no-op) DPMI context.
 */
uint32_t fn_03f084(const uint16_t *arg1, const uint16_t *arg2, uint16_t *arg3, const uint16_t *arg4) {
    (void)arg1; (void)arg4;
    arg3[0] = arg2[0];
    arg3[2] = arg2[2];
    arg3[4] = arg2[4];
    arg3[6] = arg2[6];   /* arg2[7] (offset 0x14) dropped */
    return (uint32_t)arg2[6];
}

/* 0x0451bc: typed field formatter/deserializer (1096 bytes, ~354 instructions, L3).
 *
 * Reads a field described by a state struct `st` (type code st[0x15] + flags st[0x14])
 * from the byte stream at `*cur` (a read pointer, advanced by element size), formats the
 * value into `out`, updates st's length/size/remainder fields, and returns the output
 * pointer (the original `out` for numeric fields, the resolved source pointer for string
 * fields, or the callback result for E/G/e/g fields).
 *
 * ABI: out=EAX, cur=EDX (pointer to the stream read pointer), st=EBX (state), alt=ECX
 * (fallback pointer for null string sources).
 *
 * State fields: st+0x15 type; st+0x14 flags (0x01/02/04/10/20/40/80); st+0x16 char ('*'
 * sentinel); st+0x17..19 3-byte prefix; st+0x04 size; st+0x08 length; st+0x0c remainder.
 *
 * Faithful quirks (verified in full disasm):
 *  - Pre-pass value read: the word (flags&0x10) case still advances *cur by 4, not 2.
 *  - 'o' prefix: writes '0' at out[0] and formats at out+1; leading-zero suppression writes
 *    null at out[0] when st+0x08==0 and the formatted digit is '0'.
 *  - The epilogue subtracts (prefix_len + length + remainder) from st+0x04 (size).
 *  - 'S' (upper) reads a length/count word and returns the pointer past it; 's' (lower)
 *    computes the string length and returns the string start.
 */
uint8_t *fn_0451bc(uint8_t *out, uint8_t **cur, uint8_t *st, uint8_t *alt) {
    uint32_t val = 0;                 /* EBP: value read in the pre-pass */
    int32_t outlen = 10;              /* EBX default base / uVar8 */
    int has_prefix = 1;               /* bVar7 */
    uint8_t *ret = out;               /* returned pointer (ECX) */
    uint8_t type = st[0x15];
    int32_t *plen = (int32_t *)(st + 0x0c);   /* remainder */
    int32_t *pglen = (int32_t *)(st + 0x08);  /* length */
    int32_t *psize = (int32_t *)(st + 0x04);  /* size */
    *plen = 0;                        /* st+0x0c = 0 */

    /* --- pre-pass: read `val` for {X,o,u,x}; set st[0x16]=0x20 for {d,i} and after reads --- */
    {
        int do_read = 0, do_st16 = 0;
        if (type < 0x69) {
            if (type == 0x58) do_read = 1;        /* 'X' */
            else if (type == 0x64) do_st16 = 1;   /* 'd' */
        } else if (type == 0x69) {
            do_st16 = 1;                          /* 'i' */
        } else if (type < 0x75) {
            if (type == 0x6f) do_read = 1;        /* 'o' */
        } else if (type == 0x75 || type == 0x78) {
            do_read = 1;                          /* 'u', 'x' */
        }
        if (do_read) {
            if (st[0x14] & 0x10) {
                val = *(uint16_t *)(*cur);        /* word, zero-extended */
                *cur += 4;                        /* faithful: still advance by 4 */
            } else {
                val = *(uint32_t *)(*cur);        /* dword */
                *cur += 4;
            }
            st[0x14] &= 0xf9;
            do_st16 = 1;
        }
        if (do_st16 && (int32_t)*pglen != -1) {
            st[0x16] = 0x20;
        }
    }

    /* --- main switch (0x45247) --- */
    switch (type) {
    case 0x45: case 0x47: case 0x65: case 0x67:   /* E G e g -> callback */
        ret = (uint8_t *)(uintptr_t)cb_indirect_53644();
        outlen = (int32_t)strn_len(ret, (size_t)-1);
        break;
    case 0x46: case 0x66:                          /* F f */
        if (st[0x14] & 0x10) {
            val = *(uint32_t *)(*cur); *cur += 4;
            fn_0450da((char *)out, (int32_t)val, (int *)st);
            outlen = (int32_t)strn_len(out, (size_t)-1);
        } else {
            ret = (uint8_t *)(uintptr_t)cb_indirect_53644();
            outlen = (int32_t)strn_len(ret, (size_t)-1);
        }
        break;
    case 0x50: case 0x70:                          /* P p (date/time) */
        if ((int32_t)*psize == 0) *psize = (st[0x14] & 0x80) ? 13 : 8;
        st[0x14] &= 0xf9;
        {
            uint32_t ts = *(uint32_t *)(*cur); *cur += 4;
            if (st[0x14] & 0x80) {
                uint32_t secs = *(uint32_t *)(*cur); *cur += 4;
                fn_045079(secs & 0xffff, (char *)out, 4);
                out[4] = ':';
                fn_045079(ts, (char *)out + 5, 8);
            } else {
                fn_045079(ts, (char *)out, 8);
            }
            if (type == 0x50) string_upper((char *)out);
        }
        outlen = (int32_t)strn_len(out, (size_t)-1);
        break;
    case 0x53: case 0x73:                          /* S s (string) */
        {
            uint8_t *src;
            uint8_t f = st[0x14];
            if (f & 0x80) {
                uint8_t *p = *cur; *cur = p + 8;
                uint32_t ptr = *(uint32_t *)p;
                uint16_t len = *(uint16_t *)(p + 4);
                src = (ptr == 0 && len == 0) ? alt : (uint8_t *)(uintptr_t)ptr;
            } else {
                uint8_t *p = *cur; *cur = p + 4;
                uint32_t ptr = *(uint32_t *)p;
                src = (ptr == 0) ? alt : (uint8_t *)(uintptr_t)ptr;
            }
            st[0x14] &= 0xf9;
            has_prefix = 0;
            if (type == 0x53) {                    /* 'S': read a length/count word */
                if (f & 0x20) { outlen = (int32_t)*(uint16_t *)src; src += 2; }
                else { outlen = (int32_t)*(uint8_t *)src; src += 1; }
            } else {                                /* 's': compute the string length */
                if (f & 0x20) outlen = (int32_t)word_nlen((const uint16_t *)src, (size_t)(int32_t)*pglen);
                else outlen = (int32_t)strn_len(src, (size_t)-1);
            }
            ret = src;
        }
        break;
    case 0x58: case 0x78:                          /* X x (hex) */
        if (st[0x14] & 0x01 && val != 0) {
            st[0x17] = '0'; st[0x18] = type; st[0x19] = 0;
        }
        itoa32_tbl(val, (char *)out, 16);
        if (type == 0x58) string_upper((char *)out);
        if ((int32_t)*pglen == 0 && out[0] == '0') out[0] = 0;
        outlen = (int32_t)strn_len(out, (size_t)-1);
        break;
    case 0x63:                                      /* c (single char) */
        {
            uint8_t b = **cur; *cur += 4;
            out[0] = b; out[1] = 0;
            *pglen = 1; outlen = 1; has_prefix = 0;
            st[0x14] &= 0xf9;
        }
        break;
    case 0x64: case 0x69:                          /* d i (decimal signed) */
        {
            uint32_t f = st[0x14];
            if (f & 0x20) {
                val = *(uint32_t *)(*cur); *cur += 4;
                itoa32s_tbl((int32_t)val, (char *)out, 10);
            } else if (f & 0x10) {
                int16_t w = *(int16_t *)(*cur); *cur += 4;
                itoa32s_tbl((int32_t)w, (char *)out, 10);
            } else {
                val = *(uint32_t *)(*cur); *cur += 4;
                itoa32s((int32_t)val, (char *)out, 10);
            }
        }
        if ((int32_t)*pglen == 0 && out[0] == '0') out[0] = 0;
        outlen = (int32_t)strn_len(out, (size_t)-1);
        break;
    case 0x6f:                                      /* o (octal) */
        {
            uint8_t f = st[0x14];
            uint8_t *wp;
            if (f & 0x01) { out[0] = '0'; wp = out + 1; itoa32s_tbl((int32_t)val, (char *)wp, 8); }
            else { wp = out; itoa32s_tbl((int32_t)val, (char *)out, 8); }
            if ((int32_t)*pglen == 0 && wp[0] == '0') out[0] = 0;
        }
        outlen = (int32_t)strn_len(out, (size_t)-1);
        break;
    case 0x75:                                      /* u (unsigned decimal) */
        itoa32_tbl(val, (char *)out, 10);
        if ((int32_t)*pglen == 0 && out[0] == '0') out[0] = 0;
        outlen = (int32_t)strn_len(out, (size_t)-1);
        break;
    default:                                        /* all other type codes */
        out[0] = type; out[1] = 0;
        *pglen = 1; *psize = 0;
        outlen = 1; has_prefix = 0;
        st[0x14] &= 0xf9;
        break;
    }

    /* --- epilogue (0x4556a) --- */
    if (has_prefix) {
        if (ret[0] == '-') {
            ret++;
            st[0x18] = 0;
            outlen--;
            st[0x17] = '-';
        } else {
            if (st[0x14] & 0x02) st[0x17] = ' ';
            if (st[0x14] & 0x04) st[0x17] = '+';
            st[0x18] = 0;
        }
    }
    if (outlen > (int32_t)*pglen) *pglen = outlen;
    else *plen = (int32_t)*pglen - outlen;
    if (st[0x16] == '*') { st[0x17] = 0; st[0x14] &= 0xf9; }
    if ((int32_t)*pglen != -1 && outlen < (int32_t)*pglen && type != 0x63) *pglen = outlen;
    {
        int32_t prefix_len = (int32_t)strn_len(st + 0x17, (size_t)-1);
        *psize -= (prefix_len + (int32_t)*pglen + (int32_t)*plen);
    }
    return ret;
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

void delay_units(uint32_t arg)   /* 0x4564f (also thunk 0x042958) */
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

/* 0x041c28 (L1, 9 bytes): NULL-tolerant free. TEST EAX,EAX; JNZ heap_free_host
 * (0x41c31); RET. Tail-call: frees `data` only when nonzero, else returns. */
void heap_free_guard(uint32_t data) { if (data) heap_free_host(data); }

/* 0x045605: upper-case a NUL-terminated string in place. Faithful to the
 * disasm: for each byte until NUL, EAX = zero-extended byte, EAX = to_upper(EAX),
 * store low byte back, advance. Uses the already-ported to_upper (0x494bd). */
void string_upper(char *s) {
    while (*s != '\0') {
        *s = (char)to_upper((uint8_t)(unsigned char)*s);
        s++;
    }
}

/* 0x0427f3 (L1, 104 bytes): build a temp filename from the tick counter and
 * a small index. Register ABI: EAX = output buffer (>=13 bytes),
 * EDX = index (low 8 bits used). counter = get_dat_53390() (0x44be6).
 *
 * out[0]='t'; out[1..4]=4 lowercase hex digits of counter&0xffff (big-endian:
 *   out[1]=bits12-15 ... out[4]=bits0-3); out[5]='_'; out[6..7]=2 hex digits
 *   of index&0xff; out[8]='.'; out[9..11]="tmp"; out[12]=0.
 *   e.g. counter=0x0000a1b2, index=0x5a -> "ta1b2_5a.tmp"
 * Nibble->char via digit_chr (0x427e7, ported). The high 16 bits of the
 * 32-bit counter and the high 24 bits of index are discarded.
 */
void tmp_filename(char *out, uint32_t index) {
    uint32_t c = (uint32_t)get_dat_53390();   // 0x44be6
    out[0] = 't';
    for (int pos = 4; pos >= 1; pos--) {      // right-to-left nibbles (out[4..1])
        out[pos] = (char)digit_chr(c & 0xf);
        c >>= 4;
    }
    out[5] = '_';
    out[6] = (char)digit_chr((index >> 4) & 0xf);
    out[7] = (char)digit_chr(index & 0xf);
    out[8] = '.';
    out[9]  = 't';
    out[10] = 'm';
    out[11] = 'p';
    out[12] = 0;
}

/* 0x04a24d (L1, 89 bytes): config key=value table lookup.
 * g_dat_537d4 (DAT_000537d4) is a NUL-terminated array of "key=value" char*.
 * Register ABI: EAX = key string; EDX is a preserved scratch register (returned
 * in the high word, unused). Returns a pointer to the char immediately after the
 * first '=' of the matching entry, or 0 if none. A match requires BOTH:
 *   strn_casecmp(entry, key, strlen(key)) == 0   (case-insensitive prefix)
 *   AND entry[strlen(key)] == '='                ('=' right after the key)
 * Length is computed exactly as the original (SCASB over the key). strn_casecmp
 * is 0x4a7a2 (ported, batch I).
 */
char *config_value_ptr(const char *key) {
    char **table = g_dat_537d4;
    if (table == 0 || key == 0) return 0;
    uint32_t len = (uint32_t)strlen(key);
    for (char **e = table; *e != 0; e++) {
        if (strn_casecmp((const uint8_t *)(*e), (const uint8_t *)key, len) == 0
            && (*e)[len] == '=') {
            return *e + len + 1;   /* pointer just past '=' */
        }
    }
    return 0;
}

/* 0x04457f: hex-digit value. Faithful to the disasm: if '0'-'9' return c-'0';
 * else to_lower(c) (0x44859, ported), if 'a'-'f' return c-0x57 (10-15); else 0x10.
 * The 0x445a0 tail (MOV EAX,0x10;RET) is the shared "not a digit" return. */
int hex_digit_value(int c) {
    if (c >= 0x30 && c <= 0x39) return c - 0x30;
    c = to_lower(c);
    if (c >= 0x61 && c <= 0x66) return c - 0x57;
    return 0x10;
}

/* 0x041e9f: LCG PRNG step. Faithful to the disasm: EAX = rng_state_ptr();
 * if (!EAX) return 0; EDX = *EAX * 0x41c64e6d + 0x3039 (IMUL low32, mod 2^32);
 * *EAX = EDX; return (EDX >> 16) & 0x7fff. The NULL guard is dead in the port
 * (rng_state_ptr always returns &g_rng_state) but kept for fidelity. */
uint32_t prng_next(void) {
    uint32_t *s = rng_state_ptr();
    if (s == (uint32_t *)0) return 0;
    uint32_t v = *s * 0x41c64e6d + 0x3039;
    *s = v;
    return (v >> 16) & 0x7fff;
}

/* 0x041ec3: seed the PRNG. Faithful: EDX = seed (EAX in); EAX = rng_state_ptr();
 * if (EAX) *EAX = EDX. */
void prng_seed(uint32_t seed) {
    uint32_t *s = rng_state_ptr();
    if (s != (uint32_t *)0) *s = seed;
}

/* 0x041f42: signed itoa. Faithful: EAX=value(signed), EDX=buf, EBX=base.
 * if (base==10 && value<0) { value=-value; buf[0]='-'; buf++; } itoa32(value,buf,base);
 * returns the ORIGINAL buf start (ECX held it across the itoa32 call). */
char *itoa32s(int32_t value, char *buf, uint32_t base) {
    char *p = buf;
    uint32_t u = (uint32_t)value;      /* NEG EAX wraps mod 2^32 (INT_MIN -> INT_MIN) */
    if (base == 10 && value < 0) { u = -u; *buf = '-'; buf++; }
    itoa32(u, buf, base);
    return p;
}

/* 0x0494a2: identical to itoa32s but calls itoa32_tbl (0x49459, delegates to itoa32). */
char *itoa32s_tbl(int32_t value, char *buf, uint32_t base) {
    char *p = buf;
    uint32_t u = (uint32_t)value;
    if (base == 10 && value < 0) { u = -u; *buf = '-'; buf++; }
    itoa32_tbl(u, buf, base);
    return p;
}
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

/* IEEE-754 float32 -> fxnum.  orig: 0x04811a
 * top = (int32)v>>23;  byte = top & 0xff (the IEEE exponent field, bits 23..30).
 *   byte==0    -> { lo=0, hi=v,        exp=0 }      (zero/subnormal passthrough)
 *   byte==0xff -> { lo=0, hi=<below>,  exp=0xffff } (NaN/Inf marker)
 *   else       -> { lo=0, hi=<below>,  exp=byte+0x3f80 }   (= unbiassed + 0x3fff)
 * hi = (v<<8 & 0x7fffffff) | ((v<<8 & 0x7fffffff)!=0 ? 0x40000000 : 0) | 0x80000000.
 * The trailing `ADD ECX,ECX; RCR CX,1` nets to identity only when CF=0 (pre-double
 * value < 0x80000000); for inputs with bit31 set CF=1 flips exp's top bit, so the
 * carry is computed explicitly (mirrors the hardware). */
void fx_from_f32(struct fxnum *out, uint32_t v)   /* 0x4811a */
{
    uint32_t eax = v;
    uint32_t edx = 0;                              /* SUB EDX,EDX */
    uint32_t ecx = (uint32_t)((int32_t)v >> 23);   /* SAR ECX,0x17 */
    ecx = (ecx & 0xffff0000u) | (ecx & 0xffu);     /* AND CX,0xff (CH:=0) */
    if ((ecx & 0xffffu) == 0) {                    /* JZ 0x48153 */
        out->lo = edx;                              /* 0 */
        out->hi = eax;                              /* v (unchanged) */
        out->exp = (uint16_t)ecx;                   /* 0 */
        return;
    }
    eax = v << 8;                                   /* SHL EAX,8 */
    uint8_t cl = (uint8_t)ecx;                      /* byte = (v>>23)&0xff */
    if (cl == 0xff) {
        ecx = (ecx & 0xffff0000u) | 0xffffu;        /* MOV CH,0xff */
    } else {
        ecx = (ecx & 0xffff0000u) | ((uint16_t)ecx + 0x3f80u); /* ADD CX,0x3f80 */
    }
    eax &= 0x7fffffffu;                             /* AND EAX,0x7fffffff */
    if (eax != 0) eax |= 0x40000000u;               /* JZ skip; OR EAX,0x40000000 */
    uint32_t cf = (ecx >> 31) & 1u;                 /* carry of ADD ECX,ECX */
    ecx = ecx + ecx;                                 /* ADD ECX,ECX */
    uint16_t cx = (uint16_t)ecx;
    cx = (uint16_t)(((cf) << 15) | (cx >> 1));      /* RCR CX,1 */
    ecx = (ecx & 0xffff0000u) | cx;
    eax |= 0x80000000u;                              /* OR EAX,0x80000000 */
    out->lo = edx;                                   /* 0 */
    out->hi = eax;
    out->exp = (uint16_t)ecx;
}

/* ---- 0x45d56: in-place scientific-notation float-string cleanup ----
 * param_1 = char *s (the float string, e.g. "123.450e6", modified in place);
 * param_2 = a record: byte[0x14] = skip-flag (bit0), byte[0x15] = format char.
 * If rec[0x14] & 1: do nothing. Otherwise:
 *   advance to 'e'/'E' (or NUL); step back one; if rec[0x15] is 'g'/'G' strip
 *   trailing '0's; if the stepped-back char is '.' overwrite it (drop the dot);
 *   copy from the 'e' position leftward to the stepped-back position, NUL
 *   terminating — removes the stripped zeros and/or the adjacent dot. */
void float_str_sci(char *s, const uint8_t *rec)   /* 0x45d56 */
{
    if (rec[0x14] & 1) return;                        /* TEST [EDX+0x14],1; JNZ */
    char *e = s;
    while (*e && *e != 'e' && *e != 'E') e++;         /* find 'e'/'E' or end */
    char *src = e;                                    /* EDX = EAX */
    char *dst = e - 1;                                /* DEC EAX */
    if (rec[0x15] == 'g' || rec[0x15] == 'G')         /* CL==0x67/0x47 */
        while (*dst == '0') dst--;                    /* strip trailing '0's */
    if (*dst != '.') dst++;                           /* CMP [EAX],0x2e; JZ skip INC */
    for (;;) {                                        /* copy until NUL */
        char c = *src++;
        *dst++ = c;
        if (c == '\0') break;
    }
}

/* ---- 0x4a8b9: 64-bit unsigned -> fixed-width right-justified ASCII ----
 * param_1 = const uint64_t *val (lo=[0], hi=[4]); param_2 = char *buf; param_3
 * = int width (digit slots; also the field offset within buf). Writes
 * buf[0..width-1] (right-justified, leading-zero padded) and a NUL at
 * buf[width]. Repeatedly extracts val%10000 (a 4-digit group) — the original
 * splits this into two 32-bit DIVs (hi/10000, then (hi%10000):lo/10000), which
 * is exactly val/10000 and val%10000 — converts the group to 4 ASCII digits
 * (via AAM in the original), and shifts left, padding '0' groups until width
 * slots are filled. */
void fmt_u64_fixed(const uint64_t *val, char *buf, int width)   /* 0x4a8b9 */
{
    uint64_t v = *val;
    int pos = width;                        /* EBP offset (EBP = buf + pos) */
    int cnt = width;                        /* EBX (digit counter) */
    buf[pos] = '\0';                         /* MOV [EBP],0 sentinel */
    for (;;) {
        uint32_t g = (uint32_t)(v % 10000u); /* 4-digit group (V%10000) */
        char d0 = '0' + (char)(g % 10u);          /* ones    */
        char d1 = '0' + (char)((g / 10u) % 10u);  /* tens    */
        char d2 = '0' + (char)((g / 100u) % 10u); /* hundreds*/
        char d3 = '0' + (char)((g / 1000u) % 10u);/* thousands*/
        v /= 10000u;                           /* V /= 10000 */
        pos -= 4;                              /* SUB EBP,4 */
        buf[pos + 3] = d0; if (--cnt == 0) break;   /* [EBP+3]=AL */
        buf[pos + 2] = d1; if (--cnt == 0) break;   /* [EBP+2]=AH */
        buf[pos + 1] = d2; if (--cnt == 0) break;   /* [EBP+1]=DL */
        buf[pos]     = d3; if (--cnt != 0) continue; else break; /* [EBP]=DH */
    }
}

/* ---- 0x4a84e: fixed-point mantissa scaler/normalizer ----
 * in = 64-bit value [lo,hi]; hi's top 12 bits (bits 20-31) are a biased
 * exponent (bias 0x433); the mantissa is hi's low 20 bits (bit 20 forced to 1,
 * the implicit leading 1) + lo (53 bits total). Shifts the 53-bit mantissa
 * left (exp>0x433) or right (exp<0x433) by exp-0x433, round-to-nearest-even on
 * right-shift, and stores the 64-bit result to out. The original uses a per-bit
 * SHR/RCR loop with EAX/ESI tracking the shifted-out bits for the RNE decision
 * (top discarded bit = EAX bit31, lower discarded = EAX low bits / ESI). */
void fx_mantissa_scale(uint32_t *out, const uint32_t *in)   /* 0x4a84e */
{
    uint32_t hi = in[1];
    uint32_t lo = in[0];
    uint32_t mant_hi = (hi & 0x000fffff) | 0x100000;   /* bit 20 = implicit 1 */
    int32_t shift = ((hi >> 20) & 0xfff) - 0x433;
    uint64_t v = ((uint64_t)mant_hi << 32) | lo;
    uint64_t r;
    if (shift == 0) {
        r = v;
    } else if (shift > 0) {
        r = (shift >= 64) ? 0 : (v << shift);
    } else {
        int32_t sa = -shift;
        if (sa >= 64) {
            r = 0;
        } else {
            uint64_t q = v >> sa;
            int rnd    = (int)((v >> (sa - 1)) & 1);
            int sticky = (v & (((uint64_t)1 << (sa - 1)) - 1)) != 0;  /* bits 0..sa-2 */
            if (rnd && sticky)          r = q + 1;  /* round up */
            else if (rnd && (q & 1))    r = q + 1;  /* tie -> even */
            else                        r = q;      /* truncate */
        }
    }
    out[0] = (uint32_t)r;
    out[1] = (uint32_t)(r >> 32);
}

/* ---- 0x49132: 64-bit -> IEEE-754 float32 converter ----
 * Input 64-bit value in EDX:EAX (hi,lo). If hi's top 12 bits (0x7ff00000) are
 * 0 -> underflow -> 0. Else: shift the 64-bit value left 1 (capturing the top
 * bit into EBX as the f32 sign 0x80000000), add the 0x20000000 mantissa bias,
 * then clamp/normalize: hi==0 or hi>=0x8fe00000 -> 0x7f800000|sign (max);
 * hi<0x70200000 -> 0; else hi-=0x70000000, shift left 2, OR the sign, return
 * the high word as the float32. Ported as an exact register-op replication. */
uint32_t fx64_to_f32(uint32_t lo, uint32_t hi)   /* 0x49132 */
{
    uint64_t v = ((uint64_t)hi << 32) | lo;
    if ((hi & 0x7ff00000u) == 0) return 0;        /* TEST EDX,0x7ff00000; JZ */
    uint32_t ebx = ((v >> 63) & 1) ? 0x80000000u : 0u;  /* RCR EBX,1 (top bit) */
    v = (v << 1);                                   /* ADD EAX,EAX; ADC EDX,EDX */
    v += 0x20000000ull;                             /* ADD EAX,0x20000000; ADC EDX,0 */
    uint32_t h = (uint32_t)(v >> 32);
    if (h == 0)                  return 0x7f800000u | ebx;  /* JZ 0x49171 */
    if (h >= 0x8fe00000u)        return 0x7f800000u | ebx;  /* CMP;JNC 0x49171 */
    if (h < 0x70200000u)         return 0;         /* CMP;JC 0x4917a */
    v = (v & 0xffffffffull) | ((uint64_t)(h - 0x70000000u) << 32); /* SUB EDX,0x70000000 */
    v = (v << 2);                                   /* ADD EAX,EAX;ADC x2 */
    h = (uint32_t)(v >> 32);
    h |= ebx;                                       /* OR EDX,EBX */
    return h;                                       /* MOV EAX,EDX */
}

/* ---- 0x49db2: decimal number formatter (integer '.' fractional) ----
 * out=EAX (advances, returned as new position), intlen=EBX (signed),
 * fractlen=ECX (signed), src=EDX (digit chars, advances). If fractlen==0 &&
 * intlen<=0 -> write "0.". Else write intlen digit chars (from src until NUL,
 * then '0' pad), then '.', then the fractional part via a shared ESI/ECX
 * control flow: loop A (while fractlen!=0 && esi!=0: esi++;'0';fractlen--) -
 * which fires only for the negative-intlen edge case - then loop B (copy
 * fractlen chars from src until NUL) then '0' padding. The original shares its
 * exit with 0x49db0 (POP ESI;RET, the tail of the previous function). */
uint8_t *fmt_decimal(uint8_t *out, int32_t intlen, int32_t fractlen,
                     const uint8_t *src)   /* 0x49db2 */
{
    int32_t esi = intlen;
    uint8_t *o = out;
    const uint8_t *s = src;
    if (fractlen != 0 || intlen > 0) {
        while (esi > 0 && *s != 0) { *o++ = *s++; esi--; }   /* part1 copy */
        while (esi > 0)                { esi--;  *o++ = '0'; } /* part1 pad */
        *o++ = '.';                                            /* decimal pt */
        while (fractlen != 0 && esi != 0) { esi++; *o++ = '0'; fractlen--; } /* loop A */
        if (fractlen <= 0) return o;                           /* 0x49df9 */
        while (fractlen != 0 && *s != 0) { *o++ = *s++; fractlen--; }        /* loop B */
        if (fractlen == 0) return o;                           /* 0x49e0a */
        while (fractlen != 0) { fractlen--; *o++ = '0'; }      /* loop B pad */
        return o;                                              /* 0x49e0c */
    }
    *o++ = '0';
    *o++ = '.';
    return o;
}

/* ---- 0x49d0b: fixed-width signed integer formatter (integer part) ----
 * out=EAX, width=EBX (0 -> default 2, or 3 for 3-digit magnitudes), maxw=ECX
 * (a cutoff: if width+1>maxw or digits>width, write nothing and return the
 * count), val=EDX (signed). Writes the sign ('+'/'-'), leading '0' padding to
 * reach `width`, then the absolute value as 1/2/3 digits (base 100/10/1 chosen
 * by magnitude, IDIV extraction). Returns the field count (width+1 or
 * digits+1). Sibling of fmt_decimal (0x49db2); both share the 0x49db0 epilogue. */
int32_t fmt_int_signed(uint8_t *out, int32_t width, int32_t maxw, int32_t val)
                                                       /* 0x49d0b */
{
    uint8_t sign = (val < 0) ? '-' : '+';
    int32_t esi  = (val < 0) ? -val : val;   /* abs (NEG wraps on INT_MIN) */
    int32_t base, digits;
    if      (esi >= 100) { base = 100; digits = 3; }
    else if (esi >= 10)  { base = 10;  digits = 2; }
    else                 { base = 1;   digits = 1; }
    int32_t eax = width;
    if (eax == 0) { eax = 2; if (digits == 3) eax = 3; }
    int32_t ebp = eax + 1;                          /* LEA EBP,[EAX+1] */
    if (ebp > maxw)      return ebp;                /* CMP EBP,ECX; JG */
    if (digits > eax)    { ebp = digits + 1; return ebp; }  /* 0x49da6 */
    uint8_t *o = out;
    *o++ = sign;                                    /* 0x49d66 */
    while (eax > digits) { eax--; *o++ = '0'; }     /* leading '0' pad */
    while (1) {
        int32_t q = esi / base;                     /* IDIV EBX */
        *o++ = (uint8_t)(q + '0');
        esi = esi % base;
        int32_t nb = base / 10;                     /* EBX = EBX/10 */
        if (nb == 0) break;                         /* TEST EAX; JNZ */
        base = nb;
    }
    return ebp;                                     /* 0x49da9 */
}

/* ---- 0x47ea4 (+thunks 0x47e4a mode 0xa0, 0x47e9f mode 0x1f): fixed-point
 * exp-based mantissa extractor ----
 * p -> { u32 lo; u32 hi; u16 exp; } (exp at [8], byte [9] = hi byte of exp).
 * cl = (exp & 0x7fff) - 0x3ffe (bias 0x3ffe). cl<0 -> 0; cl>0x20 -> 0x80000000;
 * cl > (mode & 0x3f) -> 0x80000000. Else the top `cl` bits of hi are extracted
 * (or all of hi when cl==0x20), and a mode/exp-driven bit chain
 * (RCR CH,1 / SHLD / ADD / ADC / NEG) sets the sign and a +1 carry. lo only
 * affects the result through bit31(lo) -> RCH CH,1. Faithful instruction-by-
 * instruction replication with explicit CF tracking. */
int32_t fx_extract(const uint8_t *p, int32_t mode)   /* 0x47ea4 */
{
    uint32_t lo  = *(const uint32_t*)(p + 0);
    uint32_t hi  = *(const uint32_t*)(p + 4);
    uint16_t exp = *(const uint16_t*)(p + 8);

    uint8_t bl = (uint8_t)mode;
    uint8_t bh = (uint8_t)(exp >> 8);      /* MOV BH,[p+9] */
    bh >>= 1;                              /* SHR BH,1 */
    bh |= bl;                              /* OR BH,BL */

    uint16_t cx = exp & 0x7fff;            /* AND CX,0x7fff */
    int16_t  sx = (int16_t)(cx - 0x3ffe);  /* SUB CX,0x3ffe */
    if (sx < 0)      return 0;             /* JL 0x47f09 */
    if (sx > 0x20)   return 0x80000000;    /* JG 0x47f01 */

    uint8_t blm = (uint8_t)(mode & 0x3f);  /* AND BL,0x3f */
    uint8_t cl  = (uint8_t)(cx - 0x3ffe);  /* CL */
    if (cl > blm)  return 0x80000000;      /* CMP CL,BL; JG */

    uint8_t  ch  = bh;                     /* MOV CH,BH */
    uint32_t ebx = lo;                     /* MOV EBX,[p] */
    uint32_t eax = hi;                     /* MOV EAX,[p+4] */
    uint32_t cf  = (lo >> 31) & 1;         /* ADD EBX,EBX */
    ebx = lo << 1;
    uint8_t ch_r = (uint8_t)((ch << 1) | cf); /* RCR CH,1 */
    cf = (ch >> 7) & 1;
    ch = ch_r;

    if (cl == 0x20) {                      /* CL==0x20 path */
        cf = (ch >> 7) & 1;                /* ADD CH,CH */
        ch = (uint8_t)(ch << 1);
        /* eax stays hi */
    } else {                               /* CL!=0x20 (SHLD) path */
        ebx = 0;                           /* SUB EBX,EBX */
        cf = 0;                            /* SHLD cf = bit(32-cl) of 0 */
        uint32_t eax0 = eax;
        if (cl > 0) {
            ebx = eax0 >> (32 - cl);       /* SHLD EBX,EAX,cl */
            eax  = eax0 << cl;             /* SHL EAX,cl */
            cf   = (eax0 >> (32 - cl)) & 1;/* SHL cf */
        }
        cf = (ch >> 7) & 1;                /* ADD CH,CH */
        ch = (uint8_t)(ch << 1);
        cf = (eax >> 31) & 1;              /* ADD EAX,EAX */
        eax <<= 1;
        eax = ebx;                         /* MOV EAX,EBX */
    }
    /* tail 0x47eed */
    uint8_t cl2 = (uint8_t)(0xfe | cf);    /* MOV CL,0xff; RCR CL,1 */
    ch = (uint8_t)(ch & cl2);              /* AND CH,CL */
    uint32_t cf1 = (ch >> 7) & 1;          /* ADD CH,CH */
    ch = (uint8_t)(ch << 1);
    eax += cf1;                            /* ADC EAX,0 */
    uint32_t cf2 = (ch >> 7) & 1;          /* ADD CH,CH */
    if (cf2) eax = (uint32_t)(-(int32_t)eax); /* NEG EAX (JNC guard) */
    return (int32_t)eax;
}

/* ---- 0x47fd4: fixed-point mantissa normalizer ----
 * p -> { u32 lo; u32 hi; u16 exp; }. M = (hi + 0x100) & 0xffffff00 (or
 * 0x80000000 if hi+0x100 overflows, in which case exp is +1). If the (hi==0x80,
 * lo==0) underflow edge hits, the mask is 0. cx = (exp + carry) & 0x7fff:
 *   cx==0 -> M; cx==0x7fff -> ((M<<1)>>8 | 0xff000000) << 1;
 *   else cx2=(cx+0xc080)&0xffff: cx2>=0x8000 -> 0; cx2>=0xff -> 0xff000000<<1;
 *   else (cx2<0xff) -> ((M<<1)<<8 | (hi<<25)>>24) << 1.
 * (EBX=hi<<25 so BX==0, hence every RCR EAX,1 is a plain <<1.) Instruction-
 * faithful with explicit CF. */
int32_t fx_norm32(const uint8_t *p)   /* 0x47fd4 */
{
    uint32_t lo  = *(const uint32_t*)(p + 0);
    uint32_t hi  = *(const uint32_t*)(p + 4);
    uint16_t exp = *(const uint16_t*)(p + 8);

    uint32_t edx = 0xffffff00;
    uint32_t ecx = hi << 0x19;           /* SHL ECX,0x19 */
    uint32_t cf  = (hi >> 7) & 1;        /* CF = bit7(hi) */
    uint16_t cx  = exp;
    if (cf && ecx == 0 && lo == 0) edx = 0;  /* ADD EDX,EDX (underflow edge) */

    uint32_t eax = hi + 0x100;           /* ADD EAX,0x100 */
    if (eax < 0x100) { eax = 0x80000000; cx++; }  /* JNC / MOV / INC */
    eax &= edx;                          /* AND EAX,EDX */
    cx &= 0x7fff;                        /* AND CX,0x7fff */

    if (cx == 0) return (int32_t)eax;    /* JZ 0x4804f */
    if (cx == 0x7fff) {                  /* 0x48015 special */
        eax = (eax << 1) >> 8;           /* ADD EAX,EAX; SHR EAX,8 */
        eax |= 0xff000000;               /* OR EAX,0xff000000 */
        eax = (eax << 1);                /* RCR EAX,1 (CF=0) */
        return (int32_t)eax;
    }
    uint16_t cx2 = (uint16_t)(cx + 0xc080); /* ADD CX,0xc080 */
    if (cx2 & 0x8000) return 0;          /* JNS fail -> SUB EAX,EAX */
    if (cx2 >= 0xff) {                   /* CMP CX,0xff; JL fail */
        eax = 0xff000000;
        eax = (eax << 1);                /* RCR EAX,1 (CF=0) */
        return (int32_t)eax;
    }
    /* 0x48044: cx2 < 0xff */
    eax = (eax << 1);                    /* ADD EAX,EAX */
    eax = (eax << 8) | (ecx >> 24);      /* SHRD EAX,ECX,8 */
    eax = (eax << 1);                    /* RCR EAX,1 (CF=0) */
    return (int32_t)eax;
}

/* ---- 0x4a9da: IEEE-754 double -> 32-bit fixed-point exp ----
 * *out = EDX. hi = bits 32-63 of the double. If the double is exactly +/-0
 * (hi & 0x7fffffff == 0 && lo == 0) then *out = 0; otherwise
 * *out = (int32)(((hi >> 20) & 0xff) - 0x3fe)  (the low 8 bits of the biased
 * exponent, minus bias 0x3fe). The original also rewrites [ESP+6] and does
 * FLD double, but the FPU value is never stored so it does not affect EDX;
 * those are omitted as no-ops. */
void fx_double_exp(double d, uint32_t *out)   /* 0x4a9da */
{
    union { double dd; uint64_t u; } cv; cv.dd = d;
    uint32_t lo = (uint32_t)cv.u;
    uint32_t hi = (uint32_t)(cv.u >> 32);
    uint32_t edx = 0;
    if ((hi & 0x7fffffff) != 0 || lo != 0) {      /* JNZ / TEST lo / JZ */
        edx = (uint32_t)((int32_t)((hi >> 20) & 0xff) - 0x3fe);
    }
    *out = edx;
}

/* ---- 0x45687: DOS date/time packer ----
 * Reads the DOS clock (INT 21h 2Ch get date, 2Ah get time) via sys_getdate /
 * sys_gettime and packs it. local0 = (month<<24)|(day<<16); local1 =
 * (hour<<24)|(low8(minute*256+second-0x76c)<<16). The SHL ECX,16 zeroes the low
 * byte of each local, so ESI (built from local0's low byte) is always 0 and the
 * "day-change" branch (CMP local1.byte0 vs local2.byte0, both 0) is always
 * equal -> dead. Hence this always returns 0 and writes the struct below. */
uint32_t datetime_pack(struct datetime_pack *out)   /* 0x45687 */
{
    struct dos_date d = sys_getdate();
    struct dos_time t = sys_gettime();
    uint16_t timeval = (uint16_t)((uint16_t)t.minute * 256 + t.second - 0x76c);
    out->f0  = 0;                    /* local0 byte1 = 0 */
    out->f4  = d.day;                /* local0 byte2 = day */
    out->f8  = d.month;              /* local0 byte3 = month */
    out->fc  = 0;                    /* local1 byte0 = 0 */
    out->f10 = (uint32_t)(0 - 1);    /* local1 byte1 (0) - 1 */
    out->f14 = (uint8_t)timeval;     /* local1 byte2 = low8(timeval) */
    out->f20 = 0xffffffff;           /* constant */
    return 0;
}

/* ---- 0x4a2a7: double NaN/Inf inline-string writer ----
 * If the double's low 8 exponent bits are all 1s ((hi>>20)&0xff==0xff), writes an
 * inline string into out: "inf" (mantissa 0) or "nan" (mantissa nonzero), with a
 * leading '-' if the sign bit is set, and returns 1; else returns 0 leaving out
 * untouched. The check tests only 8 of the 11 exponent bits (latent original bug):
 * e.g. 1.0 (exp 0x3ff, mantissa 0) is classified "inf". */
int fx_double_naninf(double d, char *out)   /* 0x4a2a7 */
{
    union { double dd; uint64_t u; } cv; cv.dd = d;
    uint32_t lo = (uint32_t)cv.u;
    uint32_t hi = (uint32_t)(cv.u >> 32);
    if (((hi >> 20) & 0xff) != 0xff) return 0;
    uint32_t mantissa = lo | (hi & 0xfffff);
    const char *s = (mantissa != 0) ? "nan" : "inf";   /* 0x51848 : 0x51844 */
    if (hi & 0x80000000) { out[0] = '-'; memcpy(out + 1, s, 4); }
    else { memcpy(out, s, 4); }
    return 1;
}

/* ---- 0x47ab6: fixed-point lexicographic compare ----
 * a/b point to {u32 lo, u32 hi, u16 exp} (exp at [8]). A value with exp==0x7fff
 * and nonzero (hi<<1)|lo is "special" and the compare returns 2. Otherwise it
 * is a lexicographic UNSIGNED compare of (exp, hi, lo): the first differing
 * field sets the sign (CF = a<b unsigned). Returns -1 (a<b), 0 (a==b), +1 (a>b).
 * The original's RCR EAX/XOR ESI/ADD ESI at 0x47b10-0x47b14 is dead (EAX is
 * overwritten by EDI at 0x47b1c). */
int fx_compare(const uint8_t *a, const uint8_t *b)   /* 0x47ab6 */
{
    uint32_t alo = *(const uint32_t *)a;
    uint32_t ahi = *(const uint32_t *)(a + 4);
    uint16_t aexp = *(const uint16_t *)(a + 8);
    uint32_t blo = *(const uint32_t *)b;
    uint32_t bhi = *(const uint32_t *)(b + 4);
    uint16_t bexp = *(const uint16_t *)(b + 8);
    if (aexp == 0x7fff && (uint32_t)((ahi << 1) | alo) != 0) return 2;
    if (bexp == 0x7fff && (uint32_t)((bhi << 1) | blo) != 0) return 2;
    if (aexp != bexp) return (aexp < bexp) ? -1 : 1;
    if (ahi != bhi)   return (ahi < bhi) ? -1 : 1;
    if (alo != blo)   return (alo < blo) ? -1 : 1;
    return 0;
}

/* ---- 0x48666: fixed-point value classifier ----
 * Register ABI (original): EAX = fx value ptr {u32 lo@0, u32 hi@4, u16 exp@8},
 * EBP = context struct ptr (status word at [EBP+4], flags word at [EBP+8]),
 * EDI = class byte-offset (0/2/4) selecting masks from the 8-word table at
 * 0x45e1c. If (mask22 & flags) == mask22 the value is already flagged: status=
 * 0x41. Otherwise mask22 is cleared from flags and the value is classified by
 * its exponent magnitude mag = exp & 0x7fff:
 *   mag==0     : (hi|lo)!=0 -> 0x44, else flags|=mask1c, 0x40
 *   mag==0x7fff: flags|=mask20; (lo==0 && hi==0x80000000) ? 0x05 : 0x01
 *   else       : (hi&0x80000000) ? 0x04 : (flags|=mask20, 0x44)
 * The high byte of the status word is then set to (code | sign) where sign is
 * 2 if (short)exp<0; low byte and bits 9,10 (0x600) of status are preserved per
 * the 0xb8ff mask. */
static const uint16_t g_fxmask_tbl[8] =
    {0x0001,0x0000,0x0002,0x0003,0xfff3,0x0004,0x0000,0x0008};   /* 0x45e1c */
void fx_classify_status(const uint8_t *fx, uint8_t *ctx, uint32_t edi)   /* 0x48666 */
{
    uint16_t *flags = (uint16_t *)(ctx + 8);
    uint16_t *status = (uint16_t *)(ctx + 4);
    uint16_t mask1c = g_fxmask_tbl[edi / 2 + 0];   /* 0x45e1c + EDI */
    uint16_t mask20 = g_fxmask_tbl[edi / 2 + 2];   /* 0x45e20 + EDI */
    uint16_t mask22 = g_fxmask_tbl[edi / 2 + 3];   /* 0x45e22 + EDI */
    uint32_t lo = *(const uint32_t *)fx;
    uint32_t hi = *(const uint32_t *)(fx + 4);
    uint16_t exp = *(const uint16_t *)(fx + 8);
    uint8_t al;
    uint8_t dh = (exp & 0x8000) ? 0x02 : 0x00;     /* (short)exp<0 -> 2 */
    if ((mask22 & *flags) == mask22) { al = 0x41; goto store; }
    *flags &= (uint16_t)~mask22;
    uint16_t mag = exp & 0x7fff;
    if (mag == 0) {
        if ((hi | lo) != 0) { al = 0x44; goto store; }
        *flags |= mask1c;
        al = 0x40;
        goto store;
    }
    if (mag == 0x7fff) {
        *flags |= mask20;
        al = 0x01;
        if (lo == 0 && hi == 0x80000000u) al = 0x05;
        goto store;
    }
    if (hi & 0x80000000u) { al = 0x04; goto store; }
    *flags |= mask20;
    al = 0x44;
store:
    *status = (*status & 0xb8ff) | (uint16_t)(((uint8_t)al | dh) << 8);
}

/* ---- 0x47d4e: fixed-point 64x64 -> normalized 64-bit multiply ----
 * Register ABI: A = EDX:EAX (a_hi:a_lo), B = ECX:EBX (b_hi:b_lo), ESI =
 * (exp_hi:16 | exp_lo:16). Returns the normalized 64-bit product (EDX:EAX);
 * 0x8000000000000000 (fx infinity) on overflow.
 *
 * BUG FIX: the earlier literal Ghidra-decompile rewrite was WRONG whenever both
 * operands had nonzero high 32-bits: its 128-bit schoolbook assembly dropped the
 * carry of the middle-word add, corrupting the result high word. E.g.
 * fx_mul64(0x12345678,0x9abcdef0,0x12345678,0x9abcdef0,0x1) returned 0xb00ea4f0
 * instead of the correct 0 (true 128-bit product 0x14b66dc1df4d840 normalizes to
 * 0 via the exponent). Its own test only covered hi==0 inputs plus a wrong
 * expected value (1 instead of 2 for (0,1)*(0,1) @ 0x3fff).
 * We now delegate to the instruction-level emulator fn_047d4e, differentially
 * verified against the original assembly (scratch/fp_047d4e_ref.py): (lo,hi) agree
 * on a 500-input sweep. */
uint64_t fx_mul64(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi,
                  uint32_t esi)                                                /* 0x47d4e */
{
    fx64_t o;
    fn_047d4e(a_lo, a_hi, b_lo, b_hi, esi, &o);
    return ((uint64_t)o.hi << 32) | o.lo;
}

/* ---- 0x4a923: no-op stub ----
 * Disassembly-verified no-op. Body is a single `CALL 0x4a9d8` followed by
 * zero-filler (0x0000 pairs). 0x4a9d8 is not a Ghidra function; its two bytes
 * are `POP EDI; RET`. So calling 0x4a923: (1) CALL pushes 0x4a928 and jumps
 * to 0x4a9d8, (2) POP EDI loads 0x4a928 into EDI (clobber), (3) RET pops the
 * caller's return address and jumps back to 0x4a923's caller. Net effect:
 * return void to the caller with EDI clobbered to 0x4a928 and no return
 * value; any stack args pushed by the caller are left in place (caller-cleans).
 * No observable state change -> empty function. Called from 0x4a814. */
void fx_stub_4a923(void)                                                /* 0x4a923 */
{
}

/* 0x4815f: 32-entry mask table for fx_round_down (0x481df). Each entry clears
 * the top (i+1) bits of a 32-bit mantissa word (0x7fffffff ... 0x00000000). */
static const uint32_t g_fx_round_tbl[32] = {
    0x7fffffffu, 0x3fffffffu, 0x1fffffffu, 0x0fffffffu,
    0x07ffffffu, 0x03ffffffu, 0x01ffffffu, 0x00ffffffu,
    0x00ffff7fu, 0x00ffff3fu, 0x00ffff1fu, 0x00ffff0fu,
    0x00ffff07u, 0x00ffff03u, 0x00ffff01u, 0x00ffff00u,
    0x00ff7f00u, 0x00ff3f00u, 0x00ff1f00u, 0x00ff0f00u,
    0x00ff0700u, 0x00ff0300u, 0x00ff0100u, 0x00ff0000u,
    0x007f0000u, 0x003f0000u, 0x001f0000u, 0x000f0000u,
    0x00070000u, 0x00030000u, 0x00010000u, 0x00000000u,
};

/* 0x481df: round a fixed-point value v = {u32 lo, u32 hi, u16 exp} (exp at
 * offset 8) toward zero, down to a target precision, with optional
 * round-half (mode 0) and underflow to +inf/zero. Faithful to the disassembly
 * 0x481df..0x4830f. Register trace (EBX=magnitude-cursor, ECX/ESI=mantissa,
 * EAX=mask+1) is mirrored literally below. mode: 0=round-half, 0x400=truncate,
 * other nonzero=truncate. */
void fx_round_down(uint32_t *v, uint16_t mode)                                 /* 0x481df */
{
    uint16_t bx;
    uint32_t ecx, esi, eax, cf;
    const uint16_t e = (uint16_t)v[2];

    bx = (uint16_t)(e & 0x7fff);                                              /* AND BX,0x7fff */
    if ((mode == 0x400 || mode == 0x800) && ((int16_t)e < 0)) mode ^= 0xc00;  /* 0x481e8 */
    if (mode == 0xc00) mode = 0x400;                                          /* 0x481ff */

    bx = (uint16_t)(bx - 0x403f);                                             /* SUB BX,0x403f */
    if (bx < 0x8000) return;                                                  /* JL: (int16)bx>=0 (mag>=0x403f) -> no-op */
    ecx = v[0];                                                               /* MOV ECX,[EDI] */
    bx = (uint16_t)(bx + 0x20);                                               /* ADD BX,0x20 (mag-0x401f) */
    if (bx < 0x8000) goto tableA;                                             /* JL 0x4826e: (int16)bx>=0 -> tableA */

    /* underflow: mag < 0x401f (0x4826e) */
    v[0] = 0;                                                                 /* lo = 0 */
    esi = v[1];                                                               /* ESI = hi */
    bx = (uint16_t)(bx + 0x20);                                               /* ADD BX,0x20 (mag-0x3fff) */
    if (bx < 0x8000) goto tableB;                                            /* JL 0x482d1: (int16)bx>=0 -> tableB */

    /* zero/inf: mag < 0x3fff (0x482d1) */
    if (mode == 0x400) { v[1] = 0; v[2] = 0; return; }                        /* 0x48306 */
    if (mode != 0) { v[1] = 0x80000000u; v[2] = (uint16_t)(e | 0x3fff); return; } /* 0x482f6 */
    bx = (uint16_t)(bx + 1);                                                  /* INC BX (mag-0x3ffe) */
    if (bx != 0) { v[1] = 0; v[2] = 0; return; }                              /* mag != 0x3ffe -> 0 */
    if (esi != 0x80000000u) { v[1] = 0x80000000u; v[2] = (uint16_t)(e | 0x3fff); return; }
    if (ecx == 0) { v[1] = 0; v[2] = 0; return; }                             /* ecx = original lo */
    v[1] = 0x80000000u; v[2] = (uint16_t)(e | 0x3fff); return;                /* +inf */

tableA:                                                                        /* 0x48220, bx=mag-0x401f (0..31), on lo */
    eax = g_fx_round_tbl[bx];                                                  /* MOV EAX,[EBX*4+0x4815f] */
    ecx &= eax;                                                                /* AND ECX,EAX */
    v[0] ^= ecx;                                                               /* XOR [EDI],ECX */
    eax++;                                                                     /* INC EAX (mask+1) */
    if (mode == 0x400) return;                                                 /* JZ 0x4826c */
    if (mode != 0) goto Aadd;                                                  /* JNZ 0x48259 */
    ecx += ecx;                                                                /* ADD ECX,ECX */
    if (eax > ecx) return;                                                     /* CMP; JA 0x4826c */
    if (eax != ecx) goto Aadd;                                                 /* JNZ 0x48259 */
    cf = eax >> 31;                                                            /* ROL EAX,1 */
    eax = (eax << 1) | cf;
    if (cf) { if ((v[1] & eax) == 0) return; }                                 /* TEST hi */
    else      { if ((v[0] & eax) == 0) return; }                               /* TEST lo */
    eax = (eax >> 1) | (cf ? 0x80000000u : 0);                                 /* ROR EAX,1 (restore) */
Aadd:
    if (ecx == 0) return;                                                      /* OR ECX,ECX; JZ */
    { uint64_t s  = (uint64_t)v[0] + eax; v[0] = (uint32_t)s;
      uint32_t c1 = (uint32_t)(s >> 32);
      uint64_t s2 = (uint64_t)v[1] + c1; v[1] = (uint32_t)s2;                  /* ADD/ADC */
      if (s2 >> 32) { v[1] = (v[1] >> 1) | 0x80000000u; v[2]++; } }            /* RCR; INC exp */
    return;

tableB:                                                                        /* 0x4827e, bx=mag-0x3fff (0..31), on hi */
    eax = g_fx_round_tbl[bx];                                                  /* MOV EAX,[EBX*4+0x4815f] */
    esi &= eax;                                                                /* AND ESI,EAX */
    v[1] ^= esi;                                                               /* XOR [EDI+4],ESI */
    eax++;                                                                     /* INC EAX (mask+1) */
    if (mode == 0x400) return;                                                 /* JZ 0x482ce */
    if (mode != 0) { if ((ecx | esi) == 0) return; goto Bc2; }                 /* 0x482be w/ unmodified ecx/esi */
    cf = ecx >> 31;                                                            /* ROL ECX,1 (ecx=lo) */
    {
        uint32_t lo1 = (ecx << 1) | cf;
        esi = esi + esi + cf;                                                  /* ADC ESI,ESI */
        if (eax > esi) return;                                                 /* CMP; JA 0x482ce */
        if (eax != esi) { if ((lo1 | esi) == 0) return; goto Bc2; }            /* JNZ 0x482be */
        if (lo1 != 0) goto Bc2;                                                /* JNZ 0x482c2 (skip JZ) */
        { uint32_t d = eax; uint32_t cf2 = d >> 31; d = (d << 1) | cf2;        /* ADD EAX,EAX */
          if (!cf2) { if ((v[1] & d) == 0) return; }                           /* JC skip; TEST hi; JZ */
          eax = d >> 1; }                                                      /* SHR EAX,1 */
        if ((lo1 | esi) == 0) return;                                          /* 0x482be: OR ECX,ESI; JZ */
    }
Bc2:
    { uint64_t s = (uint64_t)v[1] + eax; v[1] = (uint32_t)s;
      if (s >> 32) { v[1] = (v[1] >> 1) | 0x80000000u; v[2]++; } }             /* ADD; RCR; INC exp */
    return;
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
/* Parse leading decimal digits (v = v*10 + digit) until a non-digit. Returns a
 * pointer to the first non-digit char (0x49a48 returns EAX = ptr past the
 * digits); the caller (0x049a6e) uses it as the new scan position. */
const char *parse_decimal(const char *s, uint32_t *out)   /* 0x49a48 */
{
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') {         /* 0x49a4e CMP JC/JA */
        v = v * 10u + (uint32_t)(*s - '0');  /* IMUL EDX,EDX,0xa; ...; SUB 0x30 */
        s++;
    }
    *out = v;
    return s;                                /* EAX = pointer past the digits */
}

/* 0x049a6e: time-string parser (see f.h for the contract). Register ABI:
 * EAX=str, EDX=prefix_buf, EBX=secs_out; returns EAX (position after time).
 * The segmented prefix copy (REP MOVSD/MOVSB ES:EDI, ESI) is a plain memcpy in
 * the flat model; ESI (the copy source) is later repurposed as the negate flag. */
const char *parse_time_str(const char *str, char *prefix_buf, uint32_t *secs_out)
{
    const char *p = str;
    if (*p == ':') ++p;                       /* 0x49a7b skip leading ':' */
    const char *start = p;                    /* 0x49a81 ESI = start */
    uint8_t ch;
    for (;;) {                                /* 0x49a83 find the break char */
        ch = (uint8_t)*p;
        if (ch == 0 || ch == ',' || ch == '-' || ch == '+') break;  /* 0x49a8a..f */
        if (ch < '0') { ++p; continue; }      /* 0x49a99 JC (< '0') */
        if (ch <= '9') break;                 /* 0x49a9e JBE (digit) */
        ++p;                                  /* 0x49aa3 advance (char > '9') */
    }
    uint32_t offset = (uint32_t)(p - start);  /* 0x49aa8 */
    if (offset > 0x1e) offset = 0x1e;         /* 0x49aae clamp to 30 */
    memcpy(prefix_buf, start, offset);        /* 0x49ace/0x49ad5 */
    prefix_buf[offset] = 0;                   /* 0x49ae2 NUL-terminate */
    int negate = (ch == '-') ? 1 : 0;         /* 0x49ae8/0x49aed */
    const char *timepos = p + 1;              /* 0x49ae5/0x49af9 break_pos+1 */
    uint8_t next = (uint8_t)timepos[0];       /* 0x49afb AL=[EBP] (in-bounds: NUL-terminated) */
    if (next < '0' || next > '9') return timepos;  /* 0x49afe/0x49b00/0x49b08 */
    uint32_t c[3] = {0, 0, 0};                /* [ESP]=sec [ESP+4]=min [ESP+8]=hr */
    const char *q = parse_decimal(timepos, &c[2]);   /* hours -> c[2] */
    if (*q == ':') {                          /* 0x49b28 */
        q = parse_decimal(q + 1, &c[1]);      /* minutes -> c[1] */
        if (*q == ':') {                       /* 0x49b3d (CH==CL, both ':') */
            q = parse_decimal(q + 1, &c[0]);   /* seconds -> c[0] */
        }
    }
    uint32_t total = c[0] + c[1] * 60u + c[2] * 3600u;  /* 0x49b4b..0x49b6c */
    if (negate) total = (uint32_t)(-(int32_t)total);    /* 0x49b74/0x49b76 */
    *secs_out = total;
    return q;                                  /* 0x49b7a EAX = EBP */
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

/* ---- 0x047xxx fn_04794a wrappers (register-ABI) ----
 * All three load input A (EAX) and B (EDX struct for 0x0478f0/0x0478e2, or 3
 * stack scalars for 0x04791a), compose ESI=(b_exp_hi<<16)|a->exp, call
 * fn_04794a, and store the 64-bit result + a->exp into the OUT struct (the
 * original EBX register: pushed at entry, overwritten as a temp, popped, then
 * written to). 0x0478e2 additionally flips bit15 of b->exp before composing.
 * Verified structurally against the differentially-verified fn_04794a. */
static void fx4794a_store(fx128 *out, uint32_t a_lo, uint32_t a_hi, uint16_t a_exp,
                          uint32_t b_lo, uint32_t b_hi, uint32_t exp_hi)
{
    uint32_t esi = ((exp_hi & 0xffffu) << 16) | (a_exp & 0xffffu);
    uint64_t r = fn_04794a(a_lo, a_hi, b_lo, b_hi, esi);
    out->lo = (uint32_t)(r & 0xffffffffu);
    out->hi = (uint32_t)(r >> 32);
    out->exp = a_exp;
}

void fn_0478f0(fx128 *a, fx128 *out, const fx128 *b)
{ fx4794a_store(out, a->lo, a->hi, a->exp, b->lo, b->hi, (uint32_t)b->exp); }

void fn_0478e2(fx128 *a, fx128 *out, const fx128 *b)
{ fx4794a_store(out, a->lo, a->hi, a->exp, b->lo, b->hi, (uint32_t)(b->exp ^ 0x8000u)); }

void fn_04791a(fx128 *a, fx128 *out, uint32_t b_lo, uint32_t b_hi, uint32_t exp_hi)
{ fx4794a_store(out, a->lo, a->hi, a->exp, b_lo, b_hi, exp_hi); }

/* 0x048f4c: register-ABI loop over an array of fx128 (see f.h for the call
 * pattern). ESI is tracked literally: ROL ESI,16 = swap the two 16-bit halves;
 * MOV SI,x = set low16, preserve high16. acc=EAX:EDX starts at arr[0] and is
 * re-threaded through each fn_04794a result; each iteration reads arr[i+1].
 * Faithful mirror of the 32-instruction disasm (verified ESI trace matches
 * fx_mul64<-ESI=(E0<<16)|arr[0].exp every iter, fn_04794a<-ESI=(arr[i+1].exp<<16)|arr[0].exp). */
uint64_t fn_048f4c(uint32_t mant_lo, uint32_t mant_hi, uint32_t count,
                   const fx128 *arr, uint32_t exp_base, uint32_t *out_final_esi)
{
    uint32_t EAX = arr[0].lo, EDX = arr[0].hi;
    uint32_t ESI = (uint32_t)arr[0].exp;      /* 048f5e: MOV SI,[arr[0]+8], high16=0 */
    uint32_t E0 = exp_base & 0xffffu;         /* [EBP+8] = orig ESI low16 */
    for (uint32_t i = 0; i < count; i++) {
        ESI = (ESI << 16) | (ESI >> 16);      /* 048f63 ROL ESI,16 */
        ESI = (ESI & 0xffff0000u) | E0;       /* 048f66 MOV SI,[EBP+8] */
        ESI = (ESI << 16) | (ESI >> 16);      /* 048f6a ROL ESI,16 */
        fx64_t o; fn_047d4e(EAX, EDX, mant_lo, mant_hi, ESI, &o); /* 048f73: rd4e returns exp in ESI */
        EAX = o.lo; EDX = o.hi;
        ESI = (o.exp << 16) | (o.exp >> 16);   /* 048f82 ROL ESI,16 on rd4e's RETURNED exp */
        ESI = (ESI & 0xffff0000u) | (arr[i+1].exp & 0xffffu);   /* 048f85 MOV SI,[arr[i+1]+8] */
        ESI = (ESI << 16) | (ESI >> 16);      /* 048f8a ROL ESI,16 */
        uint64_t r = fn_04794a(EAX, EDX, arr[i+1].lo, arr[i+1].hi, ESI); /* 048f8d */
        EAX = (uint32_t)r; EDX = (uint32_t)(r >> 32);
    }
    if (out_final_esi) *out_final_esi = ESI;   /* final 32-bit ESI (post last ROL) */
    return ((uint64_t)EDX << 32) | EAX;
}

/* 0x048f2c: register-ABI in-place wrapper around 0x048f4c. Loads a (EAX:
 * {lo,hi,exp}), arr=EDX, count=EBX, calls 048f4c(a->lo,a->hi,count,arr,a->exp),
 * stores the 64-bit result back into a (lo,hi); a->exp is preserved (the loaded
 * SI is written back verbatim). */
void fn_048f2c(fx128 *a, const fx128 *arr, uint32_t count)
{
    uint64_t r = fn_048f4c(a->lo, a->hi, count, arr, (uint32_t)a->exp, NULL);
    a->lo = (uint32_t)r;
    a->hi = (uint32_t)(r >> 32);
}

/* 0x048f9e: register-ABI in-place. Computes a = (048f4c(a*a, count, arr, a->exp))
 * * a. ESI for both fx_mul64 calls = (a->exp<<16)|a->exp (the 16-bit half is
 * replicated). a->exp preserved. Sequence: R=a*a (fx_mul64); S=048f4c(R,count,
 * arr,a->exp); a = S*a (fx_mul64). Faithful mirror of the 30-instruction disasm. */
void fn_048f9e(fx128 *a, uint32_t count, const fx128 *arr)
{
    uint32_t esi = ((uint32_t)a->exp << 16) | (uint32_t)a->exp;
    uint64_t r = fx_mul64(a->lo, a->hi, a->lo, a->hi, esi);        /* R = a^2 */
    uint64_t s = fn_048f4c((uint32_t)r, (uint32_t)(r >> 32), count, arr, (uint32_t)a->exp, NULL);
    uint64_t t = fx_mul64((uint32_t)s, (uint32_t)(s >> 32), a->lo, a->hi, esi); /* t = s * a */
    a->lo = (uint32_t)t;
    a->hi = (uint32_t)(t >> 32);
}

/* ===================== fn_048a48 (0x00048a48, L3, 502B, 171 insns) =====================
 * In-place fx128 transform: void fn_048a48(fx128 *a, uint16_t esi).  The ONLY callers
 * are the 3 L4 thunks 0x048c3f/0x048c49/0x048c56 (esi=0/2/1), which set only ESI; EDX/
 * ECX/EBX are phantom Ghidra params.  5-step algorithm (trace-faithful to
 * scratch/dis_048a48_raw.txt; Python ref scratch/fp_048a48_ref.py):
 *   1. r=0485eb(a.a, b=0x2168c235:0xc90fdaa2, expA=a.exp, expB=0x3ffe); a={r}; r_edi=r.edi.
 *   2. signblock iff a.exp bit7: a=04791a(a,a,b,0x3ffe); base=(sign?r_edi-1:r_edi).
 *   3. F=(base&0xffff0000)|((esi&0xff)<<8)|(((base&0xff)+((esi&0xff)&2))&7); F0=F&0xff,
 *      F1=(F>>8)&0xff; AL&1 block iff (F0&1): a=04791a(a,a,b,0xbffe); a.exp^=0x8000.
 *   4. d2=(int16)a.exp-0x3ffe; if d2<-0x20: triple={0,0x80000000,0x3fff}; else
 *      idx=clamp(-d2,0,8), scale=0x48a36[idx]; f4c iff ((F0+1)&2)||F1==1 (triple=f4c(a^2));
 *      f9e iff ((F0+1)&2)==0||F1==1 (a=f9e(a), in-place).
 *   5. select a iff ((F0+1)&2)==0 else triple; sel.exp^=0x8000 iff (F0&4). If F1!=1 write
 *      sel. Else (CH, esi==1): ch=the OTHER source; ch.exp^=0x8000 iff ((F0+2)&4); build
 *      ESI=(ch.exp<<16)|sel.exp; if ch.all-zero: a={0,0x80000000,0x7fff} else
 *      a=047b79(sel.lo,sel.hi,ESI,ch.hi,ch.lo).
 * CORRECTIONS vs the earlier (c3258f5) spec: 0485eb expB=0x3ffe (DI; MOV EDX clobbers EDX,
 * NOT EDI -- the 8aba586 "correction" was itself wrong); select/ch re-read use (F0+1)&2 /
 * (F0+3)&2 (complementary, keyed on F0&3), NOT "F0 even/odd"; the callee ESI high16 (H)
 * is discarded by the CH re-read (no fn_048f9e ESI out needed). */
const fx128 g_coeff_48982[9] = {          /* 0x048982 (048f9e path) */
    {0x856a5359u, 0xca963b81u, 0x3fce}, {0x9dc0f88fu, 0xd73f9f39u, 0xbfd6},
    {0x43684be5u, 0xb092309du, 0x3fde}, {0xaa271c7fu, 0xd7322b3fu, 0xbfe5},
    {0xb6399c7du, 0xb8ef1d2au, 0x3fec}, {0x0d00d00cu, 0xd00d00d0u, 0xbff2},
    {0x88888888u, 0x88888888u, 0x3ff8}, {0xaaaaaaaau, 0xaaaaaaaau, 0xbffc},
    {0x00000000u, 0x80000000u, 0x3fff}
};
const fx128 g_coeff_489dc[9] = {          /* 0x0489dc (f4c path) */
    {0x9dc0f88fu, 0xd73f9f39u, 0x3fd2}, {0x03e4e905u, 0xc9cba546u, 0xbfda},
    {0xc6c4bdaau, 0x8f76c77fu, 0x3fe2}, {0xc4fae397u, 0x93f27dbbu, 0xbfe9},
    {0x0d00d00du, 0xd00d00d0u, 0x3fef}, {0x0b60b60bu, 0xb60b60b6u, 0xbff5},
    {0xaaaaaaaau, 0xaaaaaaaau, 0x3ffa}, {0x00000000u, 0x80000000u, 0xbffe},
    {0x00000000u, 0x80000000u, 0x3fff}
};
const uint16_t g_scale_48a36[9] = { 0, 1, 2, 3, 4, 4, 5, 5, 6 }; /* 0x048a36 */

static uint64_t r91a_pair(uint32_t lo, uint32_t hi, uint32_t a_exp,
                          uint32_t b_lo, uint32_t b_hi, uint32_t exp_hi)
{
    /* fn_04791a (fx4794a_store): esi=(exp_hi<<16)|a_exp; mantissa=04794a(a,b,esi);
     * out.exp = a_exp (preserved). Returns (hi<<32)|lo. */
    uint32_t esi = ((exp_hi & 0xffffu) << 16) | (a_exp & 0xffffu);
    return fn_04794a(lo, hi, b_lo, b_hi, esi);
}

void fn_048a48(fx128 *a, uint16_t esi_in)
{
    uint32_t lo = a->lo, hi = a->hi;
    uint32_t exp = a->exp & 0xffffu;
    uint32_t esi = esi_in & 0xffffu;

    /* STEP 1: 0485eb align-subtract-normalize (expA=a.exp, expB=0x3ffe=DI) */
    fx_sub_t r;
    fn_0485eb(lo, hi, 0x2168c235u, 0xc90fdaa2u, exp, 0x3ffe, &r);
    lo = r.lo; hi = r.hi; exp = r.exp;
    uint32_t r_edi = r.edi;

    /* STEP 2: sign block iff a.exp bit7 */
    int signpath = (exp & 0x8000u) != 0;
    uint32_t base = signpath ? (r_edi - 1u) : r_edi;
    if (signpath) {
        uint64_t m = r91a_pair(lo, hi, exp, 0x2168c235u, 0xc90fdaa2u, 0x3ffe);
        lo = (uint32_t)(m & 0xffffffffu); hi = (uint32_t)(m >> 32);
    }

    /* STEP 3: F-flags + AL&1 block */
    uint32_t F = (base & 0xffff0000u) | ((esi & 0xffu) << 8)
                | (((base & 0xffu) + ((esi & 0xffu) & 2u)) & 7u);
    uint32_t F0 = F & 0xffu;
    uint32_t F1 = (F >> 8) & 0xffu;
    if ((F0 & 1u) != 0) {
        uint64_t m = r91a_pair(lo, hi, exp, 0x2168c235u, 0xc90fdaa2u, 0xbffe);
        lo = (uint32_t)(m & 0xffffffffu); hi = (uint32_t)(m >> 32);
        exp = (exp ^ 0x8000u) & 0xffffu;
    }

    /* STEP 4: d2 + poly */
    int d2 = (int)(int16_t)exp - 0x3ffe;
    uint32_t t_lo = 0, t_hi = 0, t_exp = 0;
    if (d2 < -0x20) {
        t_lo = 0; t_hi = 0x80000000u; t_exp = 0x3fff;
    } else {
        int idx = (d2 < 0) ? (-d2) : 0;
        if (idx > 8) idx = 8;
        uint32_t scale = g_scale_48a36[idx];
        if (((F0 + 1u) & 2u) != 0 || F1 == 1u) {                 /* f4c path */
            fx64_t sq;
            fn_047d4e(lo, hi, lo, hi, ((exp & 0xffffu) << 16) | (exp & 0xffffu), &sq);
            uint32_t f4_esi;
            uint64_t f4 = fn_048f4c(sq.lo, sq.hi, (8u - scale), &g_coeff_489dc[scale],
                                    sq.exp, &f4_esi);
            t_lo = (uint32_t)(f4 & 0xffffffffu);
            t_hi = (uint32_t)(f4 >> 32);
            t_exp = f4_esi & 0xffffu;
        }
        if (((F0 + 1u) & 2u) == 0 || F1 == 1u) {                 /* f9e path (in-place) */
            fx128 tmp = { lo, hi, (uint16_t)exp };
            fn_048f9e(&tmp, (8u - scale), &g_coeff_48982[scale]);
            lo = tmp.lo; hi = tmp.hi; exp = tmp.exp;
        }
    }

    /* STEP 5: select (a iff ((F0+1)&2)==0) + conditional 047b79 */
    uint32_t s_lo, s_hi, s_exp, c_lo, c_hi, c_exp;
    if (((F0 + 1u) & 2u) == 0) {
        s_lo = lo; s_hi = hi; s_exp = exp;
        c_lo = t_lo; c_hi = t_hi; c_exp = t_exp;
    } else {
        s_lo = t_lo; s_hi = t_hi; s_exp = t_exp;
        c_lo = lo; c_hi = hi; c_exp = exp;
    }
    if (F0 & 4u) s_exp = (s_exp ^ 0x8000u) & 0xffffu;

    if (F1 != 1u) {                                              /* no CH: write select */
        a->lo = s_lo; a->hi = s_hi; a->exp = (uint16_t)s_exp;
        return;
    }

    /* CH block (F1 == 1, esi == 1) */
    if ((F0 + 2u) & 4u) c_exp = (c_exp ^ 0x8000u) & 0xffffu;
    uint32_t ESI = ((c_exp & 0xffffu) << 16) | (s_exp & 0xffffu);
    if (c_lo == 0 && c_hi == 0 && (ESI & 0x7fff0000u) == 0) {    /* underflow -> 0x1.0 */
        a->lo = 0; a->hi = 0x80000000u; a->exp = 0x7fff;
        return;
    }
    uint32_t o_lo, o_hi; uint16_t o_si;
    fn_047b79(s_lo, s_hi, ESI, c_hi, c_lo, &o_lo, &o_hi, &o_si);
    a->lo = o_lo; a->hi = o_hi; a->exp = o_si;
}

/* L4 thunks around fn_048a48 (register-ABI, a in EAX, ESI preserved): each is
 * PUSH ESI; MOV/SUB ESI,k; CALL 0x048a48; POP ESI; RET. 0x048c3f->k=0 (SUB ESI,ESI),
 * 0x048c49->k=2, 0x048c56->k=1. */
void fn_048c3f(fx128 *a) { fn_048a48(a, 0); }
void fn_048c49(fx128 *a) { fn_048a48(a, 2); }
void fn_048c56(fx128 *a) { fn_048a48(a, 1); }

/* 0x4872c / 0x4870e coefficient tables (10-byte fx128 entries) for fn_0487b0,
 * extracted byte-for-byte from NIGHT.EXE (file offset = RVA - 0x2600). 0x4872c:
 * 4 entries (f4c count=3 -> arr[0..3]); 0x4870e: 3 entries (count=2 -> arr[0..2]).
 * 0x4872c[0] is the zero/exp-0x3fff sentinel (byte-identical to g_coeff_48312[8]). */
static const fx128 g_coeff_4872c[4] = {
    {0x00000000u, 0x80000000u, 0x3fff},
    {0x3e7076bbu, 0x8eac025bu, 0xc004},
    {0xa933ef60u, 0x9c041fd0u, 0x4007},
    {0x6c83bb96u, 0xc05ff4e0u, 0xc008},
};
static const fx128 g_coeff_4870e[3] = {
    {0xb5e946e9u, 0xca20ad9au, 0xbffe},
    {0xb57f6509u, 0x83125100u, 0x4003},
    {0x9dacd228u, 0x803ff895u, 0xc005},
};

/* ROL ESI,16; MOV SI,src_low16; ROL ESI,16 -- the ESI re-derivation idiom the
 * 0x0487b0 disasm repeats after each callee clobbers ESI. (Callees return only
 * low16 of their post-call ESI via o_si/out_final_esi; the high16 is always
 * washed out by the MOV SI,src step, so low16 suffices for a faithful result.) */
static uint32_t jb70_juggle(uint32_t esi, uint32_t src)
{
    esi = (esi << 16) | (esi >> 16);
    esi = (esi & 0xffff0000u) | (src & 0xffffu);
    return (esi << 16) | (esi >> 16);
}

/* 0x0487b0 (L2, 464B, 158 insns): in-place fx128 transform. Faithful mirror of
 * scratch/fp_0487b0_ref.py (trace-faithful to scratch/dis_0487b0.txt). 16 callee
 * calls: r94a x6, rb79 x2, rd4e x5, f4c x2, f0e x1. ESI is tracked literally (each
 * callee clobbers it; re-derived via jb70_juggle / [ESP] reads). flagword =
 * orig_exp - 0x3ffe (else) / -0x3fff (if, DEC [ESP]); the JZ short path (fw==0)
 * stores {r3}; otherwise the f0e block (fx_from_s32) runs and result is {r5}. */
void fn_0487b0(fx128 *a)
{
    uint32_t alo = a->lo, ahi = a->hi;
    uint32_t orig_exp = a->exp & 0xffffu;

    /* 0x0487b5-0x0487cf: fw = orig_exp - 0x3ffe (32-bit wrap); a.exp <- 0x3ffe */
    uint32_t fw = orig_exp - 0x3ffeu;
    uint32_t aexp = 0x3ffe;

    /* 0x0487d4-0x0487e5: R = r94a(a, 0, 0x80000000, 0xbffe3ffe) */
    uint64_t r = fn_04794a(alo, ahi, 0, 0x80000000u, 0xbffe3ffeu);
    uint32_t Rlo = (uint32_t)r, Rhi = (uint32_t)(r >> 32);

    /* 0x0487ea-0x048826: if-branch iff (hi==0xb504f333 && lo<=0xf9de6484) */
    if (ahi == 0xb504f333u && alo <= 0xf9de6484u) {
        alo = Rlo; ahi = Rhi; aexp = 0x3ffe;
        fw = fw - 1u;                       /* 0x048826 DEC [ESP] */
    }

    /* 0x048829-0x04884a: t2 = r94a(a, 0, 0x80000000, 0x3ffe3ffd) */
    r = fn_04794a(alo, ahi, 0, 0x80000000u, 0x3ffe3ffdu);
    uint32_t t2lo = (uint32_t)r, t2hi = (uint32_t)(r >> 32);

    /* 0x04884f-0x04885c: o = rb79(R, 0x3ffd3ffe, t2) */
    uint32_t olo, ohi; uint16_t oesi;
    fn_047b79(Rlo, Rhi, 0x3ffd3ffeu, t2hi, t2lo, &olo, &ohi, &oesi);

    /* 0x048861-0x04887a: a = rd4e(o, o, (oesi<<16)|oesi) */
    fx64_t o; fn_047d4e(olo, ohi, olo, ohi, ((uint32_t)oesi << 16) | (uint32_t)oesi, &o);
    alo = o.lo; ahi = o.hi; aexp = o.exp & 0xffffu;

    /* 0x04887e-0x0488a5: f4c#1 = f4c(a,3,0x4872c,a.exp); f4c#2 = f4c(a,2,0x4870e,a.exp) */
    uint32_t f4c1esi, f4c2esi;
    uint64_t f1 = fn_048f4c(alo, ahi, 3, g_coeff_4872c, aexp, &f4c1esi);
    uint32_t f4c1lo = (uint32_t)f1, f4c1hi = (uint32_t)(f1 >> 32);
    uint64_t f2 = fn_048f4c(alo, ahi, 2, g_coeff_4870e, aexp, &f4c2esi);
    uint32_t f4c2lo = (uint32_t)f2, f4c2hi = (uint32_t)(f2 >> 32);

    /* 0x0488aa-0x0488b6: o2 = rb79(f4c2, juggle(f4c2esi,f4c1esi), f4c1) */
    uint32_t esi_j = jb70_juggle(f4c2esi, f4c1esi);
    uint32_t o2lo, o2hi; uint16_t o2esi;
    fn_047b79(f4c2lo, f4c2hi, esi_j, f4c1hi, f4c1lo, &o2lo, &o2hi, &o2esi);

    /* 0x0488bb-0x0488ca: m2 = rd4e(o2, a, juggle(o2esi, a.exp)) */
    esi_j = jb70_juggle(o2esi, aexp);
    fn_047d4e(o2lo, o2hi, alo, ahi, esi_j, &o);
    uint32_t m2lo = o.lo, m2hi = o.hi, m2exp = o.exp;

    /* 0x0488cf-0x0488e1: m3 = rd4e(m2, o, juggle(m2exp, oesi)) */
    esi_j = jb70_juggle(m2exp, oesi);
    fn_047d4e(m2lo, m2hi, olo, ohi, esi_j, &o);
    uint32_t m3lo = o.lo, m3hi = o.hi, m3exp = o.exp;

    /* 0x0488e6-0x0488f2: r3 = r94a(m3, o, juggle(m3exp, oesi)) */
    esi_j = jb70_juggle(m3exp, oesi);
    r = fn_04794a(m3lo, m3hi, olo, ohi, esi_j);
    uint32_t r3lo = (uint32_t)r, r3hi = (uint32_t)(r >> 32);

    /* 0x0488f7-0x0488fa: JZ (fw==0) -> short path {r3} */
    if (fw == 0) {
        a->lo = r3lo; a->hi = r3hi; a->exp = esi_j & 0xffffu;
        return;
    }

    /* 0x0488fc-0x048903: a = fx_from_s32(fw) (f0e) */
    struct fxnum fn; fx_from_s32(&fn, (int32_t)fw);
    alo = fn.lo; ahi = fn.hi; aexp = fn.exp & 0xffffu;

    /* 0x04890d-0x048927: c1 = rd4e(a, 0x865435c, 0xde8082e3, a.exp|0xbff20000) */
    fn_047d4e(alo, ahi, 0x0865435cu, 0xde8082e3u, (aexp & 0xffffu) | 0xbff20000u, &o);
    uint32_t c1lo = o.lo, c1hi = o.hi, c1exp = o.exp;

    /* 0x04892c-0x048938: r4 = r94a(c1, r3, juggle(c1exp, m3exp)) [ESI_p3 low16=m3exp low16] */
    esi_j = jb70_juggle(c1exp, m3exp);
    r = fn_04794a(c1lo, c1hi, r3lo, r3hi, esi_j);
    uint32_t r4lo = (uint32_t)r, r4hi = (uint32_t)(r >> 32);

    /* 0x048940-0x04895c: c2 = rd4e(a, 0, 0xb1800000, a.exp|0x3ffe0000) */
    fn_047d4e(alo, ahi, 0, 0xb1800000u, (aexp & 0xffffu) | 0x3ffe0000u, &o);
    uint32_t c2lo = o.lo, c2hi = o.hi, c2exp = o.exp;

    /* 0x048961-0x04896d: r5 = r94a(c2, r4, juggle(c2exp, c1exp)) [ESI_p4 low16=c1exp low16] */
    esi_j = jb70_juggle(c2exp, c1exp);
    r = fn_04794a(c2lo, c2hi, r4lo, r4hi, esi_j);
    uint32_t r5lo = (uint32_t)r, r5hi = (uint32_t)(r >> 32);

    /* 0x048972-0x048977: a = {r5, c2exp_low16} */
    a->lo = r5lo; a->hi = r5hi; a->exp = esi_j & 0xffffu;
}

/* 0x04876d: a=EAX, b=EDX (both fx128*). Three in-place fx128 steps:
 *   (1) 0x048773 a = fn_0487b0(a)              (in place)
 *   (2) 0x04877c b = a * b                      (fn_047d24(a,b,out=b), in place on b)
 *   (3) 0x048782-..a6 b = b * fx128(0x5c17f0bd,0xb8aa3b29)  in place,
 *       esi=(b->exp<<16)|0x3fff, result exp written back to b->exp.
 * The 047d24 out is b (MOV EBX,EDX; out=EBX); EDI=b (PUSH EDX;POP EDI) is the tail
 * base. Result in b; void return (EAX clobbered by the multiply). */
void fn_04876d(fx128 *a, fx128 *b)
{
    fn_0487b0(a);                                                    /* 0x048773 */
    fn_047d24((uint32_t *)a, (uint32_t *)b, (uint32_t *)b);          /* 0x04877c: b=a*b, out=b */
    uint32_t esi = ((uint32_t)b->exp << 16) | 0x3fffu;               /* 0x048782/8b/8e */
    fx64_t o;
    fn_047d4e(b->lo, b->hi, 0x5c17f0bd, 0xb8aa3b29, esi, &o);        /* 0x04879c */
    b->lo = o.lo;                                                    /* 0x0487a1 */
    b->hi = o.hi;                                                    /* 0x0487a3 */
    b->exp = (uint16_t)o.exp;                                        /* 0x0487a6 */
}

/* fn_048c8c  orig: 0x00048c8c (L2, 467B, 163 insns)
 * In-place single-arg fx128 op. Sole caller: FUN_000461cc (big parser/dispatcher).
 * Trace-faithful mirror (scratch/dis_048c8c_raw.txt); the Python reference
 * (scratch/fp_048c8c_ref.py) is the differential oracle.
 *
 *   ESI := 0 (neg flag); a.exp &= 0x7fff
 *   if !(a == 0):
 *     if orig_exp bit15: ESI := 0xffffffff; a = fn_04791a(a,{0,0x80000000},0xbfff); a.exp ^= 0x8000
 *     normalize (2 shift-left loops + overflow ECX); a := (EAX,EDX,BX); carry := ECX
 *   sq = a*a; t1 = sq*(0x1bf21f8c,0xa6829a79); t2 = sq*(0x9d7bfdb,0xec96f0d6)
 *   t3 = t2*(0xe536e187,0xe6d5051a); t4 = t3*a; u = t4*t1; v = t4*t1
 *   a = fn_047b79(v, u, esi7)
 *   poly-eval over the carry bits: a = TBL[bit] * a for each set bit (TBL @ 0x048c64, 10-byte stride)
 *   a.exp += (ESI & 0xffff); a = fn_04791a(a,{0,0x80000000},0xbfff)
 *
 * NOTE (verified genuine): for every normalizable input the square's exp (~0x7fff)
 * plus the t2 constant's exp (0x3ffa) overflows, so t2 = +/-inf and fn_047b79(v=u)
 * yields {0,0,0}; the final fn_04791a then maps that to a sign sentinel: {0,0x80000000,0x0000}
 * (exp bit15 clear) or {0,0,0xffff} (set). Verified across 262K inputs (perfect 50/50
 * sign split). Because a = {0,0,0} after fn_047b79, the poly-eval loop multiplies zero
 * by the (finite) TBL coefficients and is a no-op; the ref's carry-bit-iteration form is
 * output-equivalent to the disasm loop (whose intricate register flow reads *(0x53030),
 * the last fn_04794a ECX, but is likewise a no-op on the saturated a).
 */
static const struct { uint32_t lo, hi; uint16_t exp; } fp_048c8c_tbl[32] = {
    {0xcc487b15,0x85aac367,0x3fff}, {0xea8bd6e7,0x8b95c1e3,0x3fff},
    {0x8db8a96f,0x9837f051,0x3fff}, {0xf9de6484,0xb504f333,0x3fff},
    {0x52565755,0xf88b5351,0xf62b}, {0x084f8b66,0x7f096780,0x8366},
    {0x7500087f,0x047f8309,0x7500}, {0x003f8303,0x0b666674,0x79c9},
    {0xff684e1a,0x680000bf,0x0000}, {0x006a8000,0xdf8bc78b,0x4fe8},
    {0x80ffffec,0x66800973,0x5f8b}, {0xc3836608,0x8bc92b04,0x0457},
    {0x8166078b,0x723ffffb,0x030a}, {0x13d213c0,0xeb4b66c9,0x0bef},
    {0x0b0275d2,0x2b0475c0,0xebdb}, {0x78d20b10,0x13c00308,0x66d2},
    {0x66f4eb4b,0x8904eb83,0x8907}, {0x89660457,0x02eb085f,0xc92b},
    {0x078b5156,0x6604578b,0x778b}, {0x10e6c108,0x08778b66,0xd88b},
    {0x1be8ca8b,0x50fffff0,0x5652}, {0x829a79b9,0x1f8cbba6,0x1bf2},
    {0xffffe681,0xce810000,0x0000}, {0xf9e84003,0x8bffffeb,0x87ec},
    {0x55870845,0x00758704,0xd6b9}, {0xbbec96f0,0x09d7bfdb,0xe681},
    {0x0000ffff,0x0000ce81,0x3ffa}, {0xffefd7e8,0x051ab9ff,0xe6d5},
    {0x36e187bb,0xffe681e5,0x00ff}, {0x00ce8100,0xe8400100,0xebb8},
    {0x1f8bffff,0xc1044f8b,0x10c6}, {0x08778b66,0xe810c6c1,0xefa8},
};

void fn_048c8c(fx128 *a)
{
    uint32_t esi = 0;                       /* neg flag (0 or 0xffffffff)        */
    uint16_t cx = a->exp;                   /* original exp                       */
    a->exp = a->exp & 0x7fff;               /* AND byte[9], 0x7f                 */
    uint32_t carry = 0;
    if (!(a->exp == 0 && a->hi == 0 && a->lo == 0)) {
        if (cx & 0x8000) {                  /* STEP 3: negate                    */
            esi = 0xffffffffu;
            fn_04791a(a, a, 0u, 0x80000000u, 0xbfffu);
            a->exp ^= 0x8000;
        }
        /* STEP 4: normalize (two shift-left passes + overflow ECX) */
        uint32_t bx = (uint32_t)(a->exp + 4) & 0xffff;
        uint32_t ecx = 0;
        uint32_t eax = a->lo, edx = a->hi;
        while (bx >= 0x3fff) {              /* pass 1: while BX >= 0x3fff        */
            uint32_t c1 = (eax >> 31) & 1u, c2 = (edx >> 31) & 1u;
            eax = eax << 1;
            edx = (edx << 1) | c1;
            ecx = (ecx << 1) | c2;
            bx = (bx - 1) & 0xffff;
        }
        if (edx != 0 || eax != 0) {
            while (!(edx & 0x80000000u)) {  /* pass 2: while hi top bit clear    */
                uint32_t c1 = (eax >> 31) & 1u;
                eax = eax << 1;
                edx = (edx << 1) | c1;
                bx = (bx - 1) & 0xffff;
            }
            bx = (bx - 4) & 0xffff;
        } else {
            bx = 0;
        }
        a->lo = eax; a->hi = edx; a->exp = (uint16_t)bx;
        carry = ecx;
    }
    /* STEP 5: math chain */
    fx64_t sq;
    fn_047d4e(a->lo, a->hi, a->lo, a->hi,
              ((uint32_t)a->exp << 16) | (uint32_t)a->exp, &sq);
    uint64_t t1 = fn_04794a(sq.lo, sq.hi, 0x1bf21f8cu, 0xa6829a79u,
                            0x40030000u | (sq.exp & 0xffffu));
    fx64_t t2;
    fn_047d4e(sq.lo, sq.hi, 0x9d7bfdbu, 0xec96f0d6u,
              0x3ffa0000u | (sq.exp & 0xffffu), &t2);
    uint64_t t3 = fn_04794a(t2.lo, t2.hi, 0xe536e187u, 0xe6d5051au,
                            0x40010000u | (t2.exp & 0xffffu));
    fx64_t t4;
    fn_047d4e((uint32_t)(t3 & 0xffffffffu), (uint32_t)(t3 >> 32),
              a->lo, a->hi,
              ((uint32_t)a->exp << 16) | (t2.exp & 0xffffu), &t4);
    uint64_t u = fn_04794a(t4.lo, t4.hi, (uint32_t)(t1 & 0xffffffffu), (uint32_t)(t1 >> 32),
                           ((sq.exp & 0xffffu) << 16) | ((t4.exp & 0xffffu) ^ 0x8000u));
    uint64_t v = fn_04794a(t4.lo, t4.hi, (uint32_t)(t1 & 0xffffffffu), (uint32_t)(t1 >> 32),
                           ((sq.exp & 0xffffu) << 16) | (t4.exp & 0xffffu));
    uint32_t o_lo, o_hi; uint16_t o_si;
    fn_047b79((uint32_t)(v & 0xffffffffu), (uint32_t)(v >> 32),
              ((((t4.exp & 0xffffu) ^ 0x8000u) << 16) | (t4.exp & 0xffffu)),
              (uint32_t)(u >> 32), (uint32_t)(u & 0xffffffffu),
              &o_lo, &o_hi, &o_si);
    a->lo = o_lo; a->hi = o_hi; a->exp = o_si;
    /* STEP 6: poly-eval over the carry bits (no-op on the saturated a = {0,0,0}) */
    uint32_t c = carry, i = 0;
    while (c != 0) {
        uint32_t bit = c & 1u; c >>= 1;
        if (bit) {
            const struct { uint32_t lo, hi; uint16_t exp; } *k = &fp_048c8c_tbl[i];
            fx64_t o;
            fn_047d4e(k->lo, k->hi, a->lo, a->hi,
                      ((uint32_t)a->exp << 16) | (uint32_t)k->exp, &o);
            a->lo = o.lo; a->hi = o.hi; a->exp = (uint16_t)o.exp;
        }
        i++;
    }
    /* STEP 7: exp += negflag; final scale */
    a->exp = (uint16_t)((uint16_t)a->exp + (uint16_t)(esi & 0xffffu));
    fn_04791a(a, a, 0u, 0x80000000u, 0xbfffu);
}

/* 0x048754: register-ABI in-place wrapper. a = fn_04791a(a, a, 0, 0x80000000,
 * 0x3fff). EAX = a (used as both input struct and out pointer; the 3 stack args
 * are constants b_lo=0, b_hi=0x80000000, exp_hi=0x3fff). Faithful mirror of the
 * 24-byte disasm. */
void fn_048754(fx128 *a)
{ fn_04791a(a, a, 0u, 0x80000000u, 0x3fffu); }

/* ---- 0x048422: FPU/fx128 composite (354B) ----
 * a = fx128* {lo,hi,exp:u16}; byte9 = high byte of exp, sign = exp&0x8000.
 * Mirrors the asm: clear sign; sentinel?; if exp>=0x3fff rescale via 0x047b79 (CH|=2);
 * threshold block (if (exp,hi,lo) > (0x3ffd,0x8930a2f4,0xf66ab09b) lexicographically):
 * 0x04794a/fx_mul64/0x04794a/0x047b79 sequence (CH|=1); poly-eval fn_048f9e(a,8,coeffs);
 * conditional 0x04791a scales (CH bit0=threshold, bit1=exp>=0x3fff); restore sign.
 * Coeff table is 90B (9 fx128, 10-byte stride) at 0x48312; unpacked to the port's
 * 12-byte fx128 before the poly-eval (values preserved). See STATUS.md spec. */
static const uint8_t g_coeff_48312[90] = {
    0x4e,0x18,0xe6,0xc0, 0xe4,0xc7,0xd1,0xb5, 0xfa,0x3f,
    0x29,0x0b,0x01,0x08, 0x08,0x9b,0xc6,0x84, 0xfb,0xbf,
    0x1a,0x26,0xc5,0x6c, 0x2e,0x02,0x46,0x9d, 0xfb,0x3f,
    0x2a,0x98,0x6b,0x0a, 0x9d,0xb9,0x2b,0xba, 0xfb,0xbf,
    0x71,0x01,0x2b,0x95, 0x27,0x27,0x8e,0xe3, 0xfb,0x3f,
    0xfc,0x68,0x9c,0xdd, 0x72,0x24,0x49,0x92, 0xfc,0xbf,
    0xc1,0xad,0xeb,0x94, 0xcc,0xcc,0xcc,0xcc, 0xfc,0x3f,
    0xf2,0x81,0x97,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xfd,0xbf,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x80, 0xff,0x3f,
};

void fn_048422(fx128 *a)
{
    uint8_t CL = (uint8_t)((a->exp >> 8) & 0xff);   /* byte9 (high byte of exp) */
    a->exp = (uint16_t)(a->exp & 0x7fff);           /* AND [EDI+9],0x7f: clear bit15 */

    /* Sentinel: exp==0x3fff && hi==0x80000000 && lo==0 */
    if (a->exp == 0x3fff && a->hi == 0x80000000u && a->lo == 0u) {
        a->lo = 0x2168c235u;
        a->hi = 0xc90fdaa2u;
        a->exp = (uint16_t)((((0x3fu | (CL & 0x80)) << 8) | 0xfeu));   /* AX=0x3ffe; AND CL,0x80; OR AH,CL */
        return;
    }

    uint8_t CH = 0;

    /* if exp>=0x3fff (cmp word,0x3fff; JL => op runs when exp>=0x3fff): rescale */
    if (a->exp >= 0x3fff) {
        uint32_t o_lo, o_hi; uint16_t o_exp;
        fn_047b79(0u, 0x80000000u, ((uint32_t)a->exp << 16) | 0x3fffu, a->hi, a->lo,
                  &o_lo, &o_hi, &o_exp);
        a->lo = o_lo; a->hi = o_hi; a->exp = o_exp;
        CH |= 2;
    }

    /* Threshold block: (exp,hi,lo) > (0x3ffd,0x8930a2f4,0xf66ab09b) lexicographically (unsigned) */
    int run_block = 0;
    if (a->exp > 0x3ffd) run_block = 1;
    else if (a->exp == 0x3ffd) {
        if (a->hi > 0x8930a2f4u) run_block = 1;
        else if (a->hi == 0x8930a2f4u && a->lo > 0xf66ab09bu) run_block = 1;
    }
    if (run_block) {
        uint32_t esi0 = (0x3fffu << 16) | a->exp;   /* (0x3fff<<16)|exp */
        uint64_t t1 = fn_04794a(a->lo, a->hi, 0xc265539eu, 0xddb3d742u, esi0);
        uint32_t t1_lo = (uint32_t)t1, t1_hi = (uint32_t)(t1 >> 32);
        fx64_t t2;
        fn_047d4e(a->lo, a->hi, 0xc265539eu, 0xddb3d742u, esi0, &t2);   /* t2 = {lo,hi,exp=r_exp} */
        /* 0x0484fc-03: ROL16;SI=0xbfff;ROL16 on r_exp (the multiply's output exp) */
        uint32_t esi2 = (0xbfffu << 16) | (t2.exp & 0xffffu);
        uint64_t t3 = fn_04794a(t2.lo, t2.hi, 0u, 0x80000000u, esi2);
        uint32_t t3_lo = (uint32_t)t3, t3_hi = (uint32_t)(t3 >> 32);
        uint32_t o_lo, o_hi; uint16_t o_exp;
        /* 0x04850c: EBX=esi0<<16 (a->exp<<16); 0x04850f: MOV BX,SI (low16=0xbfff) */
        fn_047b79(t3_lo, t3_hi, ((uint32_t)a->exp << 16) | 0xbfffu, t1_hi, t1_lo,
                  &o_lo, &o_hi, &o_exp);
        a->lo = o_lo; a->hi = o_hi; a->exp = o_exp;
        CH |= 1;
    }

    /* Poly-eval: fn_048f9e(a, 8, coeffs). Unpack 10-byte table -> 12-byte fx128. */
    fx128 tbl[9];
    for (int i = 0; i < 9; i++) {
        const uint8_t *p = g_coeff_48312 + i * 10;
        tbl[i].lo = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
        tbl[i].hi = (uint32_t)p[4] | (uint32_t)p[5] << 8 | (uint32_t)p[6] << 16 | (uint32_t)p[7] << 24;
        tbl[i].exp = (uint16_t)(p[8] | (p[9] << 8));
    }
    fn_048f9e(a, 8, tbl);

    /* Conditional scales (SHR CH,1; JNC): bit0=threshold, bit1=exp>=0x3fff. */
    if (CH & 1) fn_04791a(a, a, 0x6b9b2c23u, 0x860a91c1u, 0x3ffeu);   /* scale 1 */
    if (CH & 2) {
        fn_04791a(a, a, 0x2168c235u, 0xc90fdaa2u, 0xbfffu);           /* scale 2 */
        a->exp = (uint16_t)(a->exp ^ 0x8000);                          /* 0x048572: XOR [EBX+9],0x80 */
    }

    /* Restore original sign (0x048576-7b: XOR [EDI+9],0x80 if CL bit7) */
    if (CL & 0x80) a->exp = (uint16_t)(a->exp ^ 0x8000);
}

/* fn_04836c  orig: 0x0004836c (L4, 53 insns) -- fx128 binary op (a OP b) with
 *   zero/inf special cases + a sign-dependent post-scale.
 *   Register-ABI: EAX=a (fx128*), EDX=b (fx128*), EBX=out (fx128*). a->exp is the
 *   16-bit word at +8 (byte9 = sign, bit15); the byte9 "flag" reads are exp>>8.
 *   - a==0 (a->exp&0x7fff==0): out = b>=0 ? {0,0,0} : {0x2168c235,0xc90fdaa2,0x4000}
 *   - else b==0 (b->exp&0x7fff==0): out = {0x2168c235,0xc90fdaa2,(a->exp&0x8000)|0x3fff}
 *   - else: out = a OP b (fn_047b4f); fn_048422(out) normalizes; then
 *       (a negative && out flag>0) -> scale (0x2168c235,0xc90fdaa2,0xc000)
 *       (a non-neg  && out flag<0) -> scale (0x2168c235,0xc90fdaa2,0x4000).
 *   LATENT ORIG QUIRK: the original mangles EAX low16 via `MOV CX,[EDI+8]` before
 *   CALL 0x047b4f (which reads a from EAX), feeding a corrupted `a`; the C port
 *   passes the correct a pointer (the clear intent -- EDI is preserved). */
void fn_04836c(fx128 *a, const fx128 *b, fx128 *out)
{
    if ((a->exp & 0x7fff) == 0) {                      /* 0x048371-78: a is zero */
        if ((b->exp & 0x8000) == 0) {                  /* 0x04837a: b non-negative */
            out->exp = 0; out->hi = 0; out->lo = 0;
        } else {                                       /* b negative */
            out->exp = 0x4000; out->hi = 0xc90fdaa2; out->lo = 0x2168c235;
        }
        return;
    }
    if ((b->exp & 0x7fff) == 0) {                      /* 0x0483aa-b1: b is zero */
        out->exp = (uint16_t)((a->exp & 0x8000) | 0x3fff); out->hi = 0xc90fdaa2; out->lo = 0x2168c235;
        return;
    }
    int8_t ch = (int8_t)(a->exp >> 8);                 /* 0x0483d2: CH = a->flag (byte9) */
    fn_047b4f(a, b, out);                              /* 0x0483d5: out = a OP b */
    fn_048422(out);                                    /* 0x0483dc: normalize */
    int8_t dl = (int8_t)(out->exp >> 8);               /* 0x0483e1: DL = out->flag (byte9) */
    if (ch < 0) {                                      /* 0x0483e4: a negative */
        if (dl > 0)                                    /* 0x048406: out positive */
            fn_04791a(out, out, 0x2168c235u, 0xc90fdaa2u, 0xc000u);
    } else {                                           /* a non-negative */
        if (dl < 0)                                    /* 0x0483ea: out negative */
            fn_04791a(out, out, 0x2168c235u, 0xc90fdaa2u, 0x4000u);
    }
}

/* fn_047d4e  orig: 0x047d4e  (fx_mul64, 108 insns) — 64x64->128 fixed-point multiply
 * with 96-bit mantissa normalize + round-half + exponent re-bias.
 * Register-ABI: in {EAX=a_lo, EDX=a_hi, EBX=b_lo, ECX=b_hi, ESI=esi(exp field, 32-bit,
 * 16-bit halves)}; out {EAX=r_lo, EDX=r_hi, ESI=r_exp}. Bias 0xc002; overflow ->
 * 0x8000000000000000 (ESI=0x7fff); underflow -> 0.
 * Faithful asm-mirror (word-by-word 128-bit MAC preserved so the trailing SF/CF that
 * feed the JS at 0x047de4 and the round block match the disasm exactly).
 * See scratch/fp_047d4e_ref.py for the matching Python reference (196 differential vectors). */
static void fn_047d4e_epi(uint32_t ECX, uint32_t EAX, uint32_t EDX, fx64_t *out)
{
    uint32_t sum = ECX + ECX;
    uint32_t c = (sum < ECX) ? 1u : 0u;            /* 0x047e33 ADD ECX,ECX (CF) */
    ECX = sum;                                      /* ECX now holds the doubled value */
    uint32_t cx = ((((ECX & 0xffffu) >> 1) | (c << 15)) & 0xffffu); /* 0x047e35 RCR CX,1 (on doubled ECX) */
    ECX = (ECX & 0xffff0000u) | cx;
    out->lo = EAX; out->hi = EDX; out->exp = ECX;   /* 0x047e38 MOV ESI,ECX; RET */
}

void fn_047d4e(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi, uint32_t esi, fx64_t *out)
{
    uint32_t EAX=a_lo, EDX=a_hi, EBX=b_lo, ECX=b_hi, ESI=esi, EDI=0, EBP=0;
    uint32_t CF=0, ZF=0, SF=0;
    /* 0x047d4e OR EAX,EAX ; 0x047d50 JNZ */
    ZF = (EAX==0)?1u:0u;
    if (ZF) {
        ZF = (EDX==0)?1u:0u;                        /* 0x047d52 OR EDX,EDX ; 0x047d54 JNZ */
        if (ZF) {
            uint32_t si = ((ESI & 0xffffu) * 2u);   /* 0x047d56 ADD SI,SI */
            CF = (si > 0xffffu)?1u:0u; si &= 0xffffu;
            ESI = (ESI & 0xffff0000u) | si;
            ZF = (si==0)?1u:0u;
            if (ZF) { out->lo=0; out->hi=0; out->exp=ESI; return; } /* 0x047d5b RET */
            uint32_t old=si; si=((si>>1)|(CF<<15))&0xffffu; CF=old&1u; /* 0x047d5c RCR SI,1 */
            ESI=(ESI&0xffff0000u)|si;
        }
    }
    /* 0x047d5f OR ECX,ECX ; 0x047d61 JNZ */
    ZF = (ECX==0)?1u:0u;
    if (ZF) { EBX; ZF=(EBX==0)?1u:0u; }             /* 0x047d63 OR EBX ; 0x047d65 JNZ */
    if (ZF) { ZF = ((ESI & 0x7fff0000u)==0)?1u:0u; }/* 0x047d67 TEST ; 0x047d6d JNZ */
    if (ZF) { out->lo=0; out->hi=0; out->exp=0; return; } /* 0x047d6f */
    /* main 0x047d76: XCHG ESI,ECX */
    { uint32_t t=ESI; ESI=ECX; ECX=t; }
    EDI=ECX;                                        /* 0x047d7a MOV EDI,ECX */
    EDI=((EDI<<16)|(EDI>>16));                       /* 0x047d7c ROL EDI,16 */
    { uint32_t x=EDI>>16; EDI=(x&0x8000u)?(x-0x10000u):x; } /* 0x047d7f SAR EDI,16 (arith sign-extend) */
    { uint32_t x=ECX>>16; ECX=(x&0x8000u)?(x-0x10000u):x; } /* 0x047d82 SAR ECX,16 (arith sign-extend) */
    EDI &= 0x80007fffu; ECX &= 0x80007fffu;         /* 0x047d85/8b AND */
    { uint32_t r=ECX+EDI; CF=(r<ECX)?1u:0u; ECX=r; }/* 0x047d91 ADD ECX,EDI */
    { uint32_t lo=ECX&0xffffu; CF=(lo<0x3feu)?1u:0u; uint32_t cx=(lo-0x3feu)&0xffffu; ECX=(ECX&0xffff0000u)|cx; } /* 0x047d93 SUB CX,0x3fe */
    if (!CF) {                                       /* 0x047d98 JC -> underflow */
        CF = ((ECX&0xffffu) < 0x7fffu)?1u:0u; ZF=((ECX&0xffffu)==0x7fffu)?1u:0u; /* 0x047d9a CMP CX,0x7fff */
        if (!CF) { ECX=(ECX&0xffff0000u)|0x7fffu; EDX=0x80000000u; EAX=0; fn_047d4e_epi(ECX,EAX,EDX,out); return; } /* 0x047da1 JMP overflow */
    }
    { uint32_t cx=ECX&0xffffu; int32_t s=(cx&0x8000u)?(int32_t)cx-0x10000:(int32_t)cx; /* 0x047da6 CMP CX,-0x40 */
      if (s < -0x40) { out->lo=0; out->hi=0; out->exp=0; return; } }
    /* multiply 0x047db7 */
    uint32_t exp_adj_ecx=ECX; EBP=0;
    uint32_t push_esi=ESI, push_edx=EDX, push_eax=EAX;
    { uint64_t p=(uint64_t)EAX*EBX; EAX=(uint32_t)p; EDX=(uint32_t)(p>>32); } /* 0x047dbd MUL EBX */
    { uint32_t tt=EAX; EAX=ESI; ESI=tt; }           /* 0x047dbf XCHG EAX,ESI */
    ECX=EDX;                                        /* 0x047dc0 MOV ECX,EDX */
    EDX=push_eax;                                   /* 0x047dc2 POP EDX */
    { uint64_t p=(uint64_t)EAX*EDX; EAX=(uint32_t)p; EDX=(uint32_t)(p>>32); } /* 0x047dc3 MUL EDX */
    EDI=EDX;                                        /* 0x047dc5 MOV EDI,EDX */
    { uint32_t r=ECX+EAX; CF=(r<ECX)?1u:0u; ECX=r; }/* 0x047dc7 ADD ECX,EAX */
    { uint32_t r=EDI+EBP+CF; CF=(r<EDI)?1u:0u; EDI=r; } /* 0x047dc9 ADC EDI,EBP */
    { uint32_t r=EBP+EBP+CF; CF=(r<EBP)?1u:0u; EBP=r; } /* 0x047dcb ADC EBP,EBP */
    EAX=push_edx;                                   /* 0x047dcd POP EAX */
    { uint32_t tt=EAX; EAX=EBX; EBX=tt; }           /* 0x047dce XCHG EAX,EBX */
    { uint64_t p=(uint64_t)EAX*EBX; EAX=(uint32_t)p; EDX=(uint32_t)(p>>32); } /* 0x047dcf MUL EBX */
    { uint32_t r=ECX+EAX; CF=(r<ECX)?1u:0u; ECX=r; }/* 0x047dd1 ADD ECX,EAX */
    { uint32_t r=EDI+EDX+CF; CF=(r<EDI)?1u:0u; EDI=r; } /* 0x047dd3 ADC EDI,EDX */
    { uint32_t r=EBP+0u+CF; CF=(r<EBP)?1u:0u; EBP=r; } /* 0x047dd5 ADC EBP,0 */
    EAX=EBX;                                        /* 0x047dd8 MOV EAX,EBX */
    EDX=push_esi;                                   /* 0x047dda POP EDX */
    { uint64_t p=(uint64_t)EAX*EDX; EAX=(uint32_t)p; EDX=(uint32_t)(p>>32); } /* 0x047ddb MUL EDX */
    { uint32_t r=EAX+EDI; CF=(r<EAX)?1u:0u; EAX=r; }/* 0x047ddd ADD EAX,EDI */
    { uint32_t r=EDX+EBP+CF; CF=(r<EDX)?1u:0u; EDX=r; SF=(EDX&0x80000000u)?1u:0u; } /* 0x047ddf ADC EDX,EBP */
    EBX=ECX;                                        /* 0x047de1 MOV EBX,ECX */
    ECX=exp_adj_ecx;                                /* 0x047de3 POP ECX */
    /* round/normalize 0x047de4 JS */
    if (!SF) {
        { uint32_t r=EBX+EBX; CF=(r<EBX)?1u:0u; EBX=r; } /* 0x047de6 ADD EBX,EBX */
        { uint32_t r=EAX+EAX+CF; CF=(r<EAX)?1u:0u; EAX=r; } /* 0x047de8 ADC EAX,EAX */
        { uint32_t r=EDX+EDX+CF; CF=(r<EDX)?1u:0u; EDX=r; } /* 0x047dea ADC EDX,EDX */
        { uint32_t cx=((ECX&0xffffu)-1u)&0xffffu; ECX=(ECX&0xffff0000u)|cx; } /* 0x047dec DEC CX */
    }
    { uint32_t r=EBX+EBX; CF=(r<EBX)?1u:0u; EBX=r; ZF=(EBX==0)?1u:0u; } /* 0x047dee ADD EBX,EBX */
    if (CF) {                                        /* 0x047df0 JNC */
        if (ZF) {                                    /* 0x047df2 JNZ not taken (EBX==0) */
            ESI=ESI|ESI; ZF=(ESI==0)?1u:0u;          /* 0x047df4 OR ESI,ESI */
            /* 0x047df6 SETNZ BL (unused below) */
            CF=EBX&1u; EBX=(EBX>>1)&0xffffffffu;     /* 0x047df9 SHR EBX,1 */
            if (!CF) { ESI=(EAX>>1)&0xffffffffu; }   /* 0x047dfb JC; 0x047dfd/ff */
            { uint32_t r=EAX+0u+CF; CF=(r<EAX)?1u:0u; EAX=r; } /* 0x047e01 ADC EAX,0 */
            { uint32_t r=EDX+0u+CF; CF=(r<EDX)?1u:0u; EDX=r; } /* 0x047e04 ADC EDX,0 */
        } else {
            { uint32_t r=EAX+0u+CF; CF=(r<EAX)?1u:0u; EAX=r; } /* 0x047e01 ADC EAX,0 */
            { uint32_t r=EDX+0u+CF; CF=(r<EDX)?1u:0u; EDX=r; } /* 0x047e04 ADC EDX,0 */
        }
    }
    if (CF) {                                        /* 0x047e07 JNC not taken */
        { uint32_t old=EDX; EDX=((EDX>>1)|(CF<<31))&0xffffffffu; CF=old&1u; } /* 0x047e09 RCR EDX,1 */
        { uint32_t old=EAX; EAX=((EAX>>1)|(CF<<31))&0xffffffffu; CF=old&1u; } /* 0x047e0b RCR EAX,1 */
        { uint32_t old16=ECX&0xffffu; uint32_t cx=(old16+1u)&0xffffu; ECX=(ECX&0xffff0000u)|cx; } /* 0x047e0d INC CX */
        CF=((ECX&0xffffu)<0x7fffu)?1u:0u; ZF=((ECX&0xffffu)==0x7fffu)?1u:0u; /* 0x047e0f CMP CX,0x7fff */
        if (ZF) { ECX=(ECX&0xffff0000u)|0x7fffu; EDX=0x80000000u; EAX=0; fn_047d4e_epi(ECX,EAX,EDX,out); return; } /* 0x047e14 JZ overflow */
    }
    /* 0x047e1a OR CX,CX ; 0x047e1d JG (OF=0 for OR) */
    { uint32_t CX=ECX&0xffffu; ZF=(CX==0)?1u:0u; SF=(CX&0x8000u)?1u:0u;
      if (!(SF==0u && !ZF)) {                        /* JG not taken (CX<=0 signed) */
        if (!ZF) { CX=(-CX)&0xffffu; } else { CX=1u; } /* 0x047e1f JNZ; 0x047e25 NEG / 0x047e21 */
        EBX=0u;                                      /* 0x047e28 SUB EBX,EBX */
        uint32_t CL=CX&31u;                          /* x86 masks SHRD count to 5 bits */
        if (CL) { EAX=((EAX>>CL)|(EDX<<(32-CL)))&0xffffffffu; EDX=(EDX>>CL)&0xffffffffu; } /* 0x047e2a/2d SHRD */
        CX=0u; ECX=(ECX&0xffff0000u)|CX;             /* 0x047e30 SUB CX,CX */
      } }
    fn_047d4e_epi(ECX,EAX,EDX,out);
}

/* fn_047b79  orig: 0x00047b79 (L1, 177 insns)
 *   Pure-integer fixed-point binary op (multiply/divide with mantissa
 *   normalization and long-division remainder loops). Register-ABI (from the
 *   thin wrapper 0x047b4f): in EAX=a0, EDX=a1, ESI=(b2<<16|a2), ECX=b1,
 *   EBX=b0 ; out EAX=f0, EDX=f1, SI(=ESI&0xffff)=f2. Uses a stack frame
 *   (PUSH EBP; MOV EBP,ESP) with [EBP-0xc..0x1c] dword locals that hold the
 *   pushed intermediate quotients (all written by PUSH before any read).
 *   The two CALL 0x0463dc dispatches are net no-ops (0x0463dc copies a 27-dword
 *   state buffer through the no-op primitive 0x042a07; EAX is reset right
 *   after each) and are omitted -- same treatment as fn_048053/fn_047f54.
 *   Disasm-faithful instruction translation (generated by scratch/gen_047b79_c.py
 *   from the Ghidra disasm, mirroring scratch/fp_047b79_ref.py). Differential
 *   vs the emulator: 20,000 random vectors, 0 mismatch. DIV #DE (overflow) can
 *   fire on malformed non-normalized inputs, but the game always feeds
 *   normalized fixed-point values so it is unreachable. */
/* Disasm-faithful C translation of FUN_00047b79 (pure-integer fixed-point
 * binary op), generated by gen_047b79_c.py from the Ghidra 177-instruction disasm,
 * mirroring the flag/register/stack semantics of fp_047b79_ref.py. Verified by
 * differential testing. Two CALL 0x463dc dispatches are net no-ops (handler
 * 0x042a07) and are omitted. In: EAX,EDX,ESI,ECX,EBX ; out: EAX,EDX,SI. */
void fn_047b79(uint32_t eax, uint32_t edx, uint32_t esi, uint32_t ecx, uint32_t ebx,
               uint32_t *o_eax, uint32_t *o_edx, uint16_t *o_si)
{
    uint32_t ebp=0, edi=0;
    int cf=0, zf=0, sf=0, of=0;
    uint32_t stk[128]; unsigned esp=96, ebsp=0;
    (void)of;
    goto l_47b79;
    l_47b79: { /* OR ECX,ECX */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(ecx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        ecx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47b7b: { /* JNZ 0x47bc4 */
        if(zf==0) goto l_47bc4;
    }
    l_47b7d: { /* OR EBX,EBX */
        { uint32_t _d=(ebx)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        ebx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47b7f: { /* JNZ 0x47bc4 */
        if(zf==0) goto l_47bc4;
    }
    l_47b81: { /* TEST ESI,0x7fff0000 */
        { uint32_t _r=(((esi)&(((uint32_t)(0x7fff0000))))&4294967295); cf=0; of=0; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47b87: { /* JNZ 0x47bc4 */
        if(zf==0) goto l_47bc4;
    }
    l_47b89: { /* OR EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47b8b: { /* JNZ 0x47bae */
        if(zf==0) goto l_47bae;
    }
    l_47b8d: { /* OR EDX,EDX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47b8f: { /* JNZ 0x47bae */
        if(zf==0) goto l_47bae;
    }
    l_47b91: { /* MOV EAX,ESI */
        eax=((uint32_t)(esi))&0xffffffffu;
    }
    l_47b93: { /* ADD AX,AX */
        { uint32_t _d=(eax)&65535; uint32_t _s=(eax)&65535; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)65535)?1:0; _r=(uint32_t)(_t64&65535); of=(((~(_d^_s))&(_d^_r))>>15)&1u;
        eax=(eax&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47b96: { /* JNZ 0x47bae */
        if(zf==0) goto l_47bae;
    }
    l_47b98: { /* MOV AL,0x1 */
        eax=(eax&0xffffff00u)|((((uint32_t)(((uint32_t)(0x1))))&0xffu));
    }
    l_47b9a: { /* MOV AH,0x81 */
        eax=(eax&0xffff00ffu)|((((uint32_t)(((uint32_t)(0x81))))&0xffu)<<8);
    }
    l_47ba1: { /* MOV EDX,0xc0000000 */
        edx=((uint32_t)(((uint32_t)(0xc0000000))))&0xffffffffu;
    }
    l_47ba6: { /* SUB EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ba8: { /* MOV SI,0xffff */
        esi=(esi&0xffff0000u)|((((uint32_t)(((uint32_t)(0xffff))))&0xffffu));
    }
    l_47bac: { /* JMP 0x47bc3 */
        goto l_47bc3;
    }
    l_47bae: { /* MOV AL,0x4 */
        eax=(eax&0xffffff00u)|((((uint32_t)(((uint32_t)(0x4))))&0xffu));
    }
    l_47bb0: { /* MOV AH,0x83 */
        eax=(eax&0xffff00ffu)|((((uint32_t)(((uint32_t)(0x83))))&0xffu)<<8);
    }
    l_47bb7: { /* MOV EDX,0x80000000 */
        edx=((uint32_t)(((uint32_t)(0x80000000))))&0xffffffffu;
    }
    l_47bbc: { /* SUB EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bbe: { /* OR SI,0x7fff */
        { uint32_t _d=(esi)&65535; uint32_t _s=(((uint32_t)(0x7fff)))&65535; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&65535;
        esi=(esi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47bc3: { /* RET */
        *o_eax=eax; *o_edx=edx; *o_si=(uint16_t)esi; return;
    }
    l_47bc4: { /* OR EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bc6: { /* JNZ 0x47bd5 */
        if(zf==0) goto l_47bd5;
    }
    l_47bc8: { /* OR EDX,EDX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bca: { /* JNZ 0x47bd5 */
        if(zf==0) goto l_47bd5;
    }
    l_47bcc: { /* ADD SI,SI */
        { uint32_t _d=(esi)&65535; uint32_t _s=(esi)&65535; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)65535)?1:0; _r=(uint32_t)(_t64&65535); of=(((~(_d^_s))&(_d^_r))>>15)&1u;
        esi=(esi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47bcf: { /* JNZ 0x47bd2 */
        if(zf==0) goto l_47bd2;
    }
    l_47bd1: { /* RET */
        *o_eax=eax; *o_edx=edx; *o_si=(uint16_t)esi; return;
    }
    l_47bd2: { /* RCR SI,0x1 */
        { uint32_t _d=(esi)&65535; uint32_t _c=(((uint32_t)(0x1)))&31u; int _i; uint32_t _b0; for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<15))&65535); cf=_b0?1:0; } esi=(esi&0xffff0000u)|((((uint32_t)(_d))&0xffffu)); }
    }
    l_47bd5: { /* PUSH EBP */
        esp--; stk[esp]=(uint32_t)(ebp);
    }
    l_47bd6: { /* MOV EBP,ESP */
        ebsp=esp; ebp=esp;
    }
    l_47bd8: { /* PUSH EDI */
        esp--; stk[esp]=(uint32_t)(edi);
    }
    l_47bd9: { /* XCHG ESI,ECX */
        { uint32_t _t=esi; esi=((uint32_t)(ecx))&0xffffffffu; ecx=_t; }
    }
    l_47bdb: { /* MOV EDI,ECX */
        edi=((uint32_t)(ecx))&0xffffffffu;
    }
    l_47bdd: { /* ROL EDI,0x10 */
        { uint32_t _d=(edi)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(32-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(32-_c)))&4294967295; edi=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47be0: { /* SAR EDI,0x10 */
        { uint32_t _d=(edi)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(_c-1u))&1u):0u; _r=(uint32_t)((int32_t)_d >> _c); edi=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47be3: { /* SAR ECX,0x10 */
        { uint32_t _d=(ecx)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(_c-1u))&1u):0u; _r=(uint32_t)((int32_t)_d >> _c); ecx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47be6: { /* AND EDI,0x80007fff */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(((uint32_t)(0x80007fff)))&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d&_s)&4294967295;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bec: { /* AND ECX,0x80007fff */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(((uint32_t)(0x80007fff)))&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d&_s)&4294967295;
        ecx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bf2: { /* ROL EDI,0x10 */
        { uint32_t _d=(edi)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(32-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(32-_c)))&4294967295; edi=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bf5: { /* ROL ECX,0x10 */
        { uint32_t _d=(ecx)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(32-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(32-_c)))&4294967295; ecx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bf8: { /* ADD DI,CX */
        { uint32_t _d=(edi)&65535; uint32_t _s=(ecx)&65535; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)65535)?1:0; _r=(uint32_t)(_t64&65535); of=(((~(_d^_s))&(_d^_r))>>15)&1u;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47bfb: { /* ROL EDI,0x10 */
        { uint32_t _d=(edi)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(32-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(32-_c)))&4294967295; edi=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47bfe: { /* ROL ECX,0x10 */
        { uint32_t _d=(ecx)&4294967295; uint32_t _c=(((uint32_t)(0x10)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(32-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(32-_c)))&4294967295; ecx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c01: { /* OR DI,DI */
        { uint32_t _d=(edi)&65535; uint32_t _s=(edi)&65535; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&65535;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c04: { /* JNZ 0x47c10 */
        if(zf==0) goto l_47c10;
    }
    l_47c06: { /* ADD EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c08: { /* ADC EDX,EDX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c0a: { /* DEC DI */
        { uint32_t _d=(edi)&65535; of=0; uint32_t _r=(_d - 1u)&65535; edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu)); zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c0c: { /* OR EDX,EDX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c0e: { /* JNS 0x47c06 */
        if(sf==0) goto l_47c06;
    }
    l_47c10: { /* OR CX,CX */
        { uint32_t _d=(ecx)&65535; uint32_t _s=(ecx)&65535; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&65535;
        ecx=(ecx&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c13: { /* JNZ 0x47c1f */
        if(zf==0) goto l_47c1f;
    }
    l_47c15: { /* ADD EBX,EBX */
        { uint32_t _d=(ebx)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        ebx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c17: { /* ADC ESI,ESI */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(esi)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c19: { /* DEC CX */
        { uint32_t _d=(ecx)&65535; of=0; uint32_t _r=(_d - 1u)&65535; ecx=(ecx&0xffff0000u)|((((uint32_t)(_r))&0xffffu)); zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c1b: { /* OR ESI,ESI */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(esi)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c1d: { /* JNS 0x47c15 */
        if(sf==0) goto l_47c15;
    }
    l_47c1f: { /* SUB DI,CX */
        { uint32_t _d=(edi)&65535; uint32_t _s=(ecx)&65535; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&65535; of=(((_d^_s)&~(_d^_r))>>15)&1u;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c22: { /* ADD DI,0x3fff */
        { uint32_t _d=(edi)&65535; uint32_t _s=(((uint32_t)(0x3fff)))&65535; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)65535)?1:0; _r=(uint32_t)(_t64&65535); of=(((~(_d^_s))&(_d^_r))>>15)&1u;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c27: { /* JS 0x47c42 */
        if(sf==1) goto l_47c42;
    }
    l_47c29: { /* CMP DI,0x7fff */
        { uint32_t _d=(edi)&65535; uint32_t _s=(((uint32_t)(0x7fff)))&65535; uint32_t _r=(_d-_s)&65535; cf=(_d<_s)?1:0; of=(((_d^_s)&~(_d^_r))>>15)&1u; zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c2e: { /* JC 0x47c42 */
        if(cf==1) goto l_47c42;
    }
    l_47c30: { /* MOV EDI,ECX */
        edi=((uint32_t)(ecx))&0xffffffffu;
    }
    l_47c32: { /* MOV DI,0x7fff */
        edi=(edi&0xffff0000u)|((((uint32_t)(((uint32_t)(0x7fff))))&0xffffu));
    }
    l_47c36: { /* MOV EDX,0x80000000 */
        edx=((uint32_t)(((uint32_t)(0x80000000))))&0xffffffffu;
    }
    l_47c3b: { /* SUB EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c3d: { /* JMP 0x47d1a */
        goto l_47d1a;
    }
    l_47c42: { /* CMP DI,-0x40 */
        { uint32_t _d=(edi)&65535; uint32_t _s=(((uint32_t)(-0x40)))&65535; uint32_t _r=(_d-_s)&65535; cf=(_d<_s)?1:0; of=(((_d^_s)&~(_d^_r))>>15)&1u; zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c46: { /* JGE 0x47c53 */
        if((sf==of)) goto l_47c53;
    }
    l_47c48: { /* SUB EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c4a: { /* SUB EDX,EDX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c4c: { /* SUB EDI,EDI */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(edi)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c4e: { /* JMP 0x47d1a */
        goto l_47d1a;
    }
    l_47c53: { /* PUSH EDI */
        esp--; stk[esp]=(uint32_t)(edi);
    }
    l_47c54: { /* PUSH ESI */
        esp--; stk[esp]=(uint32_t)(esi);
    }
    l_47c55: { /* PUSH EBX */
        esp--; stk[esp]=(uint32_t)(ebx);
    }
    l_47c56: { /* MOV ECX,ESI */
        ecx=((uint32_t)(esi))&0xffffffffu;
    }
    l_47c58: { /* MOV EDI,EDX */
        edi=((uint32_t)(edx))&0xffffffffu;
    }
    l_47c5a: { /* MOV ESI,EAX */
        esi=((uint32_t)(eax))&0xffffffffu;
    }
    l_47c5c: { /* SUB EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c5e: { /* CMP ECX,EDI */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(edi)&4294967295; uint32_t _r=(_d-_s)&4294967295; cf=(_d<_s)?1:0; of=(((_d^_s)&~(_d^_r))>>31)&1u; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c60: { /* JA 0x47c65 */
        if((cf==0 && zf==0)) goto l_47c65;
    }
    l_47c62: { /* SUB EDX,ECX */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(ecx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c64: { /* INC EAX */
        { uint32_t _d=(eax)&4294967295; of=0; uint32_t _r=(_d + 1u)&4294967295; eax=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c65: { /* PUSH EAX */
        esp--; stk[esp]=(uint32_t)(eax);
    }
    l_47c66: { /* MOV EAX,ESI */
        eax=((uint32_t)(esi))&0xffffffffu;
    }
    l_47c68: { /* DIV ECX */
        { uint32_t _dv=(ecx)&0xffffffffu; uint64_t _div=((uint64_t)edx<<32)|(uint64_t)eax; eax=(uint32_t)(_div/_dv); edx=(uint32_t)(_div%_dv); }
    }
    l_47c6a: { /* PUSH EAX */
        esp--; stk[esp]=(uint32_t)(eax);
    }
    l_47c6b: { /* XCHG EAX,EBX */
        { uint32_t _t=eax; eax=((uint32_t)(ebx))&0xffffffffu; ebx=_t; }
    }
    l_47c6c: { /* MUL EBX */
        { uint64_t _p=((uint64_t)eax*((uint32_t)(ebx)&0xffffffffu)); edx=(uint32_t)(_p>>32); eax=(uint32_t)_p; cf=of=(edx!=0)?1:0; }
    }
    l_47c6e: { /* XCHG EAX,ECX */
        { uint32_t _t=eax; eax=((uint32_t)(ecx))&0xffffffffu; ecx=_t; }
    }
    l_47c6f: { /* XCHG EDX,EBX */
        { uint32_t _t=edx; edx=((uint32_t)(ebx))&0xffffffffu; ebx=_t; }
    }
    l_47c71: { /* MUL EDX */
        { uint64_t _p=((uint64_t)eax*((uint32_t)(edx)&0xffffffffu)); edx=(uint32_t)(_p>>32); eax=(uint32_t)_p; cf=of=(edx!=0)?1:0; }
    }
    l_47c73: { /* ADD EAX,EBX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c75: { /* ADC EDX,0x0 */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(((uint32_t)(0x0)))&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c78: { /* MOV EBX,[EBP-0x10] */
        ebx=((uint32_t)(stk[ebsp + -4]))&0xffffffffu;
    }
    l_47c7b: { /* TEST [EBP-0x14],0x1 */
        { uint32_t _r=(((stk[ebsp + -5])&(((uint32_t)(0x1))))&65535); cf=0; of=0; zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47c7f: { /* JZ 0x47c86 */
        if(zf==1) goto l_47c86;
    }
    l_47c81: { /* ADD EAX,EBX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c83: { /* ADC EDX,[EBP-0xc] */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(stk[ebsp + -3])&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c86: { /* NEG ECX */
        { uint32_t _d=(ecx)&4294967295; cf=(_d!=0)?1:0; of=0; uint32_t _r=(0u-_d)&4294967295; ecx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c88: { /* SBB ESI,EAX */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c8a: { /* SBB EDI,EDX */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c8c: { /* JZ 0x47ca0 */
        if(zf==1) goto l_47ca0;
    }
    l_47c8e: { /* SUB [EBP-0x18],0x1 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -6]); uint32_t _s=(((uint32_t)(0x1))); uint32_t _r; cf=(_d<_s)?1:0; _r=(_d-_s); of=0; stk[ebsp + -6]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c92: { /* SBB [EBP-0x14],0x0 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -5]); uint32_t _s=(((uint32_t)(0x0))); uint32_t _r; _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0; stk[ebsp + -5]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c96: { /* ADD ECX,EBX */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        ecx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c98: { /* ADC ESI,[EBP-0xc] */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(stk[ebsp + -3])&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c9b: { /* ADC EDI,0x0 */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(((uint32_t)(0x0)))&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47c9e: { /* JNZ 0x47c8e */
        if(zf==0) goto l_47c8e;
    }
    l_47ca0: { /* MOV EDI,ESI */
        edi=((uint32_t)(esi))&0xffffffffu;
    }
    l_47ca2: { /* MOV ESI,ECX */
        esi=((uint32_t)(ecx))&0xffffffffu;
    }
    l_47ca4: { /* MOV ECX,[EBP-0xc] */
        ecx=((uint32_t)(stk[ebsp + -3]))&0xffffffffu;
    }
    l_47ca7: { /* CMP ECX,EDI */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(edi)&4294967295; uint32_t _r=(_d-_s)&4294967295; cf=(_d<_s)?1:0; of=(((_d^_s)&~(_d^_r))>>31)&1u; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ca9: { /* JA 0x47cb5 */
        if((cf==0 && zf==0)) goto l_47cb5;
    }
    l_47cab: { /* SUB EDI,ECX */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(ecx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cad: { /* ADD [EBP-0x18],0x1 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -6]); uint32_t _s=(((uint32_t)(0x1))); uint32_t _r; _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>0x100000000u)?1:0; _r=(uint32_t)_t64; of=0; stk[ebsp + -6]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cb1: { /* ADC [EBP-0x14],0x0 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -5]); uint32_t _s=(((uint32_t)(0x0))); uint32_t _r; _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>0x100000000u)?1:0; _r=(uint32_t)_t64; of=0; stk[ebsp + -5]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cb5: { /* MOV EDX,EDI */
        edx=((uint32_t)(edi))&0xffffffffu;
    }
    l_47cb7: { /* MOV EAX,ESI */
        eax=((uint32_t)(esi))&0xffffffffu;
    }
    l_47cb9: { /* DIV ECX */
        { uint32_t _dv=(ecx)&0xffffffffu; uint64_t _div=((uint64_t)edx<<32)|(uint64_t)eax; eax=(uint32_t)(_div/_dv); edx=(uint32_t)(_div%_dv); }
    }
    l_47cbb: { /* PUSH EAX */
        esp--; stk[esp]=(uint32_t)(eax);
    }
    l_47cbc: { /* OR EAX,EAX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&4294967295;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cbe: { /* JZ 0x47cec */
        if(zf==1) goto l_47cec;
    }
    l_47cc0: { /* XCHG EAX,EBX */
        { uint32_t _t=eax; eax=((uint32_t)(ebx))&0xffffffffu; ebx=_t; }
    }
    l_47cc1: { /* MUL EBX */
        { uint64_t _p=((uint64_t)eax*((uint32_t)(ebx)&0xffffffffu)); edx=(uint32_t)(_p>>32); eax=(uint32_t)_p; cf=of=(edx!=0)?1:0; }
    }
    l_47cc3: { /* XCHG EAX,ECX */
        { uint32_t _t=eax; eax=((uint32_t)(ecx))&0xffffffffu; ecx=_t; }
    }
    l_47cc4: { /* XCHG EDX,EBX */
        { uint32_t _t=edx; edx=((uint32_t)(ebx))&0xffffffffu; ebx=_t; }
    }
    l_47cc6: { /* MUL EDX */
        { uint64_t _p=((uint64_t)eax*((uint32_t)(edx)&0xffffffffu)); edx=(uint32_t)(_p>>32); eax=(uint32_t)_p; cf=of=(edx!=0)?1:0; }
    }
    l_47cc8: { /* ADD EAX,EBX */
        { uint32_t _d=(eax)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        eax=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cca: { /* ADC EDX,0x0 */
        { uint32_t _d=(edx)&4294967295; uint32_t _s=(((uint32_t)(0x0)))&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ccd: { /* NEG ECX */
        { uint32_t _d=(ecx)&4294967295; cf=(_d!=0)?1:0; of=0; uint32_t _r=(0u-_d)&4294967295; ecx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ccf: { /* SBB ESI,EAX */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(eax)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cd1: { /* SBB EDI,EDX */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(edx)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cd3: { /* JZ 0x47cec */
        if(zf==1) goto l_47cec;
    }
    l_47cd5: { /* SUB [EBP-0x1c],0x1 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -7]); uint32_t _s=(((uint32_t)(0x1))); uint32_t _r; cf=(_d<_s)?1:0; _r=(_d-_s); of=0; stk[ebsp + -7]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cd9: { /* SBB [EBP-0x18],0x0 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -6]); uint32_t _s=(((uint32_t)(0x0))); uint32_t _r; _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0; stk[ebsp + -6]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cdd: { /* SBB [EBP-0x14],0x0 */
        { uint64_t _t64; uint32_t _d=(stk[ebsp + -5]); uint32_t _s=(((uint32_t)(0x0))); uint32_t _r; _t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0; stk[ebsp + -5]=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ce1: { /* ADD ECX,[EBP-0x10] */
        { uint32_t _d=(ecx)&4294967295; uint32_t _s=(stk[ebsp + -4])&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        ecx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ce4: { /* ADC ESI,[EBP-0xc] */
        { uint32_t _d=(esi)&4294967295; uint32_t _s=(stk[ebsp + -3])&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        esi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47ce7: { /* ADC EDI,0x0 */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(((uint32_t)(0x0)))&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_s^_r))&(_d^_r))>>31)&1u;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cea: { /* JNZ 0x47cd5 */
        if(zf==0) goto l_47cd5;
    }
    l_47cec: { /* POP EAX */
        eax=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47ced: { /* POP EDX */
        edx=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47cee: { /* POP EBX */
        ebx=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47cef: { /* ADD ESP,0x8 */
        esp += 2;
    }
    l_47cf2: { /* POP EDI */
        edi=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47cf3: { /* DEC DI */
        { uint32_t _d=(edi)&65535; of=0; uint32_t _r=(_d - 1u)&65535; edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu)); zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47cf5: { /* SHR EBX,0x1 */
        { uint32_t _d=(ebx)&4294967295; uint32_t _c=(((uint32_t)(0x1)))&31u; of=0; uint32_t _r; cf=_c?((_d>>(_c-1u))&1u):0u; _r=(_d>>_c)&4294967295; ebx=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cf7: { /* JNC 0x47cfe */
        if(cf==0) goto l_47cfe;
    }
    l_47cf9: { /* RCR EDX,0x1 */
        { uint32_t _d=(edx)&4294967295; uint32_t _c=(((uint32_t)(0x1)))&31u; int _i; uint32_t _b0; for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<31))&4294967295); cf=_b0?1:0; } edx=((uint32_t)(_d))&0xffffffffu; }
    }
    l_47cfb: { /* RCR EAX,0x1 */
        { uint32_t _d=(eax)&4294967295; uint32_t _c=(((uint32_t)(0x1)))&31u; int _i; uint32_t _b0; for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<31))&4294967295); cf=_b0?1:0; } eax=((uint32_t)(_d))&0xffffffffu; }
    }
    l_47cfd: { /* INC EDI */
        { uint32_t _d=(edi)&4294967295; of=0; uint32_t _r=(_d + 1u)&4294967295; edi=((uint32_t)(_r))&0xffffffffu; zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47cfe: { /* OR DI,DI */
        { uint32_t _d=(edi)&65535; uint32_t _s=(edi)&65535; uint64_t _t64; uint32_t _r;
        cf=0; of=0; _r=(_d|_s)&65535;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47d01: { /* JG 0x47d1a */
        if((sf==of && zf==0)) goto l_47d1a;
    }
    l_47d03: { /* JNZ 0x47d09 */
        if(zf==0) goto l_47d09;
    }
    l_47d05: { /* MOV CL,0x1 */
        ecx=(ecx&0xffffff00u)|((((uint32_t)(((uint32_t)(0x1))))&0xffu));
    }
    l_47d07: { /* JMP 0x47d0f */
        goto l_47d0f;
    }
    l_47d09: { /* NEG DI */
        { uint32_t _d=(edi)&65535; cf=(_d!=0)?1:0; of=0; uint32_t _r=(0u-_d)&65535; edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu)); zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47d0c: { /* MOV CX,DI */
        ecx=(ecx&0xffff0000u)|((((uint32_t)(edi))&0xffffu));
    }
    l_47d0f: { /* SUB EBX,EBX */
        { uint32_t _d=(ebx)&4294967295; uint32_t _s=(ebx)&4294967295; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&4294967295; of=(((_d^_s)&~(_d^_r))>>31)&1u;
        ebx=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47d11: { /* SHRD EAX,EDX,CL */
        { uint32_t _d=eax; uint32_t _s=edx; uint32_t _c=(ecx)&31u; uint64_t _v; of=0; cf=_c?((_d>>(_c-1u))&1u):0u; _v=((uint64_t)_d<<32)|(uint32_t)_s; _v>>=_c; eax=((uint32_t)(((uint32_t)(_v>>32))))&0xffffffffu; edx=((uint32_t)(((uint32_t)_v)))&0xffffffffu; }
    }
    l_47d14: { /* SHRD EDX,EBX,CL */
        { uint32_t _d=edx; uint32_t _s=ebx; uint32_t _c=(ecx)&31u; uint64_t _v; of=0; cf=_c?((_d>>(_c-1u))&1u):0u; _v=((uint64_t)_d<<32)|(uint32_t)_s; _v>>=_c; edx=((uint32_t)(((uint32_t)(_v>>32))))&0xffffffffu; ebx=((uint32_t)(((uint32_t)_v)))&0xffffffffu; }
    }
    l_47d17: { /* SUB DI,DI */
        { uint32_t _d=(edi)&65535; uint32_t _s=(edi)&65535; uint64_t _t64; uint32_t _r;
        cf=(_d<_s)?1:0; _r=(_d-_s)&65535; of=(((_d^_s)&~(_d^_r))>>15)&1u;
        edi=(edi&0xffff0000u)|((((uint32_t)(_r))&0xffffu));
        zf=(_r==0)?1:0; sf=(_r>>15)&1; }
    }
    l_47d1a: { /* ADD EDI,EDI */
        { uint32_t _d=(edi)&4294967295; uint32_t _s=(edi)&4294967295; uint64_t _t64; uint32_t _r;
        _t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)4294967295)?1:0; _r=(uint32_t)(_t64&4294967295); of=(((~(_d^_s))&(_d^_r))>>31)&1u;
        edi=((uint32_t)(_r))&0xffffffffu;
        zf=(_r==0)?1:0; sf=(_r>>31)&1; }
    }
    l_47d1c: { /* RCR DI,0x1 */
        { uint32_t _d=(edi)&65535; uint32_t _c=(((uint32_t)(0x1)))&31u; int _i; uint32_t _b0; for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<15))&65535); cf=_b0?1:0; } edi=(edi&0xffff0000u)|((((uint32_t)(_d))&0xffffu)); }
    }
    l_47d1f: { /* MOV ESI,EDI */
        esi=((uint32_t)(edi))&0xffffffffu;
    }
    l_47d21: { /* POP EDI */
        edi=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47d22: { /* POP EBP */
        ebp=((uint32_t)(stk[esp++]))&0xffffffffu;
    }
    l_47d23: { /* RET */
        *o_eax=eax; *o_edx=edx; *o_si=(uint16_t)esi; return;
    }
    return;
}


/* fn_047b4f  orig: 0x00047b4f (L1, 18 insns)
 *   Thin register-ABI wrapper over fn_047b79 (the fixed-point binary op).
 *   a (EAX) and b (EDX) are fx128 { lo u32@0, hi u32@4, exp u16@8 }; the
 *   result is written to the struct pointed to by EBX (out):  out = a <op> b.
 *   Packs the 0x047b79 register-ABI exactly:
 *       ESI = (b->exp << 16) | a->exp   (SHL ESI,0x10 on b->exp, then MOV SI,a->exp)
 *       EAX = a->lo, EDX = a->hi, ECX = b->hi, EBX = b->lo
 *   EBX is PUSHed on entry and reused as b->lo across the CALL, then POPped to
 *   recover the out pointer for the write-back (out->lo, out->hi, out->exp-word). */
void fn_047b4f(fx128 *a, const fx128 *b, fx128 *out)
{
    uint32_t esi = ((uint32_t)b->exp << 16) | (uint32_t)a->exp;   /* 0x047b5b/0x047b5e */
    uint32_t r_lo, r_hi; uint16_t r_exp;
    fn_047b79(a->lo, a->hi, esi, b->hi, b->lo, &r_lo, &r_hi, &r_exp);  /* 0x047b67 */
    out->lo = r_lo; out->hi = r_hi; out->exp = r_exp;                 /* 0x047b6d..0x047b72 */
}

/* fn_045bb7  orig: 0x00045bb7 (L1, 71 insns) — DOS far-memory allocate/resize/verify manager.
 *   Register-ABI: EAX = size (primary arg), EDI = secondary register arg; returns
 *   int32 (a handle, or -1 on failure). Reads DAT_53398 (size threshold),
 *   DAT_533b2/b3 (flags), DAT_53384 (stored handle).
 *   FLAT MODEL: no DOS far memory, so every INT 21 (AH=0xED query, AH=0x4A
 *   allocate/resize) fails. Control-flow: the threshold check (EAX < DAT_53398)
 *   jumps straight to the fail path; otherwise both flag sub-paths converge on the
 *   terminal INT 21 AH=0x4A (0x45c14), which fails (JGE 0x45c61 not taken) and falls
 *   to the fail path (0x45c1e). So EVERY execution reaches the fail path:
 *       r = const_64008();  *r = 5;  return -1.
 *   In the flat port *0x64008 is the global g_dat_64008. The register args do not
 *   affect the flat-model outcome (all DOS ops fail). */
int32_t fn_045bb7(uint32_t size)
{
    (void)size;              /* EAX: only gates the threshold branch, which also lands on the fail path */
    g_dat_64008 = 5;         /* 0x45c23 MOV [EAX],0x5 with EAX = const_64008() = 0x64008 */
    return -1;               /* 0x45c29 MOV EAX,0xffffffff */
}

/* fn_04a814  orig: 0x0004a814 (L1, 57 insns) -- linear UPPER-BOUND scan over the
 * 8-byte {hi,lo} powers-of-10 table at 0x04a928 (a decimal-magnitude lookup used by
 * the FP cluster FUN_0004a367/0x04a3a8/0x04a84e).
 * Register-ABI: EAX = start index (int32), EDX = pointer to a 64-bit key {lo,hi};
 * returns the int32 counter (EAX). The CALL 0x04a923 (fx_stub_4a923, a no-op) sets
 * EDI to the table base 0x04a928 and leaves EAX unchanged; INC EAX; EDI += EAX*8
 * starts the scan at entry (start+1). Forward: advance while key >= entry; backward:
 * step back while entry > key. "key < entry" (64-bit) = (hi<ehi)|| (hi==ehi && lo<elo).
 * Table (bit-exact from 0x04a928..0x04a9d0): [0]=0, [1..20]=10^0..10^19,
 * [21]=UINT64_MAX (the binary's own sentinel at 0x04a9d0) -- OOB-safe. */
static const uint64_t g_pow10_tbl_04a928[22] = {
    0ULL,
    1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL,
    10000000ULL, 100000000ULL, 1000000000ULL, 10000000000ULL,
    100000000000ULL, 1000000000000ULL, 10000000000000ULL, 100000000000000ULL,
    1000000000000000ULL, 10000000000000000ULL, 100000000000000000ULL,
    1000000000000000000ULL, 10000000000000000000ULL,
    UINT64_MAX
};

static int key_lt_entry_814(uint32_t hi, uint32_t lo, uint64_t ent)
{
    uint32_t ehi = (uint32_t)(ent >> 32);
    uint32_t elo = (uint32_t)ent;
    return (hi < ehi) || (hi == ehi && lo < elo);
}

int32_t fn_04a814(int32_t start, const uint32_t *key)
{
    uint32_t lo = key[0];                              /* [EDX] */
    uint32_t hi = key[1];                              /* [EDX+4] */
    int e = (int)((start + 1) & 0xffffffffu);          /* EDI = base + (start+1)*8 */
    int cnt = 0;
    for (;;) {                                         /* forward: key >= entry */
        if (key_lt_entry_814(hi, lo, g_pow10_tbl_04a928[e]))
            break;
        e++;
        cnt++;
    }
    for (;;) {                                         /* backward: entry > key */
        e--;
        if (!key_lt_entry_814(hi, lo, g_pow10_tbl_04a928[e]))
            break;
        e--;
        cnt--;
    }
    return cnt;
}

/* ---- 0x4a367: FP table-lookup cluster caller (64 B, 31 insns) ----
 * Register+stack ABI (resolved from the sole caller FUN_0004a48f @0x04a59b):
 *   arg0  = [EBP+8]   low 32 bits of the base double
 *   arg1  = [EBP+0xc] high 32 bits of the base double
 *   arg2  = [EBP+0x10] target index (int, from the caller's clamped EBX)
 *   arg3  = [EBP+0x14] ptr to a 64-bit {lo,hi} (mantissa/key source)
 *   EAX-in (implicit register input) = the caller's [ECX]
 *   returns int32 (the 0x04a814 scan count).
 *
 * Body: base = arg0 | arg1<<32 (double); if (EAX-in != arg2) base =
 *   fn_04a3a8(base, arg2 - (EAX-in));  FSTP base -> [EBP+8];
 *   fx_mantissa_scale(out=&[EBP+8], in=arg3);  return fn_04a814(arg2, arg3).
 *
 * DEAD-STORE ANALYSIS (verified vs FUN_0004a48f): arg0/arg1 are pushed BY VALUE
 * (copies of the caller's [EBP+8]/[EBP+0xc]); the FPU block writes only to those
 * pushed copies (0x04a367's [EBP+8]/[EBP+0xc] slots), which are popped by RET 0x10
 * and never read back by the caller. arg3 is the read-only `in` source (fx_mantissa_scale
 * writes to out=&[EBP+8], not to *arg3). Hence the FPU block (0x04a3a8 +
 * fx_mantissa_scale + FSTP) has NO observable effect. The function's sole
 * observable behavior is its return value = fn_04a814(arg2, arg3).
 * We reproduce the FPU block faithfully (it is executed in the original) into a
 * local `dslot`; fn_04a3a8 and fx_mantissa_scale are pure, so the result is not
 * observable and dslot is unused. */
int32_t fn_04a367(uint32_t arg0, uint32_t arg1, int32_t arg2, const uint32_t *arg3, uint32_t eax_in)
{
    double dslot;
    uint64_t bits = (uint64_t)arg0 | ((uint64_t)arg1 << 32);   /* base double (arg0=low, arg1=high) */
    memcpy(&dslot, &bits, 8);
    if (eax_in != (uint32_t)arg2)                              /* JZ 0x4a38d: skip when equal */
        dslot = fn_04a3a8(dslot, (int32_t)((uint32_t)arg2 - eax_in));  /* CALL 0x04a3a8; FSTP -> dead */
    fx_mantissa_scale((uint32_t *)&dslot, arg3);               /* CALL 0x04a84e; out=&[EBP+8] -> dead */
    (void)dslot;                                               /* dead store, not observable */
    return fn_04a814(arg2, arg3);                              /* MOV EAX,ECX; MOV EDX,EBX; CALL 0x04a814 */
}

/* ---- 0x040efd / 0x0413e7 / 0x041410 (L12, 40 B each): thin port-write wrappers ----
 * Each is: PUSH 0x1c; CALL 0x0418ee (frame/canary, absorbed by the C frame); save EBX..EBP;
 * MOV EAX, <const>; CALL 0x041340; restore; RET. The register save/restore and the
 * 0x1c-byte frame have no observable effect, so each is observably just
 *   return fn_041340(const)
 * with const = 0xd0 / 0xd1 / 0xd3 respectively (device port-write command codes). */
uint32_t fn_040efd(void) { (void)canary_id(0, 0x1c); return fn_041340(0xd0); }  /* 0x040efd */
uint32_t fn_0413e7(void) { (void)canary_id(0, 0x1c); return fn_041340(0xd1); }  /* 0x0413e7 */
uint32_t fn_041410(void) { (void)canary_id(0, 0x1c); return fn_041340(0xd3); }  /* 0x041410 */

/* ---- 0x035c2d (L11, 45 B): thin wrapper -> fn_0358c7((uint8)arg) ----
 * PUSH 0x20;CALL 0x418ee (frame); save regs; EAX=zero-extend(AL); CALL 0x0358c7; restore; RET.
 * The save/zero/reload of AL just zero-extends the input byte (a uint8_t arg already is),
 * so observably: return fn_0358c7(arg). */
int32_t fn_035c2d(uint8_t arg) { (void)canary_id(0, 0x20); return fn_0358c7((uint8_t)arg); }  /* 0x035c2d */

/* ---- 0x040948 (L12, 53 B): two-command device write ----
 * PUSH 0x20;CALL 0x418ee (frame); save regs; save EAX; CALL 0x041340(0x40) (write cmd 0x40,
 * result discarded); reload EAX; CALL 0x041340(arg) (write arg, return result); restore; RET.
 * Observably: fn_041340(0x40); return fn_041340(arg). */
uint32_t fn_040948(uint32_t arg) { (void)canary_id(0, 0x20); fn_041340(0x40); return fn_041340(arg); }  /* 0x040948 */

/* ---- 0x040e8d (L14, 111 B): timer/device setup guard ----
 * canary prologue PUSH 0x20; CALL 0x418ee; save EBX..EBP. If g_63f90 == 0 (not
 * initialized) it returns. Otherwise:
 *   local = (g_dat_63f84 == 0xa) ? 0x72 : g_dat_63f84 + 8        (0x40eae-0x40ec8)
 *   CALL 0x407f8([0x63fb0])              -> fn_000407f8()   DPMI real-mode request, no-op
 *   CALL 0x42361(local,[0x63f54],[0x63f58]) -> isr_set(...)  INT 21h set-vector, no-op
 *   CALL 0x41410()                        -> fn_041340(0xd3)  device port-write (only observable effect)
 * The handler far-pointer (offset [0x63f54] / segment [0x63f58]) and the DPMI request
 * [0x63fb0] feed no-op sys stubs, so they have no flat-model effect. */
uint32_t g_63f90 = 0;   /* DAT_00063f90: timer/device init flag (0x040e8d guard) */
void fn_040e8d(void) {
    (void)canary_id(0, 0x20);                                            /* 0x40e8d-0x40e92 */
    if (g_63f90 == 0) return;                                            /* 0x40ea5-0x40eac */
    uint32_t local = (g_dat_63f84 == 0xa) ? 0x72 : g_dat_63f84 + 8;       /* 0x40eae-0x40ec8 */
    fn_000407f8();                                                        /* 0x40ed0, no-op */
    isr_set((uint8_t)local, 0, 0);                                       /* 0x40eea, no-op (orig handler [0x63f54]/[0x63f58]) */
    fn_041410();                                                          /* 0x40eef, fn_041340(0xd3) observable */
}

uint32_t g_dat_63f88 = 0;  /* DAT_00063f88: 16-bit device value (0x0411d9 combines two status bytes) */

/* 0x0411d9 (L12, 99 B): read a 16-bit device value. If fn_041340(0xe1) (write cmd 0xe1)
 * succeeds (ready), read two status bytes via fn_0412a3 (first=high, second=low), combine
 * (hi<<8)|lo, store to DAT_00063f88, and return it; else return 0. */
uint32_t fn_0411d9(void) {
    (void)canary_id(0, 0x28);                                   /* PUSH 0x28;CALL 0x418ee */
    uint32_t r;
    if (fn_041340(0xe1) != 0) {                                 /* MOV EAX,0xe1; CALL 0x41340; TEST/JNZ */
        uint32_t hi = fn_0412a3();                              /* CALL 0x412a3 (high byte) */
        uint32_t lo = fn_0412a3();                              /* CALL 0x412a3 (low byte) */
        r = (hi << 8) | lo;                                     /* SHL 8; OR */
        g_dat_63f88 = r;                                        /* MOV [0x63f88],EDX */
    } else {
        r = 0;                                                  /* not ready */
    }
    return r;
}

/* 0x040f4f (L12, 99 B): device-discovery scan. Probe 8 consecutive port bases
 * (g_dat_53334, +0x10, ... +0x70): at each, fn_04123d() (init) then fn_0412a3() (status);
 * if status==0xaa return the current base; else advance g_dat_53334 by 0x10. If none match
 * in 8, return 0. Mutates g_dat_53334 (callers restore if needed). */
uint32_t fn_040f4f(void) {
    (void)canary_id(0, 0x24);                                  /* PUSH 0x24;CALL 0x418ee */
    int32_t i = 0;
    for (;;) {
        if (i >= 8) return 0;                                  /* CMP i,8; JGE */
        fn_04123d();                                           /* device-port init at base */
        if (fn_0412a3() == 0xaa) return g_dat_53334;           /* signature found */
        i++;
        g_dat_53334 += 0x10;                                   /* advance port window */
    }
}

/* 0x035a19 (L11, 122 B): 3-byte device write sequence. Write a0 (EAX), a1 (EDX), and --
 * if a2 (EBX) >= 0 -- a2, each via fn_0358c7((uint8)x). Return -1 on the first write timeout
 * (fn_0358c7 != 0), else 0. */
int32_t fn_035a19(int32_t a0, int32_t a1, int32_t a2) {
    (void)canary_id(0, 0x24);                                   /* PUSH 0x24;CALL 0x418ee */
    if (fn_0358c7((uint8_t)a0) != 0) return -1;                 /* write a0 */
    if (fn_0358c7((uint8_t)a1) != 0) return -1;                 /* write a1 */
    if (a2 >= 0 && fn_0358c7((uint8_t)a2) != 0) return -1;      /* write a2 only if a2>=0 */
    return 0;
}

/* 0x035329 (L10, 166 B): scan an array of 0xe-byte records for the smallest value.
 * a0 (EAX) = base of records: tag byte at +0x5, int32 value at +0xa. a1 (EDX) = result
 * struct: +0x10 out_cur(int32), +0x14 out_idx(uint32), +0x1c count(uint32), +0x20 flag(byte).
 * cur starts at a0->+0xa; for i in 0..count-1, if record i's tag != 0x2f and cur > its value,
 * set idx=i and cur=value, then clear a1->+0x20 (the flag-clear is gated on tag!=0x2f: a 0x2f
 * record skips straight to i++). After the loop a1->+0x14 = idx, a1->+0x10 = cur. Addresses are
 * flat arena offsets (g_heap_base + off). */
void fn_035329(uint8_t *a0, uint8_t *a1) {
    (void)canary_id(0, 0x2c);                                   /* PUSH 0x2c;CALL 0x418ee */
    uint8_t *p0 = a0;                                           /* records (raw ptr) */
    uint8_t *p1 = a1;                                           /* result (raw ptr) */
    int32_t  cur = *(int32_t *)(p0 + 0xa);                      /* cur = a0->+0xa */
    p1[0x20] = 1;                                               /* flag = 1 */
    uint32_t idx = 0, i = 0;                                    /* [EBP-0x10], [EBP-0x14] */
    uint32_t count = *(uint32_t *)(p1 + 0x1c);                  /* a1->+0x1c */
    while (i < count) {                                         /* CMP i,count; JNC (unsigned) */
        uint8_t tag = p0[i * 0xe + 0x5];
        if (tag != 0x2f) {                                      /* CMP tag,0x2f; JZ skip */
            int32_t val = *(int32_t *)(p0 + i * 0xe + 0xa);
            if (cur > val) { idx = i; cur = val; }              /* CMP cur,val; JLE skip */
            p1[0x20] = 0;                                       /* clear flag (non-skip only) */
        }
        i++;
    }
    *(uint32_t *)(p1 + 0x14) = idx;                             /* a1->+0x14 = idx */
    *(int32_t  *)(p1 + 0x10) = cur;                             /* a1->+0x10 = cur */
}

/* 0x035bf4 (L12, 56 B): thin wrapper -> fn_035a19(al, dl, -1). Args in AL/DL (bytes);
 * a2 = EBX = -1 so the 3rd byte write is skipped. Returns fn_035a19's result. */
int32_t fn_035bf4(uint8_t al, uint8_t dl) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    return fn_035a19((int32_t)al, (int32_t)dl, -1);             /* EBX = -1 -> a2 skipped */
}

/* 0x035ac0 (L12, 75 B): device-init sequence. for i in 0..15: fn_035a19(i+0xb0, 0x78, 0).
 * No args; the incidental EAX (last write's result) is ignored (void). */
void fn_035ac0(void) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    for (int32_t i = 0; i < 16; i++)                            /* 0x10 iters */
        (void)fn_035a19(i + 0xb0, 0x78, 0);                    /* write (i+0xb0),0x78,0 */
}

/* 0x0357f2 (L10, 212 B): device read with status handshaking. Arg AL (byte a0).
 * Phase 1: wait for status bit6 (0x40) to clear on inb(g_dat_530ac+1), up to 0x2000
 *   iters; timeout -> -1. Then outb(g_dat_530ac+1, a0) (write the data byte to the
 *   status port). Phase 2: wait for status bit7 (0x80) to clear on inb(+1), up to
 *   0x2000 iters (a 0x7530 re-check follows but is dead since i < 0x2000 < 0x7530)
 *   -> -1. Read data from inb(g_dat_530ac); if 0xfe -> -2, else 0.
 * NOTE: the outb writes to +1 (the same port phase-2 polls), so phase-2 sees `a0`
 * after the write -- a0's bit7 (0x80) therefore gates the phase-2 wait. */
int32_t fn_0357f2(uint8_t a0) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    int32_t i = 0;
    for (;;) {                                                  /* phase 1: wait bit6 clear */
        if (i >= 0x2000) break;                                 /* 0x3581c */
        uint8_t s = inb((uint16_t)(g_dat_530ac + 1));           /* 0x35825/82b */
        if (s & 0x40) { i++; continue; }                        /* 0x35830 */
        break;
    }
    if (i >= 0x2000) return -1;                                 /* 0x3583f */
    outb((uint16_t)(g_dat_530ac + 1), a0);                      /* 0x35858: write a0 */
    i = 0;
    for (;;) {                                                  /* phase 2: wait bit7 clear */
        if (i >= 0x2000) break;                                 /* 0x3586c */
        uint8_t s = inb((uint16_t)(g_dat_530ac + 1));           /* 0x35875/87b */
        if (s & 0x80) { i++; continue; }                        /* 0x35880 */
        break;
    }
    if (i >= 0x7530) return -1;                                 /* 0x35888 (dead) */
    uint32_t data = inb((uint16_t)g_dat_530ac);                 /* 0x3589f */
    return (data == 0xfe) ? -2 : 0;                             /* 0x358a4 */
}

/* 0x035941 (L11, 127 B): device-init handshake over fn_0357f2.
 *   g_530ac = a0;
 *   if (fn_0357f2(0xff) != 0 && fn_0357f2(0xff) != 0) return -1;
 *   if (fn_0357f2(0x3f) != 0) return -1;
 *   (void)inb(g_530ac + 1);   // 0x359a6: read +1 (result discarded)
 *   return 0;
 * Note: under the static sys-port map the first fn_0357f2(0xff) writes 0xff to
 * +1 (bit6 set), so every subsequent phase-1 poll times out and the function
 * returns -1; a live device would clear bit6 between polls. */
int32_t fn_035941(uint32_t a0) {
    (void)canary_id(0, 0x28);                                   /* PUSH 0x28;CALL 0x418ee */
    g_dat_530ac = a0;                                           /* 0x3595f */
    if (fn_0357f2(0xff) != 0 && fn_0357f2(0xff) != 0)           /* 0x35969/77 */
        return -1;                                              /* 0x35980 */
    if (fn_0357f2(0x3f) != 0)                                   /* 0x3598e */
        return -1;                                              /* 0x35997 */
    (void)inb((uint16_t)(g_dat_530ac + 1));                     /* 0x359a6 */
    return 0;                                                   /* 0x359ae */
}

/* 0x0359c1 (L12, 87 B): device-base probe. Try each base in
 * {0x300,0x310,0x320,0x330} (step 0x10); return 0 on the first fn_035941(base)
 * that succeeds (returns 0), else -1 after exhausting all bases. fn_035941 sets
 * g_530ac to the last base tried (0x330 on the all-fail path). Under the static
 * sys-port map fn_035941 always returns -1, so this returns -1 and leaves
 * g_530ac == 0x330 (a live device would clear its status bit and let a base pass). */
int32_t fn_0359c1(void) {
    (void)canary_id(0, 0x24);                                   /* PUSH 0x24;CALL 0x418ee */
    for (uint32_t i = 0x300; i <= 0x330; i += 0x10) {           /* 0x359d9/e0/e2/e6/ed */
        if (fn_035941(i) == 0)                                  /* 0x359ef/f2/f7/f9 */
            return 0;                                           /* 0x359fb */
    }
    return -1;                                                  /* 0x35a06 */
}

/* 0x035a94 (L13, 43 B): trivial passthrough -- return fn_0359c1(). PUSH 0x20;CALL
 * 0x418ee frame; CALL 0x0359c1; return EAX. No state of its own. */
int32_t fn_035a94(void) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    return fn_0359c1();                                         /* 0x35aac/ab1/ab4 */
}

/* 0x035b91 (L12, 98 B): args AL/DL/BL. val = (a2 * g_dat_530b8) / 100 (signed); take the
 * low byte and clamp to 0x7f (bytes 0x80..0xff, i.e. negative quotients, clamp to 127).
 * Return fn_035a19(a0, a1, clamped). */
int32_t fn_035b91(uint8_t a0, uint8_t a1, uint8_t a2) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    int32_t prod = (int32_t)a2 * g_dat_530b8;                   /* IMUL EDX,[0x530b8] */
    int32_t q    = prod / 100;                                  /* IDIV EBX=0x64 (signed) */
    uint8_t b    = (uint8_t)q;                                  /* low byte of quotient */
    if (b > 0x7f) b = 0x7f;                                     /* clamp */
    return fn_035a19((int32_t)a0, (int32_t)a1, (int32_t)b);     /* write a0,a1,clamped */
}

/* 0x035b0c (L13, 132 B): device-command dispatcher. Args a0=EAX(AL), a1=EDX(DL),
 * a2=EBX(BL). If (a0&0xf0)==0xb0 && a1==7: store a2 into the command table
 * g_dat_530bc[a0&0xf] (a 32-bit slot) then call fn_035b91(a0,a1,a2); else call
 * fn_035a19(a0,a1,a2). Both callees use the a0=EAX,a1=EDX,a2=EBX convention and
 * both branches load EAX=a0,EDX=a1,EBX=a2 identically, so the port passes (a0,a1,a2)
 * to either. Returns the callee's result (EAX). */
int32_t fn_035b0c(uint8_t a0, uint8_t a1, uint8_t a2) {
    (void)canary_id(0, 0x20);                                   /* PUSH 0x20;CALL 0x418ee */
    if ((a0 & 0xf0) == 0xb0 && a1 == 0x7) {                     /* 0x35b2b-0x35b46 */
        g_dat_530bc[a0 & 0xf] = a2;                             /* 0x35b48-0x35b5a */
        return fn_035b91(a0, a1, a2);                           /* 0x35b6f */
    }
    return fn_035a19((int32_t)a0, (int32_t)a1, (int32_t)a2);    /* 0x35b85 */
}

/* 0x034eb0 (L13, 128 B): device/IRQ teardown. Gated on g_63324: disable IRQs
 * (irq_cli), retime the PIT (fn_03575d(0)), reinstall IRQ8 (isr_set(8, g_63334,
 * g_63338-low16)), re-init the device command table (fn_035ac0), re-enable IRQs
 * (irq_sti), clear g_63324; then if g_6333c != 0 free that block via fn_041c31 and
 * clear g_6333c. No return value. */
void fn_034eb0(void) {
    (void)canary_id(0, 0x1c);                                   /* PUSH 0x1c;CALL 0x418ee */
    if (g_dat_63324 == 0) return;                               /* 0x34ec8/cf */
    irq_cli();                                                  /* 0x34ed1 */
    (void)fn_03575d(0);                                         /* 0x34ed6/ed8 */
    isr_set(0x8, g_dat_63334, (uint16_t)g_dat_63338);           /* 0x34edd-0x34ef4 */
    fn_035ac0();                                                /* 0x34ef9 */
    irq_sti();                                                  /* 0x34efe */
    g_dat_63324 = 0;                                            /* 0x34f03 */
    if (g_dat_6333c != 0) {                                     /* 0x34f0d/14 */
        heap_free_host(g_dat_6333c);                         /* 0x34f16/1b */
        g_dat_6333c = 0;                                        /* 0x34f20 */
    }
}

/* 0x03565a (L12, 95 B): div = input / 1000 (signed). If div == 0, r = 0x10000; else
 * r = (div * 0x4aa) / g_dat_63304 (unsigned DIV). Then fn_000356ba(r) (PIT ch0 setup, void)
 * and return r. (0x0356ba was already ported as fn_000356ba, old 000-prefix naming.) */
int32_t fn_03565a(int32_t input) {
    (void)canary_id(0, 0x1c);                                   /* PUSH 0x1c;CALL 0x418ee */
    int32_t div = input / 1000;                                 /* IDIV EBX=0x3e8 (signed) */
    int32_t r;
    if (div == 0) {
        r = 0x10000;                                            /* 0x03567f */
    } else {
        int32_t num = div * 0x4aa;                              /* IMUL EAX, 0x4aa (signed) */
        r = (int32_t)((uint32_t)num / g_dat_63304);             /* DIV [0x63304] (unsigned) */
    }
    fn_000356ba((uint32_t)r);                                   /* CALL 0x0356ba (PIT setup) */
    return r;                                                   /* divisor (incidental EAX) */
}
