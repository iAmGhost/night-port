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
    printf(failures ? "== FAIL (%d) ==\n" : "== ALL PASS ==\n", failures);
    return failures != 0;
}
