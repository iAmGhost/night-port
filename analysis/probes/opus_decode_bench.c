/* Decode 40 seconds of the same music two ways, in wasm, and say what each costs.

   The question docs/463 left open was whether the stutter on a phone is the decoder. minimp3 in wasm
   costs about 8x what the same C costs on the desktop CPU, so nothing measured natively answers it --
   this program has to run in the same module the game runs in.  It reports microseconds of main-thread
   time per second of audio, which is the only figure that maps onto "does a frame miss its budget".

   Run: node opus_decode_bench.js track.nkop track.mp3     (absolute paths; built with -sNODERAWFS=1)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opus.h"
#include "mp3_stream.h"

#define TARGET_SECONDS 40.0
#define MAX_PACKET 4096
#define MAX_FRAMES 5760            /* one Opus packet is at most 120 ms */
#define FIFO_FRAMES 4096

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

static uint8_t *slurp(const char *p, size_t *len)
{
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "open %s: no\n", p); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc((size_t)n);
    if (fread(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); return NULL; }
    fclose(f);
    *len = (size_t)n;
    return b;
}

static void report(const char *what, double us, double seconds)
{
    printf("%-10s %8.1f s audio in %10.1f ms  ->  %9.1f us per second of audio  ->  %7.0fx realtime\n",
           what, seconds, us / 1e3, us / seconds, seconds * 1e6 / us);
}

/* ---- Opus: the fixed-stride container pack_opus.py writes -------------------------- */
static int bench_opus(const char *path)
{
    size_t len = 0;
    uint8_t *f = slurp(path, &len);
    if (!f) return -1;
    uint32_t version, packet_bytes, frame_samples, rate, channels, packets, head, pad;
    memcpy(&version, f + 4, 4);      memcpy(&packet_bytes, f + 8, 4);
    memcpy(&frame_samples, f + 12, 4); memcpy(&rate, f + 16, 4);
    memcpy(&channels, f + 20, 4);    memcpy(&packets, f + 24, 4);
    memcpy(&head, f + 28, 4);        memcpy(&pad, f + 32, 4);
    (void)version; (void)pad;
    if (memcmp(f, "NKOP", 4) != 0) { fprintf(stderr, "%s: not NKOP\n", path); return -1; }
    printf("opus: %u packets of %uB, %u spf @ %u Hz %u ch, pre_skip=%u\n",
           packets, packet_bytes, frame_samples, rate, channels, head);

    int err = 0;
    OpusDecoder *d = opus_decoder_create((opus_int32)rate, (int)channels, &err);
    if (err != OPUS_OK || !d) { fprintf(stderr, "opus_decoder_create: %d\n", err); return -1; }

    int16_t *pcm = malloc(sizeof(int16_t) * MAX_FRAMES * channels);
    double t0 = now_us();
    uint64_t samples = 0;
    uint32_t used = 0;
    for (uint32_t i = 0; i < packets &&
         (double)samples / ((double)rate * channels) < TARGET_SECONDS; i++) {
        const uint8_t *p = f + 36 + (size_t)i * packet_bytes;
        int n = opus_decode(d, p, (int)packet_bytes, pcm, MAX_FRAMES, 0);
        if (n < 0) { fprintf(stderr, "opus_decode packet %u: %d\n", i, n); return -1; }
        samples += (uint64_t)n * channels;
        used = (uint32_t)n;
    }
    double us = now_us() - t0;
    (void)used; (void)head;
    report("libopus", us, (double)samples / ((double)rate * channels));
    opus_decoder_destroy(d);
    free(pcm);
    free(f);
    return 0;
}

/* ---- MP3: the byte source the port would hand mp3_stream.c ------------------------- */
typedef struct { const uint8_t *b; size_t len; } mem;

static int mem_read(void *user, uint64_t off, void *dst, uint32_t n)
{
    mem *m = user;
    if (off + n > m->len) return -1;
    memcpy(dst, m->b + off, n);
    return 0;
}

static int bench_mp3(const char *path)
{
    size_t len = 0;
    uint8_t *f = slurp(path, &len);
    if (!f) return -1;
    printf("mp3: %zu bytes\n", len);

    static nk_mp3s s;
    static int16_t fifo[FIFO_FRAMES * 2];
    static uint8_t chunk[16384];
    mem m = { f, len };
    if (nk_mp3s_init(&s, mem_read, &m, fifo, FIFO_FRAMES, chunk, sizeof chunk) != 0) {
        fprintf(stderr, "nk_mp3s_init failed\n");
        return -1;
    }
    nk_mp3s_set_delay(&s, 1105);         /* the measured LAME delay on this encode */
    if (nk_mp3s_seek(&s, 0) != 0) { fprintf(stderr, "seek failed\n"); return -1; }

    int16_t *out = malloc(sizeof(int16_t) * FIFO_FRAMES * 2);
    double t0 = now_us();
    uint64_t samples = 0;
    while ((double)samples / (2.0 * s.rate) < TARGET_SECONDS) {
        uint32_t got = nk_mp3s_fill(&s, out, FIFO_FRAMES);
        if (!got) break;
        samples += (uint64_t)got * 2u;
    }
    double us = now_us() - t0;
    report("minimp3", us, (double)samples / (2.0 * (double)s.rate));
    printf("mp3: %u kbps %u Hz, decoded frames=%llu, holes=%llu\n",
           s.bitrate_kbps, s.rate, (unsigned long long)s.frames, (unsigned long long)s.silence_frames);
    nk_mp3s_free(&s);
    free(out);
    free(f);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s track.nkop track.mp3\n", argv[0]); return 2; }
    if (bench_opus(argv[1]) != 0) return 1;
    if (bench_mp3(argv[2]) != 0) return 1;
    return 0;
}
