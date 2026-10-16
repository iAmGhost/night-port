/* test_f1.c — verification for batch 1 (13 L0 functions).
 * Strategy: differential tests against an independent reference model,
 * plus I/O sequence capture for the board ports.
 * Exit 0 = all pass. */
#include "../src/f/f.h"
#include "../src/sys/sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int failures = 0;
static void check(int ok, const char *what) {
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

/* ---------- reference models (independent, straightforward C) ---------- */

static void ref_put_px(uint8_t *buf, int x, int y, uint8_t c, int stride) {
    buf[(size_t)y * stride + x] = c;
}

/* dst ends up at (dst - n + 1) .. dst; src[0] lands at dst, src[1] at dst-1, ...
 * (asm: MOV [EDI],[ESI]; INC ESI; DEC EDI — write starts AT dst) */
static void ref_cpy_back(uint8_t *dst, const uint8_t *src, int n) {
    for (int i = 0; i < n; i++) dst[-i] = src[i];
}

static void ref_blt_rows(uint8_t *dst, const uint8_t *src, int w, int h, int sp, int dp) {
    for (int r = 0; r < h; r++)
        for (int i = 0; i < w; i++)
            dst[r * dp + i] = src[r * sp + i];
}

static void ref_hline(uint8_t *buf, int b, int c, int d, int e, uint8_t f) {
    for (int i = 0; i < e; i++) buf[d * b + c + i] = f;
}

static void ref_vline(uint8_t *buf, int b, int c, int d, int e, uint8_t f) {
    for (int i = 0; i < e; i++) buf[d * b + c + i * b] = f;
}

static void ref_rect(uint8_t *buf, int b, int c, int d, int e, uint8_t f, int stride) {
    for (int r = 0; r < e; r++)
        for (int i = 0; i < d; i++)
            buf[(size_t)(c + r) * stride + b + i] = f;
}

/* ---------- tests ---------- */

static void test_put_px(void) {
    printf("put_px (0x040024)\n");
    g_video_stride = 320;
    uint8_t a[200 * 320], r[200 * 320];
    memset(a, 0x5a, sizeof a); memset(r, 0x5a, sizeof r);
    put_px(a, 7, 123, 0xAB); ref_put_px(r, 7, 123, 0xAB, 320);
    put_px(a, 319, 199, 0x01); ref_put_px(r, 319, 199, 0x01, 320);
    check(memcmp(a, r, sizeof a) == 0, "pixel lands at y*stride + x");
}

static void test_cpy_back(void) {
    printf("cpy_back (0x040475)\n");
    for (int n = 1; n <= 40; n += 7) {
        uint8_t a[64], r[64];
        memset(a, 0, sizeof a); memset(r, 0, sizeof r);
        uint8_t src[40];
        for (int i = 0; i < n; i++) src[i] = (uint8_t)(i * 31 + n);
        int dst = 40 + n / 2;
        cpy_back(a + dst, src, n);
        ref_cpy_back(r + dst, src, n);
        if (memcmp(a, r, sizeof a)) { check(0, "matches reference"); return; }
    }
    check(1, "reverse-traversal copy matches reference (n=1..34)");
}

static void test_cpy_dw(void) {
    printf("cpy_dw (0x0404c6)\n");
    for (int n = 0; n <= 12; n += 5) {
        uint32_t a[20] = {0}, r[20] = {0};
        uint32_t src[12];
        for (int i = 0; i < 12; i++) src[i] = 0xA5A50000u + i;
        cpy_dw(src, n, a + 3);
        memcpy(r + 3, src, (size_t)n * 4);
        if (memcmp(a, r, sizeof a)) { check(0, "matches reference"); return; }
    }
    check(1, "dword copy matches reference (n=0..10)");
}

static void test_blt_rows(void) {
    printf("blt_rows (0x040491)\n");
    for (int w = 1; w <= 13; w++) {          /* odd + even widths */
        for (int h = 1; h <= 5; h++) {
            int sp = w + (w & 1);             /* pitch >= width, odd pitch too */
            int dp = w + 2;
            uint8_t s[16 * 16], d[16 * 16], r[16 * 16];
            for (int i = 0; i < 256; i++) { s[i] = (uint8_t)i; d[i] = (uint8_t)(i + 1); r[i] = (uint8_t)(i + 1); }
            blt_rows(s, sp, (uint32_t)w, h, d, dp);
            ref_blt_rows(r, s, w, h, sp, dp);
            if (memcmp(d, r, 256)) {
                check(0, "matches reference"); return;
            }
        }
    }
    check(1, "strided row blit matches reference (w,h sweep, odd/even)");
}

static void test_hline_vline_rect(void) {
    printf("hline_set (0x0404db) / vline_set (0x040506) / rect_fill (0x04052c)\n");
    int stride = 320;
    g_video_stride = stride;
    for (int len = 1; len <= 17; len += 4) {
        uint8_t a[8192], r[8192];
        memset(a, 0x11, sizeof a); memset(r, 0x11, sizeof r);
        int b = 3, c = 9, d = 60;
        hline_set(a, b, c, d, (uint32_t)len, 0x7E);
        ref_hline(r, b, c, d, len, 0x7E);
        if (memcmp(a, r, sizeof a)) { check(0, "hline matches reference"); return; }
        int step = 40, col = 7, row = 3;
        vline_set(a, step, col, row, len, 0x3C);
        ref_vline(r, step, col, row, len, 0x3C);
        if (memcmp(a, r, sizeof a)) { check(0, "vline matches reference"); return; }
    }
    check(1, "hline/vline fill match reference (odd+even lengths)");
    for (int w = 1; w <= 17; w += 5) {
        for (int h = 1; h <= 6; h += 2) {
            uint8_t a[200 * stride], r[200 * stride];
            memset(a, 0x22, sizeof a); memset(r, 0x22, sizeof r);
            rect_fill(a, 5, 30, (uint32_t)w, h, 0xC3);
            ref_rect(r, 5, 30, w, h, 0xC3, stride);
            if (memcmp(a, r, sizeof a)) { check(0, "rect matches reference"); return; }
        }
    }
    check(1, "rect fill matches reference (odd/even w, multi-row)");
}

static void test_board(void) {
    printf("board_out (0x040042) / board_in (0x040061)\n");
    sys_ports_reset();
    uint8_t outbuf[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    board_out(0x55, 3, outbuf);
    check(sys_inb(0x3c8) == 0x55, "out: control byte to 0x3C8");
    uint8_t seen[9];
    int ok = sys_hist(seen, 9) == 9 && memcmp(seen, outbuf, 9) == 0;
    check(ok, "out: 3n bytes streamed to 0x3C9 in order");
    sys_ports_reset();
    for (int i = 0; i < 9; i++) sys_port_set(0x3c9, (uint8_t)(0xA0 + i)); /* seed: last set wins */
    uint8_t inbuf[9];
    board_in(0x77, 3, inbuf);
    check(sys_inb(0x3c7) == 0x77, "in: control byte to 0x3C7");
    check(inbuf[0] == 0xA8 && inbuf[8] == 0xA8, "in: 3n bytes read from 0x3C9");
}

static void test_abs(void) {
    printf("abs32 (0x041f76)\n");
    check(abs32(0) == 0 && abs32(123) == 123 && abs32(-123) == 123
          && abs32(-2147483647 - 1) == (-2147483647 - 1), "abs values (INT_MIN wraps as on x86)");
}

static const char *g_fatal_msg;
static void fatal_hook(const char *m) { g_fatal_msg = m; }

/* ---------- batch 2: segment / memory / string / FPU primitives ---------- */

static void test_seg_save(void) {
    printf("seg_save (0x041df2)\n");
    uint16_t got[6] = {0};
    uint16_t want[6];
    sys_get_segments(want);
    seg_save(got);
    check(memcmp(got, want, sizeof got) == 0,
          "captures 6 segments in ES,CS,SS,DS,FS,GS order (== sys table)");
    check(got[0] == 0 && got[1] == 0x23 && got[2] == 0x2b && got[3] == 0,
          "flat-model values (ES=0 CS=0x23 SS=0x2b DS=0)");
    check(got[2] == sys_ss(), "SS matches sys_ss");
}

static void test_mem_copy(void) {
    printf("mem_copy (0x041e6c)\n");
    int ok_ret = 1;
    for (int n = 0; n <= 36; n++) {        /* spans the dword+tail boundary */
        uint8_t a[80], r[80], src[40];
        for (int i = 0; i < 80; i++) { a[i] = (uint8_t)(0x80 ^ i); r[i] = a[i]; }
        for (int i = 0; i < 40; i++) src[i] = (uint8_t)(i * 7 + n);
        int base = 20 + (n & 1);           /* odd + even dst alignment */
        uint8_t *ret = mem_copy(a + base, src, (uint32_t)n);
        if (ret != a + base) ok_ret = 0;
        memcpy(r + base, src, (size_t)n);
        if (memcmp(a, r, sizeof a)) { check(0, "byte-identical to memcpy (n sweep)"); return; }
    }
    check(ok_ret, "returns the original dst pointer");
    check(1, "byte-identical to memcpy (n=0..36, odd/even dst align)");
}

static void test_rng_state_ptr(void) {
    printf("rng_state_ptr (0x041e99)\n");
    uint32_t *p = rng_state_ptr();
    check(p == &g_rng_state, "returns &g_rng_state");
    g_rng_state = 0;
    *p = 0x12345678;
    check(g_rng_state == 0x12345678, "write through pointer updates global");
}

static void ref_itoa(uint32_t v, char *out, uint32_t base) {
    static const char d[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[33]; int n = 0, i = 0;
    do { tmp[n++] = d[v % base]; v /= base; } while (v != 0);
    while (n > 0) out[i++] = tmp[--n];
    out[i] = 0;
}

static void test_itoa32(void) {
    printf("itoa32 (0x041ef8)\n");
    int ok = 1;
    for (uint32_t base = 2; base <= 36; base++) {
        uint32_t vals[4] = {0, 1, base - 1, 0xffffffffu};
        for (int k = 0; k < 4; k++) {
            char a[40], r[40];
            memset(a, 0xEE, sizeof a);
            char *ret = itoa32(vals[k], a, base);
            ref_itoa(vals[k], r, base);
            if (ret != a || strcmp(a, r) != 0) { ok = 0; }
        }
    }
    check(ok, "matches reference for base 2..36 (0,1,base-1,max) + returns out");
    char b[8];
    itoa32(255, b, 16);
    check(strcmp(b, "ff") == 0, "255 in base 16 == \"ff\" (lowercase a-z)");
}

static void test_str_len(void) {
    printf("str_len (0x041f5d)\n");
    const char *strs[] = {"", "a", "hello", "NIGHT PORT 320x200"};
    int ok = 1;
    for (int i = 0; i < 4; i++)
        if (str_len(strs[i]) != strlen(strs[i])) ok = 0;
    check(ok, "matches strlen for empty + non-empty");
}

static void test_fpu_rndint(void) {
    printf("fpu_rndint (0x041f7e)\n");
    /* x87 FRNDINT, RM=11 = round-to-nearest-even (half-to-even). */
    check(fpu_rndint(2.5) == 2.0 && fpu_rndint(3.5) == 4.0,
          "half-to-even: 2.5->2, 3.5->4");
    check(fpu_rndint(0.5) == 0.0 && fpu_rndint(1.5) == 2.0,
          "half-to-even: 0.5->0, 1.5->2");
    check(fpu_rndint(2.4) == 2.0 && fpu_rndint(2.6) == 3.0,
          "non-ties round to nearest");
    check(fpu_rndint(5.0) == 5.0 && fpu_rndint(-2.5) == -2.0,
          "integers unchanged; -2.5->-2 (even)");
}

/* ---------- batch 3: low-level I/O + interrupt-vector primitives ---------- */

static void test_outb(void) {
    printf("outb (0x04231d)\n");
    sys_ports_reset();
    outb(0x40, 0x12);   /* PIT channel 0 */
    check(sys_inb(0x40) == 0x12, "PIT port 0x40 recorded");
    outb(0x43, 0x36);   /* PIT control */
    check(sys_inb(0x43) == 0x36, "PIT control port 0x43 recorded");
    outb(0x20, 0x20);   /* 8259 EOI */
    check(sys_inb(0x20) == 0x20, "PIC EOI port 0x20 recorded");
    outb(0x3c9, 0xAB);  /* board data port still in history */
    uint8_t h[1];
    check(sys_hist(h, 1) == 1 && h[0] == 0xAB, "board 0x3C9 still in write history");
}

static void test_irq_nops(void) {
    printf("seg_ds_load (0x042310) / irq_cli (0x04231b) / irq_sti (0x04232b)\n");
    seg_ds_load(); irq_cli(); irq_sti();   /* must not crash */
    check(1, "privileged no-ops run without fault");
}

static void test_isr(void) {
    printf("isr_get (0x04232d) / isr_set (0x042361)\n");
    check(isr_get(0x1c) == 0 && isr_get(0xa) == 0,
          "isr_get returns 0 (no real OS vector)");
    isr_set(0x1c, 0x1234, 0x2b);           /* must not crash */
    check(1, "isr_set no-op runs without fault");
}

/* ---------- batch 4: I/O read, no-op cb, str_icmp, res_reader, digit, fmt ---------- */

static void test_inb(void) {
    printf("inb (0x04238c)\n");
    sys_ports_reset();
    sys_port_set(0x40, 0x77);
    check(inb(0x40) == 0x77, "inb reads a seeded port");
    check(inb(0x20) == 0, "unset port -> 0");
}

static void test_cb_noop(void) {
    printf("cb_noop (0x0423b6)\n");
    cb_noop();                       /* must not crash */
    check(1, "cb_noop runs without fault");
}

static void test_str_icmp(void) {
    printf("str_icmp (0x0423e5)\n");
    check(str_icmp("ABC", "abc", 3) == 0, "case-insensitive equal");
    check(str_icmp("AbC", "aBc", 3) == 0, "mixed case equal");
    check(str_icmp("abc", "abd", 3) == ('c' - 'd'), "differs negative");
    check(str_icmp("abd", "abc", 3) == ('d' - 'c'), "differs positive");
    check(str_icmp("abX", "abY", 2) == 0, "n limits before mismatch");
    check(str_icmp("hello", "hello", 0) == 0, "n=0 -> 0");
    check(str_icmp("abc", "ab", 3) == ('c' - 0), "mismatch vs NUL");
}

static void test_res_consume(void) {
    printf("res_consume (0x042438)\n");
    uint8_t buf[16] = {0};
    res_reader r = { .pos = buf, .remaining = 10, .end = buf + 4, .flags = 0x10, .id = 0 };
    int rc = res_consume(&r, 4);        /* end-pos=4 <= 4 <= remaining=10 -> ok */
    check(rc == 0, "success rc==0");
    check(r.pos == buf + 4, "pos advanced by amount");
    check(r.remaining == 6, "remaining decremented");
    check(r.flags == 0, "flag bit 0x10 cleared");
    res_reader r2 = { .pos = buf, .remaining = 10, .end = buf + 8, .flags = 0, .id = 0 };
    rc = res_consume(&r2, 3);           /* 3 < end-pos=8 -> fail */
    check(rc == 1, "fail rc==1");
    check(r2.pos == buf + 8, "pos = end on fail");
    check(r2.remaining == 0, "remaining = 0 on fail");
}

static void test_digit_chr(void) {
    printf("digit_chr (0x0427e7)\n");
    check(digit_chr(0) == '0' && digit_chr(9) == '9', "0-9 -> '0'-'9'");
    check(digit_chr(10) == 'a' && digit_chr(35) == 'z', "10-35 -> 'a'-'z'");
    check(digit_chr(15) == 'f', "15 -> 'f'");
}

static void test_fmt_emit(void) {
    printf("fmt_emit (0x0428e5)\n");
    uint8_t dst[32] = {0};
    fmt_state st = { .pos = dst, .width = 0, .slen = 0, .zeros = 0, .count = 0 };
    fmt_emit(&st, 'H');
    fmt_emit(&st, 'i');
    check(dst[0] == 'H' && dst[1] == 'i', "bytes written in order");
    check(st.pos == dst + 2, "pos advanced");
    check(st.count == 2, "count == 2");
}

static void test_canary(void) {    printf("canary (0x0418e6 / 0x0418ee / 0x041901)\n");
    canary_ss_save();
    check(g_ss_saved == sys_ss(), "ss_save stores SS snapshot");
    g_stack_bound = 0x4000;
    /* normal prologue: small frame, plenty of room -> returns d = ESP - frame */
    uintptr_t r = canary_id(0xDEAD, 0x7c);
    check(r == 0xDEAD, "canary_id preserves caller EAX");
    r = canary_check(0x7c);
    check(r > (uintptr_t)g_stack_bound, "normal frame returns distance > bound");
    /* overflow (no room below bound) with foreign SS: returns SS, no death */
    g_stack_bound = (int32_t)-1;              /* d > bound never true */
    g_ss_saved = (uint16_t)(sys_ss() + 1);
    check(canary_check(0x7c) == (uintptr_t)sys_ss(),
          "overflow + SS mismatch returns SS (no death)");
    /* overflow with matching SS: fatal path (hooked, no abort) */
    g_ss_saved = sys_ss();
    g_fatal_msg = 0;
    sys_set_fatal_handler(fatal_hook);
    canary_check(0x7c);
    check(g_fatal_msg && strncmp(g_fatal_msg, "Stack Overflow", 14) == 0,
          "overflow + SS match takes fatal path");
    sys_set_fatal_handler(0);
}

static void test_get_key(void) {
    printf("get_key (0x04295d)\n");
    g_keybuf = 'A';
    check(get_key() == 'A' && g_keybuf == 0, "returns buffered key and clears it");
    g_keybuf = 0;
    check(get_key() == 0, "no buffer -> sys_getkey stub returns 0");
    g_keybuf = 0x2A;
    check(get_key() == 0x2A, "returns arbitrary buffered byte");
}

static void test_mem_cmp(void) {
    printf("mem_cmp (0x0429a8)\n");
    check(mem_cmp("abc", "abc", 3) == 0, "equal -> 0");
    check(mem_cmp("abc", "abd", 3) == -1, "a<b -> -1");
    check(mem_cmp("abd", "abc", 3) == 1, "a>b -> +1");
    check(mem_cmp("abx", "abx", 2) == 0, "n limits the compare");
    check(mem_cmp("abc", "ab", 2) == 0, "mismatch beyond n ignored");
    check(mem_cmp("abc", "abd", 0) == 0, "n=0 -> 0");
    check(mem_cmp("abc", "abd", 3) == -1, "first-diff decides (not last)");
}

static void test_cb_noop2(void) {
    printf("cb_noop2 (0x042a07)\n");
    cb_noop2();
    check(1, "calls without crash");
}

static void test_get_key_echo(void) {
    printf("get_key_echo (0x043263)\n");
    g_keybuf = 'B';
    check(get_key_echo() == 'B' && g_keybuf == 0, "returns buffered key and clears it");
    g_keybuf = 0;
    check(get_key_echo() == 0, "no buffer -> sys_getkey_echo stub returns 0");
}

static void test_heap_grow(void) {
    printf("heap_grow (0x04322c)\n");
    check(heap_grow(0x100) == 0, "stub always returns 0 (no growth)");
    check(heap_grow(0) == 0, "size 0 also returns 0");
}

static void test_memsetw(void) {
    printf("memsetw (0x0432b7)\n");
    uint32_t buf[24];
    int all_ok = 1;
    for (int n = 0; n <= 10; n++) {
        for (int i = 0; i < 24; i++) buf[i] = 0xEEEE;
        uint32_t *r = memsetw(buf, 0xA5A5, (uint32_t)n);
        if (r != buf + n) { all_ok = 0; }
        for (int i = 0; i < n; i++) if (buf[i] != 0xA5A5) all_ok = 0;
        for (int i = n; i < 24; i++) if (buf[i] != 0xEEEE) all_ok = 0;
    }
    check(all_ok, "fills exactly n dwords for n=0..10, leaves rest, returns dst+n");
    uint32_t one[1] = {0};
    check(memsetw(one, 0xDEADBEEF, 1) == one + 1 && one[0] == 0xDEADBEEF,
          "single dword fill");
}

/* ---- heap reference model (batches 7-8) ---------------------------
 * Independent invariant checker: walk the region's data area AND the free
 * list separately, then cross-check that they describe the same set of blocks
 * and that the counters agree. Catches any pointer/size corruption.
 * what==NULL => quiet on success (used by the stress loop). */
static inline uint32_t *hw(uint32_t off) { return (uint32_t *)(g_heap_base + off); }
static int heap_verify(uint32_t region, const char *what) {
    uint32_t total = hw(region)[0];
    uint32_t data0 = region + 0x2c;
    uint32_t sent  = region + 0x20;
    /* pass 1: walk the data area block by block */
    uint32_t cur = data0, count = 0, sum = 0, nfree = 0, nalloc = 0;
    uint32_t free_off[512]; int fi = 0;
    while (cur != region + total) {
        uint32_t sz   = hw(cur)[0];
        uint32_t real = sz & 0xFFFFFFFEu;
        if (cur + real > region + total) { printf("  !! block overrun @%x\n", cur); return 1; }
        if (sz & 1) nalloc++; else { if (fi < 512) free_off[fi++] = cur; nfree++; }
        sum += real; cur += real; count++;
        if (count > 8192) { printf("  !! too many blocks\n"); return 1; }
    }
    if (hw(region + total)[0] != 0xFFFFFFFFu) { printf("  !! end marker missing\n"); return 1; }
    if (sum != total - 0x2c) { printf("  !! size sum %x != %x\n", sum, total - 0x2c); return 1; }
    /* pass 2: walk the free list from sentinel.next, back to sentinel */
    int flc = 0, si = 0; uint32_t flseen[512];
    uint32_t p = hw(sent)[2];
    while (p != sent) {
        uint32_t sz = hw(p)[0];
        if (sz & 1) { printf("  !! allocated block in free list %x\n", p); return 1; }
        if (si < 512) flseen[si++] = p;
        flc++;
        if (flc > 8192) { printf("  !! free list not circular\n"); return 1; }
        p = hw(p)[2];
    }
    if (flc != (int)nfree) { printf("  !! free-list count %d != %u\n", flc, nfree); return 1; }
    for (int i = 0; i < si; i++) {
        int found = 0;
        for (int j = 0; j < fi; j++) if (free_off[j] == flseen[i]) { found = 1; break; }
        if (!found) { printf("  !! free-list block %x not in data walk\n", flseen[i]); return 1; }
    }
    if (hw(region)[6] != nalloc) { printf("  !! alloc_count %u != %u\n", hw(region)[6], nalloc); return 1; }
    if (hw(region)[7] != nfree)  { printf("  !! free_count %u != %u\n", hw(region)[7], nfree); return 1; }
    if (what) printf("  ok: %s (blocks=%u free=%u alloc=%u)\n", what, count, nfree, nalloc);
    return 0;
}

static void test_heap_arena(void) {
    printf("heap arena + acquire (FUN_00043071 replacement)\n");
    heap_arena_init();
    check(g_heap_head == 0, "no regions after init");
    uint32_t r1 = heap_arena_alloc(0x200);
    uint32_t r2 = heap_arena_alloc(0x300);
    check(r1 == 0x40,   "first slot at offset 0x40 (offset 0 reserved as NULL guard)");
    check(r2 == 0x240,  "second slot right after (64-aligned)");
    check(heap_arena_alloc(0x800000) == 0, "oversize slot -> 0 (arena full)");
    heap_arena_init();
    uint32_t data = heap_acquire_region(0x1000);
    check(data != 0, "acquire_region returns a data pointer");
    uint32_t region = data - 0x30;   /* data = region + 0x2c + 4 */
    check(g_heap_head == region, "created region is the chain head");
    check(hw(region)[0] == 0x102c, "total_size = 0x2c + data_area");
    check(hw(region + 0x2c)[0] == 0x1000 && !(hw(region + 0x2c)[0] & 1),
          "whole data area is one free block");
    heap_verify(region, "fresh region (1 free block)");
}

static void test_heap_insert_init(void) {
    printf("heap_insert_init (0x042e2d)\n");
    heap_arena_init();
    uint32_t A = heap_arena_alloc(0x100);  /* 0 */
    uint32_t B = heap_arena_alloc(0x100);  /* 0x100 */
    uint32_t C = heap_arena_alloc(0x100);  /* 0x200 */
    hw(B)[0] = 0x100; heap_insert_init(B);              /* first -> head */
    check(g_heap_head == B, "first insert is head");
    check(hw(B + 0x100)[0] == 0xFFFFFFFFu, "end marker at region+total"); /* before adjacent region clobbers it */
    hw(A)[0] = 0x100; heap_insert_init(A);              /* A < B -> head */
    check(g_heap_head == A, "smaller offset becomes head");
    hw(C)[0] = 0x100; heap_insert_init(C);              /* C > B -> tail */
    check(hw(A)[2] == B && hw(B)[2] == C && hw(C)[2] == 0, "chain sorted A->B->C (NULL tail)");
    check(hw(B)[1] == A && hw(C)[1] == B && hw(A)[1] == 0, "prev links NULL<-A<-B<-C");
    check(hw(B + 0x2c)[0] == 0x100 - 0x2c, "block0.size = total-0x2c");
    check(hw(B)[3] == B + 0x20 && hw(B)[8] == 0, "root = sentinel, sentinel.size = 0");
}

static void test_heap_unlink(void) {
    printf("heap_unlink (0x042c8d)\n");
    heap_arena_init();
    uint32_t A = heap_arena_alloc(0x100);  /* 0 */
    uint32_t B = heap_arena_alloc(0x100);  /* 0x100 */
    uint32_t C = heap_arena_alloc(0x100);  /* 0x200 */
    hw(A)[0] = hw(B)[0] = hw(C)[0] = 0x100;
    heap_insert_init(A); heap_insert_init(B); heap_insert_init(C);
    g_heap_cursor = B;
    heap_unlink(B);   /* remove middle node */
    check(g_heap_cursor == C, "cursor advances past removed node");
    check(g_heap_head == A,   "head unchanged when removing non-head");
    check(hw(A)[2] == C,      "prev->next bypasses removed node");
    check(hw(C)[1] == A,      "next->prev updated (full double unlink, disasm-verified)");
    heap_unlink(A);   /* remove head */
    check(g_heap_head == C, "head advances past removed head");
    check(hw(C)[1] == 0, "C.prev becomes NULL after head removed");
    heap_unlink(C);   /* remove last */
    check(g_heap_head == 0, "head becomes 0 after removing last");
}

/* Mirror the original's alignment exactly (disasm 0x42ae4-42af7):
 *   t = size + 7; t = (t & 0xFFFFFF00) | ((t & 0xFF) & 0xFC); return max(t, 0xc).
 * The 0xfc mask is on the LOW byte only, so high bytes are preserved. */
static uint32_t orig_aligned(uint32_t size) {
    uint32_t t = size + 7;
    t = (t & 0xFFFFFF00u) | ((t & 0xFFu) & 0xFCu);
    return (t >= 0xc) ? t : 0xc;
}

static void test_heap_alloc_free(void) {
    printf("heap_alloc + heap_free (0x042ada / 0x042b82)\n");
    /* alignment unit checks (low-byte 0xfc mask) */
    check(orig_aligned(0x40) == 0x44, "align(0x40) = 0x44 (low-byte mask)");
    check(orig_aligned(0x100) == 0x104, "align(0x100) = 0x104 (0x07&0xfc=0x04)");
    check(orig_aligned(0x101) == 0x108, "align(0x101) = 0x108");
    check(orig_aligned(0xff8) == 0xffc, "align(0xff8) = 0xffc (no-split candidate)");
    check(orig_aligned(0x1) == 0xc && orig_aligned(0x8) == 0xc, "min align = 0xc");
    heap_arena_init();
    g_heap_seg_limit = 0;
    uint32_t data   = heap_acquire_region(0x1000);
    uint32_t region = data - 0x30;
    heap_verify(region, "start");

    uint32_t p1 = heap_alloc(region, 0x100);
    uint32_t p2 = heap_alloc(region, 0x100);
    uint32_t p3 = heap_alloc(region, 0x100);
    check(p1 && p2 && p3, "3 allocations succeed");
    check(p2 - p1 == 0x104 && p3 - p2 == 0x104, "0x100 allocs are 0x104-strided (align)");
    heap_verify(region, "after 3 allocs");

    heap_free(region, p2);
    heap_verify(region, "after freeing p2");
    uint32_t p4 = heap_alloc(region, 0x100);
    check(p4 == p2, "freed slot reused for same size");
    heap_verify(region, "after reuse");

    heap_free(region, p1);
    heap_free(region, p4);
    heap_free(region, p3);
    heap_verify(region, "after freeing all (fully coalesced)");
    check(hw(region + 0x2c)[0] == 0x1000 && !(hw(region + 0x2c)[0] & 1),
          "coalesced back to a single 0x1000 free block");
    check(hw(region)[7] == 1, "free_count == 1 after full coalesce");

    /* two adjacent 0x40 allocs (0x44 aligned stride); free both -> coalesce */
    uint32_t q1 = heap_alloc(region, 0x40);
    uint32_t q2 = heap_alloc(region, 0x40);
    check(q2 - q1 == orig_aligned(0x40), "q1/q2 adjacent with 0x44 aligned stride");
    heap_free(region, q2);
    heap_free(region, q1);
    heap_verify(region, "after forward coalesce of q1,q2");

    /* no-split path: aligned leaves remainder < 0xc, so the whole free block is
     * taken and its size is preserved (not shrunk to the aligned size). */
    uint32_t big = heap_alloc(region, 0xff8);  /* aligned 0xffc; rem 0x4 < 0xc */
    check(big != 0, "no-split allocation succeeds");
    check((hw(region + 0x2c)[0] & 1) && (hw(region + 0x2c)[0] & ~1u) == 0x1000,
          "no-split keeps whole free-block size (0x1000) marked allocated");
    heap_verify(region, "after no-split whole-block alloc");
    heap_free(region, big);
    heap_verify(region, "after freeing no-split alloc");
}

static void test_heap_top_check(void) {
    printf("heap_top_check (0x042ea1)\n");
    heap_arena_init();
    check(heap_top_check() == 0, "no regions -> 0");
    uint32_t data   = heap_acquire_region(0x1000);
    uint32_t region = data - 0x30;
    uint32_t last = hw(region + 0x20)[1];  /* sentinel.prev = last free block */
    uint32_t size = hw(last)[0];
    uint32_t end  = last + size + 4;
    g_heap_seg_limit = end;
    check(heap_top_check() == size, "top block at limit -> its size");
    g_heap_seg_limit = end + 4;
    check(heap_top_check() == 0, "top block past limit -> 0");
    g_heap_cfg_b2 = 4; g_heap_cfg_b4 = 0; g_ds_segment = 0;  /* recompute path */
    check(heap_top_check() == 0, "recompute: end != (0+1)*0x10000 -> 0");
}

static void test_heap_stress(void) {
    printf("heap stress (2000 random alloc/free + invariants)\n");
    heap_arena_init();
    uint32_t data   = heap_acquire_region(0x4000);  /* 16 KiB */
    uint32_t region = data - 0x30;
    uint32_t live[256]; int nlive = 0;
    unsigned int seed = 12345;
    for (int iter = 0; iter < 2000; iter++) {
        seed = seed * 1103515245u + 12345u;
        if (nlive < 48 && (seed & 3) != 0) {
            uint32_t sz = 4 + ((seed >> 8) & 0xFF);
            uint32_t p  = heap_alloc(region, sz);
            if (p && nlive < 256) live[nlive++] = p;
        } else if (nlive > 0) {
            seed = seed * 1103515245u + 12345u;
            int idx = (int)((seed >> 12) % (unsigned)nlive);
            heap_free(region, live[idx]);
            live[idx] = live[--nlive];
        }
        if (heap_verify(region, NULL)) { check(0, "invariant broken during stress"); return; }
    }
    check(1, "2000 random ops, invariants hold throughout");
}

/* ---- L0 batch A: real-mode/DOS stubs + command-line parser ---- */
static void test_rm_stubs(void) {
    printf("batch A: DPMI/DOS real-mode stubs (no-op in flat model)\n");
    uint32_t ctx[8] = {1,2,3,4,5,6,7,8};
    rm_switch_a(0, ctx);                 /* 0x04335c */
    rm_switch_b(0x4e, ctx);              /* 0x0433ce */
    fpu_state_save((uintptr_t)&ctx[0]);  /* 0x043979 */
    dos_install_timer_int();             /* 0x043958 */
    check(1, "real-mode/DOS trampoline stubs are safe no-ops");
}

static void test_cmdline_parse(void) {
    printf("batch A: cmdline_parse (0x043ca0)\n");
    struct cmd_ctx c;
    /* "42N" -> number 42, flags = 1|8 = 9, cursor past 'N' */
    c.flags = 0; c.num = 0;
    const char *p = cmdline_parse("42N", &c);
    check(c.num == 42 && c.flags == 9, "'42N' -> num=42 flags=0x9");
    check(*p == 0, "'42N' cursor at NUL");
    /* "*42" -> leading '*' clears bit0 -> flags=0, num=42 */
    c.flags = 0; c.num = 0;
    cmdline_parse("*42", &c);
    check(c.num == 42 && c.flags == 0, "'*42' -> num=42 flags=0");
    /* "L" -> flags = 1|0x40 = 0x41, num = 0xffffffff (absent) */
    c.flags = 0; c.num = 0;
    cmdline_parse("L", &c);
    check(c.num == 0xffffffff && c.flags == 0x41, "'L' -> num=absent flags=0x41");
    /* "hwl" -> 1|0x10|0x20|0x20 = 0x31 (h then w then l; l/w share 0x20) */
    c.flags = 0; c.num = 0;
    cmdline_parse("h", &c);
    check(c.flags == 0x11, "'h' -> flags=0x11");
    /* init flags masked: (0x15 & 3)|1 = 1|1 = 1 */
    c.flags = 0x15; c.num = 0;
    cmdline_parse("x", &c);   /* 'x' is not a switch; not a digit either */
    check((c.flags & 3) == 1, "init flags masked to low 2 bits | 1");
}

/* mock char source for cmdline_count */
static const uint8_t g_mockseq[] = { 'a','b','\t','\n','x','c', 0x00 };
static int g_mocki, g_emitted;
static uint16_t mock_next(struct cmd_ctx *ctx) { (void)ctx; return g_mockseq[g_mocki++]; }
static void mock_emit(uint16_t ch, struct cmd_ctx *ctx) { (void)ctx; g_emitted = (int)ch; }

static void test_cmdline_count(void) {
    printf("batch A: cmdline_count (0x043d79)\n");
    struct cmd_ctx c = { .next = mock_next, .emit = mock_emit, .flags = 0 };
    /* start at 'a'(idx0): next-class of 'a' is class['b']=152, &0x2=0 -> count 0, last 'a' */
    g_mocki = 0; g_emitted = -1;
    uint32_t n = cmdline_count(&c);
    check(n == 0, "'a' (next 'b' not ws) -> count 0");
    check(g_emitted == 'a', "emit called with last char 'a' (flags&2 clear)");
    /* start at '\t'(idx2): '\t'->class['\n']=3 (ws) count1; '\n'->class[0x0b]=3 (ws) count2;
     * 'x'(idx4)->class['y']=136 &0x2=0 -> stop. count=2, last='x' */
    g_mocki = 2; g_emitted = -1;
    n = cmdline_count(&c);
    check(n == 2, "'\\t','\\n' -> 2 whitespace-class chars");
    check(g_emitted == 'x', "last char emitted is 'x'");
}

/* ---- L0 batch B: emit + dispatch ---- */
static const uint8_t g_esrc[] = {'A','B','C','D'};
static int g_esrci;
static uint16_t esrc_next(struct emit_ctx *ctx) { (void)ctx; return g_esrc[g_esrci++]; }

static void test_emit_chars(void) {
    printf("batch B: emit_chars (0x043dae)\n");
    uint8_t buf[16] = {0};
    uintptr_t stream[1] = { (uintptr_t)buf }; uintptr_t *cur = stream;
    struct emit_ctx c = { .next = esrc_next, .count = 3, .flags = 0x1 };
    g_esrci = 0;
    check(emit_chars(&c, &cur) == 3, "byte mode: wrote 3");
    check(buf[0]=='A'&&buf[1]=='B'&&buf[2]=='C', "byte mode: ABC");
    check(cur == stream + 1, "near dest: pos advanced 1 entry");
    uint16_t wbuf[8] = {0};
    uintptr_t wstream[1] = { (uintptr_t)wbuf }; uintptr_t *wcur = wstream;
    struct emit_ctx c2 = { .next = esrc_next, .count = 2, .flags = 0x21 };
    g_esrci = 0;
    check(emit_chars(&c2, &wcur) == 2, "word mode: wrote 2");
    check(wbuf[0]==0x0041 && wbuf[1]==0x0042, "word mode: writes full 16-bit values");
    uint8_t pbuf[8] = {0x55,0x55,0x55};
    uintptr_t pstream[1] = { (uintptr_t)pbuf }; uintptr_t *pcur = pstream;
    struct emit_ctx c3 = { .next = esrc_next, .count = 3, .flags = 0x3 };
    g_esrci = 0;
    check(emit_chars(&c3, &pcur) == 0, "peek mode: wrote 0");
    check(pbuf[0]==0x55 && g_esrci==1, "peek mode: buffer untouched, read 1 char");
}

static void test_emit_value(void) {
    printf("batch B: emit_value (0x043f22)\n");
    uint32_t dbuf[4] = {0};
    uintptr_t dstream[1] = { (uintptr_t)dbuf }; uintptr_t *dcur = dstream;
    struct emit_ctx c = { .flags = 0x1 };
    emit_value(&c, &dcur, 0x11223344);
    check(dbuf[0] == 0x11223344, "dword write");
    uint32_t wbuf[4] = {0};
    uintptr_t wstream[1] = { (uintptr_t)wbuf }; uintptr_t *wcur = wstream;
    struct emit_ctx c2 = { .flags = 0x11 };   /* 0x10 = word */
    emit_value(&c2, &wcur, 0xaabbccdd);
    check(((uint16_t *)wbuf)[0] == 0xccdd, "word write (low word)");
}

static void test_emit_step(void) {
    printf("batch B: emit_step (0x0445a6)\n");
    struct emit_ctx c = { .next = esrc_next, .count = 5, .flags = 0 };
    g_esrci = 0;
    check(emit_step(&c) == 'A' && c.count == 4, "count 5->4, returns next()='A'");
    struct emit_ctx c2 = { .next = esrc_next, .count = 0, .flags = 0 };
    g_esrci = 0;
    check(emit_step(&c2) == 0xffffffff && g_esrci == 0, "count 0 -> 0xffffffff, no callback");
    struct emit_ctx c3 = { .next = esrc_next, .count = 3, .flags = 0x2 };
    g_esrci = 0;
    check(emit_step(&c3) == 0xffffffff && g_esrci == 1, "flags&2 -> 0xffffffff (callback ran)");
}

static int g_order[16], g_order_n;
static void hfn0(void){ g_order[g_order_n++]=0; }
static void hfn1(void){ g_order[g_order_n++]=1; }
static void hfn2(void){ g_order[g_order_n++]=2; }
static void hfn8(void){ g_order[g_order_n++]=8; }
static void hfn9(void){ g_order[g_order_n++]=9; }

static void test_dispatch(void) {
    printf("batch B: dispatch_a/b (0x044618/0x044663)\n");
    struct ev_entry save[10]; memcpy(save, g_evtab, sizeof save);
    /* dispatch_a (entries 0..7): keys 0x10,0x05,0x20; rest done. key=0x15 fires
     * 0x05 (idx1) then 0x10 (idx0); 0x20 (idx2) never fires (key > threshold). */
    g_order_n = 0;
    g_evtab[0] = (struct ev_entry){0,0x10,hfn0};
    g_evtab[1] = (struct ev_entry){0,0x05,hfn1};
    g_evtab[2] = (struct ev_entry){0,0x20,hfn2};
    for (int i = 3; i < 10; i++) g_evtab[i] = (struct ev_entry){2, 0, NULL};
    dispatch_a(0x15);
    check(g_order_n == 2 && g_order[0]==1 && g_order[1]==0, "dispatch_a(0x15): order 0x05 then 0x10");
    check(g_evtab[0].flag==2 && g_evtab[1].flag==2 && g_evtab[2].flag==0, "dispatch_a: fired marked done, 0x20 left");
    /* dispatch_b (entries 8..9): keys 0x05(hfn8),0x10(hfn9). eax=0x10, edx=0x20 ->
     * matches key<=AL rolling; fire gated by al<=dh. */
    g_order_n = 0;
    for (int i = 0; i < 8; i++) g_evtab[i] = (struct ev_entry){2, 0, NULL};
    g_evtab[8] = (struct ev_entry){0,0x05,hfn8};
    g_evtab[9] = (struct ev_entry){0,0x10,hfn9};
    dispatch_b(0x05, 0x20);
    check(g_order_n == 2, "dispatch_b: both entries fire");
    check(g_order[0]==9 && g_order[1]==8, "dispatch_b: order idx9 (key 0x10) then idx8 (key 0x05)");
    memcpy(g_evtab, save, sizeof g_evtab);
}

int main(void) {
    printf("== batch 1 (L0 primitives) ==\n");
    test_put_px();
    test_cpy_back();
    test_cpy_dw();
    test_blt_rows();
    test_hline_vline_rect();
    test_board();
    test_abs();
    test_canary();
    printf("== batch 2 (L0 seg/mem/str/fpu) ==\n");
    test_seg_save();
    test_mem_copy();
    test_rng_state_ptr();
    test_itoa32();
    test_str_len();
    test_fpu_rndint();
    printf("== batch 3 (L0 I/O + interrupt-vector) ==\n");
    test_outb();
    test_irq_nops();
    test_isr();
    printf("== batch 4 (L0 I/O read / str / reader / fmt) ==\n");
    test_inb();
    test_cb_noop();
    test_str_icmp();
    test_res_consume();
    test_digit_chr();
    test_fmt_emit();
    test_get_key();
    test_mem_cmp();
    test_cb_noop2();
    test_get_key_echo();
    test_heap_grow();
    test_memsetw();
    printf("== batch 7-8 (two-level heap) ==\n");
    test_heap_arena();
    test_heap_insert_init();
    test_heap_unlink();
    test_heap_alloc_free();
    test_heap_top_check();
    test_heap_stress();
    printf("== batch 9 (L0 real-mode/DOS + cmdline parser) ==\n");
    test_rm_stubs();
    test_cmdline_parse();
    test_cmdline_count();
    printf("== batch B (L0 emit + dispatch) ==\n");
    test_emit_chars();
    test_emit_value();
    test_emit_step();
    test_dispatch();
    printf(failures ? "== FAIL (%d) ==\n" : "== ALL PASS ==\n", failures);
    return failures != 0;
}
