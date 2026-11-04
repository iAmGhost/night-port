#include "res.h"
#include <stdlib.h>
#include <string.h>

/* Standard PCX header fields: xmax u16 @8, ymax u16 @10 (inclusive). */
int pcx_decode(const uint8_t *d, size_t n, Pcx *p) {
    memset(p, 0, sizeof *p);
    if (n < 128 + 768 || d[0] != 0x0A) return -1;
    int xmax = d[8] | (d[9] << 8);
    int ymax = d[10] | (d[11] << 8);
    int w = xmax + 1, h = ymax + 1;
    if (w <= 0 || h <= 0) return -1;

    size_t need = (size_t)w * h;
    uint8_t *idx = malloc(need);
    if (!idx) return -1;
    size_t o = 128, produced = 0;
    while (o < n - 768 && produced < need) {
        uint8_t b = d[o++];
        if (b < 0xC0) {
            idx[produced++] = b;
        } else {
            if (o >= n - 768) break;
            int cnt = b & 0x3F;
            uint8_t v = d[o++];
            for (int i = 0; i < cnt && produced < need; i++) idx[produced++] = v;
        }
    }
    if (produced != need) { free(idx); return -1; }

    /* Display palette = RAW file bytes (RGB order, no shift). Verified
     * against a neutral-profile DOSBox capture of the original menu
     * (analysis/refs/neu_menu2.png): the original DAC palette is
     * full 0-255 range and screenshot pixels match file bytes within ~20.
     * The loader's `SHR byte, 2` (FUN_0003628e @0x36314) applies to the
     * working buffer DAT_000636b8 (software shade/dither math); the DAC
     * receives the raw palette via DAT_000633b8 (uploaded by FUN_00038a20,
     * FUN_0003a829, FUN_00038445). See docs/03-formats.md §1. */
    memcpy(p->pal, d + n - 768, 768);
    p->w = w; p->h = h; p->idx = idx;
    return 0;
}

void pcx_free(Pcx *p) { free(p->idx); p->idx = NULL; }
