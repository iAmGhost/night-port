#include "res.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Dependency-free PNG writer: 8-bit grayscale (ct=0) or RGB (ct=2),
 * zlib stream with stored (uncompressed) deflate blocks. */

static uint32_t crc_table[256];
static int crc_ready;

static void crc_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[i] = c;
    }
    crc_ready = 1;
}

static uint32_t adler32_buf(const uint8_t *p, size_t n) {
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < n; i++) {
        a = (a + p[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

static void put_u32be(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}

static void chunk(FILE *f, const char *type, const uint8_t *data, size_t n) {
    put_u32be(f, (uint32_t)n);
    if (!crc_ready) crc_init();
    uint32_t c = 0xFFFFFFFFu;
    for (int i = 0; i < 4; i++) c = crc_table[(c ^ (uint8_t)type[i]) & 0xFF] ^ (c >> 8);
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    fwrite(type, 1, 4, f);
    if (n) fwrite(data, 1, n, f);
    put_u32be(f, c ^ 0xFFFFFFFFu);
}

int png_write_generic(const char *path, int w, int h, const uint8_t *px, int color_type) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "png: cannot write %s\n", path); return -1; }
    if (!crc_ready) crc_init();

    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    fwrite(sig, 1, 8, f);

    uint8_t ihdr[13];
    ihdr[0] = (uint8_t)(w >> 24); ihdr[1] = (uint8_t)(w >> 16);
    ihdr[2] = (uint8_t)(w >> 8);  ihdr[3] = (uint8_t)w;
    ihdr[4] = (uint8_t)(h >> 24); ihdr[5] = (uint8_t)(h >> 16);
    ihdr[6] = (uint8_t)(h >> 8);  ihdr[7] = (uint8_t)h;
    ihdr[8] = 8;
    ihdr[9] = (uint8_t)color_type;
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    chunk(f, "IHDR", ihdr, 13);

    int ch = (color_type == 2) ? 3 : 1;
    size_t stride = (size_t)w * ch;
    size_t rawlen = (stride + 1) * (size_t)h;
    uint8_t *raw = malloc(rawlen ? rawlen : 1);
    if (!raw) { fclose(f); return -1; }
    for (int y = 0; y < h; y++) {
        raw[y * (stride + 1)] = 0; /* filter: none */
        memcpy(raw + y * (stride + 1) + 1, px + (size_t)y * stride, stride);
    }

    /* zlib: 0x78 0x01 + stored blocks + adler32 */
    size_t zlen = 2 + (rawlen / 65535 + 1) * 5 + rawlen + 4;
    uint8_t *z = malloc(zlen);
    if (!z) { free(raw); fclose(f); return -1; }
    size_t zo = 0;
    z[zo++] = 0x78; z[zo++] = 0x01;
    size_t off = 0;
    do {
        size_t bl = rawlen - off;
        int final = (bl <= 65535);
        if (bl > 65535) bl = 65535;
        z[zo++] = (uint8_t)final;
        z[zo++] = (uint8_t)(bl & 0xFF);
        z[zo++] = (uint8_t)(bl >> 8);
        z[zo++] = (uint8_t)(~bl & 0xFF);
        z[zo++] = (uint8_t)((~bl >> 8) & 0xFF);
        if (bl) { memcpy(z + zo, raw + off, bl); zo += bl; }
        off += bl;
        if (final) break;
    } while (off < rawlen || rawlen == 0);
    uint32_t ad = adler32_buf(raw, rawlen);
    z[zo++] = (uint8_t)(ad >> 24); z[zo++] = (uint8_t)(ad >> 16);
    z[zo++] = (uint8_t)(ad >> 8);  z[zo++] = (uint8_t)ad;
    free(raw);
    chunk(f, "IDAT", z, zo);
    free(z);

    chunk(f, "IEND", (const uint8_t *)"", 0);
    fclose(f);
    return 0;
}

int png_write_gray(const char *path, int w, int h, const uint8_t *px) {
    return png_write_generic(path, w, h, px, 0);
}

int png_write_rgb(const char *path, int w, int h, const uint8_t *px) {
    return png_write_generic(path, w, h, px, 2);
}
