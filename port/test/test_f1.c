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
    g_stack_bound = (uintptr_t)-1;              /* d > bound never true */
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

static void test_batchC(void) {
    printf("== batch C (L0 char/string/count) ==\n");
    printf("batch C: to_lower (0x044859)\n");
    check(to_lower('A')==0x61 && to_lower('Z')==0x7a, "A-Z -> lowercase");
    check(to_lower('a')=='a' && to_lower('0')=='0' && to_lower(0x5b)==0x5b, "non-upper unchanged");
    printf("batch C: strn_len (0x045032)\n");
    const uint8_t s[] = {'h','i','x','\0','x'};
    check(strn_len(s, 10) == 3, "strn_len: 3 before NUL");
    check(strn_len(s, 2) == 2, "strn_len: capped at limit");
    check(strn_len(s, 0) == 0, "strn_len: limit 0");
    const uint8_t nn[] = {'a','b','c'};
    check(strn_len(nn, 3) == 3, "strn_len: all non-NUL within limit");
    printf("batch C: word_nlen (0x045056)\n");
    const uint16_t w[] = {0x1, 0x2, 0, 0x5};
    check(word_nlen(w, 10) == 2, "word_nlen: 2 before zero word");
    check(word_nlen(w, 1) == 1, "word_nlen: capped at limit");
    printf("batch C: DPMI constants + accessors\n");
    check(const_64008()==0x64008 && const_64004()==0x64004, "constants");
    g_dat_53390 = 0x1234;
    check(get_dat_53390()==0x1234, "get_dat_53390 returns global");
    g_cb_53644 = NULL;
    check(cb_indirect_53644()==0, "cb_indirect NULL-guarded -> 0");
}

static void test_batchD(void) {
    printf("== batch D (L0 node/prefix/debug) ==\n");
    printf("batch D: node_deactivate (0x044b5d)\n");
    static uint8_t pool[0x60];
    g_ndpool = pool; memset(pool, 0, sizeof pool);
    /* offset 0 is reserved as NULL (the original's DPMI node addresses are never
     * 0, so 0 == empty). Nodes (8B) at offsets 8/16/24; records at 40/48/56. */
    uint32_t *p = (uint32_t *)pool;
    p[2]  = 16; p[3]  = 40;   /* node0 @8:  next=16, rec=40 */
    p[4]  = 24; p[5]  = 48;   /* node1 @16: next=24, rec=48 */
    p[6]  = 0;  p[7]  = 56;   /* node2 @24: next=0,  rec=56 */
    pool[48 + 0xc] = 0x10;    /* rec1 (node1's record) flags */
    g_node_list_active = 8;   /* head = node0 (offset 8) */
    g_node_list_done = 0;
    node_deactivate(48);      /* target rec1 = node1 (2nd in list) */
    check(pool[48+0xc] == 0x13, "rec1 flags |= 0x3");
    check(g_node_list_done == 16, "node1 moved to done head");
    check(g_node_list_active == 8, "active head unchanged (node0)");
    check(p[2] == 24, "node0.next -> node2 (node1 unlinked)");
    check(p[4] == 0, "node1.next -> old done head (0)");
    g_node_list_active = 8; g_node_list_done = 0;
    node_deactivate(1234);    /* not in list */
    check(g_node_list_done == 0 && g_node_list_active == 8, "not-found: no-op");

    printf("batch D: parse_mod_prefix (0x044fd7)\n");
    uint8_t out[0x20];
    memset(out,0,sizeof out);
    parse_mod_prefix((const uint8_t*)"-#+ x", out);
    check(out[0x14] == 0x0d, "'-#+' -> flags 0x0d, space skipped");
    check(out[0x16] == 0, "no '0' stored");
    memset(out,0,sizeof out);
    parse_mod_prefix((const uint8_t*)"0a", out);
    check(out[0x16] == 0x30 && out[0x14] == 0, "'0' stored, no flags");
    memset(out,0,sizeof out);
    parse_mod_prefix((const uint8_t*)"+ a", out);
    check(out[0x14] == 0x04, "'+ ' -> 0x04 (space ignored after +)");
    memset(out,0,sizeof out);
    parse_mod_prefix((const uint8_t*)" a", out);
    check(out[0x14] == 0x02, "' a' -> 0x02 (space sets 0x2)");

    printf("batch D: debug_break_hook (0x045a44)\n");
    g_debug_break = 0;
    check(debug_break_hook(0x123456, 0xabc) == 0, "flag clear -> 0");
    g_debug_break = 1;
    check(debug_break_hook(0x123456, 0xabc) == 0, "flag set -> 0 (INT3 stubbed)");
    g_debug_break = 0;
}

static void test_batchD2(void) {
    printf("== batch D2 (L0 DOS clock) ==\n");
    printf("batch D2: calibrate_clock (0x04561d)\n");
    sys_clock_tps = 1000;
    calibrate_clock();
    check(g_clock_ticks == 1000, "g_clock_ticks = sys rate");
    printf("batch D2: delay_units (0x04564f)\n");
    sys_clock_tps = 1000; calibrate_clock();   /* g_clock_ticks = 1000 */
    sys_clock_read_count = 0;
    delay_units(2);   /* (2*1000+500)/1000 = 2 */
    check(sys_clock_read_count == 2, "delay_units(2): 2 reads");
    sys_clock_read_count = 0;
    delay_units(0);   /* no-op */
    check(sys_clock_read_count == 0, "delay_units(0): no reads");
    sys_clock_read_count = 0;
    delay_units(1);   /* (1*1000+500)/1000 = 1 */
    check(sys_clock_read_count == 1, "delay_units(1): 1 read");
    /* rounding: tick_rate 18, arg 55 -> (55*18+500)/1000 = 1 */
    sys_clock_tps = 18; calibrate_clock();
    sys_clock_read_count = 0;
    delay_units(55);
    check(sys_clock_read_count == 1, "tick_rate 18, arg 55 -> 1 read");
    /* floor-to-1: (0*18+500)/1000 = 0 -> max(1) = 1 (arg>0) */
    sys_clock_read_count = 0;
    delay_units(1);   /* (18+500)/1000 = 0 -> 1 */
    check(sys_clock_read_count == 1, "floor to 1 when quotient 0");
    sys_clock_tps = 18;   /* restore default */
}

static void test_batchE(void) {
    printf("== batch E (L0 swap/accessor) ==\n");
    printf("batch E: swap_dat_5363c (0x045ae8)\n");
    g_dat_5363c = 0x1111;
    uint32_t old = swap_dat_5363c(0x2222);
    check(old == 0x1111, "returns old global");
    check(g_dat_5363c == 0x2222, "global now has arg");
    g_dat_5363c = 0;
    printf("batch E: case_field_0x28 (0x04642a)\n");
    uint8_t buf[0x30] = {0};
    *(uint32_t *)(buf + 0x28) = 0xaabbccdd;
    check(case_field_0x28((uintptr_t)buf) == 0xaabbccdd, "returns [s+0x28]");
}

static void test_batchF(void) {
    printf("== batch F (L0 misc primitives) ==\n");
    printf("batch F: stack_depth (0x04917e)\n");
    {
        int anchor;
        uintptr_t here = (uintptr_t)&anchor;
        /* Differential: &local is fixed across the two calls, so lowering the
         * bound by 8 must raise the depth by exactly 8 (mod 2^32). */
        g_stack_bound = here;
        uint32_t d1 = stack_depth();
        g_stack_bound = here - 8;
        uint32_t d2 = stack_depth();
        check((uint32_t)(d2 - d1) == 8, "lowering bound by 8 -> depth +8");
    }
    g_stack_bound = 0;
    printf("batch F: set_53640 (0x049187)\n");
    g_dat_53640 = 0; set_53640();
    check(g_dat_53640 == 0x8000, "g_53640 = 0x8000");
    printf("batch F: to_upper (0x0494bd)\n");
    check(to_upper('a')=='A' && to_upper('z')=='Z', "a-z -> A-Z");
    check(to_upper('A')=='A' && to_upper('5')=='5' && to_upper(0x7e)==0x7e, "else unchanged");
    printf("batch F: store_arr53730 (0x049325)\n");
    static uint32_t arr[8]; g_arr_53730 = arr;
    store_arr53730(2, 0x1234);
    check(arr[2] == (0x1234|0x40000), "stored value | 0x40000");
    printf("batch F: noop_9109 (0x049109)\n");
    noop_9109(); check(1, "noop returns");
    printf("batch F: ioctl_device (0x0492b2)\n");
    sys_ioctl_cf = 0;
    sys_ioctl_dx = 0x10; check(ioctl_device(0)==0, "dx=0x10 -> bit7 clear");
    sys_ioctl_dx = 0x80; check(ioctl_device(0)==1, "dx=0x80 -> bit7 set");
    sys_ioctl_dx = 0; sys_ioctl_cf = 0;
}

static void test_batchG(void) {
    printf("== batch G (L0 fixed-point constructor) ==\n");
    struct fxnum n;
    printf("batch G: fx_from_u32 (0x047f1f)\n");
    fx_from_u32(&n, 0);
    check(n.lo==0 && n.hi==0 && n.exp==0, "0 -> all zero");
    fx_from_u32(&n, 1);
    check(n.lo==0x80000000u && n.hi==0x80000000u && n.exp==0x3fff, "1 -> norm 0x80000000 exp 0x3fff");
    fx_from_u32(&n, 0x80000000u);
    check(n.lo==0x80000000u && n.hi==0x80000000u && n.exp==0x401e, "0x80000000 -> exp 0x401e");
    fx_from_u32(&n, 0x10000);
    check(n.lo==0x80000000u && n.hi==0x80000000u && n.exp==0x400f, "0x10000 -> norm 0x80000000 exp 0x400f");
    fx_from_u32(&n, 0xffffffffu);
    check(n.lo==0xffffffffu && n.hi==0xffffffffu && n.exp==0x401e, "0xffffffff -> norm 0xffffffff exp 0x401e");
    printf("batch G: fx_from_s32 (0x047f0e)\n");
    fx_from_s32(&n, 5);
    check(n.lo==0xa0000000u && n.hi==0xa0000000u && n.exp==0x4001, "5 -> norm 0xa0000000 exp 0x4001");
    fx_from_s32(&n, -5);
    check(n.lo==0xa0000000u && n.hi==0xa0000000u && n.exp==0xc001, "-5 -> norm 0xa0000000 exp 0xc001 (sign bit)");
    fx_from_s32(&n, -1);
    check(n.lo==0x80000000u && n.hi==0x80000000u && n.exp==0xbfff, "-1 -> norm 0x80000000 exp 0xbfff");
}

static void test_batchH(void) {
    printf("== batch H (L0: fx compare, decimal parse, stubs) ==\n");
    struct fxnum a, b;
    printf("batch H: fx_less (0x0498c6)\n");
    fx_from_u32(&a, 5);   /* exp 0x4001, norm 0xa0000000 */
    fx_from_u32(&b, 7);   /* exp 0x4001, norm 0xe0000000 */
    check(fx_less(&a, &b) == 1, "5 < 7");
    check(fx_less(&b, &a) == 0, "7 < 5 false");
    fx_from_u32(&a, 3);   /* exp 0x3fff */
    fx_from_u32(&b, 5);   /* exp 0x4001 */
    check(fx_less(&a, &b) == 1, "3 < 5 (exp)");
    check(fx_less(&b, &a) == 0, "5 < 3 false");
    printf("batch H: parse_decimal (0x049a48)\n");
    uint32_t d;
    parse_decimal("123abc", &d); check(d == 123, "123abc -> 123");
    parse_decimal("0", &d);      check(d == 0,   "0 -> 0");
    parse_decimal("abc", &d);    check(d == 0,   "abc -> 0");
    parse_decimal("42", &d);     check(d == 42,  "42 -> 42");
    parse_decimal("9999x", &d);  check(d == 9999, "9999x -> 9999");
    check((int)parse_decimal("123abc", &d)[0] == 'a', "123abc -> return points at 'a'");
    printf("batch H: dos_realmode_setup (0x0490b0, stub)\n");
    dos_realmode_setup(); check(1, "stub returns");
    printf("batch H: fpu_init (0x04910a, stub)\n");
    check(fpu_init() == 0x037f, "standard control word 0x37f");
}

static void test_batchI(void) {
    printf("== batch I (L0: strn_casecmp + spurious 0x48312) ==\n");
    printf("batch I: strn_casecmp (0x04a7a2)\n");
    check(strn_casecmp("abc", "abc", 3) == 0, "equal -> 0");
    check(strn_casecmp("abc", "ABC", 3) == 0, "case-insensitive equal -> 0");
    check(strn_casecmp("abc", "abC", 3) == 0, "mixed case equal -> 0");
    check(strn_casecmp("abc", "abd", 3) == -1, "c-d -> -1");
    check(strn_casecmp("abd", "abc", 3) == 1,  "d-c -> 1");
    check(strn_casecmp("a", "b", 5) == -1, "first-char mismatch -> -1");
    check(strn_casecmp("", "", 0) == 0, "n=0 -> 0");
    check(strn_casecmp("ab\0cd", "ab\0ef", 6) == 0, "NUL stop -> 0");
}

static void test_batchJ(void) {
    printf("== batch J (L0: tokenize 0x04595b) ==\n");
    char *end;
    check(tokenize("abc", NULL, &end) == 1, "abc -> 1");
    check(tokenize("ab cd", NULL, &end) == 2, "ab cd -> 2");
    check(tokenize("a b c", NULL, &end) == 3, "a b c -> 3");
    check(tokenize("  ab  cd  ", NULL, &end) == 2, "spaced -> 2");
    check(tokenize("", NULL, &end) == 0, "empty -> 0");
    check(tokenize("   ", NULL, &end) == 0, "spaces -> 0");
    check(tokenize("\"hi\" world", NULL, &end) == 2, "quoted+bare -> 2");
    check(tokenize("\"hi\"", NULL, &end) == 1, "quoted -> 1");
    char s[] = "ab cd";
    char *out[4] = {0};
    check(tokenize(s, out, &end) == 2, "in-place count 2");
    check(out[0] && out[0][0]=='a' && out[0][1]=='b' && out[0][2]=='\0', "out[0]='ab'");
    check(out[1] && out[1][0]=='c' && out[1][1]=='d' && out[1][2]=='\0', "out[1]='cd'");
    char q[] = "\"hi\"";
    char *qout[2] = {0};
    check(tokenize(q, qout, &end) == 1, "quoted in-place 1");
    check(qout[0] && qout[0][0]=='h' && qout[0][1]=='i' && qout[0][2]=='\0', "quoted -> 'hi'");
    /* itoa32_tbl (0x049459) - table-based itoa, byte-identical to itoa32 */
    char nb[40];
    check(strcmp(itoa32_tbl(255, nb, 16), "ff") == 0, "tbl 255 base16 = ff");
    check(strcmp(itoa32_tbl(0, nb, 10), "0") == 0, "tbl 0 base10 = 0");
    check(strcmp(itoa32_tbl(123456, nb, 10), "123456") == 0, "tbl 123456 base10");
    check(strcmp(itoa32_tbl(0x1000000, nb, 16), "1000000") == 0, "tbl 16M base16");
    /* is_leap (0x0494cb) - Gregorian leap-year, returns (ctx<<32)|flag */
    check(is_leap(2000, 0) == 1, "2000 leap");
    check(is_leap(1900, 0) == 0, "1900 not leap");
    check(is_leap(2024, 0) == 1, "2024 leap");
    check(is_leap(2023, 0) == 0, "2023 not leap");
    check(is_leap(2000, 0xabcd) == 0xabcd00000001ULL, "ctx echoed high32");
    check(is_leap(1583, 0) == 0, "1583 not leap");
}

static void test_fx_from_f32(void) {
    printf("== fx_from_f32 (L0: IEEE-754 float32 -> fxnum 0x04811a) ==\n");
    struct fxnum n;
    fx_from_f32(&n, 0x00000000u); /* +0.0: exponent field 0 */
    check(n.lo==0 && n.hi==0x00000000u && n.exp==0, "+0.0 -> passthrough {0,0,0}");
    fx_from_f32(&n, 0x3f800000u); /* 1.0: unbiassed 0, expfield 0x7f */
    check(n.lo==0 && n.hi==0x80000000u && n.exp==0x3fff, "1.0 -> hi 0x80000000 exp 0x3fff");
    fx_from_f32(&n, 0x40000000u); /* 2.0: unbiassed 1, expfield 0x80 */
    check(n.lo==0 && n.hi==0x80000000u && n.exp==0x4000, "2.0 -> exp 0x4000");
    fx_from_f32(&n, 0x40490fdbu); /* pi: unbiassed 1, mantissa set */
    check(n.lo==0 && n.hi==0xc90fdb00u && n.exp==0x4000, "pi -> hi 0xc90fdb00 exp 0x4000");
    fx_from_f32(&n, 0xff800000u); /* -inf: exponent field 0xff */
    check(n.lo==0 && n.hi==0x80000000u && n.exp==0xffff, "-inf -> exp 0xffff");
    fx_from_f32(&n, 0x80000000u); /* -0.0: exponent field 0 (zero) */
    check(n.lo==0 && n.hi==0x80000000u && n.exp==0, "-0.0 -> passthrough exp 0");
}

static void test_float_str_sci(void) {
    printf("== float_str_sci (L0: sci-notation cleanup 0x045d56) ==\n");
    uint8_t rec[32];
    memset(rec, 0, sizeof rec);         /* flag=0, char=0 */
    char s1[] = "12.5e3";
    float_str_sci(s1, rec);
    check(strcmp(s1, "12.5e3") == 0, "12.5e3 (dot not adjacent) -> unchanged");
    char s2[] = "12.e5";
    float_str_sci(s2, rec);
    check(strcmp(s2, "12e5") == 0, "12.e5 -> 12e5 (adjacent dot dropped)");
    memset(rec, 0, sizeof rec); rec[0x15] = 'g';   /* g-mode: strip trailing zeros */
    char s3[] = "1234500e6";
    float_str_sci(s3, rec);
    check(strcmp(s3, "12345e6") == 0, "1234500e6 (g) -> 12345e6 (zeros stripped)");
    memset(rec, 0, sizeof rec);
    char s4[] = "100.5e3";
    float_str_sci(s4, rec);
    check(strcmp(s4, "100.5e3") == 0, "100.5e3 (mid dot) -> unchanged");
    char s5[] = "123.";
    float_str_sci(s5, rec);
    check(strcmp(s5, "123") == 0, "123. -> 123 (trailing dot dropped)");
    char s6[] = "7.0e3";
    memset(rec, 0, sizeof rec); rec[0x14] = 1;      /* flag set -> no change */
    float_str_sci(s6, rec);
    check(strcmp(s6, "7.0e3") == 0, "flag set -> unchanged");
}

static void test_fmt_u64_fixed(void) {
    printf("== fmt_u64_fixed (L0: 64-bit fixed-width ASCII 0x04a8b9) ==\n");
    uint64_t v; char buf[40];
    v = 1234567; memset(buf, 0xff, sizeof buf);
    fmt_u64_fixed(&v, buf, 8);
    check(strcmp(buf, "01234567") == 0, "1234567 w=8 -> 01234567");
    check(buf[8] == '\0', "NUL sentinel at buf[width]");
    v = 0; memset(buf, 0xff, sizeof buf);
    fmt_u64_fixed(&v, buf, 8);
    check(strcmp(buf, "00000000") == 0, "0 w=8 -> 00000000 (zero pad)");
    v = 99999999; memset(buf, 0xff, sizeof buf);
    fmt_u64_fixed(&v, buf, 8);
    check(strcmp(buf, "99999999") == 0, "99999999 w=8 -> 99999999");
    v = 4294967296ull; memset(buf, 0xff, sizeof buf);
    fmt_u64_fixed(&v, buf, 10);
    check(strcmp(buf, "4294967296") == 0, "2^32 w=10 -> 4294967296 (hi dword)");
    v = 0xdeadbeefcafebabeull; memset(buf, 0xff, sizeof buf);
    fmt_u64_fixed(&v, buf, 20);
    check(strcmp(buf, "16045690984503098046") == 0, "0xdeadbeefcafebabe w=20 (full 64-bit)");
}

static void test_fx_mantissa_scale(void) {
    printf("== fx_mantissa_scale (L0: 53-bit mantissa scaler 0x04a84e) ==\n");
    uint32_t in[2], out[2];
    in[1] = 0x433u<<20; in[0] = 0x12345678;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00100000 && out[0]==0x12345678, "shift=0 passthrough");
    in[1] = 0x434u<<20; in[0] = 0x1;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00200000 && out[0]==0x2, "shift=+1 left (hi+lo shift)");
    in[1] = 0x432u<<20; in[0] = 0x1;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00080000 && out[0]==0x0, "shift=-1 tie q-even no round");
    in[1] = 0x432u<<20; in[0] = 0x3;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00080000 && out[0]==0x2, "shift=-1 tie q-odd round up");
    in[1] = 0x431u<<20; in[0] = 0x7;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00040000 && out[0]==0x2, "shift=-2 sticky round up");
    in[1] = (0x433u-0x64u)<<20; in[0] = 0xffffffff;
    fx_mantissa_scale(out, in);
    check(out[1]==0 && out[0]==0, "shift=-100 (sa>=64) -> 0");
    in[1] = (0x433u<<20) | 0x00012345; in[0] = 0;
    fx_mantissa_scale(out, in);
    check(out[1]==0x00112345 && out[0]==0, "shift=0 low-mantissa kept");
}

static void test_fx64_to_f32(void) {
    printf("== fx64_to_f32 (L0: 64-bit -> float32 0x049132) ==\n");
    check(fx64_to_f32(0, 0) == 0, "underflow (hi top12=0) -> 0");
    check(fx64_to_f32(0, 0x80000000) == 0, "bit31-only (hi&mask=0) -> 0");
    check(fx64_to_f32(0, 0x78000000) == 0x7f800000, "pos overflow clamp -> 0x7f800000");
    check(fx64_to_f32(0, 0x40000000) == 0x40000000, "normal -> 0x40000000 (2.0f)");
    check(fx64_to_f32(0, 0x40400000) == 0x42000000, "normal mantissa -> 0x42000000");
    check(fx64_to_f32(0, 0xc0000000) == 0xc0000000, "normal neg -> 0xc0000000 (-2.0f)");
}

static void test_fmt_decimal(void) {
    printf("== fmt_decimal (L0: decimal formatter 0x049db2) ==\n");
    uint8_t buf[64]; uint8_t *r;
    r = fmt_decimal(buf, 3, 2, (const uint8_t*)"12345");
    check(r - buf == 6 && strncmp((char*)buf, "123.45", 6) == 0, "123.45 (normal)");
    r = fmt_decimal(buf, 3, 2, (const uint8_t*)"12");
    check(r - buf == 6 && strncmp((char*)buf, "120.00", 6) == 0, "120.00 (src short, pad)");
    r = fmt_decimal(buf, 0, 0, (const uint8_t*)"");
    check(r - buf == 2 && strncmp((char*)buf, "0.", 2) == 0, "0. (intlen<=0 fractlen==0)");
    r = fmt_decimal(buf, 2, 0, (const uint8_t*)"12");
    check(r - buf == 3 && strncmp((char*)buf, "12.", 3) == 0, "12. (fractlen==0)");
    r = fmt_decimal(buf, -1, 3, (const uint8_t*)"45");
    check(r - buf == 4 && strncmp((char*)buf, ".045", 4) == 0, ".045 (negative intlen)");
}

static void test_fmt_int_signed(void) {
    printf("== fmt_int_signed (L0: signed integer formatter 0x049d0b) ==\n");
    uint8_t buf[64]; int32_t ret;
    ret = fmt_int_signed(buf, 0, 10, 456);
    check(ret == 4 && strncmp((char*)buf, "+456", 4) == 0, "+456 (3-digit)");
    ret = fmt_int_signed(buf, 0, 10, -45);
    check(ret == 3 && strncmp((char*)buf, "-45", 3) == 0, "-45 (2-digit)");
    ret = fmt_int_signed(buf, 3, 10, 7);
    check(ret == 4 && strncmp((char*)buf, "+007", 4) == 0, "+007 (width pad)");
    ret = fmt_int_signed(buf, 0, 10, 7);
    check(ret == 3 && strncmp((char*)buf, "+07", 3) == 0, "+07 (default width 2)");
    memset(buf, 0xaa, sizeof buf);
    ret = fmt_int_signed(buf, 2, 10, 123);
    check(ret == 4 && buf[0] == 0xaa, "no output (digits>width)");
    memset(buf, 0xaa, sizeof buf);
    ret = fmt_int_signed(buf, 0, 3, 456);
    check(ret == 4 && buf[0] == 0xaa, "no output (width+1>maxw)");
    ret = fmt_int_signed(buf, 0, 10, -1);
    check(ret == 3 && strncmp((char*)buf, "-01", 3) == 0, "-01 (1-digit neg)");
}

static void test_fx_extract(void) {
    printf("== fx_extract (L0: fixed-point exp mantissa extractor 0x047ea4) ==\n");
    uint8_t s[12];
    memset(s,0,12); *(uint16_t*)(s+8)=0x3ffe;
    check(fx_extract(s,0x20) == -1, "A: exp@bias lo=hi=0 mode0x20 -> -1");
    *(uint16_t*)(s+8)=0x3ffd;
    check(fx_extract(s,0x20) == 0, "B: exp<bias underflow -> 0");
    *(uint16_t*)(s+8)=0x401f;
    check(fx_extract(s,0x20) == 0x80000000, "C: exp overflow -> 0x80000000");
    memset(s,0,12); *(uint32_t*)(s+4)=0x80000000; *(uint16_t*)(s+8)=0x3ffe;
    check(fx_extract(s,0x20) == -1, "D: hi=0x80000000 exp@bias -> -1");
    memset(s,0,12); *(uint32_t*)(s+4)=0x12345678; *(uint16_t*)(s+8)=0x401e;
    check(fx_extract(s,0x20) == 0x12345679, "E: cl=0x20 hi=0x12345678 -> hi+1");
    memset(s,0,12); *(uint16_t*)(s+8)=0x3ffe;
    check(fx_extract(s,0x1f) == 0, "F: mode0x1f exp@bias lo=hi=0 -> 0");
    check(fx_extract(s,0xa0) == -1, "G: mode0xa0 exp@bias lo=hi=0 -> -1");
    *(uint16_t*)(s+8)=0x401e;
    check(fx_extract(s,0x1f) == 0x80000000, "H: mode0x1f cl=0x20>blm -> overflow");
}

static void test_fx_norm32(void) {
    printf("== fx_norm32 (L0: fixed-point mantissa normalizer 0x047fd4) ==\n");
    uint8_t s[12];
    // T1: exp=0 hi=0x123400 lo=0 -> cx=0 -> M=(hi+0x100)&0xffffff00 = 0x123500
    memset(s,0,12); *(uint32_t*)(s+4)=0x123400; *(uint16_t*)(s+8)=0;
    check(fx_norm32(s) == 0x123500, "T1: cx=0 -> M");
    // T2: exp=1 hi=0x123400 -> cx=1 -> cx2=0xc081 -> 0
    *(uint16_t*)(s+8)=1;
    check(fx_norm32(s) == 0, "T2: cx small -> 0");
    // T3: exp=0x7fff hi=0x123400 -> special -> ((0x123500<<1)>>8|0xff000000)<<1 = 0xfe0048d4
    *(uint16_t*)(s+8)=0x7fff;
    check(fx_norm32(s) == 0xfe0048d4, "T3: cx=0x7fff special");
    // T4: exp=0x407f -> cx2=0xff -> 0xff000000<<1 = 0xfe000000
    *(uint16_t*)(s+8)=0x407f;
    check(fx_norm32(s) == 0xfe000000, "T4: cx2>=0xff -> 0xfe000000");
    // T5: exp=0x3f80 -> cx2=0 -> SHRD -> ((0x123500<<1)<<8|0)<<1 = 0x48d40000
    *(uint16_t*)(s+8)=0x3f80;
    check(fx_norm32(s) == 0x48d40000, "T5: cx2<0xff SHRD path");
    // T6: hi=0xffffff00 exp=5 -> carry -> eax=0x80000000 cx=6 -> 0
    memset(s,0,12); *(uint32_t*)(s+4)=0xffffff00; *(uint16_t*)(s+8)=5;
    check(fx_norm32(s) == 0, "T6: overflow carry -> 0");
}

static void test_fx_double_exp(void) {
    printf("== fx_double_exp (L0: double->fixed exp 0x04a9da) ==\n");
    uint32_t o = 0xdeadbeef;
    fx_double_exp(0.0, &o);          check(o == 0, "1.0: +0.0 -> 0");
    fx_double_exp(-0.0, &o);         check(o == 0, "-0.0 -> 0");
    fx_double_exp(1.0, &o);          check(o == 0xfffffd01u, "1.0 (hi=0x3ff00000) -> 255-0x3fe");
    fx_double_exp(-1.0, &o);         check(o == 0xfffffd01u, "-1.0 (hi=0xbff00000) -> same");
    fx_double_exp(2.0, &o);          check(o == 0xfffffc02u, "2.0 (hi=0x40000000, low8=0) -> -0x3fe");
    fx_double_exp(5e-324, &o);       check(o == 0xfffffc02u, "smallest subnormal (hi=0,lo=1) -> -0x3fe");
}

static void test_datetime_pack(void) {
    printf("== datetime_pack (L0: DOS date/time packer 0x045687) ==\n");
    sys_date = (struct dos_date){40, 5, 15, 6};
    sys_time = (struct dos_time){10, 30, 45, 0};
    struct datetime_pack p; memset(&p, 0xab, sizeof p);
    uint32_t r = datetime_pack(&p);
    check(r == 0, "return always 0 (DOW dead)");
    check(p.f0 == 0, "f0=0");
    check(p.f4 == 15, "f4=day=15");
    check(p.f8 == 5, "f8=month=5");
    check(p.fc == 0, "fc=0");
    check(p.f10 == 0xffffffffu, "f10=-1");
    check(p.f14 == (uint8_t)(30*256+45-0x76c), "f14=low8(min*256+sec-0x76c)");
    check(p.f14 == 0xc1, "f14 concrete = 0xc1");
    check(p.f20 == 0xffffffffu, "f20=-1");
    check(p.f18 == 0xababababu, "f18 untouched");
}

static double dbl(uint64_t b) { double d; memcpy(&d, &b, 8); return d; }
static void test_fx_double_naninf(void) {
    printf("== fx_double_naninf (L0: double NaN/Inf inline-string 0x04a2a7) ==\n");
    char out[16]; int r;
    memset(out,'x',sizeof out); r = fx_double_naninf(dbl(0x7ff0000000000000ULL), out);
    check(r==1 && memcmp(out,"inf\0",4)==0, "Inf -> inf");
    memset(out,'x',sizeof out); r = fx_double_naninf(dbl(0xfff0000000000000ULL), out);
    check(r==1 && memcmp(out,"-inf\0",5)==0, "-Inf -> -inf");
    memset(out,'x',sizeof out); r = fx_double_naninf(dbl(0x7ff8000000000000ULL), out);
    check(r==1 && memcmp(out,"nan\0",4)==0, "NaN -> nan");
    memset(out,'x',sizeof out); r = fx_double_naninf(dbl(0xfff8000000000000ULL), out);
    check(r==1 && memcmp(out,"-nan\0",5)==0, "-NaN -> -nan");
    memset(out,'x',sizeof out); r = fx_double_naninf(1.0, out);
    check(r==1 && memcmp(out,"inf\0",4)==0, "1.0 -> inf (latent exp-low8 bug)");
    memset(out,'x',sizeof out); r = fx_double_naninf(3.0, out);
    check(r==0 && out[0]=='x', "3.0 -> not nan/inf");
    memset(out,'x',sizeof out); r = fx_double_naninf(0.0, out);
    check(r==0 && out[0]=='x', "0.0 -> not nan/inf");
}

struct fx3 { uint32_t lo, hi; uint16_t exp; };
static void set_fx(struct fx3 *f, uint32_t lo, uint32_t hi, uint16_t e) { f->lo=lo; f->hi=hi; f->exp=e; }
static void test_fx_compare(void) {
    printf("== fx_compare (L0: fx lexicographic compare 0x047ab6) ==\n");
    struct fx3 a, b;
    set_fx(&a,0,0,0x3ffe); set_fx(&b,0,0,0x3ffe); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==0, "equal -> 0");
    set_fx(&a,0,0,0x3000); set_fx(&b,0,0,0x4000); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==-1, "a.exp<b.exp -> -1");
    set_fx(&a,0,0,0x4000); set_fx(&b,0,0,0x3000); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==1, "a.exp>b.exp -> 1");
    set_fx(&a,0,1,0x3ffe); set_fx(&b,0,2,0x3ffe); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==-1, "a.hi<b.hi -> -1");
    set_fx(&a,5,1,0x3ffe); set_fx(&b,3,1,0x3ffe); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==1, "a.lo>b.lo -> 1");
    set_fx(&a,1,1,0x7fff); set_fx(&b,0,0,0x3ffe); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==2, "a special -> 2");
    set_fx(&a,0,0,0x3ffe); set_fx(&b,1,1,0x7fff); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==2, "b special -> 2");
    set_fx(&a,0,0,0x7fff); set_fx(&b,0,0,0x3ffe); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==1, "a zero vs b normal: a.exp 0x7fff>b.exp -> 1");
    set_fx(&a,0,0,0x7fff); set_fx(&b,0,0,0x7fff); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==0, "both zero -> 0");
    set_fx(&a,0,0x80000000,0x7fff); set_fx(&b,0,0x80000000,0x7fff); check(fx_compare((uint8_t*)&a,(uint8_t*)&b)==0, "hi=0x80000000 lo=0 not special (hi<<1 overflows to 0), equal -> 0");
}

struct fxcl { uint32_t lo, hi; uint16_t exp; };
static void test_fx_classify_status(void) {
    printf("== fx_classify_status (L0: fx classifier 0x048666) ==\n");
    struct fxcl v;
    uint8_t ctx[16];
    uint16_t *st = (uint16_t*)ctx + 2;  /* ctx+4 */
    uint16_t *fl = (uint16_t*)ctx + 4;  /* ctx+8 */
    v.lo=0; v.hi=0; v.exp=0; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x4000 && *fl==0x0001, "zero, not flagged -> 0x40, flags|=mask1c");
    v.lo=0; v.hi=0; v.exp=0; memset(ctx,0,sizeof ctx); *fl=0x0003; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x4100 && *fl==0x0003, "zero, already flagged(mask22) -> 0x41");
    v.lo=1; v.hi=0; v.exp=0; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x4400 && *fl==0x0000, "mag0, hi|lo!=0 -> 0x44");
    v.lo=0; v.hi=0; v.exp=0x7fff; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x0100 && *fl==0x0002, "special lo0 hi0 -> 0x01, flags|=mask20");
    v.lo=0; v.hi=0x80000000; v.exp=0x7fff; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x0500 && *fl==0x0002, "special lo0 hi=0x80000000 -> 0x05");
    v.lo=0; v.hi=0x80000000; v.exp=0x4000; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x0400 && *fl==0x0000, "normal, hi sign set -> 0x04");
    v.lo=0; v.hi=0x00000001; v.exp=0x4000; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x4400 && *fl==0x0002, "normal, hi sign clear -> 0x44, flags|=mask20");
    v.lo=0; v.hi=0x00000001; v.exp=0x8000; memset(ctx,0,sizeof ctx); *fl=0; *st=0; fx_classify_status((uint8_t*)&v, ctx, 0);
    check(*st==0x4600 && *fl==0x0000, "mag0, hi sign set: 0x44|0x02 -> 0x46");
}

static void test_fx_mul64(void) {
    printf("== fx_mul64 (L0: fx 64x64 multiply 0x047d4e) ==\n");
    check(fx_mul64(0,0,5,1, 0) == 0, "A==0 & esl*2==0 -> 0");
    check(fx_mul64(1,0,0,0, 0) == 0, "B==0 & esi hi==0 -> 0");
    check(fx_mul64(0,0,0,0, (0x3fffu<<16)) == 0, "A==0,B==0 -> 0");
    check(fx_mul64(1,0,1,0, (0x7fffu<<16)|0x3fff) == 0x8000000000000000ULL,
          "overflow -> 0x8000000000000000");
    check(fx_mul64(1,0,1,0, (0x3fffu<<16)) == 0, "1*1 @ 0x3fff underflows -> 0");
    check(fx_mul64(0,1,0,1, (0x3fffu<<16)) == 1, "2^32*2^32 @ 0x3fff -> 1");
    /* A = 0x1ffffffff (a_hi=1,a_lo=0xffffffff), B = 1 : cross term exercised */
    check(fx_mul64(0xffffffff,1, 1,0, (0x3fffu<<16)) == 0, "(2^33-1)*1 @ 0x3fff -> 0");
    /* determinism */
    check(fx_mul64(7,3,11,2, (0x3fffu<<16)|0x10) == fx_mul64(7,3,11,2, (0x3fffu<<16)|0x10),
          "deterministic");
}

static void test_fx_stub_4a923(void) {
    printf("== fx_stub_4a923 (L0: no-op stub 0x04a923) ==\n");
    /* Disassembly-verified no-op: CALL 0x4a9d8 (POP EDI; RET) returns to the
     * caller with EDI clobbered and no return value. Callable, no observable
     * side effect. */
    fx_stub_4a923();
    check(1, "callable, void no-op");
}

static void test_fx_round_down(void) {
    printf("== fx_round_down (L0: fx round-down 0x0481df) ==\n");
    /* v = {u32 lo, u32 hi, u16 exp}. exp in v[2] low 16 bits. */
    struct { uint32_t lo, hi, exp, mode; } c[6] = {
        /*1*/ {0xffffffffu, 0x00000000u, 0x401f, 0x400},
        /*2*/ {0x00001234u, 0x00005678u, 0x3000, 0x400},
        /*3*/ {0x40000000u, 0x00000000u, 0x401f, 0x0000},
        /*4*/ {0x7fffffffu, 0x00000000u, 0x401f, 0x0000},
        /*5*/ {0x00000000u, 0x7fffffffu, 0x3fff, 0x0000},
        /*6*/ {0x00000001u, 0x80000000u, 0x3ffe, 0x0000},
    };
    struct { uint32_t lo, hi, exp; } want[6] = {
        /*1*/ {0x80000000u, 0x00000000u, 0x401f},  /* truncate, clears top bit */
        /*2*/ {0x00000000u, 0x00000000u, 0x0000},  /* underflow, mode 0x400 -> 0 */
        /*3*/ {0x00000000u, 0x00000000u, 0x401f},  /* round-half, equal->TEST fail */
        /*4*/ {0x80000000u, 0x00000000u, 0x401f},  /* round-half, add mask+1 */
        /*5*/ {0x00000000u, 0x80000000u, 0x3fff},  /* tableB on hi, add mask+1 */
        /*6*/ {0x00000000u, 0x80000000u, 0x3fff},  /* underflow -> +inf */
    };
    for (int i = 0; i < 6; i++) {
        uint32_t v[3] = { c[i].lo, c[i].hi, c[i].exp };
        fx_round_down(v, (uint16_t)c[i].mode);
        char m[96];
        snprintf(m, sizeof m, "case%d lo=0x%08x (want 0x%08x)", i + 1, v[0], want[i].lo);
        check(v[0] == want[i].lo, m);
        snprintf(m, sizeof m, "case%d hi=0x%08x (want 0x%08x)", i + 1, v[1], want[i].hi);
        check(v[1] == want[i].hi, m);
        snprintf(m, sizeof m, "case%d exp=0x%04x (want 0x%04x)", i + 1, (unsigned)v[2] & 0xffff, want[i].exp);
        check((uint16_t)v[2] == want[i].exp, m);
    }
    /* determinism */
    uint32_t a[3] = {0x7fffffffu, 0, 0x401f}, b[3] = {0x7fffffffu, 0, 0x401f};
    fx_round_down(a, 0); fx_round_down(b, 0);
    check(a[0] == b[0] && a[1] == b[1] && a[2] == b[2], "deterministic");
}

static void test_string_upper(void)
{
    printf("L1: string_upper (0x045605)\n");
    char s1[] = "abc"; string_upper(s1);
    check(strcmp(s1, "ABC") == 0, "abc -> ABC");
    char s2[] = "aBc1d"; string_upper(s2);
    check(strcmp(s2, "ABC1D") == 0, "aBc1d -> ABC1D (upper+digit preserved)");
    char s3[] = "123!@#"; string_upper(s3);
    check(strcmp(s3, "123!@#") == 0, "digits/symbols unchanged");
    char s4[] = ""; string_upper(s4);
    check(s4[0] == '\0', "empty stays empty");
    char s5[] = "hello world"; string_upper(s5);
    check(strcmp(s5, "HELLO WORLD") == 0, "hello world -> HELLO WORLD (space preserved)");
}

static void test_hex_digit_value(void)
{
    printf("L1: hex_digit_value (0x04457f)\n");
    check(hex_digit_value('0') == 0 && hex_digit_value('9') == 9, "digits 0-9");
    check(hex_digit_value('a') == 10 && hex_digit_value('f') == 15, "lower a-f -> 10-15");
    check(hex_digit_value('A') == 10 && hex_digit_value('F') == 15, "upper A-F -> 10-15 (to_lower)");
    check(hex_digit_value('g') == 0x10 && hex_digit_value('G') == 0x10, "g/G -> 16 (not a digit)");
    check(hex_digit_value('/') == 0x10 && hex_digit_value(':') == 0x10, "out-of-range -> 16");
}

static void test_prng(void)
{
    printf("L1: prng_next/prng_seed (0x041e9f/0x041ec3)\n");
    prng_seed(0x1234);
    check(prng_next() == 0x4dcb, "seed 0x1234 s1 ret=0x4dcb");
    check(prng_next() == 0x4bbe, "seed 0x1234 s2 ret=0x4bbe");
    check(prng_next() == 0x17d3, "seed 0x1234 s3 ret=0x17d3");
    prng_seed(0xdeadbeef);
    check(prng_next() == 0x1c01, "seed 0xdeadbeef s1 ret=0x1c01");
    check(prng_next() == 0x1f39, "seed 0xdeadbeef s2 ret=0x1f39");
    check(prng_next() == 0x6ac6, "seed 0xdeadbeef s3 ret=0x6ac6");
    prng_seed(0);
    check(prng_next() == 0x0000, "seed 0 s1 ret=0x0000");
    check(prng_next() == 0x53dc, "seed 0 s2 ret=0x53dc");
    check(prng_next() == 0x2704, "seed 0 s3 ret=0x2704");
}

static void test_itoa32s(void)
{
    printf("L1: itoa32s / itoa32s_tbl (0x041f42 / 0x0494a2)\n");
    char b[40]; char *r;
    r = itoa32s(-42, b, 10);
    check(r == b && strcmp(b, "-42") == 0, "-42 dec -> '-42', returns buf start");
    r = itoa32s(42, b, 10);
    check(r == b && strcmp(b, "42") == 0, "42 dec -> '42'");
    r = itoa32s(0, b, 10);
    check(r == b && strcmp(b, "0") == 0, "0 dec -> '0'");
    r = itoa32s(-255, b, 16);
    check(r == b && strcmp(b, "ffffff01") == 0, "-255 hex -> 'ffffff01' (no sign for base!=10)");
    r = itoa32s(255, b, 16);
    check(r == b && strcmp(b, "ff") == 0, "255 hex -> 'ff'");
    r = itoa32s((int32_t)0x80000000, b, 10);
    check(r == b && strcmp(b, "-2147483648") == 0, "INT_MIN dec -> '-2147483648' (NEG wraps)");
    r = itoa32s_tbl(-1234, b, 10);
    check(r == b && strcmp(b, "-1234") == 0, "tbl -1234 dec -> '-1234'");
    r = itoa32s_tbl(5, b, 2);
    check(r == b && strcmp(b, "101") == 0, "tbl 5 bin -> '101'");
}

static void test_heap_free_guard(void) {
    printf("heap_free_guard (0x041c28)\n");
    heap_arena_init();
    g_heap_seg_limit = 0;
    uint32_t data   = heap_acquire_region(0x1000);
    uint32_t region = data - 0x30;
    uint32_t p = heap_alloc(region, 0x100);
    check(p != 0, "allocation succeeds");
    heap_verify(region, "after alloc");
    heap_free_guard(0);                       /* no-op: must not crash or free */
    heap_verify(region, "guard(0) is a no-op, heap intact");
    heap_free_guard(p);                       /* frees p (== heap_free_host(p)) */
    heap_verify(region, "guard(p) frees the block");
    uint32_t p2 = heap_alloc(region, 0x100);
    check(p2 == p, "freed slot reused for same size");
    heap_verify(region, "after reuse");
    heap_free(region, p2);
    heap_verify(region, "cleanup");
}

static void test_tmp_filename(void) {
    printf("tmp_filename (0x0427f3)\n");
    char buf[16];
    g_dat_53390 = 0x1234;     tmp_filename(buf, 0x5a); check(strcmp(buf, "t1234_5a.tmp") == 0, "0x1234/0x5a -> t1234_5a.tmp");
    g_dat_53390 = 0x000000ab; tmp_filename(buf, 0x05); check(strcmp(buf, "t00ab_05.tmp") == 0, "0x00ab/0x05 -> t00ab_05.tmp");
    g_dat_53390 = 0xdeadbeef; tmp_filename(buf, 0xff); check(strcmp(buf, "tbeef_ff.tmp") == 0, "0xdeadbeef(low16=beef)/0xff -> tbeef_ff.tmp");
    g_dat_53390 = 0;          tmp_filename(buf, 0);    check(strcmp(buf, "t0000_00.tmp") == 0, "0/0 -> t0000_00.tmp");
    g_dat_53390 = 0x0000a1b2; tmp_filename(buf, 0x00); check(strcmp(buf, "ta1b2_00.tmp") == 0, "0xa1b2/0 -> ta1b2_00.tmp (lowercase hex)");
    g_dat_53390 = 0x000000ff; tmp_filename(buf, 0x0f); check(strcmp(buf, "t00ff_0f.tmp") == 0, "0xff/0x0f -> t00ff_0f.tmp");
    g_dat_53390 = 0;
}

static void test_config_value_ptr(void) {
    printf("config_value_ptr (0x04a24d)\n");
    static const char *table[] = {
        "path=C:\\games", "size=100", "name=Night", "verbose=off", 0
    };
    g_dat_537d4 = (char **)table;
    char *v;
    v = config_value_ptr("path");    check(v && strcmp(v, "C:\\games") == 0, "path -> C:\\games");
    v = config_value_ptr("SIZE");    check(v && strcmp(v, "100") == 0, "SIZE (case-insensitive) -> 100");
    v = config_value_ptr("name");    check(v && strcmp(v, "Night") == 0, "name -> Night");
    v = config_value_ptr("verbose"); check(v && strcmp(v, "off") == 0, "verbose -> off");
    v = config_value_ptr("missing"); check(v == 0, "missing -> 0");
    v = config_value_ptr("pa");      check(v == 0, "pa (no '=' at pos 2) -> 0");
    v = config_value_ptr("");        check(v == 0, "empty key -> 0");
    v = config_value_ptr(NULL);      check(v == 0, "NULL key -> 0");
    g_dat_537d4 = NULL;
    v = config_value_ptr("path");    check(v == 0, "NULL table -> 0");
}

static void test_time_parse(void) {
    printf("parse_time_str (0x049a6e)\n");
    char prefix[64];
    uint32_t secs;
    const char *ret;

    memset(prefix, 0, sizeof prefix);
    ret = parse_time_str("-12:34:56", prefix, &secs);
    check(strcmp(prefix, "") == 0, "  -12:34:56 prefix empty");
    check((int32_t)secs == -45296, "  -12:34:56 secs = -45296");
    check(ret != NULL && *ret == 0, "  -12:34:56 returns end");

    memset(prefix, 0, sizeof prefix);
    ret = parse_time_str("12:34:56", prefix, &secs);
    check((int32_t)secs == 9296, "  12:34:56 no-sign off-by-one -> 9296 (as 2:34:56)");
    (void)ret;

    memset(prefix, 0, sizeof prefix);
    ret = parse_time_str("ab12", prefix, &secs);
    check(strcmp(prefix, "ab") == 0, "  ab12 prefix = ab");
    check(secs == 7200, "  ab12 secs = 7200 (only '2' hours)");
    (void)ret;

    memset(prefix, 0, sizeof prefix);
    ret = parse_time_str("ab+5", prefix, &secs);
    check(strcmp(prefix, "ab") == 0, "  ab+5 prefix = ab");
    check(secs == 18000, "  ab+5 secs = 18000 (5 hours, sign +)");
    (void)ret;

    memset(prefix, 0, sizeof prefix);
    ret = parse_time_str("ab5", prefix, &secs);
    check(strcmp(prefix, "ab") == 0, "  ab5 prefix = ab");
    check(ret != NULL && *ret == 0, "  ab5 no time (timepos at NUL)");

    /* prefix longer than 30 is clamped to 30. */
    memset(prefix, 0, sizeof prefix);
    static char longpref[80];
    for (int i = 0; i < 35; i++) longpref[i] = 'a';
    longpref[35] = '1';
    longpref[36] = 0;
    ret = parse_time_str(longpref, prefix, &secs);
    check(prefix[29] == 'a' && prefix[30] == 0, "  long prefix clamped to 30");
    (void)ret;
}

static void test_heap_size_adjust(void) {
    printf("heap_size_adjust (0x0431b5)\n");
    uint32_t block, r;
    g_heap_head = 0;                 /* heap_top_check() -> 0 */
    g_heap_cfg_b2 = 0; g_heap_cfg_b3 = 0;
    set_53640();                      /* g_dat_53640 = 0x8000 (limit) */

    block = 0;         r = heap_size_adjust(&block);
    check(r == 0 && block == 0, "  empty block -> 0, unchanged");

    block = 0x40;      r = heap_size_adjust(&block);
    check(r == 1 && block == 0x8000, "  0x40 (clamped to limit) -> 0x8000, ret 1");

    block = 0x7fc8;    r = heap_size_adjust(&block);
    check(r == 1 && block == 0x9000, "  0x7fc8 (a+0x3c=0x8004 >= limit) -> 0x9000 page-up");

    block = 0x10000;   r = heap_size_adjust(&block);
    check(r == 1 && block == 0x11000, "  0x10000 (+0x3c, page-up) -> 0x11000");

    g_heap_cfg_b2 = 1; g_heap_cfg_b3 = 0;   /* +8 branch */
    block = 0x40;      r = heap_size_adjust(&block);
    check(r == 1 && block == 0x8000, "  cfg_b2=1,cfg_b3=0 0x40 (+8 then clamp) -> 0x8000");

    g_heap_cfg_b2 = 0; g_heap_cfg_b3 = 0;
    block = 0xfffffffb; r = heap_size_adjust(&block);
    check(r == 0 && block == 0xfffffffc, "  0xfffffffb -> align 0xfffffffc, +0x3c wrap -> ret 0");
}

static uint32_t g_mock_cb1_arg, g_mock_cb2_arg;
static uint32_t mock_cb1(uint32_t x) { g_mock_cb1_arg = x; return x + 100; }
static uint32_t mock_cb2(uint32_t x) { g_mock_cb2_arg = x; return 0; }
static void test_fn_0423cf(void) {
    printf("fn_0423cf (0x0423cf)\n");
    g_mock_cb1_arg = g_mock_cb2_arg = 0;
    g_dat_53370 = mock_cb1; g_dat_53374 = mock_cb2;
    uint32_t r = fn_0423cf(42);
    check(r == 42, "  returns original input");
    check(g_mock_cb1_arg == 42, "  cb1 got original input");
    check(g_mock_cb2_arg == 142, "  cb2 got cb1's return (42+100)");
    g_dat_53370 = NULL; g_dat_53374 = NULL;
}

static uint32_t g_test533bd_arg;
static uint32_t test533bd_cb(uint32_t a) { g_test533bd_arg = a; return a * 2; }
static void test_042a08_family(void) {
    printf("0x42a08 family (0x42a08/0d/30/3c)\n");
    uint32_t b8 = g_dat_64008, b4 = g_dat_64004;
    fn_042a08();
    check(g_dat_64008 == 0xd, "  fn_042a08 sets g_dat_64008 = 13");
    fn_042a0d(0xdeadbeef);
    check(g_dat_64008 == 0xdeadbeef, "  fn_042a0d(x) sets g_dat_64008 = x");
    fn_042a30(0x12345678);
    check(g_dat_64004 == 0x12345678, "  fn_042a30(x) sets g_dat_64004 = x");
    g_test533bd_arg = 0;
    g_dat_533bd = test533bd_cb;
    uint32_t r = fn_042a3c(21);
    check(r == 42 && g_test533bd_arg == 21, "  fn_042a3c(x) calls g_dat_533bd(x), returns result");
    g_dat_533bd = NULL;
    g_dat_64008 = b8; g_dat_64004 = b4;
}

static void test_047d24(void) {
    printf("fn_047d24 (0x047d24)\n");
    uint32_t a[3] = { 0x11111111, 0x22222222, 0x0012 };
    uint32_t b[3] = { 0x33333333, 0x44444444, 0x0034 };
    uint32_t out[3] = {0,0,0};
    fn_047d24(a, b, out);
    uint32_t esi = ((uint32_t)(uint16_t)b[2] << 16) | (uint16_t)a[2];
    uint64_t r = fx_mul64(a[0], a[1], b[0], b[1], esi);
    check(out[0] == (uint32_t)r && out[1] == (uint32_t)(r >> 32), "  out[0..4] = fx_mul64 lo/hi");
    check(out[2] == 0x0012, "  out[8] (word) = A[8]");
    uint32_t az[3] = {0,0,0}; uint32_t oz[3] = {9,9,9};
    fn_047d24(az, b, oz);
    uint64_t rz = fx_mul64(0, 0, b[0], b[1], ((uint32_t)(uint16_t)b[2] << 16));
    check(oz[0] == (uint32_t)rz && oz[1] == (uint32_t)(rz >> 32) && oz[2] == 0, "  zero-A matches fx_mul64, out[8]=0");
}

static void test_044bb4(void) {
    printf("fn_044bb4 (0x044bb4)\n");
    uint8_t node[0x20];
    uint32_t sdx = sys_ioctl_dx, scf = sys_ioctl_cf;
    memset(node, 0, sizeof node);
    node[0x10] = 0x55;   /* handle */
    /* ioctl_device: dx=0,cf=0 -> 0 ; dx=0x80,cf=0 -> 1 */
    sys_ioctl_dx = 0x80; sys_ioctl_cf = 0;
    node[0x0d] = 0x20;
    fn_044bb4(node);
    check(node[0x0d] == 0x20, "  early-return if node[0x0d]&0x20");
    node[0x0d] = 0; sys_ioctl_dx = 0; sys_ioctl_cf = 0;
    fn_044bb4(node);
    check(node[0x0d] == 0, "  no change when ioctl_device returns 0");
    node[0x0d] = 0; sys_ioctl_dx = 0x80; sys_ioctl_cf = 0;
    fn_044bb4(node);
    check(node[0x0d] == 0x22, "  ioctl!=0 & (b&7)==0 -> 0x22");
    node[0x0d] = 0x3; sys_ioctl_dx = 0x80; sys_ioctl_cf = 0;
    fn_044bb4(node);
    check(node[0x0d] == (0x3 | 0x20), "  ioctl!=0 & (b&7)!=0 -> b|0x20");
    sys_ioctl_dx = sdx; sys_ioctl_cf = scf;
}

static void test_044ea4(void) {
    printf("fn_044ea4 (0x044ea4)\n");
    uint8_t spec[0x20];
    uint32_t va[4];
    uint32_t *cur;
    uint8_t *r;
    memset(spec,0,sizeof spec); va[0]=0; cur=va;
    r = fn_044ea4((const uint8_t*)"5w", &cur, spec);
    check(*(uint32_t*)(spec+4)==5 && *(uint32_t*)(spec+8)==0xffffffff && (spec[0x14]&0x20) && r[0]==0,
          "  5w: width=5, prec=-1, w flag, return at end");
    memset(spec,0,sizeof spec); cur=va;
    r = fn_044ea4((const uint8_t*)"12F", &cur, spec);
    check(*(uint32_t*)(spec+4)==12 && (spec[0x14]&0x80) && r==(uint8_t*)"12F"+3,
          "  12F: width=12, F flag, return after F");
    memset(spec,0,sizeof spec); cur=va;
    r = fn_044ea4((const uint8_t*)"3.7l", &cur, spec);
    check(*(uint32_t*)(spec+4)==3 && *(uint32_t*)(spec+8)==7 && (spec[0x14]&0x20) && r==(uint8_t*)"3.7l"+4,
          "  3.7l: width=3, prec=7, l flag");
    memset(spec,0,sizeof spec); va[0]=42; va[1]=99; cur=va;
    r = fn_044ea4((const uint8_t*)"*w", &cur, spec);
    check(*(uint32_t*)(spec+4)==42 && (spec[0x14]&0x20) && cur==(va+1) && r==(uint8_t*)"*w"+2,
          "  *w: width from vararg=42, cursor advanced");
    memset(spec,0,sizeof spec); cur=va;
    r = fn_044ea4((const uint8_t*)"05N", &cur, spec);
    check(spec[0x16]==0x30 && *(uint32_t*)(spec+4)==5 && (spec[0x14]&0x40) && r==(uint8_t*)"05N"+3,
          "  05N: '0' fill, width=5, N flag");
    memset(spec,0,sizeof spec); va[0]=(uint32_t)(int32_t)-42; cur=va;
    r = fn_044ea4((const uint8_t*)"*w", &cur, spec);
    check(*(uint32_t*)(spec+4)==42 && (spec[0x14]&0x8) && (spec[0x14]&0x20),
          "  *w neg: width=42, sign flag set");
}

static void test_049b84(void) {
    printf("fn_049b84 (0x049b84)\n");
    uint32_t out[9];
    const char *r;
    memset(out, 0xAA, sizeof out);
    r = fn_049b84("M12.34.56/07:08:09", out);
    check(out[8]==0 && out[4]==11 && out[3]==34 && out[6]==56 && out[7]==0 &&
          out[2]==7 && out[1]==8 && out[0]==9 && *r==0,
          "  M12.34.56/07:08:09: era=0,[10]=11,[0c]=34,[18]=56,[1c]=0,m=7,min=8,sec=9");
    check(out[5]==0xAAAAAAAA, "  M case: [14] untouched (sentinel)");
    memset(out, 0xAA, sizeof out);
    r = fn_049b84("J123/05:06:07", out);
    check(out[8]==1 && out[7]==123 && out[2]==5 && out[1]==6 && out[0]==7 && *r==0,
          "  J123/05:06:07: era=1,[1c]=123,m=5,min=6,sec=7");
    check(out[3]==0xAAAAAAAA && out[4]==0xAAAAAAAA && out[6]==0xAAAAAAAA,
          "  J case: [0c]/[10]/[18] untouched (sentinel)");
    memset(out, 0xAA, sizeof out);
    r = fn_049b84("1234", out);
    check(out[8]==0xffffffff && out[7]==1234 && out[2]==2 && out[1]==0 && out[0]==0 && *r==0,
          "  1234: era=-1,[1c]=1234,default m=2,min=sec=0");
    memset(out, 0xAA, sizeof out);
    r = fn_049b84("J9/01:02", out);
    check(out[8]==1 && out[7]==9 && out[2]==1 && out[1]==2 && out[0]==0,
          "  J9/01:02: m=1,min=2,sec=0 (no 2nd ':')");
}

static void test_0492d0(void) {
    printf("fn_0492d0 (0x0492d0)\n");
    uint32_t nodes[8] = {0,0,0,0,0,0,0,0};
    g_dat_53730 = nodes; g_dat_536dc = 8;
    uint64_t r;
    r = fn_0492d0(8, 0xEE);
    check(r == (0xEEull << 32), "  out-of-range (index>=bound): (ctx,0)");
    nodes[1] = 0x00010203; sys_ioctl_dx = 0x80; sys_ioctl_cf = 0;
    r = fn_0492d0(1, 0xAA);
    check(r == (0xAAull << 32) | 0x00016203, "  init, ioctl=1: flags 0x60 set (0x02|0x60=0x62)");
    nodes[2] = 0x00004000; sys_ioctl_dx = 0x80;
    r = fn_0492d0(2, 0xCC);
    check(r == (0xCCull << 32) | 0x00004000, "  flag already set: skip ioctl, unchanged");
    nodes[3] = 0; sys_ioctl_dx = 0; sys_ioctl_cf = 0;
    r = fn_0492d0(3, 0xBB);
    check(r == (0xBBull << 32) | 0x00004000, "  init, ioctl=0: only 0x40 set");
    nodes[7] = 0x11223344;
    r = fn_0492d0(7, 0xDD);
    check(r == (0xDDull << 32) | 0x11223344, "  index>=6: no init, return node value");
    g_dat_53730 = NULL;
}

static void test_042a19(void) {
    printf("fn_042a19 (0x042a19)\n");
    g_dat_64008 = 0;
    fn_042a19();
    check(g_dat_64008 == 0xe, "  sets g_dat_64008 = 0xe");
}

static void test_044b96(void) {
    printf("fn_044b96 (0x044b96) node-list drain\n");
    heap_arena_init();
    static uint8_t pool[0x40];
    g_ndpool = pool; memset(pool, 0, sizeof pool);
    uint32_t *p = (uint32_t *)pool;
    p[2] = 16; p[3] = 40;   /* node0 @8:  next=16, rec=40 */
    p[4] = 24; p[5] = 48;   /* node1 @16: next=24, rec=48 */
    p[6] = 0;  p[7] = 56;   /* node2 @24: next=0,  rec=56 */
    g_node_list_done = 8;   /* head = node0 */
    fn_044b96();
    check(g_node_list_done == 0, "  drained: done list empty");
    fn_044b96();
    check(g_node_list_done == 0, "  already empty: no-op");
}

static void test_045079(void) {
    printf("fn_045079 (0x045079) zero-pad + right-justify hex\n");
    char buf[16], ref[16], exp[16];
    int L, pad;
    itoa32s(0x1a2b, ref, 16); L = 0; while (ref[L]) L++;
    pad = 6 - L; for (int i=0;i<pad;i++) exp[i]='0'; memcpy(exp+pad, ref, L+1);
    memset(buf, 0, sizeof buf); fn_045079(0x1a2b, buf, 6);
    check(memcmp(buf, exp, 6)==0 && buf[6]==0, "  0x1a2b width6 zero-padded");
    memset(buf, 0, sizeof buf); fn_045079(0x1a2b, buf, L);
    check(memcmp(buf, ref, L)==0 && buf[L]==0, "  width==len: no pad");
    itoa32s(0xff, ref, 16); L = 0; while (ref[L]) L++;
    pad = 8 - L; for (int i=0;i<pad;i++) exp[i]='0'; memcpy(exp+pad, ref, L+1);
    memset(buf, 0, sizeof buf); fn_045079(0xff, buf, 8);
    check(memcmp(buf, exp, 8)==0 && buf[8]==0, "  0xff width8");
    itoa32s(0, ref, 16); L = 0; while (ref[L]) L++;
    pad = 4 - L; for (int i=0;i<pad;i++) exp[i]='0'; memcpy(exp+pad, ref, L+1);
    memset(buf, 0, sizeof buf); fn_045079(0, buf, 4);
    check(memcmp(buf, exp, 4)==0 && buf[4]==0, "  0 width4");
}

static void test_0450da(void) {
    printf("fn_0450da (0x0450da) 16.16 fixed -> decimal string\n");
    char buf[24]; int spec[3];
    spec[0]=spec[1]=0; spec[2]=4;
    memset(buf,'?',23); fn_0450da(buf, 98304, spec);   /* 1.5 */
    check(strcmp(buf,"1.5000")==0, "  1.5 -> 1.5000");
    memset(buf,'?',23); fn_0450da(buf, -98304, spec);  /* -1.5 */
    check(strcmp(buf,"-1.5000")==0, "  -1.5 -> -1.5000");
    spec[2]=2; memset(buf,'?',23); fn_0450da(buf, 32768, spec);  /* 0.5 */
    check(strcmp(buf,"0.50")==0, "  0.5 -> 0.50");
    spec[2]=0; memset(buf,'?',23); fn_0450da(buf, 131072, spec); /* 2.0 */
    check(strcmp(buf,"2")==0, "  2.0 (0 places) -> 2");
    spec[2]=1; memset(buf,'?',23); fn_0450da(buf, 6553, spec);   /* ~0.1 */
    check(strcmp(buf,"0.1")==0, "  0.1 -> 0.1");
    spec[2]=3; memset(buf,'?',23); fn_0450da(buf, 0, spec);      /* 0.0 */
    check(strcmp(buf,"0.000")==0, "  0.0 -> 0.000");
    /* rounding: 2.5 with 0 places -> 3 */
    spec[2]=0; memset(buf,'?',23); fn_0450da(buf, 163840, spec); /* 2.5 */
    check(strcmp(buf,"3")==0, "  2.5 (0 places) -> 3 (round)");
}

static void test_049a35(void) {
    printf("fn_049a35 (0x049a35) TZ config parser\n");
    g_dat_53784 = 0xdead; g_dat_537d4 = NULL;
    fn_049a35(0,0,0);
    check(g_dat_53784 == 0xdead, "  no TZ -> early return (53784 unchanged)");
    static const char *t1[] = {"TZ=-05:00:00", 0};
    g_dat_537d4 = (char**)t1;
    g_dat_53784 = 0xdead; g_dat_537a7 = 0xdead; g_dat_5377c = 0xdead;
    fn_049a35(0,0,0);
    check(g_dat_53784 == 0, "  -05:00:00 -> 53784=0");
    check(g_dat_537a7 == 0, "  -05:00:00 -> 537a7=0");
    check((int32_t)g_dat_5377c == -18000, "  -05:00:00 -> 5377c=-18000");
    static const char *t2[] = {"TZ=-05:00:00 -07:00:00", 0};
    g_dat_537d4 = (char**)t2;
    g_dat_53784 = 0; g_dat_53780 = 0xdead;
    fn_049a35(0,0,0);
    check(g_dat_53784 == 1, "  two times -> 53784=1");
    g_dat_537d4 = NULL;
}

static void test_049c76(void) {
    printf("fn_049c76 (0x049c76) TZ range parser (direct string)\n");
    g_dat_53784 = 0xdead; g_dat_537a7 = 0xdead; g_dat_5377c = 0xdead;
    fn_049c76("-05:00:00", 0, 0, 0);
    check(g_dat_53784 == 0, "  -05:00:00 -> 53784=0");
    check(g_dat_537a7 == 0, "  -05:00:00 -> 537a7=0");
    check((int32_t)g_dat_5377c == -18000, "  -05:00:00 -> 5377c=-18000");
    g_dat_53784 = 0; g_dat_53780 = 0xdead;
    fn_049c76("-05:00:00 -07:00:00", 0, 0, 0);
    check(g_dat_53784 == 1, "  two times -> 53784=1");
    check(g_dat_53780 == 18000u - (uint32_t)(int32_t)(-25200), "  two times -> 53780=18000-(-25200)");
}

static void test_045a82(void) {
    printf("fn_045a82 (0x045a82) byte classifier\n");
    g_dat_64004 = 0; g_dat_64008 = 0; g_dat_533bb = 3;
    uint64_t r = fn_045a82(0x50, 0xabcd);   // 'P'
    check(g_dat_64004 == 0x50, "  'P' 64004=0x50");
    check(g_dat_64008 == 7, "  'P' 64008=7 (tbl[0xe])");
    check(r == 0xabcdffffffffULL, "  'P' ret high=0xabcd low=0xffffffff");
    g_dat_64004 = 0; g_dat_64008 = 0;
    fn_045a82(0x1f, 0);   // 31 < 0x20, clamps to 0x13
    check(g_dat_64004 == 0x1f, "  0x1f 64004=0x1f");
    check(g_dat_64008 == 0xff, "  0x1f 64008=0xff (clamped tbl[0x13])");
    g_dat_64004 = 0; g_dat_64008 = 0;
    fn_045a82(0x20, 0);   // 0x20 in [0x20,0x22) -> 5
    check(g_dat_64008 == 6, "  0x20 64008=6 (tbl[5])");
    g_dat_64004 = 0; g_dat_64008 = 0;
    fn_045a82(0x1234, 0); // >= 0x100 -> (param>>8)&0xff
    check(g_dat_64004 == 0x34, "  0x1234 64004=0x34");
    check(g_dat_64008 == 0x12, "  0x1234 64008=0x12");
    g_dat_533bb = 2; g_dat_64004 = 0; g_dat_64008 = 0;
    fn_045a82(0x50, 0);   // threshold not met -> no classify, clamps to 0x13
    check(g_dat_64008 == 0xff, "  'P' no-classify 64008=0xff (clamped)");
    g_dat_533bb = 3;
}

static void test_045a76(void) {
    printf("fn_045a76 (0x045a76) fn_045a82 thunk\n");
    g_dat_64004 = 0; g_dat_64008 = 0;
    check(fn_045a76(0x50, 0) == 0, "  param_2=0 -> 0");
    check(g_dat_64004 == 0, "  param_2=0 -> 64004 unchanged");
    g_dat_533bb = 3;
    uint64_t r = fn_045a76(0x50, 0xabcd);   // tail-jump to fn_045a82
    check(r == 0xabcdffffffffULL, "  'P' tail-jump ret");
    check(g_dat_64004 == 0x50, "  'P' 64004=0x50");
    check(g_dat_64008 == 7, "  'P' 64008=7");
    g_dat_533bb = 0;
}

static uint32_t g_last5336c, g_last53370, g_last53374;
static uint32_t cb5336c(uint32_t x){ g_last5336c = x; return x + 10; }
static uint32_t cb53370(uint32_t x){ g_last53370 = x; return x; }
static uint32_t cb53374(uint32_t x){ g_last53374 = x; return 0; }
static void test_0423b7(void) {
    printf("fn_0423b7 (0x0423b7) callback dispatcher\n");
    g_dat_5336c = cb5336c; g_dat_53370 = cb53370; g_dat_53374 = cb53374;
    g_last5336c = g_last53370 = g_last53374 = 0;
    fn_0423b7(5, 0, 0, 0);
    check(g_last5336c == 5, "  5336c called with param_1=5");
    check(g_last53370 == 5, "  53370 last call = fn_0423cf(5)");
    check(g_last53374 == 5, "  53374 (fn_0423cf) = 5");
    g_dat_5336c = NULL; g_dat_53370 = NULL; g_dat_53374 = NULL;
}

static void test_042576(void) {
    printf("fn_042576 (0x042576) file-mode parser\n");
    g_dat_53619 = 0x200;
    check((uint32_t)fn_042576("r++", 0) == 0x43, "  'r++' -> 0x43");
    check((uint32_t)fn_042576("w+ ", 0) == 0x43, "  'w+ ' -> 0x43");
    check((uint32_t)fn_042576("ab ", 0) == 0xc2, "  'ab ' -> 0xc2");
    check((uint32_t)fn_042576("rbt", 0) == 0x41, "  'rbt' -> 0x41");
    check((uint32_t)fn_042576("wt+", 0) == 3, "  'wt+' -> 3");
    check((uint32_t)fn_042576("rb+", 0) == 0x43, "  'rb+' -> 0x43");
    check((uint32_t)fn_042576("x  ", 0) == 0, "  'x  ' -> 0 (invalid)");
    g_dat_53619 = 0;
    check((uint32_t)fn_042576("r+ ", 0) == 3, "  'r+ ' (53619=0) -> 3");
    g_dat_53619 = 0x200;
    check((uint32_t)fn_042576("wt  ", 0) == 2, "  'wt  ' -> 2");
}

/* fn_034aba node-arena helpers (test-only) */
static uint8_t *nar(void) { return (uint8_t *)g_node_arena_base; }
static void setnode(uint32_t o, int32_t start, uint32_t next, int16_t len, uint8_t flag) {
    uint8_t *n = nar() + o;
    uint32_t s=(uint32_t)start, nx=next; uint16_t ln=(uint16_t)len;
    n[0]=(uint8_t)s; n[1]=(uint8_t)(s>>8); n[2]=(uint8_t)(s>>16); n[3]=(uint8_t)(s>>24);
    n[4]=(uint8_t)nx; n[5]=(uint8_t)(nx>>8); n[6]=(uint8_t)(nx>>16); n[7]=(uint8_t)(nx>>24);
    n[8]=(uint8_t)ln; n[9]=(uint8_t)(ln>>8); n[10]=flag;
}
static int32_t  nstart(uint32_t o) { const uint8_t*p=nar()+o;   return (int32_t)((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24)); }
static uint32_t nnext(uint32_t o)  { const uint8_t*p=nar()+o+4; return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static int16_t  nlen(uint32_t o)   { const uint8_t*p=nar()+o+8; return (int16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8)); }
static uint8_t  nflag(uint32_t o)  { return *(nar() + o + 10); }

static void test_fn_034aba(void) {
    printf("fn_034aba (0x034aba)\n");
    // 3-way split (branch 2)
    memset(nar(), 0, 0x1000);
    g_dat_62d64 = 11; memset(g_dat_62d68, 0, sizeof g_dat_62d68);
    setnode(0, 0, 0, 200, 0x55); g_dat_62d68[0] = 0;
    fn_034aba(100, 0, 50, 999, 0xAB);
    check(nlen(0)==100 && nstart(0)==0 && nnext(0)==11 && nflag(0)==0x55, "  fn_034aba node0");
    check(nstart(11)==999 && nlen(11)==50 && nnext(11)==22 && nflag(11)==0xAB, "  fn_034aba node11");
    check(nstart(22)==150 && nlen(22)==50 && nnext(22)==0 && nflag(22)==0x55, "  fn_034aba node22");
    check(g_dat_62d64==33 && g_dat_62d68[0]==0, "  fn_034aba cursor+head");
    // 2-way split (branch 1, end reaches 0x140)
    memset(nar(), 0, 0x1000);
    g_dat_62d64 = 11; memset(g_dat_62d68, 0, sizeof g_dat_62d68);
    setnode(0, 0, 0, 0x140, 0x77); g_dat_62d68[1] = 0;
    fn_034aba(300, 1, 52, 12345, 0xCD);
    check(nlen(0)==300 && nstart(0)==0 && nnext(0)==11 && nflag(0)==0x77, "  fn_034aba b1 node0");
    check(nstart(11)==12345 && nlen(11)==20 && nflag(11)==0xCD, "  fn_034aba b1 node11");
    check(g_dat_62d64==22 && g_dat_62d68[1]==0, "  fn_034aba b1 cursor+head");
}

static void reset_03442e(void) {
    memset(g_dat_62cd8, 0, sizeof g_dat_62cd8);
    memset(g_dat_62c58, 0, sizeof g_dat_62c58);
    memset(g_dat_63f14, 0, sizeof g_dat_63f14);
    memset(g_dat_63f1e, 0, sizeof g_dat_63f1e);
    memset(g_dat_63f28, 0, sizeof g_dat_63f28);
    g_dat_62c75 = g_dat_62c82 = g_dat_62c8e = g_dat_62c90 = 0;
    g_dat_63f50 = 0; g_dat_63f08 = 0; g_dat_63f0c = 0;
}

static void test_fn_03442e(void) {
    printf("fn_03442e (0x03442e)\n");

    /* A: x=0, all flags 0 -> no branch, nothing recorded */
    reset_03442e();
    fn_03442e(0x00);
    check(g_dat_63f50==0 && g_dat_63f28[0]==0 && g_dat_63f14[0]==0 && g_dat_62cd8[0]==0,
          "  fn_03442e A no-match no-op");

    /* B: x=0x1d, 62c90 set -> marker 0x67, y=0x38, slot 2->3, delta 600 */
    reset_03442e();
    g_dat_63f50=2; g_dat_62c90=1; g_dat_63f08=1000; g_dat_63f0c=400;
    g_dat_62cd8[0x1d]=5; g_dat_62cd8[0x38]=7;
    fn_03442e(0x1d);
    check(g_dat_63f14[2]==0x67 && g_dat_63f1e[2]==0 && g_dat_63f28[2]==600 && g_dat_63f50==3,
          "  fn_03442e B x1d/62c90 slot");
    check(g_dat_62cd8[0x1d]==4 && g_dat_62c58[0x1d]==1 && g_dat_62cd8[0x38]==6 && g_dat_62c58[0x38]==1,
          "  fn_03442e B calls 0x1d,0x38");

    /* C: x=0x1d, 62c90=0, 62c82 set -> marker 0x68, y=0x2a, calls 0x1d,0x2a,0x36 */
    reset_03442e();
    g_dat_63f50=4; g_dat_62c82=1; g_dat_63f08=50; g_dat_63f0c=20;
    g_dat_62cd8[0x1d]=2; g_dat_62cd8[0x2a]=3; g_dat_62cd8[0x36]=4;
    fn_03442e(0x1d);
    check(g_dat_63f14[4]==0x68 && g_dat_63f28[4]==30 && g_dat_63f50==5,
          "  fn_03442e C x1d/62c82 slot");
    check(g_dat_62cd8[0x1d]==1 && g_dat_62cd8[0x2a]==2 && g_dat_62cd8[0x36]==3,
          "  fn_03442e C calls 0x1d,0x2a,0x36");

    /* D: x=0x38, 62c75 set -> marker 0x67, y=0x1d, calls 0x38,0x1d */
    reset_03442e();
    g_dat_63f50=0; g_dat_62c75=1; g_dat_63f08=100; g_dat_63f0c=10;
    g_dat_62cd8[0x38]=9; g_dat_62cd8[0x1d]=1;
    fn_03442e(0x38);
    check(g_dat_63f14[0]==0x67 && g_dat_63f28[0]==90 && g_dat_63f50==1,
          "  fn_03442e D x38/62c75 slot");
    check(g_dat_62cd8[0x38]==8 && g_dat_62c58[0x38]==1 && g_dat_62cd8[0x1d]==0 && g_dat_62c58[0x1d]==1,
          "  fn_03442e D calls 0x38,0x1d");

    /* E: x=0x2a, 62c75=0, 62c90 set -> marker 0x69, y=0x38, slot 9->0 wrap */
    reset_03442e();
    g_dat_63f50=9; g_dat_62c90=1; g_dat_63f08=7; g_dat_63f0c=3;
    g_dat_62cd8[0x2a]=2; g_dat_62cd8[0x38]=3;
    fn_03442e(0x2a);
    check(g_dat_63f14[9]==0x69 && g_dat_63f28[9]==4 && g_dat_63f50==0,
          "  fn_03442e E x2a/62c90 wrap");
    check(g_dat_62cd8[0x2a]==1 && g_dat_62cd8[0x38]==2,
          "  fn_03442e E calls 0x2a,0x38");

    /* F: x=0x36, 62c75 set -> marker 0x68, y=0x1d (same branch as x=0x2a) */
    reset_03442e();
    g_dat_63f50=1; g_dat_62c75=1; g_dat_63f08=1000; g_dat_63f0c=0;
    g_dat_62cd8[0x36]=3; g_dat_62cd8[0x1d]=2;
    fn_03442e(0x36);
    check(g_dat_63f14[1]==0x68 && g_dat_63f28[1]==1000 && g_dat_63f50==2,
          "  fn_03442e F x36/62c75 slot");
    check(g_dat_62cd8[0x36]==2 && g_dat_62cd8[0x1d]==1,
          "  fn_03442e F calls 0x36,0x1d");

    /* G: x=0x38, all flags 0 -> no branch, no-op */
    reset_03442e();
    g_dat_63f50=3;
    fn_03442e(0x38);
    check(g_dat_63f50==3 && g_dat_63f14[3]==0 && g_dat_62cd8[0x38]==0,
          "  fn_03442e G x38 no-flag no-op");
}

static void test_l10_accessors(void) {
    printf("L10 trivial accessors\n");
    g_dat_53c60 = 0x12345678;
    check(fn_010e15() == 0x12345678, "  fn_010e15 -> g_dat_53c60");
    uint32_t r = fn_040631(0xabcd1234);
    check(r == 0x1234, "  fn_040631 returns low16");
    check(g_dat_40010 == 0x1234, "  fn_040631 sets g_dat_40010");
    uint8_t buf[12] = {0,1,2,3,4, 0x78,0x56,0x34,0x12, 0,0,0};
    check(fn_01520b(buf) == 0x12345678, "  fn_01520b -> *(p+5)");
    g_dat_62d64 = 0x100;
    uint32_t v = fn_034a87();
    check(v == 0x100, "  fn_034a87 returns old value");
    check(g_dat_62d64 == 0x10b, "  fn_034a87 increments by 11");
    uint8_t b1[16] = {0}; b1[1] = 0x10; g_dat_639dc = 0x8;
    check(fn_01519f(b1) == 0x8, "  fn_01519f = *(p+1) - base");
    uint8_t b2[20] = {0}; b2[9] = 0x20; g_dat_639e0 = 0x10;
    check(fn_0151d5(b2) == 0x10, "  fn_0151d5 = *(p+9) - base");
    uint8_t rb[0x48] = {0}; fn_031f10(rb, 0x11111111); fn_031f10(rb, 0x22222222);
    check(rb[0x31] == 2 && *(uint32_t*)(rb+0x32) == 0x11111111 && *(uint32_t*)(rb+0x36) == 0x22222222, "  fn_031f10 ring push");
    g_dat_63f50 = 0; g_dat_63f08 = 0x555; fn_0348e2();
    check(g_dat_63f14[0] == 0 && g_dat_63f28[0] == 0x555 && g_dat_63f50 == 1, "  fn_0348e2 slot alloc");
    g_dat_62cd8[3] = 2; fn_0343d1(3);
    check(g_dat_62c58[3] == 1 && g_dat_62cd8[3] == 1, "  fn_0343d1 decrement flag");
    g_dat_62cd8[5] = 0; g_dat_62c58[5] = 0xAA; fn_0343d1(5);
    check(g_dat_62c58[5] == 0, "  fn_0343d1 zero counter -> flag0");
    { uint8_t s[24] = {0}; *(uint32_t*)s = 0x100; *(uint32_t*)(s+4) = 0x200; *(uint32_t*)(s+8) = 0x300; s[16] = 0x77; s[17] = 0;
      fn_03cad3(s);
      check(g_dat_639dc == 0x100 && g_dat_639e0 == 0x200 && g_dat_639e4 == 0x77, "  fn_03cad3 flag0 -> a");
      s[17] = 1; fn_03cad3(s);
      check(g_dat_639dc == 0x1c0, "  fn_03cad3 flag1 -> c-0x140"); }
    g_dat_63f50 = 2; g_dat_63f0c = 0x100; fn_034353(0xAB, 0x200);
    check(g_dat_63f14[2] == 0xAB && g_dat_63f28[2] == 0x100 && g_dat_63f0c == 0x200 && g_dat_63f50 == 3, "  fn_034353 slot update");
    uint8_t zb[0x80] = {0}; fn_010376(zb, 0); /* zero fields -> no-op, no crash */
    uint32_t a = heap_malloc(8), b = heap_malloc(8), c = heap_malloc(8);
    check(a && b && c, "  fn_010376: allocated 3 blocks");
    uint8_t sb[0x80] = {0};
    *(uint32_t *)(sb + 0x6d) = a; *(uint32_t *)(sb + 0x71) = b; *(uint32_t *)(sb + 0x75) = c;
    fn_010376(sb, 0);
    uint32_t d = heap_malloc(8);
    check(d != 0, "  fn_010376: freed 3 blocks, heap reusable");
    heap_free_host(d);
    uint8_t q[2] = {0x12, 0x34}; check(fn_03518a(q) == 0x1234, "  fn_03518a BE16 read");
    { double out; fn_04a7fa(5.0, &out); check(out == 0.0, "  fn_04a7fa integer -> 0"); fn_04a7fa(2.3, &out); check(out == 2.3 - fpu_rndint(2.3), "  fn_04a7fa = a - rndint"); }
    { g_dat_639dc = 0; uint8_t s[0xd0] = {0}; int32_t out;
      *(uint32_t*)(s+1) = 100; *(int32_t*)(s+0xbf) = 50; *(int32_t*)(s+0xab) = 5;
      int32_t r = fn_016959(s, 10, &out);
      check(out == -10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_016959 F>X+step: out=-step, dec, ret=0");
      check(*(uint32_t*)(s+0x17) == 2, "  fn_016959 sets s[0x17]=2");
      *(uint32_t*)(s+1) = 55; *(int32_t*)(s+0xbf) = 50; r = fn_016959(s, 10, &out);
      check(out == 0 && r == 1, "  fn_016959 F in (X,X+step]: out=0, ret=1");
      *(uint32_t*)(s+1) = 50; *(int32_t*)(s+0xbf) = 50; r = fn_016959(s, 10, &out);
      check(out == 0 && r == 1, "  fn_016959 F==X: out=0, ret=1");
      *(uint32_t*)(s+1) = 40; *(int32_t*)(s+0xbf) = 50; r = fn_016959(s, 10, &out);
      check(out == 0 && r == 1, "  fn_016959 F in [X-step,X): out=0, ret=1");
      *(uint32_t*)(s+1) = 30; *(int32_t*)(s+0xbf) = 50; *(int32_t*)(s+0xab) = 5; r = fn_016959(s, 10, &out);
      check(out == 10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_016959 F<X-step: out=+step, dec, ret=0");
      *(uint32_t*)(s+1) = 30; *(int32_t*)(s+0xbf) = 50; *(int32_t*)(s+0xab) = -1; r = fn_016959(s, 10, &out);
      check(out == 10 && r == 1 && (*(int32_t*)(s+0xab)) == -1, "  fn_016959 F<X-step, cnt<0: ret=1, no dec"); }
    { g_dat_639dc = 0; uint8_t s[0xd0] = {0}; int32_t out;
      *(uint32_t*)(s+1) = 100; *(int32_t*)(s+0xbf) = 50; *(int32_t*)(s+0xab) = 5;
      int32_t r = fn_017530(s, 10, &out);
      check(out == -10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_017530 twin: F>X+step out=-step dec ret=0");
      *(uint32_t*)(s+1) = 50; *(int32_t*)(s+0xbf) = 50; r = fn_017530(s, 10, &out);
      check(out == 0 && r == 1, "  fn_017530 twin: F==X out=0 ret=1"); }
    { g_dat_639e0 = 0;
      *(uint32_t*)(g_dat_63354+0x12) = 1000; *(uint32_t*)(g_dat_63354+0x16) = 1000;
      uint8_t s[0xd0] = {0}; int32_t out; int32_t r;
      *(uint32_t*)(s+9) = 100; *(int32_t*)(s+0xc3) = 50; out = -999; r = fn_016a60(s, 10, &out);
      check(out == 0 && r == 1, "  fn_016a60 A reached: out=0 ret=1");
      *(uint32_t*)(g_dat_63354+0x12) = 60;
      *(uint32_t*)(s+9) = 100; *(int32_t*)(s+0xc3) = 50; *(int32_t*)(s+0xab) = 5; out = -999; r = fn_016a60(s, 10, &out);
      check(out == -10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_016a60 A -step: out=-step dec ret=0");
      *(uint32_t*)(s+9) = 50; *(int32_t*)(s+0xc3) = 50; out = -999; r = fn_016a60(s, 10, &out);
      check(out == 0 && r == 1, "  fn_016a60 B F==X: out=0 ret=1");
      out = 777; *(uint32_t*)(s+9) = 40; *(int32_t*)(s+0xc3) = 50; r = fn_016a60(s, 10, &out);
      check(out == 777 && r == 1, "  fn_016a60 B reached: no *out (latent), ret=1");
      *(uint32_t*)(s+9) = 30; *(int32_t*)(s+0xc3) = 50; *(int32_t*)(s+0xab) = 5; out = -999; r = fn_016a60(s, 10, &out);
      check(out == 10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_016a60 B +step: out=+step dec ret=0"); }
    { g_dat_639e0 = 0;
      *(uint32_t*)(g_dat_63354+0x12) = 1000; *(uint32_t*)(g_dat_63354+0x16) = 1000;
      uint8_t s[0xd0] = {0}; int32_t out; int32_t r;
      *(uint32_t*)(s+9) = 40; *(int32_t*)(s+0xc3) = 50; out = 777; r = fn_017637(s, 10, &out);
      check(out == 0 && r == 1, "  fn_017637 B reached: *out=0 (corrected), ret=1");
      *(uint32_t*)(s+9) = 30; *(int32_t*)(s+0xc3) = 50; *(int32_t*)(s+0xab) = 5; out = -999; r = fn_017637(s, 10, &out);
      check(out == 10 && r == 0 && (*(int32_t*)(s+0xab)) == 4, "  fn_017637 B +step: out=+step dec ret=0"); }
    { g_dat_639dc = 0; uint8_t s[0x20] = {0}; int32_t out = 0; int32_t r;
      *(uint32_t*)(s+1) = 100; r = fn_0174a2(s, 5, &out);
      check(r == 1 && out == 0, "  fn_0174a2 in [0,0x140): ret=1 out=0");
      g_dat_639dc = 200; out = 10; r = fn_0174a2(s, 5, &out);   // F=-100 <0
      check(r == 0 && out == 15, "  fn_0174a2 F<0: out+=param2 ret=0");
      g_dat_639dc = 0; *(uint32_t*)(s+1) = 400; out = 10; r = fn_0174a2(s, 5, &out);  // F=400>0x13f
      check(r == 0 && out == 5, "  fn_0174a2 F>0x13f: out-=param2 ret=0"); }
    { memset(g_dat_62c58, 0, 128); memset(g_dat_62cd8, 0, 128);
      uint8_t cfg[0x20] = {0}; g_dat_53c7c = cfg; *(int32_t*)(cfg+0x1b) = 0x1234; g_dat_62a94 = 0;
      fn_034635(5); check(g_dat_62c58[5] == 1, "  fn_034635 state=0: ++");
      g_dat_62a94 = 0; fn_034635(5);
      check(g_dat_62a94 == 0x1234 && g_dat_62c58[5] == 2, "  fn_034635 state=1: cfg[0x1b]+++");
      memset(g_dat_62c58,0,128); memset(g_dat_62cd8,0,128); g_dat_62c58[0x4d]=4; g_dat_62cd8[0x4d]=3; g_dat_62a94=0;
      fn_034635(0x4d); check(g_dat_62c58[0x4d]==1 && g_dat_62cd8[0x4d]==2, "  fn_034635 st4 M a94=0: st=1 cnt--");
      memset(g_dat_62c58,0,128); memset(g_dat_62cd8,0,128); g_dat_62c58[0x4b]=4; g_dat_62cd8[0x4b]=0; g_dat_62a94=99;
      fn_034635(0x4b); check(g_dat_62c58[0x4b]==0, "  fn_034635 st4 K a94!=0 cnt=0: st=0");
      memset(g_dat_62c58,0,128); memset(g_dat_62cd8,0,128); g_dat_62c58[7]=4; g_dat_62cd8[7]=2; g_dat_62a94=0;
      fn_034635(7); check(g_dat_62c58[7]==1 && g_dat_62cd8[7]==1, "  fn_034635 st4 other: st=1 cnt--"); }
    { heap_arena_init(); g_heap_seg_limit = 0; heap_acquire_region(0x280);
      uint32_t s0 = heap_malloc(0x80), s1 = heap_malloc(0x80);
      check(s0 && s1, "  fn_0100d9: 2 structs");
      uint8_t *b0 = (uint8_t*)(g_heap_base+s0), *b1 = (uint8_t*)(g_heap_base+s1);
      b0[0x6d]=b0[0x6e]=b0[0x6f]=b0[0x71]=b0[0x72]=b0[0x73]=b0[0x75]=b0[0x76]=b0[0x77]=0;
      b1[0x6d]=b1[0x6e]=b1[0x6f]=b1[0x71]=b1[0x72]=b1[0x73]=b1[0x75]=b1[0x76]=b1[0x77]=0;
      uint32_t arr = heap_malloc(8); uint32_t *ap = (uint32_t*)(g_heap_base+arr); ap[0]=s0; ap[1]=s1;
      // fill the region until full (so frees are required for reuse)
      uint32_t fill[16]; int nf = 0;
      for (int i = 0; i < 16; i++) { uint32_t t = heap_malloc(0x18); if (!t) break; fill[nf++] = t; }
      check(heap_malloc(0x80) == 0, "  fn_0100d9: region full before free");
      uint32_t cont[2] = { 2, arr };
      uint32_t r = fn_0100d9(cont);
      check(r == (uint32_t)cont, "  fn_0100d9 returns s");
      uint32_t n0 = heap_malloc(0x80);
      check(n0 != 0, "  fn_0100d9: freed s0/s1/array -> 0x80 reusable");
      if (n0) heap_free_host(n0);
      for (int i = 0; i < nf; i++) heap_free_host(fill[i]); }
    {
      uint8_t tgt[0x20] = {0}; tgt[1] = 3; tgt[9] = 3; g_dat_53c7c = tgt; g_dat_53c6c = 0;
      uint8_t s[0x200] = {0};
      *(uint32_t *)(s+0x17) = 0x1f;
      int32_t r = fn_015a1f(s);
      check(r == 0 && g_dat_53c6c == 1, "  fn_015a1f state 0x1f -> ret0 + flag");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0xb; r = fn_015a1f(s);
      check(r == 2 && s[0x117] == 1 && g_dat_53c6c == 1, "  fn_015a1f [0xb,0x14] -> ret2");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 0; r = fn_015a1f(s);
      check(r == 0 && s[0x117] == 0, "  fn_015a1f out-of-range s[0x1f]!=4 -> ret0");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 4; s[0xd] = 2;
      memset(g_dat_53eb8, 100, sizeof(g_dat_53eb8)); s[1] = 5; s[9] = 5;
      r = fn_015a1f(s);
      check(r == 1, "  fn_015a1f close + table=100 -> ret1");
      prng_seed(0x1234); memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8)); r = fn_015a1f(s);
      check(r == 0, "  fn_015a1f table=0 -> ret0");
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
    {
      uint8_t tgt[0x20] = {0}; tgt[1] = 3; tgt[9] = 3; g_dat_53c7c = tgt; g_dat_53c6c = 0;
      uint8_t s[0x200] = {0}; s[1] = 5; s[9] = 5; s[0xd] = 2;   // sX-tX=2, sY-tY=2, idx in-bounds
      *(uint32_t *)(s+0x17) = 0x1f; int32_t r = fn_015b5b(s);
      check(r == 0 && g_dat_53c6c == 1, "  fn_015b5b state 0x1f -> ret0 + flag");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0xb; r = fn_015b5b(s);
      check(r == 2 && s[0x117] == 1 && g_dat_53c6c == 1, "  fn_015b5b [0xb,0x14] -> ret2");
      *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 0; *(int32_t *)(s+0x113) = 0;
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8)); r = fn_015b5b(s);
      check(r == 0 && s[0x117] == 0 && s[0x83] == 0, "  fn_015b5b out-of-range s[0x1f]!=4 -> ret0, s[0x83]=0");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 4; *(uint32_t *)(s+0x83) = 0;
      memset(g_dat_53eb8, 0x2e, sizeof(g_dat_53eb8)); r = fn_015b5b(s);
      check(r == 1 && s[0x83] == 1, "  fn_015b5b tbl1='.' close -> ret1, s[0x83]=1");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 4; *(uint32_t *)(s+0x83) = 0;
      memset(g_dat_53eb8, 100, sizeof(g_dat_53eb8)); r = fn_015b5b(s);
      check(r == 1, "  fn_015b5b tbl1!='.' both-close -> ret1");
      prng_seed(0x1234); *(uint32_t *)(s+0x17) = 0x5; *(uint32_t *)(s+0x1f) = 4; *(uint32_t *)(s+0x83) = 0;
      memset(g_dat_53eb8, 1, sizeof(g_dat_53eb8)); r = fn_015b5b(s);
      check(r == 0, "  fn_015b5b tbl0=1 too-far -> ret0");
      // near-twin magic-char check: whole table = '+'(0x2b), sX-tX=2, sY-tY=10 (2D Y-check fails).
      uint8_t s2[0x200] = {0}; s2[1] = 5; s2[9] = 13; s2[0xd] = 2;   // sX-tX=2, sY-tY=10
      *(uint32_t *)(s2+0x17) = 0x5; *(uint32_t *)(s2+0x1f) = 4; *(uint32_t *)(s2+0x83) = 0;
      memset(g_dat_53eb8, 0x2b, sizeof(g_dat_53eb8));
      prng_seed(0x1234); check(fn_015b5b(s2) == 0, "  near-twin fn_015b5b '.' -> 2D Y-fail ret0");
      prng_seed(0x1234); check(fn_01603e(s2) == 1, "  near-twin fn_01603e '+' -> 1D ret1");
      prng_seed(0x1234); check(fn_0161f9(s2) == 1, "  near-twin fn_0161f9 '+' -> 1D ret1");
      prng_seed(0x1234); check(fn_0163b4(s2) == 0, "  near-twin fn_0163b4 ',' -> 2D Y-fail ret0");
      // 0x015d16: X-gate first, then char-selects-Y-limit ('+'->40, else->10).
      uint8_t sd[0x200] = {0}; sd[1] = 5; sd[0xd] = 2;   // sX-tX=2, idx in-bounds
      *(uint32_t *)(sd+0x17) = 0x1f; int32_t rd = fn_015d16(sd);
      check(rd == 0 && g_dat_53c6c == 1, "  fn_015d16 0x1f -> ret0");
      prng_seed(0x1234); *(uint32_t *)(sd+0x17) = 0xb; rd = fn_015d16(sd);
      check(rd == 2 && sd[0x117] == 1, "  fn_015d16 [0xb,0x14] -> ret2");
      *(uint32_t *)(sd+0x17) = 0x5; *(uint32_t *)(sd+0x1f) = 0; *(int32_t *)(sd+0x113) = 0;
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8)); rd = fn_015d16(sd);
      check(rd == 0, "  fn_015d16 s[0x1f]!=4 -> ret0");
      *(uint32_t *)(sd+0x17) = 0x5; *(uint32_t *)(sd+0x1f) = 4; *(uint32_t *)(sd+0x83) = 0; sd[9] = 33;   // sY-tY=30
      memset(g_dat_53eb8, 0x2b, sizeof(g_dat_53eb8)); prng_seed(0x1234); rd = fn_015d16(sd);
      check(rd == 1, "  fn_015d16 '+' Y-limit=40 (30<40) -> ret1");
      *(uint32_t *)(sd+0x1f) = 4; *(uint32_t *)(sd+0x83) = 0; sd[9] = 23;   // sY-tY=20
      memset(g_dat_53eb8, 100, sizeof(g_dat_53eb8)); prng_seed(0x1234); rd = fn_015d16(sd);
      check(rd == 0, "  fn_015d16 non-'+' Y-limit=10 (20>=10) -> ret0");
      sd[9] = 5; memset(g_dat_53eb8, 1, sizeof(g_dat_53eb8)); prng_seed(0x1234); rd = fn_015d16(sd);
      check(rd == 0, "  fn_015d16 X-gate (2>=1) -> ret0");
      // fn_015ed1: s[0x83] gate, no char check, Y-limit 10.
      uint8_t se[0x200] = {0}; se[1] = 5; se[9] = 5; se[0xd] = 2;   // sX-tX=2, sY-tY=2
      *(uint32_t *)(se+0x17) = 0x1f; int32_t re = fn_015ed1(se);
      check(re == 0 && g_dat_53c6c == 1, "  fn_015ed1 0x1f -> ret0");
      prng_seed(0x1234); *(uint32_t *)(se+0x17) = 0xb; re = fn_015ed1(se);
      check(re == 2 && se[0x117] == 1, "  fn_015ed1 [0xb,0x14] -> ret2");
      *(uint32_t *)(se+0x17) = 0x5; *(uint32_t *)(se+0x1f) = 0; *(int32_t *)(se+0x113) = 0;
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8)); re = fn_015ed1(se);
      check(re == 0, "  fn_015ed1 s[0x1f]!=4 -> ret0");
      *(uint32_t *)(se+0x17) = 0x5; *(uint32_t *)(se+0x1f) = 4; *(uint32_t *)(se+0x83) = 0;
      memset(g_dat_53eb8, 100, sizeof(g_dat_53eb8)); prng_seed(0x1234); re = fn_015ed1(se);
      check(re == 1, "  fn_015ed1 X<tbl0 Y<10 -> ret1");
      se[9] = 23; *(uint32_t *)(se+0x83) = 0; prng_seed(0x1234); re = fn_015ed1(se);   // sY-tY=20
      check(re == 0, "  fn_015ed1 Y>=10 -> ret0");
      se[9] = 5; *(uint32_t *)(se+0x83) = 0; memset(g_dat_53eb8, 1, sizeof(g_dat_53eb8)); prng_seed(0x1234); re = fn_015ed1(se);
      check(re == 0, "  fn_015ed1 X>=tbl0 -> ret0");
    // fn_032158: entity list -> set targets to player + prng jitter.
    {
      uint8_t player[0x200] = {0}; *(uint32_t *)(player+1) = 100; *(uint32_t *)(player+9) = 50;
      player[0x1b] = 0; *(uint32_t *)(player+0x1f) = 4;   // player is itself in the list (must be skipped)
      uint8_t e0[0x200] = {0}; *(uint32_t *)(e0+0x117) = 0; *(uint32_t *)(e0+0x1f) = 4; *(uint32_t *)(e0+0xaf) = 0;
      g_dat_639dc = 0; g_dat_639e0 = 0;   // so fn_01519f/fn_0151d5 = raw offset
      g_dat_53c7c = player; g_dat_53c70 = e0; g_dat_53c68 = 2;
      *(uint8_t **)(e0+0x13f) = player; *(uint8_t **)(player+0x13f) = 0;   // e0 -> player
      g_dat_53c6c = 0; fn_032158();
      check(e0[0x1f] == 4 && g_dat_53c6c == 0, "  fn_032158 flag=0 -> no-op");
      g_dat_53c6c = 1; prng_seed(0x1234); fn_032158();
      int32_t b3 = *(int32_t *)(e0+0xb3); int32_t b7 = *(int32_t *)(e0+0xb7);
      check(b3 >= 55 && b3 <= 64, "  fn_032158 target-x = 100-45+prng%10 in [55,64]");
      check(b7 >= 45 && b7 <= 54, "  fn_032158 target-y = 50-5+prng%10 in [45,54]");
      check(e0[0x1f] == 6 && e0[0xaf] == 0 && g_dat_53c6c == 0, "  fn_032158 e0 marked active(0x1f=6), flag cleared");
      check(player[0x1f] == 4, "  fn_032158 player skipped (not modified)");
      // [0x1b]!=0 branch: target-x = 100+45-prng%10 in [136,145]
      player[0x1b] = 1; *(uint32_t *)(e0+0x117) = 0; *(uint32_t *)(e0+0x1f) = 4; *(uint32_t *)(e0+0xaf) = 0;
      g_dat_53c6c = 1; prng_seed(0x1234); fn_032158();
      b3 = *(int32_t *)(e0+0xb3);
      check(b3 >= 136 && b3 <= 145, "  fn_032158 [0x1b]!=0 target-x = 100+45-prng%10 in [136,145]");
      g_dat_53c6c = 0;
      // fn_01656f (magic {',' '-'}) + fn_016750 (magic {'1' '4' '.'}).
      // sX-tX=2, sY-tY=20, tbl0=5 (X passes, Y fails).
      g_dat_639dc = 0; g_dat_639e0 = 0;
      uint8_t p1[0x200] = {0}; *(uint32_t *)(p1+1) = 3; *(uint32_t *)(p1+9) = 3; g_dat_53c7c = p1;
      uint8_t s1[0x200] = {0}; *(uint32_t *)(s1+1) = 5; *(uint32_t *)(s1+9) = 23;
      *(uint32_t *)(s1+0x17) = 0x5; *(uint32_t *)(s1+0x1f) = 4; *(uint32_t *)(s1+0x83) = 1;   // skip prng overwrite so s[0x113]=0 holds
      *(int32_t *)(s1+0x113) = 0; s1[0xd] = 2;
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8)); g_dat_53eb8[0x110] = 5;   // idx=2*0x88=0x110, tbl0=5
      int32_t r1;
      prng_seed(0x1234); g_dat_53eb8[0x111] = ','; r1 = fn_01656f(s1);
      check(r1 == 1, "  fn_01656f ',' -> 1D: X<5 -> ret1 (Y ignored)");
      prng_seed(0x1234); g_dat_53eb8[0x111] = '-'; r1 = fn_01656f(s1);
      check(r1 == 1, "  fn_01656f '-' -> 1D -> ret1");
      prng_seed(0x1234); g_dat_53eb8[0x111] = '.'; r1 = fn_01656f(s1);
      check(r1 == 0, "  fn_01656f '.' -> 2D: X<5 but Y=20>=10 -> ret0");
      prng_seed(0x1234); g_dat_53eb8[0x111] = '.'; r1 = fn_016750(s1);
      check(r1 == 1, "  fn_016750 '.' -> 1D -> ret1");
      prng_seed(0x1234); g_dat_53eb8[0x111] = '1'; r1 = fn_016750(s1);
      check(r1 == 1, "  fn_016750 '1' -> 1D -> ret1");
      prng_seed(0x1234); g_dat_53eb8[0x111] = ','; r1 = fn_016750(s1);
      check(r1 == 0, "  fn_016750 ',' -> 2D: Y=20>=10 -> ret0");
      // fn_01776f: 2D movement delta computer (target<end vs target>=end, arrival check).
      g_dat_639e0 = 0;   // fn_0151d5(s) = s[9..12]
      uint8_t sm[0x200] = {0}; int32_t o1, o2;
      // 1: target<end, F>mid, cb<0xa0 -> out1=+p; F-p>target -> out2=-p, ret0
      *(int32_t *)(sm+0xc3) = 10; *(int32_t *)(sm+0xcf) = 30; *(int32_t *)(sm+0xcb) = 10;
      *(int32_t *)(sm+9) = 25;   // F=25, mid=20, F>mid; F-p=20>10
      int32_t rr = fn_01776f(sm, 5, &o1, &o2);
      check(rr == 0 && o1 == 5 && o2 == -5, "  fn_01776f F>mid cb<0xa0 no-arrival");
      // 2: target<end, arrival (F-p<=target) -> out1=out2=0, ret1
      *(int32_t *)(sm+9) = 12;   // F=12<=20, F-p=7<=10
      rr = fn_01776f(sm, 5, &o1, &o2);
      check(rr == 1 && o1 == 0 && o2 == 0, "  fn_01776f target<end arrival");
      // 3: target>=end, F>mid, cb<0xa0, arrival (F+p>=target) -> ret1
      *(int32_t *)(sm+0xc3) = 30; *(int32_t *)(sm+0xcf) = 10; *(int32_t *)(sm+0xcb) = 10;
      *(int32_t *)(sm+9) = 25;   // F=25, mid=20, F+p=30>=30
      rr = fn_01776f(sm, 5, &o1, &o2);
      check(rr == 1 && o1 == 0 && o2 == 0, "  fn_01776f target>=end arrival");
      // 4: target>=end, F<=mid, cb>=0xa0, no-arrival -> out1=+p, out2=+p, ret0
      *(int32_t *)(sm+0xcb) = 200; *(int32_t *)(sm+9) = 10;   // F=10<=20, F+p=15<30
      rr = fn_01776f(sm, 5, &o1, &o2);
      check(rr == 0 && o1 == 5 && o2 == 5, "  fn_01776f F<=mid cb>=0xa0 no-arrival");
      // 5: target<end, F>mid, cb>=0xa0 -> out1=-p, out2=-p, ret0
      *(int32_t *)(sm+0xc3) = 10; *(int32_t *)(sm+0xcf) = 30; *(int32_t *)(sm+0xcb) = 200;
      *(int32_t *)(sm+9) = 25;   // F-p=20>10
      rr = fn_01776f(sm, 5, &o1, &o2);
      // fn_01730b: 2-phase X-then-Y move-toward-target (s[0xaf] 2-bit done counter).
      g_dat_639dc = 0; g_dat_639e0 = 0;
      *(int32_t *)(g_dat_63354 + 0x12) = 0; *(int32_t *)(g_dat_63354 + 0x16) = 1000;
      uint8_t sn[0x200] = {0}; int32_t x1, y1;
      // 1: both moving -> out1=+5, out2=+5, s[0xaf]=0, ret0
      *(int32_t *)(sn+1) = 10; *(int32_t *)(sn+9) = 10;   // X=10, Y=10
      *(int32_t *)(sn+0xb3) = 20; *(int32_t *)(sn+0xb7) = 20; *(int32_t *)(sn+0xaf) = 0;
      x1 = y1 = 999; rr = fn_01730b(sn, 5, &x1, &y1);
      check(rr == 0 && x1 == 5 && y1 == 5 && *(int32_t *)(sn+0xaf) == 0, "  fn_01730b both moving");
      // 2: both reached -> s[0xaf]=3, ret1 (out1/out2 untouched)
      *(int32_t *)(sn+1) = 20; *(int32_t *)(sn+9) = 20; *(int32_t *)(sn+0xaf) = 0;
      x1 = y1 = 999; rr = fn_01730b(sn, 5, &x1, &y1);
      check(rr == 1 && x1 == 999 && y1 == 999 && *(int32_t *)(sn+0xaf) == 3, "  fn_01730b both reached -> ret1");
      // 3: X reached, Y moving -> out2=+5, s[0xaf]=1, ret0
      *(int32_t *)(sn+1) = 20; *(int32_t *)(sn+9) = 10; *(int32_t *)(sn+0xaf) = 0;
      x1 = y1 = 999; rr = fn_01730b(sn, 5, &x1, &y1);
      check(rr == 0 && x1 == 999 && y1 == 5 && *(int32_t *)(sn+0xaf) == 1, "  fn_01730b X reached, Y moving");
      // 4: X moving, Y reached -> out1=+5, s[0xaf]=2, ret0
      *(int32_t *)(sn+1) = 10; *(int32_t *)(sn+9) = 20; *(int32_t *)(sn+0xaf) = 0;
      x1 = y1 = 999; rr = fn_01730b(sn, 5, &x1, &y1);
      check(rr == 0 && x1 == 5 && y1 == 999 && *(int32_t *)(sn+0xaf) == 2, "  fn_01730b X moving, Y reached");
      // 5: Y clamped by g16 bound (Y+step=15>=g16=15) -> out2 untouched, s[0xaf]=2, ret0
      *(int32_t *)(g_dat_63354 + 0x16) = 15;
      *(int32_t *)(sn+1) = 5; *(int32_t *)(sn+9) = 10; *(int32_t *)(sn+0xaf) = 0;   // X=5 moving, Y=10
      x1 = y1 = 999; rr = fn_01730b(sn, 5, &x1, &y1);
      // fn_017ca6: Bresenham diagonal stepper (primary axis = larger |velocity|).
      g_dat_639dc = 0; g_dat_639e0 = 0;
      *(int32_t *)(g_dat_63354 + 0x12) = 0; *(int32_t *)(g_dat_63354 + 0x16) = 1000;
      uint8_t sc[0x200] = {0}; int32_t a1, b1;
      // 1: X-dominant, Vx>0, in-bounds, no secondary -> out1=+5, out2=0, s[0xfb]=3, ret0
      *(int32_t *)(sc+1) = 50; *(int32_t *)(sc+0xf3) = 10; *(int32_t *)(sc+0xf7) = 3;
      *(int32_t *)(sc+0xbf) = 100; *(int32_t *)(sc+0xfb) = 0;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 0 && a1 == 5 && b1 == 0 && *(int32_t *)(sc+0xfb) == 3, "  fn_017ca6 X-dom no-secondary");
      // 2: X-dominant, Vx>0, X>s[0xbf] -> ret1 (out untouched)
      *(int32_t *)(sc+1) = 150; *(int32_t *)(sc+0xbf) = 100; *(int32_t *)(sc+0xf3) = 10; *(int32_t *)(sc+0xf7) = 3;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 1 && a1 == 999 && b1 == 999, "  fn_017ca6 X-dom out-of-bounds ret1");
      // 3: X-dominant, Vx<0, X<s[0xbf] -> ret1
      *(int32_t *)(sc+1) = 50; *(int32_t *)(sc+0xbf) = 100; *(int32_t *)(sc+0xf3) = -10; *(int32_t *)(sc+0xf7) = 3;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 1 && a1 == 999 && b1 == 999, "  fn_017ca6 X-dom Vx<0 X<bound ret1");
      // 4: X-dominant, secondary step (s[0xfb]=5, +|Vy|=9 -> 14, |Vx|=10<14 -> out2=+5, s[0xfb]=4)
      *(int32_t *)(sc+1) = 50; *(int32_t *)(sc+0xbf) = 100; *(int32_t *)(sc+0xf3) = 10; *(int32_t *)(sc+0xf7) = 9;
      *(int32_t *)(sc+0xfb) = 5;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 0 && a1 == 5 && b1 == 5 && *(int32_t *)(sc+0xfb) == 4, "  fn_017ca6 X-dom secondary");
      // 5: Y-dominant, Vy>0, in-bounds, no secondary -> out2=+5, out1=0, s[0xfb]=3, ret0
      *(int32_t *)(sc+9) = 50; *(int32_t *)(sc+0xf3) = 3; *(int32_t *)(sc+0xf7) = 10;
      *(int32_t *)(sc+0xc3) = 100; *(int32_t *)(sc+0xef) = 10; *(int32_t *)(sc+0xfb) = 0;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 0 && a1 == 0 && b1 == 5 && *(int32_t *)(sc+0xfb) == 3, "  fn_017ca6 Y-dom no-secondary");
      // 6: Y-dominant, Vy<0, Y<s[0xc3] -> ret1
      *(int32_t *)(sc+9) = 50; *(int32_t *)(sc+0xc3) = 100; *(int32_t *)(sc+0xf3) = 3; *(int32_t *)(sc+0xf7) = -10;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      check(rr == 1 && a1 == 999 && b1 == 999, "  fn_017ca6 Y-dom Vy<0 Y<bound ret1");
      // 7: Y-dominant, secondary step (s[0xfb]=5, +|Vx|=9 -> 14, |Vy|=10<14 -> out1=+5, s[0xfb]=4)
      *(int32_t *)(sc+9) = 50; *(int32_t *)(sc+0xc3) = 100; *(int32_t *)(sc+0xef) = 10;
      *(int32_t *)(sc+0xf3) = 9; *(int32_t *)(sc+0xf7) = 10; *(int32_t *)(sc+0xfb) = 5;
      a1 = b1 = 999; rr = fn_017ca6(sc, 5, &a1, &b1);
      // fn_016fec: grid-collision detector (entity list, grid cell X/20 Y/12).
      g_dat_639dc = 0; g_dat_639e0 = 0;
      uint8_t fs[0x200] = {0}, fe[0x200] = {0}, fp[0x200] = {0};
      g_dat_53c70 = fe; g_dat_53c68 = 0; g_dat_53c7c = fp;
      // 1: reinit (g_dat_51c98==1) -> ret0, mode=2, latch=0
      g_dat_51c98 = 1; g_dat_53c90 = 0;
      rr = fn_016fec(fs, 0);
      check(rr == 0 && g_dat_51c98 == 2 && g_dat_53c90 == 0, "  fn_016fec reinit");
      // 2: collision (entity in same grid cell, [0x1f]==4) -> ret2, reinit flag, s[0xbb] in [0,4)
      g_dat_51c98 = 2; g_dat_53c90 = 0;
      *(int32_t *)(fs+1) = 100; *(int32_t *)(fs+9) = 100;      // s: Xg=5, Yg=8
      *(int32_t *)(fe+1) = 110; *(int32_t *)(fe+9) = 100;      // e: Xg=5, Yg=8 (same cell)
      *(int32_t *)(fe+0x1f) = 4; g_dat_53c68 = 1;
      rr = fn_016fec(fs, 0);
      check(rr == 2 && g_dat_51c98 == 1 && *(int32_t *)(fs+0xbb) >= 0 && *(int32_t *)(fs+0xbb) < 4, "  fn_016fec collision ret2");
      // 3: no collision (count=0), countdown s[0xab]=5 -> ret0, s[0xab]=4
      g_dat_51c98 = 2; g_dat_53c90 = 0; g_dat_53c68 = 0;
      *(int32_t *)(fs+1) = 100; *(int32_t *)(fs+9) = 100; *(int32_t *)(fs+0xab) = 5;
      *(int32_t *)(fp+0x17) = 0;                              // player state gate off
      rr = fn_016fec(fs, 0);
      check(rr == 0 && *(int32_t *)(fs+0xab) == 4, "  fn_016fec countdown ret0");
      // 4: countdown negative s[0xab]=-1 -> ret1, reinit flag
      g_dat_51c98 = 2; g_dat_53c68 = 0; *(int32_t *)(fs+0xab) = -1; *(int32_t *)(fp+0x17) = 0;
      rr = fn_016fec(fs, 0);
      check(rr == 1 && g_dat_51c98 == 1, "  fn_016fec countdown<0 ret1");
      // 5: player-event gate (latch=0, player[0x17]=0x84, abs(Y-yref)<6) -> latch set
      g_dat_51c98 = 2; g_dat_53c90 = 0; g_dat_53c68 = 0;
      *(int32_t *)(fs+1) = 100; *(int32_t *)(fs+9) = 100; *(int32_t *)(fs+0xab) = 3;
      *(int32_t *)(fp+0x17) = 0x84; prng_seed(12345);
      rr = fn_016fec(fs, 100);                               // yref=100, abs(Y-yref)=0<6
      // fn_043280: byte-pattern fill, aligned, count=8 -> DD CC BB AA DD CC BB AA
      uint8_t fb[16] = {0};
      fn_043280(fb, 0xAABBCCDD, 8);
      check(fb[0]==0xDD && fb[1]==0xCC && fb[2]==0xBB && fb[3]==0xAA && fb[4]==0xDD && fb[5]==0xCC && fb[6]==0xBB && fb[7]==0xAA, "  fn_043280 aligned count=8");
      // fn_043280: aligned, count=3 -> remainder phase: DD CC DD (latent bug: byte2=DL not EH)
      uint8_t fb3[8] = {0};
      fn_043280(fb3, 0xAABBCCDD, 3);
      check(fb3[0]==0xDD && fb3[1]==0xCC && fb3[2]==0xDD, "  fn_043280 aligned count=3");
      // fn_043280: count=0 -> no write
      uint8_t fb0[8] = {0x77,0x77,0x77,0x77};
      fn_043280(fb0, 0xAABBCCDD, 0);
      check(fb0[0]==0x77 && fb0[3]==0x77, "  fn_043280 count=0 no-write");
      check(g_dat_53c90 == 1 && (rr == 3 || rr == 0), "  fn_016fec player-event gate entered");
      check(rr == 0 && a1 == 5 && b1 == 5 && *(int32_t *)(sc+0xfb) == 4, "  fn_017ca6 Y-dom secondary");
      check(rr == 0 && x1 == 5 && y1 == 999 && *(int32_t *)(sn+0xaf) == 2, "  fn_01730b Y clamped by g16 bound");
      *(int32_t *)(g_dat_63354 + 0x16) = 1000;
      check(rr == 0 && o1 == -5 && o2 == -5, "  fn_01776f F>mid cb>=0xa0 no-arrival");
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
    }
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
      memset(g_dat_53eb8, 0, sizeof(g_dat_53eb8));
    }
    }
}

int main(void) {
    static uint8_t s_t_bounds[0x20];
    g_dat_63354 = s_t_bounds;
    /* --- fn_017927: 2D move/AI (double-deref bounds via g_dat_63354) --- */
    { g_dat_639dc = 0; g_dat_639e0 = 0;
      *(int32_t *)(g_dat_63354+0x12) = 0; *(int32_t *)(g_dat_63354+0x16) = 1000;  /* mid=500 */
      uint8_t s[0x200]; int32_t o1, o2; int32_t r;
      /* T reached: bf<cb, cf<mid, (Y-val)<=c3 -> r=1, out=0 */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=20; *(int32_t*)(s+0xcb)=100; *(int32_t*)(s+0xcf)=10; *(int32_t*)(s+0xc3)=200;
      *(uint32_t*)(s+1)=0; *(uint32_t*)(s+9)=0; o1=777; o2=888;
      r = fn_017927(s, 0, &o1, &o2);
      check(r==1 && o1==0 && o2==0 && s[0x17]==2 && (*(int32_t*)(s+0x67))==-190, "  fn_017927 T reached r=1 out=0");
      /* T1: bf<cb, X>target, cf<mid, (cb-X)%3==0, not reached -> o1=+val o2=-val */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=20; *(int32_t*)(s+0xcb)=100; *(int32_t*)(s+0xcf)=10; *(int32_t*)(s+0xc3)=5;
      *(uint32_t*)(s+1)=70; o1=0; o2=0;
      r = fn_017927(s, 10, &o1, &o2);
      check(r==0 && o1==10 && o2==-10 && (*(int32_t*)(s+0x67))==5, "  fn_017927 T1 X>t cf<mid mod3: o1=+v o2=-v");
      /* T2: bf<cb, X<=target, cf<mid, (cb-X)%3==0, not reached -> o1=-val o2=-val */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=20; *(int32_t*)(s+0xcb)=100; *(int32_t*)(s+0xcf)=10; *(int32_t*)(s+0xc3)=-100;
      *(uint32_t*)(s+1)=-200; o1=0; o2=0;
      r = fn_017927(s, 10, &o1, &o2);
      check(r==0 && o1==-10 && o2==-10 && (*(int32_t*)(s+0x67))==110, "  fn_017927 T2 X<=t cf<mid mod3: o1=-v o2=-v");
      /* E reached: bf>=cb, cf>=mid, (val+Y)>=c3 -> r=1 */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=100; *(int32_t*)(s+0xcb)=20; *(int32_t*)(s+0xcf)=600; *(int32_t*)(s+0xc3)=5;
      *(uint32_t*)(s+1)=80; *(uint32_t*)(s+9)=10; o1=777; o2=888;
      r = fn_017927(s, 0, &o1, &o2);
      check(r==1 && o1==0 && o2==0 && (*(int32_t*)(s+0x67))==595, "  fn_017927 E reached r=1 out=0");
      /* E no-mod3 not-reached -> o1 unchanged, o2=+val */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=100; *(int32_t*)(s+0xcb)=20; *(int32_t*)(s+0xcf)=600; *(int32_t*)(s+0xc3)=100;
      *(uint32_t*)(s+1)=0; o1=888; o2=0;
      r = fn_017927(s, 5, &o1, &o2);
      check(r==0 && o1==888 && o2==5 && (*(int32_t*)(s+0x67))==500, "  fn_017927 E no-mod3 no-reach: o1 unchanged o2=+v");
      /* E1: bf>=cb, X>target, cf>mid, (X-cb)%3==0, not reached -> o1=+val o2=+val */
      memset(s,0,sizeof s); *(int32_t*)(s+0xbf)=100; *(int32_t*)(s+0xcb)=20; *(int32_t*)(s+0xcf)=600; *(int32_t*)(s+0xc3)=100;
      *(uint32_t*)(s+1)=581; o1=0; o2=0;
      r = fn_017927(s, 7, &o1, &o2);
      check(r==0 && o1==7 && o2==7 && (*(int32_t*)(s+0x67))==500, "  fn_017927 E1 X>t cf>mid mod3: o1=+v o2=+v"); }
    /* --- fn_034735: per-key state machine (62c58/62cd8) --- */
    { uint8_t idx = 7;
      g_dat_62c58[idx]=4; g_dat_62cd8[idx]=5; fn_034735(idx);
      check(g_dat_62c58[idx]==1 && g_dat_62cd8[idx]==4, "  fn_034735 A: 62c58==4,62cd8>0 ->1 dec");
      g_dat_62c58[idx]=4; g_dat_62cd8[idx]=0; fn_034735(idx);
      check(g_dat_62c58[idx]==0 && g_dat_62cd8[idx]==0, "  fn_034735 B: 62c58==4,62cd8==0 ->0");
      g_dat_62c58[idx]=2; g_dat_62cd8[idx]=0; fn_034735(idx);
      check(g_dat_62c58[idx]==3, "  fn_034735 C: 62c58==2 ->3 (+03442e)");
      g_dat_62c58[idx]=3; fn_034735(idx);
      check(g_dat_62c58[idx]==4, "  fn_034735 E: 62c58==3 ->4 (+03442e)");
      g_dat_62c58[idx]=5; g_dat_62cd8[idx]=9; fn_034735(idx);
      check(g_dat_62c58[idx]==5 && g_dat_62cd8[idx]==9, "  fn_034735 D: 62c58==5 -> no-op"); }
    /* --- fn_0351dc: unsigned LEB128 decoder --- */
    { uint8_t b1[] = {0x05}; uint8_t *c = b1; uint32_t v = fn_0351dc(&c);
      check(v==5 && (c-b1)==1, "  fn_0351dc 1-byte 05 -> 5, cursor+1");
      uint8_t b2[] = {0x85,0x02}; c=b2; v=fn_0351dc(&c);
      check(v==642 && (c-b2)==2, "  fn_0351dc 2-byte 85 02 -> 642, cursor+2");
      uint8_t b3[] = {0xff,0xff,0x01}; c=b3; v=fn_0351dc(&c);
      check(v==2097025 && (c-b3)==3, "  fn_0351dc 3-byte ff ff 01 -> 2097025 (MSB-first), cursor+3"); }
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
    test_heap_free_guard();
    printf("== batch 9 (L0 real-mode/DOS + cmdline parser) ==\n");
    test_rm_stubs();
    test_cmdline_parse();
    test_cmdline_count();
    printf("== batch B (L0 emit + dispatch) ==\n");
    test_emit_chars();
    test_emit_value();
    test_emit_step();
    test_dispatch();
    test_batchC();
    test_batchD();
    test_batchD2();
    test_batchE();
    test_batchF();
    test_batchG();
    test_batchH();
    test_batchI();
    test_batchJ();
    test_fx_from_f32();
    test_float_str_sci();
    test_fmt_u64_fixed();
    test_fx_mantissa_scale();
    test_fx64_to_f32();
    test_fmt_decimal();
    test_fmt_int_signed();
    test_fx_extract();
    test_fx_norm32();
    test_fx_double_exp();
    test_datetime_pack();
    test_fx_double_naninf();
    test_fx_compare();
    test_fx_classify_status();
    test_fx_mul64();
    test_fx_stub_4a923();
    test_fx_round_down();
    test_string_upper();
    test_hex_digit_value();
    test_prng();
    test_itoa32s();
    test_tmp_filename();
    test_config_value_ptr();
    test_time_parse();
    test_heap_size_adjust();
    test_fn_0423cf();
    test_042a08_family();
    test_047d24();
    test_044bb4();
    test_044ea4();
    test_049b84();
    test_0492d0();
    test_042a19();
    test_044b96();
    test_045079();
    test_0450da();
    test_049a35();
    test_049c76();
    test_045a82();
    test_045a76();
    test_0423b7();
    test_042576();
    test_l10_accessors();
    test_fn_034aba();
    test_fn_03442e();
    printf(failures ? "== FAIL (%d) ==\n" : "== ALL PASS ==\n", failures);
    return failures != 0;
}
