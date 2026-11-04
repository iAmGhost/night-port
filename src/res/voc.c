#include "res.h"
#include <stdlib.h>
#include <string.h>

int voc_decode(const uint8_t *d, size_t n, Voc *v) {
    memset(v, 0, sizeof *v);
    if (n < 26) return -1;
    if (memcmp(d, "Creative Voice File", 19) != 0) return -1;

    size_t cap = 0, len = 0;
    size_t o = 26;
    while (o + 4 <= n) {
        uint8_t type = d[o];
        uint32_t sz = (uint32_t)d[o + 1] | ((uint32_t)d[o + 2] << 8) | ((uint32_t)d[o + 3] << 16);
        o += 4;
        if (o + sz > n) return -1;
        if (type == 0x00) break; /* stop tag, end of stream */
        if (type == 0x01) {      /* voice: [handler][rate_idx][pcm...] */
            if (sz < 2) return -1;
            uint32_t m = (uint32_t)sz - 2;
            if (len + m > cap) {
                cap = (len + m) * 2 + 64;
                uint8_t *np = realloc(v->pcm, cap);
                if (!np) { free(v->pcm); return -1; }
                v->pcm = np;
            }
            memcpy(v->pcm + len, d + o + 2, m);
            len += m;
            v->n_voice_blocks++;
        }
        o += sz; /* 0x02 retrigger / 0x08 reserved / others: skipped */
    }
    v->n_samples = (int)len;
    return 0;
}

void voc_free(Voc *v) { free(v->pcm); memset(v, 0, sizeof *v); }
