/* wav.c — minimal RIFF PCM WAV reader (16-bit, mono/stereo).
 * The CD-DA tracks were extracted to standard PCM WAVs by
 * scripts/cdda2wav.py (44100 Hz, 2 ch, 16-bit, interleaved). */
#include "res.h"
#include <string.h>
#include <stdlib.h>

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int wav_open(const uint8_t *d, size_t n, Wav *w) {
    size_t off;
    memset(w, 0, sizeof(*w));
    if (n < 44 || memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) return 0;
    off = 12;
    while (off + 8 <= n) {
        char id[5] = { 0 };
        memcpy(id, d + off, 4);
        uint32_t sz = rd32(d + off + 4);
        if (off + 8 + sz > n) return 0;
        if (!strcmp(id, "fmt ")) {
            if (sz < 16) return 0;
            uint16_t fmt = (uint16_t)(d[off + 8] | d[off + 9] << 8);
            w->channels = d[off + 10] | d[off + 11] << 8;
            w->rate = (int)(d[off + 12] | (uint32_t)d[off + 13] << 8 | (uint32_t)d[off + 14] << 16 | (uint32_t)d[off + 15] << 24);
            w->bits = d[off + 22] | d[off + 23] << 8;
            if (fmt != 1 || w->bits != 16 || w->channels < 1 || w->channels > 2) return 0;
        } else if (!strcmp(id, "data")) {
            if (sz & 1) return 0;
            w->samples = malloc(sz);
            if (!w->samples) return 0;
            memcpy(w->samples, d + off + 8, sz);
            w->nframes = sz / (w->channels * 2);
            return 1;
        }
        off += 8 + sz + (sz & 1);
    }
    free(w->samples);
    w->samples = NULL;
    return 0;
}

void wav_free(Wav *w) { free(w->samples); w->samples = NULL; }
