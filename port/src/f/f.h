/* f.h — ported original functions (batch 1: L0 primitives).
 * One original function => one C function. Argument letters follow the
 * original PUSH order: a = 1st arg ([EBP+8]), b = 2nd, ...
 * Argument semantics (x/y/width/...) get pinned when callers are ported.
 * Semantics doc: docs/12-functions/primitives.md */
#ifndef NIGHT_F_H
#define NIGHT_F_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- globals mirrored from original BSS (see docs/12-functions/globals.md) ---- */
extern int32_t  g_video_stride;  /* DAT_00040010: frame pitch (320 for 320x200) */
extern uint16_t g_ss_saved;      /* DAT_0005333c: SS snapshot (canary) */
extern uintptr_t  g_stack_bound;   /* DAT_00053394: canary stack bound */

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

/* 0x049459: table-based base-c itoa (runtime digit table at 0x049434); its
 * observable output is byte-identical to itoa32 (0x041ef8). Ported as a thin
 * delegation to itoa32. */
char *itoa32_tbl(uint32_t a, char *b, uint32_t c);

/* 0x0494cb: Gregorian leap-year test. Returns (ctx << 32) | is_leap, where
 * is_leap = 1 iff (year%4==0 && (year%100!=0 || year%400==0)). Low 32 bits carry
 * the 0/1 flag; high 32 bits echo ctx (param_2, the caller's preserved EDX). */
uint64_t is_leap(uint32_t year, uint32_t ctx);

/* 0x0498f6: date/time decoder (108 ins). Fields in 4-byte order, +0x00..+0x1c. */
struct dec_datetime {
    int32_t sec;    /* +0x00 */
    int32_t min;    /* +0x04 */
    int32_t hour;   /* +0x08 */
    int32_t day;    /* +0x0c */
    int32_t month;  /* +0x10 */
    int32_t year;   /* +0x14 */
    int32_t dow;    /* +0x18 */
    int32_t doy;    /* +0x1c */
    int32_t mode;   /* +0x20: set by caller 0x049a0b, read by 0x049506 */
};
struct dec_datetime *fn_0498f6(int32_t base_day, uint32_t seconds, int32_t delta, struct dec_datetime *out);
int32_t fn_04574a(struct dec_datetime *d);                                   /* 0x04574a: recurring-event date/time normalizer (in-place; returns unix-epoch-ish offset or -1) */
int32_t fn_049506(struct dec_datetime *d, int32_t ref_year);                 /* 0x049506: recurring-event date -> day-of-year offset */
int32_t fn_0495eb(struct dec_datetime *x, struct dec_datetime *y, int32_t ref_year); /* 0x0495eb: 1 iff 0x049506(x)>0x049506(y) */
int32_t fn_049613(struct dec_datetime *d);  /* 0x049613: recurring-event date arithmetic; returns EDI (also d->mode) */
int32_t fn_04322f(void);                                            /* 0x04322f: return -1 */
int32_t fn_044848(uint32_t handle, uint32_t a2, uint32_t a3, uint32_t a4); /* 0x044848: lseek(h,0,SEEK_SET) */
void fn_049a0b(uint32_t *seconds, struct dec_datetime *out, uint32_t p3, uint32_t p4); /* 0x049a0b: unix-time decode */
void fn_03eff2(void);                          /* 0x03eff2: install IRQ 0x1c (FPU) handler (flag-guarded) */
void fn_03ffab(void);                          /* 0x03ffab: install IRQ 9 handler (flag-guarded) */
extern uint8_t *g_dat_6334c;   /* DAT_0006334c: stage room-chain head */
void fn_037b03(int32_t resume_flag);
extern uint32_t g_dat_63398;   /* DAT_00063398: room orientation byte (room node +0x11), set by fn_0374ad */
extern uint32_t g_tab_52c3c[0x104]; /* DAT_00052c3c: 10 dwords per stage, indexed by room serial; no producer in the EXE (read-only: 0x0374ad/0x0376c8/0x03af18/0x03d244) */
void fn_037347(void);   /* 0x037347: THE STAGE DATA TEARDOWN (free-only half of fn_0374ad): per-type .spr/.act, the stage .spr and the room chain; leaves DAT_63354 dangling by design */
void fn_0374ad(void);
extern uintptr_t g_dat_53c84; /* DAT_00053c84: the player (HUD) ENTITY POINTER (second slot), set by fn_0376c8; widened from a dword so the 64-bit host pointer survives (the original keeps a flat dword address) */
extern uint32_t g_tab_52738[0x100]; /* DAT_00052738: per-ship dword table indexed by entity +0x28; no producer in the EXE (reads only) */
void fn_0376c8(void);   /* 0x0376c8: THE PLAYER SPAWN -- builds the hero entity (fn_010efc type 0) into DAT_53c7c, picks the active key/catch/action tables from DAT_63378/6337c, resume-loads stage data, then seeds the entity position/orientation */   /* 0x0374ad: THE STAGE RESET/RELOAD -- frees the previous stage's per-type .spr/.act, stage .spr and room chain, reloads via fn_037b03, then seeds the player config block (DAT_53c7c) from the new room node */   /* 0x037b03: THE STAGE LOADER -- stage%d.wal (room chain via fn_03c853), st%ds.spr, stage%d.stg (0x59 B records; kind 1 loads a type's .spr/.act, kind 5 the DAT_5d0b4 array), night%d.chr (the 101-entry entity-type table) */
void fn_037306(void);                          /* 0x037306: cleanup -- if g_639b8: fn_0427a3(g_639b8); fn_00040570; fn_03ffab; fn_03eff2 */
void fn_03575d(uint32_t divisor);              /* 0x03575d: PIT ch0 rate setup (divisor in EAX) */
void fn_03f3be(uint32_t color);                /* 0x03f3be: VGA palette-set, index 0x63e5c (color in EAX) */
void fn_03f462(uint32_t base, int32_t row2, uint32_t width, uint32_t count, const uint8_t *src); /* 0x03f462: row-wrap fill into A0000 */
void fn_03a5f7(const uint8_t *base); /* 0x03a5f7: progressive horizontal reveal of 200-row source into A0000 */
void fn_038c23(uint32_t base, uint8_t *font, int32_t col0, int32_t start_row, int32_t number); /* 0x038c23: render 0..99 as 2 digit glyphs */
void fn_038b30(uint32_t base, uint8_t *font, int32_t col0, int32_t start_row, int32_t number); /* 0x038b30: render int as 5-glyph right-justified field */
void fn_0137c1(uint8_t *entity, uint32_t base); /* 0x0137c1: HUD countdown number display (global font 0x63e04, tick 0x63360) */
void fn_0383a4(uint32_t base, uint8_t *font, int32_t col0, int32_t start_row, int32_t number); /* 0x0383a4: render integer digits left-to-right (8-col spacing) */
void fn_03df96(uint32_t base); /* 0x03df96: sprite/entity blit (per-record glyph via fn_03bb42) */
void fn_03f410(uint32_t color);                /* 0x03f410: VGA palette-set, index 0x63e58 (color in EAX) */
void fn_1053f(void);                            /* 0x01053f: DPMI INT 0x31 AX=0x101 (no-op) */
void fn_0003fade(void);                         /* 0x0003fade: zero DAT_000a0000[64000] (DPMI region, no-op) */
void fn_00040570(void);                         /* 0x00040570: DPMI real-mode call {3} via fn_041e51 (no-op) */
void fn_000405a7(void);                         /* 0x000405a7: DPMI real-mode call {0x13} via fn_041e51 (no-op) */
void fn_000407f8(void);                         /* 0x000407f8: DPMI real-mode call {0x101} via fn_041e51 (no-op) */
void fn_00010dde(void);                         /* 0x00010dde: build stack struct + fn_00010570 (no-op) */
void fn_0003fcbd(uint32_t p1, uint32_t p2, uint32_t p3); /* 0x0003fcbd: DPMI real-mode call {0x1012} via fn_041e51 (no-op) */
int   fn_00040788(void);                        /* 0x00040788: DPMI mem alloc INT31h 0x100 (no-op, returns 0) */
void fn_000104c7(void);                         /* 0x000104c7: DPMI mem alloc + DAT_00053c58/5c setup (no-op) */
void fn_0003f645(void);                         /* 0x0003f645: VGA palette + A0000 memset loop (no-op) */
void fn_00035ec2(void);                         /* 0x00035ec2: VGA 0x3DA sequencer ready-wait (no-op, degenerate spin) */
void fn_040660(const uint8_t *arg1, uint32_t arg2); /* 0x040660: VGA DAC row-blit fade-in (ascending, i 0..arg2-1) */
void fn_0406f5(const uint8_t *arg1, uint32_t arg2); /* 0x0406f5: VGA DAC row-blit fade-out (descending, i arg2..0) */
uint32_t fn_00041bdc(uint32_t size);            /* 0x00041bdc: heap_malloc wrapper (size 0->1) */
void fn_00035d0f(void);                         /* 0x00035d0f: DMA channel busy-wait (no-op, degenerate spin) */
void fn_00035d5d(void);                         /* 0x00035d5d: 4-flag DMA/channel busy-wait (no-op, degenerate spin) */
int  fn_00035dfc(void);                         /* 0x00035dfc: 4-flag busy-wait + settle query (no-op, degenerate spin, returns 0) */
void fn_000356ba(uint32_t ticks);               /* 0x000356ba: PIT ch0 rate setup from ticks (reg-ABI) */
extern int32_t  g_dat_530b4;                    /* DAT_000530b4: timer rate multiplier */
extern int32_t  g_dat_530b8;                    /* DAT_000530b8: timer rate multiplier #2 (fn_035b91) */
extern int32_t  g_dat_530bc[16];                /* DAT_000530bc: 16-entry command table (fn_035b0c writes [a0&0xf]) */
extern uint32_t g_dat_63304;                    /* DAT_00063304: timer divisor (fn_03565a denominator) */
extern int32_t  g_dat_63308;                    /* DAT_00063308: rate/scale word (fn_0353d0 cmd 0x51 path writes it) */
extern uint32_t g_dat_63330;                    /* DAT_00063330: last timer tick value */
extern uint32_t g_dat_63324;                    /* DAT_00063324: IRQ/device enabled flag (gate + cleared by fn_034eb0) */
extern uint32_t g_dat_63334;                    /* DAT_00063334: IRQ8 handler offset (isr_set arg) */
extern uint32_t g_dat_6330c;                    /* DAT_0006330c: timer-tick counter (fn_034c40) */
extern void   (*g_cb_63334)(void);              /* host callable for the CALL through DAT_00063334 */
extern uint32_t g_dat_63338;                    /* DAT_00063338: IRQ8 handler segment (isr_set arg, low 16) */
extern uint32_t g_dat_6333c;                    /* DAT_0006333c: memory block freed via fn_041c31 (fn_034eb0) */
void fn_00010a56(void);                         /* 0x00010a56: CD track-entry submit (no-op, feeds fn_00010570) */
void fn_0003f592(void);                         /* 0x0003f592: VGA vline to A0000 + palette (no-op, unmodeled buffer) */
void fn_0003c7d7(uint8_t *s);                   /* 0x0003c7d7: zero 6 struct fields + memset DAT_0005d08c/5d0a0 */
void fn_03c853(uint8_t *node, uint8_t *io);   /* 0x03c853: recursive stage/room node loader (stream -> 0x354 B node + geometry blocks; chains via +0x34c/+0x354, gate ++DAT_6335c == DAT_639d8) */
extern uint32_t g_dat_639e8;   /* DAT_000639e8: stage-node index counter */
extern uint32_t g_dat_6335c;   /* DAT_0006335c: loaded-room counter (fn_03c853 gate) */
extern uint32_t g_dat_639d8;   /* DAT_000639d8: room count of the current stage chain */
uint8_t *fn_03c6ec(uint8_t *s);                 /* 0x03c6ec: reset one node (fn_03c7d7 + zero +0x44), return s */
uint32_t fn_03d88f(void);                       /* 0x03d88f: reset every node in list at g_dat_63350 (next=+0x34c) */
extern uint8_t *g_dat_63350;                    /* DAT_00063350: reset-list head */
extern uint8_t g_dat_5d08c[0x14];               /* DAT_0005d08c */
extern uint8_t g_dat_5d0a0[0x14];               /* DAT_0005d0a0 */
uint8_t *fn_0003b5c0(uint8_t *node, uint8_t flag, uint32_t d_ecx, uint32_t d_edx, uint32_t d_ebx); /* 0x0003b5c0: linked-list insert (reg-ABI) */
uint8_t *fn_0003b67e(uint8_t *node);            /* 0x0003b67e: linked-list remove (reg-ABI) */
void fn_03b9b7(void);                           /* 0x03b9b7: drain entity list (remove+free each node) */
extern uint8_t *g_dat_639cc;                    /* DAT_000639cc: list head */
extern uint8_t *g_dat_639d0;                    /* DAT_000639d0: list tail */
extern int32_t  g_dat_639c8;                    /* DAT_000639c8: list count */
extern uint8_t *g_dat_639d4;                  /* DAT_000639d4: drain cursor (fn_03b9b7) */
void fn_00035c5c(void);                         /* 0x00035c5c: entity-list init pass (walk + set fields) */
extern uint8_t *g_dat_53c70;                    /* DAT_00053c70: entity list head */
extern uint8_t *g_dat_53c78;                    /* DAT_00053c78: entity list cursor */
extern uint8_t  g_dat_53ea8[];                  /* DAT_00053ea8: 0x88-byte entry table */
void fn_03ef29(uint32_t timer_value);           /* 0x03ef29: install IRQ 0x1c (divisor in EAX) */
void fn_03feb8(void);                            /* 0x03feb8: install IRQ 9 + timer/DMA state init */
void     fn_103e0(void);                        /* 0x0103e0: DPMI real-mode I/O wrapper (no-op) */
void     fn_10570(const uint8_t *buf, uint32_t len);   /* 0x010570: DPMI real-mode read via 0x103e0 (no-op) */
void     fn_0105f7(uint8_t al);                  /* 0x0105f7: build 24-byte DMA cmd from DAT_00051a14 + g_dat_53c5c patch; write al to byte 0 of *g_dat_53c5c (far-pointer buffer); submit via fn_10570 (no-op) */
void     fn_01072f(void);                         /* 0x01072f: build 26-byte cmd from DAT_00051a58; g_dat_53c4c[0]=4; round-trip 9 bytes staging<->*g_dat_53c5c; submit via fn_10570 (no-op) */
uint8_t *fn_010010(uint8_t *block, const char *name); /* 0x010010: record-table loader (count + dword array of 0x79-byte record blocks via fn_010181) */
uint8_t *fn_010181(uint8_t *p, uint8_t *node);   /* 0x010181: resource-record deserializer (fixed field layout via fn_04192e/fn_041c8f + 3 heap arrays sized by the leading count) */
int32_t  fn_010cbd(void);                         /* 0x010cbd: 24-byte DPMI call block from DAT_00051b38 + paragraph-split far ptr fields; 1-byte mem_copy (uninit src, latent); !(block[3]&0x200) */
uint32_t fn_03e14c(uint32_t a0, uint32_t a1);     /* 0x03e14c: 2-node prev/next swap (0x51/0x55) gated by g_dat_63e14 counter; return a0 */
void     fn_03ee0a(void);                          /* 0x03ee0a: destroy prev-list: g_63e20=g_63e18; while(g_63e20) heap_free_guard(fn_03e14c(g_63e20,0)); g_63e20=node->0x51; then g_62a8c=0 */
uint32_t fn_03e0a0(uint32_t node_off, uint8_t *io_node);  /* 0x03e0a0: build+append doubly-linked node (offset model); node[0]=fn_041c8f, node[1..80]=fn_04192e; return node_off */
uint32_t fn_01435f(uint32_t a0, uint8_t a1, uint8_t a2);  /* 0x01435f: find first straddling interval i (0..5); return 0 iff a1==a0->0x17 && a2==i else 1 */
void     fn_031f5f(uint32_t a0);     /* 0x031f5f: remove first array element (shift left by one), save slot0 to a0->0x17, set g_dat_63364/63368 from P2[a0->0x17] */
void     fn_03d67c(uint32_t a0, uint32_t a1, uint32_t a2);  /* 0x03d67c: move element a2 to index a1 of a 3-byte-stride array (shift [a1,a2-1] right) */
void     fn_03d606(uint32_t a0);  /* 0x03d606: loop i< a0->0x3c: fn_03d67c(a0+0x48, pairs[i*2], pairs[i*2+1]) (pairs at heap-off a0->0x44) */
int32_t  fn_017218(uint8_t *a, int32_t a1, int32_t *o1, int32_t *o2);  /* 0x017218: set *o1/*o2 to a sign-combo of a1 by idx=a->0xbb (0..3); then a->0xab-- (ret 1 if it was <0). Args are host pointers: the caller (0x017f13) passes the addresses of its own stack locals */
int32_t  fn_034f31(uint32_t a0, uint32_t a1, uint32_t a2);  /* 0x034f31: MIDI RIFF deserializer (MThd header + N MTrk tracks) -> a2 struct; a0/a1/a2 heap offsets */
uint64_t fn_10876(void);                        /* 0x010876: CD dir-entry read+checksum (no-op -> 0) */
void     fn_107b9(void);                        /* 0x0107b9: CD dir-entry read (no-op) */
void     fn_10664(void);                        /* 0x010664: CD request build + DPMI read (no-op) */
void     fn_10b1d(void);                        /* 0x010b1d: CD track/TOC read (no-op) */
void     fn_010d47(void);                       /* 0x010d47: periodic CD-refresh tick (gated on g_63e38/490 > g_53c48) */
void     fn_048e60(uint32_t *p);                /* 0x048e60: 128-bit fixed-point normalize + Newton reciprocal divide */
void     fn_047f54(uint32_t lo_in, uint32_t hi_in, uint32_t *out); /* 0x047f54: fixed-point normalize (EAX:EDX in, EBX out) */
uint64_t fn_048053(uint32_t f0, uint32_t f1, uint16_t f2);         /* 0x048053: fixed-point -> double normalize (12B in, EDX:EAX out) */
void fn_047b79(uint32_t eax, uint32_t edx, uint32_t esi, uint32_t ecx, uint32_t ebx,
               uint32_t *o_eax, uint32_t *o_edx, uint16_t *o_si);    /* 0x047b79: pure-integer fixed-point binary op (mult/div); register-ABI in EAX,EDX,ESI,ECX,EBX, out EAX,EDX,SI */
uint64_t fn_04794a(uint32_t eax, uint32_t edx, uint32_t ebx, uint32_t ecx, uint32_t esi); /* 0x04794a: complex fixed-point normalize, register-ABI, returns (EDX<<32)|EAX */
/* 0x0485eb register-ABI align-subtract-normalize: A={a_hi:a_lo}, B={b_hi:b_lo}, expA/expB 15-bit.
 * Output registers (read by caller): lo=EAX, hi=EDX, exp=ESI, ebx=EBX(=expA&0x8000),
 * bx=BX(=(expA&0x8000)^(expB&0xffff)), edi=EDI(neg flag, NEG if bit7(bx)). */
typedef struct { uint32_t lo, hi, exp, ebx, edi; uint16_t bx; } fx_sub_t;
void fn_0485eb(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi, uint32_t expA, uint32_t expB, fx_sub_t *out); /* 0x0485eb */
/* 0x047xxx FPU cluster value: 64-bit mantissa (lo:hi) + 16-bit exponent (lo@0,hi@4,exp@8). */
typedef struct { uint32_t lo; uint32_t hi; uint16_t exp; } fx128;
/* 0x047d4e (fx_mul64): 64x64->128 fixed-point multiply. reg-ABI in {EAX=a_lo,EDX=a_hi,
 * EBX=b_lo,ECX=b_hi,ESI=esi(exp field)}; out {EAX=r_lo,EDX=r_hi,ESI=r_exp}. Bias 0xc002;
 * overflow -> 0x8000000000000000 (ESI=0x7fff); underflow -> 0. */
typedef struct { uint32_t lo; uint32_t hi; uint32_t exp; } fx64_t;
void fn_047d4e(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi, uint32_t esi, fx64_t *out); /* 0x047d4e */
/* 0x048585: thin wrapper over fn_0485eb. a,b are fx128 {lo,hi,exp}. If a==0 return 0;
 * if b==0 zero *a and return 0; else *a = (0x0485eb(a,b)) result {lo,hi,exp}, return EDI (neg flag). */
uint32_t fn_048585(fx128 *a, fx128 *b); /* 0x048585 */
void fn_0478f0(fx128 *a, fx128 *out, const fx128 *b); /* 0x0478f0: fn_04794a wrapper (EAX=a,EBX=out,EDX=b); out={r_lo,r_hi,a->exp} */
void fn_0478e2(fx128 *a, fx128 *out, const fx128 *b); /* 0x0478e2: as 0x0478f0 but b->exp bit15 flipped before compose */
void fn_04791a(fx128 *a, fx128 *out, uint32_t b_lo, uint32_t b_hi, uint32_t exp_hi); /* 0x04791a: fn_04794a wrapper w/ scalar b (stack args; exp_hi low16) */
void fn_047b4f(fx128 *a, const fx128 *b, fx128 *out);                /* 0x047b4f: thin wrapper, out = a <op> b via fn_047b79 (register-ABI pack) */
void fn_04836c(fx128 *a, const fx128 *b, fx128 *out);                /* 0x04836c: fx128 a OP b w/ zero/inf special cases + sign post-scale */

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
    uint32_t  pos;      /* 0x0: current read position (32-bit addr/offset) */
    uint32_t  remaining;/* 0x4: bytes remaining in resource  */
    uint32_t  end;      /* 0x8: end of current block (32-bit addr/offset)  */
    uint8_t   flags;    /* 0xc: state flags (bit 0x10)       */
    uint8_t   flags2;   /* 0xd: secondary flags (0x10/0x8 in io write/seek) */
    uint8_t   _pad[2];  /* 0xe-0xf: (unused, keep id at 0x10) */
    uint32_t  id;       /* 0x10: resource id / handle        */
} res_reader;   /* 16-bit node layout: pos/end/remaining/id are 32-bit */

/* Internal state of the callback-based streaming printf (FUN_00044c23), as
 * seen by its byte-sink callback. Raw 32-bit descriptor layout (matches the
 * byte-array fn_044c23 passes to cb, and fmt_emit's disasm: pos=[st+0x00]
 * 32-bit flat addr, `inc DWORD [st+0x10]`). pos is a 32-bit flat address. */
typedef struct {
    uint32_t pos;   /* 0x0: write position (32-bit flat addr)  */
    int32_t  width; /* 0x4: field width            */
    int32_t  slen;  /* 0x8: field/string length    */
    int32_t  zeros; /* 0xc: leading-zero count     */
    int32_t  count; /* 0x10: bytes emitted (the printf's return value) */
} fmt_state;

/* IN AL, DX — single-byte port read (companion to outb).  orig: 0x04238c */
uint8_t  inb(uint16_t port);
extern uint32_t g_dat_63f84;               /* DAT_00063f84: IRQ/device state (0x041439 sets 2) */
extern uint32_t g_dat_53334;               /* DAT_00053334: device port base */
void     fn_041439(void);                   /* 0x041439: IRQ handler (PIC EOI, sets DAT_00063f84=2) */
extern uint32_t g_dat_63e28;               /* DAT_00063e28: tick counter (wraps at 8) */
extern uint32_t g_dat_63e24;               /* DAT_00063e24: per-tick counter */
extern uint32_t g_dat_63e3c;               /* DAT_00063e3c: tick counter (wraps at 18) */
extern uint32_t g_dat_63e38;               /* DAT_00063e38: per-tick counter */
extern uint32_t g_dat_63e40;               /* DAT_00063e40: coarse counter (every 18) */
extern uint32_t g_dat_63e30;               /* DAT_00063e30: frame/time counter (18-bit) */
extern uint32_t g_dat_63e2c;               /* DAT_00063e2c: tick rate increment */
extern uint32_t g_dat_63e34;               /* DAT_00063e34: high frame counter */
extern uint8_t  g_dat_63e4a;               /* DAT_00063e4a: IRQ 0x1c pending-install flag */
extern uint32_t g_dat_63e44;               /* DAT_00063e44: IRQ 0x1c handler offset */
extern uint16_t g_dat_63e48;               /* DAT_00063e48: IRQ 0x1c handler segment */
extern uint8_t  g_dat_63e86;               /* DAT_00063e86: IRQ 9 pending-install flag */
extern uint32_t g_dat_63e80;               /* DAT_00063e80: IRQ 9 handler offset */
extern uint16_t g_dat_63e84;               /* DAT_00063e84: IRQ 9 handler segment */
extern uint8_t  g_dat_63f51;               /* DAT_00063f51: IRQ 9 source byte */
extern uint8_t  g_dat_00417;               /* DAT_00000417: real-mode low-memory flag */
extern uint32_t g_dat_6332c;               /* DAT_0006332c: PIT ch0 status */
extern uint32_t g_dat_63318;               /* DAT_00063318: PIT ch0 divisor */
extern uint32_t g_dat_63e50[256];          /* DAT_00063e50: per-register palette cache */
extern uint32_t g_dat_63e5c;               /* DAT_00063e5c: palette index (fn_03f3be) */
extern uint32_t g_dat_63e58;               /* DAT_00063e58: palette index (fn_03f410) */
extern uint32_t g_dat_63e60;               /* DAT_00063e60: VBE bytes per line (ModeInfoBlock +0x10) */
extern uint32_t g_dat_63e68;               /* DAT_00063e68: VBE y resolution (ModeInfoBlock +0x14) */
extern uint32_t g_dat_531f8;               /* DAT_000531f8: g_dat_531f4 - 1 */
extern uint32_t g_dat_533a4;                /* DAT_000533a4: pending-input latch (0x04295d consumes it) */
extern uint32_t g_dpmi_free_dos_paragraphs; /* the DPMI 0x0031 free-memory reply (flat-model stand-in) */
void fn_03ae8e(void);   /* 0x03ae8e: THE FREE-MEMORY GATE -- DPMI 0x0031 reply < 0x17c -> set the mode, paint ram.pcx, wait for a key, terminate. */
void fn_038a20(void);   /* 0x038a20: THE COMPANY-LOGO SCREEN -- VESA set, palette clear, logo.pcx, DAC fade in/out with two 0x400-unit holds. */
void fn_038ae0(void);   /* 0x038ae0: THE OPENING MOVIE STEP -- CD track 1, the opening.fli timed-event player, then the VESA mode is restored. */
int32_t fn_03f15f(void);   /* 0x03f15f: THE VESA MODE SET (mode 0x101, 640x480x8) -- fills DAT_531f4/531f8/63e4c/63e58/63e5c/63e60/63e64/63e68; returns 1, or 0 with a banner when VESA or the mode set fails. */
extern uint32_t g_dat_531f4;               /* DAT_000531f4: VESA row width (VBE[0x8]<<10), init'd by 0x03f15f */
extern uint32_t g_dat_63e64;               /* DAT_00063e64: VESA bytes-per-line/pitch (VBE[0x12]), init'd by 0x03f15f */
extern uint32_t g_dat_53104;               /* DAT_00053104: row-progress divisor (0x46 default, fn_03a5f7) */
extern uint32_t g_dat_63e4c;               /* DAT_00063e4c: palette color shift */
extern uintptr_t g_dat_53c5c;               /* DAT_00053c5c: DPMI far-pointer (debug-reg) */
extern uint8_t  g_dat_53c4c[9];            /* DAT_00053c4c: 9-byte real-mode command staging area */
extern uint8_t  g_dat_63e88[0x80];          /* DAT_00063e88: DMA/channel status (IRQ9 install) */
extern uint32_t g_dat_63f4c;               /* DAT_00063f4c: -1 sentinel (IRQ9 install) */
void     fn_03ee88(void);                   /* 0x03ee88: timer/IRQ tick handler (counter wraps) */
void     fn_043381(void);                   /* 0x043381: DPMI real-mode return (flat no-op) */
void     fn_04391d(void);                   /* 0x04391d: DOS/DPMI timer-vector setup (flat no-op) */
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
int32_t  fn_04247a(res_reader *r, int32_t value, int32_t mode);  /* 0x04247a: io write/seek, dispatch on flags/mode */
/* Digit value (0..35) -> ASCII char: 0-9 -> '0'-'9', 10-35 -> 'a'-'z'.
 * orig: 0x0427e7 */
int      digit_chr(int d);
void     tmp_filename(char *out, uint32_t index);   // 0x427f3 L1: "t<ticks4>_<idx2>.tmp"
/* Byte-sink callback for the streaming printf: store `byte` at st->pos,
 * advance st->pos, increment st->count.  orig: 0x0428e5 */
void     fmt_emit(fmt_state *st, uint8_t byte);
uint32_t fn_0428f8(uint8_t *buf, const uint8_t *fmt, uint8_t **src); /* 0x0428f8: streaming sprintf -> buf, NUL-terminate, return n */

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

/* ---- batch 6: heap-grow stub, echo input, dword fill ---- */

/* ---- heap (two-level DPMI allocator), batches 7-8 ------------------
 * Every heap pointer is a uint32 OFFSET from g_heap_base (the arena). This
 * preserves the original's exact byte layout (12-byte free blocks: size@0,
 * prev@4, next@8, user data @4; 44-byte region header) and the block+size
 * stride / 0xc minimum / size arithmetic byte-for-byte; only the pointer
 * VALUES are base-relative offsets (faithful 32->64-bit adaptation).
 * See docs/12-functions/heap.md.
 *
 * Region chain: linear NULL-terminated doubly-linked list (prev@4, next@8),
 *   sorted by offset, head = g_heap_head, cursor = g_heap_cursor.
 * Per-region free list: circular, sentinel at region+0x20, blocks at
 *   region+0x2c..region+total, 0xffffffff end-marker at region+total,
 *   traversal via next@8, size@0 with bit 0 = the allocated flag. */
extern uintptr_t g_heap_base;      /* base address of the heap arena */
/* Recover a host pointer from a 32-bit arena-relative offset. The original stored
 * these as 32-bit flat pointers; the port stores (buf - g_heap_base), which is a
 * SIGNED quantity whenever `buf` is a data-segment global that happens to sit
 * below the arena. Sign-extending (not zero-extending) makes the round-trip exact
 * for any global/arena link order, while being identical for in-arena offsets. */
static inline uint8_t *flat_ofs(uint32_t off) {
    return (uint8_t *)g_heap_base + (int32_t)off;
}
extern uint32_t  g_heap_head;      /* DAT_00053350: all-blocks chain head */
extern uint32_t  g_heap_cursor;    /* DAT_00053354: malloc search cursor */
extern uint32_t  g_heap_highwater; /* DAT_00053358: global size high-water */
extern uint32_t  g_heap_seg_limit; /* DAT_00053384: DPMI segment limit */
extern uint8_t   g_heap_cfg_b2;    /* DAT_000533b2: top-check mode byte */
extern uint8_t   g_heap_cfg_b3;    /* DAT_000533b3: top-check mode byte (0x0431b5) */
extern uint8_t   g_heap_cfg_b4;    /* DAT_000533b4: top-check flag */
extern uint8_t   g_dat_5340c;      /* DAT_0005340c: system-init "armed" flag (1 = run init) */
extern uint8_t   g_heap_flag;      /* DAT_00063fbd: wrappers zero it, never read */
extern uint32_t  g_dat_63e14;      /* DAT_00063e14: 2-node swap counter (fn_03e14c) */
extern uint32_t  g_dat_63e18;      /* DAT_00063e18: cached prev-node offset (fn_03e14c) */
extern uint32_t  g_dat_63e1c;      /* DAT_00063e1c: cached next-node offset (fn_03e14c) */
extern uint32_t  g_dat_63364;      /* DAT_00063364: removed-element flag byte (fn_031f5f) */
extern uint32_t  g_dat_63368;      /* DAT_00063368: removed-element value word (fn_031f5f) */
extern uint32_t  g_dat_63e20;      /* DAT_00063e20: 3rd cache slot (zeroed, not read) */
extern uint32_t  g_dat_62a8c;      /* DAT_00062a8c: zeroed by fn_03ee0a (list-destroy) */
extern uint32_t  g_ds_segment;     /* DPMI DS selector (flat model: 0) */

/* Arena support (the original grows via DPMI; the port uses a static arena). */
void     heap_arena_init(void);    /* set up the static arena + globals */
uint32_t heap_arena_alloc(uint32_t size); /* 64-byte-aligned region slot, or 0 */
/* The original grows the DPMI heap on demand (heap_grow 0x04322c, still a stub in the port). The port
 * arena is a fixed static block; the *usable* cap starts at NIGHT_HEAP_ARENA (1 MiB) so the historical
 * "arena full" behaviour of existing tests is preserved, and can be raised for real CD data (a stage's
 * .wal alone is ~0.8 MiB) up to the physical arena size. */
void     heap_arena_cap_set(uint32_t cap);
uint32_t heap_arena_cap(void);
/* Region granularity of the arena substitution: minimum data area of a freshly acquired region
 * (0 = size the region to the triggering request, the allocator tests' default). The DPMI original
 * acquires whole blocks (FUN_00043071) far larger than the request, so real data loads raise this to
 * avoid one region per allocation. */
void     heap_region_floor_set(uint32_t bytes);
uint32_t heap_region_floor(void);

/* Unlink a region from the all-blocks chain: advance head/cursor if it is the
 * node, then prev->next = next AND next->prev = prev, each NULL-guarded
 * (full double unlink, verified in disasm 0x042c8d).  orig: 0x042c8d */
void heap_unlink(uint32_t node);

/* Best-fit allocation within one region: walk the free list, split a block if
 * its remainder is >= 0xc else take the whole block, mark bit 0 (allocated);
 * returns the data offset (block+4), or 0.  orig: 0x042ada */
uint32_t heap_alloc(uint32_t region, uint32_t size);

/* Free a data offset within a region: forward-coalesce with the next block if
 * it is free, else re-insert address-sorted; run the fragmentation "rebalance"
 * walk; backward-coalesce if the predecessor is immediately adjacent; update
 * alloc/free counters + largest + high_water.  orig: 0x042b82 */
void heap_free(uint32_t region, uint32_t data);

/* Top check: if the first region's last free block ends exactly at the DPMI
 * segment limit, return its size (recomputing the limit from DS when
 * 2<=b2<=8 && b4==0); else 0.  orig: 0x042ea1 */
uint32_t heap_top_check(void);
/* 0x0431b5: heap size-adjuster (see f.c for the full 10-step algorithm). */
uint32_t heap_size_adjust(uint32_t *p);

/* ---- L1: registered-callback dispatcher (0x0423cf) ---- */
typedef uint32_t (*u32_cb_fn)(uint32_t);
extern u32_cb_fn g_dat_5336c;   /* DAT_0005336c: registered callback */
extern u32_cb_fn g_dat_53370;   /* DAT_00053370: registered callback */
extern u32_cb_fn g_dat_53374;   /* DAT_00053374: registered callback */
/* 0x0423cf: call g_dat_53370(input), call g_dat_53374(first callback's return),
 * then return the original input. The original calls both slots unconditionally
 * (a NULL slot faults, exactly as in the original). */
uint32_t fn_0423cf(uint32_t input);
void fn_0423b7(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4); /* 0x0423b7 */

/* Insert+init a freshly-placed region into the all-blocks chain (sorted by
 * offset) and set up its header, sentinel, one big free block for the whole
 * data area, and the end-marker. Returns the data offset (region+0x2c).
 * The caller must preset total_size at region+0x00.  orig: 0x042e2d */
uint32_t heap_insert_init(uint32_t region);

/* Create one usable region holding at least `size` bytes of data: arena slot +
 * preset total_size + insert_init + mark-big-block-allocated + alloc_count++ +
 * high_water=0xffffffff + free the whole block (the original's region-creation
 * pattern, which links the block into the free list). Port replacement for the
 * DPMI acquirer FUN_00043071 (DPMI -> arena). Returns the data offset
 * (region+0x2c+4), or 0 if the arena is full. */
uint32_t heap_acquire_region(uint32_t size);

/* malloc/free entry points (the wrappers), operating over the whole
 * all-blocks chain.  orig: 0x041af8 / 0x041c31 */
uint32_t heap_malloc(uint32_t size);
void     heap_free_host(uint32_t data);
void     heap_free_guard(uint32_t data);   // 0x41c28 L1: NULL-tolerant free (free if nonzero)

/* Outer memory-acquire stub: always fails (XOR EAX,EAX; RET).  orig: 0x04322c */
uint32_t heap_grow(uint32_t size);

/* DOS INT 21h AH=01 (cooked character input, with echo), sharing the same
 * 1-char cache g_keybuf as get_key (AH=08, no echo): returns the buffered key
 * if present, else a read from the echo input backend.  orig: 0x043263 */
uint8_t  get_key_echo(void);
/* Try to grow/acquire more heap memory of `size`; returns non-zero on success,
 * 0 on failure. Currently a stub that always returns 0 (heap never grows).
 * Sole caller: the malloc wrapper FUN_00041af8 (the outer memory-acquire step).
 * orig: 0x04322c */
uint32_t heap_grow(uint32_t size);
/* memsetw: fill `n` 32-bit words at `dst` with `value`; returns `dst + n`.
 * (dst is dword-aligned in practice; callers align first.)  orig: 0x0432b7 */
uint32_t *memsetw(uint32_t *dst, uint32_t value, uint32_t n);

/* ---- L0 batch A (9): real-mode/DOS stubs + command-line parser ----
 * The original builds real-mode register contexts and far-calls thunks
 * (3*idx + 0x43402) or issues DOS INT 21h to reach real-mode drivers (CD,
 * keyboard, 8253 timer). The modern flat port never switches to real mode and
 * provides that I/O through the sys backend, so these trampoline/DOS functions
 * are faithful no-op stand-ins (their observable protected-mode behaviour is
 * "do not transition"), and their I/O effect lives in sys. */
/* DPMI real-mode trampoline A: thunk = 3*thunk_idx + 0x43402, 8-byte register
 * context from `saved_ctx`.  orig: 0x04335c */
void rm_switch_a(uint32_t thunk_idx, const uint32_t *saved_ctx);
/* DPMI real-mode trampoline B: thunk keyed by `key`, 32-byte register context
 * from `saved_ctx`.  orig: 0x0433ce */
void rm_switch_b(uint32_t key, const uint32_t *saved_ctx);
/* FSAVE [dst]; WAIT: real-mode FPU context save. No-op in the flat model.
 *  orig: 0x043979 */
void fpu_state_save(uintptr_t dst);
/* DOS INT 21h AH=25h: install the int 0x35 (8253 soft-timer) handler. The
 * modern port installs its timer via SDL; faithful sys stub.  orig: 0x043958 */
void dos_install_timer_int(void);
void     fn_0438a3(void);                    /* 0x0438a3: DPMI/DOS system-init dispatcher (flat: clear g_dat_5340c) */
void     fn_041fa1(void);                    /* 0x041fa1: JMP thunk -> fn_0438a3 */

/* Command-line parser context. The original passes a struct with a next-char
 * callback at +0x00, an emit callback at +0x04, a parsed number at +0x0c, and
 * a flags byte at +0x10. */
struct cmd_ctx {
    uint16_t (*next)(struct cmd_ctx *);        /* +0x00 */
    void (*emit)(uint16_t, struct cmd_ctx *);  /* +0x04 */
    uint32_t _pad;                             /* +0x08 */
    uint32_t num;                              /* +0x0c */
    uint8_t  flags;                            /* +0x10 */
};
/* Parse a DOS command line: an optional leading '*', an optional decimal
 * number, then a run of switches (N=0x08, F=0x04, L=0x40, h=0x10, l/w=0x20).
 * ctx->flags starts as (init & 3) | 1 (bit 0 cleared if leading '*');
 * ctx->num is the parsed number or 0xffffffff. Returns the position just past
 * the last consumed char.  orig: 0x043ca0 */
const char *cmdline_parse(const char *line, struct cmd_ctx *ctx);
/* Count successive chars returned by ctx->next() whose *next* char-class
 * (class[(c+1)&0xff]) has bit 0x02 (whitespace) set; unless ctx->flags & 0x2,
 * call ctx->emit(last_char, ctx) once with the final char. Returns the count.
 *  orig: 0x043d79 */
uint32_t cmdline_count(struct cmd_ctx *ctx);

/* ---- L0 batch B (9): buffered emit + event dispatch ---- */
/* Context for the emit_* family: a next-char callback at +0x00, a count at
 * +0x0c, and a flags byte at +0x10 (the original's emit context struct). */
struct emit_ctx {
    uint16_t (*next)(struct emit_ctx *);   /* +0x00 */
    void (*emit)(uint16_t, struct emit_ctx *); /* +0x04: consume/putback one char */
    uint32_t _r2;                          /* +0x08 */
    uint32_t count;                        /* +0x0c */
    uint8_t  flags;                        /* +0x10 */
};
/* Buffered char emit: read a destination address from the position stream at
 * *pos (near: dest = **pos, advance 1 slot; far 0x4: dest = second slot of an
 * 8-byte {seg,addr} pair, advance 2) when flags&0x1, then write up to
 * ctx->count chars from ctx->next() to it. Flags: 0x1 write, 0x4 far,
 * 0x8/else near, 0x2 peek (stop after the first read), 0x20 word (2-byte
 * writes). Returns chars processed.  orig: 0x043dae */
uint32_t emit_chars(struct emit_ctx *ctx, uintptr_t **pos);
/* Emit a single value to the destination read from *pos (same 0x1/0x4/0x8
 * dest convention); a word when flags&0x10, a dword otherwise.  orig: 0x043f22 */
void emit_value(struct emit_ctx *ctx, uintptr_t **pos, uint32_t value);
/* Decrement ctx->count; if the previous count != 0, call ctx->next() once and
 * return its result, else return 0xffffffff (also returned when flags&0x2).  orig: 0x0445a6 */
uint32_t emit_step(struct emit_ctx *ctx);
/* Emit-subsystem numeric tokenizer. Skips leading chars whose class
 * (kCharClass[(c+1)&0xff]) has bit 0x2 (whitespace); unless flags&0x2 (peek),
 * decrements ctx->count (0 -> emit+return with no value), reads an optional
 * sign (+/-, only when allow_sign != 0), auto-detects the base (0 -> 0x/0X
 * prefix = 16, a lone leading 0 = 8, else 10; else uses the given base, still
 * tolerating a 0x prefix when base==16), accumulates hex_digit_value digits,
 * and (if flags&0x1 and at least one digit) stores the (optionally negated,
 * word/dword per flags&0x10) value through the value-buffer stream cursor at
 * **pos (mirroring emit_value: dest = **pos; *pos += 1). The
 * trailing non-digit char is passed to ctx->emit (consuming it) unless EOF. In
 * the flat model the original's near/far store variants collapse to one
 * *(uint32_t *)dest = value (or uint16 when flags&0x10). Returns total chars
 * consumed (skip + sign + prefix + digits).
 *  orig: 0x044387 */
int fn_044387(struct emit_ctx *ctx, uintptr_t **pos, int base, int allow_sign);
uint32_t fn_043fc3(struct emit_ctx *ctx, uintptr_t **pos, const uint8_t **src);
uint32_t fn_043e34(struct emit_ctx *ctx, uintptr_t *pos);
int32_t fn_043a7f(struct emit_ctx *ctx, const char *fmt, uintptr_t **pos); /* 0x043a7f: format-string scanner driving the emit/parse family (%d %i %o %u %x %c %n %s %[ %e/%f/%g) */
uint32_t fn_0439fd(int32_t ch, uint8_t *node);   /* 0x0439fd (untracked): push one char back into an io node buffer; -1 when refused */
int32_t fn_041ffc(uint8_t *node, const char *fmt, uintptr_t **pos); /* 0x041ffc: stream-scan front end (fixed ctx, scanner 0x043a7f) */
int32_t fn_042023(uint8_t *node, const char *fmt, uintptr_t *slots); /* 0x042023: scanf-style entry; slots = destination address list */
extern uint8_t g_dat_62ac8[0xc8];   /* DAT_00062ac8: per-id byte latch (written by 0x032464) */
extern uint8_t g_dat_62b90[0xc8];   /* DAT_00062b90: per-id byte latch (written by 0x032464) */
uint8_t *fn_032464(uint8_t *block, const char *name); /* 0x032464: numeric text-table loader from the g_639b8 stream */
/* 0x0440a9: stream token parser -> list append (241 insn). Reads a
 * whitespace-delimited token from ctx, parses int/frac/exp/sign, appends a
 * 4/8-byte element to the *pos list-pool cursor, returns the char count.
 * The 0x53648 converter slot (g_fp_0442fa, default fn_045dad) is invoked for
 * the non-integer (flags&0x10 clear) output path. */
uint32_t fn_0440a9(struct emit_ctx *ctx, uintptr_t **pos);
typedef void (*fp_conv_t)(const char *buf, uint64_t *out);
extern fp_conv_t g_fp_0442fa;   /* the 0x53648 string->64-bit converter slot */
void fn_042ccc(void);
uint32_t fn_042eef(uint32_t arg);
uint32_t fn_042fc2(uint32_t size);   /* 0x042fc2: DPMI/DOS heap alloc (INT 0x21 AH=48); flat no-op, returns 0 */
uint32_t fn_043071(uint32_t size);   /* 0x043071: DPMI/DOS core heap acquirer; flat no-op, returns 0 */
uint32_t fn_042d2a(uint32_t arg1, uint32_t arg2);
uint32_t fn_048fe6(uint32_t arg1, uint32_t arg2);
int32_t fn_045bb7(uint32_t size);            /* 0x045bb7: DOS far-memory alloc/resize/verify; flat model always fails -> g_dat_64008=5, return -1 */
int32_t fn_04a814(int32_t start, const uint32_t *key);  /* 0x04a814: upper-bound scan over powers-of-10 table @0x04a928 */
int32_t fn_04a367(uint32_t arg0, uint32_t arg1, int32_t arg2, uint32_t *arg3, uint32_t eax_in); /* 0x04a367: *arg3(out) = scaled mantissa of the 10^(arg2-eax_in)-scaled double; return = fn_04a814(arg2, arg3) */
char *fn_04a48f(double d, uint32_t width, uint32_t *p_cnt, uint32_t *p_flag, uint32_t format, char *out); /* 0x04a48f: double -> fixed/exponent decimal-ASCII string; returns out */
int32_t fn_049e80(uint8_t *dest, const double *srcp, int32_t E, int32_t plus, int32_t W,
                  int32_t P, int32_t M, uint32_t radix, uint32_t fmt); /* 0x049e80: printf-style float formatter (G/E/F, '*' overflow, right-align) */
uint8_t *fn_045c79(uint8_t *dest, uint8_t *rec, uint8_t **argp); /* 0x045c79: printf float conversion handler (E/F/G/e/f/g via 0x049e80 + 0x045d56) */

/* Event-dispatch table (DAT_000537dc): 10 entries of {flag(2=done), key, fn}.
 * fn pointers reference game handlers (wired when those are ported; NULL until
 * then). Original targets: 0x418e6,0x43702,0x41f9c,0x439bd,0x4561d,0x4589e,
 * 0x4933a,0x4aa49,0x41fa1,0x493b2. */
struct ev_entry { uint8_t flag; uint8_t key; void (*fn)(void); };
extern struct ev_entry g_evtab[10];
/* Dispatch over entries 0..7: each pass selects the last not-done entry whose
 * key is <= the rolling threshold (starts at `key`, drops to each match's key),
 * fires it, marks it done, and re-scans until none match.  orig: 0x044618 */
void dispatch_a(uint32_t key);
/* Dispatch over entries 8..9: reversed comparison (entry.key >= rolling AL) and
 * a DH threshold (edx low byte) gating the fire; always marks the selected
 * entry done.  orig: 0x044663 */
void dispatch_b(uint32_t eax_arg, uint32_t edx_arg);

/* ---- L0 batch C (9): char/string/count primitives + DPMI constants ---- */
/* Uppercase ASCII letter (0x41-0x5a) -> lowercase (+0x20); else unchanged.  orig: 0x044859 */
int to_lower(int c);
/* Count non-NUL bytes in `src`, capped at `limit` (strnlen).  orig: 0x045032 */
size_t strn_len(const uint8_t *src, size_t limit);
/* Count non-zero 16-bit words in `src`, capped at `limit` words.  orig: 0x045056 */
size_t word_nlen(const uint16_t *src, size_t limit);
/* DPMI host linear-base constants (no meaning in the flat model, preserved for
 * 1:1).  orig: 0x045a6a / 0x045a70 */
uint32_t const_64008(void);
uint32_t const_64004(void);
/* globals written by the 0x42a08/0x42a0d/0x42a30 family */
extern uint32_t g_dat_64008;    /* DAT_00064008 */
extern uint32_t g_dat_53619;    /* DAT_00053619: 0x200 => default binary file mode */
extern uint32_t g_dat_64004;    /* DAT_00064004 */
extern uint8_t  g_dat_533bb;    /* DAT_000533bb: classify-enable threshold (>2) */
extern u32_cb_fn  g_dat_533bd;  /* DAT_000533bd: callback slot (0x042a3c) */
void     fn_042a08(void);       /* 0x042a08: g_dat_64008 = 13 */
void     fn_042a0d(uint32_t x); /* 0x042a0d: g_dat_64008 = x */

/* 0x041e94: pure JMP thunk to 0x041bdc (heap_malloc). Same reg-ABI (EAX=size). */
uint32_t fn_041e94(uint32_t size); /* 0x041e94: heap_malloc alias */

/* 0x04123d: device-port init. canary_id(0,0x20); outb(g_dat_53334+6,1); 10x
 * inb(g_dat_53334+6); outb(g_dat_53334+6,0). No stack args (RET 0). */
void fn_04123d(void);              /* 0x04123d: device-port init loop */

/* 0x0412e9/0x041390: CD-ROM/device status spin-polls (no args). Both poll
 * inb(g_dat_53334+off) up to 0x7fff times. 0x0412e9 (+0xe) returns 1 when the
 * polled byte has bit 0x80 SET; 0x041390 (+0xc) returns 1 when bit 0x80 CLEAR;
 * both return 0 on the 0x7fff-iter timeout. */
uint32_t fn_0412e9(void);          /* 0x0412e9: poll +0xe for bit7 set   */
uint32_t fn_041390(void);          /* 0x041390: poll +0xc for bit7 clear */
uint32_t fn_0412a3(void);          /* 0x0412a3: ready-gated inb(base+0xa) status read */
uint32_t fn_041340(uint32_t cmd);  /* 0x041340: ready-gated outb(base+0xc, cmd) write */
uint32_t fn_040efd(void);  /* 0x040efd: thin wrapper -> fn_041340(0xd0) */
uint32_t fn_0413e7(void);  /* 0x0413e7: thin wrapper -> fn_041340(0xd1) */
uint32_t fn_041410(void);  /* 0x041410: thin wrapper -> fn_041340(0xd3) */
/* 0x0353d0 device-command queue: arg is a cursor struct; head = orig 4-B pointer widened to 8 B
 * (pointer-width adaptation -> f4/f5 shift from orig offset 4/5 to 8/9). */
struct dev_cmd_ctx {
    uint32_t head_off;  /* +0: 4-byte g_heap_base-relative cursor (rec[0]) */
    uint8_t f4;         /* +4: cmd byte  (rec[4]) */
    uint8_t f5;         /* +5: tag byte  (rec[5]) */
};
void fn_0353d0(uint8_t cmd, struct dev_cmd_ctx *arg); /* 0x0353d0: MIDI/device command dispatcher (offset cursor) */
void fn_035263(uint8_t *base, uint8_t *ctx);          /* 0x035263: MIDI sequencer step -- advance min track (idx=ctx[0x14]), re-find min via fn_035329 */
void fn_034e3c(void);                                  /* 0x034e3c: MIDI sequencer control loop -- advance tracks while ctx[0xc]>=ctx[0x10], then parse-next (fn_034f31) or teardown (fn_034eb0) */
int32_t  fn_035c2d(uint8_t arg);  /* 0x035c2d: thin wrapper -> fn_0358c7((uint8)arg) */
int32_t  fn_035a19(int32_t a0, int32_t a1, int32_t a2);  /* 0x035a19: 3-byte write (a0,a1,a2-if>=0) via fn_0358c7; -1 on first timeout */
void     fn_035329(uint8_t *a0, uint8_t *a1);  /* 0x035329: scan 0xe-byte records (tag@+0x5,val@+0xa) for min -> a1{+0x10 cur,+0x14 idx,+0x20 flag}; a0/a1 raw ptrs */
int32_t  fn_035bf4(uint8_t al, uint8_t dl);  /* 0x035bf4: thin -> fn_035a19(al,dl,-1) (3rd byte skipped) */
void     fn_035ac0(void);                    /* 0x035ac0: device-init, for i in 0..15 fn_035a19(i+0xb0,0x78,0) */
int32_t  fn_0357f2(uint8_t a0);              /* 0x0357f2: device read w/ status handshake; -1 phase1 timeout, -2 data==0xfe, 0 ok */
int32_t  fn_035941(uint32_t a0);             /* 0x035941: device init handshake: g_530ac=a0; retry fn_0357f2(0xff) then fn_0357f2(0x3f); 0 on success else -1 */
int32_t  fn_0359c1(void);                    /* 0x0359c1: probe device bases 0x300,0x310,0x320,0x330: first fn_035941(base)==0 -> 0 else -1 */
int32_t  fn_035a94(void);                    /* 0x035a94: passthrough -> fn_0359c1() */
int32_t  fn_035b91(uint8_t a0, uint8_t a1, uint8_t a2);  /* 0x035b91: clamp (a2*g_dat_530b8)/100 low byte to 0x7f, fn_035a19(a0,a1,clamped) */
int32_t  fn_035b0c(uint8_t a0, uint8_t a1, uint8_t a2);  /* 0x035b0c: (a0&0xf0==0xb0&&a1==7)? g_530bc[a0&0xf]=a2; fn_035b91 : fn_035a19 */
void     fn_034eb0(void);                    /* 0x034eb0: device/IRQ teardown: if g_63324 {irq_cli;fn_03575d(0);isr_set(8,g_63334,g_63338);fn_035ac0;irq_sti;g_63324=0; if g_6333c {fn_041c31;g_6333c=0}} */
void     fn_034c40(void);                    /* 0x034c40: IRQ8 tick handler: seg_ds_load;irq_cli;fn_034e3c;g_6330c++; if g_6332c>=0x10000 {call [g_63334]; g_6332c-=0x10000}; g_6332c+=g_63318; outb(0x20,0x20); irq_sti */
int32_t  fn_03565a(int32_t input);  /* 0x03565a: div=input/1000; r=div==0?0x10000:(div*0x4aa)/g_dat_63304; fn_000356ba(r); return r */
uint32_t fn_040948(uint32_t arg);  /* 0x040948: fn_041340(0x40); return fn_041340(arg) */
void fn_040e8d(void);  /* 0x040e8d: if g_63f90 {fn_000407f8();isr_set(local,0,0);fn_041410()} */
extern uint32_t g_63f90;  /* DAT_00063f90: timer/device init flag (0x040e8d guard) */
extern uint32_t g_dat_63f88;  /* DAT_00063f88: 16-bit device value (0x0411d9) */
uint32_t fn_0411d9(void);  /* 0x0411d9: if fn_041340(0xe1) { combine 2x fn_0412a3 as (hi<<8)|lo -> DAT_63f88 } else 0 */
uint32_t fn_040f4f(void);  /* 0x040f4f: scan 8 port bases (g_dat_53334 += 0x10); return base where fn_0412a3()==0xaa else 0 */
int32_t  fn_0358c7(uint8_t arg);   /* 0x0358c7: wait +1 bit0x40 clear, outb(base,arg); -1 on 0x2000 timeout */
extern uint32_t g_dat_530ac;       /* DAT_000530ac: device port base */
uint8_t *fn_0326c1(uint8_t *base); /* 0x0326c1: free per-element heap offsets at +0x68/+0x1f8, return base */
void     fn_042a30(uint32_t x); /* 0x042a30: g_dat_64004 = x */
uint32_t fn_042a3c(uint32_t x); /* 0x042a3c: return g_dat_533bd(x) */
/* 0x047d24: register-ABI wrapper around fx_mul64 (a=EAX, b=EDX, out=EBX). */
void fn_047d24(uint32_t *a, uint32_t *b, uint32_t *out);

/* FPU-interp (0x0461cc) caseD opcode handlers: fr=EBP frame base, esi/edi =
 * operand byte-offsets into the EBP+0x1c operand area (operand = fx128 layout
 * lo@0/hi@4/exp@8). Transcribe the 3 LEA + CALL 1:1; the `JMP [EBP+0x76]`
 * tail-dispatch (owned by the 0x0461cc interpreter) is omitted. */
void fn_046d32(uint8_t *fr, uint32_t esi, uint32_t edi); /* caseD_0: fn_0478f0(a=out=+EDI, b=+ESI) */
void fn_046d47(uint8_t *fr, uint32_t esi, uint32_t edi); /* caseD_8: fn_047d24(a=out=+EDI, b=+ESI) */
void fn_0473e5(uint8_t *fr, uint32_t esi, uint32_t edi); /* caseD_4: fn_0478f0(a=out=+ESI, b=+EDI) */
void fn_044bb4(uint8_t *node);   /* 0x044bb4: ioctl node flag-setter */
uint32_t fn_042a43(uint8_t *node);   /* 0x042a43: device/heap node initializer (size by flags, malloc+inline fallback) */
int32_t fn_042928(uint8_t *node);    /* 0x042928: node lseek-offset compute (lseek pos +/- field4 by flag 0x10) */
int32_t fn_04285c(uint8_t *a0, int32_t a1);  /* 0x04285c: io_state/node destructor (flush/lseek/close/free/filename) */
uint32_t fn_042717(uint32_t a0);  /* 0x042717: find node with rec==a0: active+flags&0x3 -> fn_04285c(rec,1); done -> reactivate; else g_64008=4, return 0 */
int32_t fn_0427d2(uint8_t *a0, int32_t a1);  /* 0x0427d2: r=fn_04285c(a0,a1); node_deactivate(a0); return r */
int32_t fn_0427a3(uint32_t a0);  /* 0x0427a3: walk active list; node rec==a0 -> fn_0427d2(base+rec,1); else -1 */
extern uint8_t  *g_ndpool;          /* node pool base (offset 0 = NULL sentinel) */
extern uint32_t g_node_list_active; /* 0x63ffc: active list head (offset; 0=empty) */
extern uint32_t g_node_list_done;   /* 0x63ff8: done list head (offset; 0=empty) */
extern uint8_t  g_dat_53451;         /* DAT_00053451: file-structure flag */
void     fn_04933a(void);            /* 0x04933a: rebuild active node list from file-struct table @0x53410 */
extern uint32_t g_dat_53c60;         /* DAT_00053c60: directory entry count */
extern uint32_t g_dat_53c64;         /* DAT_00053c64: dir-entry buffer heap offset */
void     fn_010927(void);            /* 0x010927: CD/MSCDEX root-directory read */
/* Global set by the DOS4GW entry (0x420bc) during startup init; returned by
 * get_dat_53390.  orig: DAT_00053390 */
extern int g_dat_53390;
int get_dat_53390(void);   /* orig: 0x044be6 */
extern char **g_dat_537d4;   /* DAT_000537d4: array of "key=value" char*, NUL-terminated */
/* Indirect-call thunk through callback slot DAT_00053644. Port guards against
 * NULL (the original calls unconditionally).  orig: 0x0451b5 */
extern uint32_t (*g_cb_53644)(void);
uint32_t cb_indirect_53644(void);

/* ---- L0 batch D (9): node list, modifier-prefix parser, debug hook ---- */
/* Node for the active/done node lists (8-byte original layout): next @+0, record
 * offset @+4. Records carry flags at offset +0xc. Both nodes and records live in
 * the node pool referenced by g_ndpool (offsets, 0 = null).  orig layout: node
 * {next,rec}, list heads DAT_00063ff8 (done) / DAT_00063ffc (active). */
struct nd_node { uint32_t next; uint32_t rec; };
extern uint8_t  *g_ndpool;         /* base of the node/record pool */
extern uint32_t  g_node_list_active; /* DAT_00063ffc: head offset */
extern uint32_t  g_node_list_done;   /* DAT_00063ff8: head offset */
/* Find the active-list node whose record == rec_off, mark that record's flags
 * |= 0x3, unlink the node from the active list, and push it onto the front of
 * the done list. No-op if not found.  orig: 0x044b5d */
void node_deactivate(uint32_t rec_off);

/* 0x044ab5: file-structure record allocator. Allocates a 0x1a-byte record from the
 * 26-entry pool at g_ndpool+0x53410 (heap fallback / in-heap overflow), wires a fresh
 * node onto the front of the ACTIVE list. Returns the record pointer, or NULL (and
 * g_dat_64008=5) on allocation failure. */
uint8_t *fn_044ab5(void);

/* 0x043234: walk the active node list (head g_node_list_active, offset-based via
 * g_ndpool). For each node, rec = base + node->rec (the io_state). If (rec[0xc] &
 * mask) count++; if additionally (rec[0xd] & 0x10) call fn_044748(rec) (flush).
 * Returns the count of matching records. */
uint32_t fn_043234(uint32_t mask);
uint32_t fn_049192(uint32_t ch, uint8_t *node);   /* 0x049192: putc to node I/O buffer (mask is ECX; CRLF expand; fn_044748 flush) */
uint32_t fn_0446b2(uint8_t *p, uint32_t ch);      /* 0x0446b2: putc to node@[p]+heap_base, then INC [p+0x10] (write counter) */
uint32_t fn_041c8f(uint8_t *node);              /* 0x041c8f: getch from node buffer (CR-consume, EOF) */
int32_t fn_041d18(uint8_t *node, uint32_t param_2); /* 0x041d18: pull one byte from node buffer (fn_041d47 + advance) */
int32_t fn_041d47(uint8_t *node, uint32_t param_2); /* 0x041d47: I/O fill step (key-echo or device read into node buffer) */
uint32_t fn_04192e(uint8_t *dest, uint32_t count, uint32_t mult, uint8_t *node); /* 0x04192e: stream read node->dest (byte/bulk); \r\n + 0x1a handling; returns total_read/count */
void fn_03fb73(uint8_t *node, uint32_t size); /* 0x03fb73: RLE screen-block expander into video (g_dat_51a0c); fills buf via fn_04192e */
void fn_03fa3b(uint8_t *node, uint32_t size); /* 0x03fa3b: 200-row RLE screen decoder into video (per-row op count, ctl<0x80 FILL / ctl>=0x80 COPY) */
void fn_035f1b(const char *name, uint32_t dl); /* 0x035f1b: screen + 768 B font-table loader (fn_04192e table >>2; RLE via fn_041c8f into g_video_ram) */
void fn_036041(const char *name, uint32_t dl); /* 0x036041: screen loader w/ 0xff-skip RLE (FILL value==0xff / literal 0xff skip) */
void fn_036190(const char *name, uint8_t *dest); /* 0x036190: screen RLE loader into caller 64000-byte dest buffer (no board_out); table g_dat_633b8 */
void fn_036417(const char *name, uint8_t *dest, uint32_t dl); /* 0x036417: screen RLE loader into caller 0x4b000-byte dest; dl->board_out; table g_dat_636b8 */
void fn_03628e(const char *name, uint32_t dl);               /* 0x03628e: screen RLE loader into the 0xa0000 video window (wraps at 0x10000, fn_03f3be fade per wrap); dl->board_out */
void fn_03fb0e(uint8_t *node, uint32_t count); /* 0x03fb0e: load 64000 B screen image from I/O node into g_dat_51a0c via heap buf + fn_04192e */
uint32_t fn_03f6bc(const char *name);   /* 0x03f6bc: THE TIMED EVENT PLAYER -- 0x80 B header tagged 0xaf11 at +4, MOVsx count word at +6, frame waits on DAT_63e40, per-tick sound-slot starts via DAT_53200/DAT_532f4 -> DAT_53d0c -> fn_04185e, resource ops through fn_03f80d; 0 = tag/count, 1 = key cut or op stream done */
extern uint32_t g_tab_53200[0x3d];      /* 0x53200: the timed-event tick table (61 dwords; DAT_53200 + 61*4 = DAT_532f4 exactly); no producer inside the EXE -- seedable, awaits the CD backend */
extern uint8_t  g_tab_532f4[0x44];      /* 0x532f4: byte table of DAT_53d0c sound-slot indices (68 bytes, up to DAT_53338); no producer inside the EXE -- seedable */
extern uint8_t  g_dat_63ec1;            /* 0x63ec1: key-pressed byte (spin/abort in the picture + timed-event drivers) */
extern uint8_t  g_dat_63ea5;   /* DAT_00063ea5: third key byte (key waits in 0x03a224 / 0x038cde) */
extern uint8_t  g_dat_63ec0;   /* DAT_00063ec0: fourth key byte (same key waits) */
extern uint8_t  g_dat_63ea4;            /* 0x63ea4: second key byte (same gates) */
extern uint8_t  g_dat_63e89;            /* 0x63e89: abort/quit byte (unwinds the slide show and the timed event play) */
uint32_t fn_03f80d(res_reader *node);  /* 0x03f80d: resource-op block gate (16 B header, tag 0xf1fa at +4, int16 count at +6 -> fn_03f885 N times; 0 on tag mismatch) */
void fn_03f885(res_reader *node);              /* 0x03f885: resource-op dispatcher (6B header: dword size + word opcode -> palette/screen handlers; default fn_04247a(node,size-6,1)) */
void fn_03fc1d(uint8_t *node, uint32_t size); /* 0x03fc1d: VGA-palette DAC load (read 4B hdr + size-4 B table; board_out(hdr[2], 0x100-hdr[2], table)) */
/* Parse a modifier-prefix string into a flags byte at out+0x14 (and out+0x16):
 * '-' |=0x8, '#' |=0x1, '+' |=0x4 & &=~0x2, ' ' |=0x2 (only if 0x4 unset),
 * '0' -> out+0x16 = '0'. Stops at the first other character.  orig: 0x044fd7 */
const uint8_t *parse_mod_prefix(const uint8_t *s, uint8_t *out);
/* 0x044ea4: parse a numeric conversion specifier (width / precision / suffix). */
uint8_t *fn_044ea4(const uint8_t *s, uint32_t **va, uint8_t *spec);
uint32_t fn_044c23(uint8_t **src, void (*cb)(uint8_t *st, uint8_t ch), uint8_t *buf, const uint8_t *fmt); /* 0x044c23: format-string field processor */
/* 0x049b84: parse a [J|M]N[.N[.N]][/month:minute:second] date/time string into
 * a 9-dword struct; returns the position after the parsed string. */
const char *fn_049b84(const char *s, uint32_t *out);
void     fn_049a35(uint32_t p1, uint32_t p2, uint32_t p3); /* 0x049a35: TZ config parser (params unused) */
void     fn_049c76(const char *s, uint32_t p2, uint32_t p3, uint32_t p4); /* 0x049c76: TZ range parser (direct string) */
uint64_t fn_045a82(uint32_t param_1, uint32_t param_2); /* 0x045a82: byte classifier (g_dat_64004/64008) */
/* 0x044a50: reg-ABI EAX=index, EDX=ctx. Lazy-init the node (fn_0492d0), mark the
 * table slot (store_arr53730 low|0x40); if bit13 set, two AH=0x44 ioctls with
 * RCL/ROR sign gates -> fn_045a82 error (returns -1) else 0. */
int32_t fn_044a50(uint32_t index, uint32_t ctx);        /* 0x044a50 */
/* 0x049236: reg-ABI EAX=index, EDX=ctx, EBX=count. fn_0492d0 lazy-init; if AL
 * bit7, AH=0x42 lseek(end) sign-gate -> fn_045a82 (-1); AH=0x40 write sign-gate
 * -> fn_045a82; if written!=count fn_042a0d(0xc); return written. */
int32_t fn_049236(uint32_t index, uint32_t ctx, uint32_t count);  /* 0x049236 */

/* 0x14-byte I/O state struct used by 0x044748: f4=count, f8=buffer, f12=flags
 * (byte0 @0x0c, byte1 @0x0d), f16=handle. */
typedef struct {
    uint32_t f0;
    uint32_t f4;
    uint32_t f8;
    uint32_t f12;
    uint32_t f16;
} io_state_t;
/* 0x044748: reg-ABI EAX=io_state_t*. Flush (f13 bit4) or seek-back (else); tail sets f0=f8, f4=0. */
int32_t fn_044748(io_state_t *in);
int32_t fn_0446c3(io_state_t *node, const uint8_t *fmt, uint8_t **src); /* 0x0446c3: printf-to-node sink (fn_044c23 + fn_0446b2); reg-ABI EAX=node,EDX=fmt,EBX=src */
/* DAT_0005342a console io node. The original node extends past the 0x14-byte core
 * struct: fn_049192 uses the dword at +0x14 as the buffer capacity (flush when
 * f4 == that field), so the global needs 0x18 bytes of storage. */
typedef struct { uint32_t f0, f4, f8, f12, f16, cap; } io_node_full_t;   /* 0x18 */
extern io_node_full_t g_dat_5342a;                       /* DAT_0005342a: global console printf io node */
void fn_042394(const uint8_t *fmt, uint8_t *varargs);   /* 0x042394: fixed-node printf wrapper -> fn_0446c3(&g_dat_5342a,...) */
void fn_036537(uint8_t *varargs);                       /* 0x036537: boot/console init sequence (DPMI/IRQ setup + banner printf + dispatcher) */

/* 0x04488b: reg-ABI, 4 stack args (name, mode, extra, val). File open/create with
 * node-table registration; returns handle (16-bit) or fn_045a82(-1) on error. */
int32_t fn_04488b(const uint8_t *name, uint16_t mode, uint8_t extra, uint32_t val);
/* 0x044867: 4 stack args (p1, p2, name, val); p1/p2 unused. Thin wrapper ->
 * fn_04488b(name, 0, val&0xff, val). */
int32_t fn_044867(uint32_t p1, uint32_t p2, const uint8_t *name, uint32_t val);
uint64_t fn_045a76(uint32_t param_1, uint32_t param_2); /* 0x045a76: fn_045a82 thunk (param_2==0 -> 0) */
uint64_t fn_042576(const char *s, uint32_t param_2); /* 0x042576: file-mode parser (r/w/a + +/-/b/t) */
res_reader *fn_04262a(const uint8_t *name, const char *modestr, res_reader *node, uint8_t extra); /* 0x04262a: open file into 0x18-byte io node; NULL on failure */
res_reader *fn_0426f2(const uint8_t *name, const char *modestr, uint8_t extra); /* 0x0426f2: alloc record (fn_044ab5) + open */
res_reader *fn_04270d(const uint8_t *name, const char *modestr); /* 0x04270d: fn_0426f2 with extra=0 */
uint32_t fn_03c72b(uint32_t ent);   /* 0x03c72b: recursive entity teardown (offset ABI): free child via 0x41c28, free+clear +0x348 and +0x44 via 0x41c31 */
void fn_03d0c0(uint8_t *ent);       /* 0x03d0c0: build 200 row-interval lists (4 branches) via fn_03ba2c; clears ent[0x1e]/[0x22] */
int32_t fn_0417f3(const char *name, uint8_t *dest); /* 0x0417f3: resource payload (skips 0x1a header) */

/* ---- L10 trivial accessors (canary prologue not replicated) ---- */
extern uint32_t g_dat_53c60;    /* DAT_00053c60 */
extern uint32_t g_dat_40010;    /* DAT_00040010 */
void     fn_043280(uint8_t *dest, uint32_t pattern, uint32_t count);   /* 0x043280: byte-pattern fill */
extern uint32_t g_dat_62d60;    /* DAT_00062d60: node-arena base (s_node_arena in the port) */
extern uint32_t g_dat_62d64;    /* DAT_00062d64 */
uint32_t fn_010e15(void);                /* 0x010e15: return g_dat_53c60 */
uint32_t fn_040631(uint32_t param);      /* 0x040631: g_dat_40010 = low16(param); return low16 */
extern uint32_t g_dat_40014;   /* DAT_00040014 (low16 slot) */
extern uint32_t g_dat_40018;   /* DAT_00040018 (low16 slot) */
extern uint32_t g_dat_4001c;   /* DAT_0004001c (low16 slot) */
extern uint32_t g_dat_40020;   /* DAT_00040020 (low16 slot) */
void fn_0405de(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);  /* 0x0405de: store 4 low16 to 0x40014..0x40020 */
uint32_t fn_01520b(const uint8_t *p);    /* 0x01520b: return *(uint32*)(p+5) */
uint32_t fn_034a87(void);                /* 0x034a87: v=g_dat_62d64; g_dat_62d64+=11; return v */
void fn_034990(void);                    /* 0x034990: node-pool/video-arena init (200 heads) */
void fn_03dcbd(uint32_t *a0);            /* 0x03dcbd: struct destructor -- free 14 sub-obj heap-objs at +0x4..+0x38 (each if non-zero), null */
int32_t fn_03d9cf(uint32_t *s, const char *name); /* 0x03d9cf: two-pass parallel-array table loader (headers->s[1..12], record offsets->s[13], payload->s[14]); returns 1 */
uint32_t *fn_03d8d4(uint32_t *s, const char *name); /* 0x03d8d4: clear s[1..14] then fn_03d9cf; return s */
uint32_t *fn_03d999(uint32_t *a0, uint32_t a1); /* 0x03d999: wrapper -- fn_03dcbd(a0); return a0 (a1 unused) */
uint32_t fn_03de85(uint32_t *s, uint32_t count, uint32_t last_size); /* 0x03de85: struct ctor -- free old (fn_03dcbd), alloc 14 sub-objs at +0x4..+0x38, return 1 */
extern uintptr_t g_node_arena_base;    /* node-arena base (s_node_arena) */
extern uint32_t  g_dat_62d68[200];      /* DAT_00062d68: head-node offset per list idx (200 lists) */

/* Video-RAM model (flat 0xa0000 VGA segment): 64 KB window [0xa0000,0xb0000). */
extern uint8_t g_video_ram[0x10000];
extern uint8_t *g_dat_51a0c;     /* DAT_00051a0c: raw video base (0xa0000) -> g_video_ram (fn_03fb73) */
extern uint8_t g_dat_636b8[0x300]; /* DAT_000636b8: 768 B font/glyph table (fn_035f1b loads + >>2) */
extern uint8_t g_dat_633b8[0x300]; /* DAT_000633b8: 768 B font/glyph table (fn_036190 loads + >>2) */
extern uint32_t g_dat_63e7c;     /* DAT_00063e7c: RLE screen-buffer offset (fn_03fb73) */
extern uint32_t g_dat_63e78;     /* DAT_00063e78: RLE row-count counter (fn_03fb73) */
void fn_03c65b(uint8_t *a0);           /* 0x03c65b: video flatten (200 lists -> 0x140 packed records) */
void fn_03ce0b(uint8_t *ent, uint8_t *dst);  /* 0x03ce0b: 4-branch sprite-draw (viewport blit, video-arena src) */

/* 0x03bebb: constant-fill row-RLE blit (forward/reverse). EAX=base(flat 0xa0000 video
 * addr), EDX=dest_off(signed,<0x140), EBX=start_row(signed), ECX=col; stack: row_count,
 * rle, mode, fill_byte. 0x03bb42: identical but copy (mem_copy/cpy_back) instead of fill. */
void fn_03bebb(uint32_t base, int32_t dest_off, int32_t start_row, int32_t col,
               uint32_t row_count, const uint8_t *rle, int32_t mode, uint8_t fill_byte);
void fn_03bb42(uint32_t base, int32_t dest_off, int32_t start_row, int32_t col,
               uint32_t row_count, const uint8_t *rle, int32_t mode);
/* Off-screen 64000 B screens (DAT_000633b0 / DAT_000633b4), modelled as a flat arena right
 * above the A0000 video window so fn_03bb42/fn_03bebb base arithmetic resolves the same way. */
#define SCREEN_BASE  0xb0000u                       /* VIDEO_BASE + VIDEO_SIZE */
#define SCREEN_SIZE  0xfa00u                        /* 64000 B */
extern uint8_t  g_screen_ram[2 * SCREEN_SIZE];
extern uint32_t g_dat_633b0;                        /* DAT_000633b0: back screen (flat addr) */
extern uint32_t g_dat_633b4;                        /* DAT_000633b4: working screen (flat addr) */
/* DAT_000639c4 HUD blit control block. The original keeps its four pointer fields 4 bytes
 * apart (+4, +8, +0x34, +0x38); widening them in place would OVERLAP, so the port models the
 * block as this struct (field order = original order). The dwords inside the referenced
 * objects (+0x2c / +0x30) stay 32-bit and are still read by byte offset. */
struct blit_ctl {
    uint8_t *col_obj;   /* orig +0x04: column dwords   @+0x2c / @+0x30 */
    uint8_t *row_obj;   /* orig +0x08: row-count dwords @+0x2c / @+0x30 */
    uint8_t *off_obj;   /* orig +0x34: RLE byte offsets @+0x2c / @+0x30 */
    uint8_t *rle_base;  /* orig +0x38: base of the RLE streams */
};
extern struct blit_ctl *g_dat_639c4;                /* DAT_000639c4: HUD blit control block */
void fn_03ace9(void);                               /* 0x03ace9: 20-frame HUD composite (633b0/633b4 + 2 sprites via fn_03bb42) */
void fn_012efa(uint8_t *ent, uint32_t base);
void fn_03203d(uint32_t base);
extern uint8_t g_blk_51c24[0x1dc];   /* DS 0x51c24: initialized data-segment block (see f.c for the map) */
#define g_blk_51c80   (g_blk_51c24 + 0x5c)          /* DS 0x51c80 view (legacy name) */
#define g_tab_51c90   (g_blk_51c24 + 0x6c)          /* DS 0x51c90 gauge fill colours; +1 = DAT_00051c91 */
#define g_tab_51c24   ((const uint32_t *)g_blk_51c24) /* DS 0x51c24: entity type -> sound-slot index */

void fn_01348f(uint8_t *ent, uint32_t base, int32_t slot);
void sys_key_script_clear(void);   /* reset the scripted key timeline + tick counter */
void sys_key_script_add(uint32_t tick, uint8_t *slot, int value); /* set *slot on the Nth sys_key_spin() */
void sys_key_spin(void);           /* the port's stand-in for a keyboard busy-wait: SDL pumps input, tests run the script */
extern uint8_t g_dat_63eb2;        /* DAT_00063eb2: key byte (the "any key" set of the help/title screens) */
extern uint8_t g_dat_63ebe;        /* DAT_00063ebe: key byte (same set) */

extern uint8_t *g_dat_53dd4;       /* DAT_00053dd4: the title attract command cursor */
extern uint8_t *g_dat_53ddc;       /* DAT_00053ddc: the title "start" command cursor */
extern uint8_t  g_dat_63ed0;       /* DAT_00063ed0: select-held key byte (title menu) */
extern uint8_t  g_dat_63ed8;       /* DAT_00063ed8: mirrored select-held key byte */

int32_t fn_03a224(void);   /* 0x03a224: THE TITLE SCREEN -- wipe-in, attract stream, a two-entry menu (buf[0]/buf[1]) with DAT_63ed0/63ed8, returns 0/1 (entry) or 3 (quit/timeout). */
void fn_038cde(void);   /* 0x038cde: THE HELP / INSTRUCTIONS SCREEN -- pages help%d.pcx (10..0x23) with DAT_63ed3/63ed5, three animated palette bands every 3rd frame, fades in/out with 50 DAC frames. */
void fn_03a70e(void);   /* 0x03a70e: THE PICTURE / CG SLIDE-SHOW DRIVER -- plays na<stage>%04d.pcx one picture per key press, count from DAT_52f08[stage], then wipes with the progressive reveal and restores the tick counters + keyboard pump. */
extern uint32_t g_dat_52f08[0x40];   /* DAT_00052f08[]: picture count per stage */
extern uint8_t *g_dat_53dd8;        /* DAT_00053dd8: the slide-show command cursor */

void fn_0381d7(uint32_t base, int32_t dx, int32_t dy, int32_t steps, int32_t facing);   /* 0x0381d7: THE PLAYER STEP-ANIMATION LOOP -- walks the player block N steps (x +/- dx by the facing arg, y += dy, frame index +0x11 wrapping at the action's frame count) and redraws the paced composite after every step. */

void fn_038098(uint32_t base);   /* 0x038098: THE PACED FRAME PUMP -- sort the entity list, draw the room plane + entity sweeps + both gauge panels and push the work screen to VGA, but only while the frame budget 0x1ea/DAT_53104 is ahead of the tick DAT_63e24; then wait and subtract the budget. */

void fn_0113e7(uint8_t *ent);
                                          /* 0x0113e7: THE THREE-ACTOR ESCORT SPAWN -- spawns three type-0xfa actors flanking the source (x -0xc/-0x2c/+0x16, or mirrored), stamps +0x1b/+0x30/+0x17, then starts a device command stream chosen by the source's entity type. */

void fn_013908(uint8_t *ent);
                                          /* 0x013908: THE ENTITY OCCUPANCY REGISTER -- same action/frame resolution as fn_012efa but feeds the silhouette RLE into the interval-list builder fn_03ba86 (per-row column spans, x = e[1]-xhotspot or e[1]-(width-xhotspot) when mirrored, y = e[5]+e[9]-yhotspot), so collision/occlusion tracks the drawn frame. */

void fn_03801b(uint32_t base);
                                          /* 0x03801b: THE HUD COMPOSITE OF ONE FRAME -- room blit (fn_03ce0b) + entity sweep (fn_03203d) + stage glyphs (fn_03df96) + the player's gauge/countdown (fn_01348f/fn_0137c1), then cpy_dw the whole 64000 B work screen into the A0000 window. EAX = work-screen base. */

                                          /* 0x01348f: THE HUD GAUGE DRAW -- one player's status-bar panel: splits the int16 gauge words at entity+0x23/+0x29 (/100,%100 for the player, /200 and half%100 otherwise, colour 0x95 past index 5/4), blits face.spr + gage.spr records via fn_03bb42 and paints the bars with rect_fill/vline_set. EBX = HUD slot (0xaa px wide). */

                                          /* 0x03203d: THE ENTITY DRAW SWEEP -- fn_012efa over the DAT_53c70 entity list (+0x13f links) for DAT_53c68 iterations, count re-read each pass; EAX = destination base forwarded as EDX. */

                                          /* 0x012efa: THE ENTITY SPRITE DRAW -- resolves the entity's action record (e[0x123] container -> [+4] record array -> [e[0x17]&0x7fffffff]), picks frame e[0x11] %% the record's frame count, then blits via fn_03bebb (kind 2 pulse, fill 0x9c), fn_03c231 (silhouette, fill 0xd0) and/or fn_03bb42 (the sprite itself). blink gate on kind 1, mirror on dword[e+0x1b]. */
void fn_03c231(uint32_t dest, int32_t col, int32_t row, int32_t width,
               uint32_t rows, const uint8_t *rle, int32_t flip, uint32_t fill_byte);
                                          /* 0x03c231: THE INTERLACED FILL BLIT -- draws a solid silhouette (each run filled with one byte via fn_041e20) from an fn_03bb42-style RLE stream; rows<<1 iterations with only the even ones drawing (odd ones consume a stream row), forward or mirrored (flip byte), with left/right clipping against 0..0x140 and width */
void fn_03b953(uint32_t base);                 /* 0x03b953: THE NUMBER-DISPLAY SWEEP -- walk the DAT_639cc entity list (count DAT_639c8) and run fn_03b718 on every node; EAX in = video base passed through as EDX; advances the cursor from node+0x11 AFTER fn_03b718 may have freed the node (faithful latent bug) */
void fn_03b718(uint8_t *entity, uint32_t base);   /* 0x03b718: THE FLOATING VALUE NUMBER DISPLAY -- itoa32s(entity+1) rendered as RLE digit glyphs relative to the entity (font DAT_639c0 / DAT_639c4 by entity[0]), terminator glyph from the control block, then age entity+5 (DAT_63360 %% 3) and destroy the entity past 0x28 */
void     fn_03ba2c(uint32_t c_init, uint32_t inc);  /* 0x03ba2c: build 200 interval lists */
void     fn_03ba86(int32_t a_base, int32_t idx_base, int32_t row_count, const uint8_t *p, int32_t flag);  /* 0x03ba86: nested-list builder from byte stream */
void     fn_03a594(uint8_t *dest, const uint8_t *src, uint32_t count);  /* 0x03a594: copy bytes != 0xff */
void     fn_034aba(int32_t a, int32_t idx, int32_t b, int32_t c, int32_t flag);  /* 0x034aba */
extern uint32_t g_dat_639dc;    /* DAT_000639dc */
extern uint32_t g_dat_633a0;    /* DAT_000633a0: # entries in the 0x5d884 string table */
extern uint8_t  g_tab_5d884[128 * 21];  /* RODATA string->value table @0x5d884, 21-byte entries */
extern res_reader *g_639b8;      /* DAT_000639b8: resource-reader node pointer (target of fn_0367c9 submits) */
int32_t fn_036626(const char *s);  /* 0x036626: case-insensitive string->value table lookup */
void fn_0366a4(uint8_t *node);  /* 0x0366a4: obfuscated resource-index loader -> g_dat_633a0 + g_tab_5d884 (scratch = back screen) */
int32_t fn_0367c9(const char *a0); /* 0x0367c9: find name in 0x5d884 table; on first match fn_04247a(g_639b8, entry@17, 0); 1/0 */
void fn_03a1b3(const char *name, uint8_t *dest); /* 0x03a1b3: read {A,B,data[A*B]} block from resource reader g_639b8 into dest */
int32_t fn_036853(const char *a0, int32_t a1, int32_t a2); /* 0x036853: lookup + value transform (a2: 0 -> v2+a1, 2 -> v2+v13+a1) */
extern uint32_t g_dat_639e0;    /* DAT_000639e0 */
/* 0x03df96 sprite-blit font: original is a 4-byte-dword-pointer struct; port widens each pointer
 * to 8 bytes (pointer-width adaptation). orig offsets: col_tbl=+0x04, rowcount_tbl=+0x08,
 * x_tbl=+0x0c, y_tbl=+0x10, char_table=+0x34, rle_base=+0x38. */
struct hud_font {
    uint32_t *col_tbl;       /* orig +0x04: col (ECX) table */
    uint32_t *rowcount_tbl;  /* orig +0x08: row_count table */
    uint32_t *x_tbl;         /* orig +0x0c: dest_off (x) offset table */
    uint32_t *y_tbl;         /* orig +0x10: start_row (y) offset table */
    uint32_t *char_table;    /* orig +0x34: glyph RLE-offset table */
    uint32_t *rle_base;      /* orig +0x38: RLE data base */
};
extern struct hud_font *g_dat_63dfc;  /* 0x63dfc: sprite blit font ptr (fn_03df96) */
extern uint32_t g_dat_62a90;          /* 0x62a90: sprite record count (fn_03df96) */
extern uint8_t *g_dat_5d0b4;          /* 0x5d0b4: sprite record array, 0x14 stride (fn_03df96) */
extern float g_tab_5274c[0x4f0 / 4];  /* DAT_0005274c: float rates, 10 per stage x room serial; no producer in the EXE (read by 0x03d75c) */
void fn_03d75c(int32_t cam_delta);      /* 0x03d75c: stage-record float decay (EAX = camera-x delta) */
void fn_03cb38(uint8_t *subj);          /* 0x03cb38: camera scroll-RIGHT step (room +0x08 = right edge) */
void fn_03cbd9(uint8_t *subj);          /* 0x03cbd9: camera scroll-LEFT step (room +0x00 = left x) */
void fn_03d4e4(uint8_t *room);          /* 0x03d4e4: camera policy step: bounds/flags gate the two scroll steps + vertical state machine */
uint32_t fn_01519f(const uint8_t *p);    /* 0x01519f: return *(u32*)(p+1) - g_dat_639dc */
void fn_015295(uint8_t *a0, int32_t a1); /* 0x015295: entity tick -- clamp a0->5, conditional list-insert + accumulator */
uint32_t fn_0151d5(const uint8_t *p);    /* 0x0151d5: return *(u32*)(p+9) - g_dat_639e0 */
void fn_031f10(uint8_t *p, uint32_t val);  /* 0x031f10: ring push (count@0x31, buf@0x32, cap 5) */
void fn_0348e2(void);                       /* 0x0348e2: slot-alloc (g_dat_63f50, wrap @9) */
uint8_t fn_03493e(void);                     /* 0x03493e: read g_dat_63f14 slot byte via g_dat_63f50 */
void fn_0343d1(uint8_t i);                  /* 0x0343d1: idx flag/counter (g_dat_62cd8/62c58) */
void fn_03442e(uint8_t x);                  /* 0x03442e: rotating 10-slot event buffer */
void fn_034735(uint8_t idx);                /* 0x034735: per-key state machine (62c58/62cd8) */
uint32_t fn_0351dc(uint8_t **cursor);       /* 0x0351dc: unsigned LEB128 (base-128) decoder */
uint32_t fn_03510b(const uint8_t *p);       /* 0x03510b: big-endian 32-bit read */
uint8_t *fn_041e20(uint8_t *dest, uint8_t byte, uint32_t count);  /* 0x041e20: memset via fn_043280 */
extern uint32_t g_dat_63f94;   /* DAT_00063f94: device transfer bytes remaining */
extern uint32_t g_dat_63f98;   /* DAT_00063f98: last transfer chunk length (device sees n-1) */
extern uint32_t g_dat_63f9c;   /* DAT_00063f9c: device command selector (0x14 special) */
extern uint8_t *g_dat_63fa8;   /* DAT_00063fa8: DMA source cursor (host ptr in the port) */
extern uint8_t *g_dat_63fb4;   /* DAT_00063fb4: device buffer, 0x10000-byte window (host ptr) */
extern uint32_t g_dat_63fac;   /* DAT_00063fac: 24-bit DMA address byte-sent to the device */
extern uint32_t g_dat_63f8c;   /* DAT_00063f8c: transfer-armed flag (0xffff) */
void fn_040839(uint8_t *cursor);   /* 0x040839: device command-stream walker: [kind][24-bit len][payload]; kind 0 end / 1 fn_04097e / 2 fn_040b83 / other skip */
extern uint8_t g_dat_53338;       /* DAT_00053338: sound/music enable byte (gates fn_04185e) */
extern uint8_t g_dat_63fb8;       /* DAT_00063fb8: device command-stream started flag */
void fn_04185e(uint8_t *cursor);  /* 0x04145e: start the device command stream (gated by DAT_63f90 + DAT_53338; re-entry sends cmd 0xd0 first) -- unblocks 9 pending functions */
void fn_04097e(uint8_t *hdr, uint32_t total);  /* 0x04097e: arm+start a device transfer: hdr[0]->fn_040948, hdr[1] selects g_dat_63f9c, then the DMA chunk body */
void fn_040b83(uint8_t *src, uint32_t unused_edx);  /* 0x040b83: one DMA chunk: fill 0x80 + mem_copy + DMA port burst + 3x fn_041340 */
uint32_t fn_040fb3(void);               /* 0x040fb3: save+hook INT 0x0a/0x0b/0x0d/0x0f/0x72, remask PIC 0x21/0xa1, probe cmd 0xf2, wait, restore; returns DAT_00063f84 */
uint32_t fn_040d09(void);               /* 0x040d09: device bring-up chain (fn_040f4f base scan -> fn_040fb3 probe -> fn_0411d9 id -> 64KB buffer -> hook ISR 0x415c9 -> fn_0413e7 -> DAT_63f90=1) */
uint32_t fn_03658d(void);               /* 0x03658d: master bring-up (IRQ9/timer state, DPMI, device, CD read, pitch, PIT tick, init flag, two 64KB screens) */
extern uint32_t g_dat_63370;            /* DAT_00063370: master-init done flag */
extern uint32_t g_dat_63f88;            /* DAT_00063f88: device id ((hi<<8)|lo) from fn_0411d9 */
extern uint32_t g_dat_63f90;            /* DAT_00063f90: device initialised flag */
extern uint32_t g_dat_63f54;            /* DAT_00063f54: saved device-ISR offset (0x040d09) */
extern uint16_t g_dat_63f58;            /* DAT_00063f58: saved device-ISR segment (0x040d09) */
extern uint32_t g_dat_63f5c; extern uint16_t g_dat_63f60;   /* 0x040fb3: saved INT 0x0a offset/segment */
extern uint32_t g_dat_63f64; extern uint16_t g_dat_63f68;   /* 0x040fb3: saved INT 0x0b offset/segment */
extern uint32_t g_dat_63f6c; extern uint16_t g_dat_63f70;   /* 0x040fb3: saved INT 0x0d offset/segment */
extern uint32_t g_dat_63f74; extern uint16_t g_dat_63f78;   /* 0x040fb3: saved INT 0x0f offset/segment */
extern uint32_t g_dat_63f7c; extern uint16_t g_dat_63f80;   /* 0x040fb3: saved INT 0x72 offset/segment */
int32_t  fn_0350aa(const uint8_t *a, const uint8_t *b);           /* 0x0350aa: 4-byte prefix equal? 1/-1 */
uint64_t fn_0408c5(const uint8_t *p);       /* 0x0408c5: little-endian 24-bit (3-byte) read */
void fn_03cad3(const uint8_t *p);           /* 0x03cad3: struct -> g_dat_639dc/e0/e4 */
void fn_034353(uint8_t a, uint32_t b);      /* 0x034353: slot update (g_dat_63f1e/63f0c) */
void fn_010376(uint8_t *s, uint32_t unused); /* 0x010376: free struct fields @0x6d/0x71/0x75 */
uint32_t fn_03518a(const uint8_t *p);          /* 0x03518a: BE 16-bit read (p[0]<<8)|p[1] */
void fn_04a7fa(double a, double *out);          /* 0x04a7fa: *out = fpu_rndint(a) (fraction left on the x87 stack) */
double fn_04a32b(double arg1, double arg2);     /* 0x04a32b: floor(arg1) via rndint + fraction test (arg2 unused) */
double fn_04a3a8(double arg1, int32_t arg2);     /* 0x04a3a8: clamp(+inf->+DBL_MAX, NaN->-DBL_MAX) then ~= arg1*10^arg2 via scale+table power */
int32_t fn_016959(uint8_t *s, int32_t param2, int32_t *param3); /* 0x016959: move-toward-target clamped step + countdown */
int32_t fn_017530(uint8_t *s, int32_t param2, int32_t *param3); /* 0x017530: twin of fn_016959 */
int32_t fn_016a60(uint8_t *s, int32_t param2, int32_t *out);   /* 0x016a60: clamped move-toward-target */
int32_t fn_017637(uint8_t *s, int32_t param2, int32_t *out);   /* 0x017637: clamped move-toward-target (corrected B-reached) */
int32_t fn_0174a2(uint8_t *s, int32_t param2, int32_t *out);   /* 0x0174a2: clamp to [0,0x140); adjust out */
extern uint8_t *g_dat_63354;   /* pointer to clamp-bounds struct: lo=P+0x12, hi=P+0x16 (double-deref) */
extern int32_t  g_dat_63360;   /* 0x63360: frame counter; %10 gates periodic board_out in 0x03ce0b */
extern uint8_t *g_dat_63e04;   /* 0x63e04: HUD number font ptr; +0x34=char_table(8B), +0x3c=rle_base(8B) (fn_0137c1) */
extern uint8_t *g_dat_63358;   /* 0x63358: mirror of g_dat_63354 (0x03cc6b) */
extern uint32_t g_dat_63370;   /* 0x63370: mode flag (0x03cc6b) */
extern uint32_t g_dat_63374;   /* 0x63374: = g_dat_53c7c->0x2b (0x03cc6b) */
extern uint32_t g_dat_63378;   /* 0x63378: = (byte)g_dat_53c7c->0x27 (0x03cc6b) */
extern uint32_t g_dat_6337c;   /* 0x6337c: = (byte)g_dat_53c7c->0x26 (0x03cc6b) */
extern uint32_t g_dat_63380;   /* 0x63380: = (byte)g_dat_53c7c->0x25 (0x03cc6b) */
extern uint32_t g_dat_63384;   /* 0x63384: = (byte)g_dat_53c7c->0x28 (0x03cc6b) */
void fn_03cc6b(uint8_t *a0);   /* 0x03cc6b: state machine (g_639e4 toggle, g_63354/58, kb-config copy, g_639e0++) */
void fn_034295(int32_t amount);  /* 0x034295: the player's STATE-GATED vertical move (DAT_63ed3 reversed / DAT_63ed5 plain, plus [player+0x1b] must be 0 / 1) */
extern uint8_t g_dat_63ed3;      /* DAT_00063ed3: reversed vertical-direction flag */
extern uint8_t g_dat_63ed5;      /* DAT_00063ed5: plain vertical-direction enable */
void fn_0342ff(int32_t amount);  /* 0x0342ff: the player's UNGATED vertical move (same direction sources, no state gate) */
void fn_01545e(uint8_t *e, int32_t d);  /* 0x01545e: THE Y-AXIS MOVE + WALL BANDS + TRIGGER AREAS -- Y mirror of fn_01586b over g_tab_5c8bc; rewrites the delta to stop on a band edge, fires the DAT_63364/63368 latch and puts caught entities to sleep, then hard-clamps Y to the room band (with a prng jitter). Unblocks 21 pending functions. */
int32_t fn_016b8f(uint8_t *e, int32_t step, int32_t cmp_ebx, int32_t cmp_ecx, int32_t *out_a, int32_t *out_b);  /* 0x016b8f: THE THREE-PHASE STEP/RETURN WALK (EAX=entity, EDX=step, EBX/ECX=caller comparison values, two stack out params at [ebp+0x10]/[ebp+0x14]); phases 0 approach / 1 pause / 2 walk back over [e+0x93], budget [e+0x97], accumulators [e+0xa3]/[e+0xa7]. Unblocks 20 pending functions. */
void fn_01586b(uint8_t *a0, uint32_t a1);  /* 0x01586b: clamp a0[9] to [min,max] after adding a1, g_tab_5c8bc pre-pass */
struct clamp_tab_5c8bc { int32_t a1_lo, v_lo, a1_hi, v_hi, _pad; };  /* 0x5c8bc: 20 B, 0x14 stride */
extern struct clamp_tab_5c8bc g_tab_5c8bc[512];
extern uint8_t  g_dat_639e4;      /* DAT_000639e4 (byte) */
extern uint8_t  g_dat_63f1e[16];  /* DAT_00063f1e (byte slots) */
extern uint32_t g_dat_63f0c;      /* DAT_00063f0c (slot base) */
extern uint8_t  g_dat_63f14[16];  /* DAT_00063f14 (byte slots) */
extern uint32_t g_dat_63f28[16];  /* DAT_00063f28 (dword slots) */
extern uint8_t  g_dat_63f50;      /* DAT_00063f50 (slot index) */
extern uint32_t g_dat_63f08;      /* DAT_00063f08 (slot fill value) */
extern uint8_t  g_dat_62cd8[128]; /* DAT_00062cd8 (counters, per-key) */
extern uint8_t  g_dat_62c58[128]; /* DAT_00062c58 (flags, per-key) */
extern uint32_t g_dat_62d5c;     /* DAT_00062d5c: frame counter (decremented by fn_0347e3) */
extern uint8_t  g_dat_52ff8;     /* DAT_00052ff8: keyboard pump enabled (gate for fn_0347e3) */
void fn_0347e3(void);            /* 0x0347e3: keyboard dispatch pump over g_62c58 */
extern uint8_t  g_dat_62c75;      /* DAT_00062c75 (byte flag) */
extern uint8_t  g_dat_62c82;      /* DAT_00062c82 (byte flag) */
extern uint8_t  g_dat_62c8e;      /* DAT_00062c8e (byte flag) */
extern uint8_t  g_dat_62c90;      /* DAT_00062c90 (byte flag) */
extern uint32_t g_dat_62a94;   /* DAT_00062a94 (kb input state value) */
extern uint8_t *g_dat_53c7c;   /* DAT_00053c7c (kb config ptr; [0x1b] int32) */
extern uint8_t *g_dat_53c80;   /* DAT_00053c80 (entity ptr; compared to a0 in 0x015295) */
extern int32_t  g_dat_53c48;   /* DAT_00053c48: CD-refresh threshold (cmp g_63e38/490, 0x010d47) */
extern int32_t  g_dat_53c6c;   /* DAT_00053c6c (state flag) */
extern uint8_t *g_dat_53c70;   /* DAT_00053c70 (entity list head; next at +0x13f) */
extern uint8_t *g_dat_53c74;   /* DAT_00053c74 (entity list tail; prev at +0x147) */
void fn_013aad(uint32_t a0);   /* 0x013aad: entity-list bubble-sort (ascending by node->+0x9) */
uint8_t *fn_010efc(uint8_t *block, int32_t arg_edx, int32_t arg_ebx, int32_t arg_ecx,
                   int32_t type_idx, uint8_t kind);              /* 0x010efc: THE ENTITY CONSTRUCTOR: fills the 0x147 B (0x14f here) actor block from the entity-type table (DAT_53e9c, stride 0x88) + per-type sprite struct (DAT_639ec[]), appends it to the entity list, PRNG jitter */
void fn_011827(uint8_t *src, int32_t dx, int32_t arg_ebx, int32_t dy, uint32_t stack_arg);
                   /* 0x011827: THE MIRRORED COMPANION SPAWN -- builds a type-0xfa entity offset +/-dx in x from a source entity (sign chosen by source+0x1b), copies source+0x17 to the gate, marks +0x30, and raises the 0x13 element event unless the source's +0x17 is 4 */
extern uint8_t g_dat_53e9c[];                                   /* 0x53e9c: entity-type table (stride 0x88; +0 dword/word, +4 byte, +8 word) */
uint8_t *fn_0112fd(uint8_t *node, uint32_t arg2);   /* 0x0112fd: entity-list REMOVE (head/tail/middle); returns node */
extern int32_t  g_dat_51c98;   /* DAT_00051c98 (collision mode flag: 1=reinit->2, 2=active) */
extern int32_t  g_dat_53c90;   /* DAT_00053c90 (player-event latch) */
extern int32_t  g_dat_53c68;   /* DAT_00053c68 (entity count) */
extern uint32_t g_dat_53c88;   /* DAT_00053c88 (cleared by fn_03237b conditional drain) */
extern uint8_t *g_dat_62aa0;   /* DAT_00062aa0: "gun.key" text table (fn_032464 block) */
extern uint8_t *g_dat_62aa4;   /* DAT_00062aa4: "gun1.key" */
extern uint8_t *g_dat_62aa8;   /* DAT_00062aa8: "gun2.key" */
extern uint8_t *g_dat_62aac;   /* DAT_00062aac: "gun3.key" */
extern uint8_t *g_dat_62ab0;   /* DAT_00062ab0: "gun4.key" */
extern uint8_t *g_dat_62ab4;   /* DAT_00062ab4: "catch.key" */
extern uint8_t *g_dat_62ab8;   /* DAT_00062ab8: "catch1.key" */
extern uint8_t *g_dat_62abc;   /* DAT_00062abc: "catch2.key" */
extern uint8_t *g_dat_62ac0;   /* DAT_00062ac0: "catch3.key" */
extern uint8_t *g_dat_62ac4;   /* DAT_00062ac4: "catch4.key" */
extern uint8_t *g_dat_62a98;   /* DAT_00062a98: active key table (alias of g_dat_62aa0) */
extern uint8_t *g_dat_62a9c;   /* DAT_00062a9c: active catch table (alias of g_dat_62ab4) */
extern uint32_t *g_dat_53c28;  /* DAT_00053c28: "gun.act" record container (fn_010010) */
extern uint32_t *g_dat_53c2c;  /* DAT_00053c2c: "gun1.act" */
extern uint32_t *g_dat_53c30;  /* DAT_00053c30: "gun2.act" */
extern uint32_t g_dat_53818[0x104]; /* DAT_00053818: per-type action-table heap-offset ARRAY, 0x104 (260) slots -- one per entity type 0..0x103, ending exactly at DAT_00053c28 (slot 0 = the gun action table); slots 0xc9/0xfa are shared tables the teardown skips */
extern uint32_t *g_dat_63e08;  /* DAT_00063e08: "over.spr" struct (fn_03d8d4) */
extern uint32_t *g_dat_63e0c;  /* DAT_00063e0c: "pause.spr" struct (fn_03d8d4) */
extern uint8_t *g_dat_53e24[10];   /* DAT_00053e24..0x53e48: ten device-command-stream cursors picked by entity type (fn_0113e7); BSS, no writer in the analyzed set yet */
extern uint8_t *g_dat_53d0c[100]; /* DAT_00053d0c: 100 sound-slot buffers ("1.voc".."100.voc") */
extern uint32_t  g_dat_53c94[30];  /* DAT_00053c94: 6x5 experience table from exptab.dat */
uint32_t fn_03690b(void);          /* 0x03690b: master data loader (NIGHT.RES -> all runtime tables) */
extern uint32_t *g_dat_639c0;  /* 0x639c0: struct host-ptr (destructed by fn_03d999) */
extern uint32_t *g_dat_53c8c;  /* 0x53c8c: struct host-ptr (fn_03d999) */
extern uint32_t *g_dat_63e00;  /* 0x63e00: struct host-ptr (fn_03d999) */
extern uint8_t *g_dat_639ec[0x104]; /* 0x639ec: per-type sprite-struct pointer ARRAY, 0x104 (260) slots -- one per entity type 0..0x103, ending exactly at DAT_00063dfc (element 0 = "gun.spr"; indexed by entity type in fn_010efc, filled by fn_037b03) */
extern uint32_t *g_dat_63d10;  /* 0x63d10: struct host-ptr (fn_03d999) */
extern uint32_t *g_dat_53b3c;  /* 0x53b3c: container host-ptr (fn_0100d9) */
extern uint32_t *g_dat_63dd4;  /* 0x63dd4: struct host-ptr (fn_03d999) */
extern uint32_t *g_dat_53c00;  /* 0x53c00: container host-ptr (fn_0100d9) */
extern uint32_t  g_pool_53d0c[100];  /* 0x53d0c: 100 pooled object offsets */
void fn_037922(void);   /* 0x037922: clear-all-state -- free 100 pool + destruct+free+null 8 struct globals */
void fn_0323f8(void);   /* 0x0323f8: drain entity list -- remove+free every node until empty */
void fn_03237b(void);   /* 0x03237b: conditional drain -- remove+free nodes with dword@+0xd != 0; g_dat_53c88=0 */
#define FN_015A1F_TBL 0x2000
extern uint8_t  g_dat_53eb8[FN_015A1F_TBL]; /* DAT_00053eb8 (zeroed runtime table) */
void fn_034635(uint8_t key);
uint32_t fn_0100d9(uint32_t *s);      /* 0x0100d9: free container {count, array} + free array */
int32_t  fn_015a1f(uint8_t *s);        /* 0x015a1f: state machine, data-table distance check */
int32_t  fn_015b5b(uint8_t *s);        /* 0x015b5b: family core, magic '.' */
int32_t  fn_01603e(uint8_t *s);        /* 0x01603e: family core, magic '+' */
int32_t  fn_0161f9(uint8_t *s);        /* 0x0161f9: family core, magic '+' */
int32_t  fn_0163b4(uint8_t *s);        /* 0x0163b4: family core, magic ',' */
int32_t  fn_01656f(uint8_t *s);        /* 0x01656f: family core, magic {',' '-'} */
int32_t  fn_016750(uint8_t *s);        /* 0x016750: family core, magic {'1' '4' '.'} */
int32_t  fn_016fec(uint8_t *s, int32_t yref);   /* 0x016fec: grid-collision detector */
int32_t  fn_017ca6(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2);   /* 0x017ca6: Bresenham diagonal stepper */
int32_t  fn_01730b(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2);   /* 0x01730b: 2-phase X-then-Y move-toward-target */
int32_t  fn_01776f(uint8_t *s, int32_t param2, int32_t *out1, int32_t *out2);   /* 0x01776f: 2D movement delta computer */
int32_t  fn_017927(uint8_t *s, int32_t val, int32_t *out1, int32_t *out2);      /* 0x017927: 2D move/AI (double-deref bounds) */
void     fn_017f13(uint8_t *e);
void     fn_018e88(uint8_t *e);   /* 0x018e88: THE BEHAVIOUR-0x01 ENTITY AI HANDLER -- fn_017f13's near-clone: two speed-index words (record +0x04 / +0x80, the latter forced by entity[0x6f]), no entity[0x8b] recon block, and a six-slot hiding-spot draw (j = prng%4 with a +prng%2 sub-draw for 2/3) whose jitter pairs live at DS 0x51c9c/0x51ca0/0x51ca4/0x51ca8 stride 0x10 */   /* 0x017f13: THE BEHAVIOUR-0x00 ENTITY AI HANDLER -- speed bookkeeping on the float field e[0x63], the fn_015a1f proximity latch, then a switch on e[0x1f] (0..0x16) over the twelve action routines with the family's register ABI (EAX=e, EDX=speed, EBX/ECX=player screen x/y, two stack out slots); the two deltas feed fn_01545e / fn_01586b and e[0x1b] tracks facing */
void     fn_032158(void);              /* 0x032158: entity list -> set targets to player + prng jitter */
int32_t  fn_015ed1(uint8_t *s);        /* 0x015ed1: family variant (s[0x83] gate, no char, Y-limit 10) */
int32_t  fn_015d16(uint8_t *s);        /* 0x015d16: family variant (X-gate + char-selects-Y-limit) */            /* 0x034635: keyboard input state machine */
/* 0x0492d0: node/device-table accessor. */
extern uint32_t g_dat_536dc;              /* DAT_000536dc: node index bound */
extern uint32_t g_dat_536d8;              /* DAT_000536d8: create-mode xor (load-time 0) */
extern uint32_t *g_dat_53730;            /* DAT_00053730: node table (4B each; byte1=flags) */
extern uint32_t g_dat_53780;             /* DAT_00053780: TZ range diff */
extern uint32_t g_dat_53784;             /* DAT_00053784: TZ has-second-part flag (0/1) */
extern uint32_t g_dat_53788;             /* DAT_00053788: TZ time prefix buf (4B) */
extern uint32_t g_dat_537a7;             /* DAT_000537a7: TZ time2 prefix buf (4B) */
extern uint32_t g_dat_5377c;             /* DAT_0005377c: TZ seconds */
extern struct dec_datetime g_dat_53734;      /* DAT_00053734: recurring-event reference date G1 */
extern struct dec_datetime g_dat_53758;      /* DAT_00053758: recurring-event reference date G2 */
extern uint8_t *g_dat_537ca;                 /* DAT_000537ca: ptr to enable flag (0 -> fn_049613 returns 0) */
uint64_t fn_0492d0(uint32_t index, uint32_t ctx);
void     fn_042a19(void);       /* 0x042a19: g_dat_64008 = 0xe (tail-jump fn_042a0d) */
void     fn_044b96(void);       /* 0x044b96: drain g_node_list_done via heap_free_host */
void     fn_045079(uint32_t value, char *buf, int width); /* 0x045079: zero-pad + right-justify hex */
void     fn_0450da(char *out, int32_t value, int *spec);  /* 0x0450da: 16.16 fixed -> decimal string */
uint8_t *fn_0451bc(uint8_t *out, uint8_t **cur, uint8_t *st, uint8_t *alt); /* 0x0451bc: typed field formatter */
const uint8_t *fn_043f80(const uint8_t *src, uint8_t *dest); /* 0x043f80: 8-bit presence bitmap */
void fn_049e15(char *dest, int count2, int count3); /* 0x049e15: in-buffer range shift + fixup + fill */
uint32_t fn_041e38(uint32_t a, uint32_t b, uintptr_t ctx, uint32_t d); /* 0x041e38: DPMI real-mode trampoline B */
uint32_t fn_041e51(uint32_t a, uint32_t b, uint32_t c, uint32_t d); /* 0x041e51: seg-save + DPMI real-mode call (no-op, returns 0) */
uint32_t fn_0422d5(uint32_t a, const char *b, uint32_t c, uint32_t d, uint8_t e); /* 0x0422d5: DOS open/seek/term wrapper (dispatch_b only) */
void fn_0429c8(const char *msg, uint32_t param2);  /* 0x0429c8: fatal-error reporter (debug_break_hook -> fn_0422d5) */
uint32_t fn_0429e5(uint32_t a, const char *b, uint32_t c, uint32_t d, uint8_t e); /* 0x0429e5: JMP thunk -> fn_0422d5 */
void fn_0429ea(const char *msg, uint32_t param2);  /* 0x0429ea: fatal-error reporter (direct fn_0422d5) */
void fn_0418c6(void);                              /* 0x0418c6: report undefined ctor/dtor via fn_0429c8 */
void fn_045dad(void);                              /* 0x045dad: report FPU-not-loaded via fn_0429ea */
void fn_043324(uintptr_t context, uintptr_t rm_ptr); /* 0x043324: DPMI real-mode transition (save) */
/* 0x04335c is rm_switch_a (L0 batch A). */
uint32_t fn_03f084(const uint16_t *arg1, const uint16_t *arg2, uint16_t *arg3, const uint16_t *arg4); /* 0x03f084: DPMI INT 31h ctx wrapper */
int32_t fn_042aba(uint32_t handle, void *buf, uint32_t count); /* 0x042aba: DOS AH=3f read */
int32_t fn_044807(uint32_t handle, int32_t offset, int32_t whence); /* 0x044807: DOS AH=42 lseek */
int32_t fn_044bec(uint32_t handle);                              /* 0x044bec: DOS AH=3e close-file */
int32_t fn_049415(const char *name); /* 0x049415: DOS AH=41 delete */
int32_t fn_044c1e(const char *name); /* 0x044c1e: JMP thunk -> fn_049415 */
/* Debug-break gate: if g_debug_break (DAT_00053624) is set, fire a breakpoint
 * with (eax, edx&0xffff); always returns 0. Port routes INT3 to sys_breakpoint.
 *  orig: 0x045a44 */
extern uint8_t g_debug_break;
uint32_t debug_break_hook(uint32_t eax, uint32_t edx);

/* ---- L0 batch D2 (2): DOS clock calibration + delay (INT 21h/0x2Ch) ---- */
/* Tick rate measured by calibrate_clock (DAT_00064000). The original counts
 * INT 21h/0x2Ch calls per real second (~18.2 Hz); the port takes the value from
 * the sys layer. */
extern uint32_t g_clock_ticks;
/* Measure the clock tick rate into g_clock_ticks (orig counts INT 21h/0x2Ch
 * ticks over one real second).  orig: 0x04561d */
void calibrate_clock(void);
/* Delay: compute n = max(1, (arg*g_clock_ticks + 500) / 1000) and perform n
 * clock reads (the original's busy-wait of INT 21h/0x2Ch). No-op if arg == 0.
 *  orig: 0x04564f */
void delay_units(uint32_t arg);

/* ---- L0 batch E (2): swap-global + jump-table field accessor ---- */
/* Swap an argument with global DAT_0005363c, returning the old global value.
 * (A state save/restore thunk used indirectly.)  orig: 0x045ae8 */
extern uint32_t g_dat_5363c;
uint32_t swap_dat_5363c(uint32_t v);
/* Jump-table case: return the dword at offset 0x28 of the struct the
 * dispatcher places in EBP.  orig: 0x04642a (body: XOR EBX,EBX; ADD EBX,[EBP+0x28]; RET) */
uint32_t case_field_0x28(uintptr_t s);

/* ---- L0 batch F (6): misc primitives + spurious ---- */
/* Return the current stack depth: ESP - g_stack_bound. Port approximates ESP
 * with the address of a local (a few bytes off true ESP, same semantic).
 *  orig: 0x04917e (MOV EAX,ESP; SUB EAX,[0x53394]; RET) */
uint32_t stack_depth(void);
/* Set global DAT_00053640 = 0x8000.  orig: 0x049187 */
extern uint32_t g_dat_53640;
void set_53640(void);
/* Uppercase a lowercase ASCII letter: a-z (0x61-0x7a) -> A-Z, else unchanged.
 * (Mirror of to_lower, which is the reverse direction.)  orig: 0x0494bd */
uint32_t to_upper(uint32_t c);
/* 0x045605: upper-case a NUL-terminated string in place (per-char to_upper). */
void     string_upper(char *s);
/* 0x04457f: hex-digit value: '0'-'9'->0-9, 'a'-'f' (via to_lower)->10-15, else 16. */
int      hex_digit_value(int c);
/* 0x041e9f: LCG PRNG step. state = state*0x41c64e6d + 0x3039 (mod 2^32);
 * returns (state>>16)&0x7fff (15-bit), or 0 if the PRNG is uninitialized. */
uint32_t prng_next(void);
/* 0x041ec3: seed the PRNG state word (no-op if uninitialized). */
void     prng_seed(uint32_t seed);
/* 0x041f42: signed itoa (via itoa32): if base==10 && value<0 write '-' and
 * negate; returns the ORIGINAL buffer start (not the end pointer). */
char    *itoa32s(int32_t value, char *buf, uint32_t base);
/* 0x0494a2: same as itoa32s but calls itoa32_tbl (0x49459) instead of itoa32. */
char    *itoa32s_tbl(int32_t value, char *buf, uint32_t base);
/* Store value|0x40000 into the DAT_00053730 table at [index] (sets bit 22 as a "set" flag).
 *  orig: 0x049325 (store [0x53730 + index*4] = value | 0x40000)
 * DAT_00053730 has ONE table; the port once modelled it twice (g_dat_53730 read by fn_0492d0,
 * g_arr_53730 written by store_arr53730), which let the two halves drift apart. g_arr_53730 is now
 * a macro alias of g_dat_53730 so there is a single table pointer. */
#define g_arr_53730 g_dat_53730
void store_arr53730(uint32_t index, uint32_t value);
/* No-op (single RET).  orig: 0x049109 */
void noop_9109(void);
/* DOS IOCTL (INT 21h/0x44h/AL=0) on a device handle; returns bit7 of the
 * rotated result. Port stubs the INT via sys_dos_ioctl then applies the exact
 * RCL/ROR bit ops.  orig: 0x0492b2 */
uint32_t ioctl_device(uint32_t handle);

/* ---- L0 batch G (2): fixed-point number constructor ---- */
/* Fixed-point number (10 bytes, 4-aligned): 64-bit normalized mantissa {hi,lo}
 * + 16-bit exponent. The mantissa's MSB is placed at bit 31; exp =
 * (MSB-bitpos) + 0x3fff, with bit 0x8000 set for negatives. Built from a 32-bit
 * int.  orig: 0x047f1f (unsigned), 0x047f0e (signed entry / sign thunk). */
struct fxnum { uint32_t lo; uint32_t hi; uint32_t exp; };
void fx_from_u32(struct fxnum *out, uint32_t v);   /* 0x47f1f */
void fx_from_s32(struct fxnum *out, int32_t v);    /* 0x47f0e */
void fx_from_f32(struct fxnum *out, uint32_t v);   /* 0x4811a (IEEE-754 float32) */
void float_str_sci(char *s, const uint8_t *rec);   /* 0x45d56 sci-notation cleanup */
void fmt_u64_fixed(const uint64_t *val, char *buf, int width); /* 0x4a8b9 */
void fx_mantissa_scale(uint32_t *out, const uint32_t *in);     /* 0x4a84e */
uint32_t fx64_to_f32(uint32_t lo, uint32_t hi);                /* 0x49132 */
uint8_t *fmt_decimal(uint8_t *out, int32_t intlen, int32_t fractlen,
                     const uint8_t *src);                       /* 0x49db2 */
int32_t  fmt_int_signed(uint8_t *out, int32_t width, int32_t maxw,
                        int32_t val);                           /* 0x49d0b */
int32_t  fx_extract(const uint8_t *p, int32_t mode);            /* 0x47ea4 */
int32_t  fx_norm32(const uint8_t *p);                           /* 0x47fd4 */
void     fx_double_exp(double d, uint32_t *out);                /* 0x4a9da */
int      fx_double_naninf(double d, char *out);                 /* 0x4a2a7 */
int      fx_compare(const uint8_t *a, const uint8_t *b);        /* 0x47ab6 */
void     fx_classify_status(const uint8_t *fx, uint8_t *ctx, uint32_t edi); /* 0x48666 */
uint64_t fx_mul64(uint32_t a_lo, uint32_t a_hi, uint32_t b_lo, uint32_t b_hi,
                  uint32_t esi);                                          /* 0x47d4e */
/* 0x048f4c: register-ABI. acc=arr[0] (lo:hi); loop `count` times: acc =
 * fn_04794a(fx_mul64(acc, mant, (exp_base<<16)|arr[0].exp), arr[i+1],
 * (arr[i+1].exp<<16)|arr[0].exp). Returns final (EDX<<32)|EAX. Reg-ABI: EAX=mant_lo,
 * EDX=mant_hi, EBX=count, ECX=arr, ESI=exp_base (low16). out_final_esi (optional,
 * may be NULL) receives the loop's final 32-bit ESI (after the last iteration's ROL);
 * callers (e.g. fn_0487b0) re-derive it via PUSH/POP. Only its low16 survives the
 * caller juggle, but the full 32b is exposed to mirror the register. */
uint64_t fn_048f4c(uint32_t mant_lo, uint32_t mant_hi, uint32_t count,
                   const fx128 *arr, uint32_t exp_base, uint32_t *out_final_esi); /* 0x048f4c */
/* 0x048f2c: register-ABI in-place. a = 048f4c(a, arr, count, exp=a->exp); a->exp
 * unchanged. EAX=a (in/out {lo,hi,exp}), EDX=arr, EBX=count. */
void fn_048f2c(fx128 *a, const fx128 *arr, uint32_t count);               /* 0x048f2c */
/* 0x048f9e: register-ABI in-place. a = (048f4c(a*a, count, arr, a->exp)) * a;
 * a->exp unchanged. EAX=a (in/out), EBX=count, EDX=arr. */
void fn_048f9e(fx128 *a, uint32_t count, const fx128 *arr);               /* 0x048f9e */
void fn_048a48(fx128 *a, uint16_t esi);                                   /* 0x048a48 */
void fn_048c3f(fx128 *a);                                                 /* 0x048c3f (esi=0) */
void fn_048c49(fx128 *a);                                                 /* 0x048c49 (esi=2) */
void fn_048c56(fx128 *a);                                                 /* 0x048c56 (esi=1) */
/* 0x048754: register-ABI in-place. a = fn_04791a(a, a, 0, 0x80000000, 0x3fff).
 * EAX=a (in/out). Sign/normalize op (b=0x80000000_00000000, exp_hi=0x3fff). */
void fn_048754(fx128 *a);                                                  /* 0x048754 */
void fn_048422(fx128 *a);                                                  /* 0x048422: FPU/fx128 composite (354B) */
void fn_0487b0(fx128 *a);                                                  /* 0x0487b0: FPU/fx128 composite (464B) */
void fn_04876d(fx128 *a, fx128 *b);                                        /* 0x04876d: a=0487b0(a); b=a*b; b=b*const (in place) */
void fn_048c8c(fx128 *a);                                                  /* 0x048c8c: in-place fx128 composite (negate+normalize+square+poly); sign-sentinel on normalizable inputs */
void     fx_stub_4a923(void);                                             /* 0x4a923 */
void     fx_round_down(uint32_t *v, uint16_t mode);                       /* 0x481df */

/* ---- L0 batch H (4): fx compare + decimal parse + DOS/FPU stubs ---- */
/* Lexicographic less-than on an fxnum by (exp, hi, lo).  orig: 0x0498c6 */
uint32_t fx_less(const struct fxnum *a, const struct fxnum *b);
/* Parse leading ASCII decimal digits into *out (0 if no leading digit).
 *  orig: 0x049a48 */
const char *parse_decimal(const char *s, uint32_t *out);

/* ---- L1: time-string parser (0x049a6e) ----
 * Parse "<prefix><opt sign><HH>[:<MM>[:<SS>]]". prefix_buf (>=31 bytes) gets the
 * leading run of non-{digit/sign/comma/NUL} chars (clamped to 30), NUL-terminated.
 * *secs_out = sec + min*60 + hour*3600 (negated if the break char was '-').
 * Returns the position after the last parsed component, or break_pos+1 (with
 * *secs_out left unchanged) when the char there is not a digit.
 * LATENT BUG (replicated 1:1): the scan resumes at break_pos+1 unconditionally,
 * so a pure "HH:MM:SS" with no leading sign skips its first digit. */
const char *parse_time_str(const char *str, char *prefix_buf, uint32_t *secs_out);
/* DOS protected-mode/real-mode disk+segment setup (INT 2Fh / INT 21h/0x30h).
 * No-op stub in the modern port (the OS provides this).  orig: 0x0490b0 */
void dos_realmode_setup(void);
/* FPU init: return a standard x87 control word (modern build always has an
 * x87 FPU; the original detected it at runtime).  orig: 0x04910a */
uint16_t fpu_init(void);

/* ---- L0 batch I (1 real + 1 spurious) ---- */
/* Case-insensitive compare of up to n bytes: lowercases A-Z in both, returns
 * (a-b) at the first mismatch or 0. Stops early at a NUL byte in b.  orig: 0x04a7a2.
 * (0x048312 in the same address range is spurious disassembled data - only a DATA
 *  xref, no CALL.) */
int32_t strn_casecmp(const uint8_t *a, const uint8_t *b, uint32_t n);
char *config_value_ptr(const char *key);   // 0x4a24d L1: ptr after '=' in matching "key=value" entry, or 0

/* ---- L0 batch J (1 real) ---- */
/* In-place tokenizer (orig: 0x04595b): splits s on space/tab; unescapes \\" in
 * any token and \\\\ inside quoted tokens; quoted tokens ("...") have their
 * quotes stripped. If out != NULL, out[i] = pointer to token i's start (in the
 * compacted s) and each token is NUL-terminated in place; if out == NULL it only
 * counts (s left unmodified). *end_out receives the final scan position.
 * Returns the token count. */
uint32_t tokenize(char *s, char **out, char **end_out);

/* 0x04589e (L6): build a tokenized command line in the heap from DAT_00053388
 * (heap offset); publishes DAT_00053378 (count+1) / DAT_0005337c (appended-dword
 * offset) and copies them to DAT_00064090 / DAT_00064094. */
extern uint32_t g_dat_53388, g_dat_5338c, g_dat_53378, g_dat_5337c;
extern uint32_t g_dat_64090, g_dat_64094;
void fn_04589e(void);

/* ---- L0 batch K ---- */
/* DOS date/time packer (orig: 0x045687). Fills a struct from the DOS clock:
 * f4=day, f8=month, f14=low8(minute*256+second-0x76c), f20=0xffffffff, f0=fc=0,
 * f10=0xffffffff. The SHL ECX,16 zeroes the low byte of each packed local, so
 * the DOW-based return and the "day-change" branch are dead and it returns 0. */
struct datetime_pack {
    uint32_t f0, f4, f8, fc, f10, f14, f18, f1c, f20;  /* 9 dwords = 0x24; f20@0x20 == dec_datetime mode (layout fix) */
};
uint32_t datetime_pack(struct datetime_pack *out);
/* Current-DOS-clock recurring-date offset (orig: 0x04297a). Packs the DOS clock
 * into a local struct via datetime_pack, then returns fn_04574a on it; *out gets
 * the result if non-NULL. Verified against the fn_049613 no-op stub (mode=-1). */
int32_t fn_04297a(int32_t *out);

#endif
