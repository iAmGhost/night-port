/* f.c — ported original functions (batch 1: L0 primitives).
 * Bodies are 1:1 translations of the decompilation in analysis/pseudo/,
 * cross-checked against the disassembly (scratch/batch1_asm*.txt).
 * Semantics: docs/12-functions/primitives.md */
#include "f.h"
#include "../sys/sys.h"

/* ---- globals (mirrored BSS) ---- */
int32_t  g_video_stride = 0;
uint16_t g_ss_saved = 0;
int32_t  g_stack_bound = 0;

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
