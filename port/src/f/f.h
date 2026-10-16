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
extern uint32_t  g_heap_head;      /* DAT_00053350: all-blocks chain head */
extern uint32_t  g_heap_cursor;    /* DAT_00053354: malloc search cursor */
extern uint32_t  g_heap_highwater; /* DAT_00053358: global size high-water */
extern uint32_t  g_heap_seg_limit; /* DAT_00053384: DPMI segment limit */
extern uint8_t   g_heap_cfg_b2;    /* DAT_000533b2: top-check mode byte */
extern uint8_t   g_heap_cfg_b4;    /* DAT_000533b4: top-check flag */
extern uint8_t   g_heap_flag;      /* DAT_00063fbd: wrappers zero it, never read */
extern uint32_t  g_ds_segment;     /* DPMI DS selector (flat model: 0) */

/* Arena support (the original grows via DPMI; the port uses a static arena). */
void     heap_arena_init(void);    /* set up the static arena + globals */
uint32_t heap_arena_alloc(uint32_t size); /* 64-byte-aligned region slot, or 0 */

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
    uint32_t _r1;                          /* +0x04 */
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
/* Global set by the DOS4GW entry (0x420bc) during startup init; returned by
 * get_dat_53390.  orig: DAT_00053390 */
extern int g_dat_53390;
int get_dat_53390(void);   /* orig: 0x044be6 */
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
/* Parse a modifier-prefix string into a flags byte at out+0x14 (and out+0x16):
 * '-' |=0x8, '#' |=0x1, '+' |=0x4 & &=~0x2, ' ' |=0x2 (only if 0x4 unset),
 * '0' -> out+0x16 = '0'. Stops at the first other character.  orig: 0x044fd7 */
void parse_mod_prefix(const uint8_t *s, uint8_t *out);
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
/* Store value|0x40000 into g_arr_53730[index] (sets bit 22 as a "set" flag).
 *  orig: 0x049325 (store [0x53730 + index*4] = value | 0x40000) */
extern uint32_t *g_arr_53730;
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

/* ---- L0 batch H (4): fx compare + decimal parse + DOS/FPU stubs ---- */
/* Lexicographic less-than on an fxnum by (exp, hi, lo).  orig: 0x0498c6 */
uint32_t fx_less(const struct fxnum *a, const struct fxnum *b);
/* Parse leading ASCII decimal digits into *out (0 if no leading digit).
 *  orig: 0x049a48 */
void parse_decimal(const char *s, uint32_t *out);
/* DOS protected-mode/real-mode disk+segment setup (INT 2Fh / INT 21h/0x30h).
 * No-op stub in the modern port (the OS provides this).  orig: 0x0490b0 */
void dos_realmode_setup(void);
/* FPU init: return a standard x87 control word (modern build always has an
 * x87 FPU; the original detected it at runtime).  orig: 0x04910a */
uint16_t fpu_init(void);

#endif
