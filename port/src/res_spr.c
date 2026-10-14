#include "res.h"
#include <stdlib.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int spr_decode(const uint8_t *d, size_t n, Spr **out, int *count) {
    if (n < 4) return -1;
    uint32_t count_v = rd32(d);
    if (count_v > 100000) return -1;
    Spr *s = calloc(count_v, sizeof *s);
    if (!s) return -1;
    size_t off = 4;
    for (uint32_t i = 0; i < count_v; i++) {
        if (off + 52 > n) { free(s); return -1; }
        for (int k = 0; k < 12; k++) s[i].hdr[k] = rd32(d + off + k * 4);
        s[i].w = s[i].hdr[0];
        s[i].h = s[i].hdr[1];
        s[i].size = rd32(d + off + 48);
        if (off + 52 + s[i].size > n) { free(s); return -1; }
        s[i].raw = d + off + 52;
        off += 52 + s[i].size;
    }
    if (off != n) { free(s); return -1; } /* exact-to-EOF invariant */
    *out = s;
    if (count) *count = (int)count_v;
    return 0;
}

void spr_free(Spr *s, size_t count) { (void)count; free(s); }

/* Render sprite row y: fill w pixels with bg, then write segments over it. */
void spr_draw_row(uint8_t *dst, int dst_w, uint8_t bg, const Spr *s, int y) {
    if (y < 0 || (size_t)y >= s->h) return;
    const uint8_t *p = s->raw;
    for (size_t r = 0; r < (size_t)y; r++) { /* skip preceding rows */
        int nseg = p[0];
        p += 1;
        for (int k = 0; k < nseg; k++) p += 2 + p[1];
    }
    int nseg = p[0];
    p += 1;
    int x = 0;
    for (int k = 0; k < nseg; k++) {
        int skip = p[0];
        int len = p[1];
        p += 2;
        x += skip;
        for (int i = 0; i < len; i++) {
            if (x + i < dst_w) dst[x + i] = p[i];
        }
        x += len;
        p += len; /* advance past this segment's pixels */
    }
    (void)bg; /* callers fill dst with bg first */
}
