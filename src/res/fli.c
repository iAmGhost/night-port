/* fli.c — decoder for NIGHT's .fli video format (docs/03-formats.md §11)
 *
 * Layout (verified byte-exact on opening.fli, 1168 frames, all 688 patch
 * frames consume their payloads exactly):
 *
 * [128B header]
 *   +0   u32 ?                       (0x00996ECC in opening.fli)
 *   +4   u16 marker = 0xAF11
 *   +6   u16 frame_count             (1168)
 *   +8   u16 width                   (320)
 *   +10  u16 height                  (200)
 *
 * Then a stream of 0xF1FA chunks; the FIRST is the setup chunk, the rest
 * are the frame_count frames:
 *
 *   0xF1FA chunk: [u32 size (total incl 16B hdr)][u16 0xF1FA][u16 count][8B]
 *     count x sub-chunks
 *   hold frame: size == 16, count == 0 (screen unchanged)
 *
 *   sub-chunk: [u32 size (total incl 6B hdr)][u16 type][payload]
 *     0x04: [4B][768B RGB palette]; display palette = RAW file bytes
 *            (the loader's >>2 applies only to the DAT_000636b8 working
 *             buffer; the DAC receives the raw palette - see docs/03 §11)
 *     0x0C: pixel patch  [u16 start_row][u16 n_rows][n_rows rows]
 *            row: [u8 nseg][seg x nseg], written to fb + (start_row+r)*w
 *            seg: [u8 skip][u8 len][data]
 *               len <  0x80: literal, len px bytes   (consumes 2+len)
 *               len >= 0x80: run, (256-len) copies of next byte (consumes 3)
 *            row output may be < w (partial row patch)
 *     0x0F: full frame (200 rows x 320):
 *            row: [u8 nseg][seg x nseg]
 *            seg: [b0][b1][px...]
 *               b0 <  0x80: b0 copies of byte b1           (consumes 2)
 *               b0 >= 0x80: (256-b0) literal bytes from b1
 *     other (0x12 in setup, ...): skipped (size - 6)
 *
 * Ghidra refs (docs/01-executable.md): ticker FUN_0003f6bc, frame reader
 * FUN_0003f80d, sub-chunk dispatch FUN_0003f885, 0x0C patch FUN_0003fb73,
 * 0x0F full frame FUN_0003fa3b.
 */
#include "res.h"
#include <string.h>
#include <stdlib.h>

#define FLI_MARKER 0xAF11

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

int fli_open(const uint8_t *d, uint32_t n, Fli *f) {
    if (n < 128) return 0;
    if (rd16(d + 4) != FLI_MARKER) return 0;
    f->w = rd16(d + 8);
    f->h = rd16(d + 10);
    f->frame_count = rd16(d + 6);
    f->data = d;
    f->size = n;
    f->pos = 128;
    f->pal = calloc(256 * 3, 1);
    f->pal_set = 0;
    if (!f->pal || !f->w || !f->h) return 0;
    return 1;
}

void fli_free(Fli *f) { free(f->pal); f->pal = NULL; }

/* FUN_0003f885 (0x04 sub-chunk): skip 4B, read 768B. The binary shifts the
 * copy it keeps in the DAT_000636b8 working buffer (0x3f96a: SHR byte, 2),
 * but DOSBox captures of the original opening show the DISPLAYED palette is
 * full 0-255 range = raw file bytes, so the port stores them raw. */
static void apply_palette(Fli *f, const uint8_t *bgr) {
    memcpy(f->pal, bgr, 768);
    f->pal_set = 1;
}

static void draw_patch(uint8_t *fb, int w, int h, const uint8_t *p, size_t len) {
    (void)len;
    int start = rd16(p), n = rd16(p + 2);
    p += 4;
    for (int r = 0; r < n && start + r < h; r++) {
        uint8_t *dst = fb + (size_t)(start + r) * w;
        int nseg = *p++;
        for (int s = 0; s < nseg; s++) {
            int skip = *p++;
            int len2 = *p++;
            dst += skip;
            if (len2 < 0x80) {
                memcpy(dst, p, len2);
                p += len2;
                dst += len2;
            } else {
                uint8_t v = *p++;
                for (int i = 0; i < 256 - len2; i++) *dst++ = v;
            }
        }
    }
}

static void draw_full(uint8_t *fb, int w, const uint8_t *p) {
    for (int r = 0; r < 200; r++) {
        uint8_t *dst = fb + (size_t)r * w;
        int nseg = *p++;
        for (int s = 0; s < nseg; s++) {
            int b0 = *p++;
            if (b0 < 0x80) {
                uint8_t v = *p++;
                for (int i = 0; i < b0; i++) *dst++ = v;
            } else {
                int cnt = 256 - b0;
                memcpy(dst, p, cnt);
                p += cnt;
                dst += cnt;
            }
        }
    }
}

/* Process the next 0xF1FA chunk.
 * Returns 1 if a frame (or the setup) chunk was consumed, 0 if no chunk
 * remains. `fb` is fli->w x fli->h palette indices. */
int fli_next(Fli *f, uint8_t *fb) {
    const uint8_t *d = f->data;
    uint32_t n = f->size;
    if (f->pos + 16 > n) return 0;
    uint32_t size = rd32(d + f->pos);
    if (rd16(d + f->pos + 4) != 0xF1FA || size < 16 || (uint64_t)f->pos + size > n) return 0;
    int count = rd16(d + f->pos + 6);
    uint64_t p = f->pos + 16;
    for (int i = 0; i < count; i++) {
        uint32_t ss = rd32(d + p);
        uint16_t st = rd16(d + p + 4);
        const uint8_t *payload = d + p + 6;
        switch (st) {
        case 0x04: apply_palette(f, payload + 4); break;
        case 0x0C: draw_patch(fb, f->w, f->h, payload, ss - 6); break;
        case 0x0F: draw_full(fb, f->w, payload); break;
        default: break;
        }
        p += ss;
    }
    f->pos = (uint32_t)p;
    return 1;
}
