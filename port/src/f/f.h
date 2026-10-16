/* f.h — ported original functions (batch 1: L0 primitives).
 * One original function => one C function. Argument letters follow the
 * original PUSH order: a = 1st arg ([EBP+8]), b = 2nd, ...
 * Argument semantics (x/y/width/...) get pinned when callers are ported.
 * Semantics doc: docs/12-functions/primitives.md */
#ifndef NIGHT_F_H
#define NIGHT_F_H

#include <stdint.h>
#include <stddef.h>

/* ---- globals mirrored from original BSS (see docs/12-functions/globals.md) ---- */
extern int32_t  g_video_stride;  /* DAT_00040010: frame pitch (320 for 320x200) */
extern uint16_t g_ss_saved;      /* DAT_0005333c: SS snapshot (canary) */
extern int32_t  g_stack_bound;   /* DAT_00053394: canary stack bound */

/* ---- video frame buffer primitives ---- */

/* a[c * g_video_stride + b] = d                               orig: 0x040024 */
void put_px(uint8_t *a, int b, int c, uint8_t d);

/* do { *a = *b; b++; a--; n--; } while (n)                   orig: 0x040475 */
void cpy_back(uint8_t *a, const uint8_t *b, int n);

/* for (; n; n--) { *c = *a; a++; c++; }   (dwords)           orig: 0x0404c6 */
void cpy_dw(uint32_t *a, int n, uint32_t *c);

/* h rows of w bytes: odd byte, then w>>1 words;
 * src += s - w, dst += t - w per row                          orig: 0x040491 */
void blt_rows(uint8_t *a, int s, uint32_t w, int h, uint8_t *c, int t);

/* p = a + d*b + c; fill e bytes: odd byte, then e>>1 words of f|f<<8
 * a = base, b = row, c = column, d = row length, e = line length,
 * f = pixel                                            orig: 0x0404db */
void hline_set(uint8_t *a, int b, int c, int d, uint32_t e, uint8_t f);

/* p = a + d*b + c; do { *p = f; p += b; e--; } while (e)     orig: 0x040506 */
void vline_set(uint8_t *a, int b, int c, int d, int e, uint8_t f);

/* p = a + c*stride + b; e rows of d bytes (odd, then words);
 * p += stride - d per row                                     orig: 0x04052c */
void rect_fill(uint8_t *a, int b, int c, uint32_t d, int e, uint8_t f);

/* ---- input-board I/O, ports 0x3C7-0x3C9 ---- */

/* outb(0x3C8, a); REP OUTSB: 3*b bytes of c -> port 0x3C9     orig: 0x040042 */
void board_out(uint8_t a, int b, const uint8_t *c);
/* outb(0x3C7, a); REP INSB:  3*b bytes from port 0x3C9 -> c   orig: 0x040061 */
void board_in(uint8_t a, int b, uint8_t *c);

/* ---- misc ---- */

int32_t  abs32(int32_t a);         /* orig: 0x041f76 */
void     canary_ss_save(void);     /* g_ss_saved = SS          orig: 0x0418e6 */
uintptr_t canary_check(uint32_t frame_size); /* stack guard  orig: 0x041901 */
uintptr_t canary_id(uintptr_t eax, uint32_t frame_size); /* check; return eax  orig: 0x0418ee */

/* ---- segment / memory / string / FPU primitives (batch 2) ----
 * NOTE: several of these take their args in registers in the original
 * (EAX/EDX/EBX), not on the stack; a/b/c below follow param_1/2/3 order. */

/* out[6] = {ES, CS, SS, DS, FS, GS}  (DPMI real-mode seg save)  orig: 0x041df2 */
void seg_save(uint16_t *a);

/* REP MOVSD (c>>2) + REP MOVSB (c&3); returns the original dst  orig: 0x041e6c */
uint8_t *mem_copy(uint8_t *a, const uint8_t *b, uint32_t c);

/* returns &g_rng_state  (PRNG state word, DAT_0005335c)  orig: 0x041e99 */
extern uint32_t g_rng_state;
uint32_t *rng_state_ptr(void);

/* Unsigned itoa: base-c representation of a into b, NUL-terminated;
 * digit table 0-9a-z; returns the original b  orig: 0x041ef8 */
char *itoa32(uint32_t a, char *b, uint32_t c);

/* strlen with a 2^32 underflow guard  orig: 0x041f5d */
uint32_t str_len(const char *a);

/* x87 FRNDINT (round-to-int, round-to-nearest-even) on the FPU top;
 * always called between FLD and FISTP. Modern: rint().  orig: 0x041f7e */
double fpu_rndint(double a);

#endif
