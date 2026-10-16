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

/* ---- low-level I/O + interrupt-vector primitives (timer / PIT / INT 0x1C
 * subsystem). The decompiler masked several privileged x86 instructions as
 * no-ops; the disassembly shows the real opcodes. In the flat model the
 * privileged ones (DS load, CLI, STI) have no effect and are ported as
 * documented no-ops; real port I/O and the OS interrupt-vector service go
 * through the sys layer. */

/* DS = DAT_00042319 (real-mode DS). No-op: flat model, DS is constant. */
void     seg_ds_load(void);              /* orig: 0x042310 */
/* CLI (disable interrupts). No-op: no interrupt flag in the flat model. */
void     irq_cli(void);                  /* orig: 0x04231b */
/* OUT port, AL — single-byte port write (40 sites: PIC EOI, PIT, CD audio).
 * Args register-passed in the original (EAX=port, EDX=val). */
void     outb(uint16_t port, uint8_t val); /* orig: 0x04231d */
/* STI (enable interrupts). No-op. */
void     irq_sti(void);                  /* orig: 0x04232b */
/* INT 0x21h get-interrupt-vector: returns the current handler as a packed
 * far pointer (offset in low 32, segment in bits 32..47). 0 in the port. */
uint64_t isr_get(uint8_t vector);        /* orig: 0x04232d */
/* INT 0x21h set-interrupt-vector: install handler_seg:handler_off for
 * `vector`. No-op now; the timer subsystem registers its tick handler here.
 * (The original passes the vector in two registers; collapsed to one arg.) */
void     isr_set(uint8_t vector, uint32_t handler_off, uint16_t handler_seg); /* orig: 0x042361 */

/* ---- streaming-printf / resource-reader state (batches 4) ----
 * These structs mirror the original's data structures as seen by their
 * L0 callback/leaf functions; they are fully realized when the higher-level
 * functions that own them (the printf FUN_00044c23, the reader 0x0004247a)
 * are ported. Field offsets are fixed to the original layout. */

/* Chunked resource reader (CD data stream). pos/end are pointers into the
 * current block, remaining the bytes still to read, id a resource handle.
 * flags bit 0x10 is a state bit cleared on each consume. */
typedef struct {
    uint8_t  *pos;      /* 0x0: current read position        */
    uint32_t  remaining;/* 0x4: bytes remaining in resource  */
    uint8_t  *end;      /* 0x8: end of current block         */
    uint8_t   flags;    /* 0xc: state flags (bit 0x10)       */
    uint32_t  id;       /* 0x10: resource id / handle        */
} res_reader;

/* Internal state of the callback-based streaming printf (FUN_00044c23), as
 * seen by its byte-sink callback. pos is at offset 0x0, count at 0x10. */
typedef struct {
    uint8_t *pos;   /* 0x0: write position         */
    int32_t  width; /* 0x4: field width            */
    int32_t  slen;  /* 0x8: field/string length    */
    int32_t  zeros; /* 0xc: leading-zero count     */
    int32_t  count; /* 0x10: bytes emitted (the printf's return value) */
} fmt_state;

/* IN AL, DX — single-byte port read (companion to outb).  orig: 0x04238c */
uint8_t  inb(uint16_t port);
/* 1-byte RET: the default no-op for the 3-slot callback table
 * (DAT_0005336c/70/74), invoked by dispatchers 0x000423b7/0x000423cf and
 * replaced at runtime once real handlers are installed.  orig: 0x0423b6 */
void     cb_noop(void);
/* Bounded case-insensitive string compare: returns 0 if the first `n` bytes
 * of a and b are equal ignoring case, else the signed difference of the
 * first differing (lowercased) bytes. Register-passed (EAX=a, EDX=b, EBX=n).
 * orig: 0x0423e5 */
int      str_icmp(const char *a, const char *b, int n);
/* Consume `amount` bytes from a resource reader (res_reader). Success needs
 * (end-pos) <= amount <= remaining: pos += amount, remaining -= amount,
 * return 0. Otherwise pos = end, remaining = 0, return 1. Clears flag 0x10.
 * orig: 0x042438 */
int      res_consume(res_reader *r, int amount);
/* Digit value (0..35) -> ASCII char: 0-9 -> '0'-'9', 10-35 -> 'a'-'z'.
 * orig: 0x0427e7 */
int      digit_chr(int d);
/* Byte-sink callback for the streaming printf: store `byte` at st->pos,
 * advance st->pos, increment st->count.  orig: 0x0428e5 */
void     fmt_emit(fmt_state *st, uint8_t byte);

/* ---- input + compare + no-op (batch 5) ---- */

/* DAT_000533a4: a 1-char pending-key buffer. Input pollers store a key here;
 * get_key() returns it (and clears it) before falling back to a read. */
extern uint32_t g_keybuf;
/* DOS INT 21h AH=08 (direct character input) with a 1-char cache: returns the
 * buffered key from g_keybuf if present, else a read from the input backend
 * (blocking in the original).  orig: 0x04295d */
uint8_t  get_key(void);
/* Bounded memcmp: compares the first `n` bytes; returns -1 if a<b, 0 if
 * equal, +1 if a>b, decided at the first differing byte.  orig: 0x0429a8 */
int      mem_cmp(const void *a, const void *b, int n);
/* 1-byte RET: a second no-op default handler (distinct from cb_noop at
 * 0x0423b6), referenced by the data table at DAT_000533bd and the wrappers
 * 0x00042a3c / 0x000463dc.  orig: 0x042a07 */
void     cb_noop2(void);

#endif
