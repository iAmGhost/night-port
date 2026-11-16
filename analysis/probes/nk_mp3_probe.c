/* nk_mp3_probe.c -- evidence for the browser port's lossy CD-DA: decode an MP3 with the SAME code the
 * port links (wasm/src/mp3_stream.c), and answer the three questions that design rests on.  docs/463.
 *
 *   1. Is it really constant bitrate? (every frame's bitrate, and `map_err_max`)
 *   2. Is the clock exact enough to seek on? The clock says sample S is at byte
 *      first_byte + S*bytes_per_sec/rate. `map_err_max` is how wrong that is for every frame in the file
 *      -- if it stays inside a byte or two, landing on the estimate and stepping to a header is exact.
 *   3. Does a seek land on the sample it promised? Seek the stream to N and compare against the same
 *      samples taken from a straight decode of the raw Red Book track the MP3 was encoded from.
 *
 *   cc -O2 -I../.. -I../../wasm/src -I../../wasm/third_party/minimp3 -DMINIMP3_IMPLEMENTATION \
 *      -o ../../build/nk_mp3_probe nk_mp3_probe.c ../../wasm/src/mp3_stream.c
 *   ./build/nk_mp3_probe ../extracted/../wasm/dist/data/audio/track03.mp3 --raw ../extracted/cd/track03.bin
 *
 * Host-only: it reads the whole encoded file with stdio, which the port never does.
 */
#include "mp3_stream.h"   /* mp3_stream.c is the translation unit that defines the decoder */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef NK_MP3_TRACE
int nk_mp3_trace = 0;                               /* counts the frames mp3_stream.c traces */
#endif

#ifndef FIFO_FRAMES
#define FIFO_FRAMES 4096u
#endif
#ifndef CHUNK_BYTES
#define CHUNK_BYTES 16384u
#endif

typedef struct { FILE *f; uint64_t len; } src;

static int read_at(void *user, uint64_t off, void *dst, uint32_t n)
{
    src *s = user;
    if (off >= s->len) return -1;
    if (off + n > s->len) n = (uint32_t)(s->len - off);
    if (fseeko(s->f, (off_t)off, SEEK_SET) != 0) return -1;
    return fread(dst, 1, n, s->f) == n ? 0 : -1;
}

/* Raw Red Book samples: the track file is 588 stereo frames per 2352-byte sector. */
static int read_raw(FILE *f, uint64_t frame, int16_t *dst, uint32_t n)
{
    if (fseeko(f, (off_t)(frame * 4ull), SEEK_SET) != 0) return -1;
    return fread(dst, 4, n, f) == n ? 0 : -1;
}

static uint64_t sad(const int16_t *a, const int16_t *b, uint32_t n)   /* sum of |difference| */
{
    uint64_t s = 0;
    for (uint32_t i = 0; i < n * 2u; i++) s += (uint64_t)(a[i] > b[i] ? a[i] - b[i] : b[i] - a[i]);
    return s;
}

int main(int argc, char **argv)
{
    const char *path = 0, *rawpath = 0, *seqpath = 0;
    uint64_t probes[16], rawcap = 0;
    uint32_t nprobe = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--raw") && i + 1 < argc) rawpath = argv[++i];
        else if (!strcmp(argv[i], "--seek") && i + 1 < argc) probes[nprobe++] = strtoull(argv[++i], 0, 10);
        else if (!strcmp(argv[i], "--dump-seq") && i + 1 < argc) seqpath = argv[++i];
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: %s file.mp3 [--raw track.bin] [--seek N]...\n", argv[0]); return 2; }

    src in = { fopen(path, "rb"), 0 };
    if (!in.f) { fprintf(stderr, "cannot read %s\n", path); return 2; }
    fseeko(in.f, 0, SEEK_END);
    in.len = (uint64_t)ftello(in.f);

    /* The raw track, if one was named, is both the cap for the straight decode below and the answer key
     * for the seek tests. */
    FILE *raw = rawpath ? fopen(rawpath, "rb") : 0;
    if (rawpath && !raw) { fprintf(stderr, "cannot read %s\n", rawpath); return 2; }
    if (raw) {
        fseeko(raw, 0, SEEK_END);
        rawcap = (uint64_t)ftello(raw) / 4ull;            /* 4 bytes = one stereo frame */
    }

    static int16_t fifo[FIFO_FRAMES * 2];
    static uint8_t chunk[CHUNK_BYTES];
    static int16_t out[4096 * 2];
    nk_mp3s s;
    if (nk_mp3s_init(&s, read_at, &in, fifo, FIFO_FRAMES, chunk, CHUNK_BYTES) != 0) return 2;

    /* One straight decode of the whole thing: what the port sees when it plays a track to its end. The
     * stream pads a request it cannot serve with silence rather than coming back short (a browser host
     * must not have to reason about a decode that ran out), so "the file is over" is watched for as the
     * first hole, not as a short count. */
    uint64_t produced = 0, first_decode_us = 0, cap = rawcap ? rawcap : ~0ull;
    /* Keep the whole straight decode in hand: it is the answer key for the seek tests, and unlike the raw
     * sectors it is known-good audio for this file (the head test below ties it to the sectors). */
    size_t seqcap = rawcap ? (size_t)rawcap + 4096u : 4000000u;
    int16_t *seq = malloc(seqcap * 2u * sizeof *seq);
    struct timespec ts0, ts1;
    clock_gettime(CLOCK_MONOTONIC, &ts0);
    while (produced < cap && !s.silence_frames && nk_mp3s_fill(&s, out, 588u) == 588u) {
        if (seq && produced + 588u <= seqcap) memcpy(seq + (size_t)produced * 2u, out, 588u * 2u * sizeof *out);
        produced += 588u;
    }
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    first_decode_us = (uint64_t)(ts1.tv_sec - ts0.tv_sec) * 1000000ull +
                      (uint64_t)(ts1.tv_nsec - ts0.tv_nsec) / 1000ull;
    if (seqpath && seq) {                       /* for external comparison tools: the straight decode,
                                        * with the encoder delay still in front of it */
        FILE *o = fopen(seqpath, "wb");
        if (o) { fwrite(seq, 2, (size_t)produced * 2u, o); fclose(o); }
    }
    fclose(in.f);

    printf("%s: rate=%u ch=%u kbps=%u first_byte=%llu frames=%llu samples=%llu bytes=%llu "
           "map_err_max=%llu map_err_avg=%llu.%02llu holes=%llu resyncs=%llu fifo_drop=%llu "
           "decode=%.1fx realtime\n",
           path, s.rate, s.channels, s.bitrate_kbps, (unsigned long long)s.first_byte,
           (unsigned long long)s.frames, (unsigned long long)s.samples, (unsigned long long)s.bytes_read,
           (unsigned long long)s.map_err_max,
           s.map_err_n ? (unsigned long long)(s.map_err_sum / s.map_err_n) : 0ull,
           s.map_err_n ? (s.map_err_sum * 100ull / s.map_err_n) % 100ull : 0ull,
           (unsigned long long)s.silence_frames, (unsigned long long)s.resyncs,
           (unsigned long long)s.fifo_dropped,
           first_decode_us ? (double)produced / s.rate / ((double)first_decode_us / 1000000.0) : 0.0);

    if (!raw) return 0;
    uint64_t raw_frames = rawcap;

    /* Second pass, because the seek tests need a live stream. */
    in.f = fopen(path, "rb");
    nk_mp3s_init(&s, read_at, &in, fifo, FIFO_FRAMES, chunk, CHUNK_BYTES);

    /* (3a) Where does the decoded stream start against the raw one? An encoder pads the head of its
     * output with the state its filterbank needs and only tells a tag-reader about it, so the decoded
     * head is the raw head shifted by that much. Look for the shift that lines the two up, and hand it to
     * the stream as its delay: from then on every position this file prints is a position in the raw
     * track. One decode of the head, then slide the comparison -- a seek per candidate lag would reset
     * the decoder every time and measure the reset instead of the padding. */
    /* The head test slides b across a by up to lag_max frames, so b has to hold the lag plus the head --
     * in stereo, two ints to a frame. The tail test then walks forward past the end of the track in
     * frames of its own, into the same room. */
    static int16_t a[8192 * 2];
    static int16_t b[(6144 + 8192) * 2 * 2];
    const uint64_t lag_max = 6144ull, head_n = 8192ull;
    uint32_t head = (uint32_t)(lag_max + head_n);
    if (read_raw(raw, 0, a, (uint32_t)head_n) == 0 && nk_mp3s_fill(&s, b, head) == head) {
        uint64_t best = 0, bestsad = ~0ull;
        for (uint64_t lag = 0; lag <= lag_max; lag++) {
            uint64_t v = sad(a, b + (size_t)lag * 2u, (uint32_t)head_n);
            if (v < bestsad) { bestsad = v; best = lag; }
        }
        double scale = (double)bestsad / (head_n * 2.0 * 32768.0);
        printf("  head delay=%llu samples (%.1f ms), mean |difference| %.5f of full scale there\n",
               (unsigned long long)best, 1000.0 * (double)best / (s.rate ? s.rate : 44100u), scale);
        nk_mp3s_set_delay(&s, best);              /* positions below are raw-track positions */

        /* (3b) And mid-track: seek to a raw position, and compare against the raw sectors there. This is
         * the analytic seek of nk_mp3s_seek, on the same file, against the samples it is supposed to land
         * on. A seek that was a byte or two off would show up here as loud noise, not as a small number. */
        if (nprobe == 0) {
            probes[0] = raw_frames / 10; probes[1] = raw_frames / 3;
            probes[2] = raw_frames / 2; probes[3] = raw_frames * 2 / 3;
            probes[4] = (raw_frames > 40000ull) ? raw_frames - 20000ull : raw_frames / 2;
            nprobe = 5;
            if (raw_frames < 8192ull + 20000ull) nprobe = 1;
        }
        for (uint32_t i = 0; i < nprobe && i < 16; i++) {
            uint64_t at = probes[i];
            if (at + 4096ull + 2048ull > raw_frames) at = raw_frames - 4096ull - 2048ull;
            if (at < 2048ull) at = 2048ull;
            if (read_raw(raw, at - 2048ull, a, 4096u + 2048u) != 0) continue;
            if (nk_mp3s_seek(&s, at) != 0) { printf("  seek %llu: refused\n", (unsigned long long)at); continue; }
            uint64_t pos = nk_mp3s_pos(&s);
            if (nk_mp3s_fill(&s, b, 4096u) != 4096u) { printf("  seek %llu: short\n", (unsigned long long)at); continue; }
            /* Slide as well as compare: a seek that landed a frame or two off is a different bug from one
             * that decoded the wrong bytes, and the number says which. */
            int64_t bl = 0;
            uint64_t bs = ~0ull;
            for (int64_t lag = -2048; lag <= 2048; lag++) {
                uint64_t v = sad(a + (size_t)(2048 + lag) * 2u, b, 4096u);
                if (v < bs) { bs = v; bl = lag; }
            }
            double sc = (double)bs / (4096.0 * 2.0 * 32768.0);
            /* The same samples, taken from the straight decode: this isolates the seek path from the
             * decoder. The head test above already ties the straight decode to the raw sectors, so if
             * this reads clean while the raw comparison does not, the seek landed somewhere else -- and
             * if both read dirty, the seek decoded the right bytes into the wrong samples. */
            double vs_seq = -1.0;
            if (seq && at + best + 4096ull <= produced)
                vs_seq = (double)sad(seq + (size_t)(at + best) * 2u, b, 4096ull) / (4096.0 * 2.0 * 32768.0);
            printf("  seek to raw sample %llu: pos=%llu (asked %llu), residual lag=%lld (mad %.5f), "
                   "mad at lag 0 %.5f, mad vs sequential %.5f\n", (unsigned long long)at,
                   (unsigned long long)pos, (unsigned long long)at, (long long)bl, sc,
                   (double)sad(a + 2048ull * 2ull, b, 4096ull) / (4096.0 * 2.0 * 32768.0), vs_seq);
        }
    }
    /* (4) Does the file reach the end of the track it was made from?
     *
     * An encoder puts its filterbank's warm-up in front of the music, and a decoder that knows about it
     * throws that padding away -- which takes the same number of samples off the back of the timeline the
     * port reads. So "how much of the track is really in there" is a question worth asking of every file,
     * and asking it here is cheaper than hearing it at the end of a song.
     *
     * The hole counter is the tell, not the samples: twelve of these fourteen tracks fade to silence and
     * end on samples of exactly zero, so a frame of zeros is not the file running out. A frame the stream
     * could not decode is. */
    if (raw) {
        /* Walk into the end from inside the track, rather than seeking straight to it. The stream has
         * just been dragged around by the seek tests, and where a byte cursor left off decides what the
         * next frame finds; a test whose answer depends on that measures the harness. Coming in from
         * behind, with enough decoded material in hand to cross a frame boundary, is the state the game
         * is actually in when a track reaches its end. */
        uint64_t warm = (raw_frames > 7168ull) ? raw_frames - 7168ull : 0ull;
        uint64_t p = raw_frames, slack = 0;
        int ok = (nk_mp3s_seek(&s, warm) == 0);
        for (uint32_t i = 0; ok && i < 2; i++)
            if (nk_mp3s_fill(&s, b, 3584u) != 3584u) ok = 0;
        if (ok) {
            /* Counted after the warm-up: crossing a hole on the way in is not the answer to the question
             * being asked here. */
            uint64_t before = s.silence_frames;
            p = nk_mp3s_pos(&s);                    /* where the warm-up actually left the stream */
            while (s.silence_frames == before && slack < 200000ull) {
                if (nk_mp3s_fill(&s, b, 1152u) != 1152u) break;
                uint64_t np = nk_mp3s_pos(&s);
                if (np <= p) break;                 /* the position stopped moving: it is over */
                p = np;
                if (np > raw_frames && np - raw_frames > slack) slack = np - raw_frames;
            }
        }
        printf("  tail: the file answers for %llu samples past the end of the raw track\n",
               (unsigned long long)slack);
    }

    fclose(raw);
    fclose(in.f);
    free(seq);
    return 0;
}
